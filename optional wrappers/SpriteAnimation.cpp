//
// created by maxim on 19.10.2022
//

#include "SpriteAnimation.h"

#include "looped/LoopedSprite.h"
#include "settings/CustomSettingsContainer.h"
#include "game/playerController/PlayerController.h"

SpriteAnimation::SpriteAnimation(
	const std::string& configPath,
	sptr<Node> root,
	const std::string& nameSprite,
	const std::function<void()>& onFinish)
	:
	m_currentFrame(0),
	m_finishCallback(onFinish),
	m_isComplete(false)
{
	auto& settingContainer = CustomSettingsContainer<CommonTypes::SpritesContainerSetting>::instance();
	if (!settingContainer.isInited())
	{
		settingContainer.setFunctionGetContainer([=](nlohmann::json value) -> CommonTypes::SpritesContainerSetting
		{
			auto ret = nlohmann::getSpritesContainerSetting(value);
			return ret;
		});
		settingContainer.init(configPath + "spriteContainerSettings");
	}
	m_infoSprite = settingContainer.getSetting(nameSprite);
	m_view = make::sptr<Node>();
	root->addChild(m_view, false);
	initialize();
}

void SpriteAnimation::update(float& dt)
{
	if (m_isComplete) return;

	updateDefault(dt);
}

void SpriteAnimation::restart()
{
	m_currentFrame = 0.f;
	m_isComplete = false;
	float start = 0.001;
	m_object->setEnabled(true);
	update(start);
}

void SpriteAnimation::updateDefault(float& dt)
{
	if (m_object)
	{
		float speed = m_infoSprite.speed_animation;

		m_currentFrame += (dt * speed);
		int sf = CastI(floor(m_currentFrame));
		if (m_currentFrame >= CastF(m_infoSprite.count_frames))
		{
			SAFE_PARAM(m_materialI, "frameNum", CastF(m_infoSprite.count_frames - 1));
			endAnimation();
		}
		else {
			SAFE_PARAM(m_materialI, "frameNum", CastF(sf));
		}
	}
}

void SpriteAnimation::initialize()
{
	m_materialI = createMaterial();
	createPlane();
}

void SpriteAnimation::setDisableOnEnd(bool flag)
{
	m_disableOnEnd = flag;
}

int SpriteAnimation::getCurrentFrame() const
{
	return floor(m_currentFrame);
}

void SpriteAnimation::setJpgMode(bool enabled)
{
	m_isJpgMode = true;
	if (m_materialI)
		m_materialI->setBlendFunc(BlendFactor::SRC_ALPHA, BlendFactor::ONE_MINUS_SRC_COLOR);
}

void SpriteAnimation::endAnimation()
{
//	W4_LOG_DEBUG("[SpriteAnimation] EndAnimation %s", m_object->getName().c_str());

	if (m_object)
	{
		if (m_finishCallback) m_finishCallback();
		if (m_disableOnEnd)
		{
			m_object->setEnabled(false);
		}

		m_isComplete = true;
	}
}

SpriteAnimation::~SpriteAnimation()
{
//	W4_LOG_DEBUG("call SpriteAnimation::~SpriteAnimation");
	m_view->removeAllChildren();
}

void SpriteAnimation::createPlane()
{
	vec2 size = m_infoSprite.planeSize / vec2{static_cast<float>(m_infoSprite.number_frame_x), static_cast<float>(m_infoSprite.number_frame_y)};
	float speed = m_infoSprite.speed_animation;
	int render = m_infoSprite.renderOrder;
	vec3 scale = vec3{ m_infoSprite.scale };
	vec3 pos = m_infoSprite.pos;
	vec3 rotate = m_infoSprite.rotate;

	m_object = Mesh::create::plane(size);
	m_view->addChild(m_object);
	m_object->setLocalRotation(Rotator(rotate * DEG2RAD));
	m_object->setLocalScale(vec3(scale));
	m_object->setMaterialInst(m_materialI);
	m_object->setRenderOrder(render);
	m_object->setLocalTranslation(pos);
}

sptr<MaterialInst> SpriteAnimation::createMaterial() const
{
	//DBG::WLE<char>(m_infoSprite.materialName, ' ');

	auto mat = ResourceLibrary::instance().getMaterial(m_infoSprite.materialName);
	auto txt =
		ResourceLibrary::instance().getTexture(m_infoSprite.textureName); // TODO:: come up with loading textures by url

	auto matI = mat->createInstance();
	matI->setBlendFunc(BlendFactor::SRC_ALPHA, BlendFactor::ONE_MINUS_SRC_ALPHA);
	matI->enableBlending(true);
	SAFE_PARAM(matI, "texture0", txt);
	SAFE_PARAM(matI, "countX", CastF(m_infoSprite.number_frame_x));
	SAFE_PARAM(matI, "countY", CastF(m_infoSprite.number_frame_y));
	SAFE_PARAM(matI, "frameNum", CastF(m_currentFrame));
	SAFE_PARAM(matI, "color", vec4::one);

	if(m_isJpgMode)
		matI->setBlendFunc(BlendFactor::SRC_ALPHA, BlendFactor::ONE_MINUS_SRC_COLOR);

	return matI;
}

SpriteAnimation_S::SpriteAnimation_S(MapObjectsData& mod, sptr<Node> parentNode)
	:
	SceneObject(mod, parentNode, false, false),
	m_isLopped(mod.settings.flags["SpA_isLooped"])
{
	if (m_isLopped)
		m_spriteAnimation = M_SPTR<LoopedSprite>("config/", m_root, m_mapObjectData.objName);
	else
	{
		m_spriteAnimation = M_SPTR<SpriteAnimation>("config/", m_root, m_mapObjectData.objName);
		if (!mod.settings.flags["SpA_startOnSpawn"])
			m_spriteAnimation->m_currentFrame = m_spriteAnimation->m_infoSprite.count_frames;
	}
	if (mod.settings.flags["SpA_isJpegMode"]) m_spriteAnimation->setJpgMode(true);
	m_spriteAnimation->setDisableOnEnd(mod.settings.flags["SpA_disableOnEnd"]);
	setUpdatableName(m_mapObjectData.objName);
	SpriteAnimation_S::initSpawnable(m_root);
	spawnAllChildren();
}

void SpriteAnimation_S::update(float dt)
{
	if (!m_spriteAnimation) return;
	if (!m_isLopped && m_spriteAnimation->isComplete()) return;

	m_spriteAnimation->updateDefault(dt);
}

void SpriteAnimation_S::initSpawnable(sptr<Node> rootNode)
{
	SceneObject::initSpawnable(rootNode);
	if (!m_isLopped)
	{
		addSpawnableBtn("Restart", std::bind(&SpriteAnimation::restart, m_spriteAnimation.get()));
		addSpawnableSetting(SpawnSetting("Play on start", m_mapObjectData.settings.flags["SpA_startOnSpawn"]));
		addSpawnableSetting(SpawnSetting("Disable on end?", m_mapObjectData.settings.flags["SpA_disableOnEnd"],
		[this](bool& var)
			{
				m_spriteAnimation->setDisableOnEnd(var);
			}));
	}
	addSpawnableSetting(SpawnSetting("Is looped?", m_isLopped));
	addSpawnableSetting(SpawnSetting("Current Frame", m_spriteAnimation->m_currentFrame));
	addSpawnableSetting(SpawnSetting("Speed (not exported!)", m_spriteAnimation->m_infoSprite.speed_animation));
	addSpawnableSetting(SpawnSetting("Is Jpeg texture?", m_mapObjectData.settings.flags["SpA_isJpegMode"],
		[this](bool& var)
			{
				m_spriteAnimation->setJpgMode(var);
			}));

	//todo: add speed, ro, scale override settings
}

void SpriteAnimation_S::onBeginPlay(sptr<Spawnable> selfRef, bool isTabSpawned, vec3 spawnLocation)
{
	if (isTabSpawned)
		setWorldPos(spawnLocation);
	else
		setWorldPos(m_mapObjectData.pos);
	PLAYER_CONTROLLER.addUpdatable(cast_sptr<SpriteAnimation_S>(selfRef));
}

void SpriteAnimation_S::onEndPlay(sptr<Spawnable> selfRef, const std::string& objId)
{
	PLAYER_CONTROLLER.removeUpdatable(cast_sptr<SpriteAnimation_S>(selfRef));
}

std::vector<std::string> SpriteAnimation_S::getLoadedSpriteAnims()
{
	std::vector<std::string> found;

	const std::string path = "config/";
	const std::string fileToLoad = "spriteContainerSettings";
	const std::string ext = ".json";
	for (int i = -1; i < 10; ++i)
	{
		std::string finitePath = path + fileToLoad;
		if (i >= 0) finitePath += STR(i);
		finitePath += ext;
		if (w4::filesystem::isPathExists(finitePath))
		{
			auto file = w4::filesystem::open(finitePath);
			auto json = nlohmann::json::parse(file->data(), file->data() + file->size());
			for (auto& value : json["settings"])
			{
				if (value.contains("name"))
					found.push_back(value["name"]);
			}
		}
		else
			break;
	}
	return found;
}