#include <cstdint>
#include <cstdio>
#include <vector>
#include <string>
#include <fstream>
#include <iostream>

// Tiny Thumb ROM generator for GBA Mode 3 test
// It emits a simple program that:
// - Assumes already in Thumb state and starts at 0x08000000
// - Writes a color to a handful of pixels in Mode 3 VRAM
// - Loops forever
// The instructions are pre-encoded as Thumb halfwords.
// No Nintendo header or BIOS is needed for our emulator; we just execute from 0x08000000.

static std::vector<uint16_t> build_thumb_rom(uint16_t color, uint32_t pixels) {
    std::vector<uint16_t> rom;
    rom.reserve(64);

    auto emit = [&](uint16_t hw) { rom.push_back(hw); };

    // Assemble the following Thumb code (with small unrolled stores):
    // start:
    //   MOV  r0, #0x00        ; low byte of base (0x06000000)
    //   MOV  r1, #0x00        ; middle byte parts (we'll construct base via MOV/MOV/LSL/ADD)
    //   MOV  r2, #0x06
    //   LSL  r2, r2, #24      ; r2 = 0x06000000 (we'll approximate using shifts within Thumb range)
    //   ADD  r0, r2, r0       ; r0 = base = 0x06000000
    //   MOV  r3, #<color & 0xFF>
    //   MOV  r4, #<color >> 8>
    //   LSL  r4, r4, #8
    //   ORR  r3, r4           ; r3 = color (16-bit)
    //   MOV  r5, #<pixels>    ; number of pixels to write (clamped small)
    // loop:
    //   STRH r3, [r0]
    //   ADD  r0, #2
    //   SUB  r5, #1
    //   BNE  loop
    //   B    .
    //
    // Note: Thumb immediate shifts are max 31. To form 0x06000000, we'll do:
    //   MOV r2,#6; LSL r2,#24 (but LSL #24 exceeds 31? it's fine); Our emulator's LSL handles up to 31.

    // MOV r2, #6
    emit(0x2206);
    // LSL r2, r2, #24
    emit(static_cast<uint16_t>(0x0000 | (24u<<6) | (2u<<3) | 2u));
    
    // Build color in r3
    uint8_t lo = static_cast<uint8_t>(color & 0xFF);
    uint8_t hi = static_cast<uint8_t>((color >> 8) & 0xFF);
    // MOV r3,#lo
    emit(static_cast<uint16_t>(0x2000 | (3u<<8) | lo));
    // MOV r4,#hi
    emit(static_cast<uint16_t>(0x2000 | (4u<<8) | hi));
    // LSL r4,r4,#8
    emit(static_cast<uint16_t>(0x0000 | (8u<<6) | (4u<<3) | 4u));
    // ORR r3,r4 (ALU reg: 010000 1100 sss ddd) subop=0xC
    emit(static_cast<uint16_t>(0x4000 | (0xCu<<6) | (4u<<3) | 3u));

    // MOV r5,#count (Thumb MOV imm only encodes 8-bit; clamp to 255)
    uint32_t count = pixels; if (count == 0) count = 1; if (count > 255) count = 255;
    emit(static_cast<uint16_t>(0x2000 | (5u<<8) | (count & 0xFF)));

    // loop:
    size_t loop_index = rom.size();
    // STRH r3,[r2] (STRH imm: 1000 0 imm5 Rb Rd) with imm=0, Rb=2, Rd=3
    emit(static_cast<uint16_t>(0x8000 | (0u<<6) | (2u<<3) | 3u));
    // ADD r2,#2 (ADD Rd,#imm8)
    emit(static_cast<uint16_t>(0x3000 | (2u<<8) | 2u));
    // SUB r5,#1 (SUB Rd,#imm8)
    emit(static_cast<uint16_t>(0x3800 | (5u<<8) | 1u));
    // CMP r5,#0 (CMP imm8)
    emit(static_cast<uint16_t>(0x2800 | (5u<<8) | 0u));
    // BNE loop
    int32_t from = static_cast<int32_t>((rom.size()+1) * 2);
    int32_t to   = static_cast<int32_t>(loop_index * 2);
    int32_t rel  = (to - from) / 2;
    uint16_t bne = static_cast<uint16_t>(0xD100 | (rel & 0xFF));
    emit(bne);

    // B . (infinite loop)
    emit(static_cast<uint16_t>(0xE000 | 0x7FF));

    return rom;
}

static std::vector<uint8_t> to_bytes_little_endian(const std::vector<uint16_t>& halfwords) {
    std::vector<uint8_t> bytes;
    bytes.reserve(halfwords.size()*2);
    for (uint16_t hw : halfwords) {
        bytes.push_back(static_cast<uint8_t>(hw & 0xFF));
        bytes.push_back(static_cast<uint8_t>((hw >> 8) & 0xFF));
    }
    return bytes;
}

int main(int argc, char** argv) {
    // Default: blue-ish color and 200 pixels
    uint16_t color = 0x001F; // BGR555 blue
    uint32_t pixels = 200;
    std::string outPath = "test_rom.gba";

    if (argc >= 2) outPath = argv[1];
    if (argc >= 3) color = static_cast<uint16_t>(std::stoul(argv[2], nullptr, 0));
    if (argc >= 4) pixels = static_cast<uint32_t>(std::stoul(argv[3], nullptr, 0));

    auto rom_hw = build_thumb_rom(color, pixels);
    auto rom_bytes = to_bytes_little_endian(rom_hw);

    std::ofstream ofs(outPath, std::ios::binary);
    if (!ofs) {
        std::cerr << "Failed to open output file: " << outPath << "\n";
        return 1;
    }
    ofs.write(reinterpret_cast<const char*>(rom_bytes.data()), static_cast<std::streamsize>(rom_bytes.size()));
    ofs.close();

    std::cout << "Wrote ROM: " << outPath << " (" << rom_bytes.size() << " bytes)\n";
    std::cout << "Usage: romgen [outPath] [color_bgr555 (e.g., 0x7FFF)] [pixel_count]\n";
    return 0;
}
