#pragma once

#include "W4Framework.h"
#include "common/Defines.h"
#include <any>
#include "SpawnerStructs.h"

W4_USE_UNSTRICT_INTERFACE

//forward decl
class SpawnerTab;

//struct MapObjectsData is defined in "common/nodeDebugger/SpawnerStructs.h"! via MOD_DEFINITION macro

class Spawnable
{
	friend SpawnerTab;

protected:
	//forward decl
	struct SpawnSetting;

	//Structs to hold spawnable child objects and meshes. todo: consider moving elsewhere
	struct SpawnableChild
	{
		SpawnableChild() : mod(SpawnableChild::m_defaultVal) {}; //for containers only!
		SpawnableChild(MapObjectsData& srcMod) : mod(srcMod) {};
		SpawnableChild(const SpawnableChild& src) : bSpawned(src.bSpawned), child(src.child), mod(src.mod) {};
		void operator=(const SpawnableChild& src) { bSpawned = src.bSpawned; child = src.child; mod = src.mod; };
		
		bool bSpawned = false;
		wptr<Spawnable> child;
		MapObjectsData& mod;

	private:
		inline static MapObjectsData m_defaultVal = MapObjectsData(); //for default constructor
	};
	struct SpawnableChildMesh
	{
		SpawnableChildMesh() : mmd(SpawnableChildMesh::m_defaultVal) {}; //for containers only!
		SpawnableChildMesh(MapMeshData& srcMmd) : mmd(srcMmd) {};
		SpawnableChildMesh(const SpawnableChildMesh& src) : bSpawned(src.bSpawned), child(src.child), mmd(src.mmd) {};
		void operator=(const SpawnableChildMesh& src) { bSpawned = src.bSpawned; child = src.child; mmd = src.mmd; };
		
		bool bSpawned = false;
		wptr<Mesh> child;
		MapMeshData& mmd;

	private:
		inline static MapMeshData m_defaultVal = MapMeshData(); //for default constructor
	};

private:

	//helper structs
	struct anyVar
	{
		friend SpawnSetting;

		template<typename T>
		void setValue(T val);

		template<typename T>
		void setValueByRef(T& val);

		template<typename T>
		T get() { return std::any_cast<T>(m_value); };

		template<typename T>
		T& getRef() { return std::any_cast<T&>(m_value); };

		bool hasValue() const { return m_value.has_value(); }

	private:
		std::any m_value;
		std::function<bool(const std::string&, anyVar&)> onGuiCb = nullptr;
		std::function<void(anyVar&)> refCb = nullptr;
	};


protected:

	//Struct containing information about exposed setting, callback for changing it and ImGui render for it
	struct SpawnSetting
	{
		SpawnSetting() = delete;

		//pass value by reference (will be updated automatically, no need to change in callback)
		template<typename T>
		explicit SpawnSetting(const std::string& name, T& value);
		//pass value by reference (will be updated automatically, no need to change in callback)
		template<typename T>
		explicit SpawnSetting(const std::string& name, T& value, std::function<void(std::decay_t<T>&)>&& onChangeCB);
		//pass value as copy
		template<typename T>
		explicit SpawnSetting(const std::string& name, T&& value, std::function<void(std::decay_t<T>&)>&& onChangeCB);

		//Factory method(use it in case you have issues with templated constructor)
		template<typename T>
		static SpawnSetting makeSpawnSetting(const std::string& name, std::decay_t<T> value, std::function<void(T&)>&& onChangeCB);

		std::string settingName = "setting_default_name";
		anyVar settingValue;
		std::function<void(anyVar)> onSettingChangeCB;

		std::function<bool(const std::string&, anyVar&)>& getOnGuiCb() { return settingValue.onGuiCb; }
		
	private:

		SpawnSetting(const std::string& name) : settingName(name) {};
	};

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

	std::vector<SpawnableChild>& getSpawnedChildren() { return m_children; };
	std::vector<SpawnableChildMesh>& getSpawnedChildMeshes() { return m_childMeshes; };
	sptr<Mesh> getChildMeshByName(const std::string& name) const;

private:

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

	//Used to render focus frame around Spawnable
	std::array<vec3, 2> getSpawnableBounds(sptr<Node> spawnableRoot);
	void removeFocusFrame();

	void initSpawnableCollider(std::function<void(const CollisionInfo&)> clickCb);
	std::string getNewMeshIdSuffix(const std::string& meshName);

	SpawnableChildMesh& getChildMeshHolder(const std::string& name);

public:

	bool isSpawned = false; //set to true if the object was spawned via Spawner Tab

protected:

	MapObjectsData m_mapObjectData;
	
	sptr<Node> m_spRoot = nullptr; //Spawnable object's root (used in OnGui())

private:

	inline static sptr<Plotter> m_framePlotter = nullptr;

	std::map<std::string, SpawnSetting> m_spawnableSettings; //Fields exposed to the Spawner Tab Gui
	std::map<std::string, std::function<void()>> m_spawnableBtns; //Btns exposed to the Spawner Tab Gui
	size_t m_rootNodeTransformSubsHandler = -1;

	bool m_childrenAdderEnabled = false;
	std::function<void(sptr<Spawnable>)> m_onChildSpawnCb = nullptr;

	std::vector<SpawnableChild> m_children;
	std::vector<SpawnableChildMesh> m_childMeshes;

	sptr<Collider> m_spawnableSelectCollider = nullptr;

	std::array<vec3, 2> m_cachedBounds;
	std::string m_lastSelectedChildMesh;
	std::string m_lastSelectedChildObj;

	std::string m_lastSelectedTextureForMesh = "";
	wptr<Node> m_lastSelectedParentNode;
};

#include "SpawnableImpl.inl"