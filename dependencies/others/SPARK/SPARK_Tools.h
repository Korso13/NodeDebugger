#pragma once
#include "W4Framework.h"
#include "SPARK.h"
#include "SparkParticleSystemNode.h"
#include "common/Defines.h"
#include "common/CommonTypes.h"
#include "common/nodeDebugger/NodeDebugger.h"

W4_USE_UNSTRICT_INTERFACE

inline static bool SPKInited = false;

using SPKNode = particles::spark::SparkParticleSystemNode;

class SPKParticle;
//forward declarations for SPKParticle
static void unloadSPKEmitter(SPKParticle& particle); 
static std::pair<sptr<SPKNode>, SPK::Ref<SPK::System>> loadSparkEmitter(const std::string& effectName, sptr<Node> root = Render::getRoot());
static void SPKSettingsInit();
static void setRendererOrientation(sptr<SPKParticle> particle, size_t groupIndx, SPK::OrientationPreset orientation);
static void applySpkMaterial(sptr<SPKParticle> particle, SPK::BlendMode material, std::set<int> excludedGroups = std::set<int>());
static void applySpkMaterialToGroup(sptr<SPKParticle> particle, SPK::BlendMode material, int group);
static void changeColorInterpolator(sptr<SPKParticle> particle, std::vector<SPK::Color> newColors, size_t groupIndex = 0);
static void flattenSPKSystem(sptr<SPKParticle> particle);
static void unloadSPKEmitter(sptr<SPKParticle> particle);
static void unloadSPKEmitter(SPKParticle& particle);
static void restartSPKEmitter(sptr<SPKParticle> particle);

class SPKParticle : public CommonTypes::Movable
{
	friend class SPKParticle_S;
	friend void setRendererOrientation(sptr<SPKParticle> particle, size_t groupIndx, SPK::OrientationPreset orientation);
	friend void applySpkMaterial(sptr<SPKParticle> particle, SPK::BlendMode material, std::set<int> excludedGroups);
	friend void applySpkMaterialToGroup(sptr<SPKParticle> particle, SPK::BlendMode material, int group);
	friend void changeColorInterpolator(sptr<SPKParticle> particle, std::vector<SPK::Color> newColors, size_t groupIndex);
	friend void flattenSPKSystem(sptr<SPKParticle> particle);
	friend void unloadSPKEmitter(sptr<SPKParticle> particle);
	friend void unloadSPKEmitter(SPKParticle& particle);
	friend void restartSPKEmitter(sptr<SPKParticle> particle);

public:
	SPKParticle(sptr<SPKNode> _node, SPK::Ref<SPK::System> _system)
		:
		m_node(_node),
		m_system(_system)
	{
		if (!SPKInited) SPKSettingsInit();
		m_root = make::sptr<Node>("SPKParticle_root");
		m_root->setParent(m_node->getParent());
		m_root->addChild(m_node); m_node->setLocalTranslation(vec3::zero);
		bInited = true;
	};

	SPKParticle(std::pair<sptr<SPKNode>, SPK::Ref<SPK::System>> _particle)
		:
		m_node(_particle.first),
		m_system(_particle.second)
	{
		if (!SPKInited) SPKSettingsInit();
		m_root = make::sptr<Node>("SPKParticle_root");
		m_root->setParent(m_node->getParent());
		m_root->addChild(m_node); m_node->setLocalTranslation(vec3::zero);
		bInited = true;
	};

	explicit SPKParticle(const std::string& effectName, sptr<Node> root = Render::getRoot())
	{
		if (!SPKInited) SPKSettingsInit();
		m_root = make::sptr<Node>("SPKParticle_root[" + effectName + "]");
		m_root->setParent(root);
		auto particle = loadSparkEmitter(effectName, m_root);
		if (!particle.first)
		{
			bInited = false;
			return;
		}
		m_node = particle.first;
		m_system = particle.second;
		bInited = true;
	};

	//ctor that uses a separate SPARK emitter for inverted effect (after calling setEmitterInverted(true))
	explicit SPKParticle(const std::string& effectName, const std::string& invertedEffectName, sptr<Node> root = Render::getRoot())
	{
		if (!SPKInited) SPKSettingsInit();
		m_root = make::sptr<Node>("SPKParticle_root[" + effectName + "]");
		m_root->setParent(root);
		auto particle = loadSparkEmitter(effectName, m_root);
		if (!particle.first)
		{
			bInited = false;
			return;
		}
		m_node = particle.first;
		m_system = particle.second;

		auto invertedEmitter = loadSparkEmitter(invertedEffectName, m_root);
		if (!invertedEmitter.first)
			return;
		m_nodeInverted = invertedEmitter.first;
		m_systemInverted = invertedEmitter.second;
		m_nodeInverted->setEnabled(false);

		ND_NODE(m_node, m_root->getName()+"_default");
		ND_NODE(m_nodeInverted, m_root->getName()+"_inverted");
		bInited = true;
	};

	static sptr<SPKParticle> make(const std::string& effectName, sptr<Node> root = Render::getRoot())
	{
		return make::sptr<SPKParticle>(effectName, root);
	}

	SPKParticle() = delete;

	~SPKParticle()
	{
		unloadSPKEmitter(*this);
	}

	sptr<Node> getRoot() override {return m_root;}
	void setWorldScale(vec3 newPos) { m_root->setWorldScale(newPos); }
	void setLocalScale(vec3 newPos) { m_root->setLocalScale(newPos); }
	vec3 getWorldScale() const { return m_root->getWorldScale(); }
	vec3 getLocalScale() const { return m_root->getLocalScale(); }
	
	void setRenderOrder(int32_t renderOrder) { m_node->setRenderOrder(renderOrder); if (m_nodeInverted) m_nodeInverted->setRenderOrder(renderOrder); }
	int32_t getRenderOrder() const { return m_node->getRenderOrder(); }

	//mirrorMod only used if there is no m_nodeInverted and if bIsInverted == true
	void setEmitterInverted(bool bIsInverted, const vec2& mirrorModInverted = vec2::zero)
	{
		if (m_nodeInverted)
		{
			m_node->setEnabled(!bIsInverted);
			m_nodeInverted->setEnabled(bIsInverted);
		}
		else
		{
			m_node->foreachSurface(
				[this, bIsInverted, mirrorModInverted](Surface& surf)
					{
						SAFE_PARAM(surf.getMaterialInst(), "mirrorMod", (bIsInverted ? mirrorModInverted : vec2::zero));
					});
		}
	}

	SPK::Ref<SPK::System>& getMainSystem() { return m_system; }
	std::optional<SPK::Ref<SPK::System>> getInvertedSystem() { if (!m_nodeInverted) return std::nullopt; return m_systemInverted; }

public:
	bool bInited = false;

private:
	sptr<Node> m_root = nullptr;

	sptr<SPKNode> m_node = nullptr;
	SPK::Ref<SPK::System> m_system = SPK::NullReferenceValue();
	sptr<SPKNode> m_nodeInverted = nullptr;
	SPK::Ref<SPK::System> m_systemInverted = SPK::NullReferenceValue();
};

static void SPKSettingsInit()
{
	if (SPKInited)
		return;
	SPKInited = true;
	SPK::System::setClampStep(true, 0.1f); // clamp the step to 100 ms
	SPK::System::useAdaptiveStep(0.001f, 0.01f); // use an adaptive step from 1ms to 10ms (1000fps to 100fps)
}

static void applySpkMaterialToGroup(SPK::Ref<SPK::System>& system, SPK::BlendMode material, int group);//forward decl
static std::pair<sptr<SPKNode>, SPK::Ref<SPK::System>> loadSparkEmitter(const std::string& effectName, sptr<Node> root)
{
	auto emitterNode = make::sptr<SPKNode>();
	root->addChild(emitterNode);
	emitterNode->setWorldTranslation(root->getWorldTranslation());

	if (!w4::filesystem::isPathExists("Spark/Effects/" + effectName + ".xml"))
	{
		err("Spark Effect file %s not found!", CSTR("Spark/Effects/" + effectName + ".xml"));
		return std::pair<sptr<SPKNode>, SPK::Ref<SPK::System>>(nullptr, SPK::NullReferenceValue());
	}
	SPK::Ref<SPK::System> emitterSystem = emitterNode->loadFromFile("Spark/Effects/" + effectName + ".xml");
	
	//guessing correct material based on texture extension or material name (material takes priority)
	//requires patched SparkParticleSystemQuadRenderer.h !
	for (size_t groupIdx = 0; groupIdx < emitterSystem->getNbGroups(); groupIdx++)
	{
		auto rendererAttrib = emitterSystem->getGroup(groupIdx)->getRenderer()->exportAttributes();
		auto TextureAttrib = rendererAttrib.getAttributeWithValue("texture");
		auto MaterialAttrib = rendererAttrib.getAttributeWithValue("material");
		bool setMaterial = false;
		if (MaterialAttrib)
		{
			std::string materialName = MaterialAttrib->getValue<std::string>(); 
			if (!materialName.empty())
			{
				setMaterial = true;
				if(materialName == "add_alpha")
					applySpkMaterialToGroup(emitterSystem, SPK::BLEND_MODE_ADD, groupIdx);
				else if(materialName == "alpha")
					applySpkMaterialToGroup(emitterSystem, SPK::BLEND_MODE_ALPHA, groupIdx);
			}
		}
		if (TextureAttrib && !setMaterial)
		{
			auto textureName = TextureAttrib->getValue<std::string>();
			if (textureName.find(".png") != std::string::npos)
			{
				applySpkMaterialToGroup(emitterSystem, SPK::BLEND_MODE_ALPHA, groupIdx);
			}
			else if (textureName.find(".jpg") != std::string::npos)
			{
				applySpkMaterialToGroup(emitterSystem, SPK::BLEND_MODE_ADD, groupIdx);
			}

		}
	}

	return { emitterNode, emitterSystem };
}

//BLEND_MODE_ADD = ParticleMaterialAddAlpha(use it with jpg atlases); BLEND_MODE_ALPHA = ParticleMaterialAlpha (use it with png-based emmitters); BLEND_MODE_NONE = unused/unknown; 
static void applySpkMaterial(SPK::Ref<SPK::System>& system, SPK::BlendMode material, std::set<int> excludedGroups = std::set<int>())
{
	if (!system)
		return;

	render::BlendFactor srcBlendFunc, dstBlendFunc;
	bool blendingEnabled = false;

	switch (material)
	{
	case SPK::BLEND_MODE_NONE:
		srcBlendFunc = render::BlendFactor::ONE;
		dstBlendFunc = render::BlendFactor::ZERO;
		blendingEnabled = false;
		break;
	case SPK::BLEND_MODE_ADD:
		srcBlendFunc = render::BlendFactor::SRC_ALPHA;
		dstBlendFunc = render::BlendFactor::ONE;
		blendingEnabled = true;
		break;
	case SPK::BLEND_MODE_ALPHA:
		srcBlendFunc = render::BlendFactor::SRC_ALPHA;
		dstBlendFunc = render::BlendFactor::ONE_MINUS_SRC_ALPHA;
		blendingEnabled = true;
		break;
	default:
		SPK_LOG_WARNING("[applySpkMaterial] - Unsupported blending mode. Nothing happens");
		return;
		break;
	}

	for (size_t i = 0; i < system->getNbGroups(); i++)
	{
		if (excludedGroups.contains(i))
			continue;

		SPK::dynamicCast<particles::spark::SparkParticleSystemQuadRenderer>(system->getGroup(i)->getRenderer())->m_surface->getMaterialInst()->enableBlending(blendingEnabled);
		SPK::dynamicCast<particles::spark::SparkParticleSystemQuadRenderer>(system->getGroup(i)->getRenderer())->m_surface->getMaterialInst()->setBlendFunc(srcBlendFunc, dstBlendFunc);
	}
}
static void applySpkMaterial(sptr<SPKParticle> particle, SPK::BlendMode material, std::set<int> excludedGroups)
{
	if (!particle->bInited)
		return;

	applySpkMaterial(particle->m_system, material, excludedGroups);
	if (particle->m_nodeInverted)
		applySpkMaterial(particle->m_systemInverted, material, excludedGroups);
}

static void applySpkMaterialToGroup(SPK::Ref<SPK::System>& system, SPK::BlendMode material, int group)
{
	if (!system)
		return;

	std::set<int> excludedGroups;
	for (size_t i = 0; i < system->getNbGroups(); i++)
	{
		if (i != group)
			excludedGroups.insert(i);
	}
	applySpkMaterial(system, material, excludedGroups);
}
static void applySpkMaterialToGroup(sptr<SPKParticle> particle, SPK::BlendMode material, int group)
{
	if (!particle->bInited)
		return;

	applySpkMaterialToGroup(particle->m_system, material, group);
	if (particle->m_nodeInverted)
		applySpkMaterialToGroup(particle->m_systemInverted, material, group);
}

static void changeColorInterpolator(SPK::Ref<SPK::System>& system, std::vector<SPK::Color> newColors, size_t groupIndex = 0)
{
	if (!system)
		return;

	auto attribs = system->getGroup(groupIndex)->getColorInterpolator()->exportAttributes();
	for (size_t i = 0; i < attribs.getNbAttributes(); i++)
	{
		if (attribs.getAttribute(i).getType() == SPK::IO::AttributeType::ATTRIBUTE_TYPE_COLORS)
		{
			attribs.getAttribute(i).setValues<SPK::Color>(&newColors[0], newColors.size()); 
			system->getGroup(groupIndex)->getColorInterpolator()->importAttributes(attribs);
			break;
		}
	}
}
static void changeColorInterpolator(sptr<SPKParticle> particle, std::vector<SPK::Color> newColors, size_t groupIndex)
{
	if (!particle->bInited)
		return;

	changeColorInterpolator(particle->m_system, newColors, groupIndex);
	if (particle->m_nodeInverted)
		changeColorInterpolator(particle->m_systemInverted, newColors, groupIndex);
}

static void setRendererOrientation(SPK::Ref<SPK::System>& system, size_t groupIndx, SPK::OrientationPreset orientation)
{
	if (!system || groupIndx > system->getNbGroups()-1)
		return;

	SPK::dynamicCast<particles::spark::SparkParticleSystemQuadRenderer>(system->getGroup(groupIndx)->getRenderer())->setOrientation(orientation);
}

static void setRendererOrientation(sptr<SPKParticle> particle, size_t groupIndx, SPK::OrientationPreset orientation)
{
	if (!particle->bInited || groupIndx > particle->m_system->getNbGroups()-1)
		return;

	SPK::dynamicCast<particles::spark::SparkParticleSystemQuadRenderer>(particle->m_system->getGroup(groupIndx)->getRenderer())->setOrientation(orientation);
	if (particle->m_nodeInverted)
		SPK::dynamicCast<particles::spark::SparkParticleSystemQuadRenderer>(particle->m_system->getGroup(groupIndx)->getRenderer())->setOrientation(orientation);
}

//removes paralax effect and RO issues
static void flattenSPKSystem(sptr<SPKNode>& node)
{
	if (!node)
		return;

	vec3 scale = node->getWorldScale();
	scale.z = 0.0000001;
	node->setWorldScale(scale);
}

static void flattenSPKSystem(sptr<SPKParticle> particle)
{
	if (!particle->bInited)
		return;

	flattenSPKSystem(particle->m_node);
	flattenSPKSystem(particle->m_nodeInverted);
}

static void unloadSPKEmitter(sptr<SPKNode>& node, SPK::Ref<SPK::System>& system)
{
	if (node)
	{
		node->setAutomaticOnUpdate(false);
		node->getParent()->removeChild(node);
	}

	system.reset();
	system = SPK::NullReferenceValue();

	if (node)
		node->m_system = SPK::NullReferenceValue();

	node.reset();
	node = nullptr;
}

static void unloadSPKEmitter(SPKParticle& particle)
{
	unloadSPKEmitter(particle.m_node, particle.m_system);
	if (particle.m_nodeInverted)
		unloadSPKEmitter(particle.m_nodeInverted, particle.m_systemInverted);
}

static void unloadSPKEmitter(sptr<SPKParticle> particle)
{
	unloadSPKEmitter(particle->m_node, particle->m_system);
	if (particle->m_nodeInverted)
		unloadSPKEmitter(particle->m_nodeInverted, particle->m_systemInverted);
}

static void restartSPKEmitter(SPK::Ref<SPK::System>& system)
{
	if (!system)
		return;

	for (size_t j = 0; j < system->getNbGroups(); j++)
		for (size_t i = 0; i < system->getGroup(j)->getNbEmitters(); i++)
		{
			if (system) system->getGroup(j)->getEmitter(i)->resetTank();
			if (system) system->getGroup(j)->getEmitter(i)->setActive(true);
		}
}

static void restartSPKEmitter(sptr<SPKParticle> particle)
{
	if (!particle->bInited)
		return;

	if (particle->m_node->isEnabled())
		restartSPKEmitter(particle->m_system);
	if (particle->m_nodeInverted && particle->m_nodeInverted->isEnabled())
		restartSPKEmitter(particle->m_systemInverted);
}

static SPK::Vector3D getVector3D(const vec3& vec)
{
	return SPK::Vector3D(vec.x, vec.y, vec.z);
}