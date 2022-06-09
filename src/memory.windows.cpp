#include "utility/memory.hpp"

#include <Windows.h>
#include <limits>
#include <cstring>
#include <memoryapi.h>

#undef max // WinDef.h defines a max macro which collides with any function named max (i.e. numeric_limits)

std::unique_ptr<char[]> lime::read(const std::uintptr_t &address, const std::size_t &size)
{
    auto buffer = std::make_unique<char[]>(size);
    std::memcpy(buffer.get(), reinterpret_cast<void *>(address), size);

    return buffer;
}

bool lime::write(const std::uintptr_t &address, const void *data, const std::size_t &size)
{
    std::memcpy(reinterpret_cast<void *>(address), data, size);
    return true;
}

bool lime::free(const std::uintptr_t &address, const std::size_t &size)
{
    return VirtualFree(reinterpret_cast<void *>(address), size, MEM_RELEASE);
}

bool lime::protect(const std::uintptr_t &address, const std::size_t &size, const std::uint8_t &prot)
{
    [[maybe_unused]] DWORD old_prot = 0; //? Required otherwise the call fails.
    return VirtualProtect(reinterpret_cast<void *>(address), size, prot, &old_prot);
}

std::shared_ptr<std::uintptr_t> lime::allocate(const std::size_t &size, const std::uint8_t &prot)
{
    auto *rtn = VirtualAlloc(nullptr, size, MEM_COMMIT | MEM_RESERVE, prot);

    if (rtn)
    {
        return {new auto(reinterpret_cast<std::uintptr_t>(rtn)), [size](auto *data) { free(*data, size); }};
    }

    return nullptr;
}

bool lime::allocate_at(const std::uintptr_t &address, const std::size_t &size, const std::uint8_t &prot)
{
    return VirtualAlloc(reinterpret_cast<void *>(address), size, MEM_COMMIT | MEM_RESERVE, prot) != nullptr;
}

std::shared_ptr<std::uintptr_t> lime::allocate_near(const std::uintptr_t &address, const std::size_t &size, const std::uint8_t &prot)
{
    MEM_ADDRESS_REQUIREMENTS requirements{};
    MEM_EXTENDED_PARAMETER param{};

    SYSTEM_INFO si{};
    GetSystemInfo(&si);
    auto allocation_granularity = si.dwAllocationGranularity;

    auto min = static_cast<std::intptr_t>(address - std::numeric_limits<std::int32_t>::max());
    min += (allocation_granularity - (min % allocation_granularity));

    auto max = static_cast<std::intptr_t>(address + std::numeric_limits<std::int32_t>::max());
    max -= (max % allocation_granularity) + 1;

    requirements.Alignment = 0;
    requirements.LowestStartingAddress = min < 0 ? nullptr : reinterpret_cast<void *>(min);
    requirements.HighestEndingAddress = reinterpret_cast<void *>(max);

    param.Type = MemExtendedParameterAddressRequirements;
    param.Pointer = &requirements;

    auto virtual_alloc_2 = reinterpret_cast<decltype(VirtualAlloc2) *>(GetProcAddress(LoadLibraryA("kernelbase.dll"), "VirtualAlloc2"));
    auto *allocated = virtual_alloc_2(GetCurrentProcess(), nullptr, size, MEM_COMMIT | MEM_RESERVE, prot, &param, 1);

    if (allocated)
    {
        return {new auto(reinterpret_cast<std::uintptr_t>(allocated)), [size](auto *data) { free(*data, size); }};
    }

    return nullptr;
}