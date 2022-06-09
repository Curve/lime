#include "page.hpp"

#include <Windows.h>

namespace lime
{
    page::page() : m_prot(0) {}
    page::page(const page &) = default;

    std::vector<page> page::get_pages()
    {
        std::vector<page> rtn;

        void *address = nullptr;
        MEMORY_BASIC_INFORMATION info;
        while (VirtualQuery(address, &info, sizeof(info)))
        {
            if (info.State == MEM_COMMIT && !(info.Protect & PAGE_GUARD))
            {
                page page;
                page.m_prot = info.Protect;
                page.m_start = reinterpret_cast<std::uintptr_t>(info.BaseAddress);
                page.m_end = page.m_start + info.RegionSize;

                rtn.emplace_back(page);
            }

            address = reinterpret_cast<void *>(reinterpret_cast<std::uintptr_t>(info.BaseAddress) + info.RegionSize);
        }

        return rtn;
    }

    std::optional<page> page::get_page_at(const std::uintptr_t &address)
    {
        MEMORY_BASIC_INFORMATION info;
        if (VirtualQuery(reinterpret_cast<void *>(address), &info, sizeof(info)))
        {
            page rtn;
            rtn.m_prot = info.Protect;
            rtn.m_start = reinterpret_cast<std::uintptr_t>(info.BaseAddress);
            rtn.m_end = rtn.m_start + info.RegionSize;

            return rtn;
        }

        return std::nullopt;
    }

    std::size_t page::get_end() const
    {
        return m_end;
    }
    std::uintptr_t page::get_start() const
    {
        return m_start;
    }
    std::uint8_t page::get_protection() const
    {
        return m_prot;
    }
} // namespace lime