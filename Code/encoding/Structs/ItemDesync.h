#pragma once

struct ItemDesync final
{
    enum Flags : uint8_t
    {
        kNone = 0,
        kAdd = 1 << 0,
        kRemove = 1 << 1,
        kActivate = 1 << 2,
        kDeactivate = 1 << 3
    };
};

inline ItemDesync::Flags operator|(ItemDesync::Flags lhs, ItemDesync::Flags rhs)
{
    return static_cast<ItemDesync::Flags>(static_cast<std::underlying_type_t<ItemDesync::Flags>>(lhs) |
                                          static_cast<std::underlying_type_t<ItemDesync::Flags>>(rhs));
}

inline ItemDesync::Flags operator&(ItemDesync::Flags lhs, ItemDesync::Flags rhs)
{
    return static_cast<ItemDesync::Flags>(static_cast<std::underlying_type_t<ItemDesync::Flags>>(lhs) &
                                          static_cast<std::underlying_type_t<ItemDesync::Flags>>(rhs));
}

inline ItemDesync::Flags& operator|=(ItemDesync::Flags& lhs, ItemDesync::Flags rhs)
{
    lhs = lhs | rhs;
    return lhs;
}

inline ItemDesync::Flags& operator&=(ItemDesync::Flags& lhs, ItemDesync::Flags rhs)
{
    lhs = lhs & rhs;
    return lhs;
}
