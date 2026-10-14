#pragma once
#include "ND_Defines.h"
#include "ND_Utilities.h"

//#ifndef FORCENODEDEBUG_OFF
//	#include "include/internal/ImGuiW4.h"
//#endif

//struct BorderContainer;

namespace NDGui
{
	template<typename T>
	bool Auto(const char* label, T& value)
	{
#ifndef FORCENODEDEBUG_OFF
		return ImGui::Auto(label, value);
#else
		return false;
#endif
	}

	//forward decl of specialization (impl is in cpp)
	// template<>
	// bool Auto(const char* label, BorderContainer& value);

	template<typename T>
	requires serializedEnumConcept<T> && std::is_enum_v<T>
	inline bool Auto(const char* label, T& value)
	{
		bool changed = false;
#ifndef FORCENODEDEBUG_OFF
		auto valueRaw = static_cast<typename std::underlying_type<T>::type>(value);
		//ImGui::Text("Raw value: %s", std::to_string(valueRaw).c_str());
		int count;
		const auto* flagValues = nlohmann::adl_serializer<T>::getValues(count);
		//std::string uLabel = std::string(label) + std::string("##") + STR(w4::utils::UIDGenerator::get());
		if (ImGui::BeginMenu(label))
		{
			//ImGui::PushID(repeatElemId++);
			ImGui::Text("Current value: %s", std::to_string(valueRaw).c_str());
			// Flags enum
			if constexpr (requires (T a, T b) { a | b; })
			{
				for (int i = 0; i < count; ++i)
				{
					auto flagValue = flagValues[i];
					auto flagValueRaw = static_cast<typename std::underlying_type<T>::type>(flagValue.first);
					changed |= ImGui::CheckboxFlags(flagValue.second, &valueRaw, flagValueRaw);
				}
				if (changed)
				{
					value = static_cast<T>(valueRaw);
				}
			}
			// Plain enum
			else
			{
				for (int i = 0; i < count; ++i)
				{
					auto flagValue = flagValues[i];
					bool checked = flagValue.first == value;
					if (ImGui::Checkbox(flagValue.second, &checked))
					{
						if (checked)
						{
							value = flagValue.first;
						}
						changed = true;
					}
				}
			}

			//ImGui::PopID();
			ImGui::EndMenu();
		}


#endif
		return changed;
	}

	template<>
	inline bool Auto(const char* label, vec2& value)
	{
#ifndef DEBUG
		return false;
#else
		bool changed = false;
		float pos[2] = { value.x, value.y };
		changed = ImGui::DragFloat2(label, pos);
		if (changed)
			value = vec2(pos[0], pos[1]);
		return changed;
#endif
	}

	template<>
	inline bool Auto(const char* label, vec3& value)
	{
#ifndef DEBUG
		return false;
#else
		bool changed = false;
		float pos[3] = { value.x, value.y, value.z };
		changed = ImGui::DragFloat3(label, pos);
		if (changed)
			value = vec3(pos[0], pos[1], pos[2]);
		return changed;
#endif
	}

	template<>
	inline bool Auto(const char* label, vec4& value)
	{
#ifndef DEBUG
		return false;
#else
		bool changed = false;
		changed = ImGui::ColorEdit(label, value);
		return changed;
#endif
	}

	template<>
	inline bool Auto(const char* label, uvec2& value)
	{
#ifndef DEBUG
		return false;
#else
		bool changed = false;
		int pos[2] = { CastI(value.x), CastI(value.y) };
		changed = ImGui::DragInt2(label, pos, 1, 0, 10000);
		if (changed)
			value = uvec2(pos[0], pos[1]);
		return changed;
#endif
	}

	template<>
	inline bool Auto(const char* label, ivec2& value)
	{
#ifndef DEBUG
		return false;
#else
		bool changed = false;
		int pos[2] = { value.x,value.y };
		changed = ImGui::DragInt2(label, pos);
		if (changed)
			value = ivec2(pos[0], pos[1]);
		return changed;
#endif
	}

	template<>
	inline bool Auto(const char* label, sptr<MaterialInst>& value)
	{
#ifndef DEBUG
		return false;
#else
		bool changed = false;
		changed = ImGuiDrawerMaterial::onGui(label, *value);
		return changed;
#endif
	}
}