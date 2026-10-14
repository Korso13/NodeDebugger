// Created by Timur Korsakov on 28.08.2025.
#pragma once
#include "UiElement.h"

//MOD settings
//flags["UIC_horizontal"] - container format (horizontal / vertical)
//vec2s["UIC_size"] - container rectangle size
//vec2s["UIC_margins"] - internal container margins
//vec2s["UIC_elementsOffset"] - desired offset between container elements
//strs["UIC_alignment"] - elements alignment
//strs["UIC_stretching"] - containers' auto stretching
//strs["UIC_child#"] - objId of #th element in container

//todo: three non-critical things needed:
//1) Notifications on UiElement size change (added child, mesh, changed size of container)
//2) Container calls to updateLayouts() on children size change

class UiContainer : public UiElement
{
public:
   UiContainer(MapObjectsData& mod, sptr<Node> parentNode);
   ~UiContainer() override {};

   //called automatically after adding object to PlayerController
   void spawnableSelfUpdate(float dt) override;

   //unused for now?
   void addNewElement(sptr<UiElement> newChild);

   //sets bounds of container
   void setContainerSize(vec2 containerSize, bool forceUpdateLayout = false);
   //alternative to setContainerSize() - force container to stretch to screenSize
   //doesn't work with containers that are child of another UiElement or UiContainer!!
   void stretchContainerSize(UiStretch stretchMode);
   //reloads defaults from m_mapObjects data, discards changes made via setContainerSize()
   void resetContainerSize();
   void setContainerFormat(bool horizontalContainer);
   void setElementsAlignment(UiAlignment alignment);
   //minimal offset of elements from container bounds
   void setMargins(vec2 margins);
   //set desired offset between elements. Needed if you picked non-centered alignment
   void setOffset(vec2 elementsOffset);
   vec2 getUiElementSize(bool bForceRecalculate = false) override;

   //forces to update container's layout.
   //Only need this of contents of one of the child elements changes (for example text)
   void updateLayout();

   MapObjectsData& getMapObjectsData() override;

protected:

   void initSpawnable(sptr<Node> rootNode) override;
   void onChildSpawned(sptr<Spawnable> newChild) override;
   void onBeginPlay(sptr<Spawnable> selfRef, bool isTabSpawned, vec3 spawnLocation) override;

   //void setParentUi(wptr<UiElement> parent) override;
   std::array<vec3, 2> getUiBounds(size_t childRecursiveDepth = 1) override;
   //void initInvisibleBounds() override;
   void updateChildList();
   vec2 getPlacementStartPosition(bool& reverseIteration, const vec2& totalContentsSize);
   void updateUiHierarchy() override;

public:

   bool prolificLogs = false;

private:

   bool m_loadComplete = false;
   bool m_horizontalContainer;
   vec2 m_containerSize = vec2::one;
   vec2 m_margins = vec2::zero;
   vec2 m_offset = vec2(0.3f);
   UiAlignment m_elementsAlignment = UiAlignment::SIZE;

   sptr<Node> m_boundDrawerNode;

   UiStretch m_stretchMode = UiStretch::NONE;
   sptr<Resize::Handle> m_resizeHandler = nullptr;

   std::vector<wptr<UiElement>> m_contentsOrder;
};