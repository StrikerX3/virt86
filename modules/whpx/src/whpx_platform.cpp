/*
Implementation of the Platform class for the Windows Hypervisor Platform
adapter.
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
#include "virt86/whpx/whpx_platform.hpp"
#include "whpx_vm.hpp"
#include "whpx_dispatch.hpp"
#include "whpx_defs.hpp"

namespace virt86::whpx {

WhpxDispatch *WhpxPlatform::s_dispatch = new(std::nothrow) WhpxDispatch();

WhpxPlatform& WhpxPlatform::Instance() noexcept {
    static WhpxPlatform instance;
    return instance;
}

WhpxPlatform::WhpxPlatform() noexcept
    : Platform("Microsoft Windows Hypervisor Platform")
{
    if (s_dispatch == nullptr) {
        m_initStatus = PlatformInitStatus::Failed;
        return;
    }

    if (!s_dispatch->Load()) {
        m_initStatus = PlatformInitStatus::Unavailable;
        return;
    }

    WHV_CAPABILITY cap;

    // Check for presence of the hypervisor platform
    UINT32 size;
    HRESULT hr = s_dispatch->WHvGetCapability(WHvCapabilityCodeHypervisorPresent, &cap, sizeof(WHV_CAPABILITY), &size);
    if (S_OK != hr) {
        m_initStatus = PlatformInitStatus::Failed;
    }
    if (!cap.HypervisorPresent) {
        m_initStatus = PlatformInitStatus::Unavailable;
        return;
    }

    // Retrieve and publish feature set
    hr = s_dispatch->WHvGetCapability(WHvCapabilityCodeFeatures, &cap, sizeof(WHV_CAPABILITY), &size);
    if (S_OK != hr) {
        m_initStatus = PlatformInitStatus::Failed;
    }

    const WhpxDefs::WHV_CAPABILITY_FEATURES features = cap.Features;
    m_features.floatingPointExtensions = FloatingPointExtension::SSE2;
    m_features.extendedControlRegisters = ExtendedControlRegister::XCR0 | ExtendedControlRegister::CR8 | ExtendedControlRegister::MXCSRMask;
    m_features.maxProcessorsPerVM = 64; // TODO: check value
    m_features.maxProcessorsGlobal = 128; // TODO: check value
    m_features.unrestrictedGuest = true;
    m_features.extendedPageTables = true;
    m_features.largeMemoryAllocation = true;
    m_features.customCPUIDs = true;
    m_features.dirtyPageTracking = features.DirtyPageTracking;
    m_features.partialDirtyBitmap = true;
    m_features.partialUnmapping = features.PartialUnmap;
    m_features.memoryAliasing = true; // TODO: check if this is true on every version of WHPX; 17763 supports it
    m_features.memoryUnmapping = true;
    m_initStatus = PlatformInitStatus::OK;

    // Retrieve and publish extended VM exits
    hr = s_dispatch->WHvGetCapability(WHvCapabilityCodeExtendedVmExits, &cap, sizeof(WHV_CAPABILITY), &size);
    if (S_OK != hr) {
        m_initStatus = PlatformInitStatus::Failed;
    }

    if (cap.ExtendedVmExits.X64CpuidExit) {
        m_features.extendedVMExits |= ExtendedVMExit::CPUID;
    }
    if (cap.ExtendedVmExits.X64MsrExit) {
        m_features.extendedVMExits |= ExtendedVMExit::MSRAccess;
    }
    if (cap.ExtendedVmExits.ExceptionExit) {
        m_features.extendedVMExits |= ExtendedVMExit::Exception;
        hr = s_dispatch->WHvGetCapability(WHvCapabilityCodeExceptionExitBitmap, &cap, sizeof(WHV_CAPABILITY), &size);
        if (S_OK != hr) {
            m_initStatus = PlatformInitStatus::Failed;
        }
        m_features.exceptionExits = static_cast<ExceptionCode>(cap.ExceptionExitBitmap);
    }
}

WhpxPlatform::~WhpxPlatform() noexcept {
    if (m_initStatus == PlatformInitStatus::OK) {
        DestroyVMs();
    }
    delete s_dispatch;
}

std::unique_ptr<VirtualMachine> WhpxPlatform::CreateVMImpl(const VMSpecifications& specifications) {
    auto vm = std::make_unique<WhpxVirtualMachine>(*this, *s_dispatch, specifications);
    if (!vm->Initialize()) {
        return nullptr;
    }
    return vm;
}

}
