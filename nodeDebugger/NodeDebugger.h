//Made by Timur Korsakov 06.09.23
//Use it to place, scale, rotate node-based objects on the screen + move camera and directional light. 
//Outputs transform data in json-ready format - just copy and paste!
#pragma once

#include "W4Framework.h"
#include <array>
#include "OnUpdateProvider.h"

#include "common/nodeDebugger/ND_Defines.h"
#include "common/nodeDebugger/ND_ImGuiUI.h"

W4_USE_UNSTRICT_INTERFACE

//quick access macros
#define NODEDEBUG NodeDebugger::get()
#define ND_NODE NodeDebugger::get()->addImGuiNode
#define ND_NNODE(node, name) NodeDebugger::get()->addImGuiNode(node, name)
#define ND_SPINE(spine) NodeDebugger::get()->addSpine(spine)
#define ND_VAR(name, variable) NodeDebugger::get()->addDebugVariable(name, variable, nullptr)
#define ND_VARCB(name, variable, callback) NodeDebugger::get()->addDebugVariable(name, variable, callback)
#define ND_FVAR(folder, name, variable) NodeDebugger::get()->addDebugVariable("[" + folder + "]" + name, variable, nullptr)
#define ND_FVARCB(folder, name, variable, callback) NodeDebugger::get()->addDebugVariable("[" + folder + "]" + name, variable, callback)
#define ND_COLLECTION(name, collection) NodeDebugger::get()->addDebugCollection(name, collection)
#define ND_BTN NodeDebugger::get()->addDebugCBButton

//kinda breakpoint that stops playable update. Can be resumed via NodeDebugger GUI
#ifndef FORCENODEDEBUG_OFF
	#define ND_BRK Game::getInstance()->setTimeScale(0.f)
#else 
	#define ND_BRK ;
#endif

//Use this before casting any rays to check if you are clicking ImGui interface. Returns false if mouse is above ImGui window
#define ND_CASTALLOWED !NodeDebugger::get()->checkRaycastCollision()
#define ND_HOTKEY(key, callback) NodeDebugger::get()->setCallbackForButton(key, callback)

//Controlls map:
/*
	Spacebar - collapse/show debugger window
	F2 - enable/disable debug camera
	Shift (when debug camera is on) - speed up camera movement

	C - standard camera keyboard-debug mode
	L - directional light keyboard-debug mode
	R - switch rotation output mode (radians|degrees)
	P - switch on/off click placement mode

	For standard camera mode:
	A-D - move camera by X
	W-S - move camera by Y
	Q-E - move camera by Z

	For debug camera mode:
	A-D - move camera by X
	W-S - move camera by Z
	Q-E - move camera by Y

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

class NodeDebugger : public ND_ImGui, protected OnUpdateProvider
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
	
protected:

	void onUpdate(float dt) override NOCOMP;

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