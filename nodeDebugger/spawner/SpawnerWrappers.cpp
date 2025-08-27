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
      newChild->getSpawnableRoot()->setLocalTranslation(newChild->getMapObjectsData().pos);
   }
}

void SceneObject::onBeginPlay(sptr<Spawnable> selfRef, bool isTabSpawned, vec3 spawnLocation)
{
   if (isTabSpawned)
      setWorldPos(spawnLocation);
   else
      setWorldPos(m_mapObjectData.pos);
   //SPAWNER.m_nodeHierarchySearchLogging = true;
   if (m_needToSpawnChildren)
      spawnAllChildren();
   //SPAWNER.m_nodeHierarchySearchLogging = false;
   if (!m_mapObjectData.settings.strs["SO_meshForPS"].empty())
      setupPlacementStrategy();
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
