#pragma once

#include <filesystem>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <glm/glm.hpp>
#include <iostream>
#include <stb_image.h>
#include <Components/ColourComponent.hpp>

#define ASSIMP_FLAGS aiProcess_Triangulate | aiProcess_JoinIdenticalVertices | aiProcess_Triangulate


struct Mmesh {
	std::vector<glm::vec3> vertices;
	std::vector<glm::vec3> normals;
	std::vector<glm::vec2> uvs;
	std::vector<uint32_t> indices;
	int matIndex;
};

struct RGBA {
	uint8_t r, g, b, a;
};
struct Mtexture {
	bool hastexture = true;
	std::vector<RGBA> data;
	std::vector<uint8_t> rawData; // For compressed textures
	stbi_uc* rawDataPtr;
	stbi_uc* roughness;
	int width;
	int height;
	int channels;
	std::string name;
	bool hasTransparency() const {
		if (channels < 4) {
			return false;
		}
		for (size_t i = 0; i < data.size(); i ++) {
			if (data[i].a < 255) {
				return true;
			}
		}
		return false;
	}
};

namespace Kiki {
	class GltfLoaderAssimp {
	public:
		static Mmesh loadMesh(const std::filesystem::path& path, int index) {
			Assimp::Importer importer;
			const aiScene* scene = importer.ReadFile(path.string(), ASSIMP_FLAGS);
			if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
				throw std::runtime_error("Failed to load glTF file: " + path.string() + " with error: " + importer.GetErrorString());
			}

			Mmesh out{};
			aiMesh* mesh = scene->mMeshes[index]; // For simplicity, we only load the first mesh
			out.vertices.reserve(mesh->mNumVertices);
			out.normals.reserve(mesh->mNumVertices);
			out.uvs.reserve(mesh->mNumVertices);
			out.matIndex = mesh->mMaterialIndex;
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

		static Mtexture loadTexture(const std::filesystem::path& path, int index) {
			Assimp::Importer importer;
			const aiScene* scene = importer.ReadFile(path.string(), ASSIMP_FLAGS);
			if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
				throw std::runtime_error("Failed to load glTF file: " + path.string() + " with error: " + importer.GetErrorString());
			}
			if (scene->HasTextures() == false) return Mtexture(false);

			const aiMaterial* mat = scene->mMaterials[index];
			aiString textureName;// = scene->mTextures[0]; // For simplicity, we only load the first texture
			aiString roughName;// = scene->mTextures[1]; // For simplicity, we only load the first texture
			mat->GetTexture(AI_MATKEY_BASE_COLOR_TEXTURE, &textureName);
			mat->GetTexture(AI_MATKEY_ROUGHNESS_TEXTURE, &roughName);

			std::cout << textureName.C_Str() << " ~ " << roughName.C_Str() << std::endl;

			int i = std::stoi(textureName.C_Str() + 1);
			int j = std::stoi(roughName.C_Str() + 1);
			const aiTexture* texture = scene->mTextures[i];
			const aiTexture* rough = scene->mTextures[j];
			
			stbi_set_flip_vertically_on_load(1);
			Mtexture out{};
			out.name = texture->mFilename.C_Str();
			out.data.reserve(texture->mWidth * texture->mHeight);
			out.rawDataPtr = stbi_load_from_memory(reinterpret_cast<const stbi_uc*>(texture->pcData), texture->mWidth, &out.width, &out.height, &out.channels, 4);
			out.roughness = stbi_load_from_memory(reinterpret_cast<const stbi_uc*>(rough->pcData), rough->mWidth, &out.width, &out.height, &out.channels, 4);
			// if (texture->mHeight == 0) {
			// 	// Compressed texture
			// 	const uint8_t* raw = reinterpret_cast<const uint8_t*>(texture->pcData);
			// 	out.rawData.assign(raw, raw + texture->mWidth);
			// 	out.width = 0;
			// 	out.height = 0;
			// 	return out;
			// }
			// for (unsigned int i = 0; i < texture->mWidth * texture->mHeight; i++) {
			// 	aiTexel texel = texture->pcData[i];
			// 	out.data.emplace_back(texel.r, texel.g, texel.b, texel.a);
			// }
			// out.width = texture->mWidth;
			// out.height = texture->mHeight;
			// out.channels = 4; // Assimp always loads textures as RGBA
			return out;
		}
		


		static void debugPrintMesh(const Mmesh& mesh) {
			// std::cout << "Vertices: " << mesh.vertices.size() << std::endl;
			// for (const auto& vertex : mesh.vertices) {
			// 	std::cout << "  " << vertex.x << ", " << vertex.y << ", " << vertex.z << std::endl;
			// }
			// std::cout << "Normals: " << mesh.normals.size() << std::endl;
			// for (const auto& normal : mesh.normals) {
			// 	std::cout << "  " << normal.x << ", " << normal.y << ", " << normal.z << std::endl;
			// }
			// std::cout << "UVs: " << mesh.uvs.size() << std::endl;
			// for (const auto& uv : mesh.uvs) {
			// 	std::cout << "  " << uv.x << ", " << uv.y << std::endl;
			// }
			// std::cout << "Indices: " << mesh.indices.size() << std::endl;
			// for (const auto& index : mesh.indices) {
			// 	std::cout << "  " << index << std::endl;
			// }

			// std::cout << "UVs: " << mesh.uvs.size() << std::endl;
			// for (const auto& uv : mesh.uvs) {
			// 	std::cout << uv << std::endl;
			// }
		}

		static void debugPrintTexture(const Mtexture& texture) {
			std::cout << "Texture Name: " << texture.name << std::endl;
			std::cout << "Width: " << texture.width << std::endl;
			std::cout << "Height: " << texture.height << std::endl;
			std::cout << "Channels: " << texture.channels << std::endl;
			std::cout << "Has Transparency: " << (texture.hasTransparency() ? "Yes" : "No") << std::endl;
		}
	};
}
