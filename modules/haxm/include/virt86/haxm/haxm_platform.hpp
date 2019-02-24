/*
Declares the implementation class for the HAXM hypervisor platform adapter.

Include this file to use HAXM as a platform. The platform instance is exposed
as a singleton:

  auto& instance = virt86::haxm::HaxmPlatform::Instance();
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

#include "virt86/platform/platform.hpp"

#include <memory>

namespace virt86::haxm {

class HaxmPlatform : public Platform {
public:
    ~HaxmPlatform() noexcept final;

    // Prevent copy construction and copy assignment
    HaxmPlatform(const HaxmPlatform&) = delete;
    HaxmPlatform& operator=(const HaxmPlatform&) = delete;

    // Prevent move construction and move assignment
    HaxmPlatform(HaxmPlatform&&) = delete;
    HaxmPlatform&& operator=(HaxmPlatform&&) = delete;

    // Disallow taking the address
    HaxmPlatform *operator&() = delete;

    static HaxmPlatform& Instance() noexcept;

    bool SetGlobalMemoryLimit(bool enabled, uint64_t limitMB) noexcept;

protected:
    std::unique_ptr<VirtualMachine> CreateVMImpl(const VMSpecifications& specifications) override;

private:
    HaxmPlatform() noexcept;

    struct Delegate;
    std::unique_ptr<Delegate> m_delegate;
};

}
