/*
Declares the implementation class for the Windows Hypervisor Platform adapter.

Include this file to use WHPX as a platform. The platform instance is exposed
as a singleton:

  auto& instance = virt86::whpx::WhpxPlatform::Instance();
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

namespace virt86::whpx {

struct WhpxDispatch;

class WhpxPlatform : public Platform {
public:
    ~WhpxPlatform() noexcept final;

    // Prevent copy construction and copy assignment
    WhpxPlatform(const WhpxPlatform&) = delete;
    WhpxPlatform& operator=(const WhpxPlatform&) = delete;

    // Prevent move construction and move assignment
    WhpxPlatform(WhpxPlatform&&) = delete;
    WhpxPlatform&& operator=(WhpxPlatform&&) = delete;

    // Disallow taking the address
    WhpxPlatform *operator&() = delete;

    static WhpxPlatform& Instance() noexcept;

protected:
    std::unique_ptr<VirtualMachine> CreateVMImpl(const VMSpecifications& specifications) override;

private:
    WhpxPlatform() noexcept;

    static WhpxDispatch *s_dispatch;
};

}
