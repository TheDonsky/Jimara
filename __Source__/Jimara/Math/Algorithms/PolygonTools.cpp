#include "PolygonTools.h"
#include <set>
#include "../../Core/Collections/Stacktor.h"


namespace Jimara {
	struct PolygonTools::Triangulator {
		enum class NodeType {
			UNDEFINED,
			TOP, // ->-
			BOTTOM, // -<-
			WALL, // |
			LEFT_CORNER, // <=
			RIGHT_CORNER, // =>
			LEFT_CUT,   // =<
			RIGHT_CUT // >=
		};

		static const constexpr size_t NO_ID = ~size_t(0u);

		struct Node {
			Vector2 position = {};
			NodeType type = NodeType::UNDEFINED;
			size_t nextVert = NO_ID;
		};

		struct Edge {
			size_t startIndex = NO_ID;
			size_t endIndex = NO_ID;
			Vector2 origin = {};
			float tilt = 0.0f;

			inline bool operator<(const Edge& other)const {
				const float time = (other.origin.x - origin.x);
				const float delta = (time > 0.0f)
					? ((origin.y + tilt * time) - other.origin.y)
					: (origin.y - (other.origin.y - other.tilt * time));
				if (delta < 0.0f)
					return true;
				else if (delta > 0.0f)
					return false;
				else if (tilt < other.tilt)
					return true;
				else if (tilt > other.tilt)
					return false;
				else if (startIndex < other.startIndex)
					return true;
				else return (startIndex == other.startIndex && endIndex < other.endIndex);
			}
		};

		using NodeConnections = Stacktor<size_t, 5u>;

		struct SubPolygonList {
			const size_t* vertexIndices = nullptr;
			const size_t* polygonSizes = nullptr;
			size_t polygonCount = 0u;
		};

		inline static constexpr float Cross(const Vector2& a, const Vector2& b) { return (a.x * b.y - a.y * b.x); }
		

		inline static Node* GetNodeBuffer(const Vector2* vertices, size_t vertexCount) {
			static thread_local std::vector<Triangulator::Node> nodes;
			if (nodes.size() < vertexCount)
				nodes.resize(vertexCount);

			const Vector2* vert = vertices;
			Node* ptr = nodes.data();
			Node* const end = ptr + vertexCount;
			while (ptr < end) {
				(*ptr) = Node{
					*vertices,
					NodeType::UNDEFINED,
					NO_ID };
				ptr++;
				vertices++;
			}

			return nodes.data();
		}

		inline static void DeriveNodeTypes(Node* nodes, size_t vertexCount) {
			for (size_t i = 0; i < vertexCount; i++) {
				Node& cur = nodes[i];
				Vector2 prevDelta = nodes[(i + vertexCount - 1u) % vertexCount].position - cur.position;
				Vector2 nextDelta = nodes[(i + 1u) % vertexCount].position - cur.position;
				if (std::abs(prevDelta.x) <= std::numeric_limits<float>::epsilon() &&
					std::abs(nextDelta.x) <= std::numeric_limits<float>::epsilon())
					cur.type = NodeType::WALL;
				else if (prevDelta.x >= 0.0f && nextDelta.x >= 0.0f) {
					if (Cross(prevDelta, nextDelta) > 0.0f)
						cur.type = NodeType::LEFT_CORNER;
					else cur.type = NodeType::LEFT_CUT;
				}
				else if (prevDelta.x <= 0.0f && nextDelta.x <= 0.0f) {
					if (Cross(prevDelta, nextDelta) > 0.0f)
						cur.type = NodeType::RIGHT_CORNER;
					else cur.type = NodeType::RIGHT_CUT;
				}
				else if ((nextDelta.x - prevDelta.x) > 0.0f)
					cur.type = NodeType::TOP;
				else cur.type = NodeType::BOTTOM;
			}
		}

		inline static const size_t* SortVertices(Node* nodes, size_t vertexCount) {
			static thread_local std::vector<size_t> orderedIndexes;
			std::vector<size_t>& order = orderedIndexes;
			if (order.size() < vertexCount)
				order.resize(vertexCount);
			for (size_t i = 0u; i < vertexCount; i++)
				order[i] = i;
			std::sort(order.data(), order.data() + vertexCount, [&](const size_t& i, const size_t& j) {
				const Vector2 a = nodes[i].position;
				const Vector2 b = nodes[j].position;
				if (a.x < b.x)
					return true;
				else if (a.x > b.x)
					return false;
				else return (a.y < b.y);
				});
			return order.data();
		}

		inline static bool CreateIndirectionCuts(Node* nodes, size_t vertexCount) {
			const size_t* sortedOrder = SortVertices(nodes, vertexCount);
			
			using EdgeSet = std::set<Edge>;
			static thread_local EdgeSet verticalEdgeSet;
			EdgeSet& verticalEdges = verticalEdgeSet;
			verticalEdges.clear();
			
			using StartPointList = Stacktor<size_t, 4u>;
			static thread_local std::vector<StartPointList> startPoints;
			if (startPoints.size() < vertexCount)
				startPoints.resize(vertexCount);
			for (size_t i = 0u; i < vertexCount; i++)
				startPoints[i].Clear();

			auto getEdge = [&](size_t startIndex, size_t endIndex) {
				Node node = nodes[startIndex];
				Vector2 endPos = nodes[endIndex].position;
				float deltaX = (endPos.x - node.position.x);
				Edge rv;
				if (deltaX <= 0.0f) {
					rv.startIndex = rv.endIndex = NO_ID;
					return rv;
				}
				rv.startIndex = startIndex;
				rv.endIndex = endIndex;
				rv.origin = node.position;
				rv.tilt = (endPos.y - node.position.y) / deltaX;
				return rv;
			};

			auto findClosestEdges = [&](Vector2 position) -> EdgeSet::const_iterator {
				Edge sampleEdge;
				{
					sampleEdge.startIndex = sampleEdge.endIndex = NO_ID;
					sampleEdge.origin = position;
					sampleEdge.tilt = 0.0f;
				}
				const EdgeSet::const_iterator ptr = verticalEdges.lower_bound(sampleEdge);
				return (ptr == verticalEdges.begin()) ? verticalEdges.end() : ptr;
			};

			auto removeEdge = [&](size_t startIndex, size_t endIndex) {
				Edge edge = getEdge(startIndex, endIndex);
				if (edge.startIndex == startIndex)
					verticalEdges.erase(edge);
			};
			
			auto addEdge = [&](size_t startIndex, size_t endIndex) {
				Edge edge = getEdge(startIndex, endIndex);
				if (edge.startIndex == startIndex) {
					verticalEdges.insert(edge);
					startPoints[endIndex].Push(startIndex);
				}
			};
			
			auto removeEdges = [&](size_t endIndex) {
				StartPointList& starts = startPoints[endIndex];
				for (size_t i = 0u; i < starts.Size(); i++)
					removeEdge(starts[i], endIndex);
				starts.Clear();
			};
			
			for (size_t i = 0; i < vertexCount; i++) {
				size_t index = sortedOrder[i];
				size_t prevIndex = (index + vertexCount - 1u) % vertexCount;
				size_t nextIndex = (index + 1) % vertexCount;
				removeEdges(index);
				Node& node = nodes[index];
				switch (node.type) {
				case NodeType::UNDEFINED:
					break;
				case NodeType::TOP:
					addEdge(index, nextIndex);
					break;
				case NodeType::BOTTOM:
					addEdge(index, prevIndex);
					break;
				case NodeType::WALL:
					break;
				case NodeType::LEFT_CORNER:
					addEdge(index, prevIndex);
					addEdge(index, nextIndex);
					break;
				case NodeType::RIGHT_CORNER:
					break;
				case NodeType::LEFT_CUT:
				{
					const EdgeSet::const_iterator top = findClosestEdges(node.position);
					if (top != verticalEdges.end()) {
						EdgeSet::const_iterator bottom = top;
						bottom--;
						node.nextVert = (top->origin.x > bottom->origin.x)
							? top->startIndex : bottom->startIndex;
						Node nextNode = nodes[node.nextVert];
						if ((nextNode.type == NodeType::RIGHT_CUT) &&
							(nextNode.nextVert < 0 || nodes[nextNode.nextVert].position.x > node.position.x)) {
							if (nextNode.nextVert < vertexCount)
								removeEdge(node.nextVert, nextNode.nextVert);
							nextNode.nextVert = index;
							nodes[node.nextVert] = nextNode;
						}
					}
					else return false;
					addEdge(index, prevIndex);
					addEdge(index, nextIndex);
				}
				break;
				case NodeType::RIGHT_CUT:
				{
					const EdgeSet::const_iterator top = findClosestEdges(node.position);
					if (top != verticalEdges.end()) {
						EdgeSet::const_iterator bottom = top;
						bottom--;
						node.nextVert = (nodes[top->endIndex].position.x < nodes[bottom->endIndex].position.x)
							? top->endIndex : bottom->endIndex;
						addEdge(index, node.nextVert);
					}
					else return false;
				}
				break;
				default:
					assert(false);
				}
			}

			return true;
		}

		inline static NodeConnections* GetConnections(const Node* nodes, size_t vertexCount) {
			static thread_local std::vector<NodeConnections> connections;
			if (connections.size() < vertexCount)
				connections.resize(vertexCount);
			for (size_t i = 0u; i < vertexCount; i++)
				connections[i].Clear();
			auto addEdge = [&](size_t a, size_t b) {
				auto add = [](NodeConnections& list, size_t v) {
					// List size can not exceed 5, so linear lookup is OK
					for (size_t i = 0u; i < list.Size(); i++)
						if (list[i] == v)
							return;
					list.Push(v);
				};
				add(connections[a], b);
				add(connections[b], a);
			};
			for (size_t i = 0; i < vertexCount; i++) {
				addEdge(i, (i + vertexCount - 1) % vertexCount);
				addEdge(i, (i + 1) % vertexCount);
				size_t link = nodes[i].nextVert;
				if (link < vertexCount)
					addEdge(i, link);
			}
			return connections.data();
		}

		static SubPolygonList ExtractMonotonePolygons(const Node* nodes, size_t vertexCount) {
			static thread_local Stacktor<size_t> vertexIndices;
			static thread_local Stacktor<size_t> polygonSizes;
			vertexIndices.Clear();
			polygonSizes.Clear();

			static thread_local std::vector<bool> edgeUsed;
			if (edgeUsed.size() < vertexCount)
				edgeUsed.resize(vertexCount);
			for (size_t i = 0u; i < vertexCount; i++)
				edgeUsed[i] = false;

			NodeConnections* connections = GetConnections(nodes, vertexCount);

			for (size_t startVert = 0u; startVert < vertexCount; startVert++) {
				if (edgeUsed[startVert])
					continue;

				size_t prevVert;
				size_t curVert = startVert;

				const size_t initialVertexCount = vertexIndices.Size();
				auto addEdge = [&](size_t endPoint) {
					if (endPoint == ((curVert + 1) % vertexCount))
						edgeUsed[curVert] = true;
					vertexIndices.Push(endPoint);
					prevVert = curVert;
					curVert = endPoint;
				};
				auto polygonSize = [&]() { 
					return vertexIndices.Size() - initialVertexCount; 
				};
				auto clearPolygon = [&]() {
					vertexIndices.Resize(initialVertexCount);
				};
				addEdge((curVert + 1) % vertexCount);

				while (curVert != startVert) {
					Vector2 direction = Math::Normalize(nodes[curVert].position - nodes[prevVert].position);
					float bestDirMatch = -std::numeric_limits<float>::infinity();
					size_t bestIndex = NO_ID;
					NodeConnections& edges = connections[curVert];
					for (size_t i = 0; i < edges.Size(); i++) {
						size_t candidate = edges[i];
						if (candidate == prevVert)
							continue;
						Vector2 dir = Math::Normalize(nodes[candidate].position - nodes[curVert].position);
						float sin = Cross(direction, dir);
						float cos = Math::Dot(direction, dir);
						float signSin = (sin >= 0.0f) ? 1.0f : -1.0f;
						float match = (1 - signSin) + cos * signSin;
						if (match > bestDirMatch) {
							bestDirMatch = match;
							bestIndex = candidate;
						}
					}
					if (bestIndex >= vertexCount) {
						clearPolygon();
						break;
					}
					addEdge(bestIndex);
					if (polygonSize() > vertexCount) {
						clearPolygon();
						break;
					}
				}
				if (polygonSize() >= 3)
					polygonSizes.Push(polygonSize());
				else clearPolygon();
			}

			SubPolygonList rv;
			rv.vertexIndices = vertexIndices.Data();
			rv.polygonSizes = polygonSizes.Data();
			rv.polygonCount = polygonSizes.Size();
			return rv;
		}

		template<typename ReportTriangleFn>
		inline static void TriangulateMonotonePolygon(const Node* nodes, const size_t* poly, size_t polySize, const ReportTriangleFn& reportTriangle) {
			if (polySize < 3)
				return;
			auto node = [&](size_t index) {
				return nodes[poly[index]].position;
			};
			auto addTriangle = [&](size_t a, size_t b, size_t c) {
				if (std::abs(Cross(node(b) - node(a), node(c) - node(a))) < std::numeric_limits<float>::epsilon())
					return;
				reportTriangle(poly[a], poly[b], poly[c]);
			};
			float minX = std::numeric_limits<float>::infinity();
			size_t startIndex = NO_ID;
			for (size_t i = 0u; i < polySize; i++) {
				float x = node(i).x;
				if (minX <= x) continue;
				minX = x;
				startIndex = i;
			}
			static thread_local std::vector<size_t> stack;
			size_t startPtr = startIndex;
			size_t topPtr = ((startIndex + 1u) % polySize);
			size_t bottomPtr = ((startIndex + polySize - 1u) % polySize);
			while (topPtr != bottomPtr) {
				stack.clear();
				stack.push_back(startPtr);
				if (node(topPtr).x > node(bottomPtr).x) {
					while (topPtr != bottomPtr) {
						size_t ptr = (bottomPtr + polySize - 1) % polySize;
						if (node(ptr).x >= node(topPtr).x) {
							size_t prev = bottomPtr;
							while (stack.size() > 0) {
								size_t a = stack.back();
								stack.pop_back();
								addTriangle(a, topPtr, prev);
								prev = a;
							}
							startPtr = bottomPtr;
							bottomPtr = ptr;
							break;
						}
						else {
							stack.push_back(bottomPtr);
							bottomPtr = ptr;
							while (stack.size() >= 2) {
								size_t a = stack.back();
								stack.pop_back();
								size_t b = stack.back();
								if (Cross(node(b) - node(a), node(a) - node(ptr)) < 0.0f) {
									stack.push_back(a);
									break;
								}
								else addTriangle(a, b, ptr);
							}
						}
					}
				}
				else while (topPtr != bottomPtr) {
					size_t ptr = (topPtr + 1u) % polySize;
					if (node(ptr).x > node(bottomPtr).x) {
						size_t prev = topPtr;
						while (stack.size() > 0) {
							size_t a = stack.back();
							stack.pop_back();
							addTriangle(a, prev, bottomPtr);
							prev = a;
						}
						startPtr = topPtr;
						topPtr = ptr;
						break;
					}
					else {
						stack.push_back(topPtr);
						topPtr = ptr;
						while (stack.size() >= 2) {
							size_t a = stack.back();
							stack.pop_back();
							size_t b = stack.back();
							if (Cross(node(b) - node(a), node(a) - node(ptr)) > 0.0f) {
								stack.push_back(a);
								break;
							}
							else addTriangle(b, a, ptr);
						}
					}
				}
			}
		}
	};

	float PolygonTools::SignedArea(const Vector2* vertices, size_t vertexCount) {
		Vector2 prev = vertices[vertexCount - 1u];
		const Vector2* const end = vertices + vertexCount;
		float sum = 0.0f;
		while (vertices < end) {
			const Vector2 cur = *vertices;
			vertices++;
			sum += (cur.x - prev.x) * (cur.y + prev.y);
			prev = cur;
		}
		return sum * 0.5f;
	}

	void PolygonTools::Triangulate(const Vector2* vertices, size_t vertexCount, const Callback<size_t, size_t, size_t>& reportTriangle) {
		if (vertexCount <= 2u)
			return;
		
		Triangulator::Node* nodes = Triangulator::GetNodeBuffer(vertices, vertexCount);
		const bool isClockwise = IsClockwise(vertices, vertexCount);
		if (!isClockwise)
			std::reverse(nodes, nodes + vertexCount);
		Triangulator::DeriveNodeTypes(nodes, vertexCount);

		if (!Triangulator::CreateIndirectionCuts(nodes, vertexCount))
			return;

		const Triangulator::SubPolygonList monotonePolys = Triangulator::ExtractMonotonePolygons(nodes, vertexCount);
		const size_t* poly = monotonePolys.vertexIndices;
		for (size_t i = 0u; i < monotonePolys.polygonCount; i++) {
			const size_t polySize = monotonePolys.polygonSizes[i];
			if (isClockwise)
				Triangulator::TriangulateMonotonePolygon(nodes, poly, polySize, reportTriangle);
			else Triangulator::TriangulateMonotonePolygon(nodes, poly, polySize, [&](size_t a, size_t b, size_t c) {
				auto fixIndex = [&](size_t index) { return (vertexCount - index - 1u); };
				reportTriangle(fixIndex(a), fixIndex(b), fixIndex(c));
				});
			poly += polySize;
		}
	}

	std::vector<size_t> PolygonTools::Triangulate(const Vector2* vertices, size_t vertexCount) {
		std::vector<size_t> result;
		auto addTri = [&](size_t a, size_t b, size_t c) {
			result.push_back(a);
			result.push_back(b);
			result.push_back(c);
		};
		Triangulate(vertices, vertexCount, Callback<size_t, size_t, size_t>::FromCall(&addTri));
		return result;
	}
}
