#pragma once
#include <memory>
#include <type_traits>


namespace kei
{
    /// A stateless deleter that forwards to a C-style destroy function.
    /// __notice__: `DestroyFn` will never be called with a `nullptr`.
    ///
    /// ## Template Parameters
    ///
    /// 1. `Pointer` -> The pointer type to destroy.
    /// 2. `DestroyFn` -> A function or callable object invocable with `Pointer`.
    ///
    template <typename Pointer, auto DestroyFn>
        requires std::is_pointer_v<Pointer> and std::is_invocable_v<decltype(DestroyFn), Pointer>
    struct deleter
    {
        void operator()(Pointer pointer) const noexcept(noexcept(DestroyFn(pointer)))
        {
            if (pointer != nullptr) DestroyFn(pointer);
        }
    };


    /// The base type for `*_handle_of` traits.
    ///
    /// ## Template Parameters
    ///
    /// 1. `Wrapper` -> A smart pointer used to manage `Tp`.
    /// 2. `Tp` -> The type to be wrapped.
    /// 3. `DestroyFn` -> The function used to release `Tp`.
    ///
    /// ### Constrains
    ///
    /// 1. `Wrapper` must either accept 2 or 1 template parameters.
    /// 2. `Wrapper`'s 1st template parameter must be the object to manage (i.e. `Tp`).
    /// 3. `Wrapper`'s 2nd template (if it has one) must be the deleter for the object.
    /// 4. If `Wrapper` doesn't accept a deleter type, `Wrapper::reset()`
    ///    must accept a deleter object as its second parameter.
    template <template <typename...> typename Wrapper, typename Tp, auto DestroyFn>
        requires std::is_invocable_v<decltype(DestroyFn), Tp *>
    class wrapper_handle_of
    {
    public:
        using value_type   = Tp;
        using pointer      = value_type *;
        using deleter_type = deleter<pointer, DestroyFn>;


        wrapper_handle_of(pointer ptr = pointer {}) noexcept { reset(ptr); }


        [[nodiscard]]
        auto get() const noexcept -> pointer
        { return m_ptr.get(); }

        [[nodiscard]]
        auto get() noexcept -> pointer
        { return m_ptr.get(); }

    protected:
        void reset(pointer ptr = pointer {}) noexcept
        {
            if constexpr (deleter_in_type)
                m_ptr.reset(ptr);
            else
                m_ptr.reset(ptr, deleter_type {});
        }

    private:
        static constexpr bool deleter_in_type
            = requires { typename Wrapper<value_type, deleter_type>; };

        static consteval auto select_wrapper() noexcept
        {
            if constexpr (deleter_in_type)
                return std::type_identity<Wrapper<value_type, deleter_type>>();
            else
                return std::type_identity<Wrapper<value_type>>();
        }


        decltype(select_wrapper())::type m_ptr;


        static_assert(
            deleter_in_type or requires { typename Wrapper<value_type>; },
            "`Wrapper` must either accept `value_type` as a template parameter, or `value_type` "
            "and `deleter_type` as a template parameter.");
    };


    template <typename Tp, auto DestroyFn>
        requires std::is_invocable_v<decltype(DestroyFn), Tp *>
    class unique_handle_of : public wrapper_handle_of<std::unique_ptr, Tp, DestroyFn>
    {
        using base = wrapper_handle_of<std::unique_ptr, Tp, DestroyFn>;

    public:
        using base::base;
        using typename base::deleter_type;
        using typename base::pointer;
        using typename base::value_type;
    };


    template <typename Tp, auto DestroyFn>
        requires std::is_invocable_v<decltype(DestroyFn), Tp *>
    class shared_handle_of : public wrapper_handle_of<std::shared_ptr, Tp, DestroyFn>
    {
        using base = wrapper_handle_of<std::shared_ptr, Tp, DestroyFn>;

    public:
        using base::base;
        using typename base::deleter_type;
        using typename base::pointer;
        using typename base::value_type;
    };
}
