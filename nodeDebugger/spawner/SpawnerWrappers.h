#pragma once

#include "W4Framework.h"
#include "others/SPARK/SPARK_Tools.h"
#include "Spawnable.h"
#include "SpawnerFactory.h"
//#include "common/nodeDebugger/customGuiTabs/SpawnerTab.h"
#include "others/utils/Subscriber.h"


W4_USE_UNSTRICT_INTERFACE

//====================================
// Spawnable simple wrapper
//====================================

class SceneObject : public Spawnable, public Subscriber, public CommonTypes::Movable
{
public:

	SceneObject(MapObjectsData& mod, sptr<Node> parentNode, bool bSpawnChildren = true, bool bCallSpawnableInit = true);
	~SceneObject() override;

	sptr<Node> getRoot() override { return m_root; }

	MapObjectsData& getMapObjectsData() override;
	sptr<Mesh> getChildMesh(const std::string& name) const {return getChildMeshByName(std::forward<const std::string&>(name));}
	std::vector<wptr<Mesh>> getAllChildMeshes(bool bForceRenewCache = false);
	sptr<Spawnable> getChildObjectById(const std::string& name);

	void spawnableSelfUpdate(float dt) override;
	void forceUpdatePlacementStrategy();

protected:

	void initSpawnable(sptr<Node> rootNode) override;
	virtual void onChildSpawned(sptr<Spawnable> newChild);

	void onBeginPlay(sptr<Spawnable> selfRef, bool isTabSpawned, vec3 spawnLocation) override;
	void onEndPlay(sptr<Spawnable> selfRef, const std::string& objId) override;

	virtual void setupPlacementStrategy();

protected:

	sptr<Node> m_root = nullptr;
	const bool m_needToSpawnChildren = true;
	sptr<class PlacementStrategies> m_placementStrategy = nullptr;
	std::vector<wptr<Mesh>> m_childMeshesCache;
};


//====================================
// SPKParticle wrapper
//====================================

static SPK::BlendMode toSPKBlend(SPK_MATERIAL mat) {
	switch (mat)
	{
	case SPK_MATERIAL::ADDALPHA:
		return SPK::BlendMode::BLEND_MODE_ADD;
		break;
	case SPK_MATERIAL::ALPHA:
		return SPK::BlendMode::BLEND_MODE_ALPHA;
		break;
	case SPK_MATERIAL::ADD:
		return SPK::BlendMode::BLEND_MODE_NONE;
		break;
	default:
		return SPK::BlendMode::BLEND_MODE_NONE;
		break;
	}
}
static SPK::OrientationPreset toSPKOrient(SPK_ORIENTATION orientation) { return static_cast<SPK::OrientationPreset>(orientation); }

class SPKParticle_S : public SPKParticle, public Spawnable
{
public:
	SPKParticle_S(MapObjectsData& spawnData, sptr<Node> root = Render::getRoot());

	static MapObjectsData& getDefaultSpawnSettings()
	{
		static MapObjectsData spawnSparkData;
		spawnSparkData.className = CLASS2STR(SPKParticle_S);
		spawnSparkData.objName = ""; //default name of spark effect to load
		return spawnSparkData;
	}

	void restart();

protected:

	void loadMaterialsData();
	MapObjectsData& getMapObjectsData() override;
	void initSpawnable(sptr<Node> rootNode) override;
};