#pragma once

#include "W4Framework.h"
#include <common/nodeDebugger/customGuiTabs/SpawnerTab.h>
#include "common/MeshesContainer.h"

#include "SpawnerFactoryImpl.inl"

#define SPAWNER SpawnerFactory::get()
//Data container for Spawnables from mapObjects.json
#define MAP_DATA SpawnerFactory::get().getAllMapData().mapObjectsData
//Data container for Meshes from mapObjects.json
#define MAP_MESH_DATA SpawnerFactory::get().getAllMapData().mapMeshData

class SpawnerFactory : protected MeshesContainer
{
public:

	//struct holding all the vital things for Spawnable entity: spawner method, constructor parameter list, render for each parameter
	struct SpawnerRecord
	{
		friend SpawnerTab;
		friend SpawnerFactory;
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
		};

		//Sets callback executed right after spawning a Spawnable entity
		SpawnerRecord& setOnSpawnCallback(std::function<void(sptr<Spawnable>)>&& cb);

		//Sets callback executed after entity removed from Spawnables list. 
		//Use it to remove all of Spawnable's sptr records to allow for deleting the object from memory
		SpawnerRecord& setOnDeleteCallback(std::function<void(sptr<Spawnable>)>&& cb);
		
		SpawnerRecord& setObjNameList(std::vector<std::string>&& listOfNames);

	private:

		std::vector<ParamPack> constructorParams; //construction parameters for Spawnable entity
		std::vector<std::function<void()>> m_argsOnGui; //ImGui render for construction parameters editor
		std::map<std::string, std::string> m_listOfMODObjNames;

		//Func to be called on pressing "Spawn" button. Basically a factory for a specific subclass of Spawnable
		std::function<sptr<Spawnable>(std::vector<ParamPack>)> m_spawnerFunc = nullptr;
		//Called on spawning entity. Takes said entity as arguement for any further actions with it
		std::function<void(sptr<Spawnable>)> m_onSpawnCb = nullptr;		
		//Called on deleting the entity from the Spawned entities list. 		
		std::function<void(sptr<Spawnable>)> m_onDeleteCb = nullptr;

		//Template method adding next construction parameter for Spawnable entity, as well as generating ImGui render function for it
		template<typename DataType>
		inline SpawnerRecord& addConstructorParam(const std::string& paramName, DataType defaultValue)
		{
			ParamPack newParam;
			newParam.paramName = paramName;
			newParam.paramValue.setValue<DataType>(defaultValue);
			constructorParams.push_back(newParam);
			int paramIndex = constructorParams.size() - 1;
#ifndef FORCENODEDEBUG_OFF
			//TODO: NEEDS CHECK IF WE CAN RENDER DataType via Auto (just in case)
			m_argsOnGui.push_back([this, paramIndex]()
				{
					ImGui::Auto(constructorParams[paramIndex].paramName.c_str(), constructorParams[paramIndex].paramValue.getRef<DataType>());
				});
#endif
			return *this;
		};
	};

public:

	static SpawnerFactory& get()
	{
		static SpawnerFactory instance;
		return instance;
	}

	void SpawnObjects();

	std::map<std::string, SpawnerRecord>& getClassRegistry() { return m_classesRegistry; }

	SpawnerRecord& getSpawnerRecord(const std::string& className);
	void setDefaultMeshesRoot(sptr<Node> newRoot);

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
	sptr<Spawnable> spawnObjectOfClass(SpawnerRecord& spawnData, bool bTabSpawned = false);

	//removes sptrs of spawned meshes/Spawnables
	void purgeSpawnedObjects();
	void purgeSpawnedMeshes();

	//returns struct with mapObjects.json data
	MapObjectsContainer& getAllMapData();

	//Spawned objects getter
	std::map<std::string, sptr<Spawnable>>& getAllSpawnables() { return m_spawnedEntities; }
	std::map<std::string, std::pair<MapMeshData, sptr<Mesh>>>& getAllMeshes() { return m_spawnedMeshes; }

	//Spawned meshes getters
	sptr<Mesh> getMeshByName(const std::string& meshName);
	std::vector<sptr<Mesh>> getAllMeshesMatching(const std::string& meshName);

	//Used by SpawnerTab::onGuiMeshPalette() to set root for newly spawned meshes
	sptr<Node>& getDefaultMeshesRoot() { return m_view; };
	std::map<std::string, CommonTypes::MeshesContainerSetting>& getMeshesList();

	//prevents repeating names wrecking ImGui tree menus
	std::string checkUniqueName(const std::string& name);
	std::string checkUniqueMeshName(const std::string& name);

	//used before exporting meshes
	void updateMeshHierarchyRecords();

	//converts node's hierarchy place into string adress. Use as<Node>() to cast your sptr<Mesh> to Node
	std::string getNodeHierarchyAddress(sptr<Node> node); 
	//converts node's string adress into place in node hierarchy
	sptr<Node> findNodeByHierarchyAddress(std::string adress); 

private:

	SpawnerFactory() = default;

	void checkNodeHierarchyNames();

private:

	MapObjectsContainer mapData;

	std::map<std::string, SpawnerRecord> m_classesRegistry;
	std::map<std::string, CommonTypes::MeshesContainerSetting> m_meshesList;

	std::map<std::string, sptr<Spawnable>> m_spawnedEntities;
	std::map<std::string, std::pair<MapMeshData, sptr<Mesh>>> m_spawnedMeshes;

	bool m_primaryMeshLoadComplete = false;
	std::map<int, MapMeshData> m_delayedMeshSpawn;
	std::map<int, sptr<Spawnable>> m_delayedSpawnableParenting;
};


//SpawnerRecord's addConstructorParam() specialization for sptr<Node>, which utilizes ImGuiNodeHierarchy for picking the node
template<>
inline SpawnerFactory::SpawnerRecord& SpawnerFactory::SpawnerRecord::addConstructorParam<sptr<Node>>(const std::string& paramName, sptr<Node> defaultNode)
{
	ParamPack newParam;
	newParam.paramName = paramName;
	newParam.paramValue.setValue<sptr<Node>>(defaultNode);
	constructorParams.push_back(newParam);
	int paramIndex = constructorParams.size() - 1;

#ifndef FORCENODEDEBUG_OFF
	m_argsOnGui.push_back([this, paramIndex]()
		{
			ND_Util::ImGuiNodeHierarchy(constructorParams[paramIndex].paramValue.getRef<sptr<Node>>(), constructorParams[paramIndex].paramName);
		});
#endif
	return *this;
}

//SpawnerRecord's addConstructorParam() specialization for MapObjectsData
template<>
inline SpawnerFactory::SpawnerRecord& SpawnerFactory::SpawnerRecord::addConstructorParam<MapObjectsData&>(const std::string& paramName, MapObjectsData& defaultMOD)
{
	ParamPack newParam;
	newParam.paramName = paramName;
	newParam.paramValue.setValue<MapObjectsData&>(defaultMOD);
	newParam.isMapData = true;
	constructorParams.push_back(newParam);
	int paramIndex = constructorParams.size() - 1;

#ifndef FORCENODEDEBUG_OFF
	m_argsOnGui.push_back([this, paramIndex]()
		{
			ImGui::Auto(constructorParams[paramIndex].paramName.c_str(), constructorParams[paramIndex].paramValue.getRef<MapObjectsData>());
		});
#endif
	return *this;
}