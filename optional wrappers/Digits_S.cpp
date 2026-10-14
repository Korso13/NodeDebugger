#include "Digits_S.h"
#include "game/playerController/PlayerController.h"
#include "others/utils/ScreenSpaceShader.h"
#include "ui/PlacementStrategies.h"

Digits_S::Digits_S(MapObjectsData& mod, sptr<Node> rootNode)
   :
SceneObject(mod, rootNode, false, true)
{
   checkModRequiredValues(m_mapObjectData);
   auto& settings = m_mapObjectData.settings;
   m_renderFloatingPart = settings.flags["MDP_useFloatingPart"];

   std::pair<int, int> intFloatValues = breakFloatIntoInts(settings.floats["MDP_value"]);
   m_digits = M_SPTR<MonospaceDigitsPlane>(
     intFloatValues.first,
     SettingsDigits{
       .renderOrder = settings.integers["MDP_ro"],
       .rootNode = m_root,
       .pos = settings.vec3s["MDP_localPos"],
       .offset = settings.vec3s["MDP_digitsOffset"],
       .scale = settings.vec3s["MDP_digitsScale"],
       .isRenderOrderUp = true,
       .aligning = strToEnum<Aligning>(settings.strs["MDP_alignment"]),
       .color = settings.vec4s["MDP_color"].asVec3(),
       .meshName = mod.objName,
       .categoriesSpaceOffset = settings.vec3s["MDP_categoriesOffset"],
       .digitInCategory = settings.integers["MDP_categoriesDigits"]
       });
   m_digits->setColorDigits(settings.vec4s["MDP_color"]); //allows setting start alpha

   if (m_renderFloatingPart)
   {
      m_floatAccuracy = std::max(1, m_mapObjectData.settings.integers["MDP_floatAccuracy"]);
      createFloatingPointPart();
   }

   //SPAWNER.m_nodeHierarchySearchLogging = true;
   spawnAllChildren();
   //SPAWNER.m_nodeHierarchySearchLogging = false;
   if (m_renderFloatingPart)
      createComma();
   if (settings.flags["MDP_needPrefix"])
      createPrefix();
   else if (settings.flags["MDP_needSuffix"])
      createSuffix();

   Digits_S::initSpawnable(m_root);
}

void Digits_S::setValue(float value, bool bInstant, bool saveValueToMod)
{
   auto values = breakFloatIntoInts(value);
   if (m_digits) m_digits->changeValue(values.first, bInstant);
   if (m_renderFloatingPart && m_floatingPoint) m_floatingPoint->changeValue(values.second, bInstant);
   if (saveValueToMod)
      m_mapObjectData.settings.floats["MDP_value"] = value;
}

void Digits_S::setValue(int value, bool bInstant, bool saveValueToMod)
{
   if (m_digits) m_digits->changeValue(value, bInstant);
   if (m_renderFloatingPart && m_floatingPoint) m_floatingPoint->changeValue(0, bInstant);
   if (saveValueToMod)
      m_mapObjectData.settings.floats["MDP_value"] = value;
}

void Digits_S::createPrefix()
{
   if (m_mapObjectData.settings.strs["MDP_prefixMesh"] == "NONE" || m_mapObjectData.settings.strs["MDP_prefixMesh"].empty()) return;
   if (!m_prefixMesh)
   {
      m_prefixMesh = getChildMesh(m_mapObjectData.settings.strs["MDP_prefixMesh"]);
      if (!m_prefixMesh)
      {
         warn("[Digits_S::createPrefix] No child mesh %s to use as prefix!", m_mapObjectData.settings.strs["MDP_prefixMesh"].c_str());
         return;
      }
   }

   //using subscription instead of childing to avoid potential issues with init time and parenting
   // m_digits->m_leftEdgeNode->subscribeToWorldTransformChanged([this](const auto& trans)
   //    {
   //       m_prefixMesh->setWorldTranslation(m_digits->m_leftEdgeNode->getWorldTranslation()
   //          + getMeshPlaneSize(m_prefixMesh) * vec3(-0.5, 0.f, 0.f));
   //    });
   m_digits->m_leftEdgeNode->addChild(m_prefixMesh);
   m_prefixMesh->setWorldTranslation(m_digits->m_leftEdgeNode->getWorldTranslation()
      + getMeshPlaneSize(m_prefixMesh) * vec3(-0.5, 0.f, 0.f));
}

void Digits_S::createSuffix()
{
   if (m_mapObjectData.settings.strs["MDP_suffixMesh"] == "NONE" || m_mapObjectData.settings.strs["MDP_suffixMesh"].empty()) return;
   if (!m_suffixMesh)
   {
      m_suffixMesh = getChildMesh(m_mapObjectData.settings.strs["MDP_suffixMesh"]);
      if (!m_suffixMesh)
      {
         warn("[Digits_S::createSuffix] No child mesh %s to use as suffix!", m_mapObjectData.settings.strs["MDP_suffixMesh"].c_str());
         return;
      }
   }

   sptr<MonospaceDigitsPlane> digitToAttachTo = m_renderFloatingPart ? m_floatingPoint : m_digits;
   // digitToAttachTo->m_rightEdgeNode->subscribeToWorldTransformChanged([this, digitToAttachTo](const auto& trans)
   //    {
   //       m_suffixMesh->setWorldTranslation(digitToAttachTo->m_rightEdgeNode->getWorldTranslation()
   //          + getMeshPlaneSize(m_suffixMesh) * vec3(0.5, 0.f, 0.f));
   //    });
   digitToAttachTo->m_leftEdgeNode->addChild(m_suffixMesh);
   m_suffixMesh->setWorldTranslation(digitToAttachTo->m_rightEdgeNode->getWorldTranslation()
      + getMeshPlaneSize(m_suffixMesh) * vec3(0.5, 0.f, 0.f));
}

void Digits_S::createComma()
{
   if (m_mapObjectData.settings.strs["MDP_commaMesh"] == "NONE" || m_mapObjectData.settings.strs["MDP_commaMesh"].empty()) return;
   if (!m_commaMesh)
   {
      m_commaMesh = getChildMesh(m_mapObjectData.settings.strs["MDP_commaMesh"]);
      if (!m_commaMesh)
      {
         warn("[Digits_S::createComma] No child mesh %s to use as comma!", m_mapObjectData.settings.strs["MDP_commaMesh"].c_str());
         return;
      }
   }

   if (m_floatingPoint)
   {
      m_floatingPoint->m_leftEdgeNode->addChild(m_commaMesh);
      // m_commaMesh->setWorldTranslation(m_floatingPoint->m_leftEdgeNode->getWorldTranslation()
      //    + getMeshPlaneSize(m_commaMesh) * vec3(-0.5, 0.f, 0.f));
      m_commaMesh->setLocalTranslation(m_mapObjectData.settings.vec3s["MDP_commaOffset"]);
   }
}

void Digits_S::spawnableSelfUpdate(float dt)
{
   if (m_digits) m_digits->update(dt);
   if (m_floatingPoint) m_floatingPoint->update(dt);
}

void Digits_S::initSpawnable(sptr<Node> rootNode)
{
   //SceneObject::initSpawnable(rootNode); don't need it here
   static auto rebuildDigits = [](sptr<MonospaceDigitsPlane> digits, SettingsStuct& settings)
      {
         if (!digits)
         digits->m_offset = settings.vec3s["MDP_digitsOffset"];
         digits->m_categoriesSpaceOffset = settings.vec3s["MDP_categoriesOffset"];
         digits->m_digitInCategory = settings.integers["MDP_categoriesDigits"];
         digits->makeFinalPlane(true);
      };

   addSpawnableSetting(SpawnSetting(
      "Digits node (local pos and world scale)",
      m_digits->getNode(),
      [this](sptr<Node> node)
         {
            auto& settings = m_mapObjectData.settings;
            settings.vec3s["MDP_digitsScale"] = m_digits->getNode()->getWorldScale();
            settings.vec3s["MDP_localPos"] = m_digits->getPos();
         }));

   addSpawnableSetting(SpawnSetting(
      "Value",
      m_mapObjectData.settings.floats["MDP_value"],
      [this](float& var)
         {
            setValue(var);
         }));

   addSpawnableSetting(SpawnSetting(
   "Render Order",
   m_mapObjectData.settings.integers["MDP_ro"],
   [this](auto& var)
      {
         if (m_digits) m_digits->setRenderOrder(var);
      }));

   addSpawnableSetting(SpawnSetting(
      "Digits offset",
      m_mapObjectData.settings.vec3s["MDP_digitsOffset"],
      [this](auto& var)
         {
            rebuildDigits(m_digits, m_mapObjectData.settings);
            rebuildDigits(m_floatingPoint, m_mapObjectData.settings);
         }));

   addSpawnableSetting(SpawnSetting(
      "Alignment",
      m_digits->m_aligning,
      [this](Aligning& var)
         {
            m_mapObjectData.settings.strs["MDP_alignment"] = enumToStr(var);
            rebuildDigits(m_digits, m_mapObjectData.settings);
            rebuildDigits(m_floatingPoint, m_mapObjectData.settings);
         }));

   addSpawnableSetting(SpawnSetting(
      "Color",
      m_mapObjectData.settings.vec4s["MDP_color"],
      [this](vec4& var)
         {
            if (m_digits)
               m_digits->setColorDigits(var);
         }));

   addSpawnableSetting(SpawnSetting(
      "Atlas name",
      m_mapObjectData.objName,
      [this](auto& var)
         {
            if (ResourceLibrary::instance().getLoadedTexturesInfo().find(var) == ResourceLibrary::instance().getLoadedTexturesInfo().end())
               return;
            m_digits->m_meshName = m_mapObjectData.objName;
            rebuildDigits(m_digits, m_mapObjectData.settings);
            rebuildDigits(m_floatingPoint, m_mapObjectData.settings);
         }));

   addSpawnableSetting(SpawnSetting(
      "Categories offset",
      m_mapObjectData.settings.vec3s["MDP_categoriesOffset"],
      [this](auto& var)
         {
            rebuildDigits(m_digits, m_mapObjectData.settings);
            rebuildDigits(m_floatingPoint, m_mapObjectData.settings);
         }));
   addSpawnableSetting(SpawnSetting(
      "Digits in categories",
      m_mapObjectData.settings.integers["MDP_categoriesDigits"],
      [this](auto& var)
         {
            rebuildDigits(m_digits, m_mapObjectData.settings);
            rebuildDigits(m_floatingPoint, m_mapObjectData.settings);
         }));

   //floating point mode
   addSpawnableSetting(SpawnSetting(
      "With floating point",
      m_mapObjectData.settings.flags["MDP_useFloatingPart"],
      [this](auto& var)
         {
            m_renderFloatingPart = var;
            auto& settings = m_mapObjectData.settings;
            std::pair<int, int> intFloatValues = breakFloatIntoInts(settings.floats["MDP_value"]);

            if (m_renderFloatingPart)
            {
              createFloatingPointPart();
               if (m_commaMesh)
                  m_commaMesh->setEnabled(true);
               else
                  createComma();
            }
            else
            {
               if (m_commaMesh)
               {
                  auto parent = m_commaMesh->getParent();
                  if (parent)parent->removeChild(m_commaMesh);
                  m_commaMesh = nullptr;
               }
               if (m_floatingPoint)
               {
                  auto parent = m_floatingPoint->getNode()->getParent();
                  if (parent) parent->removeChild(m_floatingPoint->getNode());
                  m_floatingPoint = nullptr;
               }
            }
            setValue(m_mapObjectData.settings.floats["MDP_value"]);
         }));

   addSpawnableSetting(SpawnSetting(
      "Floating point part offset",
      m_mapObjectData.settings.vec3s["MDP_floatOffset"],
      [this](auto& var)
         {
            if (m_floatingPoint)
               m_floatingPoint->setPos(var);
         }));
   addSpawnableSetting(SpawnSetting(
      "Floating point offset",
      m_mapObjectData.settings.vec3s["MDP_commaOffset"],
      [this](auto& var)
         {
            if (m_commaMesh)
               m_commaMesh->setLocalTranslation(var);
         }));

   addSpawnableSetting(SpawnSetting(
      "Floating point accuracy",
      m_mapObjectData.settings.integers["MDP_floatAccuracy"],
      [this](auto& var)
         {
            m_floatAccuracy = std::max(1, var);
            setValue(m_mapObjectData.settings.floats["MDP_value"]);
         }));

   //prefix, suffix, comma selectors //todo: add category separator mesh selector, when their support is implemented
   auto childMeshListGetter = [this]() -> std::map<std::string, std::string>
      {
         std::map<std::string, std::string> meshList = {{"NONE", "NONE"}};
         for (auto& child : m_mapObjectData.childMeshes)
            if (child)
               meshList.try_emplace(child->meshId, child->meshId);
         return meshList;
      };
   addSpawnableSettingList(
      "Prefix mesh",
      m_mapObjectData.settings.strs["MDP_prefixMesh"],
      childMeshListGetter,
      [this](std::string& var)
         {
            if (var.empty()) return;
            if (var == "NONE")
            {
               auto parent = m_prefixMesh->getParent();
               if (parent)parent->removeChild(m_prefixMesh);
               m_prefixMesh = nullptr;
               return;
            }
            m_prefixMesh = getChildMeshByName(var);
            createPrefix();
         }
   );
   addSpawnableSettingList(
      "Suffix mesh",
      m_mapObjectData.settings.strs["MDP_suffixMesh"],
      childMeshListGetter,
      [this](std::string& var)
         {
            if (var.empty()) return;
            if (var == "NONE")
            {
               auto parent = m_suffixMesh->getParent();
               if (parent)parent->removeChild(m_suffixMesh);
               m_suffixMesh = nullptr;
               return;
            }
            m_suffixMesh = getChildMeshByName(var);
            createSuffix();
         }
   );
   addSpawnableSettingList(
      "Comma mesh",
      m_mapObjectData.settings.strs["MDP_commaMesh"],
      childMeshListGetter,
      [this](std::string& var)
         {
            if (var.empty()) return;
            if (var == "NONE")
            {
               auto parent = m_commaMesh->getParent();
               if (parent)parent->removeChild(m_commaMesh);
               m_commaMesh = nullptr;
               return;
            }
            m_commaMesh = getChildMeshByName(var);
            createComma();
         }
   );
}

std::pair<int, int> Digits_S::breakFloatIntoInts(const float value) const
{
   std::pair<int, int> result;
   float intPart = 0.f;
   result.first = floor(value);
   float elevatedFloat, integralPart;
   elevatedFloat = modf(value, &integralPart);
   elevatedFloat *= pow(10, m_floatAccuracy);
   result.second = std::round(elevatedFloat);
   //debug("[Digits_S::breakFloatIntoInts] In: %f, Out: %d , %d", value, result.first, result.second);

   return result;
}

void Digits_S::onBeginPlay(sptr<Spawnable> selfRef, bool isTabSpawned, vec3 spawnLocation)
{
   if (isTabSpawned)
      setWorldPos(spawnLocation);
   else
   {
      if (!m_mapObjectData.isChildObj)
         setWorldPos(m_mapObjectData.pos);
      else
         setLocalPos(m_mapObjectData.local_pos);
   }
   PLAYER_CONTROLLER.addObject(selfRef);
}

void Digits_S::onEndPlay(sptr<Spawnable> selfRef, const std::string& objId)
{
   PLAYER_CONTROLLER.removeObject(selfRef);
}

void Digits_S::createFloatingPointPart()
{
    auto& settings = m_mapObjectData.settings;
    std::pair<int, int> intFloatValues = breakFloatIntoInts(settings.floats["MDP_value"]);
    m_floatingPoint = M_SPTR<MonospaceDigitsPlane>(
      intFloatValues.second,
       SettingsDigits{
       .renderOrder = settings.integers["MDP_ro"],
       .rootNode = m_root,
       .pos = settings.vec3s["MDP_localPos"],
       .offset = settings.vec3s["MDP_digitsOffset"],
       .scale = settings.vec3s["MDP_digitsScale"],
       .isRenderOrderUp = true,
       .aligning = Aligning::LEFT,
       .color = settings.vec4s["MDP_color"].asVec3(),
       .meshName = m_mapObjectData.objName,
       .categoriesSpaceOffset = settings.vec3s["MDP_categoriesOffset"],
       .digitInCategory = settings.integers["MDP_categoriesDigits"]
        });
   m_floatingPoint->getNode()->setName(m_floatingPoint->getNode()->getName() + "_floatingPoint");
   m_digits->getRightEdgeNode()->addChild(m_floatingPoint->getNode());
   m_floatingPoint->setPos(settings.vec3s["MDP_floatOffset"]);
   m_floatingPoint->setColorDigits(settings.vec4s["MDP_color"]); //allows setting start alpha
}

void Digits_S::checkModRequiredValues(MapObjectsData& mod)
{
   auto& settings = mod.settings;
   settings.flags.reserve(10);settings.integers.reserve(10);settings.floats.reserve(10);settings.strs.reserve(10);settings.vec2s.reserve(10);settings.vec3s.reserve(10);settings.vec4s.reserve(10);
   if (settings.vec3s["MDP_digitsScale"] == vec3::zero)
      settings.vec3s["MDP_digitsScale"] = vec3::one;
   if (settings.strs["MDP_alignment"].empty())
      settings.strs["MDP_alignment"] = enumToStr(Aligning::CENTER);
   if (mod.objName.empty())
   {
      auto It = ResourceLibrary::instance().getLoadedTexturesInfo().find("nums_atlas");
      if (It != ResourceLibrary::instance().getLoadedTexturesInfo().end())
         mod.objName = (*It).first;
      else
         mod.objName = "numbers";
   }
   if (settings.vec4s["MDP_color"] == vec4::zero)
      settings.vec4s["MDP_color"] = vec4::one;
   if (settings.integers["MDP_ro"] == 0)
      settings.integers["MDP_ro"] = 10;
   if (settings.integers["MDP_categoriesDigits"] == 0)
      settings.integers["MDP_categoriesDigits"] = 3;
}
