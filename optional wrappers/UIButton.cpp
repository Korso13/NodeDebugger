#include "UIButton.h"
#include <CommonJsInterface.h>
#include <common/Defines.h>

#include "common/nodeDebugger/spawner/SpawnerFactory.h"
#include "game/playerController/PlayerController.h"
#include "others/utils/ScreenSpaceShader.h"
#include "ui/PlacementStrategies.h"

UIButton::UIButton(sptr<Mesh> mesh, bool isDontShowForGoogle)
	: m_mesh(mesh), m_meshScale(mesh->getWorldScale()), m_isDontShowForGoogle(isDontShowForGoogle)
{
	debug("[UIButton::UIButton]");
	m_root = make::sptr<Node>();
	m_onTouchEndHandler = event::Touch::End::subscribe([this](const Touch::End& evt)
		{
			playReleaseAnim();
		});
	w4::js::CommonJsInterface::subscribeOnCancel([this]() {playReleaseAnim(); }); //no way to unsubscribe for now, may cause issues on deleted btns in the future :( Try checking this in that case
}

UIButton::UIButton(sptr<Node> nodeForBtnRoot, sptr<Mesh> mesh, bool isDontShowForGoogle)
	: m_mesh(mesh), m_meshScale(mesh->getWorldScale()), m_isDontShowForGoogle(isDontShowForGoogle)
{
	debug("[UIButton::UIButton]");
	m_root = nodeForBtnRoot;
	if (mesh->getParent() != m_root)
	{
		m_root->addChild(mesh);
	}
	mesh->setLocalTranslation(vec3::zero);
	setUpdatableName(mesh->getName() + "_Btn");
	if (m_isDontShowForGoogle && IS_GOOGLE) {
		mesh->setEnabled(false);
	}
	m_onTouchEndHandler = event::Touch::End::subscribe([this](const Touch::End& evt)
		{
			playReleaseAnim();
		});
	w4::js::CommonJsInterface::subscribeOnCancel([this]() {playReleaseAnim(); }); //no way to unsubscribe for now, may cause issues on deleted btns in the future :( Try checking this in that case
}

sptr<UIButton> UIButton::make(
	sptr<Mesh> mesh, 
	std::function<void(const CollisionInfo& hitInfo)> onPressCallback, 
	std::function<void(const CollisionInfo& hitInfo)> onReleaseCallback, 
	std::string name,
	bool isDontShowForGoogle
	)
{
	sptr<UIButton> btn = make::sptr<UIButton>(mesh, isDontShowForGoogle);
	auto parent = btn->m_mesh->getParent();
	if (parent) parent->removeChild(btn->m_mesh); //to prevent rebuilding indices
	if (name == "UIButton") name = name + "_" + STR(utils::UIDGenerator::get());
	btn->m_mesh->setName(name);
	btn->m_root->setName(name + "_root");
	if (parent) parent->addChild(btn->m_root);
	btn->m_root->setWorldTranslation(btn->m_mesh->getWorldTranslation());
	btn->m_root->addChild(btn->m_mesh);
	btn->m_mesh->setLocalTranslation(vec3::zero);
	btn->setUpdatableName(name);

	if (btn->m_isDontShowForGoogle && IS_GOOGLE) {
		btn->getMesh()->setEnabled(false);
	}
	else {
		btn->setPressCollider(onPressCallback);
		btn->setReleaseCollider(onReleaseCallback);
	}

	return btn;
}

UIButton::~UIButton()
{
	debug("[UIButton::~UIButton] %s", getUpdatableName().c_str());
	if (m_mesh)
	{
		m_mesh->removeAllChildren();
		if (m_mesh->getParent()) m_mesh->getParent()->removeChild(m_mesh);
	}
	m_root->removeAllChildren();
	m_onTouchEndHandler->unsubscribe();
	setRemoveFlag(true);
}

void UIButton::update(float dt)
{
	if (m_press)
	{
		m_press->updValue(dt);
		m_press->checkCb();
		if (m_press->checkIsDone())
			m_press = nullptr;
	}
}

void UIButton::blockClickability(bool bBlocked)
{
	m_isBlocked = bBlocked;
	m_collider->setBlockScreencasts(!bBlocked);
	m_collider->setBlockRaycasts(!bBlocked);
}

void UIButton::blockClickAnim(bool bBlocked)
{
	m_isBlockedAnim = bBlocked;
}

void UIButton::setPressCollider(std::function<void(const CollisionInfo& hitInfo)> onPressCallback)
{
	createCollider();

	m_collider->setScreencastCallback(ScreencastEvent::Down, 
		[this, onPressCallback](const CollisionInfo& hitInfo)
		{
			if (m_isBlocked) {
				info("[%s]blocked!", getUpdatableName().c_str());
				return;
			}
			//if btn is based on Hud - check if screencast originated from HudPass (see ScreenSpaceShader class)
			if (m_root->getRoot() == Render::getHudPass().getRoot() && !(hitInfo.source && hitInfo.source->getParent()->getName() == "screenFx_camera"))
				return;

			if (onPressCallback) onPressCallback(hitInfo);
			playPressAnim();
		});
}

void UIButton::setReleaseCollider(std::function<void(const CollisionInfo& hitInfo)> onReleaseCallback)
{
	createCollider();

	m_collider->setScreencastCallback(ScreencastEvent::Up, 
		[this, onReleaseCallback](const CollisionInfo& hitInfo)
		{
			if (m_isBlocked) {
				info("[%s]blocked!", getUpdatableName().c_str());
				return;
			}
			//if btn is based on Hud - check if screencast originated from HudPass (see ScreenSpaceShader class)
			if (m_root->getRoot() == Render::getHudPass().getRoot() && !(hitInfo.source && hitInfo.source->getParent()->getName() == "screenFx_camera"))
			{
				info("[%s]Hud based btn, but not ScreenFX screencast! Ignoring", getUpdatableName().c_str());
				return;
			}

			if (onReleaseCallback) onReleaseCallback(hitInfo);
			playReleaseAnim();
		});
}

void UIButton::setHovered(bool isHovered, bool instant)
{
	if ((m_isHovered == isHovered) || m_isPressed || m_isBlockedAnim) return;
	
	m_isHovered = isHovered;
	if (isHovered)
	{
		SAFE_PARAM(m_mesh, "color", vec4(1.4, 1.4, 1.4, 1.f));
		if (m_press) { m_press->updValue(UI_CLICK_TIME * 1.1); m_press->checkCb(); }
		if (instant)
		{
			m_root->setLocalScale(vec3(HOVER_SCALE_UP));
			return;
		}

		vec3 startScale = vec3::one;
		float animTime = UI_CLICK_TIME / 2.f;
		m_press = make::sptr<CustomTweens::Tween<vec3>>(
			startScale,
			[this](vec3 newScale)
			{m_root->setLocalScale(newScale); }
		);
		m_press->addMore(
			vec3(HOVER_SCALE_UP),
			animTime,
			Easing::SineInOut
		);
	}
	else
	{
		SAFE_PARAM(m_mesh, "color", vec4::one);
		if (m_press) { m_press->updValue(UI_CLICK_TIME * 1.1); m_press->checkCb(); }
		if (instant)
		{
			m_root->setLocalScale(vec3::one);
			return;
		}

		vec3 startScale = m_root->getLocalScale();
		float animTime = UI_CLICK_TIME / 2.f;
		m_press = make::sptr<CustomTweens::Tween<vec3>>(
			startScale,
			[this](vec3 newScale)
			{m_root->setLocalScale(newScale); }
		);
		m_press->addMore(
			vec3::one,
			animTime,
			Easing::SineInOut
		);
	}
}

void UIButton::createCollider()
{
	if (m_collider)
		return;

	vec3 colliderSize = getMeshPlaneSize(m_mesh);
	colliderSize.z = 0.001;

	m_collider = m_root->addCollider<core::AABB>(getUpdatableName() + "_collider", colliderSize);
	//m_collider = m_root->addCollider<core::AABB>(getUpdatableName() + "_collider", m_mesh);
	m_collider->setReceiveRaycasts(true);
	m_collider->setBlockScreencasts(true);
	m_collider->setBlockRaycasts(true);
	m_collider->setLocalTranslation(vec3::zero);
}

void UIButton::playPressAnim()
{
	if (m_isPressed || !m_root || m_isBlockedAnim)
		return;

	info("[UIButton::playPressAnim]");
	m_isPressed = true;
	setHovered(false, true);
	if (m_press) { m_press->updValue(UI_CLICK_TIME * 1.1); m_press->checkCb(); }
	vec3 startScale = (m_root->getLocalScale() > vec3(0.8)) ? m_root->getLocalScale() : vec3::one;
	float animTime = (UI_CLICK_TIME / 2.f) * ((startScale - vec3(0.8)).length() / vec3(0.2).length());
	m_press = make::sptr<CustomTweens::Tween<vec3>>(
		startScale,
		[this](vec3 newScale)
			{
				m_root->setLocalScale(newScale);
				m_collider->setLocalScale(vec3::one/newScale);
			}
	); 
	m_press->addMore(
		vec3(0.8), 
		animTime,
		Easing::SineInOut 
	); 
	SAFE_PARAM(m_mesh, "color", vec4(0.5, 0.5, 0.5, 1.f)); 
}

void UIButton::playReleaseAnim()
{
	if (!m_isPressed || !m_root)
		return;

	info("[UIButton::playReleaseAnim]");
	m_isPressed = false;
	if (m_press) { m_press->updValue(UI_CLICK_TIME * 1.1); m_press->checkCb(); }
	vec3 startScale = (m_root->getLocalScale() < vec3::one) ? m_root->getLocalScale() : vec3(0.8);
	float animTime = (UI_CLICK_TIME / 2.f) * ((vec3::one - startScale).length() / vec3(0.2).length());
	m_press = make::sptr<CustomTweens::Tween<vec3>>(
		vec3(0.8),
		[this](vec3 newScale)
			{
				m_root->setLocalScale(newScale);
				m_collider->setLocalScale(vec3::one/newScale);
			}
	);
	m_press->addMore(
		vec3::one,
		UI_CLICK_TIME / 2.f,
		Easing::SineInOut
	);
	SAFE_PARAM(m_mesh, "color", vec4::one);
}

UIButton_S::UIButton_S(MapObjectsData& mod, sptr<Node> parentNode)
	:
SceneObject(mod, parentNode, false, true)
{
	//info("[UIButton_S::UIButton_S] Root {%s}, address {%s}", m_root->getName().c_str(), SPAWNER.getNodeHierarchyAddress(m_root).c_str());
	if (mod.objId.empty())
	{
		mod.objId = SPAWNER.checkUniqueName(mod.objName + "_Btn");
		m_root->setName(mod.objId);
	}
	if (m_mapObjectData.childMeshes.empty()) //used when creating btn from Spawner tab
	{
		m_mapObjectData.childMeshes.emplace_back(make::sptr<MapMeshData>());
		m_mapObjectData.childMeshes.back()->className = "Mesh";
		m_mapObjectData.childMeshes.back()->isChild = true;
		m_mapObjectData.childMeshes.back()->meshName = mod.objName;
		m_mapObjectData.childMeshes.back()->meshId = SPAWNER.checkUniqueMeshName("BtnMesh_" + mod.objName);
		m_mapObjectData.childMeshes.back()->node = SPAWNER.getNodeHierarchyAddress(m_root);
		m_mapObjectData.childMeshes.back()->useWorldPos = false;
		m_mapObjectData.childMeshes.back()->position_local = vec3::zero;
		m_mapObjectData.childMeshes.back()->renderOrder = 2;
	}
	spawnAllChildren();
	auto buttonMesh = getChildMeshByName("BtnMesh_" + mod.objName);
	if (buttonMesh)
		m_button = M_SPTR<UIButton>(m_root, buttonMesh);
	else
	{
		err("[UIButton_S::UIButton_S]Can't find mesh %s, using any child mesh available!",
			("BtnMesh_" + mod.objName).c_str());
		auto childMeshes = getSpawnedChildMeshes();
		if (!childMeshes.empty() && !childMeshes[0].child.expired())
			m_button = M_SPTR<UIButton>(m_root, childMeshes[0].child.lock());
		else
			err("[UIButton_S::UIButton_S] No child meshes found!");
	}
	if (m_button)
	{
		m_button->getMesh()->setLocalTranslation(vec3::zero);
		m_button->setPressCollider(nullptr);
		m_button->setReleaseCollider(nullptr);
	}
}

void UIButton_S::initSpawnable(sptr<Node> rootNode)
{
	//SceneObject::initSpawnable(rootNode); //no need to call - called from SceneObject itself
	if (m_placementStrategy)
		addSpawnableSetting(SpawnSetting("Placement Strategy",m_placementStrategy));
	else
		addSpawnableBtn("Add placement strategy",
			[this]()
				{
					if (m_placementStrategy)
						return;
					m_placementStrategy = M_SPTR<PlacementStrategies>();
					float distToCamera = 0.f;
					if (m_root->getParent() == Render::getHudPass().getRoot())
						distToCamera = 10.f;
					else
						distToCamera = neAbs(Render::getScreenCamera()->getWorldTranslation().z);

					m_placementStrategy->setContext(m_root, m_button->getMesh(), distToCamera);
					addSpawnableSetting(SpawnSetting("Placement Strategy",m_placementStrategy));
					m_placementStrategy->enableAutoResize();
					removeSpawnableBtn("Add placement strategy");
				});
}

void UIButton_S::onChildSpawned(sptr<Spawnable> newChild)
{
	if (newChild->getSpawnableRoot())
	{
		m_root->addChild(newChild->getSpawnableRoot());
		newChild->getSpawnableRoot()->setLocalTranslation(newChild->getMapObjectsData().local_pos);
	}
	//SceneObject::onChildSpawned(newChild);
}

void UIButton_S::onBeginPlay(sptr<Spawnable> selfRef, bool isTabSpawned, vec3 spawnLocation)
{
	if (isTabSpawned)
		setWorldPos(spawnLocation);
	else
		setWorldPos(m_mapObjectData.pos);

	if (m_mapObjectData.settings.flags["use_placement_strategy"] && m_button)
	{
		m_placementStrategy = PlacementStrategies::makeStrategyFromMOD(m_mapObjectData, m_root, m_button->getMesh());
		m_placementStrategy->onResize();
		addSpawnableSetting(SpawnSetting("Placement Strategy",m_placementStrategy));
		removeSpawnableBtn("Add placement strategy");
	}
	UIButton_S::initSpawnable(m_root);
	if (m_button)
		PLAYER_CONTROLLER.addUpdatable(m_button);
}

void UIButton_S::onEndPlay(sptr<Spawnable> selfRef, const std::string& objId)
{
	PLAYER_CONTROLLER.removeUpdatable(m_button);
}
