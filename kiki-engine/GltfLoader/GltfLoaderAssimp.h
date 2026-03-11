#include <filesystem>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <glm/glm.hpp>
#include <iostream>

struct mesh {
	std::vector<glm::vec3> vertices;
	std::vector<glm::vec3> normals;
	std::vector<glm::vec2> uvs;
	std::vector<uint32_t> indices;
};

namespace Kiki {
	class GltfLoaderAssimp {
	public:
		static mesh loadMesh(const std::filesystem::path& path) {
			Assimp::Importer importer;
			const aiScene* scene = importer.ReadFile(path.string(), aiProcess_Triangulate | aiProcess_FlipUVs);
			if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
				throw std::runtime_error("Failed to load glTF file: " + path.string() + " with error: " + importer.GetErrorString());
			}
			mesh out{};
			aiMesh* mesh = scene->mMeshes[0]; // For simplicity, we only load the first mesh
			out.vertices.reserve(mesh->mNumVertices);
			out.normals.reserve(mesh->mNumVertices);
			out.uvs.reserve(mesh->mNumVertices);
			for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
				out.vertices.emplace_back(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
				if (mesh->HasNormals()) {
					out.normals.emplace_back(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
				}
				if (mesh->HasTextureCoords(0)) {
					out.uvs.emplace_back(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
				}
			}
			for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
				aiFace face = mesh->mFaces[i];
				for (unsigned int j = 0; j < face.mNumIndices; j++) {
					out.indices.push_back(face.mIndices[j]);
				}
			}
			return out;
		}
	};
}
