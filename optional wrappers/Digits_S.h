#pragma once

#include "nodeDebugger/spawner/SpawnerWrappers.h"
#include "MonospaceDigitsPlane.h"

class Digits_S : public SceneObject
{
public:

   Digits_S(MapObjectsData& mod, sptr<Node> rootNode);
   void setValue(float value, bool bInstant = false, bool saveValueToMod = false);
   void setValue(int value, bool bInstant = false, bool saveValueToMod = false);
   sptr<MonospaceDigitsPlane> getDigits() {return m_digits;};

   void spawnableSelfUpdate(float dt) override;

   //MapObjectsData& getMapObjectsData() override;
   //adds minimum required settings to spawn digits
   static void checkModRequiredValues(MapObjectsData& mod);

protected:

   void initSpawnable(sptr<Node> rootNode) override;
   //void onChildSpawned(sptr<Spawnable> newChild) override;

   std::pair<int, int> breakFloatIntoInts(const float value) const;

private:

   void onBeginPlay(sptr<Spawnable> selfRef, bool isTabSpawned, vec3 spawnLocation) override;
   void onEndPlay(sptr<Spawnable> selfRef, const std::string& objId) override;

   void createFloatingPointPart();
   void createPrefix();
   void createSuffix();
   void createComma();

private:

   bool m_renderFloatingPart = false;
   int m_floatAccuracy = 2;
   sptr<MonospaceDigitsPlane> m_digits = nullptr;
   sptr<MonospaceDigitsPlane> m_floatingPoint = nullptr;
   sptr<Mesh> m_commaMesh = nullptr;
   sptr<Mesh> m_prefixMesh = nullptr;
   sptr<Mesh> m_suffixMesh = nullptr;
};


