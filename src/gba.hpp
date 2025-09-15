#pragma once
#include "cpu/cpu.hpp"
#include "bus/bus.hpp"
#include "cart/rom.hpp"
#include "ppu/ppu.hpp"
#include <vector>

namespace gba {

struct GBA {
    CPU cpu;
    Bus bus;
    Cartridge cart;
    PPU ppu;

    void reset() {
        cpu.reset();
        bus.connect(&ppu, &cart);
        cpu.attach_bus(&bus);
    }
    bool load(const std::string& romPath) { return cart.load_from_file(romPath); }

    void render_mode3_to_argb(std::vector<uint32_t>& out) {
        out.resize(PPU::WIDTH * PPU::HEIGHT);
        for (int i = 0; i < PPU::WIDTH * PPU::HEIGHT; ++i) {
            out[i] = PPU::bgr555_to_argb8888(ppu.vram[i]);
        }
    }
};

}
