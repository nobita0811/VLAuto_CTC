// LPROTECT - mini virtual machine for code virtualization.
//
// Idea (same family as commercial VM protectors, in miniature): instead of
// shipping a sensitive routine as native x86, you express it as bytecode for a
// small custom stack machine and ship the interpreter + the (encrypted) bytecode.
// An analyst no longer sees your algorithm as x86 instructions; they must first
// understand YOUR vm, then lift the bytecode. That is the cost multiplier.
//
// This VM is intentionally small and readable so you can learn from and extend
// it. Polymorphism is ALREADY on: the opcode<->byte mapping is permuted at
// compile time from LP_BUILD_SEED, and the bytecode cipher is also seeded by it,
// so the same program emits different bytes in every build. For more strength
// add opcodes/handlers and virtualize only hot/critical checks (it is ~100x
// slower than native).
//
// NOTE: because the physical opcode bytes depend on LP_BUILD_SEED, a VmProgram's
// bytes are only valid for binaries built with the SAME seed. Build the bytecode
// in-process (as the demo does); do not persist it across differently-seeded
// builds.
//
// Build bytecode with the VmBuilder, then run it:
//
//   lp::VmBuilder b;
//   b.ldarg(0); b.ldarg(1); b.mul();     // a*b
//   b.ldarg(2); b.add();                 // + c
//   b.push(0x1234); b.xor_();            // ^ key
//   b.ret();
//   auto prog = b.finish(0xC0FFEEu);     // encrypt with per-build key
//   int64_t args[3] = {a, b, c};
//   lp::VmResult r = lp::vm_run(prog, args, 3);
//   if (r.ok) use(r.value);
#ifndef LPROTECT_LP_VM_H
#define LPROTECT_LP_VM_H

#include "lp_config.h"
#include <vector>

namespace lp {

// LOGICAL opcode ids (0..17). The PHYSICAL byte written into bytecode is a
// per-build permutation of these (see lp_vm.cpp), so these values are stable in
// source but the emitted bytes are not.
enum VmOp : uint8_t {
    VM_HALT  = 0,
    VM_PUSH  = 1,   // + int32 immediate (little-endian)
    VM_LDARG = 2,   // + uint8 arg index
    VM_POP   = 3,
    VM_DUP   = 4,
    VM_ADD   = 5,
    VM_SUB   = 6,
    VM_MUL   = 7,
    VM_XOR   = 8,
    VM_AND   = 9,
    VM_OR    = 10,
    VM_SHL   = 11,
    VM_SHR   = 12,
    VM_CMPEQ = 13,  // pop a,b -> push (a==b)?1:0
    VM_CMPLT = 14,  // pop a,b -> push (b<a)?1:0
    VM_JMP   = 15,  // + int16 relative offset from end of operand
    VM_JZ    = 16,  // + int16 rel; pop cond, jump if zero
    VM_RET   = 17   // return top of stack
};

constexpr int LP_VM_OPCOUNT = 18; // VM_HALT .. VM_RET

// An encrypted VM program: bytecode + the key needed to decrypt it at run time.
struct VmProgram {
    std::vector<uint8_t> code;  // encrypted
    uint32_t             key;   // per-build decryption key
};

struct VmResult {
    bool    ok;      // false on fault (stack over/underflow, bad opcode, no ret)
    int64_t value;   // return value when ok
};

// Execute. `args`/`argc` provide VM_LDARG inputs. Bytecode is decrypted on the
// fly into a scratch buffer; the plaintext bytecode is not kept resident.
VmResult vm_run(const VmProgram& prog, const int64_t* args, size_t argc);

// Fluent builder that emits bytecode and encrypts on finish().
class VmBuilder {
public:
    VmBuilder& push(int32_t imm);
    VmBuilder& ldarg(uint8_t idx);
    VmBuilder& pop();
    VmBuilder& dup();
    VmBuilder& add();
    VmBuilder& sub();
    VmBuilder& mul();
    VmBuilder& xor_();
    VmBuilder& and_();
    VmBuilder& or_();
    VmBuilder& shl();
    VmBuilder& shr();
    VmBuilder& cmpeq();
    VmBuilder& cmplt();
    VmBuilder& ret();
    VmBuilder& halt();

    // Labels for simple branching.
    int  label();                 // reserve a label id
    VmBuilder& bind(int lbl);      // mark current position as label target
    VmBuilder& jmp(int lbl);
    VmBuilder& jz(int lbl);

    VmProgram finish(uint32_t key);

private:
    void emit(uint8_t b) { plain_.push_back(b); }
    void emit32(int32_t v);
    std::vector<uint8_t> plain_;
    struct Fixup { size_t at; int lbl; };
    std::vector<Fixup>   fixups_;
    std::vector<int>     label_pos_;
};

} // namespace lp

#endif // LPROTECT_LP_VM_H
