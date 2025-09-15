#pragma once
#include <cstdint>
#include <array>

namespace gba {

// Mode 3: 240x160, 15-bit BGR (5 bits each), 1 frame buffer at 0x06000000
struct PPU {
    static constexpr int WIDTH = 240;
    static constexpr int HEIGHT = 160;

    // Simulated VRAM Mode 3 (each pixel 16-bit BGR555)
    std::array<uint16_t, WIDTH * HEIGHT> vram{};

    // Convert BGR555 to ARGB8888 for the SDL front-end
    static inline uint32_t bgr555_to_argb8888(uint16_t px) {
        uint32_t b = (px & 0x1F);
        uint32_t g = (px >> 5) & 0x1F;
        uint32_t r = (px >> 10) & 0x1F;
        // Expand 5-bit to 8-bit by bit replication
        auto exp = [](uint32_t v){ return (v << 3) | (v >> 2); };
        return 0xFF000000u | (exp(r) << 16) | (exp(g) << 8) | exp(b);
    }
};

}
