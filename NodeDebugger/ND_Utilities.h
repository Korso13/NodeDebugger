//Various utility functions for NodeDebugger components
#pragma once

#include "W4Framework.h"
#include "common/NodeDebugger/ND_Defines.h"

W4_USE_UNSTRICT_INTERFACE

namespace ND_Util
{
	std::string jsstring(vec3 vec);
	vec2 vec2FromGui(float inVec[2]);
	vec3 vec3FromGui(float inVec[3]);
	vec3 vec3FromJSString(std::string jsstring);

	void updateTweens(float& dt);

	void printDebugNodeInfo(sptr<Node> printNode);
	void printKeymapping();

	void setSkin(sptr<Spine> spine, const std::string& name);
	void addSkin(sptr<Spine> spine, const std::string& name);

	void ImGuiTransforms(sptr<Node> _node);
	void ImGuiCopyVec3Btn(vec3 vec);
	void ImGuiNodeSettings(bool bNoGizmoSettings = false);
	void NoConflictGizmo(sptr<Node> _node, const std::string& name);
	void ImGuiSettings();
	void resetImGuiRepeatElemId();
	void resetPlotterWindowFlag();

	ImVec2 getSizeForChild(float windowHeightPercent = 0.5);

	bool ImGuiSearchBar(std::string& lastSearchVal);
	//use to filter which items to show after getting filter from searchbar. Returns true if no filter is set or filter string found in itemName(case non-sensitive!)
	bool checkFilterConditions(const std::string& itemName, const std::string& filter);

	void ImGuiTweensBody(sptr<Node> _node, tweenDebugRecord& tweenRecord);
	void ImGuiTweenPlotter(sptr<Node> _node, tweenDebugRecord& tweenRecord);
	void plotterDockWindowWrapper();
	void ImGuiTweens(sptr<Node> _node);

	static std::map<sptr<Node>, tweenDebugRecord> debugTweensMove;
	static std::map<sptr<Node>, tweenDebugRecord> debugTweensScale;
	static std::map<sptr<Node>, tweenDebugRecord> debugTweensRotate;

	static std::map<tweenTransformType, int> lastSelectedTweenSample = 
	{
		{tweenTransformType::MOVE,	-1},
		{tweenTransformType::SCALE, -1},
		{tweenTransformType::ROTATE,-1},
	};

	static bool bShowPlotter = false;
	static bool callSeparatePlotterWindow = false;
	static bool needDrawPlotterWindow = false;
	static sptr<Node> separatePlotterWindow_node = nullptr;
	static tweenTransformType separatePlotterWindow_recordType = tweenTransformType::MOVE;

	static bool loadedEasings = false;

	static std::string m_lastGizmoNode = "";
	static const char* easingNames[31];
}

#ifndef FORCENODEDEBUG_OFF

#define NDS ND_Settings::get()

class ND_Settings
{
public:

	static ND_Settings* get()
	{
		if (!m_instance)
		{
			m_instance = new ND_Settings();
		}

		return m_instance;
	}

private:

	ND_Settings() = default;

public:

	vec3 m_lastCopiedVec3 = vec3::zero;

private: 

	static ND_Settings* m_instance;
};

#endif