#include "utility/memory.hpp"

#include <cstring>
#include <sys/mman.h>
#include <sys/unistd.h>

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
    return munmap(reinterpret_cast<void *>(address), size) == 0;
}

bool lime::protect(const std::uintptr_t &address, const std::size_t &size, const std::uint8_t &prot)
{
    return mprotect(reinterpret_cast<void *>(address), size, prot) == 0;
}

std::shared_ptr<std::uintptr_t> lime::allocate(const std::size_t &size, const std::uint8_t &prot)
{
    auto *rtn = mmap(nullptr, size, prot, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if (rtn != reinterpret_cast<void *>(-1))
        return {new auto(reinterpret_cast<std::uintptr_t>(rtn)), [size](const std::uintptr_t *data) {
                    free(*data, size);
                    delete data;
                }};

    return nullptr;
}

bool lime::allocate_at(const std::uintptr_t &address, const std::size_t &size, const std::uint8_t &prot)
{
    return mmap(reinterpret_cast<void *>(address), size, prot, MAP_PRIVATE | MAP_ANON | MAP_FIXED_NOREPLACE, -1, 0) != reinterpret_cast<void *>(-1);
}

std::shared_ptr<std::uintptr_t> lime::allocate_near(const std::uintptr_t &address, const std::size_t &size, const std::uint8_t &prot)
{
    auto aligned = address & (getpagesize() - 1) ? (address + getpagesize()) & ~(getpagesize() - 1) : address;
    auto *rtn = reinterpret_cast<void *>(-1);
    std::size_t skipped_pages = 0;

    while (true)
    {
        const auto diff = static_cast<std::int64_t>(aligned) - static_cast<std::int64_t>(address);
        if (diff != static_cast<std::int32_t>(diff))
            break;

        rtn = mmap(reinterpret_cast<void *>(aligned), size, prot, MAP_PRIVATE | MAP_ANON | MAP_FIXED_NOREPLACE, -1, 0);

        aligned = (aligned + getpagesize()) & ~(getpagesize() - 1);
        skipped_pages++;
    }

    if (rtn != reinterpret_cast<void *>(-1))
    {
        const auto allocated_page = reinterpret_cast<std::uintptr_t>(rtn);

        return {new auto(allocated_page), [size](const std::uintptr_t *data) {
                    free(*data, size);
                    delete data;
                }};
    }

    return nullptr;
}