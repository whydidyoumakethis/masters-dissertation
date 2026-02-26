#include "Shaders.hpp"

#include <cassert>
#include <cstdio>

#include "../../logging/FatalError.hpp"

namespace rutils {
    std::vector<std::uint32_t> loadShader(char const* aPath) {
		assert( aPath );

		if (std::FILE* fin = std::fopen(aPath, "rb")) {
			std::fseek(fin, 0, SEEK_END);
			auto const bytes = std::size_t(std::ftell(fin));
			std::fseek(fin, 0, SEEK_SET);

			// SpirV consists of a number of 32-bit = 4 byte words
			assert(0 == bytes % 4);
			auto const words = bytes / 4;

			std::vector<std::uint32_t> code(words);
			
			std::size_t offset = 0;
			while (offset != words) {
				auto const read = std::fread(code.data() + offset, sizeof(std::uint32_t), words - offset, fin);

				if (0 == read) {
					auto const err = std::ferror(fin), eof = std::feof(fin);
					std::fclose(fin);

					throw Kiki::FatalError("Error reading '%s': ferror = %d, feof = %d", aPath, err, eof);
				}

				offset += read;
			}

			std::fclose(fin);

			return code;
		}

		throw Kiki::FatalError("Cannot open '{}' for reading", aPath);
	}
}