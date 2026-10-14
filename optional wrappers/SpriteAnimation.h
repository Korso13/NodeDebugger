#pragma once
#include "common/Defines.h"
#include "common/nodeDebugger/spawner/SpawnerWrappers.h"

class SpriteAnimation
{
 friend class SpriteAnimation_S;

public:
  SpriteAnimation(
	  const std::string &configPath,
	  sptr<Node> root,
	  const std::string &nameSprite,
	  const std::function<void()> &onFinish = nullptr
  );

  ~SpriteAnimation();
  virtual void update(float &dt);
  void initialize();
  void setDisableOnEnd(bool flag);
  int getCurrentFrame() const;
  void setJpgMode(bool enabled);

  [[nodiscard]] sptr<Mesh> getMesh() { return m_object; }
  [[nodiscard]] sptr<Node> getRoot() { return m_view; }

  [[nodiscard]] vec3 getPosition() { return m_object->getLocalTranslation(); }

  void setPosition(vec3 pos) { m_object->setLocalTranslation(pos); }

  [[nodiscard]] bool isComplete() const { return m_isComplete; }

  void restart();

 protected:
  virtual void updateDefault(float &dt);
  virtual void endAnimation();

 private:
  void createPlane();
  [[nodiscard]] sptr<MaterialInst> createMaterial() const;

 protected:
  std::function<void()> m_finishCallback;

  sptr<Mesh> m_object;
  sptr<MaterialInst> m_materialI;

  CommonTypes::SpritesContainerSetting m_infoSprite;

  float m_currentFrame;

  bool m_isComplete = false;
  bool m_disableOnEnd = true;
  bool m_isJpgMode = false;

 private:
  sptr<Node> m_view;
};

//====================================
// Spawnable wrapper for SpriteAnimation & LoopedSprite
//====================================
class SpriteAnimation_S : public SceneObject, public Updatable
{
public:

  SpriteAnimation_S(MapObjectsData& mod, sptr<Node> parentNode);
  sptr<SpriteAnimation> getStoredSprite() {return m_spriteAnimation;}
  void update(float dt) override;

  bool isLooped() const {return m_isLopped;}
  void restart() {if (m_spriteAnimation) m_spriteAnimation->restart();}

 //returns vector of all SpriteContainerSettings
 static std::vector<std::string> getLoadedSpriteAnims();

protected:

 void initSpawnable(sptr<Node> rootNode) override;

private:

 void onBeginPlay(sptr<Spawnable> selfRef, bool isTabSpawned, vec3 spawnLocation) override;
 void onEndPlay(sptr<Spawnable> selfRef, const std::string& objId) override;

private:

 bool m_isLopped = false;
 sptr<SpriteAnimation> m_spriteAnimation = nullptr;
};
