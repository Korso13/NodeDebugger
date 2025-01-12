#include "VariablesTab.h"

#ifndef FORCENODEDEBUG_OFF
#include "../ND_JsonTools.h"
#include <common/nodeDebugger/ND_ContainerTools.h>

//json serializers for settings export
namespace nlohmann
{
	template<>
	struct adl_serializer<std::unordered_map<std::string, bool>>
	{
		static void to_json(json& j, const std::unordered_map<std::string, bool> &obj)
		{
			for(auto& [key, value] : obj)
				j["bool"][key] = value;
		}
	};
	template<>
	struct adl_serializer<std::unordered_map<std::string, int>>
	{
		static void to_json(json& j, const std::unordered_map<std::string, int> &obj)
		{
			for (auto& [key, value] : obj)
				j["int"][key] = value;
		}
	};
	template<>
	struct adl_serializer<std::unordered_map<std::string, float>>
	{
		static void to_json(json& j, const std::unordered_map<std::string, float> &obj)
		{
			for (auto& [key, value] : obj)
				j["float"][key] = value;
		}
	};
	template<>
	struct adl_serializer<std::unordered_map<std::string, vec2>>
	{
		static void to_json(json& j, const std::unordered_map<std::string, vec2> &obj)
		{
			for (auto& [key, value] : obj)
				j["vector2"][key] = value;
		}
	};
	template<>
	struct adl_serializer<std::unordered_map<std::string, vec3>>
	{
		static void to_json(json& j, const std::unordered_map<std::string, vec3> &obj)
		{
			for (auto& [key, value] : obj)
				j["vector3"][key] = value;
		}
	};
	template<>
	struct adl_serializer<std::unordered_map<std::string, vec4>>
	{
		static void to_json(json& j, const std::unordered_map<std::string, vec4> &obj)
		{
			for (auto& [key, value] : obj)
				j["vector4"][key] = value;
		}
	};
	template<>
	struct adl_serializer<std::unordered_map<std::string, std::string>>
	{
		static void to_json(json& j, const std::unordered_map<std::string, std::string>& obj)
		{
			for (auto& [key, value] : obj)
				j["string"][key] = value;
		}
	};
}

VariablesTab::VariablesTab()
	: 
#ifdef ND_ACCESSENABLED
	m_boolSettings(SETTINGS.getContainerRef<bool>()),
	m_intSettings(SETTINGS.getContainerRef<int>()),
	m_floatSettings(SETTINGS.getContainerRef<float>()),
	m_stringSettings(SETTINGS.getContainerRef<std::string>()),
	m_vec2Settings(SETTINGS.getContainerRef<vec2>()),
	m_vec3Settings(SETTINGS.getContainerRef<vec3>()),
	m_vec4Settings(SETTINGS.getContainerRef<vec4>()),
#endif
	iGuiTab("Variables")
{

}

void VariablesTab::onGuiCall()
{
	if (ImGui::BeginTabBar("NewTabBar"))
	{
		if (ImGui::BeginTabItem("Variables"))
		{
			if ((m_debugFloats.size() + m_debugInts.size() + m_debugVec2.size() + m_debugVec3.size() + m_debugBools.size()) > 6)
				ND_Util::ImGuiSearchBar(m_variablesFilter);
			else if (!m_variablesFilter.empty())
				m_variablesFilter.clear();

			if (ImGui::BeginChild("Variables scroll zone"))
			{
				//collections
				for (auto& [name_vectCol, vect] : m_debugVectors)
				{
					drawVectorCollection(name_vectCol, vect);
				}
				for (auto& [name_mapCol, map] : m_debugMaps)
				{
					drawMapCollection(name_mapCol, map);
				}

				for (auto& [name_f, var_f] : m_debugFloats)
				{
					if (!ND_Util::checkFilterConditions(name_f, m_variablesFilter))
						continue;

					if (ImGui::InputFloat(name_f.c_str(), var_f.var, 0.f, 0.f, IMG_FLOATFORMAT))
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
					valueChanged = ImGui::InputFloat2(name_v2.c_str(), vec2var, IMG_VECFORMAT);
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
					valueChanged = ImGui::InputFloat3(name_v3.c_str(), vec3var, IMG_VECFORMAT);
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
				for (auto& [name_iv2, var_iv2] : m_debugIVec2)
				{
					if (!ND_Util::checkFilterConditions(name_iv2, m_variablesFilter))
						continue;

					bool valueChanged = false;
					//TODO: very shittiy AUTO impl, make your own!!
					if (ImGui::Auto(name_iv2.c_str(), *var_iv2.var))
					{
						if (var_iv2.onChangeCb)
							var_iv2.onChangeCb();
					}
				}
				for (auto& [name_b, var_b] : m_debugStrings)
				{
					if (!ND_Util::checkFilterConditions(name_b, m_variablesFilter))
						continue;

					if (ImGui::InputText(name_b.c_str(), var_b.var))
						if (var_b.onChangeCb) var_b.onChangeCb();
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

			ImGui::EndTabItem(); //end of variables tab
		}

		if (ImGui::BeginTabItem("Settings"))
		{
			drawSettingsSubtab();

			ImGui::EndTabItem(); //end of Settings tab
		}

	}
	ImGui::EndTabBar();
}

bool VariablesTab::isEmpty()
{
	return (m_debugFloats.empty() &&
			m_debugInts.empty() &&
			m_debugBools.empty() &&
			m_debugVec2.empty() &&
			m_debugVec3.empty()&&
			m_debugStrings.empty()&&
			m_debugVectors.empty()&&
			m_debugMaps.empty());
}

void VariablesTab::purgeValues()
{
	m_debugFloats.clear();
	m_debugInts.clear();
	m_debugBools.clear();
	m_debugVec2.clear();
	m_debugVec3.clear();
	m_debugStrings.clear();
	m_debugVectors.clear();
	m_debugMaps.clear();
}

void VariablesTab::drawVectorCollection(const std::string& name, debugVectors& collection)
{
	//if (!collection.bools->empty())
	//	ImGui::Auto(name.c_str(), *collection.bools);  //for some reason - not supported
	if (collection.ints)
		ImGui::Auto(name.c_str(), *collection.ints);
	if (collection.floats)
		ImGui::Auto(name.c_str(), *collection.floats);
	if (collection.strings)
		ImGui::Auto(name.c_str(), *collection.strings);
	if (collection.vec2s)
		ImGui::Auto(name.c_str(), *collection.vec2s);
	if (collection.vec3s)
		ImGui::Auto(name.c_str(), *collection.vec3s);
	if (collection.vec4s)
		ImGui::Auto(name.c_str(), *collection.vec4s);
}

void VariablesTab::drawMapCollection(const std::string& name, debugMapCollection& collection)
{
	if (collection.int_maps.bools )
		ImGui::Auto(name.c_str(), *collection.int_maps.bools);
	if (collection.int_maps.ints )
		ImGui::Auto(name.c_str(), *collection.int_maps.ints);
	if (collection.int_maps.floats )
		ImGui::Auto(name.c_str(), *collection.int_maps.floats);
	if (collection.int_maps.strings )
		ImGui::Auto(name.c_str(), *collection.int_maps.strings);
	if (collection.int_maps.vec2s )
		ImGui::Auto(name.c_str(), *collection.int_maps.vec2s);
	if (collection.int_maps.vec3s )
		ImGui::Auto(name.c_str(), *collection.int_maps.vec3s);
	if (collection.int_maps.vec4s )
		ImGui::Auto(name.c_str(), *collection.int_maps.vec4s);

	if (collection.int_maps.um_bools )
		ImGui::Auto(name.c_str(), *collection.int_maps.um_bools);
	if (collection.int_maps.um_ints )
		ImGui::Auto(name.c_str(), *collection.int_maps.um_ints);
	if (collection.int_maps.um_floats )
		ImGui::Auto(name.c_str(), *collection.int_maps.um_floats);
	if (collection.int_maps.um_strings )
		ImGui::Auto(name.c_str(), *collection.int_maps.um_strings);
	if (collection.int_maps.um_vec2s )
		ImGui::Auto(name.c_str(), *collection.int_maps.um_vec2s);
	if (collection.int_maps.um_vec3s )
		ImGui::Auto(name.c_str(), *collection.int_maps.um_vec3s);
	if (collection.int_maps.um_vec4s )
		ImGui::Auto(name.c_str(), *collection.int_maps.um_vec4s);

	if (collection.str_maps.bools )
		ImGui::Auto(name.c_str(), *collection.str_maps.bools);
	if (collection.str_maps.ints )
		ImGui::Auto(name.c_str(), *collection.str_maps.ints);
	if (collection.str_maps.floats )
		ImGui::Auto(name.c_str(), *collection.str_maps.floats);
	if (collection.str_maps.strings )
		ImGui::Auto(name.c_str(), *collection.str_maps.strings);
	if (collection.str_maps.vec2s )
		ImGui::Auto(name.c_str(), *collection.str_maps.vec2s);
	if (collection.str_maps.vec3s )
		ImGui::Auto(name.c_str(), *collection.str_maps.vec3s);
	if (collection.str_maps.vec4s )
		ImGui::Auto(name.c_str(), *collection.str_maps.vec4s);

	if (collection.str_maps.um_bools)
		ImGui::Auto(name.c_str(), *collection.str_maps.um_bools);
	if (collection.str_maps.um_ints)
		ImGui::Auto(name.c_str(), *collection.str_maps.um_ints);
	if (collection.str_maps.um_floats)
		ImGui::Auto(name.c_str(), *collection.str_maps.um_floats);
	if (collection.str_maps.um_strings)
		ImGui::Auto(name.c_str(), *collection.str_maps.um_strings);
	if (collection.str_maps.um_vec2s)
		ImGui::Auto(name.c_str(), *collection.str_maps.um_vec2s);
	if (collection.str_maps.um_vec3s)
		ImGui::Auto(name.c_str(), *collection.str_maps.um_vec3s);
	if (collection.str_maps.um_vec4s)
		ImGui::Auto(name.c_str(), *collection.str_maps.um_vec4s);
}

void VariablesTab::drawSettingsSubtab()
{
	if (ImGui::BeginChild("Settings scroll zone"))
	{
#ifdef ND_ACCESSENABLED
		//Settings export button
		if (ImGui::Button("Export settings"))
		{
			nlohmann::json exportJson;
			nlohmann::adl_serializer<std::unordered_map<std::string, bool>>::to_json(exportJson, m_boolSettings);
			nlohmann::adl_serializer<std::unordered_map<std::string, int>>::to_json(exportJson, m_intSettings);
			nlohmann::adl_serializer<std::unordered_map<std::string, float>>::to_json(exportJson, m_floatSettings);
			nlohmann::adl_serializer<std::unordered_map<std::string, vec2>>::to_json(exportJson, m_vec2Settings);
			nlohmann::adl_serializer<std::unordered_map<std::string, vec3>>::to_json(exportJson, m_vec3Settings);
			nlohmann::adl_serializer<std::unordered_map<std::string, vec4>>::to_json(exportJson, m_vec4Settings);
			nlohmann::adl_serializer<std::unordered_map<std::string, std::string>>::to_json(exportJson, m_stringSettings);

			std::string flatJson = jsonUnflatten(exportJson);
			jsWriteEvent(flatJson.c_str(), "settings.json");
		}

		//settings filter
		ND_Util::ImGuiSearchBar(m_settingsFilter);

		//Settings list renderers
		ND_ContainerTools::ContainerView("Bools", m_boolSettings, m_settingsFilter);
		ND_ContainerTools::ContainerView("Ints", m_intSettings, m_settingsFilter);
		ND_ContainerTools::ContainerView("Floats", m_floatSettings, m_settingsFilter);
		ND_ContainerTools::ContainerView("vec2", m_vec2Settings, m_settingsFilter);
		ND_ContainerTools::ContainerView("vec3", m_vec3Settings, m_settingsFilter);
		ND_ContainerTools::ContainerView("vec4", m_vec4Settings, m_settingsFilter);
		ND_ContainerTools::ContainerView("Strings", m_stringSettings, m_settingsFilter);
#else
		ImGui::Text("Settings import/export Not enabled!\n Update Settings and SettingsContainer.h!");
#endif
	}
	ImGui::EndChild(); //ends Settings scroll zone
}

#endif