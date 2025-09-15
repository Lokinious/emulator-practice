#include "cpu.hpp"
#include "../bus/bus.hpp"

namespace gba {

void CPU::reset() {
    for (auto &reg : r) reg = 0;
    cpsr = 0;
    // Start in Thumb for now with PC at ROM base
    cpsr |= (1u << 5); // T bit
    r[PC] = 0x08000000; // cartridge ROM base
    r[SP] = 0x03007F00; // placeholder stack in IWRAM range (not mapped yet)
}

uint16_t CPU::fetch16(uint32_t addr) const {
    return bus ? bus->read16(addr) : 0;
}

uint16_t CPU::fetch16_pc() {
    uint16_t v = fetch16(r[PC] & ~1u);
    r[PC] += 2; // Thumb advances 2 bytes
    return v;
}

static inline uint32_t rotate_right(uint32_t value, uint32_t amount) {
    amount &= 31;
    return (value >> amount) | (value << (32 - amount));
}

uint32_t CPU::lsl_c(uint32_t value, uint32_t amount, bool& c_out) const {
    if (amount == 0) return value; // C unaffected
    if (amount < 32) {
        c_out = (value & (1u << (32 - amount))) != 0;
        return value << amount;
    }
    if (amount == 32) {
        c_out = (value & 1u) != 0;
        return 0;
    }
    c_out = false; return 0;
}

uint32_t CPU::lsr_c(uint32_t value, uint32_t amount, bool& c_out) const {
    if (amount == 0) return value; // C unaffected
    if (amount < 32) {
        c_out = (value & (1u << (amount - 1))) != 0;
        return value >> amount;
    }
    if (amount == 32) {
        c_out = (value & 0x80000000u) != 0; // Actually for LSR #32, C = bit 31
        return 0;
    }
    c_out = false; return 0;
}

uint32_t CPU::asr_c(uint32_t value, uint32_t amount, bool& c_out) const {
    if (amount == 0) return value; // C unaffected
    if (amount < 32) {
        c_out = (value & (1u << (amount - 1))) != 0;
        uint32_t sign = value & 0x80000000u;
        uint32_t res = (value >> amount) | (sign ? ~(0xFFFFFFFFu >> amount) : 0);
        return res;
    }
    c_out = (value & 0x80000000u) != 0;
    return (value & 0x80000000u) ? 0xFFFFFFFFu : 0;
}

uint32_t CPU::ror_c(uint32_t value, uint32_t amount, bool& c_out) const {
    amount &= 31;
    if (amount == 0) return value; // C unaffected
    uint32_t res = rotate_right(value, amount);
    c_out = (res & 0x80000000u) != 0;
    return res;
}

bool CPU::cond_passed(uint32_t cond) const {
    bool n = (cpsr & FLAG_N) != 0;
    bool z = (cpsr & FLAG_Z) != 0;
    bool c = (cpsr & FLAG_C) != 0;
    bool v = (cpsr & FLAG_V) != 0;
    switch (cond) {
        case 0x0: return z;              // EQ
        case 0x1: return !z;             // NE
        case 0x2: return c;              // CS/HS
        case 0x3: return !c;             // CC/LO
        case 0x4: return n;              // MI
        case 0x5: return !n;             // PL
        case 0x6: return v;              // VS
        case 0x7: return !v;             // VC
        case 0x8: return c && !z;        // HI
        case 0x9: return !c || z;        // LS
        case 0xA: return n == v;         // GE
        case 0xB: return n != v;         // LT
        case 0xC: return !z && (n == v); // GT
        case 0xD: return z || (n != v);  // LE
        default: return true;            // 0xE AL (unused in Thumb cond branch)
    }
}

void CPU::exec_thumb(uint16_t op) {
    // Shifts by immediate: LSL/LSR/ASR
    if ((op & 0xF800) == 0x0000) {
        // LSL Rd, Rs, #imm5
        uint32_t imm5 = (op >> 6) & 0x1F;
        uint32_t rs = (op >> 3) & 0x7;
        uint32_t rd = op & 0x7;
        bool c = (cpsr & FLAG_C) != 0;
        uint32_t res = r[rs];
        if (imm5) {
            c = (res & (1u << (32 - imm5))) != 0;
            res <<= imm5;
            cpsr = (c ? (cpsr | FLAG_C) : (cpsr & ~FLAG_C));
        }
        setNZ(res);
        r[rd] = res;
        return;
    }
    if ((op & 0xF800) == 0x0800) {
        // LSR Rd, Rs, #imm5
        uint32_t imm5 = (op >> 6) & 0x1F;
        uint32_t rs = (op >> 3) & 0x7;
        uint32_t rd = op & 0x7;
        uint32_t res;
        bool c;
        if (imm5 == 0) { c = (r[rs] >> 31) & 1; res = 0; }
        else { c = (r[rs] >> (imm5 - 1)) & 1; res = r[rs] >> imm5; }
        cpsr = (c ? (cpsr | FLAG_C) : (cpsr & ~FLAG_C));
        setNZ(res);
        r[rd] = res;
        return;
    }
    if ((op & 0xF800) == 0x1000) {
        // ASR Rd, Rs, #imm5
        uint32_t imm5 = (op >> 6) & 0x1F;
        uint32_t rs = (op >> 3) & 0x7;
        uint32_t rd = op & 0x7;
        uint32_t res;
        bool c;
        if (imm5 == 0) { c = (r[rs] >> 31) & 1; res = (r[rs] & 0x80000000u) ? 0xFFFFFFFFu : 0; }
        else {
            c = (r[rs] >> (imm5 - 1)) & 1;
            res = static_cast<uint32_t>(static_cast<int32_t>(r[rs]) >> imm5);
        }
        cpsr = (c ? (cpsr | FLAG_C) : (cpsr & ~FLAG_C));
        setNZ(res);
        r[rd] = res;
        return;
    }

    // MOV/CMP/ADD/SUB immediate
    if ((op & 0xE000) == 0x2000) {
        // MOV Rd, #imm8
        uint32_t rd = (op >> 8) & 0x7;
        uint32_t imm8 = op & 0xFF;
        r[rd] = imm8;
        setNZ(r[rd]);
        return;
    }
    if ((op & 0xF800) == 0x2800) {
        // CMP Rd, #imm8
        uint32_t rd = (op >> 8) & 0x7;
        uint32_t imm8 = op & 0xFF;
        uint32_t res = r[rd] - imm8;
        setSubNZCV(r[rd], imm8, res);
        return;
    }
    if ((op & 0xF800) == 0x3000) {
        // ADD Rd, #imm8
        uint32_t rd = (op >> 8) & 0x7;
        uint32_t imm8 = op & 0xFF;
        uint32_t res = r[rd] + imm8;
        setAddNZCV(r[rd], imm8, res);
        r[rd] = res;
        return;
    }
    if ((op & 0xF800) == 0x3800) {
        // SUB Rd, #imm8
        uint32_t rd = (op >> 8) & 0x7;
        uint32_t imm8 = op & 0xFF;
        uint32_t res = r[rd] - imm8;
        setSubNZCV(r[rd], imm8, res);
        r[rd] = res;
        return;
    }

    // ALU operations register (010000)
    if ((op & 0xFC00) == 0x4000) {
        uint32_t subop = (op >> 6) & 0xF;
        uint32_t rs = (op >> 3) & 0x7;
        uint32_t rd = op & 0x7;
        uint32_t a = r[rd], b = r[rs], res;
        bool ctmp = getC();
        switch (subop) {
            case 0x0: res = a & b; setLogicNZC(res, getC()); r[rd] = res; break; // AND
            case 0x1: res = a ^ b; setLogicNZC(res, getC()); r[rd] = res; break; // EOR
            case 0x2: res = lsl_c(a, b & 0xFF, ctmp); setLogicNZC(res, ctmp); r[rd] = res; break; // LSL (reg)
            case 0x3: res = lsr_c(a, b & 0xFF, ctmp); setLogicNZC(res, ctmp); r[rd] = res; break; // LSR (reg)
            case 0x4: res = asr_c(a, b & 0xFF, ctmp); setLogicNZC(res, ctmp); r[rd] = res; break; // ASR (reg)
            case 0x5: res = a + b + (getC() ? 1u : 0u); setAddNZCV(a, b + (getC()?1u:0u), res); r[rd] = res; break; // ADC
            case 0x6: res = a - b - (getC() ? 0u : 1u); setSubNZCV(a, b + (getC()?0u:1u), res); r[rd] = res; break; // SBC
            case 0x7: res = ror_c(a, b & 0xFF, ctmp); setLogicNZC(res, ctmp); r[rd] = res; break; // ROR (reg)
            case 0x8: res = a & b; setLogicNZC(res, getC()); break; // TST
            case 0x9: res = static_cast<uint32_t>(-static_cast<int32_t>(b)); setAddNZCV(0, ~b + 1, res); r[rd] = res; break; // NEG
            case 0xA: res = a - b; setSubNZCV(a, b, res); break; // CMP
            case 0xB: res = a + b; setAddNZCV(a, b, res); break; // CMN
            case 0xC: res = a | b; setLogicNZC(res, getC()); r[rd] = res; break; // ORR
            case 0xD: res = a * b; setNZ(res); cpsr &= ~(FLAG_C|FLAG_V); r[rd] = res; break; // MUL
            case 0xE: res = a & ~b; setLogicNZC(res, getC()); r[rd] = res; break; // BIC
            case 0xF: res = ~b; setLogicNZC(res, getC()); r[rd] = res; break; // MVN
        }
        return;
    }

    // LDR literal (PC-relative) 01001
    if ((op & 0xF800) == 0x4800) {
        uint32_t rd = (op >> 8) & 0x7;
        uint32_t imm = (op & 0xFF) << 2;
        uint32_t base = (r[PC] & ~2u); // PC already advanced by 2; align
        if (bus) r[rd] = bus->read32(base + imm);
        return;
    }

    // STR/LDR immediate word, byte, halfword
    if ((op & 0xF800) == 0x6000) {
        uint32_t rb = (op >> 3) & 0x7;
        uint32_t rd = (op >> 0) & 0x7;
        uint32_t imm = ((op >> 6) & 0x1F) << 2;
        if (bus) bus->write32(r[rb] + imm, r[rd]);
        return;
    }
    if ((op & 0xF800) == 0x6800) {
        uint32_t rb = (op >> 3) & 0x7;
        uint32_t rd = (op >> 0) & 0x7;
        uint32_t imm = ((op >> 6) & 0x1F) << 2;
        if (bus) r[rd] = bus->read32(r[rb] + imm);
        return;
    }
    if ((op & 0xF800) == 0x7000) {
        uint32_t rb = (op >> 3) & 0x7;
        uint32_t rd = (op >> 0) & 0x7;
        uint32_t imm = ((op >> 6) & 0x1F);
        if (bus) bus->write8(r[rb] + imm, static_cast<uint8_t>(r[rd]));
        return;
    }
    if ((op & 0xF800) == 0x7800) {
        uint32_t rb = (op >> 3) & 0x7;
        uint32_t rd = (op >> 0) & 0x7;
        uint32_t imm = ((op >> 6) & 0x1F);
        if (bus) r[rd] = bus->read8(r[rb] + imm);
        return;
    }
    if ((op & 0xF800) == 0x8000) {
        uint32_t rb = (op >> 3) & 0x7;
        uint32_t rd = (op >> 0) & 0x7;
        uint32_t imm = ((op >> 6) & 0x1F) << 1;
        if (bus) bus->write16(r[rb] + imm, static_cast<uint16_t>(r[rd]));
        return;
    }
    if ((op & 0xF800) == 0x8800) {
        uint32_t rb = (op >> 3) & 0x7;
        uint32_t rd = (op >> 0) & 0x7;
        uint32_t imm = ((op >> 6) & 0x1F) << 1;
        if (bus) r[rd] = bus->read16(r[rb] + imm);
        return;
    }

    // ADD Rd, PC, #imm (10100)
    if ((op & 0xF800) == 0xA000) {
        uint32_t rd = (op >> 8) & 0x7;
        uint32_t imm = (op & 0xFF) << 2;
        uint32_t base = (r[PC] & ~2u);
        r[rd] = base + imm;
        return;
    }

    // Conditional branch (1101 cccc oooooooo)
    if ((op & 0xF000) == 0xD000) {
        uint32_t cond = (op >> 8) & 0xF;
        int32_t imm8 = static_cast<int8_t>(op & 0xFF); // sign-extend 8-bit
        if (cond == 0xF) {
            // SWI (ignored for now)
            return;
        }
        if (cond_passed(cond)) {
            int32_t offset = imm8 << 1;
            r[PC] = static_cast<uint32_t>(r[PC] + offset);
        }
        return;
    }

    // Unconditional branch (11100)
    if ((op & 0xF800) == 0xE000) {
        int32_t imm11 = op & 0x7FF;
        if (imm11 & 0x400) imm11 |= ~0x7FF;
        int32_t offset = imm11 << 1;
        r[PC] = static_cast<uint32_t>(r[PC] + offset);
        return;
    }

    // Unknown/unsupported: do nothing
}

void CPU::step() {
    // For now assume Thumb mode; fetch and execute one halfword
    uint16_t op = fetch16_pc();
    exec_thumb(op);
}

}
