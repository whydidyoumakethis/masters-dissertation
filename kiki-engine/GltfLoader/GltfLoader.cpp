#include <GltfLoader/GltfLoader.hpp>
#include <iostream>
#include <glm/glm.hpp>
namespace Kiki {

	 bool GltfLoader::loadMesh(const std::filesystem::path& path) {
		
		fastgltf::Parser parser;
		std::cout << std::filesystem::current_path() << std::endl;
		auto data = fastgltf::GltfDataBuffer::FromPath(path);
		if (data.error() != fastgltf::Error::None) {
			throw std::runtime_error("Failed to load glTF file: " + path.string());
		}

		auto asset = parser.loadGltf(data.get(), path.parent_path(), fastgltf::Options::None);
		if (auto error = asset.error(); error != fastgltf::Error::None) {
			throw std::runtime_error("Failed to parse glTF file: " + path.string() + " with error code: " + std::to_string(static_cast<std::uint64_t>(asset)));
		}

		for(auto& buffer : asset->meshes) {
			std::cout << "Mesh: " << buffer.name << std::endl;
			//printing vert positions

		}

		return true;
	}
}