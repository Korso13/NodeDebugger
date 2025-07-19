#pragma once
#include "W4Framework.h"
#include "./InstanceInitiation.h"
#include "materialSystem/MaterialSystem.h"

W4_USE_UNSTRICT_INTERFACE

struct TextureInfo {
	sptr<w4::resources::Texture> texture;
	bool isLoaded = false;
};

class ResourceLibrary : public InstanceInitiation
{
public:
	static ResourceLibrary& instance()
	{
		static ResourceLibrary instance;
		return instance;
	}

	ResourceLibrary();
	virtual ~ResourceLibrary();


	bool isAllTexturesLoaded()
	{
		checkLoadedTextures();
		if (m_countLoadedTextures == m_texturesData.size()) return true;
		return false;
	}

	sptr<Asset> getModel(const std::string& key)
	{
		initCheck();
		if (m_modelsData.find(key) == m_modelsData.end()) {
			W4_LOG_ERROR("ResourceLibrary::getModel {%s} not found", std::to_string(key).c_str());
		}
		return m_modelsData[key];
	}
	sptr<Material> getMaterial(const std::string& key)
	{
//		initCheck();
//		if (m_materialsData.find(key) == m_materialsData.end()) {
//			W4_LOG_ERROR("ResourceLibrary::getMaterial {%s} not found", std::to_string(key).c_str());
//		}
//		return m_materialsData[key];

		return MaterialSystem::instance().getMaterial(key);
	}
	sptr<Texture> getTexture(const std::string& key)
	{
		initCheck();
		if (m_texturesData.find(key) == m_texturesData.end()) {
			W4_LOG_ERROR("ResourceLibrary::getTexture {%s} not found", std::to_string(key).c_str());
			return nullptr;
		}
		return m_texturesData[key]->texture;
		auto txt = m_texturesData.find(key);
	}

	std::string getModelPath(const std::string& key)
	{
		initCheck();
		if (m_modelsDataPath.find(key) == m_modelsDataPath.end()) {
			W4_LOG_ERROR("ResourceLibrary::getModelPath {%s} not found", std::to_string(key).c_str());
		}
		return m_modelsDataPath[key];
	}
	std::string getMaterialPath(const std::string& key)
	{
		initCheck();
		if (m_materialsDataPath.find(key) == m_materialsDataPath.end()) {
			W4_LOG_ERROR("ResourceLibrary::getMaterialPath {%s} not found", std::to_string(key).c_str());
		}
		return m_materialsDataPath[key];
	}
	std::string getTexturePath(const std::string& key)
	{
		initCheck();
		if (m_texturesDataPath.find(key) == m_texturesDataPath.end()) {
			W4_LOG_ERROR("ResourceLibrary::getTexturePath {%s} not found", std::to_string(key).c_str());
		}
		return m_texturesDataPath[key];
	}

	std::string getRemoteTexturePath(const std::string& key)
	{
		initCheck();

		if (m_remoteTexturesDataPath.find(key) != m_remoteTexturesDataPath.end()) {
			return m_remoteTexturesDataPath[key];
		}
		return "none";	
	}

	const std::unordered_map<std::string, sptr<TextureInfo>>& getLoadedTexturesInfo() const { return m_texturesData; }

private:
	virtual void loadConfig() override;

	void checkLoadedTextures()
	{
		for (auto& item : m_texturesData)
		{
			auto textureInfo = item.second;
			if (!textureInfo->isLoaded)
			{
				if (textureInfo->texture->getSize().has_value())
				{
					textureInfo->isLoaded = true;
					m_countLoadedTextures++;
				}
			}
		}
	}

private:
	std::unordered_map<std::string, std::string> m_modelsDataPath;
	std::unordered_map<std::string, std::string> m_materialsDataPath;
	std::unordered_map<std::string, std::string> m_texturesDataPath;
	std::unordered_map<std::string, std::string> m_remoteTexturesDataPath;

	std::unordered_map<std::string, sptr<Asset>> m_modelsData;
	std::unordered_map<std::string, sptr<Material>> m_materialsData;
	//std::unordered_map<std::string, sptr<Texture>> m_texturesData;
	std::unordered_map<std::string, sptr<TextureInfo>> m_texturesData;

	std::string m_configName = "resources.json";

	int m_countLoadedTextures = 0;

};
