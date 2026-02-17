//ImGui UI for NodeDebugger
#pragma once

#include "W4Framework.h"
#include "common/nodeDebugger/ND_Defines.h"

W4_USE_UNSTRICT_INTERFACE

class SpineAnimationBase;
class EditorCamera;
class iGuiTab;

class ND_ImGui
{
public:

	ND_ImGui() NOCOMP;

	void ImGuiUpdate(float dt)NOCOMP;
	void onGui(float dt) NOCOMP;

	void addImGuiNode(sptr<Node> inNode, const std::string& altName = "default") NOCOMP;
	void removeImGuiNode(const std::string& nodeName) NOCOMP;
	void removeAllImGuiNodes() NOCOMP;

	void addSpine(sptr<SpineAnimationBase> spineToDebug, std::string altName = "default") NOCOMP;
	void addSpine(sptr<Spine> spineToDebug, std::string altName = "default") NOCOMP;

	void addSpotLight(sptr<SpotLight> light, const std::string& altName = "default") NOCOMP;

	void addDebugCBButton(std::string name, std::function<void()> cb) NOCOMP;
	
	template<typename T>
	void addDebugVariable(std::string name, T* variablePtr, std::function<void()> cb) NOCOMP;
	
	template<typename Container, typename VT = typename Container::value_type>
	void addDebugCollection(std::string name, Container& collection) NOCOMP;
	template<typename VT>
	void addDebugCollection(std::string name, std::vector<VT>& collection)NOCOMP;

	void setMinimumImGuiLogLevel(LogLevel level) { minLoggingLevel = CastI(level); };
	const bool checkRaycastCollision() const NOCOMP_B;

	sptr<iGuiTab> getCustomTabByName(std::string tabName) NOCOMP_PTR;

protected:

	// used for ImGuiColliderProbe()
	void probeColliders() NOCOMP;

private:

	//called on init
	void initCustomTabs() NOCOMP;
	void setUiTheme(const std::string themeName) NOCOMP;

	void onResize(const event::Resize& evt) NOCOMP;

	//ImGui NodeDebugger methods
	void ImGuiCamera() NOCOMP;
	void ImGuiColliderProbe() NOCOMP;
	void ImGuiConsole() NOCOMP;

protected:

	//ImGUI
	sptr<ImGuiNode> m_imguiNode;

	bool m_foldImGuiMenu = false;
	bool m_windowFocused = false;
	bool m_isCameraTabActive = false;

	float m_imGuiFontSize = IMG_FONTSIZE; 

	vec4 m_clearColor = Render::getScreenCamera()->getClearColor();

	std::unordered_map<std::string, std::function<void()>> m_debugCallbacks;
	std::vector<sptr<iGuiTab>> m_extraTabs;

	//Editor camera
	sptr<EditorCamera> m_editorCamera;

	struct ColliderProbeData
	{
		std::string colliderDisplayName;
		Node* parent = nullptr;
		std::string_view hitParentName;
		vec3 colliderPos;
		vec3 colliderSize;
		bool blocksScreencasts;
		bool blocksRaycasts;
	};
	std::vector<ColliderProbeData> m_lastColliderProbe;
};

#ifndef FORCENODEDEBUG_OFF
#include <sources/common/nodeDebugger/customGuiTabs/VariablesTab.h>

template<typename T>
inline void ND_ImGui::addDebugVariable(std::string name, T* variablePtr, std::function<void()> cb)
{
	if (!getCustomTabByName("Variables"))
		return;

    cast_sptr<VariablesTab>(getCustomTabByName("Variables"))->addItemTyped(name, variablePtr, cb);
}

template<typename VT>
inline void ND_ImGui::addDebugCollection(std::string name, std::vector<VT>& collection)
{
	if (!getCustomTabByName("Variables"))
		return;

	cast_sptr<VariablesTab>(getCustomTabByName("Variables"))->addCollectionTyped<std::vector<VT>>(name, collection);
}

template<typename Container, typename VT>
inline void ND_ImGui::addDebugCollection(std::string name, Container& collection)
{
	if (!getCustomTabByName("Variables"))
		return;

	cast_sptr<VariablesTab>(getCustomTabByName("Variables"))->addCollectionKeyTyped<Container>(name, collection);
}
#endif
