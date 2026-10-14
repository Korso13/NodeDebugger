//
// Created by Timur Korsakov on 14.11.2025.
//
#include "SpawnerTools.h"
#include "Spawnable.h"
#include "SpawnerFactory.h"
#include "SpawnerStructs.h"
#include "common/nodeDebugger/ND_JsonTools.h"

void SP_Utils::exportMaterialParams(sptr<MaterialInst> material, MapMeshData& mmd)
{
   if (!material) return;
   auto paramData = material->saveParams();
   for (auto& [paramName, param] : paramData)
   {
      switch (param.type) {
         case ParamType::INT:
            mmd.material_settings.integers[paramName] = param.data.i;
            break;
         case ParamType::FLOAT:
            mmd.material_settings.floats[paramName] = param.data.f;
            break;
         case ParamType::BOOL:
            mmd.material_settings.flags[paramName] = param.data.b;
            break;
         case ParamType::VEC2:
            mmd.material_settings.vec2s[paramName] = param.data.v2;
            break;
         case ParamType::VEC3:
            mmd.material_settings.vec3s[paramName] = param.data.v3;
            break;
         case ParamType::VEC4:
            mmd.material_settings.vec4s[paramName] = param.data.v4;
            break;
            //NOTE: unsupported for now
         case ParamType::MAT4:
         case ParamType::TEXTURE:
         case ParamType::INVALID:
         case ParamType::CUBEMAP:
            break;
      }
   }
}

void SP_Utils::loadMaterialParams(sptr<MaterialInst> material, const MapMeshData& mmd)
{
   if (!material) return;

   for (const auto& [paramName, paramValue] : mmd.material_settings.flags)
   {
      SAFE_PARAM(material, paramName, paramValue);
   }
   for (const auto& [paramName, paramValue] : mmd.material_settings.integers)
   {
      SAFE_PARAM(material, paramName, paramValue);
   }
   for (const auto& [paramName, paramValue] : mmd.material_settings.floats)
   {
      SAFE_PARAM(material, paramName, paramValue);
   }
   for (const auto& [paramName, paramValue] : mmd.material_settings.vec2s)
   {
      SAFE_PARAM(material, paramName, paramValue);
   }
   for (const auto& [paramName, paramValue] : mmd.material_settings.vec3s)
   {
      SAFE_PARAM(material, paramName, paramValue);
   }
   for (const auto& [paramName, paramValue] : mmd.material_settings.vec4s)
   {
      SAFE_PARAM(material, paramName, paramValue);
   }
}

#ifndef FORCENODEDEBUG_OFF
void SP_Utils::onGuiSingleExportBtns(std::pair<MapMeshData, sptr<Mesh>>& meshInfo)
{
   static auto mmdGetter = [](std::pair<MapMeshData, sptr<Mesh>>& meshInfo) -> std::string
   {
      SPAWNER.updateMeshHierarchyRecords();
      SP_Utils::exportMaterialParams(meshInfo.second->getMaterialInst(), meshInfo.first);
      if (neAbs(meshInfo.first.position_local.z) < 0.001) meshInfo.first.position_local.z = 0.f;
      if (neAbs(meshInfo.first.position_world.z) < 0.001) meshInfo.first.position_world.z = 0.f;

      MapObjectsContainer tempContainer;
      tempContainer.mapMeshData.push_back(meshInfo.first);
      nlohmann::json exportJson;
      nlohmann::adl_serializer<MapObjectsContainer>::to_json(exportJson, tempContainer);
      std::string flatJson = jsonUnflatten(exportJson);
      return flatJson;
   };

   ImGui::Columns(2);
   if (ImGui::Button("Copy mesh JSON"))
   {
      // todo: export only mesh without json structure of MOC
      ImGui::SetClipboardText(mmdGetter(meshInfo).c_str());
   }
   ImGui::NextColumn();
   if (ImGui::Button("Export mesh JSON"))
   {
      jsWriteEvent(mmdGetter(meshInfo).c_str(), "extraMeshes.json");
   }
   ImGui::Columns(1);
   ImGui::Separator();
}

void SP_Utils::onGuiSingleExportBtns(Spawnable& obj)
{
   static auto modGetter = [](Spawnable& obj) -> std::string
   {
      SPAWNER.updateMeshHierarchyRecords();
      obj.updateChildrenData();

      MapObjectsContainer tempContainer;
      tempContainer.mapObjectsData.push_back(obj.getMapObjectsData());
      nlohmann::json exportJson;
      nlohmann::adl_serializer<MapObjectsContainer>::to_json(exportJson, tempContainer);
      std::string flatJson = jsonUnflatten(exportJson);
      return flatJson;
   };

   ImGui::Columns(2);
   if (ImGui::Button("Copy object JSON"))
   {
      // todo: export only object without json structure of MOC
      ImGui::SetClipboardText(modGetter(obj).c_str());
   }
   ImGui::NextColumn();
   if (ImGui::Button("Export object JSON"))
   {
      jsWriteEvent(modGetter(obj).c_str(), "extraObjects.json");
   }
   ImGui::Columns(1);
   ImGui::Separator();
}
#endif

