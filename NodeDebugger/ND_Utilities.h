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
	void ImGuiNodeSettings();
	void ImGuiSettings();
	void resetImGuiRepeatElemId();

	bool ImGuiSearchBar(std::string& lastSearchVal);
	//use to filter which items to show after getting filter from searchbar. Returns true if no filter is set or filter string found in itemName(case non-sensitive!)
	bool checkFilterConditions(const std::string& itemName, const std::string& filter);

	void ImGuiTweensBody(sptr<Node> _node, tweenDebugRecord& tweenRecord);
	void ImGuiTweens(sptr<Node> _node);

	static std::map<sptr<Node>, tweenDebugRecord> debugTweensMove;
	static std::map<sptr<Node>, tweenDebugRecord> debugTweensScale;
	static int lastSelectedTweenSample = 0;

	static bool loadedEasings = false;
	static const char* easingNames[31];
}

