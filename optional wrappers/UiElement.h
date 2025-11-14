// Created by Timur Korsakov on 28.08.2025.
#pragma once

#include "common/Defines.h"
#include "common/nodeDebugger/spawner/SpawnerWrappers.h"

//MOD settings:
//flags["UIE_selfEnabledOnStart"] - set enabled on start
//flags["UIE_childrenEnabledOnStart"] - set enabled for children on start
//flags["UIE_useBoundsForPlacement"] - use UiElement for PlacementStrategy
//integers["UIE_screenId"] - UiElement id for switching Ui Screens with UiController

class UiElement : public SceneObject, public AutoTweens
{
public:

   struct Event
   {
      friend class UiElement;
      Event() : uid(w4::utils::UIDGenerator::get()) {}
      Event(std::function<void(ScreencastEvent eventType, const CollisionInfo& hit)>&& cb)
         :
         uid(w4::utils::UIDGenerator::get()),
         m_callback(std::move(cb))  {}

      //or you can simply erase all sptr copies of event
      void unsubscribe() {m_callback = nullptr; m_canBeDeleted = true;}

   protected:
      void emit(ScreencastEvent eventType, const CollisionInfo& hit);

   public:
      const size_t uid;

   private:
      bool m_canBeDeleted = false;
      std::function<void(ScreencastEvent eventType, const CollisionInfo& hit)> m_callback = nullptr;
   };

public:

   UiElement(MapObjectsData& mod, sptr<Node> parentNode);
   ~UiElement() override {};

   //called automatically after adding object to PlayerController
   void spawnableSelfUpdate(float dt) override;

   //Ui element's methods:

   //Setup callback on clicking UI element
   sptr<UiElement::Event> subscribeToTouchEvents(std::function<void(ScreencastEvent eventType, const CollisionInfo& hit)>&& callback);

   //toggles touch mechanic
   virtual void setDetectTouches(bool bDetectTouches);
   //toggles UiElement but not its children
   virtual void enableSelf(bool isEnabled);
   //toggles UiElement's children. applyRecursively == true - children of children are affected as well
   virtual void enableChildren(bool isEnabled, bool applyRecursively = true);

   //play FX across UiElement's parts and optionally those of its children by applying
   //some effect to nodes and spawnable objects via tween.
   //animationPred receives EITHER Node OR Spawnable - always check them for nullptr!
   //Do not set recursiveCall value yourself - it's for internal use only!
   template<typename TweenDataType>
   void playFX(const TweenDataType& from, const TweenDataType& to,
                  float time,
                  Easing easing,
                  bool applyToUiChildren,
                  std::function<void(sptr<Node>, sptr<Spawnable>, const std::decay_t<TweenDataType>& value)> animationPred,
                  std::function<void()> onAnimFinish = nullptr,
                  bool recursiveCall = false);

   //This element's controlling UiElement (if any)
   wptr<UiElement> getParentUi(){return m_parent;};
   bool isEnabled() const { return m_isEnabled; }
   bool isChildrenEnabled() const { return m_isEnabledChildren; }
   size_t getUiLayer() {return m_uiLayer;}
   const std::array<vec3, 2>& getCachedBounds() const {return m_uiBounds;}
   virtual vec2 getUiElementSize(bool bForceRecalculate = false);
   int getScreenId();

protected:

   void initSpawnable(sptr<Node> rootNode) override;
   void onChildSpawned(sptr<Spawnable> newChild) override;
   void onBeginPlay(sptr<Spawnable> selfRef, bool isTabSpawned, vec3 spawnLocation) override;
   //void onEndPlay(sptr<Spawnable> selfRef, const std::string& objId) override;

   virtual std::array<vec3, 2> getUiBounds(size_t childRecursiveDepth = 1);
   virtual void initInvisibleBounds();
   void setupPlacementStrategy() override;

   void initCollider();
   virtual void setParentUi(wptr<UiElement> parent);

   void emitAllEvents(ScreencastEvent eventType, const CollisionInfo& hit);
   virtual void updateUiHierarchy();

protected:

   bool m_isEnabled = true;
   bool m_isEnabledChildren = true;
   bool m_receivesClicks = false;
   size_t m_uiLayer = 0;
   size_t m_boundsCheckDepth = 1; //consider exposing to MOD settings too
   inline static constinit vec3 m_minBoundsSize = {0.05, 0.05, 0.0};

   wptr<UiElement> m_selfWeakRef;
   wptr<UiElement> m_parent;
   std::array<vec3, 2> m_uiBounds;
   sptr<Collider> m_clickCollider = nullptr;
   sptr<Mesh> m_invisibleBounds = nullptr;
   std::map<size_t, wptr<UiElement>> m_uiChildren;
   std::vector<wptr<UiElement::Event>> m_uiEvents;
};



template<typename TweenDataType>
inline void UiElement::playFX(const TweenDataType& from, const TweenDataType& to,
   float time,
   Easing easing,
   bool applyToUiChildren,
   std::function<void(sptr<Node>, sptr<Spawnable>, const std::decay_t<TweenDataType>& value)> animationPred,
   std::function<void()> onAnimFinish,
   bool recursiveCall)
{
   removeAutoTween("fxTween");
   wptr <UiElement> selfWeakRef;  if (!recursiveCall) selfWeakRef = SPAWNER.getObjectByName<UiElement>(m_mapObjectData.objId);
   auto tween = M_SPTR<CustomTweens::Tween<TweenDataType>>(from,
      [this, animationPred, recursiveCall, selfWeakRef](TweenDataType newValue)
         {
            //apply to self first (if it's not a call from parent - if true, parent already made these calls for the child)
            if (!recursiveCall)
            {
               animationPred(nullptr, selfWeakRef.lock(), newValue);
               animationPred(m_root, nullptr, newValue);
            }
            //apply to spawned child objects
            for (auto& childHolder : getSpawnedChildren())
               if (!childHolder.child.expired())
                  animationPred(nullptr, childHolder.child.lock(), newValue);
            //apply to immediate root's children
            for (auto childNode : m_root->getChildren())
            {
               if (childNode == m_invisibleBounds) continue;
               animationPred(childNode, nullptr, newValue);
            }
         }, "fxTween");
   tween->addMore(to, time, easing, onAnimFinish);

   if (applyToUiChildren)
   {
      for (auto& [id, uiChild] : m_uiChildren)
      {
         if (!uiChild.expired())
            uiChild.lock()->playFX(from, to, time, easing, applyToUiChildren, animationPred, onAnimFinish, true);
      }
   }
   pushTween(tween);
}
