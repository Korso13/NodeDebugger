#include "Spawnable.h"
#include "settings/JsonDeserializator.h"
#include "SpawnerFactory.h"
#include <common/nodeDebugger/NodeDebugger.h>

#include "others/utils/ScreenSpaceShader.h"
//#include "common/Digits_S.h"
#include "others/utils/CollidersExt.h"
#ifndef FORCENODEDEBUG_OFF
#include "SpawnerTools.h"
#endif

std::array<vec3, 2> Spawnable::getOtherSpawnableBounds(sptr<Spawnable> spawnable, bool forceUpdateBounds)
{
	return spawnable->getSpawnableBounds(spawnable->getSpawnableRoot(), forceUpdateBounds);
}

std::array<vec3, 2> Spawnable::getSpawnableBounds(sptr<Node> spawnableRoot, bool forceUpdateBounds)
{
	if (!spawnableRoot)
		return m_cachedBounds;

	if(!forceUpdateBounds && m_cachedBounds != std::array<w4::math::vec3, 2>())
		return m_cachedBounds;

	constexpr const static vec3 bDefaultMin = math::vec3(std::numeric_limits<float>::max());
	constexpr const static vec3 bDefaultMax = math::vec3(-std::numeric_limits<float>::max());
	vec3 bmin = bDefaultMin;
	vec3 bmax = bDefaultMax;
	spawnableRoot->traversalChildrenTyped<VisibleNode>(
		[&bmin, &bmax, this, spawnableRoot](const sptr<VisibleNode>& node)
		{
			if (!node || !node->isEnabled()) return;
			//skipping non-relevant VisibleNodes
			if (node->getWorldTranslation() == INVALIDPOS)
				return; 
			if (node->is<ParticlesEmitter>())
				return;
			if (node->is<Mesh>())
			{
				auto mesh = node->as<Mesh>();
				auto textureName = mesh->getMaterialInst()->getTexture(TextureId::TEXTURE_0)->getName();
				if (textureName.find("target.png") != std::string::npos || mesh->getName().find(getObjId()+"_spawner_collider") != std::string::npos)
				{
					//info("[Spawnable::getSpawnableBounds]Skipping mesh %s with texture name %s", node->getName().c_str(), textureName.c_str());
					return;
				}
			}

			//special mechanism for spines
			if (node->is<Spine>())
			{
				vec3 spineSize;
				auto spine = node->as<Spine>();

				spine::SkeletonBounds boundingBox;
				boundingBox.update(*(spine->skeleton.get()), true);
				if (boundingBox.getWidth() != -1.f * std::numeric_limits<float>::max()
					&& boundingBox.getWidth() != std::numeric_limits<float>::max()
					&& boundingBox.getWidth() != std::numeric_limits<float>::lowest()
					)
				{
					spineSize = vec3(boundingBox.getWidth()/1024.f * spine->getLocalScale().x,
														boundingBox.getHeight()/1024.f * spine->getLocalScale().y,
														0.1);
					//sp_info("Spine size WITH bounding box: %s (width %f)\nNumeric min: %f, Numeric max: %f",
					//	CVEC(spineSize), boundingBox.getWidth(), -1.f * std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
				}
				else
				{
					auto matrix = spine->getSkeletonCurrentBounds().getMatrix();
					vec3 scale = spine->getSkeletonCurrentBounds().scale();
					spineSize = (matrix * vec4(scale.x, scale.y, scale.z, 1)).asVec3();  // normalize to world size

					//sp_info("Spine size with NO bounding box: %s", CVEC(spineSize));
				}
				spineSize.z = 0.01;

				vec3 leftBottom = node->getWorldTranslation() - spineSize * vec3(0.5, 0.f, 1.f);
				vec3 rightTop = node->getWorldTranslation() + spineSize * vec3(0.5, 1.f, 1.f);

				if(bmin != bDefaultMin)
					bmin = std::min(bmin, leftBottom);
				else
					bmin = leftBottom;

				if(bmax != bDefaultMax)
					bmin = std::min(bmax, leftBottom);
				if(bmin != bDefaultMin)
					bmax = std::max(bmin, rightTop);

				if(bmax != bDefaultMax)
					bmax = std::max(bmax, rightTop);
				else
					bmax = rightTop;
			}
			// All other visibleNodes
			else if (node->getSurfacesCount() > 0)
			{
				//calculating visible node's bounds and matching against current min-max bounds
				for (auto it = node->getGeometryIterator(); it; ++it)
				{
					bmin = std::min(bmin, it->pos*node->getWorldScale() + node->getWorldTranslation());
					bmax = std::max(bmax, it->pos*node->getWorldScale() + node->getWorldTranslation());
				}
			}
			//sp_debug("After node %s (of %s) \nmin: %s, max: %s", node->getName().c_str(), spawnableRoot->getName().c_str(),CVEC(bmin), CVEC(bmax));
		});

	bmin = std::min(bmin, spawnableRoot->getWorldTranslation());
	bmax = std::max(bmax, spawnableRoot->getWorldTranslation());
	//sp_debug("After root pos (of %s) min-max \nmin: %s, max: %s", spawnableRoot->getName().c_str(),CVEC(bmin), CVEC(bmax));

	if(bmin != bDefaultMin)
	{
		m_cachedBounds[0] = bmin; m_cachedBounds[0].z = -0.05;
	}
	else
	{
		m_cachedBounds[0] = vec3(-0.5, -0.5, -0.05f);
	}
	if(bmax != bDefaultMax)
	{
		m_cachedBounds[1] = bmax; m_cachedBounds[1].z =  0.05;
	}
	else
	{
		m_cachedBounds[1] = vec3(0.5, 0.5, 0.05f);
	}
	//sp_debug("Final bounds of %s) \nmin: %s, max: %s",
	//	spawnableRoot->getName().c_str(),
	//	CVEC(m_cachedBounds[0]), CVEC(m_cachedBounds[1]));

	//in case we found nothing during traversal
	if (m_cachedBounds == std::array<vec3, 2>() || (m_cachedBounds[1] - m_cachedBounds[0]).length() < 0.6f)
	{
		auto pos = spawnableRoot->getWorldTranslation();
		static std::array<vec3, 2> defaultBounds = {vec3(-0.5, -0.5, -0.05f), vec3(0.5, 0.5, 0.f)};
		//sp_debug("%s using defaulted bounds![{%s} - {%s}]\n[{%s} - {%s}]\nLength %f fo delta-vector %s",
		// 	m_mapObjectData.objId.c_str(),
		// 	CSTR(pos + defaultBounds[0]), CSTR(pos + defaultBounds[1]),
		// 	CSTR(m_cachedBounds[0]), CSTR(m_cachedBounds[1]),
		// 	(m_cachedBounds[1] - m_cachedBounds[0]).length(),
		// 	CVEC(m_cachedBounds[1] - m_cachedBounds[0]));
		m_cachedBounds = {pos + defaultBounds[0], pos + defaultBounds[1]};
	}
	else
	{
		// debug("[Spawnable::getSpawnableBounds] %s m_cachedBounds, [{%s} - {%s}], \n length %f",
		// 	m_mapObjectData.objId.c_str(),
		// 	CSTR(m_cachedBounds[0]), CSTR(m_cachedBounds[1]),
		// 	(m_cachedBounds[1] - m_cachedBounds[0]).length());
	}

	return m_cachedBounds;
}

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
	colliderSize.x = std::max(0.5f, bounds[1].x - bounds[0].x);
	colliderSize.y = std::max(0.5f, bounds[1].y - bounds[0].y);
	colliderSize.z = 0.01;
	vec3 colliderPos = bounds[0] + colliderSize / 2.f - m_spRoot->getWorldTranslation();
	//sp_warn("colliderSize: %s\nbounds: {%s} - {%s}", CVEC(colliderSize), CVEC(bounds[0]), CVEC(bounds[1]));

	m_spawnableSelectCollider = CollidersExt::MakeCollidersExt(colliderSize, m_spRoot, 15, getObjId()+"_spawner_collider", "Spawnables");
	m_spawnableSelectCollider->setColliderPos(colliderPos);
	m_spawnableSelectCollider->setBlockRaycasts(false);
	m_spawnableSelectCollider->setBlockScreencasts(false);
	m_spawnableSelectCollider->setParentObject(*this);
	// Not really used for now
	m_colliderCbHandlerId = m_spawnableSelectCollider->addOnScreencastCallback(
		[this, clickCb](const CollisionInfo& hit, CollidersExt& collider, ScreencastEvent event)
		{
			if (clickCb && event == ScreencastEvent::Down)
			{
				clickCb(hit);
			}
		});
	// currently unused
	//m_colliderRayCbHandlerId = m_spawnableSelectCollider->addOnRaycastCallback(
	//	[this](const CollisionInfo& hit, CollidersExt& collider)
	//	{});
#endif // DEBUG
}

void Spawnable::updateSpawnableCollider(const std::array<vec3, 2>& bounds)
{
	vec3 colliderSize = vec3::zero;
	colliderSize.x = std::max(0.5f, bounds[1].x - bounds[0].x);
	colliderSize.y = std::max(0.5f, bounds[1].y - bounds[0].y);
	colliderSize.z = 0.01;
	vec3 colliderPos = bounds[0] + colliderSize / 2.f - m_spRoot->getWorldTranslation();
	//sp_warn("[updateSpawnableCollider] colliderSize: %s\nbounds: {%s} - {%s}", CVEC(colliderSize), CVEC(bounds[0]), CVEC(bounds[1]));

	m_spawnableSelectCollider->setColliderSize(colliderSize);
	m_spawnableSelectCollider->setColliderPos(colliderPos);
}

Spawnable::~Spawnable()
{
	sp_debug("");
	if (m_framePlotter)
		Render::getRoot()->removeChild(m_framePlotter);

	if (m_spawnableSelectCollider)
	{
		m_spawnableSelectCollider->removeScreencastCallback(m_colliderCbHandlerId);
		m_spawnableSelectCollider->removeRaycastCallback(m_colliderRayCbHandlerId);
		m_spawnableSelectCollider = nullptr;
	}

	if (m_spRoot)
	{
		m_spRoot->removeAllChildren();
		if (m_spRoot->getParent())
			m_spRoot->getParent()->removeChild(m_spRoot);
		m_spRoot = nullptr;
	}
}

//Controls Spawnable's ImGui menu render (Root node transform menu, m_spawnableSettings onGui calls and etc)
bool Spawnable::onGui(const char* label)
{
	if (!m_spRoot && m_spawnableSettings.empty() && m_spawnableBtns.empty())
		return false;

	bool bNeedsDeletion = false; //used to communicate to SpawnerTab that the record is to be deleted via return true

#ifdef DEBUG
	//heading label with DELETE btn
	ImGui::Text("%s%s", label, " properties");
	if (ImGui::Button(STR("DELETE " + std::string(label)).c_str()))
	{
		bNeedsDeletion = true;
	}
	//Button for focusing on the object with camera
	if (m_spRoot && m_spRoot->getRoot() != SCREEN_FX.getNodeForHud())
	{
		if (ImGui::Button(("Focus on " + std::string(label)).c_str()))
		{
			vec3 camPos = Render::getScreenCamera()->getWorldTranslation();
			auto bounds = getSpawnableBounds(getSpawnableRoot());
			vec3 centerPos = bounds[0] + (bounds[1] - bounds[0]);
			camPos.x = centerPos.x; camPos.y = centerPos.y;
			Render::getScreenCamera()->setWorldTranslation(camPos);
		};
	}
	SP_Utils::onGuiSingleExportBtns(*this);

	if(!m_childrenAdderEnabled)
	{
		SpawnableMainUi();
	}
	else
	{
		if (ImGui::BeginTabBar("Spawnable settings tabs"))
		{
			if (ImGui::BeginTabItem("Spawnable Settings"))
			{
				SpawnableMainUi();
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Children"))
			{
				//Sub TabBar for child objects and meshes tabs
				{
					if (ImGui::BeginTabBar((getSpawnableRoot()->getName() + "'s Children Bar").c_str()))
					{
						childSpawnerUI();
						childMeshSpawnerUI();
					}
					ImGui::EndTabBar();
				}

				ImGui::EndTabItem(); //ends "Children" TabBar
			}
		}
		ImGui::EndTabBar(); //Ends "Spawnable settings tabs" bar
	}

	//executing deletion orders
	if (m_deferredDeleteOrder)
	{
		m_childObjectsRenderPaused = true;
		m_deferredDeleteOrder();
		m_deferredDeleteOrder = nullptr;
	}
#endif

	return bNeedsDeletion;
}

MapObjectsData& Spawnable::getMapObjectsData()
{
	return m_mapObjectData;
}

void Spawnable::childrenVisiter(const std::function<void(Spawnable&)>& visiterFunction)
{
	if (!visiterFunction)
		return;

	std::erase_if(m_children, [this](const SpawnableChild& childHolder)
		{
			return childHolder.child.expired();
		});

	for (auto& childHolder : m_children)
	{
		if (!childHolder.child.expired())
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
#ifndef DEBUG
	return;
#endif
	m_rootNodeTransformSubsHandler = m_spRoot->subscribeToWorldTransformChanged([this](const Transform& trans)
		{
			m_mapObjectData.pos = trans.translation();
			if (neAbs(m_mapObjectData.pos.z) < 0.0001f)
				m_mapObjectData.pos.z = 0.f; //sometimes the gizmo puts in double values into Z field breaking ROs. Disable if you need non-zero Z placement

			if (m_mapObjectData.isChildObj && m_spRoot)
			{
				m_mapObjectData.local_pos = m_spRoot->getLocalTranslation();
				m_mapObjectData.local_scale = m_spRoot->getLocalScale();
				m_mapObjectData.local_rotation = m_spRoot->getLocalRotation().euler();
			}
			else
			{
				m_mapObjectData.scale = m_spRoot->getWorldScale();
				m_mapObjectData.rotation = m_spRoot->getWorldRotation().euler();
			}
			m_cachedBounds = std::array<w4::math::vec3, 2>(); //clearing plotter frame pos cache
		});

	addSpawnableSetting(
		SpawnSetting(
			"Name", m_mapObjectData.objId,
			[this, nameOld = m_mapObjectData.objId](std::string& name) mutable
			{
				if (nameOld == name) return;

				m_mapObjectData.objId = SPAWNER.checkUniqueName(name);
				SPAWNER.updateSpawnableNameRegistry(nameOld, m_mapObjectData.objId);
				nameOld = m_mapObjectData.objId;
				if (m_spRoot) m_spRoot->setName(m_mapObjectData.objId);
				ND_SPAWNER->m_needUpdateEntitiesFilter = true;
			}));
}

Spawnable::SpawnSetting& Spawnable::addChildMeshSelectorSetting(const std::string& name,
	std::string& value,
	std::function<void(std::string&)>&& onChangeCB)
{
	auto meshListGetter =
		[this, cachedMap = std::map<std::string, std::string>(), lastChildCount = getSpawnedChildMeshes().size()]() mutable
		-> std::map<std::string, std::string>
		{
			if (lastChildCount != getSpawnedChildMeshes().size())
			{
				cachedMap.clear();
				cachedMap["NONE"] = "NONE"; //should always have this
				for (auto& childHolder : getSpawnedChildMeshes())
				{
					if (childHolder.child.expired()) continue;
					cachedMap.try_emplace(childHolder.mmd.meshId, childHolder.mmd.meshId);
				}
				lastChildCount = getSpawnedChildMeshes().size() - 1;
			}
			if (cachedMap.empty()) cachedMap["NONE"] = "NONE";
			return cachedMap;
		};

	return addSpawnableSettingList(name, value, meshListGetter, std::move(onChangeCB));
}

Spawnable::SpawnSetting& Spawnable::addChildSelectorSetting(const std::string& name,
	std::string& value,
	std::vector<std::string> classNameFilters,
	std::function<void(std::string&)>&& onChangeCB)
{
	auto childrenListGetter =
		[this, classNameFilters, cachedMap = std::map<std::string, std::string>(), lastChildCount = getSpawnedChildren().size()]() mutable
		-> std::map<std::string, std::string>
		{
			if (lastChildCount != getSpawnedChildren().size())
			{
				cachedMap.clear();
				cachedMap["NONE"] = "NONE"; //should always have this
				for (auto& childHolder : getSpawnedChildren())
				{
					if (childHolder.child.expired()) continue;
					for (const auto& filter : classNameFilters)
					{
						if (childHolder.mod.className == filter)
						{
							auto child = childHolder.child.lock();
							cachedMap.try_emplace(child->getMapObjectsData().objId, child->getMapObjectsData().objId);
						}
					}
				}
				lastChildCount = getSpawnedChildMeshes().size() - 1;
			}
			if (cachedMap.empty()) cachedMap["NONE"] = "NONE";
			return cachedMap;
		};

	return addSpawnableSettingList(name, value, childrenListGetter, std::move(onChangeCB));
}

void Spawnable::addChildSpawner(std::function<void(sptr<Spawnable>)> onChildSpawnCb, bool childrenSpawnerOn)
{
	m_childrenAdderEnabled = childrenSpawnerOn;
	m_onChildSpawnCb = onChildSpawnCb;
}

void Spawnable::spawnAllChildren()
{
	if (m_self.expired()) //need self to be valid before making children. SpawnerFactory will manage the deferred call
	{
		m_deferredSpawnChildrenCall = [this](){spawnAllChildren();};
		return;
	}

	if (m_childrenInited) return;
	m_childrenInited = true;
	m_deferredSpawnChildrenCall = nullptr;

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

sptr<Spawnable> Spawnable::getParent() const
{
	if (!hasParent()) return nullptr;
	return m_parent.lock();
}

sptr<Mesh> Spawnable::getChildMeshByName(const std::string& name) const
{
	for (auto& rec : m_childMeshes)
	{
		if (rec.mmd.meshId == name)
		{
			if (rec.child.expired())
			{
				err("[Spawnable::getChildMeshByName]Found child mesh %s for %s, but wptr expired!", name.c_str(), m_mapObjectData.objId.c_str());
				return nullptr;
			}
			return rec.child.lock();
		}
	}

	err("[Spawnable::getChildMeshByName]Failed to find child mesh %s for %s", name.c_str(), m_mapObjectData.objId.c_str());
	return nullptr;
}

sptr<Spawnable> Spawnable::getChildByName(const std::string& name) const
{
	for (auto& rec : m_children)
	{
		if (rec.mod.objId == name)
		{
			if (rec.child.expired())
			{
				err("[Spawnable::getChildByName]Found child object {%s} for {%s}, but wptr expired!", name.c_str(), m_mapObjectData.objId.c_str());
				return nullptr;
			}
			return rec.child.lock();
		}
	}

	err("[Spawnable::getChildByName]Failed to find child object {%s} for {%s}", name.c_str(), m_mapObjectData.objId.c_str());
	return nullptr;
}

void Spawnable::SpawnableMainUi()
{
#ifdef DEBUG
	//root transforms and re-parenting ui
	if (m_spRoot)
	{
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
	// Universal setting render function
	static auto renderSetting = [](const std::string& settingName, SpawnSetting& Setting)
		{
			if (!Setting.isEnabled()) return;
			auto guiFunc = Setting.getOnGuiCb(); //function for rendeing setting in ImGui
			if (guiFunc)
			{
				if (guiFunc((settingName.empty() ? STR(repeatElemId) : settingName), Setting.settingValue)) //checking if setting was changed
				{
					Setting.onSettingChangeCB(Setting.settingValue);
				}
			}
		};
	//First rendering settings in folders
	for (auto& [folderName, SettingsMap] : m_spawnableSettings)
	{
		if (folderName == "") continue; //render this last
		if (ImGui::TreeNode(folderName.c_str()))
		{
			for (auto& [settingName, Setting] : SettingsMap)
				renderSetting(settingName, Setting);
			ImGui::TreePop();
		}
	}
	//then rendering settings without a folder
	for (auto& [settingName, Setting] : m_spawnableSettings[""])
		renderSetting(settingName, Setting);

	//last - we render buttons
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
#endif
}

void Spawnable::childSpawnerUI()
{
#ifdef DEBUG
	if (m_childObjectsRenderPaused)
		return;

	if (ImGui::BeginTabItem("Child Objects"))
	{
		//Child class selector
		static std::string childClass = "CLASS_NAME";
		childClass = SPAWNER.getClassesImGuiCombo(childClass, "Child of Class");

		if(childClass != "CLASS_NAME")
		{
			auto& spawnerRecord = SPAWNER.getSpawnerRecord(childClass);

			//Validating last selected parent node
			if (m_lastSelectedParentNode.expired())
				m_lastSelectedParentNode = getSpawnableRoot();
			auto defaultNode = spawnerRecord.getConstructNode();
			spawnerRecord.setNewConstructNode(m_lastSelectedParentNode.lock());
			
			//making a copy of default Mod and replacing with cached child spawner mod
			if (ImGui::TreeNode("Spawn settings"))
			{
				//temporarily replacing spawner-record's parent node
				
				//New child spawn arguements gui
				for (auto& spawnArgGui : spawnerRecord.getConstructorArgsUiRender())
					if (spawnArgGui) spawnArgGui();

				//restoring spawner-record's parent node
				m_lastSelectedParentNode = spawnerRecord.getConstructNode();

				ImGui::TreePop();
			}

			if (ImGui::Button(("Spawn " + childClass + " child").c_str()))
			{
				//creating sptr<MapObjectsData> for parent's MOD and for spawner
				sptr<MapObjectsData> newChildMOD = make::sptr<MapObjectsData>();
				*newChildMOD = spawnerRecord.getMapObjectsData(); //copying data from Spawner's copy of selecetd class' MOD
				newChildMOD->className = childClass;
				newChildMOD->isChildObj = true;
				if (newChildMOD->objId.empty())
					newChildMOD->objId = "Child_" + childClass;

				//adding new child holder and spawning child
				m_mapObjectData.childObjects.push_back(newChildMOD);
				auto newChildHolder = SpawnableChild(*m_mapObjectData.childObjects.back());
				auto newChild = spawnChild(newChildMOD, newChildHolder, true);
				if (!m_lastSelectedParentNode.expired() && newChild->m_spRoot)
					m_lastSelectedParentNode.lock()->addChild(newChild->m_spRoot);
				m_children.push_back(newChildHolder);
			}
			if (spawnerRecord.getMapObjectsData().objName.empty())
			{
				ImGui::SameLine();
				ImGui::TextColored(IMG_ORANGE, "%s", "objName is empty!");
			}
			spawnerRecord.setNewConstructNode(defaultNode);
		}
		ImGui::Separator();
		ImGui::Separator();

		if (m_children.empty())
		{
			if (getSpawnableRoot())
				ImGui::TextColored(IMG_RED, "%s", (getSpawnableRoot()->getName() + " has no child objects").c_str());

			ImGui::NewLine();
			ImGui::EndTabItem();
			return;
		}

		if (getSpawnableRoot())
			ImGui::TextColored(IMG_WHITE, "%s", (getSpawnableRoot()->getName() + " child objects:").c_str());
		//if (m_lastSelectedChildMesh.empty()) m_lastSelectedChildMesh = childList.begin()->first;
		//list box with child objects
		auto selectionResult = ND_Util::ImGuiListBox<SpawnableChild>(
			"Child Objects",
			m_children,
			m_lastSelectedChildObj,
			[this](const SpawnableChild& rec)
			{
				//info("[%s] %s", m_mapObjectData.objId.c_str(), rec.mod.objId.c_str());
				return rec.mod.objId; 
			}
		);
		if(selectionResult.first && selectionResult.second.bSpawned)
			m_lastSelectedChildObj = selectionResult.second.mod.objId;

		if (m_lastSelectedChildObj.empty())
		{
			ImGui::NewLine();
			ImGui::EndTabItem();
			return;
		}

		//for some reason ImGuiListBox does not always return valid SpawnableChild - looking up by name instead
		auto selectedChildIt = std::find_if(m_children.begin(), m_children.end(), 
			[this](const SpawnableChild& childHolder) {return childHolder.mod.objId == m_lastSelectedChildObj;});

		if (selectedChildIt == m_children.end()) //child not found for some reason
		{
			ImGui::EndTabItem();
			return;
		}

		auto& selectedChildHolder = *selectedChildIt;

		//rendering selected Child's onGui and handling deletion requests
		if (ImGui::BeginChild(selectedChildHolder.mod.objId.c_str(), ND_Util::getSizeForChild(0.7), true, ImGuiWindowFlags_AlwaysAutoResize))
		{
			if (!selectedChildHolder.child.expired()) 
			{
				//Spawned child's settings (same as opeing it up in Spawned Objects subtab)
				//returns true if child requested deletion (via DELETE btn)
				bool needsToBeDeleted = selectedChildHolder.child.lock()->onGui(("Child " + selectedChildHolder.mod.objId).c_str());
				
				if (needsToBeDeleted)
				{
					std::string deletingClassName = selectedChildHolder.mod.className;
					std::string deletingObjId = m_lastSelectedChildObj;
					size_t deletingObjUid = selectedChildHolder.child.lock()->getUid();
					//wptr<Spawnable> deletedObjWptr = selectedChildHolder.child;
					m_childObjectsRenderPaused = true;
					m_deferredDeleteOrder = [this, deletingClassName, deletingObjId, deletingObjUid]()
						{
							SPAWNER.removeObjectByUid(deletingObjUid);
							std::erase_if(m_children, [this, deletingObjUid](const SpawnableChild& childHolder)
								{
									if (childHolder.child.expired())
										return true;
									return deletingObjUid == childHolder.child.lock()->getUid();
								});
							std::erase_if(m_mapObjectData.childObjects, [this, deletingObjId](const sptr<MapObjectsData>& childMod)
								{
									if (!childMod)
										return true;
									return deletingObjId == childMod->objId;
								});
							
							m_childObjectsRenderPaused = false;
						};
					m_lastSelectedChildObj.clear();
				}
			}
			else //child expired but for some reason the child holder persisted
			{
				m_childObjectsRenderPaused = true;
				std::string deletingObjId = m_lastSelectedChildObj;
				m_deferredDeleteOrder = [this, deletingObjId]()
					{
						std::erase_if(m_mapObjectData.childObjects, [this, deletingObjId](const sptr<MapObjectsData>& childMod)
							{
								if (!childMod)
									return true;
								return deletingObjId == childMod->objId;
							});
						std::erase_if(m_children, [this, deletingObjId](const SpawnableChild& childHolder)
							{
								return deletingObjId == childHolder.mod.objId;
							});

						m_childObjectsRenderPaused = false;
					};
				m_lastSelectedChildObj.clear();
			}
		}
		ImGui::EndChild(); //ends selected child object's child form 

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

void Spawnable::updateChildrenData()
{
	std::function<void(SpawnableChildMesh&)> visiterMesh = [](SpawnableChildMesh& meshHolder) {
			auto child = meshHolder.child.lock();
			meshHolder.mmd.node = SPAWNER.getNodeHierarchyAddress(child->getParent());
			meshHolder.mmd.renderOrder = child->getRenderOrder();
			meshHolder.mmd.useWorldPos = false;
			meshHolder.mmd.position_local = child->getLocalTranslation();
			meshHolder.mmd.scale_local = child->getLocalScale();
			meshHolder.mmd.rotation_local = child->getLocalRotation().euler();
#ifndef FORCENODEDEBUG_OFF
			SP_Utils::exportMaterialParams(child->getMaterialInst(), meshHolder.mmd);
#endif
			if (neAbs(meshHolder.mmd.position_local.z) < 0.001) meshHolder.mmd.position_local.z = 0.f;
		};

	//updating m_mapObjectData.childObjects via visiter since our sptrs are separated from children's MapObjectsData
	std::function<void(Spawnable&)> visiter = [visiterMesh](Spawnable& parent)
		{
			parent.childrenVisiter(visiterMesh);
			for (size_t i = 0; i < parent.m_children.size(); i++)
			{
				if (parent.m_mapObjectData.childObjects.size() > i && parent.m_children[i].bSpawned && !parent.m_children[i].child.expired())
				{
					*parent.m_mapObjectData.childObjects[i] = parent.m_children[i].child.lock()->m_mapObjectData;
				}
			}
		};

	childrenVisiter(visiter);
}

void Spawnable::onSelected()
{
	//sp_warn("Selected");
	//focus frame render
	if (m_framePlotter && m_framePlotter->getParent())
		m_framePlotter->getParent()->removeChild(m_framePlotter);
	if(getSpawnableRoot())
	{
		auto bounds = getSpawnableBounds(getSpawnableRoot(), true);
		updateSpawnableCollider(bounds);
		m_framePlotter = Plotter::buildCube(bounds, vec4(1.f, 0.f, 0.f, 1.f));
		m_framePlotter->setWorldRotation(getSpawnableRoot()->getWorldRotation());
		if (m_spRoot->getRoot())
			m_spRoot->getRoot()->addChild(m_framePlotter); //todo: consider adding DebugUiPass and attach ImGui + this to its root to always render on top
		else
			Render::getRoot()->addChild(m_framePlotter);
	}

	// Collider auto sorting
	m_colliderPriorityResetRaycast = true;
}

void Spawnable::onDeselected()
{
	//sp_warn("Deselected");
	if (m_framePlotter && m_framePlotter->getParent())
		m_framePlotter->getParent()->removeChild(m_framePlotter);
};

bool Spawnable::hasChildMesh(const std::string& name) const
{
	for (const auto& rec : m_childMeshes)
	{
		if (rec.mmd.meshId == name)
			return true;
	}
	return false;
}

bool Spawnable::hasChildObj(const std::string& name) const
{
	for (const auto& rec : m_children)
	{
		if (rec.mod.objId == name)
			return true;
	}
	return false;
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
		{
			m_lastSelectedParentNode = resultHolder;
		}

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
		if (!hasChildMesh(m_lastSelectedChildMesh)) //todo: necessary hack until figure out overspill of selected child mesh name of previously selected object
			m_lastSelectedChildMesh = "";
		if(m_lastSelectedChildMesh.empty())
		{
			ImGui::NewLine();
			ImGui::EndTabItem();
			return;
		}

		//form for selected child mesh
		if (ImGui::BeginChild(selectionResult.second.mmd.meshId.c_str(), ND_Util::getSizeForChild(0.6), true, ImGuiWindowFlags_AlwaysAutoResize))
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
					{
						//name collision check
						for (auto& childHolder : m_childMeshes)
						{
							if (childHolder.mmd.meshId == meshRec.mmd.meshId && childHolder.child.lock() != meshRec.child.lock()) //names collision!
							{
								meshRec.mmd.meshId.insert(0, "!");
								meshRec.mmd.meshId.push_back('!');
							}
						}
						spawnedMesh->setName(meshRec.mmd.meshId);
						m_lastSelectedChildMesh = meshRec.mmd.meshId;
					}
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

		ImGui::EndChild(); //Mesh-child child form's end

		ImGui::NewLine();
		ImGui::EndTabItem(); 
	}
#endif
}

sptr<Spawnable> Spawnable::spawnChild(sptr<MapObjectsData> childData, SpawnableChild& childHolder, bool bTabSpawned)
{
	auto& spawnerRecord = SPAWNER.getSpawnerRecord(childData->className);
	auto defaultNode = spawnerRecord.getConstructNode();
	sptr<Node> spawnParentNode = SPAWNER.findNodeByHierarchyAddress(childData->node);
	vec3 local_pos = childData->local_pos;
	if (spawnParentNode)
		spawnerRecord.setNewConstructNode(spawnParentNode);
	sptr<Spawnable> child = SPAWNER.spawnObjectOfClass(
		spawnerRecord,
		*childData,
		m_onChildSpawnCb, //uses default onSpawnCb if m_onChildSpawnCb == nullptr
		bTabSpawned
	);
	child->m_parent = m_self;

	if (spawnParentNode) //restoring default spawn node
		spawnerRecord.setNewConstructNode(defaultNode);
	else if (m_spRoot && child->m_spRoot) //fallback parenting logic
		m_spRoot->addChild(child->m_spRoot);

	if (child && child->getSpawnableRoot())
	{
		child->getSpawnableRoot()->setLocalTranslation(local_pos);
		childData->local_pos = local_pos; //in case it was overridden
		child->getSpawnableRoot()->setLocalScale((childData->local_scale));
		child->getSpawnableRoot()->setLocalRotation(Rotator(childData->local_rotation));
	}

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
	if (!child)
	{
		warn("[Spawnable::spawnChildMesh]Failed to spawn %s with id %s!", childData->meshName.c_str(), childData->meshId.c_str() );
		return nullptr;
	}
	else if (!child->getParent())
	{
		debug("[Spawnable::spawnChildMesh] Spawned parentless child mesh %s. Hooking to root node!", childData->meshId.c_str());
		if (m_spRoot)
		{
			m_spRoot->addChild(child);
			if (!childData->useWorldPos)
				child->setLocalTranslation(childData->position_local);
			child->setLocalScale(childData->scale_local);
			child->setLocalRotation(Rotator(childData->rotation_local));
		}
	}
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