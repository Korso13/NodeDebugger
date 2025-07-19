#pragma once
#include "ND_Defines.h"
#include "ND_Utilities.h"
#include "spawner/SpawnerStructs.h"

//#ifndef FORCENODEDEBUG_OFF
//	#include "include/internal/ImGuiW4.h"
//#endif

namespace NDGui
{
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

	template<typename T>
	bool Auto(const char* label, T& value)
	{
#ifndef FORCENODEDEBUG_OFF
		return ImGui::Auto(label, value);
#else
		return false;
#endif
	}
}