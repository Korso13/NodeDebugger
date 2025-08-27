#pragma once

#include "W4FrameworkMinimal.h"
#include "reqInc.hpp"
#include "common/Defines.h"

#define MS_MATERIAL_LIST materials::MaterialContainer::instance().getMaterialsList()

namespace materials
{
	template<typename VertexShader, typename FragmentShader>
	using MaterialPair = std::pair<VertexShader*, FragmentShader*>;

	template<typename VertexShader, typename FragmentShader>
	using MaterialStorage = std::unordered_map<std::string, MaterialPair<VertexShader, FragmentShader>>;

#define ADD_MATERIAL(container, namespace_name) \
        do { \
            using VertexShader = typename std::decay_t<decltype(namespace_name::m_vertexShader)>; \
            using FragmentShader = typename std::decay_t<decltype(namespace_name::m_fragmentShader)>; \
            container[#namespace_name] = std::make_pair(&namespace_name::m_vertexShader, &namespace_name::m_fragmentShader); \
        } while(0)


	class MaterialContainer
	{
	public:
		static MaterialContainer& instance()
		{
			static MaterialContainer instance;
			return instance;
		}

		void init()
		{
			m_isInitialize = true;

			ADD_MATERIAL(m_materialStorage, alpha_anim				);
			ADD_MATERIAL(m_materialStorage, animated				);
			ADD_MATERIAL(m_materialStorage, animated_mirror			);
			ADD_MATERIAL(m_materialStorage, animated_mirrored		);
			ADD_MATERIAL(m_materialStorage, alt_sdf_material		);
			ADD_MATERIAL(m_materialStorage, bump					);
			ADD_MATERIAL(m_materialStorage, defaultShader			);
			ADD_MATERIAL(m_materialStorage, digits					);
			ADD_MATERIAL(m_materialStorage, ocean_water				);
			ADD_MATERIAL(m_materialStorage, point_light_color		);
			ADD_MATERIAL(m_materialStorage, point_light_tex			);
			ADD_MATERIAL(m_materialStorage, point_light_tex_nm		);
			ADD_MATERIAL(m_materialStorage, progress_bar			);
			ADD_MATERIAL(m_materialStorage, progress_bar_radial		);
			ADD_MATERIAL(m_materialStorage, simple					);
			ADD_MATERIAL(m_materialStorage, sprite					);
			ADD_MATERIAL(m_materialStorage, sprite_mirrored			);
			ADD_MATERIAL(m_materialStorage, sprite_repeat			);
			ADD_MATERIAL(m_materialStorage, masked_sprite			);
			ADD_MATERIAL(m_materialStorage, unlit					);
		}

		template<typename VertexShader, typename FragmentShader>
		MaterialPair<VertexShader, FragmentShader> getMaterial(const std::string& materialName)
		{
			if (!m_isInitialize) init();

			auto it = m_materialStorage.find(materialName);
			if (it == m_materialStorage.end()) {
				err("no material {%s}", CSTR(materialName));
			}

			return it->second;
		}

		const std::map<std::string, std::string>& getMaterialsList()
		{
			if (m_materialsList.size() != m_materialStorage.size())
			{
				for (auto& [matName, matPair] : m_materialStorage)
					m_materialsList.insert({ matName, matName });
			}

			return m_materialsList;
		}

	private:
		MaterialStorage<decltype(materials::pattern::m_vertexShader), decltype(materials::pattern::m_fragmentShader)> m_materialStorage;
		std::map<std::string, std::string> m_materialsList;

		bool m_isInitialize = false;
	};
}