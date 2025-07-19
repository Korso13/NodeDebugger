#include "SpawnerFactory.h"
#include "common/nodeDebugger/NodeDebugger.h"
#include "settings/CustomSettingsContainer.h"

void SpawnerFactory::SpawnObjects(const std::string& mapDataFileOverride)
{
	getAllMapData(mapDataFileOverride, true); //loads requested or default map objects file
	info("[SpawnerFactory]Loaded %d meshes and %d Spawnable objects(not counting child objects)", MAP_MESH_DATA.size(), MAP_DATA.size());

	//Ensuring all pre-existing nodes are uniquely named! 
	checkNodeHierarchyNames();
	if(MAP_MESH_DATA.size() > 1)
	{
		std::sort(MAP_MESH_DATA.begin(), MAP_MESH_DATA.end(), [this](MapMeshData& data1, MapMeshData& data2)
			{
				return std::count(data1.node.begin(), data1.node.end(), '|') / 2 < std::count(data2.node.begin(), data2.node.end(), '|') / 2;
			});
	}
	if(MAP_DATA.size() > 1)
	{
		std::sort(MAP_DATA.begin(), MAP_DATA.end(), [this](MapObjectsData& data1, MapObjectsData& data2)
			{
				return std::count(data1.node.begin(), data1.node.end(), '|') / 2 < std::count(data2.node.begin(), data2.node.end(), '|') / 2;
			});
	}

	//spawning objects from MapObjects.json
	for (auto& obj : MAP_DATA)
	{
		if (!m_classesRegistry.contains(obj.className))
			continue;

		auto spawnerRec = getSpawnerRecord(obj.className);
		if (!spawnerRec.m_spawnerFunc)
			continue;

		//spawning new object, registering in m_spawnedEntities, executing onSpawn logic
		auto newEntity = spawnObjectOfClass(spawnerRec, obj, nullptr, false);

		//reparenting object to designated node if needed
		bool foundParentNode = obj.node.empty(); //only need to search node if it's mentioned in obj.node
		if (newEntity->getSpawnableRoot() && !foundParentNode)
		{
			auto parentNode = findNodeByHierarchyAddress(obj.node);
			if (parentNode)
			{
				foundParentNode = true;
				newEntity->getSpawnableRoot()->setParent(parentNode);
			}
		}

		//in case we failed to get getSpawnableRoot() after creating Spawnable (if it's not set in constructor)
		if (!foundParentNode) 
		{
			if (newEntity->getSpawnableRoot() && !obj.node.empty())
			{
				auto parentNode = findNodeByHierarchyAddress(obj.node);
				if (parentNode)
					newEntity->getSpawnableRoot()->setParent(parentNode);
				else
					m_delayedSpawnableParenting.insert({ m_delayedSpawnableParenting.size(), newEntity });
			}
		}
	}

	//Meshes spawner (spawns meshes from mapObjects and registers them with SpawnerTab)
	for (auto& meshData : MAP_MESH_DATA)
	{
		spawnMesh(meshData);
	}
	m_primaryMeshLoadComplete = true;
	//small hack for spawning meshes later if their node hierarchy adress relies on another mesh from MAP_MESH_DATA (should not happen due to prior sorting)
	//same for Spawnables, which had not been parented by node mentioned in their MapObjectData on spawn
	if (!m_delayedMeshSpawn.empty() || !m_delayedSpawnableParenting.empty())
	{
		//m_nodeHierarchySearchLogging = true;
		info("[SpawnerFactory::SpawnObjects] Mesh spawner has %d meshes on delayed spawn and %d delayed Spawnables. Processing...", m_delayedMeshSpawn.size(), m_delayedSpawnableParenting.size());
		for (size_t i = 0; i < 10; i++) //how many iterations to try and spawn delayed meshes and parent Spawnables. 30 should be more than enough
		{
			if (m_delayedMeshSpawn.empty() && m_delayedSpawnableParenting.empty())
				break;

			std::set<int> removalQueue1;
			std::set<int> removalQueue2;
			for (auto& [id, meshRec] : m_delayedMeshSpawn)
			{
				auto result = spawnMesh(meshRec);
				if (result)
					removalQueue1.insert(id);
			}
			std::erase_if(m_delayedMeshSpawn, [&removalQueue1, this](const std::pair<int, MapMeshData>& elem){return removalQueue1.contains(elem.first);});
		
			for (auto& [id, spawned] : m_delayedSpawnableParenting)
			{
				auto& obj = spawned->getMapObjectsData();
				if (spawned->getSpawnableRoot() && !obj.node.empty())
				{
					auto parentNode = findNodeByHierarchyAddress(obj.node);
					if (parentNode)
					{
						spawned->getSpawnableRoot()->setParent(parentNode);
						removalQueue2.insert(id);
					}
				}
			}
			std::erase_if(m_delayedSpawnableParenting, [&removalQueue2, this](const std::pair<int, sptr<Spawnable>>& elem){return removalQueue2.contains(elem.first);});
			m_nodeHierarchySearchLogging = false;
		}
		//if there're still remaining meshes, spawning them on m_view
		if (!m_delayedMeshSpawn.empty())
		{
			//W4_LOG_ERROR("[SpawnerFactory::SpawnObjects] Mesh spawner could not place %d meshes on nodes written in MapObjects.json! Spawning on default node!", m_delayedMeshSpawn.size());
			warn("[SpawnerFactory::SpawnObjects] Mesh spawner could not place %d meshes on nodes written in MapObjects.json! Spawning on default node!", m_delayedMeshSpawn.size());
			for (auto& [id, meshRec] : m_delayedMeshSpawn)
			{
				auto adressHolder = meshRec.node;
				meshRec.node.clear(); //forces m_view as parent
				spawnMesh(meshRec);
				meshRec.node = adressHolder;
			}
			m_delayedMeshSpawn.clear();
		}
	}

	//run it twice in case newly-loaded nodes are poorly-named. Not needed in release versions
	//Possibly not that nessesary...
#ifdef DEBUG
	//checkNodeHierarchyNames();
	ND_SPAWNER->initClickColliders();
#endif // DEBUG
}

std::string SpawnerFactory::getClassesImGuiCombo(const std::string& selectedClass, const std::string& comboHeader)
{
	static std::map<std::string, std::string> classMapForCombo;
	if (classMapForCombo.size() != m_classesRegistry.size())
		for (auto& [className, spawnData] : m_classesRegistry)
			classMapForCombo[className] = className;

	auto comboResult = ND_Util::ImGuiComboList(comboHeader, classMapForCombo, selectedClass);
	if (comboResult.first)
		return comboResult.second;
	else
		return selectedClass;
}

SpawnerFactory::SpawnerRecord& SpawnerFactory::getSpawnerRecord(const std::string& className)
{
	if (m_classesRegistry.contains(className))
		return m_classesRegistry[className];
	else
	{
		//err("No SpawnerRecord for %s found!", className.c_str());
		FATAL_ERROR("No SpawnerRecord for %s found!", className.c_str());
		static SpawnerRecord defaultRec("CLASS_NAME"); //mutes compiler
		return defaultRec;
	}
}

std::function<void(sptr<Spawnable>)> SpawnerFactory::getObjectOnSpawnCallback(sptr<Spawnable> spawnedObject)
{
	if(!spawnedObject)
		return [this](sptr<Spawnable>) {};
	return getClassOnSpawnCallback(spawnedObject->getMapObjectsData().className);
}

std::function<void(sptr<Spawnable>)> SpawnerFactory::getClassOnSpawnCallback(const std::string& className)
{
	if (m_classesRegistry.contains(className))
		return getSpawnerRecord(className).getOnSpawnCallback();
	else
		return [this](sptr<Spawnable>) {};
}

void SpawnerFactory::setDefaultMeshesRoot(sptr<Node> newRoot)
{
	if (!newRoot)
		return;

	m_view = newRoot;
}

std::map<std::string, CommonTypes::MeshesContainerSetting>& SpawnerFactory::getMeshesSettings()
{
	if (m_meshesList.empty())
	{
		//parses json value into MeshesContainerSetting
		auto settingGetter = 
			[=](nlohmann::json value) -> CommonTypes::MeshesContainerSetting
			{
				auto ret = nlohmann::getMeshesContainerSetting(value);
				return ret;
			};
	
		nlohmann::json json;
		if (auto file = filesystem::open("config/meshesContainerSettings.json"); file && file->good())
		{
			json = nlohmann::json::parse(file->data(), file->data() + file->size());
		}
		for (auto& value : json["settings"])
		{
			auto setting = settingGetter(value);
			m_meshesList.emplace(setting.name, setting);
		}
		for (size_t i = 0; i < 10; i++)
		{
			if (!filesystem::isPathExists("config/meshesContainerSettings" + STR(i) + ".json"))
				break;

			nlohmann::json json;
			if (auto file = filesystem::open("config/meshesContainerSettings" + STR(i) + ".json"); file && file->good())
			{
				json = nlohmann::json::parse(file->data(), file->data() + file->size());
			}
			for (auto& value : json["settings"])
			{
				auto setting = settingGetter(value);
				m_meshesList.emplace(setting.name, setting);
			}
		}
	}

	return m_meshesList;
}

std::vector<std::string> SpawnerFactory::getMeshesList()
{
	std::vector<std::string> result;
	for (const auto& [meshSettingName, setting] : m_meshesList)
		result.push_back(meshSettingName);
	return result;
}

std::string SpawnerFactory::checkUniqueName(const std::string& name)
{
	if (!m_spawnedEntities.contains(name))
		return name;

	std::string zeroes = "00"; //for better sorting
	for (size_t i = 0; i < 500; i++)
	{
		if (i == 10 || i == 100)
			zeroes.pop_back();

		if (!m_spawnedEntities.contains(name + "_" + zeroes + std::to_string(i)))
			return name + "_" + zeroes + std::to_string(i);
	}

	return "NoUniqueName!";
}

std::string SpawnerFactory::checkUniqueMeshName(const std::string& name)
{
	if (!m_spawnedMeshes.contains(name))
		return name;

	std::string zeroes = "00"; //for better sorting
	for (size_t i = 0; i < 500; i++)
	{
		if (i == 10 || i == 100)
			zeroes.pop_back();

		if (!m_spawnedMeshes.contains(name + "(" + zeroes + std::to_string(i) + ")"))
			return name + "(" + zeroes + std::to_string(i) + ")";
	}

	return "NoUniqueName!";
}

void SpawnerFactory::updateMeshHierarchyRecords()
{
	for (auto& [name, Spawned] : m_spawnedEntities)
	{
		if(auto spawnedRootParent = Spawned->getSpawnableRoot()->getParent(); spawnedRootParent)
			Spawned->getMapObjectsData().node = getNodeHierarchyAddress(spawnedRootParent);
	}
	for (auto& [name, DataAndMesh] : m_spawnedMeshes)
	{
		DataAndMesh.first.node = getNodeHierarchyAddress(DataAndMesh.second->getParent()->as<Node>());
	}
}

void SpawnerFactory::checkNodeHierarchyNames()
{
	sptr<Node> rootNode = Render::getRoot();
	sptr<Node> hudRootNode = Render::getHudPass().getRoot();

	//cuts traversal 
	auto predicateCheck = [this](Node& node)
		{
			return !node.getChildren().empty();
		};

	auto nodeNameCheck = [this](Node& node)
		{
			auto parentName = node.getName();
			int32_t unnamedChildrenCount = 0;
			std::set<std::string> children;
			for (auto& child : node.getChildren())
			{
				if (!child)
					continue;

				if (child->getName() == "unnamed")
					child->setName(parentName + "_child_" + STR(unnamedChildrenCount++));
				else if (children.contains(child->getName()))
				{
					for (size_t i = 0; i < 500; i++)
					{
						if (!children.contains(child->getName() + "_" + STR(i)))
						{
							child->setName(child->getName() + "_" + STR(i));
							break;
						}
					}
				}

				children.insert(child->getName());
			}
		};

	if (rootNode)
	{
		rootNode->traversal(predicateCheck, nodeNameCheck);
	}
	if (hudRootNode)
	{
		hudRootNode->traversal(predicateCheck, nodeNameCheck);
	}
}

//helper recursive function for SpawnerFactory::getNodeHierarchyAddress()
//writes result into hierarchyAdress string
void recursiveNodeGetter(sptr<Node> node, std::string& hierarchyAdress)
{
	if (!node)
	{
		W4_LOG_ERROR("[SpawnerFactory::getNodeHierarchyAddress->recursiveNodeGetter] Null node!");
		return;
	}
	if (node == Render::getRoot())
	{
		hierarchyAdress.insert(0, "Root");
		return;
	}
	if (node == Render::getHudPass().getRoot())
	{
		hierarchyAdress.insert(0, "HUD Root");
		return;
	}

	if (node->getParent())
	{
		hierarchyAdress.insert(0, "||" + node->getName());
		recursiveNodeGetter(node->getParent(), hierarchyAdress);
	}
}

std::string SpawnerFactory::getNodeHierarchyAddress(sptr<Node> node)
{
	std::string hierarchyAdress = "";
	recursiveNodeGetter(node, hierarchyAdress);

	return hierarchyAdress;
}

sptr<Node> SpawnerFactory::findNodeByHierarchyAddress(std::string adress)
{
	//debug("[SpawnerFactory::findNodeByHierarchyAddress] %s", adress.c_str());
	sptr<Node> foundNode = nullptr;
	if (adress.empty())
		return nullptr;
	do
	{
		if (!foundNode) //first iteration
		{
			std::string firstNodeName;
			if (adress.find("||") != std::string::npos)
			{
				firstNodeName = adress.substr(0, adress.find("||")); //getting sub string up to the next separator
				adress.erase(0, adress.find("||") + 2); //removing found node from the adress along with the separator
			}
			else //adress is just root node
			{
				firstNodeName = adress;
				adress.clear();
			}
			if(m_nodeHierarchySearchLogging)
				info("\nfirstNodeName: %s", firstNodeName.c_str());

			if (firstNodeName == "Root")
				foundNode = Render::getRoot();
			else if (firstNodeName == "HUD Root")
				foundNode = Render::getHudPass().getRoot();
		}
		else
		{
			std::string nextNodeName = adress.substr(0, adress.find("||")); //getting sub string up to the next separator
			if (m_nodeHierarchySearchLogging)
				info("\nnextNodeName: %s", nextNodeName.c_str());
			auto children = foundNode->getChildren();
			auto childIt = children.begin();
			bool bFound = false;
			for (; childIt != children.end(); childIt++)
			{
				if ((*childIt)->getName() == nextNodeName)
				{
					if (m_nodeHierarchySearchLogging)
						warn("Found child %s matching name %s. It's parent %s",
							(*childIt)->getName().c_str(),
							nextNodeName.c_str(), 
							(*childIt)->getParent()->getName().c_str());
					foundNode = *childIt;
					bFound = true;
					break;
				}
			}

			if (bFound)
			{
				if (m_nodeHierarchySearchLogging)
				{
					std::string output = "Children of " + foundNode->getName() + " " + STR(children.size()) + "\n";
					warn("%s", output.c_str());
					for (auto& child : children)
					{
						info("%s", child->getName().c_str());
					}
				}

				adress.erase(0, adress.find("||") + 2); //removing found node from the adress along with the separator
				if (m_nodeHierarchySearchLogging)
					info("\n Adress after iteration: \n%s", adress.c_str());
			}
			else
			{
				if (m_nodeHierarchySearchLogging)
					warn("\nCoundn't find %s", nextNodeName.c_str());
				foundNode = nullptr;
				break;
			}
		}
	} while (adress.find("||") != std::string::npos);
	
	//final iteration
	if (!adress.empty())
	{
		std::string lastNodeName = adress;
		auto children = foundNode->getChildren();
		auto childIt = children.begin();
		bool bFound = false;
		for (; childIt != children.end(); childIt++)
		{
			if ((*childIt)->getName() == lastNodeName)
			{
				foundNode = *childIt;
				bFound = true;
				break;
			}
		}
		if (!bFound)
		{
			if (m_nodeHierarchySearchLogging)
			{
				warn("\nCoundn't find last node: %s", lastNodeName.c_str());
				std::string output = "Children " + STR(children.size()) + "\n";
				for (auto& child : children)
				{
					output = output + "\n" + child->getName();
				}
				warn("%s", output.c_str());
			}
			foundNode = nullptr;
		}
	}

	return foundNode;
}

sptr<Mesh> SpawnerFactory::getMeshByName(const std::string& meshName)
{
	if (m_spawnedMeshes.contains(meshName))
		return m_spawnedMeshes[meshName].second;
	else
		return nullptr;
}

std::vector<sptr<Mesh>> SpawnerFactory::getAllMeshesMatching(const std::string& meshName)
{
	std::vector<sptr<Mesh>> searchResult;
	for (auto& [name, spawnedMesh] : m_spawnedMeshes)
	{
		if (!ND_Util::checkFilterConditions(name, meshName) || !spawnedMesh.second)
			continue;
		
		searchResult.push_back(spawnedMesh.second);
	}

	return searchResult;
}

sptr<Mesh> SpawnerFactory::spawnMesh(MapMeshData& meshData)
{
	//spawn logic
	sptr<Node> parentNode = nullptr;
	if (meshData.node.empty()) //spawned from tab
		parentNode = m_view;
	else
		parentNode = findNodeByHierarchyAddress(meshData.node);

	//nessesary hack for meshes from MapObjects.json if they rely on other mesh being spawned first
	if (!parentNode)
	{
		if(!m_primaryMeshLoadComplete) 
			m_delayedMeshSpawn.insert({ CastI(m_delayedMeshSpawn.size()), meshData });
		return nullptr;
	}

	sptr<Mesh> newMesh = createMesh(meshData.meshName, "", "sprite", true, nullptr); //material replaced with unique instance later
	
	//naming and parenting																				 //naming and parenting
	std::string uniqueName; 
	if(!meshData.meshId.empty())
		uniqueName = checkUniqueMeshName(meshData.meshId);
	else
		uniqueName = checkUniqueMeshName(meshData.meshName);

	if (uniqueName != meshData.meshId)
		meshData.meshId = uniqueName;
	newMesh->setName(uniqueName);
	parentNode->addChild(newMesh, false);

	//making unique material instance
	auto newMatInst = RMat(meshData.material)->createInstance();
	newMatInst->setTexture(TextureId::TEXTURE_0, newMesh->getMaterialInst()->getTexture(TextureId::TEXTURE_0));
	if (meshData.material == "sprite_mirrored" && meshData.isNeedToTurnLeft)
	{
		newMatInst->setParam("mirrorMod", vec2(1, 0));
	}
	newMesh->setMaterialInst(newMatInst);
	newMesh->setRenderOrder(meshData.renderOrder);
	if (meshData.useWorldPos)
		newMesh->setWorldTranslation(meshData.position_world);
	else
		newMesh->setLocalTranslation(meshData.position_local);
	newMesh->setLocalScale(meshData.scale_local);
	newMesh->setLocalRotation(Rotator(meshData.rotation_local));

	//Adds mesh to list of spawned entities
	if(!meshData.isChild)
	{
		m_spawnedMeshes.insert({ uniqueName, {meshData, newMesh} });
		newMesh->subscribeToWorldTransformChanged([this, uniqueName](const Transform& trans)
			{
				if (!m_spawnedMeshes.contains(uniqueName))
					return;

				m_spawnedMeshes[uniqueName].first.position_local = m_spawnedMeshes[uniqueName].second->getLocalTranslation();
				m_spawnedMeshes[uniqueName].first.position_world = trans.translation();
				m_spawnedMeshes[uniqueName].first.scale_local = m_spawnedMeshes[uniqueName].second->getLocalScale();
				m_spawnedMeshes[uniqueName].first.rotation_local = m_spawnedMeshes[uniqueName].second->getLocalRotation().euler();
				m_spawnedMeshes[uniqueName].first.renderOrder = m_spawnedMeshes[uniqueName].second->getRenderOrder();
			});
	}

	ND_NODE(newMesh);

	return newMesh;
}

//const MapObjectsData SpawnerFactory::SpawnerRecord::getMapObjectsData()
//{
//	for (auto& param : this->constructorParams)
//	{
//		if (param.isMapData)
//		{
//			return param.paramValue.get<MapObjectsData>();
//		}
//	}
//
//	//muting compilationError
//	return MapObjectsData();
//}

void SpawnerFactory::SpawnerRecord::setNewMapObjectsData(MapObjectsData& newMOD)
{
	for (auto& param : this->constructorParams)
	{
		if (param.isMapData)
		{
			m_defaultMod = param.paramValue.get<MapObjectsData>();
			return param.paramValue.setValue<MapObjectsData>(newMOD);
		}
	}
}

void SpawnerFactory::SpawnerRecord::resetToDefaultMapObjectsData()
{
	for (auto& param : this->constructorParams)
	{
		if (param.isMapData)
		{
			return param.paramValue.setValue<MapObjectsData>(m_defaultMod);
		}
	}
}

sptr<Node> SpawnerFactory::SpawnerRecord::getContructNode() const
{
	for (const auto& param : constructorParams)
		if (!param.node.expired())
			return param.node.lock();
	return nullptr;
}

void SpawnerFactory::SpawnerRecord::setNewConstructNode(sptr<Node> newNode)
{
	for (auto& param : constructorParams)
	{
		if (!param.node.expired())
		{
			param.paramValue.setValue<sptr<Node>>(newNode);
			param.node = newNode;
		}
	}
}

sptr<Spawnable> SpawnerFactory::spawnObjectOfClass(SpawnerRecord& spawnData, bool bTabSpawned, vec3 spawnAt)
{
	spawnData.resetToDefaultMapObjectsData();
	auto newEntity = spawnData.m_spawnerFunc(spawnData.constructorParams);
	if (!newEntity)
	{
		err("[SpawnerFactory::spawnObjectOfClass]Failed to spawn new object!");
		return nullptr;
	}

	newEntity->isSpawned = bTabSpawned; //marks entity as spawned via ND tab
	
	std::string newEntityName; 
	if(!newEntity->getMapObjectsData().objId.empty())
		newEntityName = checkUniqueName(newEntity->getMapObjectsData().objId);
	else if (newEntity->getSpawnableRoot() && newEntity->getSpawnableRoot()->getName() != "Unnamed")
		newEntityName = checkUniqueName(newEntity->getSpawnableRoot()->getName());
	else if(!newEntity->getMapObjectsData().className.empty())
		newEntityName = checkUniqueName(newEntity->getMapObjectsData().className);
	if (newEntityName.empty()) newEntityName = checkUniqueName("Unnamed"); //must have unique, non-empty name!!
	newEntity->getMapObjectsData().objId = newEntityName;
	m_spawnedEntities.insert(
		{
			newEntityName,
			newEntity
		}
	);

	if (bTabSpawned)
	{
		ND_NODE(newEntity->getSpawnableRoot());
		if(spawnAt == INVALIDPOS)
		{
			vec3 cameraPos = Render::getScreenCamera()->getWorldTranslation();
			cameraPos.z = 0.f;
			if(newEntity->getSpawnableRoot())
				newEntity->getSpawnableRoot()->setWorldTranslation(cameraPos);
			spawnAt = cameraPos;
		}
		else if (newEntity->getSpawnableRoot())
			newEntity->getSpawnableRoot()->setWorldTranslation(spawnAt);
	}

	newEntity->onBeginPlay(newEntity, bTabSpawned, (bTabSpawned ? spawnAt : newEntity->getMapObjectsData().pos));
	if (spawnData.m_onSpawnCb)
		spawnData.m_onSpawnCb(newEntity);

	m_lastSpawnedObjectName = newEntityName;
#ifndef FORCENODEDEBUG_OFF 
	ND_SPAWNER->m_needUpdateEntitiesFilter = true;
#endif

	return newEntity;
}

sptr<Spawnable> SpawnerFactory::spawnObjectOfClass(SpawnerRecord& spawnData, MapObjectsData& customMOD, std::function<void(sptr<Spawnable>)> customOnSpawnCb, bool bTabSpawned)
{
	//Customizing Spawn's MOD
	spawnData.setNewMapObjectsData(customMOD);

	//spawning new object
	auto newEntity = spawnData.m_spawnerFunc(spawnData.constructorParams);
	if (!newEntity)
	{
		err("[SpawnerFactory::spawnObjectOfClass]Failed to spawn new object!");
		return nullptr;
	}
	newEntity->isSpawned = bTabSpawned; //marks object as spawned via ND tab
	std::string newEntityName;

	//naming new object
	if (!newEntity->getMapObjectsData().objId.empty())
		newEntityName = checkUniqueName(newEntity->getMapObjectsData().objId);
	else if (newEntity->getSpawnableRoot() && newEntity->getSpawnableRoot()->getName() != "Unnamed")
		newEntityName = checkUniqueName(newEntity->getSpawnableRoot()->getName());
	else if (!newEntity->getMapObjectsData().className.empty())
		newEntityName = checkUniqueName(newEntity->getMapObjectsData().className);
	if (newEntityName.empty()) newEntityName = checkUniqueName("Unnamed"); //must have unique, non-empty name!!
	newEntity->getMapObjectsData().objId = newEntityName;

	//restoring default MOD
	spawnData.resetToDefaultMapObjectsData();

	m_spawnedEntities.insert(
		{
			newEntityName,
			newEntity
		}
	);
	m_lastSpawnedObjectName = newEntityName;

	//placing new object in front of camera or at set coordinates
	vec3 cameraPos = Render::getScreenCamera()->getWorldTranslation();
	cameraPos.z = 0.f;
	if (newEntity->getSpawnableRoot() && bTabSpawned)
	{
		ND_NODE(newEntity->getSpawnableRoot());
		if(newEntity->getMapObjectsData().pos == vec3::zero)
		{
			newEntity->getSpawnableRoot()->setWorldTranslation(cameraPos);
		}
		else
			newEntity->getSpawnableRoot()->setWorldTranslation(newEntity->getMapObjectsData().pos);
	}

	newEntity->onBeginPlay(newEntity, bTabSpawned, (bTabSpawned ? cameraPos : newEntity->getMapObjectsData().pos));
	
	//executing onSpawn logic
	if (customOnSpawnCb)
		customOnSpawnCb(newEntity);
	else if (spawnData.m_onSpawnCb)
		spawnData.m_onSpawnCb(newEntity);

#ifndef FORCENODEDEBUG_OFF 
	ND_SPAWNER->m_needUpdateEntitiesFilter = true;
#endif

	return newEntity;
}

void SpawnerFactory::removeObject(const std::string& objId)
{
	if (!SPAWNER.getAllSpawnables().contains(objId))
		return;

	auto& spawnableForDeletion = getAllSpawnables()[objId];

	if(spawnableForDeletion->m_childrenAdderEnabled)
	{
		auto childrenArrayCopy = spawnableForDeletion->getSpawnedChildren();
		for (auto& childHolder : childrenArrayCopy)
		{
			if (!childHolder.child.expired())
				removeObject(childHolder.child.lock()->getMapObjectsData().objId);
		}
	}

	//debug("[SpawnerFactory::removeObject]Deleting %s of class %s", spawnableForDeletion->getMapObjectsData().objId.c_str(), spawnableForDeletion->getMapObjectsData().className.c_str());
	spawnableForDeletion->onEndPlay(spawnableForDeletion, objId);
	if (getClassRegistry().contains(spawnableForDeletion->getMapObjectsData().className))
	{
		auto& spawnRecord = getClassRegistry()[spawnableForDeletion->getMapObjectsData().className];
		if (spawnRecord.m_onDeleteCb)
			spawnRecord.m_onDeleteCb(spawnableForDeletion);
	}
	//removing the Spawnable from the Spawner's list of entities
	getAllSpawnables().erase(objId);
	ND_SPAWNER->m_needUpdateEntitiesFilter = true;
}

void SpawnerFactory::purgeSpawnedObjects()
{
	m_spawnedEntities.clear();
}

void SpawnerFactory::purgeSpawnedMeshes()
{
	m_spawnedMeshes.clear();
}

MapObjectsContainer& SpawnerFactory::getAllMapData(const std::string& mapDataFileOverride, bool forceReloadMapData)
{
	if ((mapData.mapObjectsData.empty() && mapData.mapMeshData.empty()) || forceReloadMapData)
	{
		mapData.mapMeshData.clear();
		mapData.mapObjectsData.clear();
		if (!mapDataFileOverride.empty())
		{
			m_mapDataFile = mapDataFileOverride;
			if (m_mapDataFile.find(".json") == std::string::npos)
				m_mapDataFile += ".json";
		}
		auto file = w4::filesystem::open("config/" + m_mapDataFile);
		auto json = nlohmann::json::parse(file->data(), file->data() + file->size());
		nlohmann::adl_serializer<MapObjectsContainer>::from_json(json, mapData);
		W4_LOG_DEBUG("Loaded {%d} MapObjects", mapData.mapObjectsData.size());
	}

	return mapData;
}

template<typename className>
inline SpawnerFactory::SpawnerRecord& SpawnerFactory::registerSpawnableClass(const std::string& entityName)
{
	std::string classID = entityName;
	if (!m_classesRegistry.contains(classID))
	{
		//method to be used for spawning the spawnable entity
		//m_classesRegistry[classID] = SpawnerRecord(classID);
		m_classesRegistry.emplace(classID, SpawnerRecord(classID));
		m_classesRegistry[classID].m_spawnerFunc =
			[this](std::vector<SpawnerRecord::ParamPack> args)
			{
				return cast_sptr<Spawnable>(std::shared_ptr<className>(new className()));
			};
	}
	return m_classesRegistry[classID];
}

SpawnerFactory::SpawnerRecord::SpawnerRecord()
{
	warn("[SpawnerRecord::SpawnerRecord] Using default constructor! Should avoid that if possible!");
	constructorParams.reserve(15);
	m_argsOnGui.reserve(20);
}

SpawnerFactory::SpawnerRecord::SpawnerRecord(const std::string& className)
	:
	m_classID(className)
{
	constructorParams.reserve(15);
	m_argsOnGui.reserve(20);
}

SpawnerFactory::SpawnerRecord& SpawnerFactory::SpawnerRecord::operator=(const SpawnerRecord& rhv)
{
	if (&rhv == this)
		return *this;

	constructorParams.reserve(15);
	m_argsOnGui.reserve(20);
	return *this;
}

SpawnerFactory::SpawnerRecord& SpawnerFactory::SpawnerRecord::setOnSpawnCallback(std::function<void(sptr<Spawnable>)>&& cb)
{
	m_onSpawnCb = cb;
	return *this;
}

SpawnerFactory::SpawnerRecord& SpawnerFactory::SpawnerRecord::setOnDeleteCallback(std::function<void(sptr<Spawnable>)>&& cb)
{
	m_onDeleteCb = cb;
	return *this;
}

SpawnerFactory::SpawnerRecord& SpawnerFactory::SpawnerRecord::setObjNameList(std::vector<std::string>&& listOfNames)
{
#ifndef FORCENODEDEBUG_OFF
	for(auto& value : listOfNames)
		m_listOfMODObjNames[value] = value; //need to convert std::vector to map for ND_Util::ImGuiComboList()

	//searching for MapObjectData(MOD) constructor parameter
	size_t paramIndex = -1;
	for (ParamPack& param : constructorParams)
	{
		paramIndex++;

		if (!param.isMapData)
			continue;

		ParamPack& MODParamPack = param;
		auto ImGuiRenderHolder = m_argsOnGui[paramIndex];
		m_argsOnGui[paramIndex] = [this, paramIndex, ImGuiRenderHolder]()
			{
				auto& MODref = constructorParams[paramIndex].paramValue.getRef<MapObjectsData>(); //MapObjectData(MOD) construct param reference
				//rendering Combo list of possible objName values
				std::pair<bool, std::string> result = ND_Util::ImGuiComboList("ObjName of " + MODref.className, m_listOfMODObjNames, MODref.objName);
				
				//checking if new value was selected from ComboList
				if (result.first)
					constructorParams[paramIndex].paramValue.getRef<MapObjectsData>().objName = result.second;

				ImGuiRenderHolder(); //render the rest of MOD struct as usual;
			};
	}
#endif

	return *this;
}

SpawnerFactory::SpawnerRecord& SpawnerFactory::SpawnerRecord::enableObjNameTextureSelector()
{
#ifndef FORCENODEDEBUG_OFF
	//searching for MapObjectData(MOD) constructor parameter
	size_t paramIndex = -1;
	for (ParamPack& param : constructorParams)
	{
		paramIndex++;

		if (!param.isMapData)
			continue;

		ParamPack& MODParamPack = param;
		//redering image selector before MOD
		m_argsOnGui.emplace_back( 
				[this, paramIndex]()
				{
					auto& MODref = constructorParams[paramIndex].paramValue.getRef<MapObjectsData>(); //MapObjectData(MOD) construct param reference
					//rendering Combo list of possible objName values
					if (ND_Util::ImGuiTextureSelector(MODref.objName, "ObjName selector"))
					{
						//constructorParams[paramIndex].paramValue.getRef<MapObjectsData>().objName = MODref.objName;
					}
				}		
			);
	}
#endif

	return *this;
}
