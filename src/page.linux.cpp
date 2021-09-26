#include <cinttypes>
#include <constants/protection.hpp>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <page.hpp>
#include <sys/unistd.h>

namespace lime
{
    using std::filesystem::path;

    struct page::impl
    {
        static std::optional<page> parse_page(const std::string &);
    };

    page::~page() = default;
    page::page() : m_prot(0){};
    page::page(page &&) noexcept = default;

    std::optional<page> page::impl::parse_page(const std::string &line)
    {
        page page;

        char module[256] = "";
        char permissions[4] = "";
        std::uintptr_t start = 0, end = 0;

        // NOLINTNEXTLINE
        sscanf(line.c_str(), "%" PRIxPTR "-%" PRIxPTR " %4c %*x %*x:%*x %*u %s", &start, &end, permissions, module);

        if (std::strstr(module, "["))
            return std::nullopt;

        if (permissions[0] != '-')
            page.m_prot |= prot_read;
        if (permissions[1] != '-')
            page.m_prot |= prot_write;
        if (permissions[2] != '-')
            page.m_prot |= prot_execute;

        page.m_end = static_cast<std::uintptr_t>(end);
        page.m_start = static_cast<std::uintptr_t>(start);

        return page;
    }

    std::vector<page> page::get_pages()
    {
        std::vector<page> pages;
        std::ifstream maps(path("/proc") / std::to_string(getpid()) / "maps");

        if (maps)
        {
            std::string line;
            while (std::getline(maps, line))
            {
                auto page = impl::parse_page(line);

                if (page)
                    pages.emplace_back(std::move(*page));
            }
        }

        return pages;
    }

    std::optional<page> page::get_page_at(const std::uintptr_t &address)
    {
        std::ifstream maps(path("/proc") / std::to_string(getpid()) / "maps");

        if (maps)
        {
            std::string line;
            while (std::getline(maps, line))
            {
                std::uintptr_t start = 0, end = 0;
                sscanf(line.c_str(), "%" PRIxPTR "-%" PRIxPTR "", &start, &end); // NOLINT

                if (start <= address && end >= address)
                {
                    return impl::parse_page(line);
                }
            }
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