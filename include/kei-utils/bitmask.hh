#pragma once
#include <concepts>
#include <type_traits>


namespace kei
{
    template <typename Enum, typename Int>
    concept enum_of = std::is_integral_v<Int> and std::is_enum_v<Enum>
                  and std::is_same_v<std::underlying_type_t<Enum>, Int>;


    /// A base class that stores a set of boolean flags in a single integer.
    ///
    /// Derive from it and define an enum of flag values whose underlying type is
    /// `base_type`. Each enumerator should be a distinct bit (e.g. `1 << n`).
    ///
    /// ## Template Parameters
    ///
    /// 1. `IntTp` -> The integer type used to store the flags.
    ///
    /// ### Constrains
    ///
    /// 1. `IntTp` must satisfy `std::is_integral_v<IntTp>`.
    /// 2. `IntTp` must be the underlying type used for the flag enum.
    ///
    /// ## Example
    ///
    /// ```cpp
    /// #include <cassert>
    /// #include <cstdint>
    ///
    /// #include <kei/bitmask.hh>
    ///
    ///
    /// class attributes final : public kei::bitmask<std::uint8_t>
    /// {
    /// public:
    ///     enum flag : base_type
    ///     {
    ///         available = 1 << 0,
    ///         clean     = 1 << 1,
    ///     };
    /// };
    ///
    /// attributes attr;
    /// assert(not attr.has(attributes::available));
    ///
    /// attr.set(attributes::clean, true);
    /// assert(attr.has(attributes::clean));
    /// ```
    template <std::integral IntTp>
    class bitmask
    {
    public:
        using base_type = IntTp;


        template <enum_of<base_type> E>
        constexpr bitmask(E flag = {}) noexcept { set(flag, true); }


        /// Returns `true` if any bit of `flag` is set. A flag with value `0` always yields `false`.
        ///
        /// ## Parameters
        ///
        /// 1. `flag: E` -> A flag enumerator whose underlying type is `base_type`.
        template <enum_of<base_type> E>
        [[nodiscard]]
        constexpr auto has(E flag) const noexcept -> bool
        { return (m_data & static_cast<base_type>(flag)) != 0; }


        /// Sets or clears all bits of `flag`.
        ///
        /// ## Parameters
        /// 1. `flag: E` -> A flag enumerator whose underlying type is `base_type`.
        /// 2. `state: bool` -> `true` to set the bits, `false` to clear them.
        template <enum_of<base_type> E>
        constexpr void set(E flag, bool state) noexcept
        {
            const auto value = static_cast<base_type>(flag);
            if (state)
                m_data |= value;
            else
                m_data &= ~value;
        }


        /// Returns `true` if no flags are set.
        [[nodiscard]] constexpr auto empty() const noexcept -> bool { return m_data == 0; }


        /// Clears the data of the bitmask, setting it to `0`.
        constexpr void clear() noexcept { m_data = 0; }

    private:
        base_type m_data = 0;
    };
}
