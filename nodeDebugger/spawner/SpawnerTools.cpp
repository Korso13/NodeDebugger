//
// Created by Timur Korsakov on 14.11.2025.
//
#include "SpawnerTools.h"
#include "SpawnerStructs.h"

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
