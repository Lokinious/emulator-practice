#include <SDL.h>
#include <cstdint>
#include <vector>
#include <string>
#include <chrono>
#include <cmath>
#include "../gba.hpp"

static constexpr int GBA_WIDTH = 240;
static constexpr int GBA_HEIGHT = 160;

int main(int argc, char* argv[]) {
    gba::GBA system;
    system.reset();

    bool hasRom = false;
    std::string romPath;
    if (argc >= 2) {
        romPath = argv[1];
        if (!system.load(romPath)) {
            SDL_Log("Failed to load ROM: %s", romPath.c_str());
        } else {
            SDL_Log("Loaded ROM: %s (%zu bytes)", romPath.c_str(), system.cart.rom.size());
            hasRom = true;
        }
    }

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS) != 0) {
        SDL_Log("SDL_Init Error: %s", SDL_GetError());
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow(
        "GBA Emulator", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        GBA_WIDTH * 3, GBA_HEIGHT * 3, SDL_WINDOW_RESIZABLE
    );
    if (!window) {
        SDL_Log("SDL_CreateWindow Error: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        SDL_Log("SDL_CreateRenderer Error: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, GBA_WIDTH, GBA_HEIGHT);
    if (!texture) {
        SDL_Log("SDL_CreateTexture Error: %s", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    std::vector<uint32_t> argb;

    bool running = true;
    auto start = std::chrono::high_resolution_clock::now();

    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = false;
            if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) running = false;
        }

        auto now = std::chrono::high_resolution_clock::now();
        float t = std::chrono::duration<float>(now - start).count();

        if (hasRom) {
            // Step CPU a bunch of instructions per frame
            // Tune this number as needed; we just want visible progress for tiny test ROMs
            constexpr int STEPS_PER_FRAME = 200000; // 200k thumb ops
            for (int i = 0; i < STEPS_PER_FRAME; ++i) {
                system.cpu.step();
            }
        } else {
            // Fallback: fill VRAM with a gradient if no ROM loaded
            for (int y = 0; y < gba::PPU::HEIGHT; ++y) {
                for (int x = 0; x < gba::PPU::WIDTH; ++x) {
                    uint8_t r5 = static_cast<uint8_t>((x * 31) / gba::PPU::WIDTH);
                    uint8_t g5 = static_cast<uint8_t>((y * 31) / gba::PPU::HEIGHT);
                    uint8_t b5 = static_cast<uint8_t>((0.5f + 0.5f * std::sin(t)) * 31);
                    uint16_t bgr555 = (r5 << 10) | (g5 << 5) | b5;
                    uint32_t addr = gba::Bus::VRAM_BASE + static_cast<uint32_t>((y * gba::PPU::WIDTH + x) * 2);
                    system.bus.write16(addr, bgr555);
                }
            }
        }

        system.render_mode3_to_argb(argb);
        SDL_UpdateTexture(texture, nullptr, argb.data(), GBA_WIDTH * sizeof(uint32_t));

        int w, h; SDL_GetRendererOutputSize(renderer, &w, &h);
        SDL_Rect dst; 
        float windowAspect = static_cast<float>(w) / static_cast<float>(h);
        float gbaAspect = 3.0f / 2.0f;
        if (windowAspect > gbaAspect) {
            dst.h = h; dst.w = static_cast<int>(h * gbaAspect);
            dst.x = (w - dst.w) / 2; dst.y = 0;
        } else {
            dst.w = w; dst.h = static_cast<int>(w / gbaAspect);
            dst.x = 0; dst.y = (h - dst.h) / 2;
        }

        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, nullptr, &dst);
        SDL_RenderPresent(renderer);
    }

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
