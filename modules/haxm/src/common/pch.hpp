/*
Precompiled header for the virt86 HAXM hypervisor platform adapter.
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
#include "virt86/vm/vm.hpp"
#include "virt86/vp/vp.hpp"

#include <memory>
#include <cstdint>
#include <cassert>
#include <string>
#include <sstream>

#ifdef _WIN32
#  include <Windows.h>
#endif

#include "interface/hax_interface.hpp"

#include "virt86/haxm/haxm_platform.hpp"
#include "haxm_vm.hpp"
#include "haxm_vp.hpp"

#include "haxm_helpers.hpp"
#include "haxm_platform_impl.hpp"
