#pragma once

#include "../ND_iGuiTab.h"
#include <regex>

class VariablesTab : public iGuiTab
{
public:

	VariablesTab();

	void onGuiCall() override NOCOMP;
	bool isEmpty() override NOCOMP_B;

	//Not usable in this case, use addItemTyped()
	void addItem(std::any item, const std::string& altName = "default") override {};
	
	template<typename T>
	void addItemTyped(const std::string& altName, T item, std::function<void()> cb) NOCOMP;

	template<typename Container, class VT = typename Container::mapped_type, class KT = typename Container::key_type>
	void addCollectionKeyTyped(const std::string& altName, Container& collection) NOCOMP;

	template<typename Container, class VT = typename Container::value_type>
	void addCollectionTyped(const std::string& altName, Container& collection) NOCOMP;

	void purgeValues() override NOCOMP;

private:

	void drawVectorCollection(const std::string& name, debugVectors& collection) NOCOMP;
	void drawMapCollection(const std::string& name, debugMapCollection& collection) NOCOMP;
	void drawSettingsSubtab() NOCOMP;
	//Logic for rendering GUI for debug variables
	void drawVariablesFolder(std::map<std::string, debugVariant>& variables) NOCOMP;

private:

	std::map<std::string, debugMapCollection> m_debugMaps;
	std::map<std::string, debugVectors> m_debugVectors;

	using variables_container = std::map<std::string, std::map<std::string, debugVariant>>;
	variables_container m_debugVariables;
	size_t m_variablesCounter = 0;

#ifdef ND_ACCESSENABLED
	std::unordered_map<std::string, bool>&			m_boolSettings;
	std::unordered_map<std::string, int>&			m_intSettings;
	std::unordered_map<std::string, float>&			m_floatSettings;
	std::unordered_map<std::string, std::string>&	m_stringSettings;
	std::unordered_map<std::string, vec2>&			m_vec2Settings;
	std::unordered_map<std::string, vec3>&			m_vec3Settings;
	std::unordered_map<std::string, vec4>&			m_vec4Settings;
#endif

	std::string m_variablesFilter = "";
	std::string m_settingsFilter = "";
};

	//Templated methods' impls:
#ifndef FORCENODEDEBUG_OFF

template<typename Container, class VT>
inline void VariablesTab::addCollectionTyped(const std::string& altName, Container& collection)
{
	if (m_debugVectors.contains(altName))
		return;

	debugVectors newDebugRec;
	newDebugRec.add(collection);

	m_debugVectors.insert({ altName, newDebugRec});
}

template<typename Container, class VT, class KT>
inline void VariablesTab::addCollectionKeyTyped(const std::string& altName, Container& collection)
{
	if (m_debugMaps.contains(altName))
		return;

	debugMaps<KT> newDebugRec;
	newDebugRec.add(collection);
	debugMapCollection maps(std::move(newDebugRec));

	m_debugMaps.insert({ altName, maps });
}

template<typename T>
void VariablesTab::addItemTyped(const std::string& altName, T item, std::function<void()> cb)
{
	debugVariant debugVar;
	std::string variableName, folderName = "none";
	variableName = altName; //default

	std::regex reg(R"(\[.+\])", std::regex::icase);
	std::smatch folder_found;
	if (std::regex_search(altName, folder_found, reg))
	{
		//warn("%s", folder_found.str().c_str());
		if (folder_found.str().size() > 2)
		{
			folderName = folder_found.str(); folderName.pop_back(); folderName.erase(0, 1);
			variableName = std::regex_replace(altName, reg, "");
		}
	}

	debugVar.variableName = variableName;
	debugVar.var = item;
	debugVar.onChangeCb = cb;
	m_debugVariables[folderName][variableName] = debugVar;
	m_variablesCounter++;
}

#endif