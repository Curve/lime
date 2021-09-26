#pragma once

namespace lime
{
    template <std::size_t size>
    signature::signature(const char (&pattern)[size], const char *mask) : m_mask(mask), m_pattern(pattern, size)
    {
    }
} // namespace lime