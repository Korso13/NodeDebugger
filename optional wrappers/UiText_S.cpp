#include "UiText_S.h"
#include "game/playerController/PlayerController.h"
#include "others/utils/ScreenSpaceShader.h"
#include "ui/PlacementStrategies.h"

SettingsText convertToSettingsText(TextSettings& importedSettings)
{
   SettingsText result = SettingsText(importedSettings.fontName);
   result.color = color(importedSettings.color);
   result.renderOrder = importedSettings.renderOrder;
   result.pos = importedSettings.position_local;
   result.scale = importedSettings.scale_local;
   result.lineSize = importedSettings.lineSize;
   result.lineOffset = importedSettings.lineOffset;
   result.align = importedSettings.align;
   return result;
}

void UiText_S::textNodeRebuilder(sptr<UiText> textNode, const std::string text, TextSettings& textSettings)
{
   textNode->m_setting.color = textSettings.color;
   textNode->m_setting.fontName = textSettings.fontName;
   textNode->m_setting.renderOrder = textSettings.renderOrder;
   textNode->m_setting.lineSize = textSettings.lineSize;
   textNode->m_setting.lineOffset = textSettings.lineOffset;
   textNode->buildTextNodes(text, textSettings.fontName, textSettings.color, textSettings.renderOrder);
   textNode->m_textRoot->setLocalScale(textSettings.scale_local);
   textNode->m_textRoot->setLocalTranslation(textSettings.position_local);
};

UiText_S::UiText_S(MapObjectsData& mod, sptr<Node> parentNode)
   :
   SceneObject(mod, parentNode, false, true)
{
   TextNodeSettings& textNodeSettings = mod.textNodeSetting;
   if (mod.objId.empty())
   {
      mod.objId = "TextNode_" + textNodeSettings.text;
      m_root->setName(mod.objId);
   }
   SettingsText settingsForText = convertToSettingsText(textNodeSettings.textSetting);

   static bool bDoOnce = false;
   if (!bDoOnce)
   {
      bDoOnce = true;
      //TextSdfNode::setOverrideMaterial(RMat("alt_sdf_material"));
   }

   m_text = M_SPTR<UiText>(m_root,
      textNodeSettings.usesTranslations ? TR(textNodeSettings.text).getCurr(TR_INST.getCurrentLang()) : textNodeSettings.text,
      settingsForText);

   //setting stroke if needed
   applyStroke();

   m_sdfCommonMaterial = m_text->m_textLines[0]->getMaterialInst();

   spawnAllChildren();
   UiText_S::initSpawnable(m_root);
}

MapObjectsData& UiText_S::getMapObjectsData()
{
   m_mapObjectData.pos = m_root->getLocalTranslation();

   return SceneObject::getMapObjectsData();
}

void UiText_S::changeText(const std::string& newText)
{
   m_mapObjectData.textNodeSetting.text = newText;
   textNodeRebuilder(m_text, newText, m_mapObjectData.textNodeSetting.textSetting);
   triggerEvent("onTextChanged", SubscriberEventPayload{.stringPayload = newText});
}

void UiText_S::initSpawnable(sptr<Node> rootNode)
{
   //don't need to call parent method
#ifndef DEBUG
   return;
#endif

   addSpawnableSetting(SpawnSetting(
      "Text",
      m_mapObjectData.textNodeSetting.text,
      [this](auto& var)
         {
            auto text = m_mapObjectData.textNodeSetting.usesTranslations ? TR(m_mapObjectData.textNodeSetting.text).getCurr(TR_INST.getCurrentLang()) : m_mapObjectData.textNodeSetting.text;
            triggerEvent("onTextChanged", SubscriberEventPayload{.stringPayload = text});
            textNodeRebuilder(m_text, text, m_mapObjectData.textNodeSetting.textSetting);
            applyStroke();
            m_sdfCommonMaterial = m_text->m_textLines[0]->getMaterialInst();
         }
   ));

   addSpawnableSetting(SpawnSetting(
      "Text settings",
      m_mapObjectData.textNodeSetting.textSetting,
      [this](auto& var)
         {
            auto text = m_mapObjectData.textNodeSetting.usesTranslations ? TR(m_mapObjectData.textNodeSetting.text).getCurr(TR_INST.getCurrentLang()) : m_mapObjectData.textNodeSetting.text;
            textNodeRebuilder(m_text, text, m_mapObjectData.textNodeSetting.textSetting);
            applyStroke();
            m_sdfCommonMaterial = m_text->m_textLines[0]->getMaterialInst();
         }
   ));

   auto strokeSettingsAdder = [this]()
      {
         addSpawnableSetting(SpawnSetting(
            "Stroke color",
            m_mapObjectData.textNodeSetting.strokeColor,
            [this](vec4& var)
               {
                  applyStroke();
                  m_sdfCommonMaterial = m_text->m_textLines[0]->getMaterialInst();
               }
         ));
         addSpawnableSetting(SpawnSetting(
             "Stroke size",
             m_mapObjectData.textNodeSetting.strokeSize,
             [this](float var)
             {
                   applyStroke();
                   m_sdfCommonMaterial = m_text->m_textLines[0]->getMaterialInst();
             }
         ));
         addSpawnableSetting(SpawnSetting(
             "Stroke smoothing",
             m_mapObjectData.textNodeSetting.strokeSmoothing,
             [this](float var)
             {
                   applyStroke();
                   m_sdfCommonMaterial = m_text->m_textLines[0]->getMaterialInst();
             }
         ));
         addSpawnableSetting(SpawnSetting(
             "Stroke edge center",
             m_mapObjectData.textNodeSetting.strokeEdgeCenter,
             [this](float var)
             {
                   applyStroke();
                   m_sdfCommonMaterial = m_text->m_textLines[0]->getMaterialInst();
             }
         ));
   };
   auto strokeSettingsRemover = [this]()
      {
         removeSpawnableSetting("Stroke color");
         removeSpawnableSetting("Stroke size");
         removeSpawnableSetting("Stroke smoothing");
         removeSpawnableSetting("Stroke edge center");
      };

   if (m_mapObjectData.textNodeSetting.needsStroke)
      strokeSettingsAdder();

   addSpawnableSetting(SpawnSetting(
       "Use stroke",
       m_mapObjectData.textNodeSetting.needsStroke,
       [this, strokeSettingsAdder, strokeSettingsRemover](bool var)
       {
             if (m_mapObjectData.textNodeSetting.needsStroke)
                strokeSettingsAdder();
             else
                strokeSettingsRemover();
      }
  ));
   addSpawnableSetting(SpawnSetting(
    "Use font's own colors",
    m_mapObjectData.textNodeSetting.useFontColor,
    [this, strokeSettingsAdder, strokeSettingsRemover](bool var)
      {
             applyStroke();
          m_sdfCommonMaterial = m_text->m_textLines[0]->getMaterialInst();
      }
  ));

   addSpawnableSetting(SpawnSetting(
      "Text Node's SDF",
      m_text->m_textLines
   ));

   addSpawnableSetting(SpawnSetting(
      "Text SDF Common material",
      m_sdfCommonMaterial
   ));
   addSpawnableBtn("Refresh common material",
      [this]()
         {
            removeSpawnableSetting("Text SDF Common material");
            addSpawnableSetting(SpawnSetting(
               "Text SDF Common material",
               m_sdfCommonMaterial
            ));
         });
}

void UiText_S::onChildSpawned(sptr<Spawnable> newChild)
{
   SceneObject::onChildSpawned(newChild);
}

void UiText_S::onBeginPlay(sptr<Spawnable> selfRef, bool isTabSpawned, vec3 spawnLocation)
{
   if (isTabSpawned && !m_mapObjectData.isChildObj)
      setWorldPos(spawnLocation);
   else if (!m_mapObjectData.isChildObj)
      setWorldPos(m_mapObjectData.pos);
   else if (isTabSpawned && m_mapObjectData.isChildObj)
      setLocalPos(vec3::zero);
   else if (!isTabSpawned && m_mapObjectData.isChildObj)
      setLocalPos(m_mapObjectData.local_pos);
}

void UiText_S::onEndPlay(sptr<Spawnable> selfRef, const std::string& objId)
{

}

void UiText_S::applyStroke()
{
   TextNodeSettings& textNodeSettings = m_mapObjectData.textNodeSetting;
   if (textNodeSettings.needsStroke)
   {
      for (auto& line : m_text->m_textLines)
      {
         //line->setMaterialInst(m_sdfCommonMaterial);
         line->setFillColor(m_mapObjectData.textNodeSetting.textSetting.color);
         line->setStrokeColor(textNodeSettings.strokeColor);
         SAFE_PARAM(line->getMaterialInst(), "outlineBorder", textNodeSettings.strokeSize);
         SAFE_PARAM(line->getMaterialInst(), "smoothing", textNodeSettings.strokeSmoothing);
         SAFE_PARAM(line->getMaterialInst(), "edgeCenter", textNodeSettings.strokeEdgeCenter);
         //SAFE_PARAM(line->getMaterialInst(), "useOriginalColor", (textNodeSettings.useFontColor) ? 1 : 0);
         SAFE_PARAM(line->getMaterialInst(), "useOriginalColor", CastI(textNodeSettings.useFontColor));
      }
   }
   else
   {
      for (auto& line : m_text->m_textLines)
      {
         //line->setMaterialInst(m_sdfCommonMaterial);
         line->setFillColor(m_mapObjectData.textNodeSetting.textSetting.color);
         line->setStrokeColor(vec4::zero);
         SAFE_PARAM(line->getMaterialInst(), "outlineBorder", 0.f);
         SAFE_PARAM(line->getMaterialInst(), "useOriginalColor", CastI(textNodeSettings.useFontColor));
         //SAFE_PARAM(line->getMaterialInst(), "useOriginalColor", (textNodeSettings.useFontColor) ? 1 : 0);
      }
   }
}
