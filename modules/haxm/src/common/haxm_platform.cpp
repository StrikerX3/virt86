/*
Implementation of the HAXM Platform class.
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
#include "virt86/haxm/haxm_platform.hpp"
#include "haxm_vm.hpp"
#include "haxm_platform_impl.hpp"

namespace virt86::haxm {

HaxmPlatform& HaxmPlatform::Instance() noexcept {
    static HaxmPlatform instance;
    return instance;
}

HaxmPlatform::HaxmPlatform() noexcept
    : Platform("Intel HAXM")
    , m_delegate(std::make_unique<Delegate>())
{
    auto& impl = m_delegate->impl;

    const auto initResult = impl.Initialize();
    if (initResult != PlatformInitStatus::OK) {
        m_initStatus = initResult;
        return;
    }
    
    // Publish capabilities
    const auto& caps = impl.m_haxCaps;
    if (caps.wstatus & HAX_CAP_STATUS_WORKING) {
        m_initStatus = PlatformInitStatus::OK;
        m_features.maxProcessorsPerVM = 64;
        m_features.maxProcessorsGlobal = 128;
        m_features.floatingPointExtensions = FloatingPointExtension::SSE2;
        m_features.unrestrictedGuest = (caps.winfo & HAX_CAP_UG) != 0;
        m_features.extendedPageTables = (caps.winfo & HAX_CAP_EPT) != 0;
        m_features.guestDebugging = (caps.winfo & HAX_CAP_DEBUG) != 0;
        m_features.guestMemoryProtection = (caps.winfo & HAX_CAP_RAM_PROTECTION) != 0;
        m_features.largeMemoryAllocation = (caps.winfo & HAX_CAP_64BIT_SETRAM) != 0;
        m_features.memoryUnmapping = true;
        m_features.partialUnmapping = true;  // TODO: since when?
        m_features.partialMMIOInstructions = true;
        m_features.extendedControlRegisters =  ExtendedControlRegister::MXCSRMask;
    }
    else {
        m_initStatus = PlatformInitStatus::Unsupported;
    }
}

HaxmPlatform::~HaxmPlatform() noexcept {
    DestroyVMs();
}

bool HaxmPlatform::SetGlobalMemoryLimit(bool enabled, uint64_t limitMB) noexcept {
    return m_delegate->impl.SetGlobalMemoryLimit(enabled, limitMB);
}

std::unique_ptr<VirtualMachine> HaxmPlatform::CreateVMImpl(const VMSpecifications& specifications) {
    auto vm = std::make_unique<HaxmVirtualMachine>(*this, specifications, m_delegate->impl);
    if (!vm->Initialize()) {
        return nullptr;
    }
    return vm;
}

}
