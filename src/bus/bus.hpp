#pragma once
#include <cstdint>
#include <vector>

namespace gba {

struct PPU;    // fwd
struct Cartridge; // fwd

struct Bus {
    // Helpers/regions
    static constexpr uint32_t VRAM_BASE = 0x06000000;
    static constexpr uint32_t VRAM_SIZE = 240 * 160 * 2; // Mode 3 only for now
    static constexpr uint32_t ROM_BASE  = 0x08000000;
    static constexpr uint32_t ROM_SIZE  = 32 * 1024 * 1024; // up to 32MB window
    static constexpr uint32_t WRAM_BASE = 0x02000000;
    static constexpr uint32_t WRAM_SIZE = 256 * 1024;

    // Connect components owned by GBA
    void connect(PPU* ppu_, Cartridge* cart_);

    // Basic memory accesses (little-endian)
    uint8_t  read8(uint32_t addr) const;
    uint16_t read16(uint32_t addr) const;
    uint32_t read32(uint32_t addr) const;

    void write8(uint32_t addr, uint8_t v);
    void write16(uint32_t addr, uint16_t v);
    void write32(uint32_t addr, uint32_t v);

private:
    PPU* ppu{nullptr};
    Cartridge* cart{nullptr};

    // Minimal on-board work RAM
    std::vector<uint8_t> wram = std::vector<uint8_t>(WRAM_SIZE);
};

}
