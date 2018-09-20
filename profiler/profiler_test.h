#pragma once

#include <type_traits>
#include <functional>
#include <cstdint>

namespace profiler {

template<class, class = void>
struct has_instrument_function : std::false_type {};

template<class T>
struct has_instrument_function<T, std::void_t<decltype(__profiler_set_version(std::declval<T>()))>> : std::true_type {};

struct TestInstrumentFunction : has_instrument_function<uint16_t> {};

} // namespace profiler
