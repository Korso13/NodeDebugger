#include "SpawnerWrappers.h"
#include "game/playerController/PlayerController.h"
#include "others/utils/ScreenSpaceShader.h"
#include "ui/PlacementStrategies.h"

SceneObject::SceneObject(MapObjectsData& mod, sptr<Node> parentNode, bool bSpawnChildren, bool bCallSpawnableInit):
   Spawnable(mod),
m_needToSpawnChildren(bSpawnChildren)
{
   std::string rootName = (!mod.objId.empty()) ? mod.objId : mod.objName;
   m_root = M_SPTR<Node>(mod.objId);
   parentNode->addChild(m_root);
   //m_mapObjectData.local_pos = mod.local_pos;
   if (bCallSpawnableInit)
      SceneObject::initSpawnable(m_root);
}

SceneObject::~SceneObject()
{
   m_root->removeAllChildren();
   if (auto parent = m_root->getParent(); parent)
      parent->removeChild(m_root);
}

MapObjectsData& SceneObject::getMapObjectsData()
{
   if (m_mapObjectData.isChildObj)
      m_mapObjectData.local_pos = m_root->getLocalTranslation();
   if (m_placementStrategy)
   {
      m_placementStrategy->onResize();
      PlacementStrategies::exportStrategyToMOD(m_mapObjectData, m_placementStrategy);
   }
   return Spawnable::getMapObjectsData();
}

std::vector<wptr<Mesh>> SceneObject::getAllChildMeshes(bool bForceRenewCache)
{
   if (m_childMeshesCache.size() == getSpawnedChildMeshes().size() && !bForceRenewCache)
      return m_childMeshesCache;

   for (auto& holder : getSpawnedChildMeshes())
      if (holder.bSpawned && !holder.child.expired())
         m_childMeshesCache.push_back(holder.child);

   return m_childMeshesCache;
}

sptr<Spawnable> SceneObject::getChildObjectById(const std::string& name)
{
   for (auto& holder : getSpawnedChildren())
      if (holder.bSpawned && !holder.child.expired())
         if (holder.child.lock()->getMapObjectsData().objId == name)
            return holder.child.lock();

   info("[SceneObject::getChildObjectById](%s) Couldn't find child %s", m_mapObjectData.objId.c_str(), name.c_str());
   return nullptr;
}

void SceneObject::spawnableSelfUpdate(float dt)
{
   for (auto& child : getSpawnedChildren())
      if (!child.child.expired())
         child.child.lock()->spawnableSelfUpdate(dt);
}

void SceneObject::forceUpdatePlacementStrategy()
{
   if (m_placementStrategy) m_placementStrategy->onResize(Render::getAspect());
}

void SceneObject::initSpawnable(sptr<Node> rootNode)
{
   Spawnable::initSpawnable(rootNode);
   addChildSpawner(std::bind(&SceneObject::onChildSpawned, this, std::placeholders::_1));

   addSpawnableSetting(SpawnSetting("Object id",m_mapObjectData.objId,
      [this](std::string& var)
         {
            if (m_root) m_root->setName(var);
         }));

   if (m_placementStrategy)
      addSpawnableSetting(SpawnSetting("Placement Strategy",m_placementStrategy));

   addSpawnableSettingList("[SceneObject] Mesh for Placement Strategy",
      m_mapObjectData.settings.strs["SO_meshForPS"],
      [this]() -> std::map<std::string, std::string>
         {
            std::map<std::string, std::string> meshList = {{"NONE", "NONE"}};
            for (auto& child : m_mapObjectData.childMeshes)
               if (child)
                  meshList.try_emplace(child->meshId, child->meshId);
            return meshList;
         },
      [this](std::string& var)
         {
            if (var == "NONE") return;
            auto meshForPS = getChildMeshByName(var);
            if (!meshForPS) return;
            if (m_placementStrategy)
            {
               m_placementStrategy->setContext(m_root, meshForPS, m_placementStrategy->getDistanceToCameraSet());
               m_placementStrategy->onResize();
            }
            else
            {
               setupPlacementStrategy();
            }
         }
   );
}

void SceneObject::onChildSpawned(sptr<Spawnable> newChild)
{
    if (!newChild)
        return;
    if (m_root && newChild->getSpawnableRoot())
    {
        m_root->addChild(newChild->getSpawnableRoot());
        newChild->getSpawnableRoot()->setLocalTranslation(newChild->getMapObjectsData().local_pos);
    }
}

void SceneObject::onBeginPlay(sptr<Spawnable> selfRef, bool isTabSpawned, vec3 spawnLocation)
{
   if (isTabSpawned && !m_mapObjectData.isChildObj)
      setWorldPos(spawnLocation);
   else if (m_mapObjectData.isChildObj)
      setLocalPos(m_mapObjectData.local_pos);
   else
      setWorldPos(m_mapObjectData.pos);

   if (m_needToSpawnChildren)
      spawnAllChildren();

   if (!m_mapObjectData.settings.strs["SO_meshForPS"].empty())
      SceneObject::setupPlacementStrategy();
   PLAYER_CONTROLLER.addObject(selfRef);
}

void SceneObject::onEndPlay(sptr<Spawnable> selfRef, const std::string& objId)
{
   PLAYER_CONTROLLER.removeObject(selfRef);
}

void SceneObject::setupPlacementStrategy()
{
   if (m_placementStrategy)
      return;
   auto meshForPS = getChildMeshByName(m_mapObjectData.settings.strs["SO_meshForPS"]);
   if (!meshForPS)
   {
      warn("[SceneObject::setupPlacementStrategy] Failed to find child mesh %s for placement strategy!", m_mapObjectData.settings.strs["SO_meshForPS"].c_str());
      return;
   }

   m_placementStrategy = PlacementStrategies::makeStrategyFromMOD(m_mapObjectData, m_root, meshForPS);
   addSpawnableSetting(SpawnSetting("Placement Strategy",m_placementStrategy));
}

SPKParticle_S::SPKParticle_S(MapObjectsData& spawnData, sptr<Node> root)
      :
      Spawnable(spawnData),
      SPKParticle(spawnData.objName, root)
{
   SPKParticle_S::setWorldPos(spawnData.pos);
   loadMaterialsData();
   m_node->setName(spawnData.objName);
   if (spawnData.settings.integers["RO"] == 0)
      spawnData.settings.integers["RO"] = 2;

   setRenderOrder(spawnData.settings.integers["RO"]);

   if (!spawnData.settings.flags["use_w_scale"] && spawnData.settings.vec3s["l_scale"] != vec3::zero)
   {
      m_node->setLocalScale(spawnData.settings.vec3s["l_scale"]);
   }
   else if (spawnData.settings.vec3s["w_scale"] != vec3::zero)
   {
      m_node->setWorldScale(spawnData.settings.vec3s["w_scale"]);
   }
   flattenSPKSystem(m_node);

#ifdef DEBUG
   SPKParticle_S::initSpawnable(SPKParticle::getRoot());
#else
   Spawnable::initSpawnable(m_node);
#endif // DEBUG
}

void SPKParticle_S::restart()
{
   restartSPKEmitter(getMainSystem());
}

void SPKParticle_S::loadMaterialsData()
{
   if (!getMapObjectsData().SPARK_Materials.empty())
   {
      for (size_t i = 0; i < m_system->getNbGroups(); i++)
      {
         if (getMapObjectsData().SPARK_Materials.size() <= i)
            continue;

         applySpkMaterialToGroup(m_system, toSPKBlend(getMapObjectsData().SPARK_Materials[i]), i);
      }
   }
   else
   {
      //prevents memory re-allocations and crashes due to pointer invalidation
      getMapObjectsData().SPARK_Materials.reserve(m_system->getNbGroups());
      getMapObjectsData().SPARK_Materials.resize(m_system->getNbGroups());

      for (size_t i = 0; i < m_system->getNbGroups(); i++)
      {
         //guessing correct material based on texture extension or material name (material takes priority)
         //requires patched SparkParticleSystemQuadRenderer.h !
         auto rendererAttrib = m_system->getGroup(i)->getRenderer()->exportAttributes();
         auto TextureAttrib = rendererAttrib.getAttributeWithValue("texture");
         auto MaterialAttrib = rendererAttrib.getAttributeWithValue("material");
         bool setMaterial = false;
         if (MaterialAttrib)
         {
            std::string materialName = MaterialAttrib->getValue<std::string>();
            if (!materialName.empty())
            {
               setMaterial = true;
               if (materialName == "add_alpha")
                  getMapObjectsData().SPARK_Materials[i] = SPK_MATERIAL::ADDALPHA;
               else if (materialName == "alpha")
                  getMapObjectsData().SPARK_Materials[i] = SPK_MATERIAL::ALPHA;
            }
         }
         if (TextureAttrib && !setMaterial)
         {
            auto textureName = TextureAttrib->getValue<std::string>();
            if (textureName.find(".png") != std::string::npos)
               getMapObjectsData().SPARK_Materials[i] = SPK_MATERIAL::ALPHA;
            else if (textureName.find(".jpg") != std::string::npos)
               getMapObjectsData().SPARK_Materials[i] = SPK_MATERIAL::ADDALPHA;
         }
      }
   }
}

MapObjectsData& SPKParticle_S::getMapObjectsData()
{
   m_mapObjectData.settings.vec3s["l_scale"] = m_node->getLocalScale();
   m_mapObjectData.settings.vec3s["w_scale"] = m_node->getWorldScale();

   return Spawnable::getMapObjectsData();
}

void SPKParticle_S::initSpawnable(sptr<Node> rootNode)
{
	Spawnable::initSpawnable(rootNode);

	//Orientation and material settings
	for (size_t i = 0; i < m_system->getNbGroups(); i++)
	{
		addSpawnableSetting(SpawnSetting(
			"Material Group " + STR(i),
			getMapObjectsData().SPARK_Materials[i],
			[this, i](SPK_MATERIAL& material)
			{
				getMapObjectsData().SPARK_Materials[i] = material; //doesn't work well without it
				loadMaterialsData();
			}
		));

		//for debugging, not exported for now
		addSpawnableSetting(SpawnSetting(
			"Group Orientation " + STR(i),
			SPK_ORIENTATION::CAMERA_PLANE_ALIGNED,
			[this, i](SPK_ORIENTATION orientation)
			{
				setRendererOrientation(m_system, i, toSPKOrient(orientation));
			}
		));
	}

	addSpawnableSetting(SpawnSetting(
		"Render Order",
		m_mapObjectData.settings.integers["RO"],
		[this](int& value)
		{
			setRenderOrder(value);
		}
	));

	addSpawnableSetting(SpawnSetting(
		"Use World Scale?",
		m_mapObjectData.settings.flags["use_w_scale"],
		[this](bool& val)
		{
			if (val)
			{
				addSpawnableSetting(SpawnSetting(
					"Scale World",
					m_mapObjectData.settings.vec3s["w_scale"],
					[this](vec3& value)
					{
						m_node->setWorldScale(value);
						flattenSPKSystem(m_node);
					}
				));
				removeSpawnableSetting("Scale Local");
			}
			else
			{
				addSpawnableSetting(SpawnSetting(
					"Scale Local",
					m_mapObjectData.settings.vec3s["l_scale"],
					[this](vec3& value)
					{
						m_node->setLocalScale(value);
						flattenSPKSystem(m_node);
					}
				));
				removeSpawnableSetting("Scale World");
			}
		}
	));

	if(m_mapObjectData.settings.flags["use_w_scale"])
	{
		addSpawnableSetting(SpawnSetting(
			"Scale World",
			m_mapObjectData.settings.vec3s["w_scale"],
			[this](vec3& value)
			{
				m_node->setWorldScale(value);
				flattenSPKSystem(m_node);
			}
		));
	}
	else
	{
		addSpawnableSetting(SpawnSetting(
			"Scale Local",
			m_mapObjectData.settings.vec3s["l_scale"],
			[this](vec3& value)
			{
				m_node->setLocalScale(value);
				flattenSPKSystem(m_node);
			}
		));
	}

	addSpawnableBtn("Flatten", [this]() {flattenSPKSystem(m_node); });
	addSpawnableBtn("Restart", [this]() {restartSPKEmitter(m_system); });
}
