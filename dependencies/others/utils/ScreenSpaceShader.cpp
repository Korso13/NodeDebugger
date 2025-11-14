// Created by Timur Korsakov on 12.08.2025.
#include "ScreenSpaceShader.h"

#include "common/nodeDebugger/NodeDebugger.h"

void ScreenSpaceShader::init(sptr<MaterialInst> shaderMaterial)
{
   if (m_isInited) return;
   debug("[ScreenSpaceShader::init]");
   m_isInited = true;

   m_material = shaderMaterial;
   m_screen = Mesh::create::plane({1,1}, true);
   m_screen->setMaterialInst(m_material);
   m_screen->setRenderOrder(1);
   m_screen->setName("Screen_Render");
   m_material->enableBlending(true);

   m_hudSceneNode = make::sptr<Node>("HudSceneNode");

   m_passId = Render::addPass();
   Render::getPass(m_passId)->setRoot(make::sptr<RootNode>("ScreenSpace_Root"));
   m_rootNode = Render::getPass(m_passId)->getRoot();
   m_rootNode->addChild(m_hudSceneNode); m_hudSceneNode->setWorldTranslation(vec3::zero);
   m_rootNode->addChild(m_screen);
   m_screen->setWorldTranslation(vec3::zero);
   m_screen->setWorldRotation(Rotator(vec3::zero));
   m_screen->setWorldScale(vec3::one);

   auto camera = Render::getScreenCamera();
   const auto& screenFxPass = Render::getPass(m_passId);
   const auto& scenePass = Render::getPass(0);
   auto& hudPass = Render::getHudPass();
   screenFxPass->setCamera(make::sptr<Camera>("screenFx_camera", camera->getFov(), camera->getAspect(), camera->getNear(), camera->getFar()));
   m_screenFxCamera = screenFxPass->getCamera();
   m_screenFxCamera->setWorldTranslation(vec3(0.f, 0.f, -10.f));

   screenFxPass->setTransparentStrategy(NodesPass::TransparentRenderStrategy::RenderOrder);

   Render::getHudPass().setRoot(screenFxPass->getRoot());

   cacheRenderTargets();
   updateRenderTargets();

   event::Resize::subscribe(std::bind(&ScreenSpaceShader::onResize, this, std::placeholders::_1));
   onResize();

   event::Touch::Begin::subscribe(std::bind(&ScreenSpaceShader::onTouchBegin, this, std::placeholders::_1));
   event::Touch::End::subscribe(std::bind(&ScreenSpaceShader::onTouchEnd, this, std::placeholders::_1));
   event::Touch::Move::subscribe(std::bind(&ScreenSpaceShader::onTouchMove, this, std::placeholders::_1));

   m_fakeCameraCollider = m_screenFxCamera->addCollider<AABB>("screenFxDetection", vec3(0.0001));

   ND_NODE(m_screen, "Screen_Render");
}

void ScreenSpaceShader::onResize(const event::Resize& evt)
{
   if (!m_screen || !m_screenFxCamera)
      return;

   debug("[ScreenSpaceShader::onResize]");

   auto camera = Render::getScreenCamera();
   m_screenFxCamera->setPerspective(camera->getFov(), camera->getAspect(), camera->getNear(), camera->getFar());
   auto planeSize = m_screenFxCamera->getFrustumPlaneSize(10.f);
   m_screen->setWorldScale(vec3(planeSize.x, planeSize.y, 1.f));

   if (needRecacheRenderTargets())
      cacheRenderTargets();
   updateRenderTargets();
}

void ScreenSpaceShader::onTouchBegin(const event::Touch::Begin& evt)
{
   if (ND_CASTALLOWED)
      handleScreenSpaceFxScreencast(evt.point, ScreencastEvent::Down);
}

void ScreenSpaceShader::onTouchEnd(const event::Touch::End& evt)
{
   if (ND_CASTALLOWED)
      handleScreenSpaceFxScreencast(evt.point, ScreencastEvent::Up);
}

void ScreenSpaceShader::onTouchMove(const event::Touch::Move& evt)
{
   if (ND_CASTALLOWED)
      handleScreenSpaceFxScreencast(evt.point, ScreencastEvent::Move);
}

void ScreenSpaceShader::handleScreenSpaceFxScreencast(const math::point& point, ScreencastEvent evt)
{
   auto ray = m_screenFxCamera->createRayFromScreen(point);
   std::multimap<CollisionInfo, Collider*, SortCollisionInfoByDistance> intersectedColliders;

   CollisionInfo collisionInfo;
   m_rootNode->traversalChildren(
      [&ray, &intersectedColliders, &collisionInfo, this](Node& node)
      {
         if (node.getColliders().empty()) return;
         if (node.getRoot() != Render::getHudPass().getRoot()) return; //avoid triggering main pass's colliders

         std::for_each(node.getColliders().begin(), node.getColliders().end(),
            [&ray, &intersectedColliders, &collisionInfo, this](const std::pair<std::string, sptr<Collider>>& namedCollider)
               {
                  auto collider = namedCollider.second.get();
                  if(collider && collider->isEnabled() && collider->intersect(ray, collisionInfo))
                  {
                     collisionInfo.source = m_fakeCameraCollider.get();
                     intersectedColliders.insert(std::make_pair(collisionInfo, collider));
                  }
               });
      });

   for(const auto& [ci, collider] : intersectedColliders)
   {
      if (!collider) continue;

      collider->onScreencast(evt, ci);
      if(collider->isBlockScreencasts())
      {
         break;
      }
   }
}

bool ScreenSpaceShader::needRecacheRenderTargets() const
{
   float aspect = Render::getAspect();
   return m_cachedAspectRatios.first != aspect && m_cachedAspectRatios.second != aspect;
}

void ScreenSpaceShader::cacheRenderTargets()
{
   debug("[ScreenSpaceShader::cacheRenderTargets]");
   uvec2 viewportSize = Render::getSize();
   uvec2 viewportSizeInverted; viewportSizeInverted.h = viewportSize.w; viewportSizeInverted.w = viewportSize.h;
   m_cachedAspectRatios = {(CastF(viewportSize.h)/CastF(viewportSize.w)), (CastF(viewportSize.w)/CastF(viewportSize.h))};
   if (Render::getAspect() <= 1)
   {
      m_screenFxRenderTargetPort = make::sptr<TextureRenderTarget>(viewportSize, false, true);
      m_screenFxRenderTargetLs = make::sptr<TextureRenderTarget>(viewportSizeInverted, false, true);
   }
   else
   {
      m_screenFxRenderTargetLs = make::sptr<TextureRenderTarget>(viewportSize, false, true);
      m_screenFxRenderTargetPort = make::sptr<TextureRenderTarget>(viewportSizeInverted, false, true);
   }
}

void ScreenSpaceShader::updateRenderTargets()
{
   debug("[ScreenSpaceShader::updateRenderTargets]");
   float aspect = Render::getAspect();
   auto camera = Render::getScreenCamera();
   const auto& screenFxPass = Render::getPass(m_passId);
   const auto& scenePass = Render::getPass(0);

   scenePass->getCamera()->setRenderTarget((aspect <= 1.f) ? m_screenFxRenderTargetPort : m_screenFxRenderTargetLs);
   screenFxPass->setEnabled(true);
   auto renderTexture = cast_sptr<TextureRenderTarget>(scenePass->getCamera()->getRenderTarget())->getColorTexture();
   m_material->setTexture(TextureId::TEXTURE_0, renderTexture);
}
