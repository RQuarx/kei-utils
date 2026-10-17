#pragma once
#include <concepts>
#include <mutex>

#include "result.hh"


namespace kei
{
    /// Describes a policy that initializes and tears down a process-wide runtime
    /// (e.g. a C library that requires `lib_init()` / `lib_quit()` calls).
    ///
    /// ## Requirements
    ///
    /// 1. `T::init()` must be a `noexcept` static function returning `result<>`.
    /// 2. `T::deinit()` must be a `noexcept` static function.
    template <typename T>
    concept runtime_policy = requires {
        { T::init() } noexcept -> std::same_as<result<>>;
        { T::deinit() } noexcept;
    };


    namespace _impl
    {
        /// A thread-safe reference counter that drives `Policy::init()` and `Policy::deinit()`.
        ///
        /// ## Template Parameters
        ///
        /// 1. `Policy` -> The policy used to initialize and deinitialize the runtime.
        template <runtime_policy Policy>
        class runtime_state
        {
        public:
            /// Registers a new user of the runtime. Calls `Policy::init()` if this is
            /// the first user.
            ///
            /// If `Policy::init()` fails, the reference count is left unchanged and
            /// the error is returned.
            ///
            /// ## Returns
            ///
            /// An empty `result<>` on success, otherwise the error from `Policy::init()`.
            [[nodiscard]]
            auto acquire() -> result<>
            {
                std::scoped_lock lock { m_mtx };
                if (m_count++ == 0)
                    if (auto res = Policy::init(); !res)
                    {
                        m_count--;
                        return res;
                    }
                return {};
            }


            /// Unregisters a user of the runtime. Calls `Policy::deinit()` if this was
            /// the last user.
            ///
            /// __notice__: Must be paired with a successful call to `acquire()`.
            void release()
            {
                std::scoped_lock lock { m_mtx };
                if (--m_count == 0) Policy::deinit();
            }

        private:
            std::mutex  m_mtx;
            std::size_t m_count = 0;
        };
    }


    /// A base class that keeps a global runtime alive for as long as at least one
    /// derived object exists.
    ///
    /// The runtime is initialized (`Policy::init()`) when the first instance is
    /// constructed and deinitialized (`Policy::deinit()`) when the last instance is
    /// destroyed. The bookkeeping is thread-safe.
    ///
    /// Copying or moving an instance counts as creating a new one, so every live
    /// object holds exactly one reference to the runtime. Assignment doesn't change
    /// the number of references.
    ///
    /// The constructor and destructor are `protected`, so it can only be used as a
    /// base class.
    ///
    /// ## Template Parameters
    ///
    /// 1. `Policy` -> The policy that initializes and deinitializes the runtime.
    ///
    /// ### Constrains
    ///
    /// 1. `Policy` must satisfy `runtime_policy`.
    ///
    /// ## Errors
    ///
    /// Constructing (including copying and moving) an instance throws the `error`
    /// returned by `Policy::init()` if the runtime fails to initialize.
    ///
    /// ## Example
    ///
    /// ```cpp
    /// #include <kei/runtime_guard.hh>
    ///
    ///
    /// struct lib_policy
    /// {
    ///     static auto init() noexcept -> kei::result<>
    ///     {
    ///         if (lib_init() != 0)
    ///             return kei::error { "lib_init failed" }.unexpected();
    ///         return {};
    ///     }
    ///
    ///     static void deinit() noexcept { lib_quit(); }
    /// };
    ///
    /// class window final : private kei::runtime_guard<lib_policy>
    /// {
    ///     /* `lib_init()` has run before any member is used. */
    /// };
    ///
    /// window a;   // calls `lib_policy::init()`
    /// window b;   // runtime is already up, only the count is incremented
    /// /* ... `lib_policy::deinit()` is called once both `a` and `b` are destroyed. */
    /// ```
    template <runtime_policy Policy>
    class runtime_guard
    {
    protected:
        /// Acquires a reference to the runtime, initializing it if needed.
        ///
        /// Throws the error returned by `Policy::init()` on failure.
        runtime_guard()
        {
            if (auto res = m_state.acquire(); !res) throw res.error();
        }

        /// Releases the reference to the runtime, deinitializing it if this was the last one.
        ~runtime_guard() { m_state.release(); }

        runtime_guard(const runtime_guard & /* unused */) : runtime_guard {} {}
        runtime_guard(runtime_guard && /* unused */) noexcept : runtime_guard {} {}
        auto operator=(const runtime_guard &) -> runtime_guard & = default;
        auto operator=(runtime_guard &&) -> runtime_guard & = default;

    private:
        inline static _impl::runtime_state<Policy> m_state;
    };
}
