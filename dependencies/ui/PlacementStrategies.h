// Created by Timur Korsakov on 31.07.2025.
#pragma once
#include "W4Framework.h"
#include "common/Defines.h"

struct MapObjectsData;
W4_USE_UNSTRICT_INTERFACE

//todo: add scaling strategies (stretching hor&vert, scaling to fit, proportional scaling(?))

//Usage:
//1) Create PlacementStrategies object 2) setContext 3) call one of strategies 4) call onResize
class PlacementStrategies
{
public:
   // Call right after instancing.
   // node - what is moved (can be == mesh), mesh that must be seen on screen (must be child of node)
   void setContext(sptr<Node> node, sptr<Mesh> mesh, float distanceFromCamera);
   // Call on each resize ...
   void onResize(float aspect = Render::getAspect());
   // ...on enable self-resizing
   void enableAutoResize();

   //pick one strategy:

   //disables onResize placement
   void disableStrategies();
   //inGameUI-like adaptive node placement. Difficult to pick the right coordinate, but scales well with extremely long devices. Does not guarantee mesh's full visibility
   void setFixedPosPlacementMode(vec3 blockPos = SV3F("ui_rightTopBlock", vec3(0.1, 0.1, 0.2)), vec3 blockPosLS = SV3F("ui_rightTopBlock", vec3(0.1, 0.1, 0.2)));
   //Places counter's center in certain part of the screen in percent, vec2 x,y values must range between -1.f and 1.f. Guarantees mesh's full visibility
   void setScreenPosPlacementMode(vec2 portraitPos, vec2 landscapePos);
   //Sets counter to a specific screen corner with fixed offset in pixels (relative to mesh's closest vertice). Guarantees mesh's full visibility
   //anchorCorner x and y must be either set to -1 or 1 (corner relative to screen's center)
   void setMarginOffsetPlacementMode(uvec2 anchorOffset, uvec2 anchorOffsetLS, ivec2 anchorCorner = ivec2(1, 1), ivec2 anchorCornerLS = ivec2(1, 1));
   //Sets counter to a specific screen corner with offset in percent of screen size (relative to mesh's closest vertice). Works better when resizing to longer devices. Guarantees mesh's full visibility
   //anchorCorner x and y must be either set to -1 or 1 (corner relative to screen's center)
   //anchorOffsetInPercent x and y must be between 0.f and 1.f (0% - 100% margin offset)
   void setMarginPercentPlacementMode(vec2 anchorOffsetInPercent, vec2 anchorOffsetLS, ivec2 anchorCorner = ivec2(1, 1), ivec2 anchorCornerLS = ivec2(1, 1));

   float getDistanceToCameraSet() const {return m_distanceFromCamera;}

   // call after picking a strategy. It will add submenu with name menuFolderName and settings for selected strategy
   void exposeDebuggingSettings(const std::string& menuFolderName);

   //don't call youself!
   bool onGui(const char* label);

   static sptr<PlacementStrategies> makeStrategyFromMOD(MapObjectsData& mod, sptr<Node> root, sptr<Mesh> mesh);
   static void exportStrategyToMOD(MapObjectsData& mod, sptr<PlacementStrategies> strategy);

private:

   void handleFixedPos(float aspect);
   void handleScreenPercent(float aspect);
   void handleMarginOffsetPixel(float aspect);
   void handleMarginOffsetPercent(float aspect);
   sptr<Camera> getCorrectCamera();

public:

   bool bVerboseDebuggerOn = false;

private:

   enum class ResizeStrategy
   {
      FixedPos, //takes vec3 to place, relies on inGameUi placement logic
      ScreenPercent, //places node in certain percent of screen from the center
      MarginOffsetPixel, //places UI mesh at certain offset from screen edges in pixels
      MarginOffsetPercent, //places UI mesh at certain offset from screen edges in percents
      None
   }  m_aligning = ResizeStrategy::None;
   vec3 m_fixedPos = vec3(0.0f);
   vec3 m_fixedPosLS = vec3(0.0f);
   vec2 m_screenPos = INVALIDPOS.withoutZ();
   vec2 m_screenPosLS = INVALIDPOS.withoutZ();
   ivec2 m_anchorCorner;
   ivec2 m_anchorCornerLS;
   ivec2 m_anchorOffset;
   ivec2 m_anchorOffsetLS;
   vec2 m_anchorOffsetInPercent;
   vec2 m_anchorOffsetInPercentLS;

   //context storage
   wptr<Node> m_node;
   wptr<Mesh> m_mesh;
   float m_distanceFromCamera = 0.2f;

   sptr<Camera> m_camera;
};
