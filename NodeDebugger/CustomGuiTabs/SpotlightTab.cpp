#include "SpotlightTab.h"

#ifndef FORCENODEDEBUG_OFF

void SpotlightTab::onGuiCall()
{
    if (m_debugSpotLights.empty()) return;

	ND_Util::ImGuiNodeSettings();

    for (auto& debugSpotLight : m_debugSpotLights)
    {
        if (ImGui::TreeNode(debugSpotLight.name.c_str()))
        {
            //Drag gizmo
            ImGuizmo::BeginFrame();
            ImGuiDrawerGizmo::drawGizmosHandles(debugSpotLight.light);

            //visibility button
            if (ImGui::Button("Toggle visibility"))
            {
                debugSpotLight.light->setEnabled(!debugSpotLight.light->isEnabled());
            }

            ND_Util::ImGuiTransforms(debugSpotLight.light);

            //printing info to console
            ImGui::Separator();

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // color
            bool colorChanged = false;
            float color[3] = { debugSpotLight.color.x, debugSpotLight.color.y, debugSpotLight.color.z };
            ND_Util::ImGuiCopyVec3Btn(ND_Util::vec3FromGui(color));
            colorChanged = ImGui::DragFloat3("Color", color, sliderSensivity, -IMG_POSRANGE, IMG_POSRANGE);
            if (colorChanged)
            {
                debugSpotLight.color = ND_Util::vec3FromGui(color);
                debugSpotLight.light->setColor(ND_Util::vec3FromGui(color));
            }

            // angle
            bool angleChanged = false;
            float* angle = &debugSpotLight.angle;
            angleChanged = ImGui::DragFloat("Angle", angle, sliderSensivity, 0, 10, "%.2f");
            if (angleChanged) debugSpotLight.light->setAngle(*angle);
            ImGui::Separator();

            // intensity
            bool intensityChanged = false;
            float* intensity = &debugSpotLight.intensity;
            intensityChanged = ImGui::DragFloat("Intencity", intensity, sliderSensivity, 0, 10, "%.2f");
            if (intensityChanged) debugSpotLight.light->setIntensity(*intensity);
            ImGui::Separator();

            // decayFactor
            bool decayFactorChanged = false;
            float* decayFactor = &debugSpotLight.decayFactor;
            decayFactorChanged = ImGui::DragFloat("Decay factor", decayFactor, sliderSensivity, 0, 1, "%.2f");
            if (decayFactorChanged) debugSpotLight.light->setDecayFactor(*decayFactor);
            ImGui::Separator();

            // decayRate
            //if (ImGui::Auto("Decay rate", &debugSpotLight.decayRate))
            //{
            //    debugSpotLight.light->setDecayRate(debugSpotLight.decayRate);
            //}
            //ImGui::Separator();

            int ratesCount;
            const auto* rates = nlohmann::adl_serializer<LightDecayRate>::getValues(ratesCount);
            const char* current;
            for (int i = 0; i < ratesCount; i++)
            {
                if (rates[i].first == debugSpotLight.decayRate)
                {
                    current = rates[i].second;
                    break;
                }
            }
            if (ImGui::BeginCombo("combo decay rate", current))
            {
                for (int n = 0; n < ratesCount; n++)
                {
                    auto rate = rates[n];
                    const bool is_selected = (rate.first == debugSpotLight.decayRate);
                    if (ImGui::Selectable(rates[n].second, is_selected))
                    {
                        debugSpotLight.decayRate = rates[n].first;
                        debugSpotLight.light->setDecayRate(debugSpotLight.decayRate);
                    }

                    // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                    if (is_selected) ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
            ImGui::Separator();

            ImGui::TreePop();
            ImGui::Separator();
        }
    }
}

void SpotlightTab::addItem(std::any item, const std::string& altName)
{
    auto light = any_cast<sptr<SpotLight>>(item);
    if (!light) return;

    SpotLightDebugging debugLightRec;
    debugLightRec.name = altName == "default" ? light->getName() : altName;
    debugLightRec.light = light;
    debugLightRec.color = light->getColor();
    debugLightRec.angle = light->getAngle();
    debugLightRec.intensity = light->getIntensity();
    debugLightRec.decayFactor = light->getDecayFactor();
    debugLightRec.decayRate = light->getDecayRate();

    m_debugSpotLights.push_back(debugLightRec);
}

#endif