//
// Created by Timur Korsakov on 14.11.2025.
//
#pragma once
#include "W4Framework.h"
#include "common/nodeDebugger/ND_Defines.h"

class Spawnable;
struct MapObjectsData;
struct MapMeshData;

W4_USE_UNSTRICT_INTERFACE

namespace SP_Utils
{
   void exportMaterialParams(sptr<MaterialInst> material, MapMeshData& mmd);
   void loadMaterialParams(sptr<MaterialInst> material, const MapMeshData& mmd);

   SHIPPING_STATIC void onGuiSingleExportBtns(std::pair<MapMeshData, sptr<Mesh>>& meshInfo) NOCOMP;
   SHIPPING_STATIC void onGuiSingleExportBtns(Spawnable& obj) NOCOMP;
}