#include "VariablesTab.h"

#ifndef FORCENODEDEBUG_OFF

void VariablesTab::onGuiCall()
{
	if ((m_debugFloats.size() + m_debugInts.size() + m_debugVec2.size() + m_debugVec3.size() + m_debugBools.size()) > 6)
		ND_Util::ImGuiSearchBar(m_variablesFilter);
	else if (!m_variablesFilter.empty())
		m_variablesFilter.clear();

	if (ImGui::BeginChild("Variables scroll zone"))
	{	
		for (auto& [name_f, var_f] : m_debugFloats)
		{
			if (!ND_Util::checkFilterConditions(name_f, m_variablesFilter))
				continue;

			if (ImGui::InputFloat(name_f.c_str(), var_f.var))
				if (var_f.onChangeCb)var_f.onChangeCb();
		}
		for (auto& [name_i, var_i] : m_debugInts)
		{
			if (!ND_Util::checkFilterConditions(name_i, m_variablesFilter))
				continue; 
		
			if (ImGui::InputInt(name_i.c_str(), var_i.var))
				if (var_i.onChangeCb)var_i.onChangeCb();
		}
		for (auto& [name_v2, var_v2] : m_debugVec2)
		{
			if (!ND_Util::checkFilterConditions(name_v2, m_variablesFilter))
				continue;
		
			bool valueChanged = false;
			float vec2var[2] = { var_v2.var->x, var_v2.var->y };
			valueChanged = ImGui::InputFloat2(name_v2.c_str(), vec2var, "%.4f");
			if (valueChanged)
			{
				*var_v2.var = ND_Util::vec2FromGui(vec2var);
				if (var_v2.onChangeCb)var_v2.onChangeCb();
			}
		}
		for (auto& [name_v3, var_v3] : m_debugVec3)
		{
			if (!ND_Util::checkFilterConditions(name_v3, m_variablesFilter))
				continue; 
		
			bool valueChanged = false;
			float vec3var[3] = { var_v3.var->x, var_v3.var->y, var_v3.var->z };
			valueChanged = ImGui::InputFloat3(name_v3.c_str(), vec3var, "%.4f");
			if (valueChanged)
			{
				*var_v3.var = ND_Util::vec3FromGui(vec3var);
				if (var_v3.onChangeCb)var_v3.onChangeCb();
			}
		}
		for (auto& [name_v4, var_v4] : m_debugVec4)
		{
			if (!ND_Util::checkFilterConditions(name_v4, m_variablesFilter))
				continue; 
		
			bool valueChanged = false;
			if (ImGui::ColorEdit(name_v4.c_str(), *var_v4.var))
			{
				if (var_v4.onChangeCb)
					var_v4.onChangeCb();
			}
		}
		for (auto& [name_b, var_b] : m_debugBools)
		{
			if (!ND_Util::checkFilterConditions(name_b, m_variablesFilter))
				continue; 
		
			if (ImGui::Checkbox(name_b.c_str(), var_b.var))
				if (var_b.onChangeCb)var_b.onChangeCb();
		}
	}
	ImGui::EndChild(); //ends variables scroll zone
}

bool VariablesTab::isEmpty()
{
	return (m_debugFloats.empty() &&
			m_debugInts.empty() &&
			m_debugBools.empty() &&
			m_debugVec2.empty() &&
			m_debugVec3.empty());
}

void VariablesTab::purgeValues()
{
	m_debugFloats.clear();
	m_debugInts.clear();
	m_debugBools.clear();
	m_debugVec2.clear();
	m_debugVec3.clear();
}

#endif