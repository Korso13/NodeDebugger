#include "SpawnerTab.h"

#ifndef FORCENODEDEBUG_OFF
#include "common/nodeDebugger/spawner/Spawnable.h"
#include "common/nodeDebugger/spawner/SpawnerFactory.h"
#include "external/boost/preprocessor/seq/for_each.hpp"
#include <common/nodeDebugger/NodeDebugger.h>
#include <common/nodeDebugger/ND_JsonTools.h>
#include "settings/JsonDeserializator.h"

void SpawnerTab::onUpdate(float dt)
{
	for (auto& [name, spawned] : SPAWNER.getAllSpawnables())
		if (spawned->isSpawned)
			spawned->spawnableSelfUpdate(dt);
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
	if (ImGui::BeginCombo("<-Mesh Name", data.meshName.c_str()))
	{
		auto It = SPAWNER.getMeshesSettings().begin();
		for (size_t i = 0; i < SPAWNER.getMeshesSettings().size(); i++)
		{
			auto& meshName = (*It).first;
			auto& meshSetting = (*It++).second;
			std::string textureName = (meshSetting.textureName.empty() ? meshName : meshSetting.textureName);
			//Size of preview icon adjusted to plane dimentions
			ImVec2 icoSize = ImVec2(100, 100);
			auto normalizedPlane = meshSetting.plane.normalize();
			icoSize.x *= normalizedPlane.x;
			icoSize.y *= normalizedPlane.y;

			bool isSelected = meshName == data.meshName;
			if (isSelected)
				ImGui::SetItemDefaultFocus();

			//TODO: Consider replacing columns with Table
			ImGui::Columns(2);
			if (ImGui::Selectable(meshName.c_str(), &isSelected))
			{
				data.meshName = meshName;
			}
			ImGui::NextColumn();
			
			auto texture = RT(textureName);
			if (!texture)
			{
				ImGui::Columns(1);
				continue;
			}
			//Clickable preview of mesh's texture (doesn't collapse Combo :( )
			if (ImGui::ImageButton(RT(textureName), icoSize, ImVec2(0, 1), ImVec2(1, 0)))
			{
				data.meshName = meshName;
			}
			ImGui::Columns(1);
		}

		ImGui::EndCombo();
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
		for (auto& [className, spawnerEntityRec] : SPAWNER.getClassRegistry())
		{
			//Rendering spawnable classes tree header
			if (ImGui::TreeNode(className.c_str()))
			{
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
				m_needUpdateEntitiesFilter = true;
			}
		}
		ImGui::EndTabItem();
	}
}

void SpawnerTab::onGuiSpawnedList()
{
	if (!SPAWNER.getAllSpawnables().empty() || !SPAWNER.getAllMeshes().empty())
	{
		if (ImGui::BeginTabItem("Spawned Objects"))
		{
			//Exports newly spawned entities plus previously loaded from mapObjects.json
			if (ImGui::Button("Export All"))
			{
				//MapObjectsContainer tempContainer = Spawnable::mapData; //to prevent duplicating entries if exporting more than once in a session
				SPAWNER.updateMeshHierarchyRecords();
				MapObjectsContainer tempContainer;
				//MapObjectsData templateRec; tempContainer.mapObjectsData.push_back(templateRec);
				for (auto& [name, spawnedObject] : SPAWNER.getAllSpawnables())
					if (spawnedObject && !spawnedObject->getMapObjectsData().isChildObj)
						tempContainer.mapObjectsData.push_back(spawnedObject->getMapObjectsData());
				for (auto& [name, spawnedMesh] : SPAWNER.getAllMeshes())
					if (spawnedMesh.second)
						tempContainer.mapMeshData.push_back(spawnedMesh.first);

				nlohmann::json exportJson;
				nlohmann::adl_serializer<MapObjectsContainer>::to_json(exportJson, tempContainer);
				std::string flatJson = jsonUnflatten(exportJson);
				jsWriteEvent(flatJson.c_str(), "mapObjects.json");
			}

			//list-filtering magic
			bool isFilterChanged = false;
			if (SPAWNER.getAllSpawnables().size() > 6)
				isFilterChanged = (ND_Util::ImGuiSearchBar(m_entitiesFilter) || m_needUpdateEntitiesFilter);
			else if (!m_entitiesFilter.empty())
				m_entitiesFilter.clear();
			if (m_needUpdateEntitiesFilter) 
				m_needUpdateEntitiesFilter = false;
			if (isFilterChanged) m_lastEntitiesFilterList.clear();

			//scrollable child panel
			if (ImGui::BeginChild("Spawned Objects Panel"))
			{
				std::string defferedDeleteName;
				//============================================================
				//renders list of spawned entities with their exposed settings
				//============================================================
				for (auto& [name, spawnedObject] : SPAWNER.getAllSpawnables())
				{
					//list-filtering logic optimizations
					if (isFilterChanged && !m_entitiesFilter.empty())
					{
						if (!ND_Util::checkFilterConditions(name, m_entitiesFilter))
							continue;
						else
							m_lastEntitiesFilterList.insert(name);
					}
					else if(!m_entitiesFilter.empty())
					{
						if (!m_lastEntitiesFilterList.contains(name))
							continue;
					}

					bool bNeedDeletion = false;
					if (spawnedObject)
					{
						//failsafe in case object was not named
						if (name.empty())
						{
							spawnedObject->onGui(SPAWNER.checkUniqueName("Unnamed").c_str()); //can't be deleted if the name is empty, so we're not using return value
							continue;
						}
						//render Spawnable's settings
						bNeedDeletion = spawnedObject->onGui(name.c_str());
					}
					//if 'x' button was pressed for current record
					if (bNeedDeletion)
						defferedDeleteName = name;
				}

				//executing delete entry order (if any) for Spawnable
				if (!defferedDeleteName.empty())
				{
					if (SPAWNER.getAllSpawnables().contains(defferedDeleteName))
					{
						//trying to use assignable callback to remove this object's sptr outside of Spawner (and hence from memory)
						auto& spawnableForDeletion = SPAWNER.getAllSpawnables()[defferedDeleteName];
						if (SPAWNER.getClassRegistry().contains(spawnableForDeletion->getMapObjectsData().className))
						{
							auto& spawnRecord = SPAWNER.getClassRegistry()[spawnableForDeletion->getMapObjectsData().className];
							if (spawnRecord.m_onDeleteCb)
								spawnRecord.m_onDeleteCb(spawnableForDeletion);
						}
						//removing the Spawnable from the Spawner's list of entities
						SPAWNER.getAllSpawnables().erase(defferedDeleteName);
					}
					defferedDeleteName.clear();
				}

				//===============================
				//renders list of spawned meshes
				//===============================
				for (auto& [name, spawnedMesh] : SPAWNER.getAllMeshes())
				{
					//list-filtering logic optimizations
					if (isFilterChanged && !m_entitiesFilter.empty())
					{
						if (!ND_Util::checkFilterConditions(name, m_entitiesFilter))
							continue;
						else
							m_lastEntitiesFilterList.insert(name);
					}
					else if(!m_entitiesFilter.empty())
					{
						if (!m_lastEntitiesFilterList.contains(name))
							continue;
					}

					if (!spawnedMesh.second)
						continue;

					if (ImGui::TreeNode(name.c_str()))
					{
						ImGui::SameLine();
						if (ImGui::Button("x"))
						{
							defferedDeleteName = name;
						}
						//Button for focusing on the object with camera
						if (ImGui::Button(("Focus on " + name).c_str()))
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
						ND_Util::ImGuiNodeRender(spawnedMesh.second, name);

						ImGui::TreePop();
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
					defferedDeleteName.clear();
				}
			}
			ImGui::EndChild(); //ends Spawned entities list scrollable zone
			ImGui::EndTabItem();
		}//end of "Spawned Objects" tab
	}
}
#endif