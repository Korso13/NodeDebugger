#include "PlacementStrategies.h"

#include "common/nodeDebugger/NodeDebugger.h"
#include "common/nodeDebugger/spawner/SpawnerStructs.h"
#include "others/utils/ScreenSpaceShader.h"
#include "sources/game/camera/CameraManager.h"

uvec2 operator+(const uvec2& lhv, const uvec2& rhv)
{
   return uvec2(lhv.x + rhv.x, lhv.y + rhv.y);
}
uvec2 operator+(const uvec2& lhv, const ivec2& rhv)
{
   return uvec2(lhv.x + rhv.x, lhv.y + rhv.y);
}

void PlacementStrategies::setContext(sptr<Node> node, sptr<Mesh> mesh, float distanceFromCamera)
{
   if (!node || !mesh)
      return;

   m_node = node;
   m_mesh = mesh;
   m_distanceFromCamera = distanceFromCamera;
   if (m_aligning == ResizeStrategy::FixedPos)
   {
      vec2::cref meshHalfSize = getMeshPlaneSize(mesh).withoutZ() /2.f;
      mesh->setLocalTranslation(vec3(-meshHalfSize.x, -meshHalfSize.y, 0.f));
   }
}

void PlacementStrategies::onResize(float aspect)
{
   if (m_node.expired() || m_mesh.expired() || m_aligning == ResizeStrategy::None)
      return;

   switch (m_aligning)
   {
      case ResizeStrategy::FixedPos:
         handleFixedPos(aspect);
         break;
      case ResizeStrategy::ScreenPercent:
         handleScreenPercent(aspect);
         break;
      case ResizeStrategy::MarginOffsetPixel:
         handleMarginOffsetPixel(aspect);
         break;
      case ResizeStrategy::MarginOffsetPercent:
         handleMarginOffsetPercent(aspect);
         break;
      case ResizeStrategy::None:
         break;
   }
}

void PlacementStrategies::enableAutoResize()
{
   event::Resize::subscribe([this](const event::Resize& evt){onResize();});
}

void PlacementStrategies::disableStrategies()
{
   if (!m_mesh.expired())
      m_mesh.lock()->setLocalTranslation(vec3::zero);
   m_aligning = ResizeStrategy::None;
};

void PlacementStrategies::setFixedPosPlacementMode(vec3 blockPos, vec3 blockPosLS)
{
   // if (!m_mesh.expired())
   // {
   //    auto mesh = m_mesh.lock();
   //    vec2::cref meshHalfSize = getMeshPlaneSize(mesh).withoutZ() /2.f;
   //    mesh->setLocalTranslation(vec3(-meshHalfSize.x, -meshHalfSize.y, 0.f));
   // }
   m_fixedPos = blockPos;
   m_fixedPosLS = blockPosLS;
   m_aligning = ResizeStrategy::FixedPos;
   onResize();
}

void PlacementStrategies::setScreenPosPlacementMode(vec2 portraitPos, vec2 landscapePos)
{
   m_screenPos = portraitPos;
   m_screenPosLS = landscapePos;
   // if (!m_mesh.expired())
   //    m_mesh.lock()->setLocalTranslation(vec3::zero);
   m_aligning = ResizeStrategy::ScreenPercent;
   onResize();
}

void PlacementStrategies::setMarginOffsetPlacementMode(uvec2 anchorOffset, uvec2 anchorOffsetLS, ivec2 anchorCorner, ivec2 anchorCornerLS)
{
   m_anchorCorner = anchorCorner;
   m_anchorCornerLS = anchorCornerLS;
   m_anchorOffset = ivec2(anchorOffset.x, anchorOffset.y);
   m_anchorOffsetLS = ivec2(anchorOffsetLS.x, anchorOffsetLS.y);
   // if (!m_mesh.expired())
   //    m_mesh.lock()->setLocalTranslation(vec3::zero);
   m_aligning = ResizeStrategy::MarginOffsetPixel;
   onResize();
}

void PlacementStrategies::setMarginPercentPlacementMode(vec2 anchorOffsetInPercent, vec2 anchorOffsetLS, ivec2 anchorCorner, ivec2 anchorCornerLS)
{
   m_anchorCorner = anchorCorner;
   m_anchorCornerLS = anchorCornerLS;
   m_anchorOffsetInPercent = anchorOffsetInPercent;
   m_anchorOffsetInPercentLS = anchorOffsetLS;
   // if (!m_mesh.expired())
   //    m_mesh.lock()->setLocalTranslation(vec3::zero);
   m_aligning = ResizeStrategy::MarginOffsetPercent;
   onResize();
}

void PlacementStrategies::exposeDebuggingSettings(const std::string& menuFolderName)
{
   if (m_node.expired() || m_mesh.expired() || m_aligning == ResizeStrategy::None)
      return;

   auto refreshCb = [this](){onResize();};
   ND_VARCB("["+menuFolderName+"] Distance from camera", &m_distanceFromCamera, refreshCb);
   switch (m_aligning)
   {
      case ResizeStrategy::FixedPos:
         ND_VARCB("["+menuFolderName+"] Fixed pos (Portrait)", &m_fixedPos, refreshCb);
         ND_VARCB("["+menuFolderName+"] Fixed pos (Landscape)", &m_fixedPosLS, refreshCb);
         break;
      case ResizeStrategy::ScreenPercent:
         ND_VARCB("["+menuFolderName+"] Screen pos in percent (Portrait)", &m_screenPos, refreshCb);
         ND_VARCB("["+menuFolderName+"] Screen pos in percent (Landscape)", &m_screenPosLS, refreshCb);
         break;
      case ResizeStrategy::MarginOffsetPixel:
         ND_VARCB("["+menuFolderName+"] Corner X (Portrait)", &m_anchorCorner.x, refreshCb);
         ND_VARCB("["+menuFolderName+"] Corner Y (Portrait)", &m_anchorCorner.y, refreshCb);
         ND_VARCB("["+menuFolderName+"] Corner X (Landscape)", &m_anchorCornerLS.x, refreshCb);
         ND_VARCB("["+menuFolderName+"] Corner Y (Landscape)", &m_anchorCornerLS.y, refreshCb);
         ND_VARCB("["+menuFolderName+"] Offset X pixels (Portrait)", &m_anchorOffset.x, refreshCb);
         ND_VARCB("["+menuFolderName+"] Offset Y pixels (Portrait)", &m_anchorOffset.y, refreshCb);
         ND_VARCB("["+menuFolderName+"] Offset X pixels (Landscape)", &m_anchorOffsetLS.x, refreshCb);
         ND_VARCB("["+menuFolderName+"] Offset Y pixels (Landscape)", &m_anchorOffsetLS.y, refreshCb);
         break;
      case ResizeStrategy::MarginOffsetPercent:
         ND_VARCB("["+menuFolderName+"] Corner X (Portrait)", &m_anchorCorner.x, refreshCb);
         ND_VARCB("["+menuFolderName+"] Corner Y (Portrait)", &m_anchorCorner.y, refreshCb);
         ND_VARCB("["+menuFolderName+"] Corner X (Landscape)", &m_anchorCornerLS.x, refreshCb);
         ND_VARCB("["+menuFolderName+"] Corner Y (Landscape)", &m_anchorCornerLS.y, refreshCb);
         ND_VARCB("["+menuFolderName+"] Offset in percent (Portrait)",  &m_anchorOffsetInPercent, refreshCb);
         ND_VARCB("["+menuFolderName+"] Offset in percent (Landscape)", &m_anchorOffsetInPercentLS, refreshCb);
         break;
      case ResizeStrategy::None:
         break;
   }
}

void PlacementStrategies::handleFixedPos(float aspect)
{
   auto offset = SV3("ui_offset");
   auto fixedPosBlock = (Render::getAspect() < 1.f) ? m_fixedPos : m_fixedPosLS;

   if (aspect >= 1.0)
   {
      fixedPosBlock = vec3{fixedPosBlock.x + ((aspect - 1) * offset.x), fixedPosBlock.y, fixedPosBlock.z};
   } else
   {
      fixedPosBlock = vec3{fixedPosBlock.x, fixedPosBlock.y - (1 - (1 / aspect)) * offset.y, fixedPosBlock.z};
   }
   m_node.lock()->setLocalTranslation(fixedPosBlock);
}

void PlacementStrategies::handleScreenPercent(float aspect)
{
   if (m_screenPos == INVALIDPOS.withoutZ() && m_screenPosLS == INVALIDPOS.withoutZ())
      return;

   vec3 meshBoundariesOffset = vec3::zero;
   if (!m_mesh.expired()) meshBoundariesOffset = getMeshPlaneSize(m_mesh.lock())/2.f;
   meshBoundariesOffset.z = 0.f;
   float distFromCamera =  m_distanceFromCamera;
   vec3 pos = vec3::zero;
   vec3 placementSetting = vec3::zero;

   if (aspect >= 1)
   {
      if (m_screenPosLS == INVALIDPOS.withoutZ())
         return;
      placementSetting = CommonFunction::vec2toVec3(m_screenPosLS);
   }
   else
   {
      if (m_screenPos == INVALIDPOS.withoutZ())
         return;
      placementSetting = CommonFunction::vec2toVec3(m_screenPos);
   }
   //adaptive screen placement
   pos = placementSetting;
   pos.x *= (distFromCamera / 2.f) * (std::max(1.f, aspect));
   pos.y *= (distFromCamera / 2.f) * (1 / std::min(1.f, aspect));
   pos.z = distFromCamera; //fixed z pos

   if (pos.x < 0) meshBoundariesOffset.x *= -1.f;
   if (pos.y < 0) meshBoundariesOffset.y *= -1.f;

   if(!m_node.expired())
   {
      auto node = m_node.lock();
      node->setLocalTranslation(pos);
      node->translateWorld(CAMERA.checkPointOutOfCam(node->getWorldTranslation() + meshBoundariesOffset));
   }
}

void PlacementStrategies::handleMarginOffsetPixel(float aspect)
{
   auto mesh = m_mesh.lock();
   auto node = m_node.lock();
   ivec2 anchorCorner = (aspect <= 1.f) ? m_anchorCorner : m_anchorCornerLS;
	ivec2 anchorOffset = (aspect <= 1.f) ? m_anchorOffset : m_anchorOffsetLS;
	ivec2 anchorCornerNonZero = (aspect <= 1.f) ? m_anchorCorner : m_anchorCornerLS;
   if (anchorCornerNonZero.x == 0) anchorCornerNonZero.x = -1; if (anchorCornerNonZero.y == 0) anchorCornerNonZero.y = -1;
	//Offset from mesh's vertice in world coords. Vertice is picked depending on m_anchorCorner
	vec3 meshBoundaries = vec3::zero;
	if (mesh) meshBoundaries = getMeshPlaneSize(mesh)/2.f;
	vec3 meshEdgePointOffset = meshBoundaries * vec3(anchorCorner.x, anchorCorner.y, 0.f); //applying signs from anchor

	//Calculating screen pixel, where mesh's selected vertice must be placed. All values are in screen pixels
	uvec2 screenSize = Platform::getSize();
	ivec2 screenHalfSize = ivec2(std::floor(screenSize.x / 2), std::floor(screenSize.y / 2));
	ivec2 screenCenter = screenHalfSize; //counts from left-top corner
	ivec2 toScreenCornerPixelVec = ivec2(CastI(screenHalfSize.x) * anchorCorner.x, CastI(screenHalfSize.y) * -anchorCorner.y);
	ivec2 screenMarginOffsetInPixels = ivec2(-anchorOffset.x * anchorCornerNonZero.x, anchorOffset.y * anchorCornerNonZero.y); //operations inversed due to it being offset from corner
	ivec2 anchorWithOffset = screenCenter + toScreenCornerPixelVec + screenMarginOffsetInPixels;

	//calculating intermediate variable, representing a 'UV'-like coordinate of screen space (or rather of slice of camera Frustum at set z offset from camera)
	vec2 edgePosScreenPercent = vec2(CastF(anchorWithOffset.x) / CastF(screenSize.x), CastF(anchorWithOffset.y) / CastF(screenSize.y));

	//calculating mesh's selected vertice position in coords local to camera center
	auto cam = getCorrectCamera();
	vec2 screenPlaneSize = cam->getFrustumPlaneSize(m_distanceFromCamera);
	vec2 halfScreenPlaneSize = screenPlaneSize / 2.f;
	vec3 camAnchorPoint = cam->getWorldTranslation() * vec3(1.f, 1.f, 0.f) + vec3(-halfScreenPlaneSize.x, halfScreenPlaneSize.y, 0.f);
	vec3 screenSpaceEdgePos = vec3(screenPlaneSize.x * edgePosScreenPercent.x, -(screenPlaneSize.y * edgePosScreenPercent.y), 0.f);

	//converting screenSpaceEdgePos to actual world coordinates and subtracting offset of mesh's selected vertice from mesh's center (which is placed by setWorldTranslation)
	vec3 newRootPos = camAnchorPoint + screenSpaceEdgePos - meshEdgePointOffset;
	newRootPos.z = cam->getWorldTranslation().z + m_distanceFromCamera;
   if (node != mesh && mesh->getLocalTranslation() != vec3::zero)
      newRootPos += node->getWorldTranslation() - mesh->getWorldTranslation();

	if(node)
		node->setWorldTranslation(newRootPos);

	//Verbose debugger
   if (bVerboseDebuggerOn)
      info("[PlacementStrategies::handleMarginOffsetPixel] \n Cam pos %s \nNode pos %s \n=================\nmeshBoundaries %s\nmeshEdgePointOffset %s\nscreenSize {%s}\nscreenHalfSize/screenCenter {%s}\ntoScreenCornerPixelVec {%s}\nscreenMarginOffsetInPixels {%s}\nanchorWithOffset(screen center + half size to anchorCorner - anchorOffset from anchorCorner) {%s}\nedgePosScreenPercent(edge of mesh in UV of screen) %s\nscreenPlaneSize %s\nhalfScreenPlaneSize %s\ncamAnchorPoint %s\nscreenSpaceEdgePos %s",
	 	   CVEC(cam->getWorldTranslation()), CVEC(newRootPos), CVEC(meshBoundaries), CVEC(meshEdgePointOffset), CSTR(screenSize), CSTR(screenHalfSize), CSTR(toScreenCornerPixelVec), CSTR(screenMarginOffsetInPixels), CSTR(anchorWithOffset), CVEC(edgePosScreenPercent), CVEC(screenPlaneSize), CVEC(halfScreenPlaneSize), CVEC(camAnchorPoint), CVEC(screenSpaceEdgePos));
}

void PlacementStrategies::handleMarginOffsetPercent(float aspect)
{
      auto mesh = m_mesh.lock();
      auto node = m_node.lock();
   	ivec2 anchorCorner = (aspect <= 1.f) ? m_anchorCorner : m_anchorCornerLS;
		vec2 anchorOffset = (aspect <= 1.f) ? m_anchorOffsetInPercent : m_anchorOffsetInPercentLS;
      ivec2 anchorCornerNonZero = (aspect <= 1.f) ? m_anchorCorner : m_anchorCornerLS;
      if (anchorCornerNonZero.x == 0) anchorCornerNonZero.x = -1; if (anchorCornerNonZero.y == 0) anchorCornerNonZero.y = -1;
		//Offset from mesh's vertice in world coords. Vertice is picked depending on anchorCorner
		vec3 meshBoundaries = vec3::zero;
		if (mesh) meshBoundaries = getMeshPlaneSize(mesh)/2.f;
		vec3 meshEdgePointOffset = meshBoundaries * vec3(anchorCorner.x, anchorCorner.y, 0.f); //applying signs from anchor

		//Calculating screen pixel, where mesh's selected vertice must be placed. All values are in screen pixels
		uvec2 screenSize = Platform::getSize();
		ivec2 screenHalfSize = ivec2(std::floor(screenSize.x / 2), std::floor(screenSize.y / 2));
		ivec2 screenCenter = screenHalfSize; //counts from left-top corner
		ivec2 toScreenCornerPixelVec = ivec2(CastI(screenHalfSize.x) * anchorCorner.x, CastI(screenHalfSize.y) * -anchorCorner.y);
		ivec2 screenMarginOffsetInPixels = ivec2(-std::floor(CastF(screenSize.x) * anchorOffset.x * CastF(anchorCornerNonZero.x)), std::floor(CastF(screenSize.y) * anchorOffset.y * CastF(anchorCornerNonZero.y))); //operations inversed due to it being offset from corner
		ivec2 anchorWithOffset = screenCenter + toScreenCornerPixelVec + screenMarginOffsetInPixels;

		//calculating intermediate variable, representing a 'UV'-like coordinate of screen space (or rather of slice of camera Frustum at set z offset from camera)
		vec2 edgePosScreenPercent = vec2(CastF(anchorWithOffset.x) / CastF(screenSize.x), CastF(anchorWithOffset.y) / CastF(screenSize.y));

		//calculating mesh's selected vertice position in coords local to camera center
		auto cam = getCorrectCamera();
		vec2 screenPlaneSize = cam->getFrustumPlaneSize(m_distanceFromCamera);
		vec2 halfScreenPlaneSize = screenPlaneSize / 2.f;
		vec3 camAnchorPoint = cam->getWorldTranslation() * vec3(1.f, 1.f, 0.f) + vec3(-halfScreenPlaneSize.x, halfScreenPlaneSize.y, 0.f);
		vec3 screenSpaceEdgePos = vec3(screenPlaneSize.x * edgePosScreenPercent.x, -(screenPlaneSize.y * edgePosScreenPercent.y), 0.f);

		//converting screenSpaceEdgePos to actual world coordinates and subtracting offset of mesh's selected vertice from mesh's center (which is placed by setWorldTranslation)
		vec3 newRootPos = camAnchorPoint + screenSpaceEdgePos - meshEdgePointOffset;
		newRootPos.z = cam->getWorldTranslation().z + m_distanceFromCamera;
      if (node != mesh && mesh->getLocalTranslation() != vec3::zero)
         newRootPos += node->getWorldTranslation() - mesh->getWorldTranslation();

		if(node)
			node->setWorldTranslation(newRootPos);

   //Verbose debugger
   if (bVerboseDebuggerOn)
      info("[PlacementStrategies::handleMarginOffsetPixel] \n Cam pos %s \nNode pos %s \n=================\nmeshBoundaries %s\nmeshEdgePointOffset %s\nscreenSize {%s}\nscreenHalfSize/screenCenter {%s}\ntoScreenCornerPixelVec {%s}\nscreenMarginOffsetInPixels {%s}\nanchorWithOffset(screen center + half size to anchorCorner - anchorOffset from anchorCorner) {%s}\nedgePosScreenPercent(edge of mesh in UV of screen) %s\nscreenPlaneSize %s\nhalfScreenPlaneSize %s\ncamAnchorPoint %s\nscreenSpaceEdgePos %s",
          CVEC(cam->getWorldTranslation()), CVEC(newRootPos), CVEC(meshBoundaries), CVEC(meshEdgePointOffset), CSTR(screenSize), CSTR(screenHalfSize), CSTR(toScreenCornerPixelVec), CSTR(screenMarginOffsetInPixels), CSTR(anchorWithOffset), CVEC(edgePosScreenPercent), CVEC(screenPlaneSize), CVEC(halfScreenPlaneSize), CVEC(camAnchorPoint), CVEC(screenSpaceEdgePos));
}

sptr<Camera> PlacementStrategies::getCorrectCamera()
{
   if (m_node.expired())
   {
      err("[PlacementStrategies::getCorrectCamera] Node expired!");
      return nullptr;
   }

   auto node = m_node.lock();
   if (node->getRoot() == Render::getHudPass().getRoot() && SCREEN_FX.getCamera())
   {
      return SCREEN_FX.getCamera();
   }
   else
      return Render::getScreenCamera();

   return nullptr;
}

bool PlacementStrategies::onGui(const char* label)
{
#ifndef DEBUG
   return false;
#else
   bool strategy_changed = false;
   bool values_changed = false;

   if (ImGui::TreeNode(label))
   {
      ImGui::Text("Placement Strategy");
      if(ImGui::RadioButton("None", m_aligning == ResizeStrategy::None)) {m_aligning = ResizeStrategy::None; strategy_changed = true;}
      if(ImGui::RadioButton("Fixed Pos", m_aligning == ResizeStrategy::FixedPos)) {m_aligning = ResizeStrategy::FixedPos; strategy_changed = true; }
      if(ImGui::RadioButton("Screen Percent", m_aligning == ResizeStrategy::ScreenPercent)) {m_aligning = ResizeStrategy::ScreenPercent; strategy_changed = true; }
      if(ImGui::RadioButton("Margin Offset in Pixels", m_aligning == ResizeStrategy::MarginOffsetPixel)) {m_aligning = ResizeStrategy::MarginOffsetPixel; strategy_changed = true; }
      if(ImGui::RadioButton("Margin Offset in Percent", m_aligning == ResizeStrategy::MarginOffsetPercent)) {m_aligning = ResizeStrategy::MarginOffsetPercent; strategy_changed = true; }

      values_changed = values_changed || ImGui::Auto("Distance from camera", &m_distanceFromCamera);
      switch (m_aligning)
      {
         case ResizeStrategy::FixedPos:
            if (strategy_changed) setFixedPosPlacementMode(m_fixedPos, m_fixedPosLS);
            values_changed = values_changed || NDGui::Auto("Fixed pos (Portrait)", m_fixedPos);
            values_changed = values_changed || NDGui::Auto("Fixed pos (Landscape)", m_fixedPosLS);
            break;
         case ResizeStrategy::ScreenPercent:
            if (strategy_changed) setScreenPosPlacementMode(m_screenPos, m_screenPosLS);
           values_changed = values_changed || NDGui::Auto("Screen pos in percent (Portrait)", m_screenPos);
           values_changed = values_changed || NDGui::Auto("Screen pos in percent (Landscape)", m_screenPosLS);
            break;
         case ResizeStrategy::MarginOffsetPixel:
            if (strategy_changed) setMarginOffsetPlacementMode(uvec2(uint32_t(m_anchorOffset.x), uint32_t(m_anchorOffset.y)), uvec2(uint32_t(m_anchorOffsetLS.x), uint32_t(m_anchorOffsetLS.y)), m_anchorCorner, m_anchorCornerLS);
            values_changed = values_changed || NDGui::Auto("Corner (Portrait)", m_anchorCorner);
            values_changed = values_changed || NDGui::Auto("Corner (Landscape)", m_anchorCornerLS);
            values_changed = values_changed || NDGui::Auto("Offset pixels (Portrait)", m_anchorOffset);
            values_changed = values_changed || NDGui::Auto("Offset pixels (Landscape)", m_anchorOffsetLS);
            break;
         case ResizeStrategy::MarginOffsetPercent:
            if (strategy_changed) setMarginPercentPlacementMode(m_anchorOffsetInPercent, m_anchorOffsetInPercentLS, m_anchorCorner, m_anchorCornerLS);
            values_changed = values_changed || NDGui::Auto("Corner (Portrait)", m_anchorCorner);
            values_changed = values_changed || NDGui::Auto("Corner (Landscape)", m_anchorCornerLS);
            values_changed = values_changed || NDGui::Auto("Offset in percent (Portrait)",  m_anchorOffsetInPercent);
            values_changed = values_changed || NDGui::Auto("Offset in percent (Landscape)", m_anchorOffsetInPercentLS);
         case ResizeStrategy::None:
            break;
      }
      if (values_changed) onResize();
      ImGui::TreePop();
   }
   return strategy_changed || values_changed;
#endif
}

sptr<PlacementStrategies> PlacementStrategies::makeStrategyFromMOD(MapObjectsData& mod, sptr<Node> root, sptr<Mesh> mesh)
{
   sptr<PlacementStrategies> newStrategy = M_SPTR<PlacementStrategies>();
   auto& settings = mod.settings;
   newStrategy->setContext(root, mesh, settings.floats["PS_distFromCamera"]);
   //usually called when creating newStrategy from empty settings, instead of loading one from mapObjects.json
   if (settings.floats["PS_distFromCamera"] == 0)
   {
      sptr<Camera> cam = newStrategy->getCorrectCamera();
      settings.floats["PS_distFromCamera"] = neAbs(cam->getWorldTranslation().z - mesh->getWorldTranslation().z);
      newStrategy->setContext(root, mesh, settings.floats["PS_distFromCamera"]);
   }

   auto strategy = static_cast<PlacementStrategies::ResizeStrategy>(settings.integers["PS_strategy"]);
   switch (strategy)
   {
      case PlacementStrategies::ResizeStrategy::FixedPos:
         newStrategy->setFixedPosPlacementMode(settings.vec3s["PS_FixedPosPort"], settings.vec3s["PS_FixedPosLS"]);
         break;
      case PlacementStrategies::ResizeStrategy::ScreenPercent:
         newStrategy->setScreenPosPlacementMode(settings.vec2s["PS_ScreenPosPort"], settings.vec2s["PS_ScreenPosLS"]);
         break;
      case PlacementStrategies::ResizeStrategy::MarginOffsetPixel:
         newStrategy->setMarginOffsetPlacementMode(
            uvec2(settings.integers["PS_AnchorOffsetX_Port"],  settings.integers["PS_AnchorOffsetY_Port"]),
            uvec2(settings.integers["PS_AnchorOffsetX_LS"],    settings.integers["PS_AnchorOffsetY_LS"]),
            ivec2(settings.integers["PS_MarginOffsetAnchorX_Port"],  settings.integers["PS_MarginOffsetAnchorY_Port"]),
            ivec2(settings.integers["PS_MarginOffsetAnchorX_LS"],    settings.integers["PS_MarginOffsetAnchorY_LS"]));
         break;
      case PlacementStrategies::ResizeStrategy::MarginOffsetPercent:
         newStrategy->setMarginPercentPlacementMode(settings.vec2s["PS_MarginPercentPort"], settings.vec2s["PS_MarginPercentLS"],
            ivec2(settings.integers["PS_MarginPercentAnchorX_Port"], settings.integers["PS_MarginPercentAnchorY_Port"]),
            ivec2(settings.integers["PS_MarginPercentAnchorX_LS"],   settings.integers["PS_MarginPercentAnchorY_LS"]));
         break;
      case PlacementStrategies::ResizeStrategy::None:
         break;
   }
   newStrategy->enableAutoResize();
   newStrategy->onResize();
   return newStrategy;
}

void PlacementStrategies::exportStrategyToMOD(MapObjectsData& mod, sptr<PlacementStrategies> strategy)
{
   if (!strategy)
      return;

   auto& settings = mod.settings;
   settings.flags["use_placement_strategy"] = true;
   settings.integers["PS_strategy"] = CastI(strategy->m_aligning);
   settings.floats["PS_distFromCamera"] = strategy->m_distanceFromCamera;

   switch (strategy->m_aligning)
   {
      case PlacementStrategies::ResizeStrategy::FixedPos:
         settings.vec3s["PS_FixedPosPort"] = strategy->m_fixedPos;
         settings.vec3s["PS_FixedPosLS"] = strategy->m_fixedPosLS;
         break;
      case PlacementStrategies::ResizeStrategy::ScreenPercent:
         settings.vec2s["PS_ScreenPosPort"] = strategy->m_screenPos;
         settings.vec2s["PS_ScreenPosLS"] = strategy->m_screenPosLS;
         break;
      case PlacementStrategies::ResizeStrategy::MarginOffsetPixel:
         settings.integers["PS_AnchorOffsetX_Port"] = strategy->m_anchorOffset.x;
         settings.integers["PS_AnchorOffsetY_Port"] = strategy->m_anchorOffset.y;
         settings.integers["PS_AnchorOffsetX_LS"] = strategy->m_anchorOffsetLS.x;
         settings.integers["PS_AnchorOffsetY_LS"] = strategy->m_anchorOffsetLS.y;
         settings.integers["PS_MarginOffsetAnchorX_Port"] = strategy->m_anchorCorner.x;
         settings.integers["PS_MarginOffsetAnchorY_Port"] = strategy->m_anchorCorner.y;
         settings.integers["PS_MarginOffsetAnchorX_LS"]   = strategy->m_anchorCornerLS.x;
         settings.integers["PS_MarginOffsetAnchorY_LS"]   = strategy->m_anchorCornerLS.y;
         break;
      case PlacementStrategies::ResizeStrategy::MarginOffsetPercent:
         settings.vec2s["PS_MarginPercentPort"] = strategy->m_anchorOffsetInPercent;
         settings.vec2s["PS_MarginPercentLS"] = strategy->m_anchorOffsetInPercentLS;
         settings.integers["PS_MarginPercentAnchorX_Port"] = strategy->m_anchorCorner.x;
         settings.integers["PS_MarginPercentAnchorY_Port"] = strategy->m_anchorCorner.y;
         settings.integers["PS_MarginPercentAnchorX_LS"] = strategy->m_anchorCornerLS.x;
         settings.integers["PS_MarginPercentAnchorY_LS"] = strategy->m_anchorCornerLS.y;
         break;
      case PlacementStrategies::ResizeStrategy::None:
         break;
   }
}
