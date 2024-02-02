//Made by Timur Korsakov 06.09.23
//Use it to place, scale, rotate node-based objects on the screen + move camera and directional light. 
//Outputs transform data in json-ready format - just copy and paste!
#pragma once

#include "W4Framework.h"
#include <array>
#include "OnUpdateProvider.h"

#include "common/NodeDebugger/ND_Defines.h"
#include "common/NodeDebugger/ND_ImGuiUI.h"

W4_USE_UNSTRICT_INTERFACE

//quick access macros
#define NODEDEBUG NodeDebugger::get()
#define ND_NODE(node) NodeDebugger::get()->addImGuiNode(node)
#define ND_SPINE(spine) NodeDebugger::get()->addSpine(spine)
#define ND_VAR(name, variable) NodeDebugger::get()->addDebugVariable(name, variable, nullptr)
#define ND_VARCB(name, variable, callback) NodeDebugger::get()->addDebugVariable(name, variable, callback)
#define ND_BTN(name, callback) NodeDebugger::get()->addDebugCBButton(name, callback)

//Controlls map:
/*
	C - camera debug mode
	L - directional light debug mode
	R - switch rotation output mode (radians|degrees)
	P - switch on/off click placement mode

	For camera mode:
	A-D - move camera by X
	W-S - move camera by Y
	Q-E - move camera by Z

	For light mode:
	A-D - change directional light direction vector by X
	W-S - change directional light direction vector by Y
	Q-E - change directional light direction vector by Z


	Node transformations:

	1-9 digits - select debugged node. Nodes are set via addNode()
	also
	1 - Gizmo Translate Mode
	2 - Gizmo Rotate Mode
	3 - Gizmo Scale Mode

	Arrow Left and Right - move by x
	Arrow Up and Down	 - move by y
	Page Up and Down	 - move by z

	NumPad1 - NumPad3 - rotation along y axis
	NumPad2 - NumPad5 - rotation along x axis
	NumPad4 - NumPad6 - rotation along z axis

	Numpad+ and Numpad-  - scale in all 3 dimentions
	Home - End			 - scale by x
	Insert - Delete		 - scale by y
	[  -  ]				 - scale by z

	Minus - Equal						- Decrease / increase translation step
	Numpad7 - Numpad9					- Decrease / increase rotation step
	NumpadDivide(/) - NumpadMultiply(*)	- Decrease / increase scaling percent

	H - print key mapping info
	N - print all conected nodes

	Any other key - print selected node's translation
*/

class EditorCamera;

class NodeDebugger : public ND_ImGui, public OnUpdateProvider
{
public:

	static NodeDebugger* get()
	{
		if (!m_instance)
		{
			m_instance = new NodeDebugger();
		}

		return m_instance;
	}

	//DO NOT CALL THIS METHOD!!! NodeDebugger is self-updating class!
	void onUpdate(float dt) override NOCOMP;

	void addNode(sptr<Node> inNode, const unsigned int assignButton) NOCOMP;
	void purgeDebugger() NOCOMP;

	//Debugger is ON by default and is automatically disabled for shipping / release builds
	void enableDebugger(bool state) { isDebuggerOn = state; };
	void switchToCamControl() { camControlOn = true; };
	void switchToLightControl() { camControlOn = false; };
	void setRotationOutputType(bool isDegrees) { degreesOutput = isDegrees; }

	void setRotationStep(float step) { m_rotStep = step; };
	void setTranslationStep(float step) { m_translateStep = step; };
	void setScalingStep(float stepPercent) { m_scaleStep = stepPercent; };
	void setFloatAccuracy(int accuracy) NOCOMP;
	void setCallbackForButton(platform::KeyboardKey key, std::function<void()> cb) NOCOMP;

	void setClickMeshZ(float targetZ) NOCOMP;
	void onClick(const Touch::Begin& clickEvent) NOCOMP;

	//Use this to alter game time scale - multiply your dt by this value
	const float getTimeScale() const NOCOMP_F;

private:

	NodeDebugger() NOCOMP;

	//Old keybord-controlled debugger functions
	void onKeyPressed(const event::Keyboard::Down& evt) NOCOMP;
	void printConnectedNodes() NOCOMP;

private:

	static NodeDebugger* m_instance;

	float m_rotStep = 0.01f;
	float m_translateStep = SFF("debugNode_moveStep", 0.1f);
	float m_scaleStep = 0.01f;

	std::map< platform::KeyboardKey, std::function<void()>> m_buttonCb;
	std::array<sptr<Node>, 10> m_nodeSwitchBoard;
	unsigned int m_selectedNode = 0;

	sptr<Mesh> m_clickMesh;
	bool m_clickPlacementMode = false;
};