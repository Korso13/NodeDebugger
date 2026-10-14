#include "Spawnable.h"
#include "../Concepts.h"

	//forward decl
public:
	struct SpawnSetting;

protected:
	//helper structs:

	//Structs to hold spawnable child objects and meshes. todo: consider moving elsewhere
	struct SpawnableChild
	{
		SpawnableChild() : mod(SpawnableChild::m_defaultVal) {}; //for containers only!
		SpawnableChild(MapObjectsData& srcMod) : mod(srcMod) {};
		SpawnableChild(const SpawnableChild& src) : bSpawned(src.bSpawned), child(src.child), mod(src.mod) {};
		void operator=(const SpawnableChild& src) { bSpawned = src.bSpawned; child = src.child; mod = src.mod; };

		bool bSpawned = true;
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

public:
	//Struct containing information about exposed setting, callback for changing it and ImGui render for it
	struct SpawnSetting
	{
		struct anyVar
		{
			friend SpawnSetting;

			template<typename T>
			void setValue(T* val);

			template<typename T>
			void setValueByCopy(T val);

			template<typename T>
			[[nodiscard]] T get();

			template<typename T>
			T* getRef();

			bool hasValue() const;

		private:
			bool m_isInited = false;
			bool m_isCopyValue = false;
			void* m_value = nullptr;
			std::any m_copyValue;
			std::function<bool(const std::string&, anyVar&)> onGuiCb = nullptr;
		};

		SpawnSetting() = delete;

		//pass value by pointer (will be updated automatically, no need to change in callback)
		template<typename T>
		requires AutoTypeConcept<T> || onGuiImplConcept<T>
		explicit SpawnSetting(const std::string& name, T& value, std::function<void(std::decay_t<T>&)>&& onChangeCB = nullptr);
		
		template<typename T>
		requires AutoTypeConcept<T> || onGuiImplConcept<T>
		explicit SpawnSetting(const std::string& name, T&& value, std::function<void(std::decay_t<T>)>&& onChangeCB);

		template<typename T>
		requires AutoTypeConcept<T> || onGuiImplConcept<T>
		explicit SpawnSetting(const std::string& name, const std::decay_t<T> value, std::function<void(std::decay_t<T>)>&& onChangeCB);

		std::string settingName = "setting_default_name";
		anyVar settingValue;
		std::function<void(anyVar&)> onSettingChangeCB;

		std::function<bool(const std::string&, anyVar&)>& getOnGuiCb() { return settingValue.onGuiCb; }
		void setSettingEnabledChecker(std::function<bool()>&& enabledCheckPredicate){ m_enableCheckCB = std::move(enabledCheckPredicate); }
		bool isEnabled() const
		{
			if(!m_enableCheckCB) return true;
			return m_enableCheckCB();
		};

	private:
		std::function<bool()> m_enableCheckCB;
		//SpawnSetting(const std::string& name) : settingName(name) {};
	};
