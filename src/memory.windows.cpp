#include <Windows.h>
#include <cstring>
#include <utility/memory.hpp>

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
    MEMORY_BASIC_INFORMATION info;
    void *page_address = reinterpret_cast<void *>(address);

    while (VirtualQuery(page_address, &info, sizeof(info)))
    {
        const auto diff = reinterpret_cast<std::int64_t>(page_address) - static_cast<std::int64_t>(address);
        if (diff != static_cast<std::int32_t>(diff))
            break;

        if (info.State & MEM_FREE)
        {

            if (allocate_at(reinterpret_cast<std::uintptr_t>(page_address), size, prot))
            {
                return {new auto(reinterpret_cast<std::uintptr_t>(page_address)), [size](auto *data) { free(*data, size); }};
            }
        }

        page_address = reinterpret_cast<void *>(reinterpret_cast<std::uintptr_t>(info.BaseAddress) + info.RegionSize);
    }

    return nullptr;
}
