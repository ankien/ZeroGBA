#pragma once

template <typename container, unsigned originalBits>
constexpr container signExtend(const container num) {
    struct {
        container num : originalBits;
    } signExtended{num};
    return signExtended.num;
}