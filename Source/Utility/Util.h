//
//  Util.h
//  Heatray5 macOS
//
//  Created by Cody White on 4/15/23.
//

#pragma once

#include <type_traits>

namespace util {

template<typename E>
constexpr typename std::underlying_type<E>::type to_underlying(E e) noexcept {
    return static_cast<typename std::underlying_type<E>::type>(e);
}

} // namespace util.
