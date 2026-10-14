#include "UiElement.h"

#include "../PlacementStrategies.h"
#include "common/nodeDebugger/NodeDebugger.h"
#include "others/utils/ScreenSpaceShader.h"
#include "ui/UiText_S.h"

void UiElement::Event::emit(ScreencastEvent eventType, const CollisionInfo& hit)
{
   if (m_callback) m_callback(eventType, hit);
}

UiElement::UiElement(MapObjectsData& mod, sptr<Node> parentNode)
   :
   SceneObject(mod, parentNode, true, true)
{
   if (!m_mapObjectData.settings.flags.contains("UIE_selfEnabledOnStart"))
      m_mapObjectData.settings.flags["UIE_selfEnabledOnStart"] = true;
   if (!m_mapObjectData.settings.flags.contains("UIE_childrenEnabledOnStart"))
      m_mapObjectData.settings.flags["UIE_childrenEnabledOnStart"] = true;
   if (m_mapObjectData.objName.empty() || m_mapObjectData.objName == "TEMPLATE_RECORD")
      m_mapObjectData.objName = m_mapObjectData.objId;

   m_uiEvents.reserve(5);
}

void UiElement::spawnableSelfUpdate(float dt)
{
   SceneObject::spawnableSelfUpdate(dt);
   AutoTweens::update(dt);
}

sptr<UiElement::Event> UiElement::subscribeToTouchEvents(
   std::function<void(ScreencastEvent eventType, const CollisionInfo& hit)>&& callback)
{
   if (!m_clickCollider) initCollider();
   m_receivesClicks = true;
   sptr<UiElement::Event> newEvent = M_SPTR<UiElement::Event>(std::move(callback));
   m_uiEvents.push_back(newEvent);

   return newEvent;
}

void UiElement::setDetectTouches(bool bDetectTouches)
{
   if (!m_clickCollider) initCollider();
   m_clickCollider->setBlockScreencasts(bDetectTouches);
   m_receivesClicks = bDetectTouches;
}

void UiElement::enableSelf(bool isEnabled)
{
   //debug("[UiElement::enableSelf](%s)", getObjId().c_str());
   m_isEnabled = isEnabled;

   //toggling meshes
   for (auto& childHolder : getSpawnedChildMeshes())
   {
      if (childHolder.child.expired()) continue;
      childHolder.child.lock()->setEnabled(isEnabled);
   }

   for (const auto& childHolder : getSpawnedChildren())
   {
      if (childHolder.child.expired())
         continue;
      auto child = childHolder.child.lock();
      if (child->getMapObjectsData().className == "UiElement" || m_uiChildren.contains(child->getUid()))
         continue;

      //toggling non-ui children
      if (child->getSpawnableRoot())
         child->getSpawnableRoot()->setEnabled(isEnabled);
   }
   if (m_clickCollider) setDetectTouches(isEnabled);
}

void UiElement::enableChildren(bool isEnabled, bool applyRecursively)
{
   m_isEnabledChildren = isEnabled;
   bool needsClearing = false;
   //debug("[UiElement::enableChildren] uiChildren found %d", m_uiChildren.size());
   for (const auto& [uid, child] : m_uiChildren)
   {
      if (child.expired())
      {
         needsClearing = true;
         continue;
      }
      child.lock()->enableSelf(isEnabled);
      if (applyRecursively)
         child.lock()->enableChildren(isEnabled, applyRecursively);
   }
}

vec2 UiElement::getUiElementSize(bool bForceRecalculate)
{
   if (m_invisibleBounds && !bForceRecalculate)
      return getMeshPlaneSize(m_invisibleBounds).withoutZ();

   m_uiBounds = getUiBounds(m_boundsCheckDepth);
   return {m_uiBounds[1].x - m_uiBounds[0].x, m_uiBounds[1].y - m_uiBounds[0].y};
}

int UiElement::getScreenId()
{
   return m_mapObjectData.settings.integers["UIE_screenId"];
}

void UiElement::initSpawnable(sptr<Node> rootNode)
{
   //Doesn't need to call parent impl of initSpawnable()
#ifdef  DEBUG
   addSpawnableSetting(SpawnSetting(
      "Is enabled?",
      m_isEnabled,
      [this](bool& var)
         {
            enableSelf(var);
            m_mapObjectData.settings.flags["UIE_selfEnabledOnStart"] = var;
         }
   ));
   addSpawnableSetting(SpawnSetting(
      "Are children enabled?",
      m_isEnabledChildren,
      [this](bool& var)
         {
            enableChildren(var);
            m_mapObjectData.settings.flags["UIE_childrenEnabledOnStart"] = var;
         }
   ));
   addSpawnableSetting(SpawnSetting(
      "Ui Screen Id(optional)",
      m_mapObjectData.settings.integers["UIE_screenId"]
   ));

   //overriding SceneObjects' setting
   removeSpawnableSetting("[SceneObject] Mesh for Placement Strategy");
   addSpawnableSettingList("[UiElement] Mesh for Placement Strategy",
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
            m_mapObjectData.settings.flags["UIE_useBoundsForPlacement"] = false;
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

   addSpawnableSetting(SpawnSetting(
      "Show Debug Bounds?",
      false,
      [this](auto& val)
         {
            if (val)
            {
               SAFE_PARAM(m_invisibleBounds, "color", vec4(1.f,1.f, 1.f, 0.7));
               m_invisibleBounds->setRenderOrder(11000);
            }
            else
            {
               SAFE_PARAM(m_invisibleBounds, "color", vec4(1.f,1.f, 1.f, 0.f));
               m_invisibleBounds->setRenderOrder(1);
            }
         }
   ));


   addSpawnableBtn("Update ui bounds", std::bind(&UiElement::initInvisibleBounds, this));
   addSpawnableBtn("Use ui bounds for placement",
      [this]()
         {
            initInvisibleBounds();
            m_mapObjectData.settings.strs["SO_meshForPS"].clear();
            m_mapObjectData.settings.flags["UIE_useBoundsForPlacement"] = true;
            if (m_placementStrategy)
            {
               m_placementStrategy->setContext(m_root, m_invisibleBounds, m_placementStrategy->getDistanceToCameraSet());
               m_placementStrategy->onResize();
            }
            else
            {
               setupPlacementStrategy();
            }
         });
#endif
}

void UiElement::onChildSpawned(sptr<Spawnable> newChild)
{
   if (!newChild)
      return;

   if (std::dynamic_pointer_cast<UiElement>(newChild))
   {
      sptr<UiElement> child = cast_sptr<UiElement>(newChild);
      debug("[UiElement::onChildSpawned] Setting %s as parent for %s", getObjId().c_str(), child->getObjId().c_str());
      child->setParentUi(m_selfWeakRef);
      child->m_uiLayer = m_uiLayer + 1;
   }
   else if (newChild->getMapObjectsData().className == "UiText_S")
   {
      cast_sptr<UiText_S>(newChild)->subscribeToEvent("onTextChanged",
         [this](auto, auto)
            {
               updateUiHierarchy();
            });
   }
   updateUiHierarchy();

   if (newChild->isSpawned)
   {
      initInvisibleBounds();
   }
   SceneObject::onChildSpawned(newChild);
}

void UiElement::initCollider()
{
   if (m_clickCollider) return;
   vec3 colliderSize = {m_uiBounds[1].x - m_uiBounds[0].x, m_uiBounds[1].y - m_uiBounds[0].y, 0.1};
   m_clickCollider = m_root->addCollider<AABB>(m_mapObjectData.objId + "_collider(UiElement)", colliderSize);
   m_clickCollider->setScreencastCallback(
      ScreencastEvent::Down,
      std::bind(&UiElement::emitAllEvents, this, ScreencastEvent::Down, std::placeholders::_1));
   m_clickCollider->setScreencastCallback(
      ScreencastEvent::Up,
      std::bind(&UiElement::emitAllEvents, this, ScreencastEvent::Up, std::placeholders::_1));
   m_clickCollider->setBlockScreencasts(true);
   m_clickCollider->setLocalTranslation(vec3(0.f, 0.f, m_uiLayer / 1000.f));
}

void UiElement::setParentUi(wptr<UiElement> parent)
{
   //removing self from old parent's child ui list
   if (!m_parent.expired())
      m_parent.lock()->m_uiChildren.erase(getUid());

   //detached from parent
   if (parent.expired())
   {
      auto parentNode = m_root->getParent();
      if (parentNode) parentNode->removeChild(m_root);
      m_parent.reset();
      info("[UiElement::setParentUi] Null parent. Detached UiElement %s from parent!", getObjId().c_str());
      return;
   }

   m_parent = parent;
   auto parentSptr = parent.lock();
   debug("[UiElement::setParentUi] Registering %s as %s's child", getObjId().c_str(), parentSptr->getObjId().c_str());
   parentSptr->m_uiChildren[getUid()] = m_selfWeakRef; //registering self as child ui of a new parent
   m_root->setParent(parentSptr->getRoot()); //any SceneObject is supposed to have root for the entire lifetime
   m_root->setLocalTranslation(m_mapObjectData.local_pos); //vec3::zero by default
}

std::array<vec3, 2> UiElement::getUiBounds(size_t childRecursiveDepth)
{
   constexpr const static vec3 bDefaultMin = vec3(std::numeric_limits<float>::max());
   constexpr const static vec3 bDefaultMax = vec3(std::numeric_limits<float>::lowest());
   vec3 bmin = vec3(std::numeric_limits<float>::max());
	vec3 bmax = vec3(std::numeric_limits<float>::lowest());

   std::array<vec3, 2> bounds;
   //this lambda collects data on child meshes' vertices and updates UiElement bounds data
   std::function<void(Spawnable&, Spawnable&)> recursiveBoundsPoller =
      [&bmin, &bmax](Spawnable& uiElement, Spawnable& superParent)
      {
         //debug("[UiElement::getUiBounds] Checking child meshes of %s", uiElement.getMapObjectsData().objId.c_str());
         for (auto& childHolder: uiElement.getSpawnedChildMeshes())
         {
            if (childHolder.child.expired()) continue;
            auto mesh = childHolder.child.lock();
            for (auto it = mesh->getGeometryIterator(); it; ++it)
            {
               bmin = std::min(bmin, it->pos * mesh->getWorldScale() + mesh->getWorldTranslation());
               bmax = std::max(bmax, it->pos * mesh->getWorldScale() + mesh->getWorldTranslation());
            }
            info("[UiElement::getUiBounds] After mesh %s min is {%s}, max is {%s}", childHolder.mmd.meshId.c_str(), CVEC(bmin), CVEC(bmax));
         }

         //special handler for texts
         if (uiElement.getMapObjectsData().className == "UiText_S")
         {
            auto text = (static_cast<UiText_S&>(uiElement)).getText();
            for (auto& sdfNode : text->getTextNodes())
            {
               for (auto it = sdfNode->getGeometryIterator(); it; ++it)
               {
                  bmin = std::min(bmin, it->pos * sdfNode->getWorldScale() + sdfNode->getWorldTranslation());
                  bmax = std::max(bmax, it->pos * sdfNode->getWorldScale() + sdfNode->getWorldTranslation());
               }
               info("[UiElement::getUiBounds] After sdfNode of %s min is {%s}, max is {%s}", uiElement.getMapObjectsData().objId.c_str(), CVEC(bmin), CVEC(bmax));
            }
         }
      };

   static size_t childDepth, maxDepth;
   childDepth = 0;
   maxDepth = childRecursiveDepth;
   //this lambda recursively visits child hierarchy and polls their meshes with boundsPoller (recursiveBoundsPoller)
   static std::function<void(std::function<void(Spawnable&, Spawnable&)>, Spawnable&, Spawnable&)> childVisiter =
      [](std::function<void(Spawnable&, Spawnable&)> boundsPoller, Spawnable& child, Spawnable& superParent)
      {
         if (childDepth > maxDepth) //NOTE: depth limiter has not been untested!
         {
            //debug("[UiElement::getUiBounds] Out of set depth of %d with depth %d", maxDepth, childDepth);
            return;
         }

         childDepth++;
         //debug("[UiElement::getUiBounds] Polling children of %s at depth %d", child.getMapObjectsData().objId.c_str(), childDepth);
         for (auto& childHolder : child.getSpawnedChildren())
         {
            if (!childHolder.child.expired())
            {
               childVisiter(boundsPoller, *childHolder.child.lock(), superParent);
            }
         }
         boundsPoller(child, superParent);
         childDepth--;
      };

   recursiveBoundsPoller(*this, *this);
   if (childRecursiveDepth > 0)
   {
      childDepth++;
      //debug("[UiElement::getUiBounds] Polling children of %s at depth %d", getMapObjectsData().objId.c_str(), childDepth);
      for (auto& childHolder : getSpawnedChildren())
      {
         if (!childHolder.child.expired())
            childVisiter(recursiveBoundsPoller, *childHolder.child.lock(), *this);
      }
   }
   //info("[UiElement::getUiBounds] Finished interating %s children. \nmin is {%s}, max is {%s}", getMapObjectsData().objId.c_str(), CVEC(bmin), CVEC(bmax));
   bmin = std::min(bmin, m_root->getWorldTranslation());
   bmax = std::max(bmax, m_root->getWorldTranslation());
   //info("[UiElement::getUiBounds] Adjusted %s min-max by root pos. \nmin is {%s}, max is {%s}", getMapObjectsData().objId.c_str(), CVEC(bmin), CVEC(bmax));
   if(bmin != bDefaultMin)
      bounds[0] = std::min(bmin - getWorldPos(), -m_minBoundsSize);
   else
      bounds[0] = -m_minBoundsSize;
   if(bmax != bDefaultMax)
      bounds[1] = std::max(bmax - getWorldPos(), m_minBoundsSize);
   else
      bounds[1] = m_minBoundsSize;
   bounds[0].z = 0.f; bounds[1].z =  0.f;
   //info("[UiElement::getUiBounds] Final %s bounds: \n{%s}\n{%s}", getMapObjectsData().objId.c_str(), CVEC(bounds[0]), CVEC(bounds[1]));

   return bounds;
}

void UiElement::onBeginPlay(sptr<Spawnable> selfRef, bool isTabSpawned, vec3 spawnLocation)
{
   m_selfWeakRef = cast_sptr<UiElement>(selfRef);
   //handles placement, children spawning, Placement strategy (by mesh, not bounds), updates subscribing
   SceneObject::onBeginPlay(selfRef,isTabSpawned, spawnLocation);

   initInvisibleBounds();
   if (m_mapObjectData.settings.flags["UIE_useBoundsForPlacement"])
      setupPlacementStrategy();
   UiElement::initSpawnable(m_root);
}

void UiElement::initInvisibleBounds()
{
   if (!m_invisibleBounds)
   {
      m_invisibleBounds = Mesh::create::plane(vec2::one);
      auto matInst = RMat("sprite")->createInstance();
      matInst->enableBlending(true);
      matInst->setBlendFunc(BlendFactor::SRC_ALPHA, BlendFactor::ONE_MINUS_SRC_ALPHA);
      matInst->setTexture(TextureId::TEXTURE_0, Texture::createWithColor(color::Black));
      SAFE_PARAM(matInst, "color", vec4(1.f,1.f, 1.f, 0.f));
      m_invisibleBounds->setRenderOrder(1);
      m_invisibleBounds->setMaterialInst(matInst);
      m_invisibleBounds->setName(m_mapObjectData.objId + "_boundsDebugMesh");
      m_root->addChild(m_invisibleBounds, false);
   }

   m_uiBounds = getUiBounds(m_boundsCheckDepth);
   if ((m_uiBounds[1] - m_uiBounds[0]).length() < 0.01)
   {
      err("[UiElement::initInvisibleBounds] %s bounds are too small!", m_mapObjectData.objId.c_str());
      return;
   }

   //info("[UiElement::initInvisibleBounds] Creating invisible bounds for %s with bounds: \n{%s}\n{%s}", getMapObjectsData().objId.c_str(), CVEC(m_uiBounds[0]), CVEC(m_uiBounds[1]));
   vec3 meshSize = {m_uiBounds[1].x - m_uiBounds[0].x, m_uiBounds[1].y - m_uiBounds[0].y, 0.01};
   m_invisibleBounds->setWorldScale(meshSize);
   m_invisibleBounds->setLocalTranslation((m_uiBounds[1] - (meshSize)/ 2.0f)*vec3(1.f, 1.f, 0.f)); //"placing" mesh right-top corner at position of right-top bound (needed in case child meshes are not at vec3::zero)

   if (m_placementStrategy && m_mapObjectData.settings.flags["UIE_useBoundsForPlacement"])
   {
      m_placementStrategy->setContext(m_root, m_invisibleBounds, m_placementStrategy->getDistanceToCameraSet());
      m_placementStrategy->onResize();
   }
}

void UiElement::setupPlacementStrategy()
{
   if (m_placementStrategy)
      return;

   if (!m_invisibleBounds && m_mapObjectData.settings.strs["SO_meshForPS"].empty())
   {
      warn("[UiElement::setupPlacementStrategy] Failed to find bounds mesh or child mesh for placement strategy!");
      return;
   }
   else if (!m_mapObjectData.settings.strs["SO_meshForPS"].empty())
   {
      SceneObject::setupPlacementStrategy();
      return;
   }

   m_placementStrategy = PlacementStrategies::makeStrategyFromMOD(m_mapObjectData, m_root, m_invisibleBounds);
   addSpawnableSetting(SpawnSetting("Placement Strategy",m_placementStrategy));
}

void UiElement::emitAllEvents(ScreencastEvent eventType, const CollisionInfo& hit)
{
   if (!ND_CASTALLOWED) return;
   //info("[UiElement::emitAllEvents](%s) Receives clicks: %s", getObjId().c_str(), BOOL_STR(m_receivesClicks));
   if (!m_receivesClicks) return;
   //clears expired / unsubscribed subscriptions and emits events where needed
   std::erase_if(m_uiEvents, [this, eventType, hit](const wptr<UiElement::Event> evt)
      {
         bool needsDeleting = evt.expired() || evt.lock()->m_canBeDeleted;
         if (!needsDeleting)  evt.lock()->emit(eventType, hit);
         return needsDeleting;
      });
}

void UiElement::updateUiHierarchy()
{
   initInvisibleBounds();
   if (!getParentUi().expired())
   {
      auto parent = getParentUi().lock();
      parent->updateUiHierarchy();
   }
}
