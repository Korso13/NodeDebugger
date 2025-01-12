#pragma once

#include "W4Framework.h"
#include "others/SPARK/SPARK_Tools.h"
#include "Spawnable.h"

W4_USE_UNSTRICT_INTERFACE

static SPK::BlendMode toSPKBlend(SPK_MATERIAL mat) {
	switch (mat)
	{
	case SPK_MATERIAL::ADDALPHA:
		return SPK::BlendMode::BLEND_MODE_NONE;
		break;
	case SPK_MATERIAL::ALPHA:
		return SPK::BlendMode::BLEND_MODE_ALPHA;
		break;
	case SPK_MATERIAL::ADD:
		return SPK::BlendMode::BLEND_MODE_ADD;
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
	SPKParticle_S(MapObjectsData& spawnData, sptr<Node> root = Render::getRoot())
		:
		Spawnable(spawnData),
		SPKParticle(spawnData.objName, root)
	{
		setLocalPos(spawnData.pos);
		loadMaterialsData();
		node->setName(spawnData.objName);
#ifdef DEBUG
		initSpawnable(node);
#else
		Spawnable::initSpawnable(node);
#endif // DEBUG
	}

protected:

	void loadMaterialsData()
	{
		if (!getMapObjectsData().SPARK_Materials.empty())
		{
			for (size_t i = 0; i < system->getNbGroups(); i++)
			{
				if (getMapObjectsData().SPARK_Materials.size() <= i)
					continue;

				applySpkMaterialToGroup(system, toSPKBlend(getMapObjectsData().SPARK_Materials[i]), i);
			}
		}
	}

	void initSpawnable(sptr<Node> rootNode) override
	{
		Spawnable::initSpawnable(rootNode);

		for (size_t i = 0; i < system->getNbGroups(); i++)
		{
			if (getMapObjectsData().SPARK_Materials.size() <= i)
			{
				getMapObjectsData().SPARK_Materials.push_back(SPK_MATERIAL::ADDALPHA);
			}

			//we can add setting with vector, but it doesn't callback when its contents changed :(
			addSpawnableSetting(SpawnSetting(
				"Material Group " + STR(i),
				getMapObjectsData().SPARK_Materials[i],
				[this, i](SPK_MATERIAL& material)
				{
					getMapObjectsData().SPARK_Materials[i] = material; //doesn't work well without it
					loadMaterialsData();
				}
			));

			//for debugging, not exported for now
			addSpawnableSetting(SpawnSetting(
				"Group Orientation " + STR(i),
				SPK_ORIENTATION::CAMERA_PLANE_ALIGNED,
				[this, i](SPK_ORIENTATION& orientation)
				{
					setRendererOrientation(system, i, toSPKOrient(orientation));
				}
			));
		}

		addSpawnableBtn("Flatten", [this]() {flattenSPKSystem(node); });
		addSpawnableBtn("Restart", [this]() {restartSPKEmitter(system); });
	}
};