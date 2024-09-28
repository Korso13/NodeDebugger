#pragma once

#include "../ND_iGuiTab.h"

class VariablesTab : public iGuiTab
{
public:

	VariablesTab();

	void onGuiCall() override NOCOMP;
	bool isEmpty() override NOCOMP_B;

	//Not working, use addItemTyped()
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

private:

	std::unordered_map<std::string, debugMapCollection> m_debugMaps;
	std::unordered_map<std::string, debugVectors> m_debugVectors;

	std::unordered_map<std::string, debugVar<std::string>> m_debugStrings;
	std::unordered_map<std::string, debugVar<float>> m_debugFloats;
	std::unordered_map<std::string, debugVar<int>  > m_debugInts;
	std::unordered_map<std::string, debugVar<bool> > m_debugBools;
	std::unordered_map<std::string, debugVar<vec2> > m_debugVec2;
	std::unordered_map<std::string, debugVar<vec3> > m_debugVec3;
	std::unordered_map<std::string, debugVar<vec4> > m_debugVec4;

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

template<>
inline void VariablesTab::addItemTyped(const std::string& altName, std::string* item, std::function<void()> cb)
{
	debugVar<std::string> debugVar;
	debugVar.var = item;
	debugVar.onChangeCb = cb;
	m_debugStrings.insert({ altName, debugVar });
}
template<>
inline void VariablesTab::addItemTyped(const std::string& altName, float* item, std::function<void()> cb)
{
	debugVar<float> debugVar;
	debugVar.var = item;
	debugVar.onChangeCb = cb;
	m_debugFloats.insert({ altName, debugVar });
}
template<>
inline void VariablesTab::addItemTyped(const std::string& altName, const float* item, std::function<void()> cb)
{
	debugVar<float> debugVar;
	debugVar.var = const_cast<float*>(item);
	debugVar.onChangeCb = cb;
	m_debugFloats.insert({ altName, debugVar });
}
template<>
inline void VariablesTab::addItemTyped(const std::string& altName, int* item, std::function<void()> cb)
{
	debugVar<int> debugVar;
	debugVar.var = item;
	debugVar.onChangeCb = cb;
	m_debugInts.insert({ altName, debugVar });
}
template<>
inline void VariablesTab::addItemTyped(const std::string& altName, const int* item, std::function<void()> cb)
{
	debugVar<int> debugVar;
	debugVar.var = const_cast<int*>(item);
	debugVar.onChangeCb = cb;
	m_debugInts.insert({ altName, debugVar });
}
template<>
inline void VariablesTab::addItemTyped(const std::string& altName, bool* item, std::function<void()> cb)
{
	debugVar<bool> debugVar;
	debugVar.var = item;
	debugVar.onChangeCb = cb;
	m_debugBools.insert({ altName, debugVar });
}
template<>
inline void VariablesTab::addItemTyped(const std::string& altName, const bool* item, std::function<void()> cb)
{
	debugVar<bool> debugVar;
	debugVar.var = const_cast<bool*>(item);
	debugVar.onChangeCb = cb;
	m_debugBools.insert({ altName, debugVar });
}
template<>
inline void VariablesTab::addItemTyped(const std::string& altName, vec2* item, std::function<void()> cb)
{
	debugVar<vec2> debugVar;
	debugVar.var = item;
	debugVar.onChangeCb = cb;
	m_debugVec2.insert({ altName, debugVar });
}
template<>
inline void VariablesTab::addItemTyped(const std::string& altName, const vec2* item, std::function<void()> cb)
{
	debugVar<vec2> debugVar;
	debugVar.var = const_cast<vec2*>(item);
	debugVar.onChangeCb = cb;
	m_debugVec2.insert({ altName, debugVar });
}
template<>
inline void VariablesTab::addItemTyped(const std::string& altName, vec3* item, std::function<void()> cb)
{
	debugVar<vec3> debugVar;
	debugVar.var = item;
	debugVar.onChangeCb = cb;
	m_debugVec3.insert({ altName, debugVar });
}
template<>
inline void VariablesTab::addItemTyped(const std::string& altName, const vec3* item, std::function<void()> cb)
{
	debugVar<vec3> debugVar;
	debugVar.var = const_cast<vec3*>(item);
	debugVar.onChangeCb = cb;
	m_debugVec3.insert({ altName, debugVar });
}
template<>
inline void VariablesTab::addItemTyped(const std::string& altName, vec4* item, std::function<void()> cb)
{
	debugVar<vec4> debugVar;
	debugVar.var = item;
	debugVar.onChangeCb = cb;
	m_debugVec4.insert({ altName, debugVar });
}

template<>
inline void VariablesTab::addItemTyped(const std::string& altName, const vec4* item, std::function<void()> cb)
{
	debugVar<vec4> debugVar;
	debugVar.var = const_cast<vec4*>(item);
	debugVar.onChangeCb = cb;
	m_debugVec4.insert({ altName, debugVar });
}

#endif