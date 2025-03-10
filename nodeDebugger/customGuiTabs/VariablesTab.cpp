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
			if (m_variablesCounter > 6)
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

				//variables in folders
				for (auto& [folder, variables] : m_debugVariables)
				{
					if (folder != "none")
					{
						if (ImGui::TreeNode(folder.c_str()))
						{
							drawVariablesFolder(variables);

							ImGui::TreePop();
						}
					}
				}
				//variables without folder
				drawVariablesFolder(m_debugVariables["none"]);
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

void VariablesTab::drawVariablesFolder(std::unordered_map<std::string, debugVariant>& variables)
{
	std::for_each(variables.begin(), variables.end(), 
		[this](const auto& v)
		{
			if (!ND_Util::checkFilterConditions(v.first, m_variablesFilter))
				return;

			const std::string& name = v.first;
			const std::function<void()>& cb = v.second.onChangeCb;
			std::visit(Visitor{
				[&name, &cb](bool* var) 
				{
					if (!var) return;
					if (ImGui::Checkbox(name.c_str(), var))
						if (cb) cb();
				},
				[&name, &cb](int* var) 
				{
					if (!var) return;
					if (ImGui::InputInt(name.c_str(), var))
						if (cb) cb();
				},
				[&name, &cb](float* var)
				{
					if (!var) return;
					if (ImGui::InputFloat(name.c_str(), var, 0.f, 0.f, IMG_FLOATFORMAT))
						if (cb) cb();
				},
				[&name, &cb](std::string* var)
				{
					if (!var) return;
					if (ImGui::InputText(name.c_str(), var))
						if (cb) cb();
				},
				[&name, &cb](vec2* var)
				{
					if (!var) return;
					bool valueChanged = false;
					float vec2var[2] = { var->x, var->y };
					valueChanged = ImGui::InputFloat2(name.c_str(), vec2var, IMG_VECFORMAT);
					if (valueChanged)
					{
						*var = ND_Util::vec2FromGui(vec2var);
						if (cb) cb();
					}
				},
				[&name, &cb](vec3* var)
				{
					if (!var) return;
					bool valueChanged = false;
					float vec3var[3] = { var->x, var->y, var->z };
					valueChanged = ImGui::InputFloat3(name.c_str(), vec3var, IMG_VECFORMAT);
					if (valueChanged)
					{
						*var = ND_Util::vec3FromGui(vec3var);
						if (cb) cb();
					}
				},
				[&name, &cb](vec4* var)
				{
					if (!var) return;
					if (ImGui::ColorEdit(name.c_str(), *var))
						if (cb) cb();
				},
				[&name, &cb](ivec2* var)
				{
					if (!var) return;
					//TODO: very shittiy AUTO impl, make your own!!
					if (ImGui::Auto(name.c_str(), var))
						if (cb) cb();
				}
			}, v.second.var);
		});
}

bool VariablesTab::isEmpty()
{
#ifdef ND_ACCESSENABLED
	return false;
#endif
	return m_debugVariables.empty() && m_debugMaps.empty() && m_debugVectors.empty();
}

void VariablesTab::purgeValues()
{
	m_debugVariables.clear();
	m_debugVectors.clear();
	m_debugMaps.clear();
	m_variablesCounter = 0;
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