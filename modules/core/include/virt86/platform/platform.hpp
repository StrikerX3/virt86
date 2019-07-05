/*
Defines the Platform interface -- the entry point of the library.

A Platform represents a hypervisor platform which can virtualize the x86
processor. Platforms expose a set of optional features, which may be queried
through the GetFeatures() method.

The name of the platform can be retrieved with the GetName() method.

Before using a platform, you must check the initialization status by calling
the GetInitStatus() method. If the return value is not PlatformInitStatus::OK,
the platform has not been initialized successfully.

The main use of a Platform is to create virtual machines. Create a
VMSpecifications struct to specify parameters such as the number of virtual
processors, additional VM exit conditions or custom CPUIDs. Call the CreateVM()
method with it and a pointer to a VirtualMachine pointer variable. If the
virtual machine is created, the function returns true and pointer will refer to
the newly created VM.

Please note that Platform instances are not meant to be used concurrently
by multiple threads. You will need to provide your own concurrency control
mechanism if you wish to share instances between threads.
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

#include "features.hpp"
#include "../vm/vm.hpp"

#include <vector>
#include <optional>
#include <memory>

namespace virt86 {

/**
 * Indicates the platform initialization status.
 */
enum class PlatformInitStatus {
    Uninitialized,  // Platform is uninitialized
    OK,             // Platform initialized successfully
    Unavailable,    // Platform is unavailable
    Unsupported,    // Platform is unsupported on the host
    Failed,         // Initialization failed for another reason
};

/**
 * Defines the interface for a virtualization platform.
 *
 * Plaforms are not meant to be instantiated in user programs.
 * Instead, each platform library will provide its own singleton instance of a
 * derived class of this class.
 */
class Platform {
public:
    // Prevent copy construction and copy assignment
    Platform(const Platform&) = delete;
    Platform& operator=(const Platform&) = delete;

    // Prevent move construction and move assignment
    Platform(Platform&&) = delete;
    Platform&& operator=(Platform&&) = delete;

    // Disallow taking the address
    Platform *operator&() = delete;

    virtual ~Platform() noexcept;

    /**
     * Retrieves the platform's name.
     */
    std::string GetName() const { return m_name; }

    /**
     * If initialized successfully, returns the platform's version as detected
     * in the system, otherwise returns an empty string.
     */
    std::string GetVersion() const noexcept { return m_version; }

    /**
     * Retrieves the platform's initialization status.
     */
    const PlatformInitStatus GetInitStatus() const noexcept { return m_initStatus; }

    /**
     * Retrieves the platform's features available and enabled on the host.
     */
    const PlatformFeatures& GetFeatures() const noexcept { return m_features; }

    /**
     * Creates a new virtual machine with the specified parameters.
     *
     * The optional contains a reference to the newly created virtual machine
     * if it is successfully created.
     */
    const std::optional<std::reference_wrapper<VirtualMachine>> CreateVM(const VMSpecifications& specifications);

    /**
     * Destroys the virtual machine if it was created with this platform.
     *
     * Returns true if the virtual machine was successfully destroyed.
     */
    const bool FreeVM(VirtualMachine& vm);

protected:
    Platform(const char *name) noexcept;

    /**
     * Destroys all virtual machines created with the platform.
     *
     * This method is meant to be used in destructors to ensure resources are
     * released in the proper order.
     */
    void DestroyVMs() noexcept;

    /**
     * Instantiates and initializes a virtual machine from the given
     * specifications.
     */
    virtual std::unique_ptr<VirtualMachine> CreateVMImpl(const VMSpecifications& specifications) = 0;

    /**
     * The platform's name.
     */
    const char *m_name;

    /**
     * The platform's version if available in the system, or an empty string
     * if the platform was not initialized.
     */
    std::string m_version;

    /**
     * The platform's initialization status.
     */
    PlatformInitStatus m_initStatus;

    /**
     * The platform's features.
     */
    PlatformFeatures m_features;

private:
    /**
     * Stores all virtual machines created with this platform
     */
    std::vector<std::unique_ptr<VirtualMachine>> m_vms;
};

}
