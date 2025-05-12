#include "W4Framework.h"
#include "common/Defines.h"
#include <settings/JsonDeserializator.h>

W4_USE_UNSTRICT_INTERFACE

struct plottedPath
{
	std::string pathName = "NewPath";
	std::string startPoint = "";
	std::string endPoint = "";
	std::vector<std::string> midPoints;
};

struct pathNode
{
	vec2 pos;
	float speed = 1.f;
};

struct PathsData
{
	std::map<std::string, pathNode> pathnodes;
	std::vector<plottedPath> paths;
};

namespace nlohmann {

	template<>
	struct adl_serializer<plottedPath>
	{
		static void from_json(const json& j, plottedPath& obj)
		{
			FIELD_DESERIALIZE(pathName);
			FIELD_DESERIALIZE(startPoint);
			FIELD_DESERIALIZE(endPoint);
			FIELD_DESERIALIZE(midPoints);
		}

		static void to_json(json& j, const plottedPath& obj)
		{
			FIELD_SERIALIZE(pathName);
			FIELD_SERIALIZE(startPoint);
			FIELD_SERIALIZE(endPoint);
			FIELD_SERIALIZE(midPoints);
		}
	};

	template<>
	struct adl_serializer<pathNode>
	{
		static void from_json(const json& j, pathNode& obj)
		{
			FIELD_DESERIALIZE(pos);
			FIELD_DESERIALIZE(speed);
		}

		static void to_json(json& j, const pathNode& obj)
		{
			FIELD_SERIALIZE(pos);
			FIELD_SERIALIZE(speed);
		}
	};

	template<>
	struct adl_serializer<PathsData>
	{
		static void from_json(const json& j, PathsData& obj)
		{
			FIELD_DESERIALIZE(pathnodes);
			FIELD_DESERIALIZE(paths);
		}

		static void to_json(json& j, const PathsData& obj)
		{
			FIELD_SERIALIZE(pathnodes);
			FIELD_SERIALIZE(paths);
		}
	};
}

#ifdef NDPATHDRAWER
#include <game/level/Pathfinder/Pathfinder.h>

static vec3 vec2toVec3(const vec2& invec) { return vec3(invec.x, invec.y, 0.f); }

static void loadPathsIntoPathfinder(
	sptr<Pathfinder> pathfinder, 
	std::map<std::string, std::function<void(std::string)>> onPointCallbacks = std::map<std::string, std::function<void(std::string)>>(),
	const std::string pathToConfig = "config/pathsContainerSettings.json",
	bool generateAjacencyMatrix = true)
{
	PathsData pathsData;
	auto file = w4::filesystem::open(pathToConfig);
	auto json = nlohmann::json::parse(file->data(), file->data() + file->size());
	nlohmann::adl_serializer<PathsData>::from_json(json, pathsData);
	W4_LOG_DEBUG("Loaded %s! Found {%d} path nodes and {%d} paths", pathToConfig.c_str(), pathsData.pathnodes.size(), pathsData.paths.size());

	//generating path node objects (not json structs!)
	std::map<std::string, sptr<PathNode>> pathNodes;
	for (auto& [nodeName, nodeData] : pathsData.pathnodes)
	{
		auto newNode = make::sptr<PathNode>(
			vec2toVec3(nodeData.pos),	//Path node position
			nodeData.speed,				//Path node speed
			nodeName,					//Path node name
			(onPointCallbacks.contains(nodeName) ? onPointCallbacks[nodeName] : nullptr) //Path node callback if there is one
		);
		pathNodes.insert({ nodeName , newNode});
	}

	for (auto& pathData : pathsData.paths)
	{
		if (!pathNodes.contains(pathData.startPoint) )
		{
			W4_LOG_ERROR("Node %s for Path %s not found! Path not created!", pathData.startPoint.c_str(), pathData.pathName.c_str());
			continue;
		}
		if (!pathNodes.contains(pathData.endPoint))
		{
			W4_LOG_ERROR("Node %s for Path %s not found! Path not created!", pathData.endPoint.c_str(), pathData.pathName.c_str());
			continue;
		}

		auto path = pathfinder->createPath(pathNodes[pathData.startPoint], pathNodes[pathData.endPoint]);
		
		//adding mid points if path has them
		if (!pathData.midPoints.empty())
		{
			for (auto& midPoint : pathData.midPoints)
			{
				path.addPath(vec2toVec3(pathsData.pathnodes[midPoint].pos), pathsData.pathnodes[midPoint].speed);
			}
		}
	}

	if(generateAjacencyMatrix)
		pathfinder->updateAdjacencyMatrix();
}
#endif