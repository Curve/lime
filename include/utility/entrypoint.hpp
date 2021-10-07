#pragma once

template <typename... params_t> void entry(const params_t &...);
template <typename... params_t> void exit(const params_t &...);

#include "entrypoint.inl"