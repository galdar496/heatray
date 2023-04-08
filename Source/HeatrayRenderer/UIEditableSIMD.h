//
//  UIEditableSIMD.h
//  Heatray5 macOS
//
//  Created by Cody White on 4/7/23.
//

#pragma once

#include <cstdint>

template <class T>
class UIEditableSIMD {
public:
    UIEditableSIMD() { m_internal.rawData = T(0); }
    ~UIEditableSIMD() = default;
    
    const T simdValue() const { return m_internal.rawData; }
    T simdValue() { return m_internal.rawData; }
    
    template<class V>
    const V* uiValue() const { return static_cast<const V*>(&(m_internal.bytes[0])); }
    template<class V>
    V* uiValue() { return reinterpret_cast<V*>(&(m_internal.bytes[0])); }
private:
    
    union {
        T rawData;
        uint8_t bytes[sizeof(T)];
    } m_internal;
};
