/*
Declares the VirtualProcessor implementation class for the Windows Hypervisor
Platform adapter.
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
#pragma once

#include "virt86/vp/vp.hpp"

#include <Windows.h>
#include <WinHvPlatform.h>
#include <WinHvEmulation.h>

namespace virt86::whpx {

class WhpxVirtualMachine;

class WhpxVirtualProcessor : public VirtualProcessor {
public:
    VPExecutionStatus RunImpl() override;

    bool PrepareInterrupt(uint8_t vector) override;
    VPOperationStatus InjectInterrupt(uint8_t vector) override;
    bool CanInjectInterrupt() const override;
    void RequestInterruptWindow() override;

    VPOperationStatus RegRead(const Reg reg, RegValue& value) override;
    VPOperationStatus RegWrite(const Reg reg, const RegValue& value) override;
    VPOperationStatus RegRead(const Reg regs[], RegValue values[], const size_t numRegs) override;
    VPOperationStatus RegWrite(const Reg regs[], const RegValue values[], const size_t numRegs) override;

    VPOperationStatus GetFPUControl(FPUControl& value) override;
    VPOperationStatus SetFPUControl(const FPUControl& value) override;

    VPOperationStatus GetMXCSR(MXCSR& value) override;
    VPOperationStatus SetMXCSR(const MXCSR& value) override;

    VPOperationStatus GetMXCSRMask(MXCSR& value) override;
    VPOperationStatus SetMXCSRMask(const MXCSR& value) override;

    VPOperationStatus GetMSR(const uint64_t msr, uint64_t& value) override;
    VPOperationStatus SetMSR(const uint64_t msr, const uint64_t value) override;
    VPOperationStatus GetMSRs(const uint64_t msrs[], uint64_t values[], const size_t numRegs) override;
    VPOperationStatus SetMSRs(const uint64_t msrs[], const uint64_t values[], const size_t numRegs) override;

protected:
    WhpxVirtualProcessor(WhpxVirtualMachine& vm, uint32_t id);
    ~WhpxVirtualProcessor() override;

private:
    WhpxVirtualMachine& m_vm;

    uint32_t m_id;
    WHV_EMULATOR_HANDLE m_emuHandle;
    WHV_RUN_VP_EXIT_CONTEXT m_exitContext;

    static HRESULT WINAPI GetVirtualProcessorRegistersCallback(VOID* Context, const WHV_REGISTER_NAME* RegisterNames, UINT32 RegisterCount, WHV_REGISTER_VALUE* RegisterValues);
    static HRESULT WINAPI SetVirtualProcessorRegistersCallback(VOID* Context, const WHV_REGISTER_NAME* RegisterNames, UINT32 RegisterCount, const WHV_REGISTER_VALUE* RegisterValues);
    static HRESULT WINAPI TranslateGvaPageCallback(VOID* Context, WHV_GUEST_VIRTUAL_ADDRESS Gva, WHV_TRANSLATE_GVA_FLAGS TranslateFlags, WHV_TRANSLATE_GVA_RESULT_CODE* TranslationResult, WHV_GUEST_PHYSICAL_ADDRESS* Gpa);
    static HRESULT WINAPI IoPortCallback(VOID* Context, WHV_EMULATOR_IO_ACCESS_INFO* IoAccess);
    static HRESULT WINAPI MemoryCallback(VOID* Context, WHV_EMULATOR_MEMORY_ACCESS_INFO* MemoryAccess);

    HRESULT HandleIO(WHV_EMULATOR_IO_ACCESS_INFO *IoAccess);
    HRESULT HandleMMIO(WHV_EMULATOR_MEMORY_ACCESS_INFO *MemoryAccess);

    bool Initialize();

    // Allow WhpxVirtualMachine to access the constructor and Initialize()
    friend class WhpxVirtualMachine;

    // ----- Helper methods ---------------------------------------------------

    VPExecutionStatus HandleIO();
    VPExecutionStatus HandleMMIO();
};

}
