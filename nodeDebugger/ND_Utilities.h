//Various utility functions for NodeDebugger components
#pragma once

#include "W4Framework.h"
#include "common/nodeDebugger/ND_Defines.h"
#include <regex>

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

	void ImGuiNodeRender(sptr<Node> _node, const std::string& name); //todo: consider adding template with node subclass hint, because is<T>() might not work properly
	void ImGuiSpineRender(sptr<Spine> _spine, const std::string& name);
	bool ImGuiTransforms(sptr<Node> _node);
	void ImGuiCopyVec3Btn(vec3 vec);
	void ImGuiNodeSettings(bool bNoGizmoSettings = false);
	void NoConflictGizmo(sptr<Node> _node, const std::string& name);
	void ImGuiSettings();
	bool ImGuiNodeHierarchy(sptr<Node>& _nodeToSet, const std::string& nodePickerLabel = "Node");
	bool ImGuiTextureSelector(std::string& textureName, const std::string& selectorCaption = "<-Mesh Name");

	void resetImGuiRepeatElemId();
	void resetPlotterWindowFlag();

#ifndef FORCENODEDEBUG_OFF
	ImVec2 getSizeForChild(float windowHeightPercent = 0.5);
#endif

	bool ImGuiSearchBar(std::string& lastSearchVal);

	//use to filter which items to show after getting filter from searchbar. Returns true if no filter is set or filter string found in itemName(case non-sensitive!)
	bool checkFilterConditions(const std::string& itemName, const std::string& filter);
	
	//Templated combolist renderer. 
	//Returns pair: bool - set to true if new item was selected this frame; T - new selected value (if no new item selected, returns value by listCurrentValue key or random if key not found)
	template<typename T>
	std::pair<bool, T> ImGuiComboList(const std::string& listName, const std::map<std::string, T>& list, const std::string& listCurrentValue);
	//Returns pair: bool - set to true if new item was selected this frame; T - new selected value (if no new item selected, returns value by listCurrentValue key or random if key not found)
	template<typename T>
	std::pair<bool, T> ImGuiComboList(const std::string& listName, const std::vector<T>& list, const std::string& listCurrentValue, std::function<const std::string (const T&)> nameGetterPred);
	
	//Templated list box renderer. onRightMouseClickPopUpRender - allows adding ImGui contents to popup window triggered by right mouse click (return true to close the popup)
	//Returns pair: bool - set to true if new item was selected this frame; T - new selected value (if no new item selected, returns value by listCurrentValue key or random if key not found)
	template<typename T>
	std::pair<bool, T> ImGuiListBox(const std::string& listName, const std::map<std::string, T>& list, const std::string& listCurrentValue, std::function<bool(std::string key)>&& onRightMouseClickPopUpRender = nullptr);
	//Templated list box renderer. onRightMouseClickPopUpRender - allows adding ImGui contents to popup window triggered by right mouse click (return true to close the popup)
	//Returns pair: bool - set to true if new item was selected this frame; T - new selected value (if no new item selected, returns value by listCurrentValue key or random if key not found)
	template<typename T>
	std::pair<bool, T> ImGuiListBox(const std::string& listName, const std::vector<T>& list, const std::string& listCurrentValue, std::function<const std::string(const T&)> nameGetterPred, std::function<bool(const T& value)>&& onRightMouseClickPopUpRender = nullptr);
	//List box renderer for simple strings. onRightMouseClickPopUpRender - allows adding ImGui contents to popup window triggered by right mouse click (return true to close the popup)
	//Returns pair: bool - set to true if new item was selected this frame; std::string - new selected value (if no new item selected, returns listCurrentValue)
	std::pair<bool, std::string> ImGuiListBox(const std::string& listName, const std::set<std::string>& list, const std::string& listCurrentValue, std::function<bool(std::string value)>&& onRightMouseClickPopUpRender = nullptr);
	//todo: add variants of ImGuiComboList and ImGuiListBox for unordered_map (add templated iterator type checker?) 
	
	std::pair<bool, std::string> ImGuiMaterialSelector(const std::string& currentMaterial);

	//if variableName contains [FolderName] returns FolderName and removes [FolderName] from variableName
	std::optional<std::string> tryGetFolderName(std::string& variableName);

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

	//template method impls:
	template<typename T>
	std::pair<bool, T> ImGuiComboList(const std::string& listName, const std::map<std::string, T>& list, const std::string& listCurrentValue)
	{
		std::pair<bool, T> result;
		result.first = false;
#ifndef FORCENODEDEBUG_OFF
		if (!list.empty())
		{
			if (ImGui::BeginCombo(SAFE_IMG_NAME(listName), listCurrentValue.c_str()))
			{
				for (auto& [listKey, listValue] : list)
				{
					if (listKey.empty())
						continue;

					bool isSelected = (listKey == listCurrentValue);
					if (isSelected)
						ImGui::SetItemDefaultFocus();

					if (ImGui::Selectable(listKey.c_str(), &isSelected))
					{
						result.first = true;
						result.second = listValue;
					}
					if(!result.first && isSelected)
						result.second = listValue;
				}

				ImGui::EndCombo();
			}
		}
#endif
		return result;
	};
}

template<typename T>
std::pair<bool, T> ND_Util::ImGuiComboList(
	const std::string& listName, 
	const std::vector<T>& list, 
	const std::string& listCurrentValue, 
	std::function<const std::string (const T&)> nameGetterPred)
{
	std::pair<bool, T> result;
	result.first = false;
#ifndef FORCENODEDEBUG_OFF
	if (!list.empty())
	{
		if (ImGui::BeginCombo(SAFE_IMG_NAME(listName), listCurrentValue.c_str()))
		{
			for (const auto& listValue : list)
			{
				auto listKey = nameGetterPred(listValue);
				bool isSelected = (listKey == listCurrentValue);
				if (isSelected)
					ImGui::SetItemDefaultFocus();

				if (ImGui::Selectable(listKey.c_str(), &isSelected))
				{
					result.first = true;
					result.second = listValue;
				}
				if (!result.first && isSelected)
					result.second = listValue;
			}

			ImGui::EndCombo();
		}
	}
#endif
	return result;
}


template<typename T>
std::pair<bool, T> ND_Util::ImGuiListBox(
	const std::string& listName, 
	const std::map<std::string, T>& list, 
	const std::string& listCurrentValue,
	std::function<bool(std::string key)>&& onRightMouseClickPopUpRender)
{
	std::pair<bool, T> result;
	result.first = false;
#ifndef FORCENODEDEBUG_OFF
	if (!list.empty())
	{
		if (ImGui::BeginListBox(SAFE_IMG_NAME(listName), ImVec2(-FLT_MIN, std::min(CastI(list.size()), IMG_LISTBOX_ENTRIESINFRAME) * ImGui::GetTextLineHeightWithSpacing()))) //use all width, IMG_LISTBOX_ENTRIESINFRAME items tall
		{
			for (auto& [listKey, listValue] : list)
			{
				if (listKey.empty())
					continue;

				bool isSelected = (listKey == listCurrentValue);
				if (isSelected)
					ImGui::SetItemDefaultFocus();

				if (ImGui::Selectable(listKey.c_str(), &isSelected))
				{
					result.first = true;
					result.second = listValue;
				}
				if (onRightMouseClickPopUpRender && ImGui::BeginPopupContextItem())
				{
					if (onRightMouseClickPopUpRender(listKey))
						ImGui::CloseCurrentPopup();
					ImGui::EndPopup();
				}
				if (!result.first && isSelected)
					result.second = listValue;
			}

			ImGui::EndListBox();
		}
	}
#endif
	return result;
}

template<typename T>
std::pair<bool, T> ND_Util::ImGuiListBox(
	const std::string& listName, 
	const std::vector<T>& list, 
	const std::string& listCurrentValue, 
	std::function<const std::string (const T&)> nameGetterPred,
	std::function<bool(const T& value)>&& onRightMouseClickPopUpRender)
{
	std::pair<bool, T> result;
	result.first = false;
#ifndef FORCENODEDEBUG_OFF
	if (!list.empty())
	{
		if (ImGui::BeginListBox(SAFE_IMG_NAME(listName), ImVec2(-FLT_MIN, std::min(CastI(list.size()), IMG_LISTBOX_ENTRIESINFRAME) * ImGui::GetTextLineHeightWithSpacing()))) //use all width, IMG_LISTBOX_ENTRIESINFRAME items tall
		{
			for (const auto& listValue : list)
			{
				auto listKey = nameGetterPred(listValue);
				if (listKey.empty())
					continue;
				bool isSelected = (listKey == listCurrentValue);
				if (isSelected)
					ImGui::SetItemDefaultFocus();

				//debug("[%s] %s", listName.c_str(), listKey.c_str()); //todo: remove it
				if (ImGui::Selectable(listKey.c_str(), isSelected))
				{
					result.first = true;
					result.second = listValue;
				}
				if (onRightMouseClickPopUpRender && ImGui::BeginPopupContextItem())
				{
					if (onRightMouseClickPopUpRender(listValue))
						ImGui::CloseCurrentPopup();
					ImGui::EndPopup();
				}
				if (!result.first && isSelected)
					result.second = listValue;
			}

			ImGui::EndListBox();
		}
	}
#endif
	return result;
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