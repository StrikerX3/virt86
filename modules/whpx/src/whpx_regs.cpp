/*
Implementations of helper functions for dealing with Windows Hypervisor
Platform data structures.
-------------------------------------------------------------------------------
MIT License

Copyright (c) 2019 Ivan Roberto de Oliveira

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#include "whpx_regs.hpp"
#include "whpx_defs.hpp"

#include "virt86/util/bytemanip.hpp"

namespace virt86::whpx {

VPOperationStatus TranslateRegisterName(const Reg reg, WHV_REGISTER_NAME& name) noexcept {
    switch (reg) {
    case Reg::CS: name = WHvX64RegisterCs; break;
    case Reg::SS: name = WHvX64RegisterSs; break;
    case Reg::DS: name = WHvX64RegisterDs; break;
    case Reg::ES: name = WHvX64RegisterEs; break;
    case Reg::FS: name = WHvX64RegisterFs; break;
    case Reg::GS: name = WHvX64RegisterGs; break;
    case Reg::LDTR: name = WHvX64RegisterLdtr; break;
    case Reg::TR: name = WHvX64RegisterTr; break;

    case Reg::GDTR: name = WHvX64RegisterGdtr; break;
    case Reg::IDTR: name = WHvX64RegisterIdtr; break;

    case Reg::AL: case Reg::AH: case Reg::AX: case Reg::EAX: case Reg::RAX: name = WHvX64RegisterRax; break;
    case Reg::CL: case Reg::CH: case Reg::CX: case Reg::ECX: case Reg::RCX: name = WHvX64RegisterRcx; break;
    case Reg::DL: case Reg::DH: case Reg::DX: case Reg::EDX: case Reg::RDX: name = WHvX64RegisterRdx; break;
    case Reg::BL: case Reg::BH: case Reg::BX: case Reg::EBX: case Reg::RBX: name = WHvX64RegisterRbx; break;
    case Reg::SPL: case Reg::SP: case Reg::ESP: case Reg::RSP: name = WHvX64RegisterRsp; break;
    case Reg::BPL: case Reg::BP: case Reg::EBP: case Reg::RBP: name = WHvX64RegisterRbp; break;
    case Reg::SIL: case Reg::SI: case Reg::ESI: case Reg::RSI: name = WHvX64RegisterRsi; break;
    case Reg::DIL: case Reg::DI: case Reg::EDI: case Reg::RDI: name = WHvX64RegisterRdi; break;
    case Reg::R8B: case Reg::R8W: case Reg::R8D: case Reg::R8: name = WHvX64RegisterR8; break;
    case Reg::R9B: case Reg::R9W: case Reg::R9D: case Reg::R9: name = WHvX64RegisterR9; break;
    case Reg::R10B: case Reg::R10W: case Reg::R10D: case Reg::R10: name = WHvX64RegisterR10; break;
    case Reg::R11B: case Reg::R11W: case Reg::R11D: case Reg::R11: name = WHvX64RegisterR11; break;
    case Reg::R12B: case Reg::R12W: case Reg::R12D: case Reg::R12: name = WHvX64RegisterR12; break;
    case Reg::R13B: case Reg::R13W: case Reg::R13D: case Reg::R13: name = WHvX64RegisterR13; break;
    case Reg::R14B: case Reg::R14W: case Reg::R14D: case Reg::R14: name = WHvX64RegisterR14; break;
    case Reg::R15B: case Reg::R15W: case Reg::R15D: case Reg::R15: name = WHvX64RegisterR15; break;

    case Reg::IP: case Reg::EIP: case Reg::RIP: name = WHvX64RegisterRip; break;

    case Reg::FLAGS: case Reg::EFLAGS: case Reg::RFLAGS: name = WHvX64RegisterRflags; break;

    case Reg::CR0: name = WHvX64RegisterCr0; break;
    case Reg::CR2: name = WHvX64RegisterCr2; break;
    case Reg::CR3: name = WHvX64RegisterCr3; break;
    case Reg::CR4: name = WHvX64RegisterCr4; break;
    case Reg::CR8: name = WHvX64RegisterCr8; break;

    case Reg::EFER: name = WHvX64RegisterEfer; break;
    case Reg::XCR0: if (WHPX_MIN_VERSION(10_0_17763_0)) { name = WHvX64RegisterXCr0; break; } return VPOperationStatus::Unsupported;

    case Reg::DR0: name = WHvX64RegisterDr0; break;
    case Reg::DR1: name = WHvX64RegisterDr1; break;
    case Reg::DR2: name = WHvX64RegisterDr2; break;
    case Reg::DR3: name = WHvX64RegisterDr3; break;
    case Reg::DR6: name = WHvX64RegisterDr6; break;
    case Reg::DR7: name = WHvX64RegisterDr7; break;

    case Reg::ST0: case Reg::MM0: name = WHvX64RegisterFpMmx0; break;
    case Reg::ST1: case Reg::MM1: name = WHvX64RegisterFpMmx1; break;
    case Reg::ST2: case Reg::MM2: name = WHvX64RegisterFpMmx2; break;
    case Reg::ST3: case Reg::MM3: name = WHvX64RegisterFpMmx3; break;
    case Reg::ST4: case Reg::MM4: name = WHvX64RegisterFpMmx4; break;
    case Reg::ST5: case Reg::MM5: name = WHvX64RegisterFpMmx5; break;
    case Reg::ST6: case Reg::MM6: name = WHvX64RegisterFpMmx6; break;
    case Reg::ST7: case Reg::MM7: name = WHvX64RegisterFpMmx7; break;

    case Reg::XMM0: name = WHvX64RegisterXmm0; break;
    case Reg::XMM1: name = WHvX64RegisterXmm1; break;
    case Reg::XMM2: name = WHvX64RegisterXmm2; break;
    case Reg::XMM3: name = WHvX64RegisterXmm3; break;
    case Reg::XMM4: name = WHvX64RegisterXmm4; break;
    case Reg::XMM5: name = WHvX64RegisterXmm5; break;
    case Reg::XMM6: name = WHvX64RegisterXmm6; break;
    case Reg::XMM7: name = WHvX64RegisterXmm7; break;
    case Reg::XMM8: name = WHvX64RegisterXmm8; break;
    case Reg::XMM9: name = WHvX64RegisterXmm9; break;
    case Reg::XMM10: name = WHvX64RegisterXmm10; break;
    case Reg::XMM11: name = WHvX64RegisterXmm11; break;
    case Reg::XMM12: name = WHvX64RegisterXmm12; break;
    case Reg::XMM13: name = WHvX64RegisterXmm13; break;
    case Reg::XMM14: name = WHvX64RegisterXmm14; break;
    case Reg::XMM15: name = WHvX64RegisterXmm15; break;
    default: return VPOperationStatus::Unsupported;
    }
    return VPOperationStatus::OK;
}

RegValue TranslateRegisterValue(const Reg reg, const WHV_REGISTER_VALUE& value) noexcept {
    RegValue result{ 0 };
    switch (reg) {
    case Reg::CS: case Reg::SS: case Reg::DS: case Reg::ES: case Reg::FS: case Reg::GS:
    case Reg::LDTR: case Reg::TR:
        result.segment.selector = value.Segment.Selector;
        result.segment.base = value.Segment.Base;
        result.segment.limit = value.Segment.Limit;
        result.segment.attributes.u16 = value.Segment.Attributes;
        break;

    case Reg::GDTR: case Reg::IDTR:
        result.table.base = value.Table.Base;
        result.table.limit = value.Table.Limit;
        break;

    case Reg::EFER:
        result.u64 = value.Reg64;
        break;

    case Reg::AL: case Reg::AH: case Reg::CL: case Reg::CH: case Reg::DL: case Reg::DH: case Reg::BL: case Reg::BH:
    case Reg::SPL: case Reg::BPL: case Reg::SIL: case Reg::DIL:
    case Reg::R8B: case Reg::R9B: case Reg::R10B: case Reg::R11B: case Reg::R12B: case Reg::R13B: case Reg::R14B: case Reg::R15B:

    case Reg::AX: case Reg::CX: case Reg::DX: case Reg::BX:
    case Reg::SP: case Reg::BP: case Reg::SI: case Reg::DI:
    case Reg::R8W: case Reg::R9W: case Reg::R10W: case Reg::R11W: case Reg::R12W: case Reg::R13W: case Reg::R14W: case Reg::R15W:
    case Reg::IP:
    case Reg::FLAGS:

    case Reg::EAX: case Reg::ECX: case Reg::EDX: case Reg::EBX:
    case Reg::ESP: case Reg::EBP: case Reg::ESI: case Reg::EDI:
    case Reg::R8D: case Reg::R9D: case Reg::R10D: case Reg::R11D: case Reg::R12D: case Reg::R13D: case Reg::R14D: case Reg::R15D:
    case Reg::EIP:
    case Reg::EFLAGS:

    case Reg::RAX: case Reg::RCX: case Reg::RDX: case Reg::RBX:
    case Reg::RSP: case Reg::RBP: case Reg::RSI: case Reg::RDI:
    case Reg::R8: case Reg::R9: case Reg::R10: case Reg::R11: case Reg::R12: case Reg::R13: case Reg::R14: case Reg::R15:
    case Reg::RIP:
    case Reg::RFLAGS:
    case Reg::CR0: case Reg::CR2: case Reg::CR3: case Reg::CR4: case Reg::CR8:
    case Reg::DR0: case Reg::DR1: case Reg::DR2: case Reg::DR3: case Reg::DR6: case Reg::DR7:
    case Reg::XCR0:
        result.u64 = value.Reg64;
        break;

    case Reg::ST0:
    case Reg::ST1:
    case Reg::ST2:
    case Reg::ST3:
    case Reg::ST4:
    case Reg::ST5:
    case Reg::ST6:
    case Reg::ST7:
        result.st.significand = value.Fp.Mantissa;
        result.st.exponentSign = value.Fp.BiasedExponent | (value.Fp.Sign << 15u);
        break;

    case Reg::MM0:
    case Reg::MM1:
    case Reg::MM2:
    case Reg::MM3:
    case Reg::MM4:
    case Reg::MM5:
    case Reg::MM6:
    case Reg::MM7:
        result.mm.i64[0] = value.Reg64;
        break;

    case Reg::XMM0:
    case Reg::XMM1:
    case Reg::XMM2:
    case Reg::XMM3:
    case Reg::XMM4:
    case Reg::XMM5:
    case Reg::XMM6:
    case Reg::XMM7:
    case Reg::XMM8:
    case Reg::XMM9:
    case Reg::XMM10:
    case Reg::XMM11:
    case Reg::XMM12:
    case Reg::XMM13:
    case Reg::XMM14:
    case Reg::XMM15:
        result.xmm.i64[0] = value.Reg128.Low64;
        result.xmm.i64[1] = value.Reg128.High64;
        break;
    }
    return result;
}

void TranslateRegisterValue(const Reg reg, const RegValue& value, WHV_REGISTER_VALUE& output) noexcept {
    switch (reg) {
    case Reg::CS: case Reg::SS: case Reg::DS: case Reg::ES: case Reg::FS: case Reg::GS:
    case Reg::LDTR: case Reg::TR:
        output.Segment.Selector = value.segment.selector;
        output.Segment.Base = value.segment.base;
        output.Segment.Limit = value.segment.limit;
        output.Segment.Attributes = value.segment.attributes.u16;
        break;

    case Reg::GDTR: case Reg::IDTR:
        output.Table.Base = value.table.base;
        output.Table.Limit = value.table.limit;
        break;

    case Reg::EFER:
        output.Reg64 = value.u64;
        break;

    case Reg::AL: case Reg::CL: case Reg::DL: case Reg::BL:
    case Reg::SPL: case Reg::BPL: case Reg::SIL: case Reg::DIL:
    case Reg::R8B: case Reg::R9B: case Reg::R10B: case Reg::R11B: case Reg::R12B: case Reg::R13B: case Reg::R14B: case Reg::R15B:
        output.Reg64 = SetLowByte(output.Reg64, value.u8);
        break;

    case Reg::AH: case Reg::CH: case Reg::DH: case Reg::BH:
        output.Reg64 = SetHighByte(output.Reg64, value.u8);
        break;

    case Reg::AX: case Reg::CX: case Reg::DX: case Reg::BX:
    case Reg::SP: case Reg::BP: case Reg::SI: case Reg::DI:
    case Reg::R8W: case Reg::R9W: case Reg::R10W: case Reg::R11W: case Reg::R12W: case Reg::R13W: case Reg::R14W: case Reg::R15W:
    case Reg::IP:
    case Reg::FLAGS:
        output.Reg64 = SetLowWord(output.Reg64, value.u16);
        break;

    case Reg::EAX: case Reg::ECX: case Reg::EDX: case Reg::EBX:
    case Reg::ESP: case Reg::EBP: case Reg::ESI: case Reg::EDI:
    case Reg::R8D: case Reg::R9D: case Reg::R10D: case Reg::R11D: case Reg::R12D: case Reg::R13D: case Reg::R14D: case Reg::R15D:
    case Reg::EIP:
    case Reg::EFLAGS:
        output.Reg64 = value.u32;
        break;

    case Reg::RAX: case Reg::RCX: case Reg::RDX: case Reg::RBX:
    case Reg::RSP: case Reg::RBP: case Reg::RSI: case Reg::RDI:
    case Reg::R8: case Reg::R9: case Reg::R10: case Reg::R11: case Reg::R12: case Reg::R13: case Reg::R14: case Reg::R15:
    case Reg::RIP:
    case Reg::RFLAGS:
    case Reg::CR0: case Reg::CR2: case Reg::CR3: case Reg::CR4: case Reg::CR8:
    case Reg::DR0: case Reg::DR1: case Reg::DR2: case Reg::DR3: case Reg::DR6: case Reg::DR7:
    case Reg::XCR0:
        output.Reg64 = value.u64;
        break;

    case Reg::ST0:
    case Reg::ST1:
    case Reg::ST2:
    case Reg::ST3:
    case Reg::ST4:
    case Reg::ST5:
    case Reg::ST6:
    case Reg::ST7:
        output.Fp.Mantissa = value.st.significand;
        output.Fp.BiasedExponent = value.st.exponentSign & 0x7FFF;
        output.Fp.Sign = value.st.exponentSign >> 15u;
        break;

    case Reg::MM0:
    case Reg::MM1:
    case Reg::MM2:
    case Reg::MM3:
    case Reg::MM4:
    case Reg::MM5:
    case Reg::MM6:
    case Reg::MM7:
        output.Reg64 = value.mm.i64[0];
        break;

    case Reg::XMM0:
    case Reg::XMM1:
    case Reg::XMM2:
    case Reg::XMM3:
    case Reg::XMM4:
    case Reg::XMM5:
    case Reg::XMM6:
    case Reg::XMM7:
    case Reg::XMM8:
    case Reg::XMM9:
    case Reg::XMM10:
    case Reg::XMM11:
    case Reg::XMM12:
    case Reg::XMM13:
    case Reg::XMM14:
    case Reg::XMM15:
        output.Reg128.Low64 = value.xmm.i64[0];
        output.Reg128.High64 = value.xmm.i64[1];
        break;
    }
}

bool TranslateMSR(uint64_t msr, WHV_REGISTER_NAME& reg) noexcept {
    // Only the following MSRs are supported:
    // TODO: make this list queryable
    //
    // MSR number   WHVP register
    // ---------------------------------------------------
    //  0000_0010   WHvX64RegisterTsc
    //  C000_0080   WHvX64RegisterEfer
    //  C000_0102   WHvX64RegisterKernelGsBase
    //  0000_001B   WHvX64RegisterApicBase
    //  0000_0277   WHvX64RegisterPat
    //  0000_0174   WHvX64RegisterSysenterCs
    //  0000_0176   WHvX64RegisterSysenterEip
    //  0000_0175   WHvX64RegisterSysenterEsp
    //  C000_0081   WHvX64RegisterStar
    //  C000_0082   WHvX64RegisterLstar
    //  C000_0083   WHvX64RegisterCstar
    //  C000_0084   WHvX64RegisterSfmask
    //
    //  0000_00FE   WHvX64RegisterMsrMtrrCap
    //  0000_02FF   WHvX64RegisterMsrMtrrDefType
    //
    //  0000_0200   WHvX64RegisterMsrMtrrPhysBase0
    //  0000_0202   WHvX64RegisterMsrMtrrPhysBase1
    //  0000_0204   WHvX64RegisterMsrMtrrPhysBase2
    //  0000_0206   WHvX64RegisterMsrMtrrPhysBase3
    //  0000_0208   WHvX64RegisterMsrMtrrPhysBase4
    //  0000_020A   WHvX64RegisterMsrMtrrPhysBase5
    //  0000_020C   WHvX64RegisterMsrMtrrPhysBase6
    //  0000_020E   WHvX64RegisterMsrMtrrPhysBase7
    //  0000_0210   WHvX64RegisterMsrMtrrPhysBase8
    //  0000_0212   WHvX64RegisterMsrMtrrPhysBase9
    //  0000_0214   WHvX64RegisterMsrMtrrPhysBaseA
    //  0000_0216   WHvX64RegisterMsrMtrrPhysBaseB
    //  0000_0218   WHvX64RegisterMsrMtrrPhysBaseC
    //  0000_021A   WHvX64RegisterMsrMtrrPhysBaseD
    //  0000_021C   WHvX64RegisterMsrMtrrPhysBaseE
    //  0000_021E   WHvX64RegisterMsrMtrrPhysBaseF
    //
    //  0000_0200   WHvX64RegisterMsrMtrrPhysMask0
    //  0000_0202   WHvX64RegisterMsrMtrrPhysMask1
    //  0000_0204   WHvX64RegisterMsrMtrrPhysMask2
    //  0000_0206   WHvX64RegisterMsrMtrrPhysMask3
    //  0000_0208   WHvX64RegisterMsrMtrrPhysMask4
    //  0000_020A   WHvX64RegisterMsrMtrrPhysMask5
    //  0000_020C   WHvX64RegisterMsrMtrrPhysMask6
    //  0000_020E   WHvX64RegisterMsrMtrrPhysMask7
    //  0000_0210   WHvX64RegisterMsrMtrrPhysMask8
    //  0000_0212   WHvX64RegisterMsrMtrrPhysMask9
    //  0000_0214   WHvX64RegisterMsrMtrrPhysMaskA
    //  0000_0216   WHvX64RegisterMsrMtrrPhysMaskB
    //  0000_0218   WHvX64RegisterMsrMtrrPhysMaskC
    //  0000_021A   WHvX64RegisterMsrMtrrPhysMaskD
    //  0000_021C   WHvX64RegisterMsrMtrrPhysMaskE
    //  0000_021E   WHvX64RegisterMsrMtrrPhysMaskF
    //
    //  0000_0250   WHvX64RegisterMsrMtrrFix64k00000
    //  0000_0258   WHvX64RegisterMsrMtrrFix16k80000
    //  0000_0259   WHvX64RegisterMsrMtrrFix16kA0000
    //  0000_0268   WHvX64RegisterMsrMtrrFix4kC0000
    //  0000_0269   WHvX64RegisterMsrMtrrFix4kC8000
    //  0000_026A   WHvX64RegisterMsrMtrrFix4kD0000
    //  0000_026B   WHvX64RegisterMsrMtrrFix4kD8000
    //  0000_026C   WHvX64RegisterMsrMtrrFix4kE0000
    //  0000_026D   WHvX64RegisterMsrMtrrFix4kE8000
    //  0000_026E   WHvX64RegisterMsrMtrrFix4kF0000
    //  0000_026F   WHvX64RegisterMsrMtrrFix4kF8000
    //
    //  C000_0103   WHvX64RegisterTscAux
    //  0000_0048   WHvX64RegisterSpecCtrl
    //  0000_0049   WHvX64RegisterPredCmd

    switch (msr) {
    case 0x00000010: reg = WHvX64RegisterTsc; break;
    case 0xC0000080: reg = WHvX64RegisterEfer; break;
    case 0xC0000102: reg = WHvX64RegisterKernelGsBase; break;
    case 0x0000001B: reg = WHvX64RegisterApicBase; break;
    case 0x00000277: reg = WHvX64RegisterPat; break;
    case 0x00000174: reg = WHvX64RegisterSysenterCs; break;
    case 0x00000176: reg = WHvX64RegisterSysenterEip; break;
    case 0x00000175: reg = WHvX64RegisterSysenterEsp; break;
    case 0xC0000081: reg = WHvX64RegisterStar; break;
    case 0xC0000082: reg = WHvX64RegisterLstar; break;
    case 0xC0000083: reg = WHvX64RegisterCstar; break;
    case 0xC0000084: reg = WHvX64RegisterSfmask; break;

    case 0x000000FE: reg = WHvX64RegisterMsrMtrrCap; break;
    case 0x000002FF: reg = WHvX64RegisterMsrMtrrDefType; break;

    case 0x00000200: reg = WHvX64RegisterMsrMtrrPhysBase0; break;
    case 0x00000202: reg = WHvX64RegisterMsrMtrrPhysBase1; break;
    case 0x00000204: reg = WHvX64RegisterMsrMtrrPhysBase2; break;
    case 0x00000206: reg = WHvX64RegisterMsrMtrrPhysBase3; break;
    case 0x00000208: reg = WHvX64RegisterMsrMtrrPhysBase4; break;
    case 0x0000020A: reg = WHvX64RegisterMsrMtrrPhysBase5; break;
    case 0x0000020C: reg = WHvX64RegisterMsrMtrrPhysBase6; break;
    case 0x0000020E: reg = WHvX64RegisterMsrMtrrPhysBase7; break;
    case 0x00000210: reg = WHvX64RegisterMsrMtrrPhysBase8; break;
    case 0x00000212: reg = WHvX64RegisterMsrMtrrPhysBase9; break;
    case 0x00000214: reg = WHvX64RegisterMsrMtrrPhysBaseA; break;
    case 0x00000216: reg = WHvX64RegisterMsrMtrrPhysBaseB; break;
    case 0x00000218: reg = WHvX64RegisterMsrMtrrPhysBaseC; break;
    case 0x0000021A: reg = WHvX64RegisterMsrMtrrPhysBaseD; break;
    case 0x0000021C: reg = WHvX64RegisterMsrMtrrPhysBaseE; break;
    case 0x0000021E: reg = WHvX64RegisterMsrMtrrPhysBaseF; break;

    case 0x00000201: reg = WHvX64RegisterMsrMtrrPhysMask0; break;
    case 0x00000203: reg = WHvX64RegisterMsrMtrrPhysMask1; break;
    case 0x00000205: reg = WHvX64RegisterMsrMtrrPhysMask2; break;
    case 0x00000207: reg = WHvX64RegisterMsrMtrrPhysMask3; break;
    case 0x00000209: reg = WHvX64RegisterMsrMtrrPhysMask4; break;
    case 0x0000020B: reg = WHvX64RegisterMsrMtrrPhysMask5; break;
    case 0x0000020D: reg = WHvX64RegisterMsrMtrrPhysMask6; break;
    case 0x0000020F: reg = WHvX64RegisterMsrMtrrPhysMask7; break;
    case 0x00000211: reg = WHvX64RegisterMsrMtrrPhysMask8; break;
    case 0x00000213: reg = WHvX64RegisterMsrMtrrPhysMask9; break;
    case 0x00000215: reg = WHvX64RegisterMsrMtrrPhysMaskA; break;
    case 0x00000217: reg = WHvX64RegisterMsrMtrrPhysMaskB; break;
    case 0x00000219: reg = WHvX64RegisterMsrMtrrPhysMaskC; break;
    case 0x0000021B: reg = WHvX64RegisterMsrMtrrPhysMaskD; break;
    case 0x0000021D: reg = WHvX64RegisterMsrMtrrPhysMaskE; break;
    case 0x0000021F: reg = WHvX64RegisterMsrMtrrPhysMaskF; break;

    case 0x00000250: reg = WHvX64RegisterMsrMtrrFix64k00000; break;
    case 0x00000258: reg = WHvX64RegisterMsrMtrrFix16k80000; break;
    case 0x00000259: reg = WHvX64RegisterMsrMtrrFix16kA0000; break;
    case 0x00000268: reg = WHvX64RegisterMsrMtrrFix4kC0000; break;
    case 0x00000269: reg = WHvX64RegisterMsrMtrrFix4kC8000; break;
    case 0x0000026A: reg = WHvX64RegisterMsrMtrrFix4kD0000; break;
    case 0x0000026B: reg = WHvX64RegisterMsrMtrrFix4kD8000; break;
    case 0x0000026C: reg = WHvX64RegisterMsrMtrrFix4kE0000; break;
    case 0x0000026D: reg = WHvX64RegisterMsrMtrrFix4kE8000; break;
    case 0x0000026E: reg = WHvX64RegisterMsrMtrrFix4kF0000; break;
    case 0x0000026F: reg = WHvX64RegisterMsrMtrrFix4kF8000; break;

    case 0xC0000103: reg = WHvX64RegisterTscAux; break;
    case 0x00000048: if (WHPX_MIN_VERSION(10_0_17763_0)) { reg = WHvX64RegisterSpecCtrl; break; } return false;
    case 0x00000049: if (WHPX_MIN_VERSION(10_0_17763_0)) { reg = WHvX64RegisterPredCmd; break; } return false;
    default: return false;
    }

    return true;
}

}
