//ImGui UI for NodeDebugger
#pragma once

#include "W4Framework.h"
#include "common/NodeDebugger/ND_Defines.h"

W4_USE_UNSTRICT_INTERFACE

class SpineAnimationBase;
class EditorCamera;
class iGuiTab;

class ND_ImGui
{
public:

	ND_ImGui() NOCOMP;

	void onGui(float dt) NOCOMP;

	void addImGuiNode(sptr<Node> inNode, std::string altName = "default") NOCOMP;
	void removeImGuiNode(const std::string nodeName) NOCOMP;
	void removeAllImGuiNodes() NOCOMP;

	void addSpine(sptr<SpineAnimationBase> spineToDebug, std::string altName = "default") NOCOMP;
	void addSpine(sptr<Spine> spineToDebug, std::string altName = "default") NOCOMP;

	void addSpotLight(sptr<SpotLight> light, const std::string& altName = "default") NOCOMP;

	void addDebugCBButton(std::string name, std::function<void()> cb) NOCOMP;
	
	template<typename T>
	void addDebugVariable(std::string name, T* variablePtr, std::function<void()> cb) NOCOMP;

	void setMinimumImGuiLogLevel(LogLevel level) { minLoggingLevel = CastI(level); };
	const bool checkRaycastCollision() const NOCOMP_B;

	sptr<iGuiTab> getCustomTabByName(std::string tabName) NOCOMP_PTR;

private:

	//called on init
	void initCustomTabs() NOCOMP;

	//ImGui NodeDebugger methods
	void ImGuiCamera() NOCOMP;
	void ImGuiConsole() NOCOMP;
	void ImGuiNodes() NOCOMP;

protected:

	//ImGUI
	sptr<ImGuiNode> m_imguiNode;

	bool m_foldImGuiMenu = false;
	bool m_windowFocused = false;

	float m_imGuiFontSize = IMG_FONTSIZE; 

	int m_firstUpdates = 5;

	ImVec2 m_ImGWindowSize = IMG_STARTSIZE;
	vec4 m_clearColor = Render::getScreenCamera()->getClearColor();

	std::unordered_map<sptr<Node>, ImG_Node> m_staticNodes;
	std::string m_nodesFilter = "";
	int m_nodeFilterItems = 0;

	std::unordered_map<std::string, std::function<void()>> m_debugCallbacks;

	std::vector<sptr<iGuiTab>> m_extraTabs;

	//Editor camera
	sptr<EditorCamera> m_editorCamera;
};

#ifndef FORCENODEDEBUG_OFF
#include <sources/common/NodeDebugger/CustomGuiTabs/VariablesTab.h>

template<typename T>
inline void ND_ImGui::addDebugVariable(std::string name, T* variablePtr, std::function<void()> cb)
{
	if (!getCustomTabByName("Variables"))
    return;

    cast_sptr<VariablesTab>(getCustomTabByName("Variables"))->addItemTyped(name, variablePtr, cb);
}
#endif
