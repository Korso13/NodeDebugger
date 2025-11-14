#include "UiContainer.h"

#include "common/nodeDebugger/PathsDeserializer.h"
#include "others/utils/ScreenSpaceShader.h"
#include "ui/PlacementStrategies.h"

UiContainer::UiContainer(MapObjectsData& mod, sptr<Node> parentNode)
   :
UiElement(mod, parentNode)
{
   m_boundDrawerNode = M_SPTR<Node>(m_mapObjectData.objId + "_boundDrawerNode");
   m_root->addChild(m_boundDrawerNode);
   if (m_mapObjectData.settings.vec2s["UIC_size"] == vec2::zero)
      m_mapObjectData.settings.vec2s["UIC_size"] = vec2::one;
}

void UiContainer::spawnableSelfUpdate(float dt)
{
   UiElement::spawnableSelfUpdate(dt);
}

void UiContainer::setElementsAlignment(UiAlignment alignment)
{
   m_elementsAlignment = alignment;
   //updateLayout();
}

void UiContainer::setMargins(vec2 margins)
{
   m_margins = margins;
   //updateLayout();
}

void UiContainer::setOffset(vec2 elementsOffset)
{
   m_offset = elementsOffset;
   //updateLayout();
}

vec2 UiContainer::getUiElementSize(bool bForceRecalculate)
{
   return m_containerSize * m_root->getLocalScale().withoutZ(); //do we really need adjusting by LocalScale?
};

MapObjectsData& UiContainer::getMapObjectsData()
{
   if (m_elementsAlignment != UiAlignment::SIZE)
      m_mapObjectData.settings.strs["UIC_alignment"] = enumToStr(m_elementsAlignment);
   if (m_stretchMode != UiStretch::NONE)
      m_mapObjectData.settings.strs["UIC_stretching"] = enumToStr(m_stretchMode);
   return UiElement::getMapObjectsData();
}

void UiContainer::initSpawnable(sptr<Node> rootNode)
{
#ifdef  DEBUG
#define S_SETTINGS m_mapObjectData.settings
   addSpawnableSetting(SpawnSetting(
      "Horizontal?",
      m_horizontalContainer,
      [this](auto& val)
      {
            S_SETTINGS.flags["UIC_horizontal"];
            updateLayout();
      }
   ));

   addSpawnableSetting(SpawnSetting(
      "Container Bounds",
      m_boundDrawerNode,
      [this](auto& val)
         {
            vec3 boundPos = m_boundDrawerNode->getLocalTranslation();
            boundPos.x = std::max(m_minBoundsSize.x, neAbs(boundPos.x));
            boundPos.y = std::max(m_minBoundsSize.y, neAbs(boundPos.y));
            //m_boundDrawerNode->setLocalTranslation(boundPos);
            m_containerSize = boundPos.withoutZ() * 2.f;
            S_SETTINGS.vec2s["UIC_size"] = m_containerSize;
            updateLayout();
            renewSpawnableBounds();
         }
   ));

   addSpawnableSetting(SpawnSetting(
      "Container Bounds (current value)",
      m_containerSize
   ));

   addSpawnableSetting(SpawnSetting(
      "Prolific console logs",
      prolificLogs
   ));

   if (getParentUi().expired())
   {
      addSpawnableSetting(SpawnSetting(
         "Container Stretch Mode",
         m_stretchMode,
         [this](UiStretch& val)
            {
               stretchContainerSize(val);
               m_mapObjectData.settings.strs["UIC_stretching"] = enumToStr(val);
               updateLayout();
               renewSpawnableBounds();
            }
      ));
   }

   addSpawnableSetting(SpawnSetting(
      "Margins",
      m_margins,
      [this](auto& val)
         {
            updateLayout();
            S_SETTINGS.vec2s["UIC_margins"] = m_margins;
         }
   ));
   addSpawnableSetting(SpawnSetting(
      "Offset",
      m_offset,
      [this](auto& val)
         {
            updateLayout();
            S_SETTINGS.vec2s["UIC_elementsOffset"] = m_offset;
         }
   ));
   addSpawnableSetting(SpawnSetting(
      "Alignment",
      m_elementsAlignment,
      [this](auto& val)
         {
            updateLayout();
            S_SETTINGS.strs["UIC_alignment"] = enumToStr(m_elementsAlignment);
         }
   ));

   addSpawnableBtn("Update layout", std::bind(&UiContainer::updateLayout, this));

#undef S_SETTINGS
#endif
}

void UiContainer::setContainerSize(vec2 containerSize, bool forceUpdateLayout)
{
   m_containerSize = containerSize;
   if (forceUpdateLayout)
   {
      updateLayout();
   }
}

void UiContainer::stretchContainerSize(UiStretch stretchMode)
{
   if (!getParentUi().expired()) return; //no stretching for UiChildren!
   if (stretchMode == UiStretch::NONE && m_resizeHandler)
   {
      m_resizeHandler->unsubscribe();
      m_resizeHandler = nullptr;
      return;
   }

   m_stretchMode = stretchMode;
   auto stretchResizer = [this](const event::Resize& evt)
      {
         sptr<Camera> camera = Render::getScreenCamera();
         if (SCREEN_FX.getCamera() && m_spRoot->getRoot() == SCREEN_FX.getNodeForHud()->getRoot()) //special check in case SCREEN_FX are enabled
            camera = SCREEN_FX.getCamera();
         auto screenPlaneSize = camera->getFrustumPlaneSize(neAbs(camera->getWorldTranslation().z - getWorldPos().z));
         switch (m_stretchMode)
         {
            case UiStretch::HORIZONTAL:
               setContainerSize(vec2(screenPlaneSize.x, m_containerSize.y), true);
               break;
            case UiStretch::VERTICAL:
               setContainerSize(vec2(m_containerSize.x, screenPlaneSize.y), true);
               break;
            case UiStretch::FULLSCREEN:
               setContainerSize(screenPlaneSize, true);
               break;
            case UiStretch::NONE:
            case UiStretch::SIZE:
               return;
               break;
         }
         forceUpdatePlacementStrategy();
      };
   m_resizeHandler = event::Resize::subscribe(stretchResizer);
   stretchResizer(Render::getSize());
}

void UiContainer::resetContainerSize()
{
   setContainerSize(m_mapObjectData.settings.vec2s["UIC_size"], true);
}

void UiContainer::setContainerFormat(bool horizontalContainer)
{
   m_horizontalContainer = horizontalContainer;
}

void UiContainer::onChildSpawned(sptr<Spawnable> newChild)
{
   UiElement::onChildSpawned(newChild);
   if (!std::dynamic_pointer_cast<UiElement>(newChild))
      return;

   m_contentsOrder.push_back(cast_sptr<UiElement>(newChild));
   updateChildList();
   updateLayout();
}

void UiContainer::onBeginPlay(sptr<Spawnable> selfRef, bool isTabSpawned, vec3 spawnLocation)
{
   auto& settings = m_mapObjectData.settings;
   m_containerSize = settings.vec2s["UIC_size"];
   auto bounds = getUiBounds();
   UiElement::onBeginPlay(selfRef, isTabSpawned, spawnLocation);
   m_loadComplete = true;

   //setting up container
   updateChildList();
   setMargins(settings.vec2s["UIC_margins"]);
   setOffset(settings.vec2s["UIC_elementsOffset"]);
   if (!settings.strs["UIC_alignment"].empty())
      setElementsAlignment(strToEnum<UiAlignment>(settings.strs["UIC_alignment"]));
   if (!settings.strs["UIC_stretching"].empty())
      stretchContainerSize(strToEnum<UiStretch>(settings.strs["UIC_stretching"]));

   setContainerFormat(settings.flags["UIC_horizontal"]);
   updateLayout();
   m_boundDrawerNode->setLocalTranslation(bounds[1]);

   UiContainer::initSpawnable(m_root);
}

std::array<vec3, 2> UiContainer::getUiBounds(size_t childRecursiveDepth)
{
   std::array<vec3, 2>  bounds;
   bounds[0] = -vec3(m_containerSize.x/2.f, m_containerSize.y/2.f, 0.f);
   bounds[1] = vec3(m_containerSize.x/2.f, m_containerSize.y/2.f, 0.f);
   return bounds;
}

void UiContainer::updateChildList()
{
   if (!m_loadComplete) //prevents unnecessary load time and issues with settings
      return;

   std::erase_if(m_contentsOrder, [this](const wptr<UiElement>& element)
      {
         return element.expired() || !m_uiChildren.contains(element.lock()->getUid());
      });

   for (int i = 0; i < m_contentsOrder.size(); ++i)
   {
      m_mapObjectData.settings.strs["UIC_child"+STR(i)] = m_contentsOrder[i].lock()->getMapObjectsData().objId;
#ifdef  DEBUG
      removeSpawnableSetting("Container element " + STR(i));
      addSpawnableSettingList(
         "Container element " + STR(i),
         m_mapObjectData.settings.strs["UIC_child"+STR(i)],
         [this]() -> std::map<std::string, std::string>
            {
               std::map<std::string, std::string> childrenList;
               for (const auto& [uid, childHolder] : m_uiChildren)
                  if (!childHolder.expired())
                  {
                     auto child = childHolder.lock();
                     childrenList[child->getMapObjectsData().objId] = child->getMapObjectsData().objId;
                  }
               return childrenList;
            },
         [this, i](auto& val)
            {
               //finding ui child by name and placing it in m_contentsOrder.
               //NOTE: does not check if it repeats! Need to come up with better system in the future!
               for (const auto& [uid, childHolder] : m_uiChildren)
                  if (!childHolder.expired() && childHolder.lock()->getMapObjectsData().objId == val)
                  {
                     m_contentsOrder[i] = childHolder;
                     return;
                  }
            }
      );
#endif
   }

   for (int i = m_contentsOrder.size(); i < m_contentsOrder.size()*2; ++i)
   {
      m_mapObjectData.settings.strs.erase("UIC_child"+STR(i));
   }
}

//bad architecture! todo: rewrite via Flyweight pattern! (static object with overloaded functions for different alignments and orientations, friend of UiContainer)
void UiContainer::updateLayout()
{
   std::string header = "[UiContainer::updateLayout](" + getObjId() + ")";
   if(prolificLogs)info("\n===================================\n%s\n===================================", header.c_str());
   initInvisibleBounds();
   if (m_contentsOrder.empty())
      return;
   vec2 availableContainerSpace = m_containerSize - m_margins*2.f;
   if (m_contentsOrder.size() > 1 && m_elementsAlignment != UiAlignment::CENTER_STRETCHED)
   {
      if (m_horizontalContainer)
         availableContainerSpace.x -= m_offset.x * (m_contentsOrder.size() - 1);
      else
         availableContainerSpace.y -= m_offset.y * (m_contentsOrder.size() - 1);
   }

#define IS_CONTAINER(item) (item->getMapObjectsData().className == "UiContainer")

   vec2 contentsTotalSize = vec2::zero;
   vec2 biggestItemSize = vec2::zero;
   vec2 sizeOrientationAdjuster = m_horizontalContainer ? vec2(1.f, 0.f) : vec2(0.f, 1.f);
   std::vector<vec2> elementsSizesCache;
   std::vector<vec3> elementsScalers;
   std::vector<vec2> elementsAdjustedSize; elementsAdjustedSize.resize(m_contentsOrder.size());
   for (auto& element : m_contentsOrder)
   {
      if (element.expired())
         continue;
      auto uiElement = element.lock();

      //saving original item's size
      if (!IS_CONTAINER(uiElement))
      {
         uiElement->getRoot()->setLocalScale(vec3::one);
         elementsSizesCache.push_back(uiElement->getUiElementSize(true));
      }
      else
      {
         elementsSizesCache.push_back(uiElement->getMapObjectsData().settings.vec2s["UIC_size"]);
      }

      auto& itemOrigSize = elementsSizesCache.back();

      //recording scaler if non-major side exceeds boundary (used with non-containers)
      if (m_horizontalContainer && itemOrigSize.y > availableContainerSpace.y)
      {
         elementsScalers.emplace_back(availableContainerSpace.y / itemOrigSize.y);
      }
      else if (!m_horizontalContainer && itemOrigSize.x > availableContainerSpace.x)
      {
         elementsScalers.emplace_back(availableContainerSpace.x / itemOrigSize.x);
      }
      else
      {
         elementsScalers.push_back(vec3::one);
      }
      contentsTotalSize += itemOrigSize * sizeOrientationAdjuster; //adding only relevant size side (x or y)
   }

   //subtracting mandatory offsets from available space (major side only)
   vec2 offsetsSum = vec2::zero;
   if (m_contentsOrder.size() > 1 && m_elementsAlignment != UiAlignment::CENTER_STRETCHED)
   {
      offsetsSum = (m_offset * sizeOrientationAdjuster) / CastF(m_contentsOrder.size() - 1);
      if(prolificLogs) info("[UiContainer::updateLayout](%s) {%s} availableContainerSpace - offsetsSum {%s}",
         getObjId().c_str(), CSTR(availableContainerSpace), CSTR(offsetsSum));
      availableContainerSpace -= offsetsSum;
   }

   //calculating, if needed, how much we need to scale down UI elements by major side to fit
   vec3 elementsScaler = vec3::one;
   if (m_horizontalContainer && contentsTotalSize.x > availableContainerSpace.x)
   {
      elementsScaler = vec3(availableContainerSpace.x / contentsTotalSize.x);
   }
   if (!m_horizontalContainer && contentsTotalSize.y > availableContainerSpace.y)
   {
      elementsScaler = vec3(availableContainerSpace.y / contentsTotalSize.y);
   }

   //correcting elements scalers + calculating real contents size for center-stretched placement
   vec2 contentsSizeScaleAdjusted = vec2::zero;
   std::string debugOut = header + "\n";
   if(prolificLogs)debugOut = debugOut + "elementsScalers size: " + STR(elementsScalers.size()) + ", elementsSizesCache size: " + STR(elementsSizesCache.size());
   for (int i = 0; i < elementsScalers.size(); ++i)
   {
      if (elementsScalers[i].x == 0.f || elementsScalers[i].y == 0.f)
      {
         elementsScalers[i] = elementsScaler;
         info("[UiContainer::updateLayout](%s) Fixed null element scaler! %d - %d", getObjId().c_str(), elementsScalers.size(), elementsSizesCache.size());
         continue;
      }

      if (!IS_CONTAINER(m_contentsOrder[i].lock()))
      {
         elementsScalers[i] = std::min(elementsScalers[i], elementsScaler);
         elementsAdjustedSize[i] = elementsScalers[i].withoutZ() * elementsSizesCache[i];
      }
      else
      {
         elementsAdjustedSize[i].x = (m_horizontalContainer) ? (elementsScaler.withoutZ() * elementsSizesCache[i]).x : std::min(elementsSizesCache[i].x, availableContainerSpace.x);
         elementsAdjustedSize[i].y = (!m_horizontalContainer) ? (elementsScaler.withoutZ() * elementsSizesCache[i]).y : std::min(elementsSizesCache[i].y, availableContainerSpace.y);
      }
      biggestItemSize.x = std::max(biggestItemSize.x, elementsAdjustedSize[i].x);
      biggestItemSize.y = std::max(biggestItemSize.y, elementsAdjustedSize[i].y);
      contentsSizeScaleAdjusted += elementsAdjustedSize[i] * sizeOrientationAdjuster;
      if(prolificLogs)debugOut = debugOut + "\n" + STR(i) + " elem adjusted size: " + STR(elementsAdjustedSize[i]) + ", original size: " + STR(elementsSizesCache[i]);
   }
   if(prolificLogs)debug("%s", debugOut.c_str());
   //adding one biggest x for vertical or one biggest y for horizontal
   contentsTotalSize += (m_horizontalContainer ? (biggestItemSize * vec2(0.f, 1.f)) : (biggestItemSize * vec2(1.f, 0.f)));
   //contentsSizeScaleAdjusted += (m_horizontalContainer ? (biggestItemSize * vec2(0.f, 1.f)) : (biggestItemSize * vec2(1.f, 0.f)));

   //calculating offsets for center-stretched alignment
   vec2 extraOffset = vec2::zero;
   if (m_elementsAlignment == UiAlignment::CENTER_STRETCHED && m_contentsOrder.size() > 1)
   {
      if (m_horizontalContainer)
         extraOffset.x = (availableContainerSpace.x - contentsSizeScaleAdjusted.x) / (m_contentsOrder.size() - 1);
      else
         extraOffset.y = (availableContainerSpace.y - contentsSizeScaleAdjusted.y) / (m_contentsOrder.size() - 1);
      if(prolificLogs)debug("%s Extra offset %s", header.c_str(), CVEC(extraOffset));
   }
   elementsScaler.z = 1.f;
   if(prolificLogs)info("%s elementsScaler: %s", header.c_str(), CVEC(elementsScaler));

   //checking alignment enum
   if (m_horizontalContainer)
   {
      if (m_elementsAlignment == UiAlignment::TOP)
         m_elementsAlignment = UiAlignment::LEFT;
      if (m_elementsAlignment == UiAlignment::BOTTOM)
         m_elementsAlignment = UiAlignment::RIGHT;
   }
   else
   {
      if (m_elementsAlignment == UiAlignment::LEFT)
         m_elementsAlignment = UiAlignment::TOP;
      if (m_elementsAlignment == UiAlignment::RIGHT)
         m_elementsAlignment = UiAlignment::BOTTOM;
   }

   //preparing for placement start
   //bool reverseIteration = false;
   bool reverseIteration = m_elementsAlignment == UiAlignment::BOTTOM || m_elementsAlignment == UiAlignment::RIGHT;
   vec2 orientationAdjuster = m_horizontalContainer ? vec2(1.f, 0.f) : vec2(0.f, -1.f);
   vec2 currentElementPlacement = getPlacementStartPosition(reverseIteration, contentsSizeScaleAdjusted);
   if(prolificLogs)info("%s currentElementPlacement (start): %s", header.c_str(), CSTR(currentElementPlacement));

   int i = reverseIteration ? (elementsSizesCache.size() - 1) : 0;
   auto It = m_contentsOrder.begin();
   auto ItEnd = m_contentsOrder.end();
   auto ItR = m_contentsOrder.rbegin();
   auto ItEndR = m_contentsOrder.rend();
   auto Iterate = [&ItR, &It]{++ItR; ++It; };

   //placing elements in order set in m_contentsOrder left to right, top to bottom
   for (;((reverseIteration) ? (ItR != ItEndR) : (It != ItEnd)); Iterate()) //reverse iterators for Right/Bottom aligned elements
   {
      const auto& element = *It;
      if (element.expired())
         continue;
      auto uiElement = element.lock();
      if(prolificLogs)debugOut = "\n" + header + " placing " + uiElement->getObjId();

      //Scaling / resizing items first
      //scaling UiElement
      if (!IS_CONTAINER(uiElement))
      {
         uiElement->getRoot()->setLocalScale(elementsScalers[i]);
      }
      //resizing if it's UiContainer
      else
      {
         vec2 newContainerSize = elementsAdjustedSize[i];
         if(prolificLogs) debug("[UiContainer::updateLayout](%s)\nnewContainerSize {%s}\nelementsSizesCache[i] : {%s}\nelementsScalers[i]: {%s}\navailableContainerSpace {%s}",
                  uiElement->getObjId().c_str(), CSTR(newContainerSize), CSTR(elementsSizesCache[i]), CVEC(elementsScalers[i]), CSTR(availableContainerSpace));
         cast_sptr<UiContainer>(uiElement)->setContainerSize(newContainerSize, true);
      }

      //now placing items
      vec2 offsetToPlace = elementsAdjustedSize[i] / 2.f; //half of element's size (after scaling)
      switch (m_elementsAlignment)
      {
         case UiAlignment::LEFT:
            currentElementPlacement.x += offsetToPlace.x;
            uiElement->setLocalPos(CommonFunction::vec2toVec3(currentElementPlacement, 0.f));
            currentElementPlacement.x += offsetToPlace.x;
            currentElementPlacement.x += m_offset.x;
            break;
         case UiAlignment::RIGHT:
            currentElementPlacement.x -= offsetToPlace.x;
            uiElement->setLocalPos(CommonFunction::vec2toVec3(currentElementPlacement, 0.f));
            currentElementPlacement.x -= offsetToPlace.x;
            currentElementPlacement.x -= m_offset.x;
            break;
         case UiAlignment::CENTER:
            if (m_horizontalContainer)
               {if(prolificLogs)debugOut = debugOut + "\n" + "==============Horizontal container(Center)==============="; }
            else
               {if(prolificLogs)debugOut = debugOut + "\n" + "==============Vertical container(Center)===============";}
            if (m_contentsOrder.size() == 1)
               offsetToPlace = vec2::zero;

            //placement logic (orientationAdjuster changes placement direction)
            if(prolificLogs)debugOut = debugOut + "\ncurrentElementPlacement: " + STR(currentElementPlacement) + " offsetToPlace: " + STR(offsetToPlace) + "\nelement's (true) size: " + STR(uiElement->getUiElementSize(true)) + " element's scale adjusted size: " + STR(elementsAdjustedSize[i]);
            currentElementPlacement += offsetToPlace * orientationAdjuster;
            if(prolificLogs)debugOut = debugOut + "\ncurrentElementPlacement after first offset(point of placement): " + STR(currentElementPlacement);
            uiElement->setLocalPos(CommonFunction::vec2toVec3(currentElementPlacement, 0.f));
            currentElementPlacement += offsetToPlace * orientationAdjuster;
            if(prolificLogs)debugOut = debugOut + "\ncurrentElementPlacement after second offset(right/bottom edge of last item): " + STR(currentElementPlacement);
            currentElementPlacement += m_offset * orientationAdjuster;
            if(prolificLogs)debugOut = debugOut + "\ncurrentElementPlacement after inter-item offset(left/top edge of next item): " + STR(currentElementPlacement);
            if(prolificLogs)debugOut = debugOut + "\n" + "=============================";
            break;
         case UiAlignment::CENTER_STRETCHED:
            if (m_horizontalContainer) //can't use orientationAdjuster because of changing sign for vertical placement
            {
               currentElementPlacement.x += offsetToPlace.x;
               uiElement->setLocalPos(CommonFunction::vec2toVec3(currentElementPlacement, 0.f));
               currentElementPlacement.x += offsetToPlace.x;
               currentElementPlacement.x += extraOffset.x;
            }
            else
            {
               currentElementPlacement.y -= offsetToPlace.y;
               uiElement->setLocalPos(CommonFunction::vec2toVec3(currentElementPlacement, 0.f));
               currentElementPlacement.y -= offsetToPlace.y;
               currentElementPlacement.y -= extraOffset.y;
            }
            break;
         case UiAlignment::TOP:
            currentElementPlacement.y -= offsetToPlace.y;
            uiElement->setLocalPos(CommonFunction::vec2toVec3(currentElementPlacement, 0.f));
            currentElementPlacement.y -= offsetToPlace.y;
            currentElementPlacement.y -= m_offset.y;
            break;
         case UiAlignment::BOTTOM:
            currentElementPlacement.y += offsetToPlace.y;
            uiElement->setLocalPos(CommonFunction::vec2toVec3(currentElementPlacement, 0.f));
            currentElementPlacement.y += offsetToPlace.y;
            currentElementPlacement.y += m_offset.y;
            break;

         default: break;
      }

      if (reverseIteration) i--;
      else i++;
      if(prolificLogs)debug("%s", debugOut.c_str());
   }

   if(prolificLogs)info("\n===================================\n[UiContainer::updateLayout](%s)FINISH\n===================================", getObjId().c_str());
}

vec2 UiContainer::getPlacementStartPosition(bool& reverseIteration, const vec2& totalContentsSize)
{
   std::string debugOut = "\n[UiContainer::getPlacementStartPosition](" + getObjId() + ")";
   vec2 orientationAdjuster = m_horizontalContainer ? vec2(1.f, 0.f) : vec2(0.f, -1.f); //allows inline logic change between vertical and horizontal container for Center-aligned ui
   vec2 currentElementPlacement = vec2::zero;
   switch (m_elementsAlignment)
   {
      case UiAlignment::LEFT:
         currentElementPlacement.x = -m_containerSize.x/2.f + m_margins.x;
         break;
      case UiAlignment::RIGHT:
         reverseIteration = true;
         currentElementPlacement.x = m_containerSize.x/2.f - m_margins.x;
         break;
      case UiAlignment::TOP:
         currentElementPlacement.y = m_containerSize.y/2.f - m_margins.y;
         break;
      case UiAlignment::BOTTOM:
         reverseIteration = true;
         currentElementPlacement.y = -m_containerSize.y/2.f + m_margins.y;
         break;

      case UiAlignment::CENTER:
      {
         if(prolificLogs)debugOut = debugOut + "\nContents size: " + STR(m_contentsOrder.size());
         if (m_contentsOrder.size() == 1)
            break;

         //still placing left to right, top to bottom
         vec2 contentsTotalSizeWithOffsets = totalContentsSize + m_offset * (m_contentsOrder.size()-1);
         currentElementPlacement -= contentsTotalSizeWithOffsets / 2.f * orientationAdjuster;
         if(prolificLogs)debugOut = debugOut + "\ncurrentElementPlacement after subtracting contents size with offsets: " + STR(currentElementPlacement) + ", contents size with offsets: " + STR(contentsTotalSizeWithOffsets);
         break;
      }

      case UiAlignment::CENTER_STRETCHED:
      {
         if (m_contentsOrder.size() == 1)
            break;

         //still placing left to right, top to bottom
         if (m_horizontalContainer)
            currentElementPlacement.x = -m_containerSize.x/2.f + m_margins.x;
         else
            currentElementPlacement.y = m_containerSize.y/2.f - m_margins.y;
         break;
      }

      default: break;
   }
   if(prolificLogs)info("%s", debugOut.c_str());

   return currentElementPlacement;
}

void UiContainer::updateUiHierarchy()
{
   updateLayout();
   UiElement::updateUiHierarchy();
}
