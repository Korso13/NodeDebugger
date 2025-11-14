//
// Created by Timur Korsakov on 14.11.2025.
//
#pragma once
#include "W4Framework.h"

struct MapMeshData;
W4_USE_UNSTRICT_INTERFACE

namespace SP_Utils
{
   void exportMaterialParams(sptr<MaterialInst> material, MapMeshData& mmd);
   void loadMaterialParams(sptr<MaterialInst> material, const MapMeshData& mmd);
}