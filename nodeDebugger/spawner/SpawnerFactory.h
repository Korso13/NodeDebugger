#pragma once

#include "W4Framework.h"
#include <common/nodeDebugger/customGuiTabs/SpawnerTab.h>
#include "common/MeshesContainer.h"
#include "../CustomAutos.h"

#include "SpawnerFactoryMacro.inl"

#define SPAWNER SpawnerFactory::get()
#define SPAWNER_REC SpawnerFactory::get().getSpawnerRecord
#define SPAWNER_CLASS_MOD(className) SPAWNER_REC(className).getMapObjectsDataRef()
//Data container for Spawnables from mapObjects.json
#define MAP_DATA SpawnerFactory::get().getAllMapData().mapObjectsData
//Data container for Meshes from mapObjects.json
#define MAP_MESH_DATA SpawnerFactory::get().getAllMapData().mapMeshData

class SpawnerFactory : protected MeshesContainer, protected OnUpdateProvider
{
public:

	//struct holding all the vital things for Spawnable entity: spawner method, constructor parameter list, render for each parameter
	struct SpawnerRecord
	{
		friend SpawnerTab;
		friend SpawnerFactory;
		//todo: consider rewriting via void pointers as was done with Spawnable's SpawnSetting
		struct variantHolder
		{
			template<typename T>
			void setValue(T val) { m_value = val; }

			template<typename T>
			T get() { return std::any_cast<T>(m_value); };

			template<typename T>
			T& getRef() { return std::any_cast<T&>(m_value); };
		private:
			std::any m_value;
		};

		//small struct to hold Spawnable entity contructor parameter and its value (changable via ImGui)
		struct ParamPack
		{
			std::string paramName;
			variantHolder paramValue;
			bool isMapData = false;
			wptr<Node> node; //specialized holder for node
		};

		SpawnerRecord();
		explicit SpawnerRecord(const std::string& className);
		SpawnerRecord& operator=(const SpawnerRecord& rhv);

		//Sets callback executed right after spawning a Spawnable entity
		SpawnerRecord& setOnSpawnCallback(std::function<void(sptr<Spawnable>)>&& cb);
		std::function<void(sptr<Spawnable>)> getOnSpawnCallback() { return m_onSpawnCb; };

		std::vector<std::function<void()>>& getConstructorArgsUiRender() { return m_argsOnGui; }

		//Sets callback executed after entity removed from Spawnables list. 
		//Use it to remove all of Spawnable's sptr records to allow for deleting the object from memory
		SpawnerRecord& setOnDeleteCallback(std::function<void(sptr<Spawnable>)>&& cb);
		std::function<void(sptr<Spawnable>)> getOnDeleteCallback() { return m_onDeleteCb; };
		
		SpawnerRecord& setObjNameList(std::vector<std::string>&& listOfNames);
		SpawnerRecord& enableObjNameTextureSelector();

		//Exposes pointer to object spawn menu (for example a field from its MapObjectsData
		template<typename DataType>
		requires AutoTypeConcept<DataType> || onGuiImplConcept<DataType> //checks if DataType can be rendered in UI
		SpawnerRecord& exposeVariableToPaletteUi(const std::string& variableName, DataType* variable);
		//More reliable method with dynamic containers
		template<typename DataType>
		requires AutoTypeConcept<DataType> || onGuiImplConcept<DataType> //checks if DataType can be rendered in UI
		SpawnerRecord& exposeVariableToPaletteUi(const std::string& variableName, std::function<std::decay_t<DataType>*()> variablePtrGetter);
		
		//use it for adding shortcuts to MOD fiends in class palette!
		MapObjectsData& getMapObjectsDataRef() {return m_defaultMod;};
		//used in spawning
		const MapObjectsData getMapObjectsData() { return m_defaultMod; };
		//sets alternative MOD for class spawner (usually temporarily)
		void setNewMapObjectsData(MapObjectsData& newMOD);
		//sets alternative MOD for class spawner (usually temporarily)
		void resetToDefaultMapObjectsData();
		//setter/getter for Node used in class constructor
		sptr<Node> getConstructNode() const;
		void setNewConstructNode(sptr<Node> newNode);

	private:

		const std::string m_classID = "CLASS_NAME";
		MapObjectsData m_defaultMod;
		std::vector<ParamPack> constructorParams; //construction parameters for Spawnable entity
		std::vector<std::function<void()>> m_argsOnGui; //ImGui render for construction parameters editor
		std::map<std::string, std::string> m_listOfMODObjNames;

		//Func to be called on pressing "Spawn" button. Basically a factory for a specific subclass of Spawnable
		std::function<sptr<Spawnable>(std::vector<ParamPack>)> m_spawnerFunc = nullptr;
		//Called on spawning entity. Takes said entity as argument for any further actions with it
		std::function<void(sptr<Spawnable>)> m_onSpawnCb = nullptr;		
		//Called on deleting the entity from the Spawned entities list. 		
		std::function<void(sptr<Spawnable>)> m_onDeleteCb = nullptr;

		//Template method adding next construction parameter for Spawnable entity, as well as generating ImGui render function for it
		template<typename DataType>
		requires AutoTypeConcept<DataType> || onGuiImplConcept<DataType> //checks if DataType can be rendered in UI
		inline SpawnerRecord& addConstructorParam(const std::string& paramName, DataType defaultValue)
		{
			ParamPack newParam;
			newParam.paramName = paramName;
			newParam.paramValue.setValue<DataType>(defaultValue);
			constructorParams.push_back(newParam);
			int paramIndex = constructorParams.size() - 1;
#ifndef FORCENODEDEBUG_OFF
			m_argsOnGui.push_back([this, paramName, paramIndex]()
				{
					NDGui::Auto(paramName.c_str(), constructorParams[paramIndex].paramValue.getRef<DataType>());
				});
#endif
			return *this;
		};

		template<typename DataType>
		inline SpawnerRecord& addConstructorParam(const std::string& paramName, DataType defaultValue)
		{
			ParamPack newParam;
			newParam.paramName = paramName;
			newParam.paramValue.setValue<DataType>(defaultValue);
			constructorParams.push_back(newParam);
			int paramIndex = constructorParams.size() - 1;
			return *this;
		};
	};

	//=====================
	//Spawner factory code
	//=====================
public:

	static SpawnerFactory& get()
	{
		static SpawnerFactory instance;
		return instance;
	}

	//void SpawnObjects(const std::string& mapDataFileOverride = "");
	/// Loads level data into Spawner
	/// @param mapDataFileOverride - name of json with main data about level
	/// @param additionalLevelFiles - names of files with additional level contents. Will overwrite objects with matching objId or meshId! Latest file takes priority!
	void SpawnObjects(const std::string& mapDataFileOverride = "", std::vector<std::string> additionalLevelFiles = {});

	std::map<std::string, SpawnerRecord>& getClassRegistry() { return m_classesRegistry; }
	//Use it to show a combo list with classes available in Spawner. 
	//Returns class name on selecting a new one or selectedClass if nothing new was selected
	//todo: use this in Class palette tab instead of list of trees?
	std::string getClassesImGuiCombo(const std::string& selectedClass = "CLASS_NAME", const std::string& comboHeader = "Classes");

	SpawnerRecord& getSpawnerRecord(const std::string& className);
	//Method for getting object of class' standard onSpawn callback. Useful for instancing child objects
	std::function<void(sptr<Spawnable>)> getObjectOnSpawnCallback(sptr<Spawnable> spawnedObject);
	//Method for getting class' standard onSpawn callback. Useful for instancing child objects
	std::function<void(sptr<Spawnable>)> getClassOnSpawnCallback(const std::string& className);

	//registerSpawnableClass() is used to expose spawnable class in the Spawner Tab
	//className / entityName - the spawnable class and its name to be exposed in the Spawner Tab
	//used only in overloads via ADD_REGISTER_SPAWNABLE_FUNC_MACRO():
	//param1, param2... - parameters for constructor ()
	//phArg / typename PHT - placeholder arguement to avoid using separate macros for them
	template<typename className>
	SpawnerRecord& registerSpawnableClass(const std::string& entityName);

	//registerSpawnableClass overloads for 1+ arguements in the className constructor. Add more if needed
	ADD_REGISTER_SPAWNABLE_FUNC_MACRO(1)
	ADD_REGISTER_SPAWNABLE_FUNC_MACRO(2)
	ADD_REGISTER_SPAWNABLE_FUNC_MACRO(3)
	ADD_REGISTER_SPAWNABLE_FUNC_MACRO(4)
	ADD_REGISTER_SPAWNABLE_FUNC_MACRO(5)
	ADD_REGISTER_SPAWNABLE_FUNC_MACRO(6)
	ADD_REGISTER_SPAWNABLE_FUNC_MACRO(7)
	ADD_REGISTER_SPAWNABLE_FUNC_MACRO(8)
	ADD_REGISTER_SPAWNABLE_FUNC_MACRO(9)
	ADD_REGISTER_SPAWNABLE_FUNC_MACRO(10)
	ADD_REGISTER_SPAWNABLE_FUNC_MACRO(11)
	ADD_REGISTER_SPAWNABLE_FUNC_MACRO(12)
	ADD_REGISTER_SPAWNABLE_FUNC_MACRO(13)
	ADD_REGISTER_SPAWNABLE_FUNC_MACRO(14)
	ADD_REGISTER_SPAWNABLE_FUNC_MACRO(15)
	;

	//Spawn new object or mesh based on settings. 
	//Used by factory on SpawnObjects() and by SpawnerTab
	sptr<Mesh> spawnMesh(MapMeshData& meshData);
	sptr<Spawnable> spawnObjectOfClass(SpawnerRecord& spawnData, bool bTabSpawned = false, vec3 spawnAt = INVALIDPOS);
	//Spawns object if class with custom MapObjectsData and optional custom onSpawnCb (default onSpawn will be used if customOnSpawnCb == nullptr)
	sptr<Spawnable> spawnObjectOfClass(SpawnerRecord& spawnData, MapObjectsData& customMOD, std::function<void(sptr<Spawnable>)> customOnSpawnCb = nullptr, bool bTabSpawned = false);
	sptr<Spawnable> spawnCopyOfObject(MapObjectsData& objectsMOD, vec3 spawnAt = INVALIDPOS, bool bTabSpawned = false);
	const std::string& getLastSpawnedObjectName() const { return m_lastSpawnedObjectName; }
	void removeObject(const std::string& objId);
	void removeObjectByUid(size_t uid);

	//removes sptrs of spawned meshes/Spawnables
	void purgeSpawnedObjects();
	void purgeSpawnedMeshes();

	//returns struct with mapObjects.json data
	MapObjectsContainer& getAllMapData(const std::string& mapDataFileOverride = "",
																			bool forceReloadMapData = false,
																			bool appendMapData = false);

	//Spawned objects getter
	std::map<std::string, sptr<Spawnable>>& getAllSpawnables() { return m_spawnedEntities; }
	std::map<std::string, std::pair<MapMeshData, sptr<Mesh>>>& getAllMeshes() { return m_spawnedMeshes; }

	//Spawned mesh getters
	sptr<Mesh> getMeshByName(const std::string& meshName);
	//Returns vector of meshes containing meshNameSearch in their name
	std::vector<sptr<Mesh>> getAllMeshesMatching(const std::string& meshNameSearch);

	//returns Spawned object of class T(must be Spawnable) by its name (how it's written in Spawned Objects subtab)
	template<typename T> requires(std::is_base_of_v<Spawnable, T> || std::derived_from<T, Spawnable>)
	sptr<T> getObjectByName(const std::string& spawnableName);
	template<typename T> requires(std::is_base_of_v<Spawnable, T> || std::derived_from<T, Spawnable>)
	sptr<T> getObjectByUid(size_t uid);
	template<typename T> requires(std::is_base_of_v<Spawnable, T> || std::derived_from<T, Spawnable>)
	std::vector<sptr<T>> getAllObjectsOfClass(const std::string className);

	//Used by SpawnerTab::onGuiMeshPalette() to set root for newly spawned meshes
	sptr<Node>& getDefaultMeshesRoot() { return m_view; };
	void setDefaultMeshesRoot(sptr<Node> newRoot);
	//Returns meshesContainerSetting contents
	std::map<std::string, CommonTypes::MeshesContainerSetting>& getMeshesSettings();
	//Returns vector of meshesContainerSetting records (meshes' names)
	std::vector<std::string> getMeshesList();

	//prevents repeating names wrecking ImGui tree menus
	std::string checkUniqueName(const std::string& name);
	std::string checkUniqueMeshName(const std::string& name);
	// Used internally by Spawnable class. Do not call yourself
	void updateSpawnableNameRegistry(const std::string& oldName, const std::string& newName);

	//used before exporting mapObjects to json
	void updateMeshHierarchyRecords();

	//converts node's hierarchy place into string adress. Use as<Node>() to cast your sptr<Mesh> to Node
	std::string getNodeHierarchyAddress(sptr<Node> node); 
	//converts node's string adress into place in node hierarchy
	sptr<Node> findNodeByHierarchyAddress(std::string address);

protected:

	void onUpdate(float dt) override;

private:

	SpawnerFactory();

	void checkNodeHierarchyNames();

public:

	bool m_nodeHierarchySearchLogging = false;

private:

	MapObjectsContainer mapData;
	std::string m_mapDataFile = "mapObjects.json";
	sptr<class DelayedTimerInside> m_delay = nullptr;

	std::map<std::string, SpawnerRecord> m_classesRegistry;
	std::map<std::string, CommonTypes::MeshesContainerSetting> m_meshesList;

	std::map<std::string, sptr<Spawnable>> m_spawnedEntities;
	std::map<size_t, sptr<Spawnable>> m_spawnedEntitiesRegistry;
	std::map<std::string, std::pair<MapMeshData, sptr<Mesh>>> m_spawnedMeshes;

	bool m_primaryMeshLoadComplete = false;
	std::map<int, MapMeshData> m_delayedMeshSpawn;
	std::map<int, sptr<Spawnable>> m_delayedSpawnableParenting;

	std::string m_lastSpawnedObjectName = "";
};

//Template specializations
#include "SpawnerFactoryImpl.inl"