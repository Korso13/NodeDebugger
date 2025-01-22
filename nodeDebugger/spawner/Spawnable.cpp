#include "Spawnable.h"
#include "settings/JsonDeserializator.h"
#include "SpawnerFactory.h"

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

//Controls Spawnable's ImGui menu render (Root node transform menu, m_spawnableSettings onGui calls and etc)
bool Spawnable::onGui(const char* label)
{
	if (!m_spRoot && m_spawnableSettings.empty())
		return false;

	bool bNeedsDeletion = false; //used to communicate to SpawnerTab that the record is to be deleted via return true

#ifdef DEBUG
	if (ImGui::TreeNode(label))
	{
		ImGui::SameLine();
		if (ImGui::Button("x"))
		{
			bNeedsDeletion = true;
		}

		if (m_framePlotter) Render::getRoot()->removeChild(m_framePlotter);
		m_framePlotter = w4::render::Plotter::buildCube(getSpawnableBounds(getSpawnableRoot()), vec4(1.f, 0.f, 0.f, 1.f));
		Render::getRoot()->addChild(m_framePlotter);

		if (ImGui::BeginChild((m_spRoot->getName() + " Panel").c_str(), ND_Util::getSizeForChild(0.7), true, ImGuiWindowFlags_AlwaysAutoResize)) //scrollable panel with exposed settings
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
			if (m_spRoot)
			{
				if (ImGui::TreeNode("Root Node Transforms"))
				{
					ND_Util::ImGuiNodeRender(m_spRoot, m_spRoot->getName());

					ImGui::TreePop();
				}
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
					//TODO: guiFunc doesn't return true for Auto used with vectors and possibly other collections :(
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
				childSpawnerUI();
		}
		ImGui::EndChild(); //ends entity scrollable zone
		ImGui::TreePop();
	}
	else 
		if (m_framePlotter) Render::getRoot()->removeChild(m_framePlotter);

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
	childrenVisiter(visiter);

	return m_mapObjectData;
}

void Spawnable::childrenVisiter(const std::function<void(Spawnable&)>& visiterFunction)
{
	if (!visiterFunction)
		return;

	for (auto childHolder : m_children)
	{
		if (childHolder.bSpawned && !childHolder.child.expired())
		{
			childHolder.child.lock()->childrenVisiter(visiterFunction);
		}
	}
	visiterFunction(*this);
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
			getMapObjectsData().pos = trans.translation();
			getMapObjectsData().pos.z = 0.f; //sometimes the gizmo puts in double values into Z field breaking ROs. Disable if you need non-zero Z placement
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
}

void Spawnable::childSpawnerUI()
{
#ifdef DEBUG

	if (ImGui::TreeNode((getSpawnableRoot()->getName() + "'s Children").c_str()))
	{
		size_t count = 0;
		bool changed = false;
		std::function<void()> deferredDelete = nullptr;

		for (auto& childMOD : getMapObjectsData().childObjects)
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

		ImGui::TreePop();
	}

#endif
}

sptr<Spawnable> Spawnable::spawnChild(sptr<MapObjectsData> childData, SpawnableChild& childHolder, bool bTabSpawned)
{
	sptr<Spawnable> child = SPAWNER.spawnObjectOfClass(
		SPAWNER.getSpawnerRecord(childData->className),
		*childData,
		m_onChildSpawnCb == nullptr, //do we need to use default onSpawnCb?
		bTabSpawned
	);
	
	if (m_onChildSpawnCb)
		m_onChildSpawnCb(child);
	SPAWNER.registerSpawnable(child, false);

	childHolder.child = child;
	childHolder.bSpawned = true;
	childHolder.mod = child->getMapObjectsData();

	return child;
}
