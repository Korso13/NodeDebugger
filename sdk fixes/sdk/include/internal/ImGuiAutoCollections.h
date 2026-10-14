#pragma once

#include "ImGuiAutoCommon.h"

namespace ImGui
{
    template <typename T>
    bool Auto (const char* label, std::vector<T>& value)
    {
        bool changed = false;
        if (ImGui::TreeNode(label))
        {
            if (ImGui::Button("+"))
            {
                value.emplace_back();
                changed = true;
            }
            ImGui::SameLine();
            if (ImGui::Button("-!"))
            {
                value.clear();
                changed = true;
            }
            // TODO add/remove/change/sort elements
            for (int i = 0; i < value.size(); ++i)
            {
                ImGui::PushID(i);
                std::string keyTitle = std::to_string(i);
                changed = changed || ImGui::Auto(keyTitle.c_str(), value[i]);
                ImGui::PopID();
            }
            ImGui::TreePop();
        }
        return changed;
    }

    template <typename TK, typename TV>
    bool Auto (const char* label, std::map<TK, TV>& value)
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
            // TODO sort elements
            for (auto& pair : value)
            {
                std::string keyString = std::to_string(pair.first);
                ImGui::PushID(keyString.c_str());
                changed = changed || ImGui::Auto(keyString.c_str(), pair.second);
                ImGui::PopID();
            }
            ImGui::TreePop();
        }
        return changed;
    }

    template <typename TK, typename TV>
    bool Auto (const char* label, std::unordered_map<TK, TV>& value)
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
            // TODO sort elements
            for (auto& pair : value)
            {
                std::string keyString = std::to_string(pair.first);
                ImGui::PushID(keyString.c_str());
                changed = changed || ImGui::Auto(keyString.c_str(), pair.second);
                ImGui::PopID();
            }
            ImGui::TreePop();
        }
        return changed;
    }

    // TODO other collections

    // TODO map editor
    // bool mapOnGui(const char* label, std::map<TK, TV> collection)
    // {
    //     bool changed = false;
    //     if (ImGui::TreeNode(label))
    //     {
    //         TK elementToRemove;
    //         ImGuiWindowFlags window_flags = ImGuiWindowFlags_None;
    //         ImGui::BeginChild(label, ImVec2(0, 260), true, window_flags);
    //         if (ImGui::BeginTable("table", 1, ImGuiTableFlags_Resizable | ImGuiTableFlags_NoSavedSettings))
    //         {
    //             for (auto& item: collection)
    //             {
    //                 if (ImGui::Button("X"))
    //                 {
    //                     elementToRemove = item.first;
    //                 }
    //                 ImGui::SameLine();
    //                 changed = item.second.onGui(item.first);
    //                 ImGui::TableNextColumn();
    //             }
    //             ImGui::EndTable();
    //         }
    //         ImGui::EndChild();
    //
    //         if (!elementToRemove.empty())
    //         {
    //             collection.erase(elementToRemove);
    //             changed = true;
    //         }
    //     }
    //     return changed;
    // }
};
