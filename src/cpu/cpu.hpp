#pragma once
#include <cstdint>

namespace gba {

struct Bus; // fwd

struct CPU {
    // ARM7TDMI (Thumb-first subset)
    enum : int { R0=0, R1, R2, R3, R4, R5, R6, R7, R8, R9, R10, R11, R12, SP=13, LR=14, PC=15 };

    // CPSR flag bits
    static constexpr uint32_t FLAG_N = 1u << 31;
    static constexpr uint32_t FLAG_Z = 1u << 30;
    static constexpr uint32_t FLAG_C = 1u << 29;
    static constexpr uint32_t FLAG_V = 1u << 28;
    static constexpr uint32_t FLAG_T = 1u << 5; // Thumb state

    uint32_t r[16]{}; // r0-r15
    uint32_t cpsr{};  // flags + T bit, etc.

    void attach_bus(Bus* b) { bus = b; }
    void reset();
    void step(); // executes one Thumb instruction for now

private:
    Bus* bus{nullptr};

    // Thumb helpers
    uint16_t fetch16(uint32_t addr) const;
    uint16_t fetch16_pc();

    // Flags helpers
    inline bool getC() const { return (cpsr & FLAG_C) != 0; }
    inline void setNZ(uint32_t result) {
        cpsr &= ~(FLAG_N | FLAG_Z);
        if ((result & 0x80000000u) != 0) cpsr |= FLAG_N;
        if (result == 0) cpsr |= FLAG_Z;
    }
    inline void setLogicNZC(uint32_t result, bool c) {
        setNZ(result);
        cpsr = (cpsr & ~FLAG_C) | (c ? FLAG_C : 0u);
        cpsr &= ~FLAG_V;
    }
    inline void setAddNZCV(uint32_t a, uint32_t b, uint32_t res) {
        setNZ(res);
        uint64_t wide = static_cast<uint64_t>(a) + static_cast<uint64_t>(b);
        cpsr = (cpsr & ~FLAG_C) | ((wide >> 32) ? FLAG_C : 0u);
        uint32_t sa = a ^ 0x80000000u; // trick alternative? Simpler formula:
        bool v = (~(a ^ b) & (a ^ res) & 0x80000000u) != 0;
        cpsr = (cpsr & ~FLAG_V) | (v ? FLAG_V : 0u);
    }
    inline void setSubNZCV(uint32_t a, uint32_t b, uint32_t res) {
        setNZ(res);
        cpsr = (cpsr & ~FLAG_C) | ((a >= b) ? FLAG_C : 0u);
        bool v = ((a ^ b) & (a ^ res) & 0x80000000u) != 0;
        cpsr = (cpsr & ~FLAG_V) | (v ? FLAG_V : 0u);
    }

    bool cond_passed(uint32_t cond) const;

    // Execute
    void exec_thumb(uint16_t op);
    uint32_t lsl_c(uint32_t value, uint32_t amount, bool& c_out) const;
    uint32_t lsr_c(uint32_t value, uint32_t amount, bool& c_out) const;
    uint32_t asr_c(uint32_t value, uint32_t amount, bool& c_out) const;
    uint32_t ror_c(uint32_t value, uint32_t amount, bool& c_out) const;
};

}
