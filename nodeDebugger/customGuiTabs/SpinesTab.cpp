#include "SpinesTab.h"

#ifndef FORCENODEDEBUG_OFF

void SpinesTab::onGuiCall()
{
    if (m_debugSpines.empty())
        return;

	ND_Util::ImGuiNodeSettings();

    if (m_debugSpines.size() > 6)
        ND_Util::ImGuiSearchBar(m_spinesFilter);
    else if (!m_spinesFilter.empty())
        m_spinesFilter.clear();

    if (ImGui::BeginChild("Filtered Spines"))
    {
        for (auto& [name, debugSpine] : m_debugSpines)
        {
            if (!ND_Util::checkFilterConditions(debugSpine.spineName, m_spinesFilter))
                continue;
            if (debugSpine.spine.expired())
                continue;

            if (ImGui::TreeNode(name.c_str()))
            {
                renderSpineProperties(debugSpine, debugSpine.spineName);

                ImGui::Separator();
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::TreePop();
            }
        }
    }
    ImGui::EndChild(); //ends spine list scrollable zone
}

void SpinesTab::onGuiAnimations(SpineDebugging& debugSpine)
{
    if (debugSpine.spine.expired())
        return;

    auto spine = debugSpine.spine.lock();

    //animation setter
    const char* items[debugSpine.animationNames.size()];
    for (size_t i = 0; i < debugSpine.animationNames.size(); i++)
    {
        items[i] = debugSpine.animationNames[i].c_str();
    }
    ImGui::ListBox("Animations", &debugSpine.lastSelected, items, debugSpine.animationNames.size(), 8);
    ImGui::Text("Animation time: %f", debugSpine.animationLength[debugSpine.lastSelected]);
    ImGui::Columns(3);
    if (ImGui::Button("Play"))
    {
        spine->state->setAnimation(0, debugSpine.animationNames[debugSpine.lastSelected].c_str(), false);
        if (!m_copyAnimNameNotLength)
            ImGui::SetClipboardText(CSTR(debugSpine.animationLength[debugSpine.lastSelected]));
        else
            ImGui::SetClipboardText(CSTR(debugSpine.animationNames[debugSpine.lastSelected]));
    }
    ImGui::NextColumn();
    if (ImGui::Button("Play looped"))
    {
        spine->state->setAnimation(0, debugSpine.animationNames[debugSpine.lastSelected].c_str(), true);
        if (!m_copyAnimNameNotLength)
            ImGui::SetClipboardText(CSTR(debugSpine.animationLength[debugSpine.lastSelected]));
        else
            ImGui::SetClipboardText(CSTR(debugSpine.animationNames[debugSpine.lastSelected]));
    }
    ImGui::NextColumn();
    if (ImGui::Button("Reset"))
    {
        if(std::find_if(debugSpine.animationNames.begin(), debugSpine.animationNames.end(), [this](std::string name) {return name == "idle"; }) != debugSpine.animationNames.end())
            spine->state->setAnimation(0, "idle", true);
        else
            spine->state->setAnimation(0, debugSpine.animationNames[0].c_str(), true);
    }
    ImGui::Columns(1);
}

void SpinesTab::onGuiSkinSetter(SpineDebugging& debugSpine)
{
    if (debugSpine.spine.expired())
        return;

    auto spine = debugSpine.spine.lock();
    const char* skin_names[debugSpine.skinNames.size()];
    for (size_t i = 0; i < debugSpine.skinNames.size(); i++)
    {
        skin_names[i] = debugSpine.skinNames[i].c_str();
    }

    if (ImGui::Combo("Skin 1", &debugSpine.selectedSkin1, skin_names, debugSpine.skinNames.size(), debugSpine.skinNames.size()))
    {
        ImGui::SetClipboardText(CSTR(debugSpine.skinNames[debugSpine.selectedSkin1]));
    }
    if (ImGui::Combo("Skin 2", &debugSpine.selectedSkin2, skin_names, debugSpine.skinNames.size(), debugSpine.skinNames.size()))
    {
        ImGui::SetClipboardText(CSTR(debugSpine.skinNames[debugSpine.selectedSkin2]));
    }
    if (ImGui::Combo("Skin 3", &debugSpine.selectedSkin3, skin_names, debugSpine.skinNames.size(), debugSpine.skinNames.size()))
    {
        ImGui::SetClipboardText(CSTR(debugSpine.skinNames[debugSpine.selectedSkin3]));
    }
    if (ImGui::Combo("Skin 4", &debugSpine.selectedSkin4, skin_names, debugSpine.skinNames.size(), debugSpine.skinNames.size()))
    {
        ImGui::SetClipboardText(CSTR(debugSpine.skinNames[debugSpine.selectedSkin4]));
    }
    if (ImGui::Combo("Skin 5", &debugSpine.selectedSkin5, skin_names, debugSpine.skinNames.size(), debugSpine.skinNames.size()))
    {
        ImGui::SetClipboardText(CSTR(debugSpine.skinNames[debugSpine.selectedSkin5]));
    }
    if (ImGui::Combo("Skin 6", &debugSpine.selectedSkin6, skin_names, debugSpine.skinNames.size(), debugSpine.skinNames.size()))
    {
        ImGui::SetClipboardText(CSTR(debugSpine.skinNames[debugSpine.selectedSkin6]));
    }
    if (ImGui::Button("Apply skins"))
    {
        bool mainSkinSet = false;
        if (debugSpine.selectedSkin1 != 0)
        {
            if (!mainSkinSet)
            {
                mainSkinSet = true;
                ND_Util::setSkin(spine, debugSpine.skinNames[debugSpine.selectedSkin1]);
            }
            else
            {
                ND_Util::addSkin(spine, debugSpine.skinNames[debugSpine.selectedSkin1]);
            }
        }
        if (debugSpine.selectedSkin2 != 0)
        {
            if (!mainSkinSet)
            {
                mainSkinSet = true;
                ND_Util::setSkin(spine, debugSpine.skinNames[debugSpine.selectedSkin2]);
            }
            else
            {
                ND_Util::addSkin(spine, debugSpine.skinNames[debugSpine.selectedSkin2]);
            }
        }
        if (debugSpine.selectedSkin3 != 0)
        {
            if (!mainSkinSet)
            {
                mainSkinSet = true;
                ND_Util::setSkin(spine, debugSpine.skinNames[debugSpine.selectedSkin3]);
            }
            else
            {
                ND_Util::addSkin(spine, debugSpine.skinNames[debugSpine.selectedSkin3]);
            }
        }
        if (debugSpine.selectedSkin4 != 0)
        {
            if (!mainSkinSet)
            {
                mainSkinSet = true;
                ND_Util::setSkin(spine, debugSpine.skinNames[debugSpine.selectedSkin4]);
            }
            else
            {
                ND_Util::addSkin(spine, debugSpine.skinNames[debugSpine.selectedSkin4]);
            }
        }
    }
}

void SpinesTab::addItem(std::any item, const std::string& altName)
{
    auto spineToDebug = any_cast<sptr<Spine>>(item);
    
    if (!spineToDebug)
        return;

    SpineDebugging spineDebugRec;
    spineDebugRec.spine = spineToDebug;
    if (altName == "default")
        spineDebugRec.spineName = spineToDebug->getName();
    else
        spineDebugRec.spineName = altName;


    if (m_debugSpines.contains(spineDebugRec.spineName))
    {
        //checks if the list contains this spine already
        if (m_debugSpines[spineDebugRec.spineName].spine.lock() == spineToDebug)
            return;

        //prevents repeating names
        std::string zeroes = "00"; //for better sorting
        for (size_t i = 1; i < 500; i++)
        {
            if (i == 10 || i == 100)
                zeroes.pop_back();

            if (!m_debugSpines.contains(spineDebugRec.spineName + "(" + zeroes + STR(i) + ")"))
            {
                spineDebugRec.spineName += "(" + zeroes + STR(i) + ")";
                break;
            }
        }
    }

    std::vector<std::string> animationNames;
    std::vector<float> animationLength;

    auto animations = spineToDebug->state->getData()->getSkeletonData()->getAnimations();
    int animCount = animations.size();
    for (size_t i = 0; i < animCount; i++)
    {
        animationNames.push_back(animations[i]->getName().buffer());
        animationLength.push_back(animations[i]->getDuration());
    }
    spineDebugRec.animationNames = animationNames;
    spineDebugRec.animationLength = animationLength;

    auto skins = spineToDebug->state->getData()->getSkeletonData()->getSkins();
    for (size_t i = 0; i < skins.size(); i++)
    {
        spineDebugRec.skinNames.push_back(skins[i]->getName().buffer());
    }

    auto spine = spineDebugRec.spine.lock();
    for (size_t i = 0; i < spine->skeleton->getSlots().size(); i++)
    {
        std::string slotName = spine->skeleton->getSlots()[i]->getData().getName().buffer();
        int slotIndex = spine->skeleton->getSlots()[i]->getData().getIndex();
        spineDebugRec.slots.insert({ slotIndex , slotName });
    }

    m_debugSpines.insert({ spineDebugRec.spineName, spineDebugRec });
}

void SpinesTab::removeItem(const std::string& itemName)
{
    if (m_debugSpines.contains(itemName))
    {
        m_debugSpines.erase(itemName);
    }
}

void SpinesTab::renderSpineProperties(SpineDebugging& debugSpine, const std::string& name, bool noTransformTabs)
{
    auto spine = debugSpine.spine.lock();
    //Drag gizmo
    ND_Util::NoConflictGizmo(debugSpine.spine.lock(), name);

    if (ImGui::BeginTabBar("Spine tab bar"))
    {
        if (ImGui::BeginTabItem("Animations"))
        {
            ImGui::Checkbox("Copy animation name(not length) on Play", &m_copyAnimNameNotLength);
            ImGui::Separator();
            onGuiAnimations(debugSpine);

            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Skins/Visuals"))
        {
            bool alphaOn = spine->getUsePremultipliedAlpha();
            if (ImGui::Checkbox("Use premult. alpha", &alphaOn))
            {
                spine->setUsePremultipliedAlpha(alphaOn);
            }

            bool turnRight = spine->skeleton->getScaleX() > 0.f;
            if (ImGui::Checkbox("Turn spine to the right", &turnRight))
            {
                spine->skeleton->setScaleX(spine->skeleton->getScaleX() * -1.f);
            }

            //visibility button
            bool isVisible = spine->isEnabled();
            if (ImGui::Checkbox("Is Visible", &isVisible))
            {
                spine->setEnabled(isVisible);
            }

            int spineRO = spine->getRenderOrder();
            bool roChanged = false;
            roChanged = ImGui::InputInt("Render order", &spineRO, 1, 10);
            if (roChanged)
                spine->setRenderOrder(spineRO);

            //skin setter
            onGuiSkinSetter(debugSpine);

            ImGui::EndTabItem();
        }

        if (!noTransformTabs)
        {
            if (ImGui::BeginTabItem("Transforms"))
            {
                ND_Util::ImGuiTransforms(spine);
                //printing info to console
                ImGui::Separator();
                if (ImGui::Button("Print transform info"))
                {
                    ND_Util::printDebugNodeInfo(spine);
                }

                ImGui::EndTabItem();
            }
        }

        if (ImGui::BeginTabItem("Materials"))
        {
            if (!debugSpine.slots.empty())
            {
                bool hasSlots = false;
                for (auto& [slotId, slotName] : debugSpine.slots)
                {
                    if (spine->getSlotMaterial(slotId))
                    {
                        ImGuiDrawerMaterial::onGui(slotName.c_str(), *spine->getSlotMaterial(slotId));
                        hasSlots = true;
                    }
                }

                if (!hasSlots)
                    ImGui::Text("%s", "No slots with editable materials!");
            }

            ImGui::EndTabItem();
        }

        if (!noTransformTabs)
        {
            if (ImGui::BeginTabItem("Tween Builder"))
            {
                ND_Util::ImGuiTweens(spine);

                ImGui::EndTabItem();
            }
        }
    }
    ImGui::EndTabBar();
}

bool SpinesTab::hasSpineRecord(const std::string& spineRecordName) const
{
    return m_debugSpines.contains(spineRecordName);
}

SpinesTab::SpineDebugging& SpinesTab::getSpineInfo(const std::string& spineRecordName)
{
    if (hasSpineRecord(spineRecordName))
        return m_debugSpines[spineRecordName];

    static SpineDebugging nullSpine;
    return nullSpine;
}

SpinesTab::SpineDebugging& SpinesTab::getSpineInfo(sptr<Spine>& spine)
{
    size_t spineHash = std::hash<sptr<Spine>>{}(spine);
    if (m_cachedSpineInfo.contains(spineHash))
        return m_cachedSpineInfo[spineHash];

    //gathering info about spine
    std::vector<std::string> animationNames;
    std::vector<float> animationLength;
    auto animations = spine->state->getData()->getSkeletonData()->getAnimations();
    int animCount = animations.size();
    for (size_t i = 0; i < animCount; i++)
    {
        animationNames.push_back(animations[i]->getName().buffer());
        animationLength.push_back(animations[i]->getDuration());
    }
    auto skins = spine->state->getData()->getSkeletonData()->getSkins();
    std::vector<std::string> skinNames;
    for (size_t i = 0; i < skins.size(); i++)
    {
        skinNames.push_back(skins[i]->getName().buffer());
    }
    std::map<int, std::string> slots;
    for (size_t i = 0; i < spine->skeleton->getSlots().size(); i++)
    {
        std::string slotName = spine->skeleton->getSlots()[i]->getData().getName().buffer();
        int slotIndex = spine->skeleton->getSlots()[i]->getData().getIndex();
        slots.insert({ slotIndex , slotName });
    }

    //adding gathered info
    m_cachedSpineInfo[spineHash] = SpineDebugging{
        .spineName = spine->getName(),
        .spine = spine,
        .animationNames = std::move(animationNames),
        .animationLength = std::move(animationLength),
        .skinNames = std::move(skinNames),
        .slots = std::move(slots)
    };

    return m_cachedSpineInfo[spineHash];
}

#endif
