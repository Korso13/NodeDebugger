#pragma once

#include "W4Framework.h"
#include "common/Defines.h"
#include <any>
#include <type_traits>
#include "SpawnerStructs.h"

W4_USE_UNSTRICT_INTERFACE

//forward decl
class SpawnerTab;
class SpawnerFactory;

//template concepts
template<typename T>
concept AutoTypeConcept = requires(T & _value)
{
#ifdef DEBUG
	ImGui::Auto("ConceptTest", _value);
#endif
};
template<typename T>
concept onGuiImplConcept = requires(T & _value)
{
#ifdef DEBUG
	_value.onGui("ConceptTest");
#endif
};

//struct MapObjectsData is defined in "common/nodeDebugger/SpawnerStructs.h"! via MOD_DEFINITION macro

class Spawnable
{
	friend SpawnerTab;
	friend SpawnerFactory;

protected:
	//moved helper structs into a separate file
#include "SpawnableHelpers.inl"

	//Spawnable interface
public:

	Spawnable() = delete;
	Spawnable(MapObjectsData& spawnData) : m_mapObjectData(spawnData) {};
	virtual ~Spawnable();

	virtual sptr<Node> getSpawnableRoot() { return m_spRoot; };

	//overload in main class to call its respective update method (allows spawned entities to recieve updates)
	//Use only if your are not setting up updates for Spawnable via 
	virtual void spawnableSelfUpdate(float dt) { };

	//Used by Spawner Tab. Do not call youreself!
	virtual bool onGui(const char* label);

	//used to get data for exporting spawned entities
	//Always call Spawnable::getMapObjectsData() if your class can have children (it updates getMapObjectsData().childObjects!!)
	virtual MapObjectsData& getMapObjectsData();

protected:

	//call in your class' constructor and pass its root node (it will be copied and used for initial placement)
	//You need to override it if you want to expose variables of your spawned objects to SpawnerTab
	//If you override it, always call Spawnable::initSpawnable(). Use addSpawnableSetting to expose your class fields to the SpawnerTab
	virtual void initSpawnable(sptr<Node> rootNode);

	//Exposes class variable to the Spawner tab (Enums must be first serialized via AUTO_SERIALIZE_ENUM or W4_NLOHMANN_JSON_SERIALIZE_ENUM macro!). 
	//Use SpawnSetting::makeSpawnSetting to generate a proper SpawnSetting!!
	//makeSpawnSetting arguements: 1) Variable name 2) Variable ref or value 3) Callback on value change
	void addSpawnableSetting(SpawnSetting&& newSetting)
	{
		m_spawnableSettings.insert({ newSetting.settingName, std::move(newSetting)});
	}	
	void removeSpawnableSetting(const std::string& settingName)
	{
		m_spawnableSettings.erase(settingName);
	}

	//Exposes method of the spawned object to its spawner tab
	void addSpawnableBtn(const std::string& btnName, std::function<void()>&& btnCall)
	{
		m_spawnableBtns[btnName] = std::move(btnCall);
	}
	void removeSpawnableBtn(const std::string& btnName)
	{
		m_spawnableBtns.erase(btnName);
	}

	//Adds UI to Spawnable settings to add and spawn children. 
	//If onChildSpawnCb is not set, regular Spawner m_onSpawnCb is used. If you want to use both - call regular Spawner's m_onSpawnCb inside onChildSpawnCb
	void addChildSpawner(std::function<void(sptr<Spawnable>)> onChildSpawnCb = nullptr, bool childrenSpawnerOn = true);
	//Spawns all children found in getMapObjectsData().childObjects (should be used on loading object from json)
	//Call after you set onChildSpawnCb via addChildSpawner(if you need such)!
	//Call only once!
	void spawnAllChildren();

	std::vector<SpawnableChild>& getSpawnedChildren() {return m_children; };
	std::vector<SpawnableChildMesh>& getSpawnedChildMeshes() { return m_childMeshes; };
	sptr<Mesh> getChildMeshByName(const std::string& name) const;

private:

	//Called immediately after constructor
	//Override to add class specific init behavior (better than using onSpawn callback)
	//selfRef - sptr to this object (do not save inside class!), isTabSpawned - is true when object is spawned from Spawner tab, spawnLocation - world coord of click or camera with zero z, if tabSpawned (if not - uses .pos from MOD)
	virtual void onBeginPlay(sptr<Spawnable> selfRef, bool isTabSpawned, vec3 spawnLocation = INVALIDPOS) {};
	
	//Called immediately on ordering Spawnable deletion from level (Better option than OnDeleteCallback)
	//selfRef - sptr to this object (do not save inside class!!), objId - name, under which the object is registered in Spawner (normally == objId dield in the object's MapObjectsData)
	virtual void onEndPlay(sptr<Spawnable> selfRef, const std::string& objId) {};

	//Renders root transforms and parenting ui + any exposed SpawnableSettings
	void SpawnableMainUi();
	//renders UI of Spawnable children interface
	void childSpawnerUI();
	//renders UI of Mesh children interface
	void childMeshSpawnerUI();
	//spawns Spawnable child and registers it where nessesary
	sptr<Spawnable> spawnChild(sptr<MapObjectsData> childData, SpawnableChild& childHolder, bool bTabSpawned = false);
	//spawns Mesh child and registers it where nessesary
	sptr<Mesh> spawnChildMesh(sptr<MapMeshData> childData, SpawnableChildMesh& childHolder);
	
	//recursively visits all descendants (child objects) of this object
	void childrenVisiter(const std::function<void(Spawnable&)>& visiterFunction);
	void childrenVisiter(const std::function<void(SpawnableChildMesh&)>& visiterFunction);

	// Used to render focus frame around Spawnable. 
	// Returns coords for approximate bounding volume around all childs of m_spRoot (except ParticleEmitter, meshes with 'target' texture, disabled VisualNodes). Spines' bounding volume data is unreliable! :(
	std::array<vec3, 2> getSpawnableBounds(sptr<Node> spawnableRoot);
	void removeFocusFrame();

	void initSpawnableCollider(std::function<void(const CollisionInfo&)> clickCb);
	std::string getNewMeshIdSuffix(const std::string& meshName);
	void updateChildrensData(); //required to call before export

	SpawnableChildMesh& getChildMeshHolder(const std::string& name);

public:

	bool isSpawned = false; //set to true if the object was spawned via Spawner Tab

protected:

	MapObjectsData m_mapObjectData;
	
	sptr<Node> m_spRoot = nullptr; //Spawnable object's root (used in OnGui())

private:

	inline static sptr<Plotter> m_framePlotter = nullptr; //red rectangle around selected object

	std::map<std::string, SpawnSetting> m_spawnableSettings; //Fields exposed to the Spawner Tab Gui (rendered in Spawnable::onGui)
	std::map<std::string, std::function<void()>> m_spawnableBtns; //Btns exposed to the Spawner Tab Gui (rendered in Spawnable::onGui)
	size_t m_rootNodeTransformSubsHandler = -1;

	bool m_childrenAdderEnabled = false;
	bool m_childObjectsRenderPaused = false; //required to avoid crashes on deleting children
	std::function<void(sptr<Spawnable>)> m_onChildSpawnCb = nullptr;

	std::vector<SpawnableChild> m_children;
	std::vector<SpawnableChildMesh> m_childMeshes;

	sptr<Collider> m_spawnableSelectCollider = nullptr;

	std::array<vec3, 2> m_cachedBounds;
	std::string m_lastSelectedChildMesh;
	std::string m_lastSelectedChildObj;

	std::string m_lastSelectedTextureForMesh = "";
	wptr<Node> m_lastSelectedParentNode;

	std::function<void()> m_deferredDeleteOrder = nullptr;
};

//implementations of template functions
#include "SpawnableImpl.inl"