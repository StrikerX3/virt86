/*
Implementation of the KVM Platform class.
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
#include "virt86/kvm/kvm_platform.hpp"
#include "kvm_vm.hpp"
#include "kvm_helpers.hpp"

#include "virt86/util/host_info.hpp"

#include <fcntl.h>
#include <linux/kvm.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <memory>

namespace virt86::kvm {

KvmPlatform& KvmPlatform::Instance() noexcept {
    static KvmPlatform instance;
    return instance;
}

KvmPlatform::KvmPlatform() noexcept
    : Platform("KVM")
    , m_fd(-1)
{
    // Open the KVM module
    m_fd = open("/dev/kvm", O_RDWR);
    if (m_fd < 0) {
        m_initStatus = PlatformInitStatus::Unavailable;
        return;
    }

    // Get KVM version
    int kvmVersion;

    // Refuse to run if API version != 12 as per API docs recommendation
    kvmVersion = ioctl(m_fd, KVM_GET_API_VERSION, nullptr);
    if (kvmVersion != 12) {
        m_initStatus = PlatformInitStatus::Unsupported;
        return;
    }

    // Check if have the capabilities we need
    int kvmCapResult;

    // User mem to VM mapping
    kvmCapResult = ioctl(m_fd, KVM_CHECK_EXTENSION, KVM_CAP_USER_MEMORY);
    if (kvmCapResult == 0) {
        m_initStatus = PlatformInitStatus::Unsupported;
        return;
    }

    // Set identity map address
    kvmCapResult = ioctl(m_fd, KVM_CHECK_EXTENSION, KVM_CAP_SET_IDENTITY_MAP_ADDR);
    if (kvmCapResult == 0) {
        m_initStatus = PlatformInitStatus::Unsupported;
        return;
    }


    // If we've got this far, we're all good
    m_initStatus = PlatformInitStatus::OK;

    // Check and publish additional capabilities

    // Maximum number of VCPUs
    kvmCapResult = ioctl(m_fd, KVM_CHECK_EXTENSION, KVM_CAP_NR_VCPUS);
    m_features.maxProcessorsPerVM = static_cast<uint32_t>((kvmCapResult == 0) ? 4 : kvmCapResult);

    kvmCapResult = ioctl(m_fd, KVM_CHECK_EXTENSION, KVM_CAP_MAX_VCPUS);
    m_features.maxProcessorsGlobal = (kvmCapResult == 0) ? m_features.maxProcessorsPerVM : kvmCapResult;

    m_features.guestPhysicalAddress.maxBits = HostInfo.gpa.maxBits;
    m_features.guestPhysicalAddress.maxAddress = HostInfo.gpa.maxAddress;
    m_features.guestPhysicalAddress.mask = HostInfo.gpa.mask;

    m_features.unrestrictedGuest = true;
    m_features.extendedPageTables = true;
    m_features.guestDebugging = ioctl(m_fd, KVM_CAP_DEBUGREGS) != 0 && ioctl(m_fd, KVM_CAP_SET_GUEST_DEBUG) != 0;
    m_features.dirtyPageTracking = true;
    m_features.partialDirtyBitmap = false;
    m_features.largeMemoryAllocation = true;
    m_features.partialUnmapping = false;
    m_features.memoryAliasing = true;
    m_features.memoryUnmapping = false;
    m_features.partialMMIOInstructions = false;
    m_features.floatingPointExtensions = HostInfo.floatingPointExtensions;
    m_features.extendedControlRegisters = ExtendedControlRegister::CR8 | ExtendedControlRegister::XCR0;
    m_features.extendedVMExits = ExtendedVMExit::Exception;
    m_features.exceptionExits = ExceptionCode::All;
    m_features.customCPUIDs = ioctl(m_fd, KVM_CHECK_EXTENSION, KVM_CAP_EXT_CPUID) != 0;

    // Get list of supported CPUIDs
    if (m_features.customCPUIDs) {
        // Start with a reasonably large starting value.
        // If the ioctl fails, check the error code:
        // - E2BIG indicates that the array is too small
        // - ENOMEM indicates that the array is too large; nent is adjusted to
        //   the correct size
        uint32_t count = 40;
        do {
            auto cpuid2 = allocVarEntry<kvm_cpuid2, kvm_cpuid_entry2>(count);
            cpuid2->nent = count;
            if (ioctl(m_fd, KVM_GET_SUPPORTED_CPUID, cpuid2) < 0) {
                if (errno == E2BIG) {
                    // Array is too small
                    // Double its size and retry
                    count *= 2;
                    free(cpuid2);
                    continue;
                }
                if (errno == ENOMEM) {
                    // Array is too large
                    count = cpuid2->nent;
                    free(cpuid2);
                    continue;
                }
                // Another error occurred, fail initialization
                free(cpuid2);
                m_initStatus = PlatformInitStatus::Failed;
                return;
            }

            // If we get to this point, the ioctl succeeded
            for (size_t i = 0; i < cpuid2->nent; i++) {
                auto &entry = cpuid2->entries[i];
                m_features.supportedCustomCPUIDs.emplace_back(entry.function, entry.eax, entry.ebx, entry.ecx, entry.edx);
            }
            free(cpuid2);
            break;
        } while (true);
    }
}

KvmPlatform::~KvmPlatform() noexcept {
    DestroyVMs();
    if (m_fd != -1) {
        close(m_fd);
        m_fd = -1;
    }
}

std::unique_ptr<VirtualMachine> KvmPlatform::CreateVMImpl(const VMSpecifications& specifications) {
    auto vm = std::make_unique<KvmVirtualMachine>(*this, specifications, m_fd);
    if (!vm->Initialize()) {
        return nullptr;
    }
    return vm;
}

}
