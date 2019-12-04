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
#include "whpx_defs.hpp"

namespace virt86::whpx {

class WhpxVirtualMachine;

class WhpxVirtualProcessor : public VirtualProcessor {
public:
    WhpxVirtualProcessor(WhpxVirtualMachine& vm, const WhpxDispatch& dispatch, uint32_t id);
    ~WhpxVirtualProcessor() noexcept final;

    // Prevent copy construction and copy assignment
    WhpxVirtualProcessor(const WhpxVirtualProcessor&) = delete;
    WhpxVirtualProcessor& operator=(const WhpxVirtualProcessor&) = delete;

    // Prevent move construction and move assignment
    WhpxVirtualProcessor(WhpxVirtualProcessor&&) = delete;
    WhpxVirtualProcessor&& operator=(WhpxVirtualProcessor&&) = delete;

    // Disallow taking the address
    WhpxVirtualProcessor *operator&() = delete;

    VPExecutionStatus RunImpl() noexcept override;

    bool PrepareInterrupt(uint8_t vector) noexcept override;
    VPOperationStatus InjectInterrupt(uint8_t vector) noexcept override;
    bool CanInjectInterrupt() const noexcept override;
    void RequestInterruptWindow() noexcept override;

    VPOperationStatus RegRead(const Reg reg, RegValue& value) noexcept override;
    VPOperationStatus RegWrite(const Reg reg, const RegValue& value) noexcept override;
    VPOperationStatus RegRead(const Reg regs[], RegValue values[], const size_t numRegs) noexcept override;
    VPOperationStatus RegWrite(const Reg regs[], const RegValue values[], const size_t numRegs) noexcept override;

    VPOperationStatus GetFPUControl(FPUControl& value) noexcept override;
    VPOperationStatus SetFPUControl(const FPUControl& value) noexcept override;

    VPOperationStatus GetMXCSR(MXCSR& value) noexcept override;
    VPOperationStatus SetMXCSR(const MXCSR& value) noexcept override;

    VPOperationStatus GetMXCSRMask(MXCSR& value) noexcept override;
    VPOperationStatus SetMXCSRMask(const MXCSR& value) noexcept override;

    VPOperationStatus GetMSR(const uint64_t msr, uint64_t& value) noexcept override;
    VPOperationStatus SetMSR(const uint64_t msr, const uint64_t value) noexcept override;
    VPOperationStatus GetMSRs(const uint64_t msrs[], uint64_t values[], const size_t numRegs) noexcept override;
    VPOperationStatus SetMSRs(const uint64_t msrs[], const uint64_t values[], const size_t numRegs) noexcept override;

private:
    WhpxVirtualMachine& m_vm;
    const WhpxDispatch& m_dispatch;

    uint32_t m_id;
    WHV_EMULATOR_HANDLE m_emuHandle;
    WHV_RUN_VP_EXIT_CONTEXT m_exitContext;

    static HRESULT WINAPI GetVirtualProcessorRegistersCallback(VOID* Context, const WHV_REGISTER_NAME* RegisterNames, UINT32 RegisterCount, WHV_REGISTER_VALUE* RegisterValues) noexcept;
    static HRESULT WINAPI SetVirtualProcessorRegistersCallback(VOID* Context, const WHV_REGISTER_NAME* RegisterNames, UINT32 RegisterCount, const WHV_REGISTER_VALUE* RegisterValues) noexcept;
    static HRESULT WINAPI TranslateGvaPageCallback(VOID* Context, WHV_GUEST_VIRTUAL_ADDRESS Gva, WHV_TRANSLATE_GVA_FLAGS TranslateFlags, WHV_TRANSLATE_GVA_RESULT_CODE* TranslationResult, WHV_GUEST_PHYSICAL_ADDRESS* Gpa) noexcept;
    static HRESULT WINAPI IoPortCallback(VOID* Context, WHV_EMULATOR_IO_ACCESS_INFO* IoAccess) noexcept;
    static HRESULT WINAPI MemoryCallback(VOID* Context, WHV_EMULATOR_MEMORY_ACCESS_INFO* MemoryAccess) noexcept;

    HRESULT HandleIO(WHV_EMULATOR_IO_ACCESS_INFO *IoAccess) noexcept;
    HRESULT HandleMMIO(WHV_EMULATOR_MEMORY_ACCESS_INFO *MemoryAccess) noexcept;

    bool Initialize() noexcept;

    // Allow WhpxVirtualMachine to access the constructor and Initialize()
    friend class WhpxVirtualMachine;

    // ----- Helper methods ---------------------------------------------------

    VPExecutionStatus HandleIO() noexcept;
    VPExecutionStatus HandleMMIO() noexcept;
    void HandleMSRAccess() noexcept;
    void HandleTSCAccess() noexcept;
    void HandleCPUIDAccess() noexcept;
};

}
