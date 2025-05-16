#include "Spawnable.h"
#include "settings/JsonDeserializator.h"
#include "SpawnerFactory.h"
#include <common/nodeDebugger/NodeDebugger.h>

std::array<vec3, 2> Spawnable::getSpawnableBounds(sptr<Node> spawnableRoot)
{
	if (!spawnableRoot)
		return m_cachedBounds;

	if(m_cachedBounds != std::array<w4::math::vec3, 2>())
		return m_cachedBounds;

	spawnableRoot->traversal([this](Node& node)
		{
			if (node.is<Mesh>())
			{
				auto mesh = node.as<Mesh>();
				vec3 meshSize;
				meshSize.x =
					neAbs(mesh->getFirstSurface().getGeometryIndexedIterator()->pos.x) //plane half height (not scale adjusted!)
					* mesh->getWorldScale().x 
					* 2.f;
				meshSize.y =
					neAbs(mesh->getFirstSurface().getGeometryIndexedIterator()->pos.y) //plane half height (not scale adjusted!)
					*  mesh->getWorldScale().y
					* 2.f;
				meshSize.z = 0.01;
				if (meshSize.length() > (m_cachedBounds[0] - m_cachedBounds[1]).length())
				{
					m_cachedBounds[0] = node.getWorldTranslation() - meshSize / 2.f;
					m_cachedBounds[1] = node.getWorldTranslation() + meshSize / 2.f;
				}
			}
			else if (node.is<Spine>())
			{

				auto spine = node.as<Spine>();
				auto& skeleton = spine->skeleton;
				auto data = spine->skeleton->getData();

				auto spineSize = vec3{ data->getWidth(), data->getHeight(), 0.01 } / 1024.f * spine->getLocalScale();  // normalize to world size
				spineSize.z = 0.01;
				if (spineSize.length() > (m_cachedBounds[0] - m_cachedBounds[1]).length())
				{
					m_cachedBounds[0] = node.getWorldTranslation() - spineSize*vec3(0.5, 0.f, 1.f);
					m_cachedBounds[1] = node.getWorldTranslation() + spineSize * vec3(0.5, 1.f, 1.f);
				}
			}
		});

	//in case we found nothing during traversal
	if (m_cachedBounds == std::array<vec3, 2>())
	{
		auto pos = spawnableRoot->getWorldTranslation();
		return { pos - vec3(0.5, 0.5, 0.01), pos + vec3(0.5, 0.5, 0.01) };
	}

	return m_cachedBounds;
}

void Spawnable::removeFocusFrame()
{
	if (m_framePlotter) Render::getRoot()->removeChild(m_framePlotter);
};

void Spawnable::initSpawnableCollider(std::function<void(const CollisionInfo&)> clickCb)
{
#ifdef DEBUG
	if (!m_spRoot)
	{
		warn("[Spawnable::initSpawnableCollider] Invalid root {%s}", m_mapObjectData.objId.c_str());
		return;
	}

	auto bounds = getSpawnableBounds(m_spRoot);
	vec3 colliderSize = vec3::zero;
	colliderSize.x = std::max(0.3f, bounds[1].x - bounds[0].x);
	colliderSize.y = std::max(0.3f, bounds[1].y - bounds[0].y);
	colliderSize.z = 0.01;

	m_spawnableSelectCollider = m_spRoot->addCollider<AABB>("spawner_collider", colliderSize);
	m_spawnableSelectCollider->setBlockRaycasts(false);
	m_spawnableSelectCollider->setBlockScreencasts(false);
	m_spawnableSelectCollider->setReceiveRaycasts(true);
	m_spawnableSelectCollider->setRaycastCallback(clickCb);
	m_spawnableSelectCollider->setScreencastCallback(ScreencastEvent::Down, clickCb);
	m_spawnableSelectCollider->setLocalTranslation(vec3(0.f, 0.f, -0.01));

	if (SB("debug_drawColliders"))
	{
		auto colliderDebug = createColliderMesh(m_spawnableSelectCollider, color::Red);
		m_spRoot->addChild(colliderDebug);
		colliderDebug->setLocalTranslation(m_spawnableSelectCollider->getLocalTranslation());
		colliderDebug->setRenderOrder(9);
		ND_NODE(colliderDebug, m_mapObjectData.objId + "_collider");
	}
#endif // DEBUG
}

Spawnable::~Spawnable()
{
	if (m_framePlotter) 
		Render::getRoot()->removeChild(m_framePlotter); 
	m_spRoot->removeAllChildren();
	m_spRoot = nullptr;
}

//Controls Spawnable's ImGui menu render (Root node transform menu, m_spawnableSettings onGui calls and etc)
bool Spawnable::onGui(const char* label)
{
	if (!m_spRoot && m_spawnableSettings.empty())
		return false;

	bool bNeedsDeletion = false; //used to communicate to SpawnerTab that the record is to be deleted via return true

#ifdef DEBUG
		ImGui::Text("%s%s", label, " properties");
		if (ImGui::Button(("DELETE " + STR(label)).c_str()))
		{
			bNeedsDeletion = true;
		}

		//focus frame render
		if (m_framePlotter) Render::getRoot()->removeChild(m_framePlotter);
		if(getSpawnableRoot())
		{
			m_framePlotter = w4::render::Plotter::buildCube(getSpawnableBounds(getSpawnableRoot()), vec4(1.f, 0.f, 0.f, 1.f));
			m_framePlotter->setWorldRotation(getSpawnableRoot()->getWorldRotation());
			Render::getRoot()->addChild(m_framePlotter);
		}

		if (m_spRoot)
		{
			//Button for focusing on the object with camera
			if (ImGui::Button(("Focus on " + m_spRoot->getName()).c_str()))
			{
				vec3 camPos = Render::getScreenCamera()->getWorldTranslation();
				auto bounds = getSpawnableBounds(getSpawnableRoot());
				vec3 centerPos = bounds[0] + (bounds[1] - bounds[0]);
				camPos.x = centerPos.x; camPos.y = centerPos.y;
				Render::getScreenCamera()->setWorldTranslation(camPos);
			};

			//Root node transforms
			if (ImGui::TreeNode("Root Node Transforms"))
			{
				ND_Util::ImGuiNodeRender(m_spRoot, m_spRoot->getName());

				ImGui::TreePop();
			}
			//Root node reparenting
			if (ImGui::TreeNode("Root Node Parent"))
			{
				auto parentNode = m_spRoot->getParent();
				if (ND_Util::ImGuiNodeHierarchy(parentNode, "Parent Node")) //returns true if new node was selected
				{
					m_spRoot->setParent(parentNode);
				}

				ImGui::TreePop();
			}
		}

		//Render for exposed class fields if any
		for (auto& [settingName, Setting] : m_spawnableSettings)
		{
			auto guiFunc = Setting.getOnGuiCb(); //function for rendeing setting in ImGui
			if (guiFunc)
			{
				if (guiFunc((settingName.empty() ? STR(repeatElemId) : settingName), Setting.settingValue)) //checking if setting was changed
				{
					Setting.onSettingChangeCB(Setting.settingValue);
				}
			}

		}

		for (auto& [btnName, btnCall] : m_spawnableBtns)
		{
			if (btnCall)
			{
				if (ImGui::Button(btnName.c_str()))
				{
					auto tempBtnCb = btnCall;
					tempBtnCb();
					break;
				}
			}
		}

		if(m_childrenAdderEnabled)
		{
			ImGui::TextColored(IMG_ORANGE, "%s", (getSpawnableRoot()->getName() + "'s Children").c_str());

			if (ImGui::BeginTabBar((getSpawnableRoot()->getName() + "'s Children Bar").c_str()))
			{
				childSpawnerUI();
				childMeshSpawnerUI();
			}
			ImGui::EndTabBar();
		}
	
#endif

	return bNeedsDeletion;
}

MapObjectsData& Spawnable::getMapObjectsData()
{
	//updating m_mapObjectData.childObjects via visiter since our sptrs are separated from children's MapObjectsData
	std::function<void(Spawnable&)> visiter = [](Spawnable& parent)
		{
			for (size_t i = 0; i < parent.m_children.size(); i++)
			{
				if (parent.m_mapObjectData.childObjects.size() > i && parent.m_children[i].bSpawned && !parent.m_children[i].child.expired())
				{
					*parent.m_mapObjectData.childObjects[i] = parent.m_children[i].child.lock()->m_mapObjectData;
				}
			}
		};

	std::function<void(SpawnableChildMesh&)> visiterMesh = [](SpawnableChildMesh& meshHolder) {
			auto child = meshHolder.child.lock();
			meshHolder.mmd.node = SPAWNER.getNodeHierarchyAddress(child->getParent());
			meshHolder.mmd.renderOrder = child->getRenderOrder();
			meshHolder.mmd.useWorldPos = false;
			meshHolder.mmd.position_local = child->getLocalTranslation();
			meshHolder.mmd.scale_local = child->getLocalScale();
			meshHolder.mmd.rotation_local = child->getLocalRotation().euler();
		};

	childrenVisiter(visiter);
	childrenVisiter(visiterMesh);

	return m_mapObjectData;
}

void Spawnable::childrenVisiter(const std::function<void(Spawnable&)>& visiterFunction)
{
	if (!visiterFunction)
		return;

	std::erase_if(m_children, [this](const SpawnableChild& childHolder)
		{
			return childHolder.child.expired() || !childHolder.bSpawned;
		});

	for (auto& childHolder : m_children)
	{
		if (childHolder.bSpawned && !childHolder.child.expired())
		{
			childHolder.child.lock()->childrenVisiter(visiterFunction);
		}
	}
	visiterFunction(*this);
}

void Spawnable::childrenVisiter(const std::function<void(SpawnableChildMesh&)>& visiterFunction)
{
	if (!visiterFunction)
		return;

	std::erase_if(m_childMeshes, [this](const SpawnableChildMesh& meshHolder)
		{
			return meshHolder.child.expired() || !meshHolder.bSpawned;
		});

	//todo: add mesh node hierarcy traversal?
	for (auto& childHolder : m_childMeshes)
	{
		if (childHolder.bSpawned && !childHolder.child.expired())
		{
			visiterFunction(childHolder);
		}
	}
}

void Spawnable::initSpawnable(sptr<Node> rootNode)
{
	if (!rootNode)
		return;

	if (m_spRoot)
		m_spRoot->unsubscribeFromWorldTransformChanged(m_rootNodeTransformSubsHandler);

	m_spRoot = rootNode;
	m_rootNodeTransformSubsHandler = m_spRoot->subscribeToWorldTransformChanged([this](const Transform& trans)
		{
			m_mapObjectData.pos = trans.translation();
			m_mapObjectData.pos.z = 0.f; //sometimes the gizmo puts in double values into Z field breaking ROs. Disable if you need non-zero Z placement
			m_cachedBounds = std::array<w4::math::vec3, 2>(); //clearing plotter frame pos cache
		});
}

void Spawnable::addChildSpawner(std::function<void(sptr<Spawnable>)> onChildSpawnCb, bool childrenSpawnerOn)
{
	m_childrenAdderEnabled = childrenSpawnerOn; 
	m_onChildSpawnCb = onChildSpawnCb;
}

void Spawnable::spawnAllChildren()
{
	for (auto& childMod : m_mapObjectData.childObjects)
	{
		SpawnableChild childHolder = SpawnableChild(*childMod);
		m_children.push_back(childHolder);

		spawnChild(childMod, m_children.back(), false);
	}
	for (auto& childMmd : m_mapObjectData.childMeshes)
	{
		SpawnableChildMesh childHolder = SpawnableChildMesh(*childMmd);
		m_childMeshes.push_back(childHolder);

		spawnChildMesh(childMmd, m_childMeshes.back());
	}
}

sptr<Mesh> Spawnable::getChildMeshByName(const std::string& name) const
{
	for (auto& rec : m_childMeshes)
	{
		if (rec.mmd.meshId == name)
			return rec.child.lock();
	}

	err("Failed to find child mesh %s for %s", name.c_str(), m_mapObjectData.objId.c_str());
	return nullptr;
}

void Spawnable::childSpawnerUI()
{
#ifdef DEBUG

	if (ImGui::BeginTabItem("Child Objects"))
	{
		size_t count = 0;
		bool changed = false;
		std::function<void()> deferredDelete = nullptr;

		for (auto& childMOD : m_mapObjectData.childObjects)
		{
			if (!childMOD)
			{
				count++;
				continue;
			}

			if (ImGui::TreeNode(("Child_" + childMOD->className + "_" + STR(count)).c_str()))
			{
				if(m_children.size() > count) //m_children.size() should be ==  m_mapObjectData.childObjects.size(). Just making sure everything is ok
				{
					if (!m_children[count].bSpawned)
					{
						//Rendering MOD struct from m_mapObjectData.childObjects
						changed = changed || ImGui::Auto(("Child_" + STR(count) + "_MOD").c_str(), *childMOD);
						//"Spawn" button for unspawned Children
						if (ImGui::Button(("Spawn Child_" + childMOD->className + "_" + STR(count)).c_str()))
						{
							spawnChild(childMOD, m_children[count], true);
						}
					}
					else if (!m_children[count].child.expired())
					{
						//Rendering MOD struct from child itself
						changed = changed || ImGui::Auto(("Child_" + STR(count) + "_MOD").c_str(), m_children[count].mod);
						//Spawned child's settings (same as opeing it up in Spawned Objects subtab)
						m_children[count].child.lock()->onGui(("Child_" + STR(count) + "_Settings").c_str());
					}
				}
				else //falback logic
					changed = changed || ImGui::Auto(("Child_" + STR(count) + "_MOD").c_str(), *childMOD);

				//"Delete" child
				if (ImGui::Button(("Delete Child_" + childMOD->className + "_" + STR(count)).c_str()))
				{
					if (m_children.size() > count && !m_children[count].bSpawned)
					{
						deferredDelete = [this, count]()
							{
								m_mapObjectData.childObjects.erase(m_mapObjectData.childObjects.begin() + count);
								m_children.erase((m_children.begin() + count));
							};
					}
					else if (m_children.size() > count && m_children[count].bSpawned)
					{
						deferredDelete = [this, count]()
							{
								auto deleteCB = SPAWNER.getSpawnerRecord(m_mapObjectData.childObjects[count]->className).getOnDeleteCallback();
								if (deleteCB) deleteCB(m_children[count].child.lock());
								m_mapObjectData.childObjects.erase(m_mapObjectData.childObjects.begin() + count);
								m_children.erase(m_children.begin() + count);
							};
					}
				}

				ImGui::TreePop();
			}

			count++;
		}

		ImGui::Separator();
		ImGui::Separator();
		//Child class selector
		static std::string childClass = "CLASS_NAME";
		childClass = SPAWNER.getClassesImGuiCombo(childClass, "Child of Class");
		
		if (ImGui::Button(("Add " + childClass + " child").c_str()))
		{
			//adding new child
			sptr<MapObjectsData> newChild = make::sptr<MapObjectsData>();
			newChild->className = childClass;
			newChild->isChildObj = true;
			m_mapObjectData.childObjects.push_back(newChild);
			m_children.push_back(SpawnableChild(*m_mapObjectData.childObjects.back()));
		}

		if (deferredDelete)
			deferredDelete();

		ImGui::EndTabItem();
	}

#endif
}

std::string Spawnable::getNewMeshIdSuffix(const std::string& meshName)
{
	std::string zeroes = "00"; //for better sorting
	size_t meshInstances = 0;
	std::for_each(m_childMeshes.begin(), m_childMeshes.end(),
		[&meshName, &meshInstances](SpawnableChildMesh& rec)
		{
			if (rec.mmd.meshName == meshName)
				meshInstances++;
		});
	if (meshInstances > 99)
		zeroes.clear();
	else if (meshInstances > 9)
		zeroes.pop_back();

	return zeroes + std::to_string(meshInstances);
}

Spawnable::SpawnableChildMesh& Spawnable::getChildMeshHolder(const std::string& name)
{
	for (auto& rec : m_childMeshes)
	{
		if (rec.mmd.meshId == name)
			return rec;
	}

	err("Failed to find child mesh record %s for %s", name.c_str(), m_mapObjectData.objId.c_str());
	static Spawnable::SpawnableChildMesh phValue;
	return phValue;
}

void Spawnable::childMeshSpawnerUI()
{
#ifdef DEBUG
	if (ImGui::BeginTabItem("Child Meshes"))
	{
		//texture selector
		ND_Util::ImGuiTextureSelector(m_lastSelectedTextureForMesh, "<-New Mesh");
		//parent node selector
		if (m_lastSelectedParentNode.expired())
			m_lastSelectedParentNode = getSpawnableRoot();
		auto resultHolder = m_lastSelectedParentNode.lock();
		if (ND_Util::ImGuiNodeHierarchy(resultHolder, "Parent node"))
			m_lastSelectedParentNode = resultHolder;

		//Adds mesh record
		if (ImGui::Button("Add new child mesh"))
		{
			//creates MapMeshData (used in export) and SpawnableChildMesh(holds mesh, its setting)
			m_mapObjectData.childMeshes.push_back(make::sptr<MapMeshData>());
			MapMeshData& mmd = *m_mapObjectData.childMeshes.back();
			mmd.node = SPAWNER.getNodeHierarchyAddress(m_lastSelectedParentNode.lock());
			mmd.isChild = true;
			mmd.useWorldPos = false;
			mmd.position_local = vec3::zero;
			mmd.meshName = m_lastSelectedTextureForMesh;
			mmd.renderOrder = 10000; //because life's too short to search for a mesh lost in low render orders!
			mmd.meshId = m_mapObjectData.objId + "_child_" + mmd.meshName + "_" + getNewMeshIdSuffix(mmd.meshName);
			SpawnableChildMesh childMeshHolder = SpawnableChildMesh(mmd);
			//spawns actual mesh
			spawnChildMesh(m_mapObjectData.childMeshes.back(), childMeshHolder);
			m_childMeshes.push_back(childMeshHolder);
		}
		ImGui::Separator();

		//list box with child meshes
		if (m_childMeshes.empty())
		{
			ImGui::NewLine();
			ImGui::EndTabItem();
			return;
		}

		ImGui::TextColored(IMG_WHITE, "%s", (getSpawnableRoot()->getName() + " child meshes:").c_str());
		//if (m_lastSelectedChildMesh.empty()) m_lastSelectedChildMesh = childList.begin()->first;
		auto selectionResult = ND_Util::ImGuiListBox<SpawnableChildMesh>(
			"Child Meshes", 
			m_childMeshes, 
			m_lastSelectedChildMesh, 
			[this](const SpawnableChildMesh& rec) 
			{return rec.mmd.meshId; }
		);
		m_lastSelectedChildMesh = selectionResult.second.mmd.meshId;

		if(m_lastSelectedChildMesh.empty())
		{
			ImGui::NewLine();
			ImGui::EndTabItem();
			return;
		}

		//form for selected child mesh
		if (ImGui::BeginChild(selectionResult.second.mmd.meshId.c_str(), ND_Util::getSizeForChild(0.5), true, ImGuiWindowFlags_AlwaysAutoResize))
		{
			SpawnableChildMesh& meshRec = getChildMeshHolder(m_lastSelectedChildMesh);
			bool bNeedToDeleteSelected = false;

			//delete mesh
			if (ImGui::Button(CSTR("DELETE " + meshRec.mmd.meshId)))
			{
				bNeedToDeleteSelected = true;
			}

			//spawned mesh UI
			else if(!meshRec.child.expired())
			{
				auto spawnedMesh = meshRec.child.lock();

				//btn to focus camera on child mesh
				if (ImGui::Button(("Focus on " + m_lastSelectedChildMesh).c_str()))
				{
					vec3 camPos = Render::getScreenCamera()->getWorldTranslation();
					vec3 centerPos = spawnedMesh->getWorldTranslation();
					camPos.x = centerPos.x; camPos.y = centerPos.y;
					Render::getScreenCamera()->setWorldTranslation(camPos);
				};
				//material selector
				auto materialSelectorResult = ND_Util::ImGuiMaterialSelector(meshRec.mmd.material);
				if (materialSelectorResult.first)
				{
					meshRec.mmd.material = materialSelectorResult.second;
					auto newMat = RMat(meshRec.mmd.material)->createInstance();
					newMat->setTexture(TextureId::TEXTURE_0, spawnedMesh->getMaterialInst()->getTexture(TextureId::TEXTURE_0));
					spawnedMesh->setMaterialInst(newMat);
				}

				//Render order (exportable)
				//if (ImGui::Auto("Render Order", meshRec.mmd.renderOrder))
				//	spawnedMesh->setRenderOrder(meshRec.mmd.renderOrder);
				
				//which type of tranforms is used
				ImGui::Auto("Use World Translation?", meshRec.mmd.useWorldPos);
				//reparenting ui
				auto parentNode = spawnedMesh->getParent();
				if (ND_Util::ImGuiNodeHierarchy(parentNode, "Parent Node")) //returns true if new node was selected
				{
					spawnedMesh->setParent(parentNode);
					meshRec.mmd.node = SPAWNER.getNodeHierarchyAddress(spawnedMesh->as<Node>());
				}
				//meshId editing UI
				if (ImGui::InputText("Mesh ID", &meshRec.mmd.meshId))
				{
					if (spawnedMesh && !meshRec.mmd.meshId.empty())
						spawnedMesh->setName(meshRec.mmd.meshId);
				}
				//Node UI renderer
				ND_Util::ImGuiNodeRender(spawnedMesh, m_lastSelectedChildMesh);
				//updates render order
				if (meshRec.mmd.renderOrder != spawnedMesh->getRenderOrder())
					meshRec.mmd.renderOrder = spawnedMesh->getRenderOrder();
			}

			//logic for deferred deletion of child meshes
			if (bNeedToDeleteSelected)
			{
				if(!selectionResult.second.child.expired())
				{
					if (auto parent = selectionResult.second.child.lock()->getParent(); parent)
					{
						parent->removeChild(selectionResult.second.child.lock());
					}
				}

				std::erase_if(m_childMeshes, [this](SpawnableChildMesh& rec)
					{
						return rec.mmd.meshId == m_lastSelectedChildMesh;
					});
				std::erase_if(m_mapObjectData.childMeshes, [this](sptr<MapMeshData>& mmd)
					{
						return mmd->meshId == m_lastSelectedChildMesh;
					});
				m_lastSelectedChildMesh.clear();
			}
		}

		ImGui::EndChild(); //Mesh child child form's end

		ImGui::NewLine();
		ImGui::EndTabItem(); 
	}
#endif
}

sptr<Spawnable> Spawnable::spawnChild(sptr<MapObjectsData> childData, SpawnableChild& childHolder, bool bTabSpawned)
{
	sptr<Spawnable> child = SPAWNER.spawnObjectOfClass(
		SPAWNER.getSpawnerRecord(childData->className),
		*childData,
		m_onChildSpawnCb, //uses default onSpawnCb if m_onChildSpawnCb == nullptr
		bTabSpawned
	);
	
	//if (m_onChildSpawnCb)
	//	m_onChildSpawnCb(child);
	//SPAWNER.registerSpawnable(child, false);

	childHolder.child = child;
	childHolder.bSpawned = true;
	childHolder.mod = child->getMapObjectsData();

	return child;
}

sptr<Mesh> Spawnable::spawnChildMesh(sptr<MapMeshData> childData, SpawnableChildMesh& childHolder)
{
	if (!childData)
		return nullptr;

	if (childData->node.empty())
		childData->node = SPAWNER.getNodeHierarchyAddress(getSpawnableRoot());
	sptr<Mesh> child = SPAWNER.spawnMesh(*childData);

	childHolder.child = child;
	childHolder.bSpawned = true;
	childHolder.mmd = *childData;

	child->subscribeToWorldTransformChanged([this, childHolder](const Transform& trans)
		{
			if (childHolder.child.expired())
				return;

			auto child = childHolder.child.lock();
			childHolder.mmd.position_local = child->getLocalTranslation();
			childHolder.mmd.position_world = trans.translation();
			childHolder.mmd.scale_local = child->getLocalScale();
			childHolder.mmd.rotation_local = child->getLocalRotation().euler();
			childHolder.mmd.renderOrder = child->getRenderOrder();
		});

	return child;
}