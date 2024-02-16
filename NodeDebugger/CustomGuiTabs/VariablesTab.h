#pragma once

#include "../ND_iGuiTab.h"

class VariablesTab : public iGuiTab
{
public:

	VariablesTab() : iGuiTab("Variables") {};

	void onGuiCall() override NOCOMP;
	bool isEmpty() override NOCOMP_B;

	//Not working, use addItemTyped()
	void addItem(std::any item, const std::string& altName = "default") override {};
	
	template<typename T>
	void addItemTyped(const std::string& altName, T item, std::function<void()> cb) NOCOMP;

	void purgeValues() override NOCOMP;

private:

	std::unordered_map<std::string, debugVar<float>> m_debugFloats;
	std::unordered_map<std::string, debugVar<int>  > m_debugInts;
	std::unordered_map<std::string, debugVar<bool> > m_debugBools;
	std::unordered_map<std::string, debugVar<vec2> > m_debugVec2;
	std::unordered_map<std::string, debugVar<vec3> > m_debugVec3;
	std::unordered_map<std::string, debugVar<vec4> > m_debugVec4;

	std::string m_variablesFilter = "";
};


template<>
inline void VariablesTab::addItemTyped(const std::string& altName, float* item, std::function<void()> cb)
{
	debugVar<float> debugVar;
	debugVar.var = item;
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
inline void VariablesTab::addItemTyped(const std::string& altName, bool* item, std::function<void()> cb)
{
	debugVar<bool> debugVar;
	debugVar.var = item;
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
inline void VariablesTab::addItemTyped(const std::string& altName, vec3* item, std::function<void()> cb)
{
	debugVar<vec3> debugVar;
	debugVar.var = item;
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
