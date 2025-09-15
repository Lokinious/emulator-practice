#pragma once
#include <cstdint>
#include <vector>
#include <string>

namespace gba {

struct Cartridge {
    std::vector<uint8_t> rom;

    bool load_from_file(const std::string& path);
};

}
