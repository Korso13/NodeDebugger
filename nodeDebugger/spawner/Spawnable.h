#pragma once

#include "W4Framework.h"
#include "common/Defines.h"
#include <any>
#include <type_traits>
#include "SpawnerStructs.h"
#include "common/nodeDebugger/Concepts.h"

//forward decl
class CollidersExt;
class SpawnerTab;
class SpawnerFactory;
class Spawnable;
W4_USE_UNSTRICT_INTERFACE

namespace SP_Utils
{
SHIPPING_STATIC void onGuiSingleExportBtns(Spawnable& obj);
}

// Logger macros for Spawnables
#define sp_debug(FORMAT, ...) W4_LOG_DEBUG((std::string("[%s][%s]") + std::string(FORMAT)).c_str(), __func__, getObjId().c_str(), ##__VA_ARGS__)
#define sp_info(FORMAT, ...) W4_LOG_INFO((std::string("[%s][%s]") + std::string(FORMAT)).c_str(), __func__, getObjId().c_str(), ##__VA_ARGS__)
#define sp_warn(FORMAT, ...) W4_LOG_WARNING((std::string("[%s][%s]") + std::string(FORMAT)).c_str(), __func__, getObjId().c_str(), ##__VA_ARGS__)
#define sp_err(FORMAT, ...) W4_LOG_ERROR((std::string("[%s][%s]") + std::string(FORMAT)).c_str(), __func__, getObjId().c_str(), ##__VA_ARGS__)

//struct MapObjectsData is defined in "common/nodeDebugger/SpawnerStructs.h"! via MOD_DEFINITION macro
class Spawnable
{
	friend SpawnerTab;
	friend SpawnerFactory;
	friend void SP_Utils::onGuiSingleExportBtns(Spawnable& obj);

protected:
	//moved helper structs into a separate file
#include "SpawnableHelpers.inl"

	//Spawnable interface
public:

	Spawnable() = delete;
	Spawnable(MapObjectsData& spawnData) : m_mapObjectData(spawnData), m_uid(utils::UIDGenerator::get()) {};
	virtual ~Spawnable();

	virtual sptr<Node> getSpawnableRoot() { return m_spRoot; };

	//overload in main class to call its respective update method (allows spawned entities to recieve updates)
	//Use only if you are not setting up updates for Spawnable via
	virtual void spawnableSelfUpdate(float dt) { };

	// Called by Spawner Tab when Spawnable object is selected.
	// Do not call yourself!
	virtual bool onGui(const char* label);

	//used to get data for exporting spawned entities
	//Always call Spawnable::getMapObjectsData() if your class can have children (it updates getMapObjectsData().childObjects!!)
	virtual MapObjectsData& getMapObjectsData();
	template<typename T>
	requires (std::is_same_v<T,bool> || std::is_same_v<T,int> || std::is_same_v<T,float> || std::is_same_v<T,std::string> || std::is_same_v<T,vec2> || std::is_same_v<T,vec3> || std::is_same_v<T,vec4>)
	std::optional<T> getSetting(const std::string& settingName) const;

	[[nodiscard]] size_t getUid() const { return m_uid; }
	const std::string& getObjId() const {return m_mapObjectData.objId;}

	std::vector<SpawnableChild>& getSpawnedChildren() {return m_children; };
	std::vector<SpawnableChildMesh>& getSpawnedChildMeshes() { return m_childMeshes; };
	sptr<Spawnable> getParent() const;
	bool hasParent() const {return m_mapObjectData.isChildObj && !m_parent.expired();}
	bool checkFlag(const std::string& flagName) { 
		if (m_mapObjectData.settings.flags.contains(flagName))
			return  m_mapObjectData.settings.flags[flagName];
		return false;
	}

protected:

	//call in your class' constructor and pass its root node (it will be copied and used for initial placement)
	//You need to override it if you want to expose variables of your spawned objects to SpawnerTab
	//If you override it, always call Spawnable::initSpawnable(). Use addSpawnableSetting to expose your class fields to the SpawnerTab
	virtual void initSpawnable(sptr<Node> rootNode);

	//Exposes class variable to the Spawner tab (Enums must be first serialized via AUTO_SERIALIZE_ENUM or W4_NLOHMANN_JSON_SERIALIZE_ENUM macro!).
	//SpawnSetting ctor arguments: 1) Variable name 2) Variable ref or value 3) Callback on value change
	//You can use returned SpawnSetting& to set predicate for easy enabling/disabling setting rendering via setSettingEnabledChecker() method
	SpawnSetting& addSpawnableSetting(SpawnSetting&& newSetting)
	{
		auto folderName = ND_Util::tryGetFolderName(newSetting.settingName);
		if (folderName)
		{
			if (m_spawnableSettings[*folderName].contains(newSetting.settingName))
				m_spawnableSettings[*folderName].erase(newSetting.settingName);
			auto result = m_spawnableSettings[*folderName].insert({ newSetting.settingName, std::move(newSetting)});
			return result.first->second; //return ref to freshly added setting
		}
		else
		{
			if (m_spawnableSettings[""].contains(newSetting.settingName))
				m_spawnableSettings[""].erase(newSetting.settingName);
			auto result = m_spawnableSettings[""].insert({ newSetting.settingName, std::move(newSetting)});
			return result.first->second; //return ref to freshly added setting
		}
	}

	void removeSpawnableSetting(const std::string& settingName)
	{
		auto editableCopy = settingName;
		auto folderName = ND_Util::tryGetFolderName(editableCopy);
		if (folderName)
		{
			m_spawnableSettings[*folderName].erase(editableCopy);
			if (m_spawnableSettings[*folderName].empty())
				m_spawnableSettings.erase(*folderName);
		}
		else
			m_spawnableSettings[""].erase(editableCopy);
	}

	//Exposes the value as a drop-down list with values provided by the listGetter param
	//listGetter - func that returns unordered_map with available items (value) paired with their name (key)
	template<typename T>
	requires AutoTypeConcept<T> || onGuiImplConcept<T>
	SpawnSetting& addSpawnableSettingList(
		const std::string& name,
		T& value,
		std::function<std::map<std::string, std::decay_t<T>>()>&& listGetter,
		std::function<void(std::decay_t<T>&)>&& onChangeCB = nullptr);

	// Special case of addSpawnableSettingList that automatically lists all child meshes and returns selected meshId in onChangeCB
	SpawnSetting& addChildMeshSelectorSetting(
		const std::string& name,
		std::string& value,
		std::function<void(std::string&)>&& onChangeCB = nullptr);

	// Special case of addSpawnableSettingList that automatically lists all child objects of classes in classNameFilters
	// Returns selected child objId in onChangeCB
	SpawnSetting& addChildSelectorSetting(
		const std::string& name,
		std::string& value,
		std::vector<std::string> classNameFilters,
		std::function<void(std::string&)>&& onChangeCB = nullptr);

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
	//Better to call it from onBeginPlay (otherwise children may fail to find proper node) or at least after initSpawnable!
	void spawnAllChildren();

	sptr<Mesh> getChildMeshByName(const std::string& name) const;
	sptr<Spawnable> getChildByName(const std::string& name) const;

	//Called immediately after constructor
	//Override to add class specific init behavior (better than using onSpawn callback)
	//selfRef - sptr to this object (do not save inside class!), isTabSpawned - is true when object is spawned from Spawner tab, spawnLocation - world coord of click or camera with zero z, if tabSpawned (if not - uses .pos from MOD)
	virtual void onBeginPlay(sptr<Spawnable> selfRef, bool isTabSpawned, vec3 spawnLocation = INVALIDPOS){};

	//Called immediately on ordering Spawnable deletion from level (Better option than OnDeleteCallback)
	//selfRef - sptr to this object (do not save inside class!!), objId - name, under which the object is registered in Spawner (normally == objId dield in the object's MapObjectsData)
	virtual void onEndPlay(sptr<Spawnable> selfRef, const std::string& objId) {};

	//recursively visits all descendants (child objects) of this object
	void childrenVisiter(const std::function<void(Spawnable&)>& visiterFunction);
	void childrenVisiter(const std::function<void(SpawnableChildMesh&)>& visiterFunction);

	// Used to render focus frame around Spawnable.
	// Returns coords for approximate bounding volume around all childs of m_spRoot (except ParticleEmitter, meshes with 'target' texture, disabled VisualNodes). Spines' bounding volume data is unreliable! :(
	std::array<vec3, 2> getSpawnableBounds(sptr<Node> spawnableRoot, bool forceUpdateBounds = false);
	std::array<vec3, 2> getOtherSpawnableBounds(sptr<Spawnable> spawnable, bool forceUpdateBounds = false);
	void renewSpawnableBounds(){m_cachedBounds = std::array<w4::math::vec3, 2>();}

	bool hasChildMesh(const std::string& name) const;
	bool hasChildObj(const std::string& name) const;

	//required to call before export - updates data relevant to children export
	//override it in case you need some specific logic on exporting to JSON only (don't forget to call Spawnable's impl too!)
	virtual void updateChildrenData();

	// Called when Spawnable gains focus (from SpawnableTab). Call default impl when overriding!
	virtual void onSelected();
	// Called when Spawnable looses focus (from SpawnableTab). Call default impl when overriding!
	virtual void onDeselected();

private:

	//Renders root transforms and parenting ui + any exposed SpawnableSettings
	void SpawnableMainUi();
	//renders UI of Spawnable children interface
	void childSpawnerUI();
	//renders UI of Mesh children interface
	void childMeshSpawnerUI();
	//spawns Spawnable child and registers it where necessary
	sptr<Spawnable> spawnChild(sptr<MapObjectsData> childData, SpawnableChild& childHolder, bool bTabSpawned = false);
	//spawns Mesh child and registers it where necessary
	sptr<Mesh> spawnChildMesh(sptr<MapMeshData> childData, SpawnableChildMesh& childHolder);


	void initSpawnableCollider(std::function<void(const CollisionInfo&)> clickCb);
	void updateSpawnableCollider(const std::array<vec3, 2>& bounds);
	std::string getNewMeshIdSuffix(const std::string& meshName);

	SpawnableChildMesh& getChildMeshHolder(const std::string& name);

public:

	bool isSpawned = false; //set to true if the object was spawned via Spawner Tab

protected:

	MapObjectsData m_mapObjectData;
	
	sptr<Node> m_spRoot = nullptr; //Spawnable object's root (used in OnGui())

private:

	inline static sptr<Plotter> m_framePlotter = nullptr; //red rectangle around selected object

	const size_t m_uid;

	//std::map<std::string, SpawnSetting> m_spawnableSettings; //Fields exposed to the Spawner Tab Gui (rendered in Spawnable::onGui)
	std::map<std::string, std::map<std::string, SpawnSetting>> m_spawnableSettings; //Fields exposed to the Spawner Tab Gui (rendered in Spawnable::onGui)
	std::map<std::string, std::function<void()>> m_spawnableBtns; //Btns exposed to the Spawner Tab Gui (rendered in Spawnable::onGui)
	size_t m_rootNodeTransformSubsHandler = -1;

	bool m_childrenAdderEnabled = false;
	bool m_childrenInited = false;
	bool m_childObjectsRenderPaused = false; //required to avoid crashes on deleting children
	std::function<void(sptr<Spawnable>)> m_onChildSpawnCb = nullptr;

	wptr<Spawnable> m_self;
	wptr<Spawnable> m_parent;
	std::vector<SpawnableChild> m_children;
	std::vector<SpawnableChildMesh> m_childMeshes;

	//sptr<Collider> m_spawnableSelectCollider = nullptr;
	sptr<CollidersExt> m_spawnableSelectCollider = nullptr;
	size_t m_colliderCbHandlerId;
	size_t m_colliderRayCbHandlerId;
	bool m_colliderPriorityResetRaycast = false;

	std::array<vec3, 2> m_cachedBounds;
	std::string m_lastSelectedChildMesh = "";
	std::string m_lastSelectedChildObj;

	std::string m_lastSelectedTextureForMesh = "";
	wptr<Node> m_lastSelectedParentNode;

	std::function<void()> m_deferredDeleteOrder = nullptr;
	std::function<void()> m_deferredSpawnChildrenCall = nullptr;
};

//implementations of template functions
#include "SpawnableImpl.inl"