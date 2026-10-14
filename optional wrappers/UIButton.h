#pragma once

#include "W4Framework.h"
#include "common/Templates.h"
#include "common/nodeDebugger/spawner/Spawnable.h"
#include "common/nodeDebugger/spawner/SpawnerWrappers.h"
#include "Nodes/Mesh.h"

W4_USE_UNSTRICT_INTERFACE

#define UI_CLICK_TIME 0.2 //full time for press + unpress animation
#define HOVER_SCALE_UP 1.1

class UIButton : public Updatable
{
public:

	explicit UIButton(sptr<Mesh> mesh, bool isDontShowForGoogle = false);
	explicit UIButton(sptr<Node> nodeForBtnRoot, sptr<Mesh> mesh, bool isDontShowForGoogle = false);
	//UIButton constructor
	static sptr<UIButton> make(
		sptr<Mesh> mesh,
		std::function<void(const CollisionInfo& hitInfo)> onPressCallback = nullptr,
		std::function<void(const CollisionInfo& hitInfo)> onReleaseCallback = nullptr,
		std::string name = "UIButton",
		bool isDontShowForGoogle = false
		);
	virtual ~UIButton();

	void update(float dt) override;
	virtual sptr<Mesh> getMesh() { return m_mesh; }
	sptr<Node> getRoot() { return m_root; }

	void blockClickability(bool bBlocked);
	void blockClickAnim(bool bBlocked);
	virtual void setPressCollider(std::function<void(const CollisionInfo& hitInfo)> onPressCallback);
	virtual void setReleaseCollider(std::function<void(const CollisionInfo& hitInfo)> onReleaseCallback);

	//highlights and scales up the button
	virtual void setHovered(bool isHovered, bool instant = false);

	virtual Mesh& operator*() { return *m_mesh; }
	virtual sptr<Mesh> operator->() { return m_mesh; }

protected:

	UIButton() = delete;

	virtual void createCollider();
	virtual void playPressAnim();
	virtual void playReleaseAnim();

protected:

	sptr<Node> m_root = nullptr;
	sptr<Mesh> m_mesh = nullptr;
	sptr<Collider> m_collider = nullptr;

	sptr<CustomTweens::Tween<vec3>> m_press = nullptr;
	const vec3 m_meshScale = vec3::one;
	bool m_isPressed = false;
	bool m_isHovered = false;
	bool m_isBlocked = false;
	bool m_isBlockedAnim = false;

	//w4::event::Handle::id_t m_onTouchEndHandlerId;
	w4::sptr<Handle> m_onTouchEndHandler;

private:

	const bool m_isDontShowForGoogle;
};

//====================================
// Spawnable wrapper for UIButton
//====================================
class UIButton_S : public SceneObject
{
public:
	//objName = mesh name
	UIButton_S(MapObjectsData& mod, sptr<Node> parentNode);
	sptr<UIButton> getButton() { return m_button; }
	//MapObjectsData& getMapObjectsData() override;

protected:

	void initSpawnable(sptr<Node> rootNode) override;
	void onChildSpawned(sptr<Spawnable> newChild) override;

private:

	void onBeginPlay(sptr<Spawnable> selfRef, bool isTabSpawned, vec3 spawnLocation) override;
	void onEndPlay(sptr<Spawnable> selfRef, const std::string& objId) override;

private:

	sptr<UIButton> m_button = nullptr;
};