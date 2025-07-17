#pragma once

#include "W4Framework.h"
#include "others/SPARK/SPARK_Tools.h"
#include "Spawnable.h"

W4_USE_UNSTRICT_INTERFACE

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
	SPKParticle_S(MapObjectsData& spawnData, sptr<Node> root = Render::getRoot())
		:
		Spawnable(spawnData),
		SPKParticle(spawnData.objName, root)
	{
		setLocalPos(spawnData.pos);
		loadMaterialsData();
		node->setName(spawnData.objName);
		if (spawnData.settings.integers["RO"] == 0)
			spawnData.settings.integers["RO"] = 2;

		spawnData.settings.integers["RO"] = 2;
		setRenderOrder(spawnData.settings.integers["RO"]);

		if (!spawnData.settings.flags["use_w_scale"] && spawnData.settings.vec3s["l_scale"] != vec3::zero)
		{
			node->setLocalScale(spawnData.settings.vec3s["l_scale"]);
		}
		else if (spawnData.settings.vec3s["w_scale"] != vec3::zero)
		{
			node->setWorldScale(spawnData.settings.vec3s["w_scale"]);
		}
		flattenSPKSystem(node);

#ifdef DEBUG
		initSpawnable(node);
#else
		Spawnable::initSpawnable(node);
#endif // DEBUG
	}

	static MapObjectsData& getDefaultSpawnSettings()
	{
		static MapObjectsData spawnSparkData;
		spawnSparkData.className = CLASS2STR(SPKParticle_S);
		spawnSparkData.objName = ""; //default name of spark effect to load
		return spawnSparkData;
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
		else
		{
			//prevents memory re-allocations and crashes due to pointer invalidation
			getMapObjectsData().SPARK_Materials.reserve(system->getNbGroups());
			getMapObjectsData().SPARK_Materials.resize(system->getNbGroups());

			for (size_t i = 0; i < system->getNbGroups(); i++)
			{
				//guessing correct material based on texture extension or material name (material takes priority)
				//requires patched SparkParticleSystemQuadRenderer.h !
				auto rendererAttrib = system->getGroup(i)->getRenderer()->exportAttributes();
				auto TextureAttrib = rendererAttrib.getAttributeWithValue("texture");
				auto MaterialAttrib = rendererAttrib.getAttributeWithValue("material");
				bool setMaterial = false;
				if (MaterialAttrib)
				{
					std::string materialName = MaterialAttrib->getValue<std::string>();
					if (!materialName.empty())
					{
						setMaterial = true;
						if (materialName == "add_alpha")
							getMapObjectsData().SPARK_Materials[i] = SPK_MATERIAL::ADDALPHA;
						else if (materialName == "alpha")
							getMapObjectsData().SPARK_Materials[i] = SPK_MATERIAL::ALPHA;
					}
				}
				if (TextureAttrib && !setMaterial)
				{
					auto textureName = TextureAttrib->getValue<std::string>();
					if (textureName.find(".png") != std::string::npos)
						getMapObjectsData().SPARK_Materials[i] = SPK_MATERIAL::ALPHA;
					else if (textureName.find(".jpg") != std::string::npos)
						getMapObjectsData().SPARK_Materials[i] = SPK_MATERIAL::ADDALPHA;
				}
			}
		}
	}

	MapObjectsData& getMapObjectsData() override
	{
		m_mapObjectData.settings.vec3s["l_scale"] = node->getLocalScale();
		m_mapObjectData.settings.vec3s["w_scale"] = node->getWorldScale();

		return Spawnable::getMapObjectsData();
	}

	void initSpawnable(sptr<Node> rootNode) override
	{
		Spawnable::initSpawnable(rootNode);

		//Orientation and material settings
		for (size_t i = 0; i < system->getNbGroups(); i++)
		{
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
				[this, i](SPK_ORIENTATION orientation)
				{
					setRendererOrientation(system, i, toSPKOrient(orientation));
				}
			));
		}

		addSpawnableSetting(SpawnSetting(
			"Render Order",
			m_mapObjectData.settings.integers["RO"],
			[this](int& value)
			{
				setRenderOrder(value);
			}
		));

		addSpawnableSetting(SpawnSetting(
			"Use World Scale?",
			m_mapObjectData.settings.flags["use_w_scale"],
			[this](bool& val)
			{
				if (val)
				{
					addSpawnableSetting(SpawnSetting(
						"Scale World",
						m_mapObjectData.settings.vec3s["w_scale"],
						[this](vec3& value)
						{
							node->setWorldScale(value);
							flattenSPKSystem(node);
						}
					));
					removeSpawnableSetting("Scale Local");
				}
				else
				{
					addSpawnableSetting(SpawnSetting(
						"Scale Local",
						m_mapObjectData.settings.vec3s["l_scale"],
						[this](vec3& value)
						{
							node->setLocalScale(value);
							flattenSPKSystem(node);
						}
					));
					removeSpawnableSetting("Scale World");
				}
			}
		));

		if(m_mapObjectData.settings.flags["use_w_scale"])
		{
			addSpawnableSetting(SpawnSetting(
				"Scale World",
				m_mapObjectData.settings.vec3s["w_scale"],
				[this](vec3& value)
				{
					node->setWorldScale(value);
					flattenSPKSystem(node);
				}
			));
		}
		else
		{
			addSpawnableSetting(SpawnSetting(
				"Scale Local",
				m_mapObjectData.settings.vec3s["l_scale"],
				[this](vec3& value)
				{
					node->setLocalScale(value);
					flattenSPKSystem(node);
				}
			));
		}

		addSpawnableBtn("Flatten", [this]() {flattenSPKSystem(node); });
		addSpawnableBtn("Restart", [this]() {restartSPKEmitter(system); });
	}
};