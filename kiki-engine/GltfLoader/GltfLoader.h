//#include <fastgltf/core.hpp>
//#include <fastgltf/types.hpp>
//#include <fastgltf/tools.hpp>
//#include <fastgltf/glm_element_traits.hpp>
//
//#include <iostream>
//#include <glm/glm.hpp>
//
//struct mesh {
//	std::vector<glm::vec3> vertices;
//	std::vector<glm::vec3> normals;
//	std::vector<glm::vec2> uvs;
//	std::vector<uint32_t> indices;
//};
//
//namespace Kiki {
//	class GltfLoader {
//	public:
//		static mesh loadMesh(const std::filesystem::path& path) {
//			fastgltf::Parser parser;
//			std::cout << std::filesystem::current_path() << std::endl;
//			auto data = fastgltf::GltfDataBuffer::FromPath(path);
//			if (data.error() != fastgltf::Error::None) {
//				throw std::runtime_error("Failed to load glTF file: " + path.string());
//			}
//
//			auto asset = parser.loadGltf(data.get(), path.parent_path(), fastgltf::Options::None);
//			if (auto error = asset.error(); error != fastgltf::Error::None) {
//				throw std::runtime_error("Failed to parse glTF file: " + path.string() + " with error code: " + std::to_string(static_cast<std::uint64_t>(asset)));
//			}
//
//			const auto& primitive = asset->meshes[0].primitives[0];
//
//			mesh out{};
//
//			auto findAccessor = [&](std::string_view name) -> const fastgltf::Accessor* {
//				auto it = primitive.findAttribute(name);
//				if( it == primitive.attributes.end()) {
//					return nullptr;
//				}
//				return &asset->accessors[it->accessorIndex];
//			};
//
//			const fastgltf::Accessor* positionAccessor = findAccessor("POSITION");
//			const fastgltf::Accessor* normalAccessor = findAccessor("NORMAL");
//			const fastgltf::Accessor* uvAccessor = findAccessor("TEXCOORD_0");
//
//			if (!positionAccessor) {
//				throw std::runtime_error("Mesh is missing POSITION attribute");
//			}
//
//			out.vertices.resize(positionAccessor->count);
//			fastgltf::copyFromAccessor<glm::vec3>(asset.get(), *positionAccessor, out.vertices.data());
//
//			if (normalAccessor) {
//				out.normals.resize(normalAccessor->count);
//				fastgltf::copyFromAccessor<glm::vec3>(asset.get(), *normalAccessor, out.normals.data());
//			}
//
//			if (uvAccessor) {
//				out.uvs.resize(uvAccessor->count);
//				fastgltf::copyFromAccessor<glm::vec2>(asset.get(), *uvAccessor, out.uvs.data());
//			}
//
//			if(primitive.indicesAccessor.has_value()) {
//				const auto& indicesAccessor = asset->accessors[*primitive.indicesAccessor];
//				out.indices.resize(indicesAccessor.count);
//				fastgltf::copyFromAccessor<uint32_t>(asset.get(), indicesAccessor, out.indices.data());
//			}else{
//				// If there are no indices, we need to generate them ourselves. We can assume that the vertices are ordered in a way that they can be rendered as triangles.
//				out.indices.resize(positionAccessor->count);
//				for (uint32_t i = 0; i < positionAccessor->count; ++i) {
//					out.indices[i] = i;
//				}
//				
//			}
//			return out;
//		}
//
//		static void debugPrintMesh(const mesh& mesh) {
//			std::cout << "Vertices: " << mesh.vertices.size() << std::endl;
//			for (const auto& vertex : mesh.vertices) {
//				std::cout << "  " << vertex.x << ", " << vertex.y << ", " << vertex.z << std::endl;
//			}
//			std::cout << "Normals: " << mesh.normals.size() << std::endl;
//			for (const auto& normal : mesh.normals) {
//				std::cout << "  " << normal.x << ", " << normal.y << ", " << normal.z << std::endl;
//			}
//			std::cout << "UVs: " << mesh.uvs.size() << std::endl;
//			for (const auto& uv : mesh.uvs) {
//				std::cout << "  " << uv.x << ", " << uv.y << std::endl;
//			}
//			std::cout << "Indices: " << mesh.indices.size() << std::endl;
//			for (const auto& index : mesh.indices) {
//				std::cout << "  " << index << std::endl;
//			}
//		}
//		
//	};
//}

