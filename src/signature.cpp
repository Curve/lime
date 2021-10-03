#include "constants/protection.hpp"
#include <module.hpp>
#include <page.hpp>
#include <sstream>
#include <utility/address.hpp>
#include <utility/memory.hpp>
#include <utility/signature.hpp>

namespace lime
{
    bool test_sig(const char *data, const std::string &pattern, const std::string &mask)
    {
        for (std::size_t j = 0; pattern.size() > j; j++)
        {
            if (mask[j] != '?')
            {
                auto byte = data[j];

                if (byte != pattern[j])
                {
                    return false;
                }
            }
        }

        return true;
    }

    sig::signature(const std::string &ida_pattern)
    {
        std::stringstream stream(ida_pattern);

        std::string current;
        while (std::getline(stream, current, ' '))
        {
            if (current.find('?') != std::string::npos)
            {
                m_pattern.push_back(0);
                m_mask.push_back('?');
            }
            else
            {
                m_pattern.push_back(static_cast<char>(std::stol(current, nullptr, 16)));
                m_mask.push_back('x');
            }
        }
    }

    template sig::cond_rtn<true> sig::find<true>();
    template sig::cond_rtn<false> sig::find<false>();
    template <bool find_all> sig::cond_rtn<find_all> sig::find()
    {
        cond_rtn<find_all> rtn;

        for (const auto &page : page::get_pages())
        {
            auto found = find_in<find_all>(page);
            if constexpr (find_all)
            {
                if (!found.empty())
                    rtn.insert(rtn.end(), found.begin(), found.end());
            }
            else
            {
                if (found)
                {
                    rtn = *found;
                    break;
                }
            }
        }

        return rtn;
    }

    template sig::cond_rtn<true> sig::find_in<true>(const page &);
    template sig::cond_rtn<false> sig::find_in<false>(const page &);
    template <bool find_all> sig::cond_rtn<find_all> sig::find_in(const page &page)
    {
        cond_rtn<find_all> rtn;

        if (page.get_protection() != prot::read_only && page.get_protection() != prot::read_write &&
            page.get_protection() != prot::read_execute && page.get_protection() != prot::read_write_execute)
            return rtn;

        for (auto current = page.get_start(); current < (page.get_end() - m_pattern.size()); current++)
        {
            auto data = read(current, m_pattern.size());

            if (data && test_sig(data.get(), m_pattern, m_mask))
            {
                if constexpr (find_all)
                {
                    rtn.emplace_back(current);
                }
                else
                {
                    rtn = {current};
                    break;
                }
            }
        }

        return rtn;
    }

    template sig::cond_rtn<true> sig::find_in<true>(const module &);
    template sig::cond_rtn<false> sig::find_in<false>(const module &);
    template <bool find_all> sig::cond_rtn<find_all> sig::find_in(const module &module)
    {
        cond_rtn<find_all> rtn;

        for (const auto &page : page::get_pages())
        {
            if (page.get_start() >= module.get_start() && page.get_end() <= (module.get_start() + module.get_size()))
            {
                auto result = find_in<find_all>(page);
                if constexpr (find_all)
                {
                    rtn.insert(rtn.end(), result.begin(), result.end());
                }
                else
                {
                    if (result)
                    {
                        rtn = *result;
                        break;
                    }
                }
            }
        }

        return rtn;
    }
} // namespace lime