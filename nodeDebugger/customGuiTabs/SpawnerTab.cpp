#include "SpawnerTab.h"

#ifndef FORCENODEDEBUG_OFF
#include "common/nodeDebugger/spawner/Spawnable.h"
#include "common/nodeDebugger/spawner/SpawnerFactory.h"
#include "external/boost/preprocessor/seq/for_each.hpp"
#include <common/nodeDebugger/NodeDebugger.h>
#include <common/nodeDebugger/ND_JsonTools.h>
#include "settings/JsonDeserializator.h"
#include "../CustomAutos.h"

static std::string lastSelectedSpawnerClass = "CLASS_NAME";

SpawnerTab::SpawnerTab()
	: iGuiTab("Spawner")
{
	m_currentMeshSpawnerSetting.className = "Mesh";
	event::Keyboard::Down::subscribe(std::bind(&SpawnerTab::onKeyPressed, this, std::placeholders::_1));
	event::Keyboard::Up::subscribe(std::bind(&SpawnerTab::onKeyReleased, this, std::placeholders::_1));
	Touch::Begin::subscribe(std::bind(&SpawnerTab::onTouchBegin, this, std::placeholders::_1));
}

void SpawnerTab::markActive(bool isActive)
{
	iGuiTab::markActive(isActive);
	
	if(!isActive)
	{
		if (SPAWNER.getAllSpawnables().contains(m_lastSelectedSpawnable))
		{
			SPAWNER.getAllSpawnables()[m_lastSelectedSpawnable]->removeFocusFrame();
			//SPAWNER.getAllSpawnables()[m_lastSelectedSpawnable]->childrenVisiter([this](Spawnable& child) {child.removeFocusFrame(); });
		}
	}
}

void SpawnerTab::setupClickCollider(const std::string& name, sptr<Spawnable> spawnable)
{
	if (!spawnable)
		return;

	spawnable->initSpawnableCollider(
		[this, name](const CollisionInfo& hit)
		{
			if (!ND_CASTALLOWED || m_activeSubtab != SpawnedObjects)
				return;

			if (m_clickFiltering && !ND_Util::checkFilterConditions(name, m_entitiesFilter))
				return;

			if (isActiveTab() && m_lastSelectedSpawnable != name)
			{
				if (SPAWNER.getAllSpawnables().contains(m_lastSelectedSpawnable))
					SPAWNER.getAllSpawnables()[m_lastSelectedSpawnable]->removeFocusFrame();
				m_lastSelectedSpawnable = name;
			}
		});
}

void SpawnerTab::setupClickCollider(const std::string& name, sptr<Mesh> newMesh)
{
	if (!newMesh)
		return;

	auto collider = newMesh->addCollider<AABB>("spawner_collider", newMesh);
	collider->setBlockRaycasts(false);
	collider->setBlockScreencasts(false);
	collider->setReceiveRaycasts(true);
	auto clickCb = [this, name](const CollisionInfo& hit)
		{
			if (!ND_CASTALLOWED || m_activeSubtab != SpawnedMeshes)
				return;

			if (m_clickFiltering && !ND_Util::checkFilterConditions(name, m_meshesFilter))
				return;

			if (isActiveTab())
				m_lastSelectedMesh = name;
		};
	collider->setRaycastCallback(clickCb);
	collider->setScreencastCallback(ScreencastEvent::Down, clickCb);
	collider->setLocalTranslation(vec3(0.f, 0.f, -0.01));
}

void SpawnerTab::initClickColliders()
{
	for (auto& [name, spawnable] : SPAWNER.getAllSpawnables())
	{
		if (!spawnable)
			continue;

		setupClickCollider(name, spawnable);
	}

	for (auto& [name, MeshData] : SPAWNER.getAllMeshes())
	{
		if (!MeshData.second)
			continue;

		auto& mesh = MeshData.second;
		setupClickCollider(name, mesh);
	}
}

void SpawnerTab::onGuiExportBtn()
{
	if (ImGui::Button("Export All"))
	{
		//MapObjectsContainer tempContainer = Spawnable::mapData; //to prevent duplicating entries if exporting more than once in a session
		SPAWNER.updateMeshHierarchyRecords();
		MapObjectsContainer tempContainer;
		//MapObjectsData templateRec; tempContainer.mapObjectsData.push_back(templateRec);
		for (auto& [name, spawnedObject] : SPAWNER.getAllSpawnables())
			if (spawnedObject && !spawnedObject->getMapObjectsData().isChildObj)
			{
				spawnedObject->updateChildrensData();
				tempContainer.mapObjectsData.push_back(spawnedObject->getMapObjectsData());
			}
		for (auto& [name, spawnedMesh] : SPAWNER.getAllMeshes())
			if (spawnedMesh.second && !spawnedMesh.first.isChild)
				tempContainer.mapMeshData.push_back(spawnedMesh.first);

		nlohmann::json exportJson;
		nlohmann::adl_serializer<MapObjectsContainer>::to_json(exportJson, tempContainer);
		std::string flatJson = jsonUnflatten(exportJson);
		jsWriteEvent(flatJson.c_str(), "mapObjects.json");
	}
}

void SpawnerTab::onUpdate(float dt)
{

}

bool SpawnerTab::isEmpty()
{ 
	return SPAWNER.getClassRegistry().empty() && SPAWNER.getMeshesSettings().empty();
};

void SpawnerTab::onGuiCall()
{
	if (ImGui::BeginTabBar("Spawner Tab Bar"))
	{
		onGuiClassPalette();
		onGuiMeshPalette();
		onGuiSpawnedList();
		onGuiSpawnedMeshesList();
	}
	ImGui::EndTabBar();
}

//Unused right now. Consider depricated for this tab
void SpawnerTab::addItem(std::any item, const std::string& altName)
{
	auto spawned = any_cast<sptr<Spawnable>>(item);
	if (spawned)
	{
		SPAWNER.getAllSpawnables().insert(
			{ 
				SPAWNER.checkUniqueName(altName == "default" ? spawned->getSpawnableRoot()->getName() : altName), 
				spawned 
			}
		);
	}
}

void SpawnerTab::purgeValues()
{
	SPAWNER.purgeSpawnedObjects();
	SPAWNER.purgeSpawnedMeshes();
}

void SpawnerTab::onGuiMapMeshData(MapMeshData& data)
{
	if (ND_Util::ImGuiTextureSelector(data.meshName))
	{
		data.meshId = data.meshName;
	}
	
	//Combo list selector for materials
	auto materialSelectorResult = ND_Util::ImGuiMaterialSelector(data.material);
	if (materialSelectorResult.first)
		data.material = materialSelectorResult.second;
	ImGui::Auto("Render Order", data.renderOrder);
	ImGui::Auto("Use World Translation?", data.useWorldPos);
	if (data.useWorldPos)
		ImGui::Auto("World Pos", data.position_world);
	else
		ImGui::Auto("Local Pos", data.position_local);
	ImGui::Auto("Local Scale", data.scale_local);
	ImGui::Auto("Local Rotation", data.rotation_local);
}

void SpawnerTab::onGuiClassPalette()
{
	if (ImGui::BeginTabItem("Class Palette"))
	{
		m_activeSubtab = Subtab::ClassPalette;
		for (auto& [className, spawnerEntityRec] : SPAWNER.getClassRegistry())
		{
			//Rendering spawnable classes tree header
			if (ImGui::TreeNode(className.c_str()))
			{
				lastSelectedSpawnerClass = spawnerEntityRec.m_classID;
				//Rendering spawn settings
				for (auto& settingRender : spawnerEntityRec.m_argsOnGui)
					if (settingRender)
						settingRender();

				//rendering spawn button
				if (spawnerEntityRec.m_spawnerFunc)
				{
					//Spawn button logic
					if (ImGui::Button("Spawn"))
					{
						warn("Spawning %s!", className.c_str());
						//Spawns new object of class and registers it to appear in Spawned subtab (kept in SpawnerFactory's getAllSpawnables())
						auto newEntity = SPAWNER.spawnObjectOfClass(spawnerEntityRec, true);
						auto name = SPAWNER.getLastSpawnedObjectName();
						setupClickCollider(name, newEntity);
						m_needUpdateEntitiesFilter = true;
					}
				}

				ImGui::TreePop();
			}

		}

		ImGui::EndTabItem();
	}
}

void SpawnerTab::onGuiMeshPalette()
{
	if (ImGui::BeginTabItem("Mesh Spawner"))
	{
		m_activeSubtab = Subtab::MeshPalette;
		//Mesh parent node selector
		ND_Util::ImGuiNodeHierarchy(SPAWNER.getDefaultMeshesRoot(), "Mesh Parent Node");

		//Mesh settings render
		onGuiMapMeshData(m_currentMeshSpawnerSetting);

		//Spawn mesh button
		if (ImGui::Button("Spawn"))
		{
			//Mesh spawner logic
			auto newMesh = SPAWNER.spawnMesh(m_currentMeshSpawnerSetting);
			if (newMesh)
			{
				info("Spawning new mesh %s!", m_currentMeshSpawnerSetting.meshName.c_str());

				vec3 cameraPos = Render::getScreenCamera()->getWorldTranslation();
				cameraPos.z = 0.f;
				newMesh->setWorldTranslation(cameraPos);

				auto name = m_currentMeshSpawnerSetting.meshId;
				m_currentMeshSpawnerSetting.meshId = m_currentMeshSpawnerSetting.meshName;

				setupClickCollider(name, newMesh);

				m_needUpdateEntitiesFilter = true;
			}
		}
		ImGui::EndTabItem();
	}
}

void SpawnerTab::onGuiSpawnedList()
{
	//TODO: 
	//- (?) Add btn to spawnable that focuses Child object in the list (as alternative to editing in parent's form)

	if (SPAWNER.getAllSpawnables().empty())
		return;

	if (ImGui::BeginTabItem("Spawned Objects"))
	{
		m_activeSubtab = Subtab::SpawnedObjects;
		//Exports newly spawned entities plus previously loaded from mapObjects.json
		onGuiExportBtn();
		ImGui::SameLine();
		ImGui::Checkbox("Click selection with filter", &m_clickFiltering);

		//list-filtering magic
		bool isFilterChanged = false;
		if (SPAWNER.getAllSpawnables().size() > 6)
		{
			isFilterChanged = ND_Util::ImGuiSearchBar(m_entitiesFilter);
		}
		else if (!m_entitiesFilter.empty())
		{
			m_entitiesFilter.clear();
			isFilterChanged = true;
		}
		if (m_lastSpawnablesFilterList.empty() && !SPAWNER.getAllSpawnables().empty())
			isFilterChanged = true;
		if (m_lastMeshesFilterList.empty() && !SPAWNER.getAllMeshes().empty())
			isFilterChanged = true;
			
		//updating list of filtered entries if needed
		if (isFilterChanged || m_needUpdateEntitiesFilter)
		{
			m_lastSpawnablesFilterList.clear();
			m_lastMeshesFilterList.clear();
			for (auto& [name, spawnedObject] : SPAWNER.getAllSpawnables())
			{
				if (ND_Util::checkFilterConditions(name, m_entitiesFilter))
					m_lastSpawnablesFilterList.insert(name);
			}
			for (auto& [name, spawnedMesh] : SPAWNER.getAllMeshes())
			{
				if (ND_Util::checkFilterConditions(name, m_entitiesFilter))
				{
					m_lastMeshesFilterList.insert(name);
				}
			}
		}
		if (m_needUpdateEntitiesFilter) m_needUpdateEntitiesFilter = false;

		//============================================================
		//renders filtered list of spawned objects with their exposed settings
		//============================================================
		auto listResult = ND_Util::ImGuiListBox("Spawned", m_lastSpawnablesFilterList, m_lastSelectedSpawnable);
		//updating spawned entity's selection
		if(listResult.first)
		{
			if (SPAWNER.getAllSpawnables().contains(m_lastSelectedSpawnable))
				SPAWNER.getAllSpawnables()[m_lastSelectedSpawnable]->removeFocusFrame();
			m_lastSelectedSpawnable = listResult.second;
		}

		//scrollable child panel
		if (ImGui::BeginChild("Spawned Objects Panel", ND_Util::getSizeForChild(0.7), true, ImGuiWindowFlags_AlwaysAutoResize))
		{
			std::string defferedDeleteName;
			//============================================================
			//Selected spawned object with its exposed settings
			//============================================================
			if (SPAWNER.getAllSpawnables().contains(m_lastSelectedSpawnable))
			{
				auto& spawnedObject = SPAWNER.getAllSpawnables()[m_lastSelectedSpawnable];
				bool bNeedDeletion = false;
				if (spawnedObject)
				{
					//onGui call renders exposed settings for the Spawnable object
					//failsafe in case object was not named
					if (m_lastSelectedSpawnable.empty())
						spawnedObject->onGui(SPAWNER.checkUniqueName("Unnamed").c_str()); //can't be deleted if the name is empty, so we're not using return value
					//render Spawnable's settings
					bNeedDeletion = spawnedObject->onGui(m_lastSelectedSpawnable.c_str());
				}
				//if 'x' button was pressed for current record
				if (bNeedDeletion)
					defferedDeleteName = m_lastSelectedSpawnable;
			}

			//executing delete entry order (if any) for Spawnable
			if (!defferedDeleteName.empty())
			{
				SPAWNER.removeObject(defferedDeleteName);
				m_lastSpawnablesFilterList.erase(defferedDeleteName);
				defferedDeleteName.clear();
			}
		}
		ImGui::EndChild(); //ends selected Spawned object scrollable zone
		ImGui::EndTabItem();
	}//end of "Spawned Objects" tab
	else if (SPAWNER.getAllSpawnables().contains(m_lastSelectedSpawnable))
		SPAWNER.getAllSpawnables()[m_lastSelectedSpawnable]->removeFocusFrame();
}

void SpawnerTab::onGuiSpawnedMeshesList()
{
	if (SPAWNER.getAllMeshes().empty())
		return;

	if (ImGui::BeginTabItem("Spawned Meshes"))
	{
		m_activeSubtab = Subtab::SpawnedMeshes;
		//Exports newly spawned entities plus previously loaded from mapObjects.json
		onGuiExportBtn();
		ImGui::SameLine();
		ImGui::Checkbox("Click selection with filter", &m_clickFiltering);

		//list-filtering magic
		bool isFilterChanged = false;
		if (SPAWNER.getAllMeshes().size() > 6)
		{
			isFilterChanged = ND_Util::ImGuiSearchBar(m_meshesFilter);
		}
		else if (!m_meshesFilter.empty())
		{
			m_meshesFilter.clear();
			isFilterChanged = true;
		}
		if (m_lastMeshesFilterList.empty() && !SPAWNER.getAllMeshes().empty())
			isFilterChanged = true;

		//updating list of filtered entries if needed
		if (isFilterChanged || m_needUpdateEntitiesFilter)
		{
			m_lastMeshesFilterList.clear();
			for (auto& [name, spawnedMesh] : SPAWNER.getAllMeshes())
			{
				if (ND_Util::checkFilterConditions(name, m_meshesFilter))
				{
					m_lastMeshesFilterList.insert(name);
				}
			}
		}
		if (m_needUpdateEntitiesFilter) m_needUpdateEntitiesFilter = false;

		//============================================================
		//renders filtered list of spawned objects with their exposed settings
		//============================================================
		auto listResult = ND_Util::ImGuiListBox("Spawned", m_lastMeshesFilterList, m_lastSelectedMesh);
		//updating spawned entity's selection
		if (listResult.first)
		{
			m_lastSelectedMesh = listResult.second;
		}
		//scrollable child panel
		if (ImGui::BeginChild("Spawned Mesh Panel", ND_Util::getSizeForChild(0.7), true, ImGuiWindowFlags_AlwaysAutoResize))
		{
			std::string defferedDeleteName;
		
			//===============================
			//renders selected spawned mesh
			//===============================
			if(SPAWNER.getAllMeshes().contains(m_lastSelectedMesh))
			{
				auto& spawnedMesh = SPAWNER.getAllMeshes()[m_lastSelectedMesh];

				if (spawnedMesh.second)
				{
					ImGui::SameLine();
					if (ImGui::Button(("DELETE " + m_lastSelectedMesh).c_str()))
					{
						defferedDeleteName = m_lastSelectedMesh;
					}
					//Button for focusing on the object with camera
					if (ImGui::Button(("Focus on " + m_lastSelectedMesh).c_str()))
					{
						vec3 camPos = Render::getScreenCamera()->getWorldTranslation();
						vec3 centerPos = spawnedMesh.second->getWorldTranslation();
						camPos.x = centerPos.x; camPos.y = centerPos.y;
						Render::getScreenCamera()->setWorldTranslation(camPos);
					};

					auto materialSelectorResult = ND_Util::ImGuiMaterialSelector(spawnedMesh.first.material);
					if (materialSelectorResult.first)
					{
						spawnedMesh.first.material = materialSelectorResult.second;
						auto newMat = RMat(spawnedMesh.first.material)->createInstance();
						newMat->setTexture(TextureId::TEXTURE_0, spawnedMesh.second->getMaterialInst()->getTexture(TextureId::TEXTURE_0));
						spawnedMesh.second->setMaterialInst(newMat);
					}

					if (ImGui::Auto("Render Order", spawnedMesh.first.renderOrder))
						spawnedMesh.second->setRenderOrder(spawnedMesh.first.renderOrder);
					ImGui::Auto("Use World Translation?", spawnedMesh.first.useWorldPos);
					auto parentNode = spawnedMesh.second->getParent();
					if (ND_Util::ImGuiNodeHierarchy(parentNode, "Parent Node")) //returns true if new node was selected
					{
						spawnedMesh.second->setParent(parentNode);
						spawnedMesh.first.node = SPAWNER.getNodeHierarchyAddress(spawnedMesh.second->as<Node>());
					}
					if (ImGui::InputText("Mesh ID", &spawnedMesh.first.meshId))
					{
						if (spawnedMesh.second && !spawnedMesh.first.meshId.empty())
							spawnedMesh.second->setName(spawnedMesh.first.meshId);
					}
					ND_Util::ImGuiNodeRender(spawnedMesh.second, m_lastSelectedMesh);
				}
			}

			//executing delete entry order (if any) for Mesh
			if (!defferedDeleteName.empty())
			{
				NODEDEBUG->removeImGuiNode(defferedDeleteName);
				if (SPAWNER.getAllMeshes().contains(defferedDeleteName))
				{
					auto meshToDelete = SPAWNER.getAllMeshes()[defferedDeleteName].second;
					meshToDelete->getParent()->removeChild(meshToDelete);
					SPAWNER.getAllMeshes().erase(defferedDeleteName);
				}
				m_lastSelectedMesh = "";
				m_lastMeshesFilterList.erase(defferedDeleteName);
				defferedDeleteName.clear();
			}

		}
		ImGui::EndChild(); //ends selected Spawned mesh scrollable zone
		ImGui::EndTabItem();
	}//end of "Spawned Meshes" tab
}

void SpawnerTab::onKeyPressed(const event::Keyboard::Down& evt)
{
	if (!isActiveTab())
		return;

	if (evt.key == w4::platform::KeyboardKey::Control && !m_clickAddModeON)
		m_clickAddModeON = true;
}

void SpawnerTab::onKeyReleased(const event::Keyboard::Up& evt)
{
	if (evt.key == w4::platform::KeyboardKey::Control && m_clickAddModeON)
		m_clickAddModeON = false;
}

void SpawnerTab::onTouchBegin(const Touch::Begin& evt)
{
	if (!isActiveTab())
		return;

	if (!m_clickAddModeON)
		return;

	if (m_activeSubtab == SpawnedObjects || m_activeSubtab == SpawnedMeshes)
		return;

	vec3 clickPos = getMouseWorldPos();

	if(m_activeSubtab == ClassPalette && lastSelectedSpawnerClass != "CLASS_NAME")
	{
		info("Spawning %s!", lastSelectedSpawnerClass.c_str());
		//Spawns new object of class and registers it to appear in Spawned subtab (kept in SpawnerFactory's getAllSpawnables())
		auto newEntity = SPAWNER.spawnObjectOfClass(SPAWNER_REC(lastSelectedSpawnerClass), true, clickPos * vec3(1.f, 1.f, 0.f));
		auto name = SPAWNER.getLastSpawnedObjectName();
		setupClickCollider(name, newEntity);
	}
	//else if (m_activeSubtab == ClassPalette)
	//	info("[SpawnerTab::onTouchBegin] Class palette tab, but class name is %s, not spawning!", lastSelectedSpawnerClass.c_str());
	else if (m_activeSubtab == MeshPalette && !m_currentMeshSpawnerSetting.meshName.empty())
	{
		auto newMesh = SPAWNER.spawnMesh(m_currentMeshSpawnerSetting);
		if (newMesh)
		{
			info("Spawning new mesh %s!", m_currentMeshSpawnerSetting.meshName.c_str());
			newMesh->setWorldTranslation(clickPos);
			auto name = m_currentMeshSpawnerSetting.meshId;
			m_currentMeshSpawnerSetting.meshId = m_currentMeshSpawnerSetting.meshName;
			setupClickCollider(name, newMesh);
		}
	}
	m_needUpdateEntitiesFilter = true;
}

#endif