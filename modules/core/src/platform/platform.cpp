/*
Base implementation of the Platform interface.
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
#include "virt86/platform/platform.hpp"

namespace virt86 {

Platform::Platform(const char *name) noexcept
    : m_name(name)
    , m_initStatus(PlatformInitStatus::Uninitialized)
{
}

Platform::~Platform() noexcept {
}

const std::optional<std::reference_wrapper<VirtualMachine>> Platform::CreateVM(const VMSpecifications& specifications) {
    auto vm = CreateVMImpl(specifications);
    if (vm != nullptr) {
        return *m_vms.emplace_back(std::move(vm));
    }

    return std::nullopt;
}

const bool Platform::FreeVM(VirtualMachine& vm) {
    for (auto it = m_vms.cbegin(); it != m_vms.cend(); it++) {
        if (it->get() == &vm) {
            m_vms.erase(it);
            return true;
        }
    }

    return false;
}

void Platform::DestroyVMs() noexcept {
    m_vms.clear();
}

}
