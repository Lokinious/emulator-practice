#include "bus.hpp"
#include "../ppu/ppu.hpp"
#include "../cart/rom.hpp"
#include <cstring>
#include <algorithm>

namespace gba {

void Bus::connect(PPU* ppu_, Cartridge* cart_) {
    ppu = ppu_;
    cart = cart_;
}

uint8_t Bus::read8(uint32_t addr) const {
    if (addr >= VRAM_BASE && addr < VRAM_BASE + VRAM_SIZE) {
        uint32_t off = addr - VRAM_BASE;
        // Mode 3 VRAM is 16-bit aligned; allow byte fetch by reading the 16-bit pixel
        uint16_t px = ppu ? ppu->vram[(off >> 1)] : 0;
        return (off & 1) ? static_cast<uint8_t>(px >> 8) : static_cast<uint8_t>(px & 0xFF);
    }
    if (addr >= ROM_BASE && addr < ROM_BASE + ROM_SIZE) {
        if (!cart || cart->rom.empty()) return 0xFF;
        uint32_t off = addr - ROM_BASE;
        if (off < cart->rom.size()) return cart->rom[off];
        return 0xFF;
    }
    if (addr >= WRAM_BASE && addr < WRAM_BASE + WRAM_SIZE) {
        return wram[addr - WRAM_BASE];
    }
    return 0; // default
}

uint16_t Bus::read16(uint32_t addr) const {
    uint16_t lo = read8(addr);
    uint16_t hi = read8(addr + 1);
    return static_cast<uint16_t>(lo | (hi << 8));
}

uint32_t Bus::read32(uint32_t addr) const {
    uint32_t b0 = read8(addr);
    uint32_t b1 = read8(addr + 1);
    uint32_t b2 = read8(addr + 2);
    uint32_t b3 = read8(addr + 3);
    return b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);
}

void Bus::write8(uint32_t addr, uint8_t v) {
    if (addr >= VRAM_BASE && addr < VRAM_BASE + VRAM_SIZE) {
        if (!ppu) return;
        uint32_t off = addr - VRAM_BASE;
        uint16_t& px = ppu->vram[(off >> 1)];
        if (off & 1) {
            px = static_cast<uint16_t>((px & 0x00FF) | (static_cast<uint16_t>(v) << 8));
        } else {
            px = static_cast<uint16_t>((px & 0xFF00) | v);
        }
        return;
    }
    if (addr >= WRAM_BASE && addr < WRAM_BASE + WRAM_SIZE) {
        wram[addr - WRAM_BASE] = v;
        return;
    }
    // ignore
}

void Bus::write16(uint32_t addr, uint16_t v) {
    if (addr >= VRAM_BASE && addr < VRAM_BASE + VRAM_SIZE) {
        if (!ppu) return;
        uint32_t off = (addr - VRAM_BASE) >> 1;
        if (off < ppu->vram.size()) ppu->vram[off] = v;
        return;
    }
    if (addr >= WRAM_BASE && addr < WRAM_BASE + WRAM_SIZE) {
        uint32_t off = addr - WRAM_BASE;
        if (off + 1 < wram.size()) {
            wram[off] = static_cast<uint8_t>(v & 0xFF);
            wram[off + 1] = static_cast<uint8_t>(v >> 8);
        }
        return;
    }
}

void Bus::write32(uint32_t addr, uint32_t v) {
    write16(addr, static_cast<uint16_t>(v & 0xFFFF));
    write16(addr + 2, static_cast<uint16_t>(v >> 16));
}

}
