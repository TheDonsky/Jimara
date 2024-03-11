#pragma once
#include "../../Core/Function.h"
#include "../Math.h"
#include <vector>
#include <set>
#include <map>
#include <algorithm>
#include <optional>
#include <type_traits>


namespace Jimara {
	namespace Algorithms {
		/// <summary>
		/// A generic A* pathfinding algorithm implementation
		/// <para/> Distance can be any type, as long as it supports comparizons and has a 'zero' value
		/// </summary>
		/// <typeparam name="GraphNode"> Graph node index/pointer/representation (anything, that can be used as an unique key, really) </typeparam>
		/// <typeparam name="IterateNeighborsFn"> 
		/// A lambda function, that receives GraphNode and another lambda as an argument 
		/// and expects that lambda to be invoked with each (GraphNode neighborNode, Distance distanceToNeighbor) as arguments
		/// </typeparam>
		/// <typeparam name="HeuristicFn"> A callable, that receives GraphNode as an argument and returns heuristic minimal distance value </typeparam>
		/// <param name="start"> Start Node </param>
		/// <param name="end"> Destination/End node </param>
		/// <param name="heuristic"> Heuristic function </param>
		/// <param name="getNeighbors"> Callback for collecting neighbor information </param>
		/// <returns> A list of GraphNodes from start to end, if the path is found; empty vector otherwise </returns>
		template<typename GraphNode, typename HeuristicFn, typename IterateNeighborsFn>
		inline static std::vector<GraphNode> AStar(GraphNode start, GraphNode end, const HeuristicFn& heuristic, const IterateNeighborsFn& getNeighbors) {
			const auto startHeuristic = heuristic(start);
			using DistanceT = std::remove_reference_t<std::remove_const_t<decltype(startHeuristic)>>;

			struct NodePath {
				GraphNode nodeId = {};
				DistanceT heuristic = {};
				DistanceT distanceSoFar = {};
				inline constexpr DistanceT MinDistance()const { return (heuristic + distanceSoFar); }
				inline constexpr bool operator<(const NodePath& other) {
					const DistanceT minDist = MinDistance();
					const DistanceT otherMinDist = other.MinDistance();
					return minDist < otherMinDist || (minDist == otherMinDist && nodeId < other.nodeId);
				}
			};

			const constexpr size_t NoId = ~size_t(0u);
			struct NodeData {
				NodePath path;
				std::optional<GraphNode> prevNode;
				size_t firstNeighborId = NoId;
				size_t neighborCount = 0u;
			};

			std::set<NodePath> availablePaths;
			std::map<GraphNode, NodeData> nodeInfos;
			std::vector<std::pair<GraphNode, DistanceT>> neighborBuffer;

			{
				NodeData startInfo = {};
				startInfo.path.nodeId = start;
				startInfo.path.heuristic = Math::Max(startHeuristic, static_cast<DistanceT>(0));
				startInfo.path.distanceSoFar = static_cast<DistanceT>(0);
				startInfo.prevNode = std::optional<GraphNode>();
				startInfo.firstNeighborId = NoId;
				startInfo.neighborCount = 0u;
				availablePaths.insert(startInfo.path);
				nodeInfos[start] = startInfo;
			}

			auto getNeighbors = [&](GraphNode nodeId) {
				NodeData& nodeData = nodeInfos[nodeId];
				if (nodeData.firstNeighborId == NoId) {
					nodeData.firstNeighborId = neighborBuffer.size();
					nodeData.neighborCount = 0u;
					auto inspectNeighbor = [&](GraphNode neighbor, DistanceT distance) {
						neighborBuffer.push_back(std::make_pair(neighbor, Math::Max(distance, static_cast<DistanceT>(0))));
						nodeData.neighborCount++;
					};
					getNeighbors(nodeId, inspectNeighbor);
					assert(nodeData.neighborCount == (neighborBuffer.size() - nodeData.firstNeighborId));
				}
				return std::make_pair(nodeData.firstNeighborId, nodeData.firstNeighborId + nodeData.neighborCount);
			};

			auto updateNeighborPaths = [&](size_t neighborId, const NodePath& curPath) {
				const auto [neighbor, distance] = neighborBuffer[neighborId];
				auto dataIt = nodeInfos.find(neighbor);

				NodePath newPath = {};
				newPath.nodeId = neighbor;
				newPath.distanceSoFar = curPath.distanceSoFar + distance;
				if (dataIt == nodeInfos.end())
					newPath.heuristic = Math::Max(heuristic(neighbor), static_cast<DistanceT>(0));
				else newPath.heuristic = dataIt->second.path.heuristic;

				if (dataIt != nodeInfos.end() && dataIt->second.path.MinDistance() <= newPath.MinDistance())
					return;

				if (dataIt != nodeInfos.end())
					availablePaths.erase(dataIt->second.path);
				availablePaths.insert(newPath);

				if (dataIt != nodeInfos.end()) {
					dataIt->second.path = newPath;
					dataIt->second.prevNode = curPath.nodeId;
				}
				else {
					NodeData& data = nodeInfos[neighbor];
					data.path = newPath;
					data.prevNode = curPath.nodeId;
					data.firstNeighborId = NoId;
					data.neighborCount = 0u;
				}
			};

			auto getPath = [&]() -> std::vector<GraphNode> {
				std::vector<GraphNode> path;
				for (std::optional<GraphNode> node = end; node.has_value(); node = nodeInfos[node.value()].prevNode) {
					path.push_back(node.value());
					if (path.size() > nodeInfos.size()) {
						assert(false);
						return {};
					}
				}
				std::reverse(path.begin(), path.end());
				return path;
			};

			while (!availablePaths.empty()) {
				const NodePath curPath = *availablePaths.begin();
				availablePaths.erase(availablePaths.begin());
				
				if (curPath.nodeId == end)
					return getPath();
				
				const auto [firstNeighborId, lastNeighborId] = getNeighbors(curPath.nodeId);
				for (size_t neighborId = firstNeighborId; neighborId < lastNeighborId; neighborId++)
					updateNeighborPaths(neighborId, curPath);
			}

			return {};
		}
	}
}
