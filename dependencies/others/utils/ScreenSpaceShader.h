// Created by Timur Korsakov on 12.08.2025.
#pragma once
#include "W4Framework.h"
#include "common/Defines.h"

W4_USE_UNSTRICT_INTERFACE

#define SCREEN_FX ScreenSpaceShader::get()

class ScreenSpaceShader
{
   public:
   static ScreenSpaceShader& get()
   {
      static ScreenSpaceShader instance;
      return instance;
   }

   void init(sptr<MaterialInst> shaderMaterial);
   sptr<Node> getNodeForHud(){return m_hudSceneNode;}

   template<class T>
   void setShaderParam(const std::string paramName, const T& paramValue)
   {
      if (!m_material) return;

      SAFE_PARAM(m_material, paramName, paramValue);
   }

   vec3 getCameraPosition(){return m_screenFxCamera->getWorldTranslation();};
   sptr<Camera> getCamera(){return m_screenFxCamera;};

private:
   ScreenSpaceShader() = default;
   void onResize(const event::Resize& evt = event::Resize(Render::getSize()));
   void onTouchBegin(const event::Touch::Begin& evt);
   void onTouchEnd(const event::Touch::End& evt);
   void onTouchMove(const event::Touch::Move& evt);
   void handleScreenSpaceFxScreencast(const math::point& point, ScreencastEvent evt);

   bool needRecacheRenderTargets() const;
   void cacheRenderTargets();
   void updateRenderTargets();

private:

   bool m_isInited = false;
   size_t m_passId;
   sptr<MaterialInst> m_material = nullptr;
   sptr<Mesh> m_screen = nullptr;
   sptr<Camera> m_screenFxCamera = nullptr;
   sptr<RootNode> m_rootNode = nullptr;
   sptr<Node> m_hudSceneNode = nullptr;
   sptr<Collider> m_fakeCameraCollider = nullptr;

   std::pair<float,float> m_cachedAspectRatios = {0.f, 0.f};
   sptr<TextureRenderTarget> m_screenFxRenderTargetPort = nullptr;
   sptr<TextureRenderTarget> m_screenFxRenderTargetLs = nullptr;
};