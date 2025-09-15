#include "rom.hpp"
#include <fstream>

namespace gba {

bool Cartridge::load_from_file(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;
    f.seekg(0, std::ios::end);
    std::streamsize size = f.tellg();
    if (size <= 0) return false;
    f.seekg(0, std::ios::beg);
    rom.resize(static_cast<size_t>(size));
    if (!f.read(reinterpret_cast<char*>(rom.data()), size)) return false;
    return true;
}

}
