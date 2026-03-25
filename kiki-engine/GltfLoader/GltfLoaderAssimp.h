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
	stbi_uc* rawDataPtr = nullptr;
	stbi_uc* roughness = nullptr;
	int width = 0;
	int height = 0;
	int channels = 0;
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

	~Mtexture() {
		if (rawDataPtr) {
			stbi_image_free(rawDataPtr);
		}
		if (roughness) {
			stbi_image_free(roughness);
		}
	}

	Mtexture() = default;

	Mtexture(const Mtexture&) = delete;
	Mtexture& operator=(const Mtexture&) = delete;

	Mtexture(Mtexture&& other) noexcept
		: hastexture(other.hastexture),
		rawDataPtr(other.rawDataPtr),
		roughness(other.roughness),
		width(other.width),
		height(other.height),
		channels(other.channels),
		name(std::move(other.name))
	{
		other.rawDataPtr = nullptr;
		other.roughness = nullptr;
	}
	explicit Mtexture(bool hasTex)
		: hastexture(hasTex) {
	}
	Mtexture& operator=(Mtexture&& other) noexcept {
		if (this != &other) {
			if (rawDataPtr) stbi_image_free(rawDataPtr);
			if (roughness) stbi_image_free(roughness);

			hastexture = other.hastexture;
			rawDataPtr = other.rawDataPtr;
			roughness = other.roughness;
			width = other.width;
			height = other.height;
			channels = other.channels;
			name = std::move(other.name);

			other.rawDataPtr = nullptr;
			other.roughness = nullptr;
		}
		return *this;
	}
};

struct Mscene {
	std::vector<Mmesh> meshes;
	std::vector<Mtexture> textures;
	glm::mat4 worldTransform;
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

			std::cout << textureName.C_Str() << "~" << roughName.C_Str() << std::endl;

			
			stbi_set_flip_vertically_on_load(1);
			Mtexture out{};
			//out.name = texture->mFilename.C_Str();
			//out.data.reserve(texture->mWidth * texture->mHeight);
			if (textureName.length > 0) {
				int i = std::stoi(textureName.C_Str() + 1);
				const aiTexture* texture = scene->mTextures[i];
				out.rawDataPtr = stbi_load_from_memory(reinterpret_cast<const stbi_uc*>(texture->pcData), texture->mWidth, &out.width, &out.height, &out.channels, 4);
			} else {
				out.rawDataPtr = stbi_load((std::filesystem::path(PROJECT_ASSETS_PATH) / "empty.png").string().c_str(), &out.width, &out.height, &out.channels, 4);
			}

			if (roughName.length > 0) {
				int j = std::stoi(roughName.C_Str() + 1);
				const aiTexture* rough = scene->mTextures[j];
				out.roughness = stbi_load_from_memory(reinterpret_cast<const stbi_uc*>(rough->pcData), rough->mWidth, &out.width, &out.height, &out.channels, 4);

			} else {
				out.roughness = stbi_load((std::filesystem::path(PROJECT_ASSETS_PATH) / "empty.png").string().c_str(), &out.width, &out.height, &out.channels, 4);
			}
			
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
		

		static Mscene loadScene(const std::filesystem::path& path){
			Assimp::Importer importer;
			const aiScene* scene = importer.ReadFile(path.string(), ASSIMP_FLAGS);
			if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
				throw std::runtime_error("Failed to load glTF file: " + path.string() + " with error: " + importer.GetErrorString());
			}
			Mscene out{};
			auto& t = scene->mRootNode->mTransformation;
			out.worldTransform = glm::mat4(
				t.a1, t.b1, t.c1, t.d1,
				t.a2, t.b2, t.c2, t.d2,
				t.a3, t.b3, t.c3, t.d3,
				t.a4, t.b4, t.c4, t.d4
			);

			for (unsigned int i = 0; i < scene->mNumMeshes; i++) {
				Mmesh mesh;
				aiMesh* aiMesh = scene->mMeshes[i];
				mesh.vertices.reserve(aiMesh->mNumVertices);
				mesh.normals.reserve(aiMesh->mNumVertices);
				mesh.uvs.reserve(aiMesh->mNumVertices);
				mesh.matIndex = aiMesh->mMaterialIndex;
				for (unsigned int j = 0; j < aiMesh->mNumVertices; j++) {
					mesh.vertices.emplace_back(aiMesh->mVertices[j].x, aiMesh->mVertices[j].y, aiMesh->mVertices[j].z);
					if (aiMesh->HasNormals()) {
						mesh.normals.emplace_back(aiMesh->mNormals[j].x, aiMesh->mNormals[j].y, aiMesh->mNormals[j].z);
					}
					if (aiMesh->HasTextureCoords(0)) {
						mesh.uvs.emplace_back(aiMesh->mTextureCoords[0][j].x, aiMesh->mTextureCoords[0][j].y);
					}
				}
				for (unsigned int j = 0; j < aiMesh->mNumFaces; j++) {
					aiFace face = aiMesh->mFaces[j];
					for (unsigned int k = 0; k < face.mNumIndices; k++) {
						mesh.indices.push_back(face.mIndices[k]);
					}
				}
				out.meshes.push_back(mesh);
			}
			for (unsigned int i = 0; i < scene->mNumMaterials; i++) {
				Mtexture texture;
				const aiMaterial* mat = scene->mMaterials[i];
				aiString textureName;
				aiString roughName;
				mat->GetTexture(AI_MATKEY_BASE_COLOR_TEXTURE, &textureName);
				mat->GetTexture(AI_MATKEY_ROUGHNESS_TEXTURE, &roughName);
				if (textureName.length > 0) {
					int j = std::stoi(textureName.C_Str() + 1);
					const aiTexture* aiTexture = scene->mTextures[j];
					texture.rawDataPtr = stbi_load_from_memory(reinterpret_cast<const stbi_uc*>(aiTexture->pcData), aiTexture->mWidth, &texture.width, &texture.height, &texture.channels, 4);
				} else {
					texture.rawDataPtr = stbi_load((std::filesystem::path(PROJECT_ASSETS_PATH) / "empty.png").string().c_str(), &texture.width, &texture.height, &texture.channels, 4);
				}
				if (roughName.length > 0) {
					int j = std::stoi(roughName.C_Str() + 1);
					const aiTexture* aiTexture = scene->mTextures[j];
					texture.roughness = stbi_load_from_memory(reinterpret_cast<const stbi_uc*>(aiTexture->pcData), aiTexture->mWidth, &texture.width, &texture.height, &texture.channels, 4);
				} else {
					texture.roughness = stbi_load((std::filesystem::path(PROJECT_ASSETS_PATH) / "empty.png").string().c_str(), &texture.width, &texture.height, &texture.channels, 4);
				}
				out.textures.push_back(std::move(texture));
			}
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
