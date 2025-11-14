#pragma once

#include "UiText.h"
#include "common/nodeDebugger/spawner/SpawnerStructs.h"
#include "common/nodeDebugger/spawner/SpawnerWrappers.h"


//====================================
// Simple spawnable wrapper for UiText_S
//====================================

class  UiText_S : public SceneObject
{
public:
   UiText_S(MapObjectsData& mod, sptr<Node> parentNode);

   sptr<UiText> getText() {return m_text;};
   MapObjectsData& getMapObjectsData() override;

   void changeText(const std::string& newText);

protected:

   void initSpawnable(sptr<Node> rootNode) override;
   void onChildSpawned(sptr<Spawnable> newChild) override;
private:

   void onBeginPlay(sptr<Spawnable> selfRef, bool isTabSpawned, vec3 spawnLocation) override;
   void onEndPlay(sptr<Spawnable> selfRef, const std::string& objId) override;

   void applyStroke();
   static void textNodeRebuilder(sptr<UiText> textNode, const std::string text, TextSettings& textSettings);

private:
   sptr<UiText> m_text = nullptr;
   sptr<MaterialInst> m_sdfCommonMaterial;
};