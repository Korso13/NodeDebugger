//Various utility functions for NodeDebugger components
#pragma once

#include "W4Framework.h"
#include "common/NodeDebugger/ND_Defines.h"
#include "common/NodeDebugger/ND_Utilities.h"

#ifndef FORCENODEDEBUG_OFF
#include <internal/ImGuiAutoCommon.h>

using namespace ImGui;

class ND_ContainerTools
{
public:

	ND_ContainerTools() = delete;

    template <typename TK, typename TV>
    static void renderMapPair(TK& key, TV& value)
    {
        std::string keyString = std::to_string(key);
        ImGui::PushID(keyString.c_str());
        ImGui::Auto(keyString.c_str(), value);
        ImGui::PopID();
    }

    template <typename TK>
    static void renderMapPair(TK& key, vec3& value)
    {
        std::string keyString = std::to_string(key);
        ImGui::PushID(keyString.c_str());
        if (ImGui::Button("Paste"))
        {
            value = NDS->m_lastCopiedVec3;
        }
        ImGui::SameLine();
        ImGui::Auto(keyString.c_str(), value);
        ImGui::PopID();
    }

    template <typename TK, typename TV>
    static bool ContainerView(const char* label, std::unordered_map<TK, TV>& value, const std::string filter = "")
    {
        static TK key;
        bool changed = false;
        if (ImGui::TreeNode(label))
        {
            if (ImGui::TreeNode("Edit"))
            {
                Auto("Key", key);
                if (ImGui::Button("Add key"))
                {
                    if (!value.contains(key))
                    {
                        value[key] = TV{};
                        changed = true;
                    }
                    else
                    {
                        W4_LOG_WARNING("Key already exist");
                    }
                }
                if (ImGui::Button("Erase key"))
                {
                    if (value.contains(key))
                    {
                        value.erase(key);
                        changed = true;
                    }
                    else
                    {
                        W4_LOG_WARNING("Key not found");
                    }
                }
                if (ImGui::Button("Clear all"))
                {
                    value.clear();
                    changed = true;
                }
                ImGui::TreePop();
            }

            for (auto& pair : value)
            {
                //filtering results
                if (!ND_Util::checkFilterConditions(pair.first, filter))
                    continue;

                renderMapPair(pair.first, pair.second);
            }
            ImGui::TreePop();
        }
        return changed;
    }

};

#endif