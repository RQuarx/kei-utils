#pragma once
#include <expected>
#include <format>
#include <source_location>


namespace kei
{
    /// A format string that also captures the location of the call site.
    ///
    /// Intended to be used through the `format_string` alias as a function parameter,
    /// so that the `std::source_location` is taken from the caller rather than from
    /// the function that receives the format string.
    ///
    /// ## Template Parameters
    ///
    /// 1. `Args...` -> The types of the arguments the format string is checked against.
    ///
    /// ### Constrains
    ///
    /// 1. `std::format_string<Args...>` must be constructible from the type of the
    ///    format string passed to the constructor (i.e. it is checked at compile time).
    template <typename... Args>
    class format_string_ext
    {
    public:
        /// ## Parameters
        ///
        /// 1. `fmt: Tp` -> The format string, usually a string literal.
        /// 2. `source: std::source_location` -> The location of the call site. Defaults
        ///    to `std::source_location::current()`.
        template <typename Tp>
        constexpr format_string_ext(const Tp                   &fmt,
                                    const std::source_location &source
                                    = std::source_location::current())
            requires std::constructible_from<std::format_string<Args...>, Tp>
            : m_fmt { fmt }, m_source { source }
        {
        }


        /// Returns the compile-time checked format string.
        [[nodiscard]]
        constexpr auto format_string() const noexcept -> std::format_string<Args...>
        { return m_fmt; }


        /// Returns the location captured when the format string was constructed.
        [[nodiscard]]
        constexpr auto source_location() const noexcept -> std::source_location
        { return m_source; }

    private:
        std::format_string<Args...> m_fmt;
        std::source_location        m_source;
    };


    /// Alias of `format_string_ext` that is a non-deduced context for `Args...`.
    ///
    /// Use it in place of `std::format_string<Args...>` when the location of the
    /// call site is needed, `Args...` will then be deduced from the other parameters.
    ///
    /// ## Example
    ///
    /// ```cpp
    /// template <typename... Args>
    /// void log(kei::format_string<Args...> fmt, Args &&...args)
    /// {
    ///     std::println("{}: {}", fmt.source_location(),
    ///                  std::format(fmt.format_string(), std::forward<Args>(args)...));
    /// }
    /// ```
    template <typename... Args>
    using format_string = std::type_identity_t<format_string_ext<Args...>>;


    /// The base type of all errors. It holds a formatted message and the location where
    /// it was created.
    ///
    /// ## Example
    ///
    /// ```cpp
    /// #include <kei/result.hh>
    ///
    ///
    /// auto parse(int value) -> kei::result<int>
    /// {
    ///     if (value < 0)
    ///         return kei::error { "negative value: {}", value }.unexpected();
    ///     return value;
    /// }
    ///
    /// auto res = parse(-1);
    /// assert(!res);
    /// std::println("{}", res.error()); // [file.cc:10:20]: negative value: -1
    /// ```
    class error : public std::exception
    {
    public:
        /// Creates an error with an explicit source location.
        ///
        /// ## Parameters
        ///
        /// 1. `source: const std::source_location &` -> The location to report.
        /// 2. `fmt: std::format_string<Args...>` -> The format string of the message.
        /// 3. `args...: Args &&` -> The arguments used to format the message.
        template <typename... Args>
        error(const std::source_location &source, std::format_string<Args...> fmt, Args &&...args)
            : m_message { std::format(fmt, std::forward<Args>(args)...) }, m_source { source }
        {
        }


        /// Creates an error located at the call site.
        ///
        /// ## Parameters
        ///
        /// 1. `fmt: format_string<Args...>` -> The format string of the message. The
        ///    source location is captured implicitly.
        /// 2. `args...: Args &&` -> The arguments used to format the message.
        template <typename... Args>
        error(format_string<Args...> fmt, Args &&...args)
            : m_message { std::format(fmt.format_string(), std::forward<Args>(args)...) },
              m_source { fmt.source_location() }
        {
        }


        /// Returns the formatted error message.
        [[nodiscard]]
        constexpr auto what() const noexcept -> const char * override
        { return m_message.c_str(); }


        /// Returns the location where the error was created.
        [[nodiscard]]
        constexpr auto where() const noexcept -> std::source_location
        { return m_source; }


        /// Wraps the error into a `std::unexpected`, ready to be returned from a
        /// function that returns a `result`.
        ///
        /// The error is copied or moved depending on the value category of `*this`.
        template <typename Er>
        [[nodiscard]]
        auto unexpected(this Er &&self) noexcept -> std::unexpected<Er>
        { return std::unexpected { std::forward<Er>(self) }; }

    private:
        std::string          m_message;
        std::source_location m_source;
    };


    /// The return type of operations that can fail.
    ///
    /// ## Template Parameters
    ///
    /// 1. `Tp` -> The type of the value on success. Defaults to `void`.
    /// 2. `Er` -> The type of the error on failure. Defaults to `error`.
    ///
    /// ### Constrains
    ///
    /// 1. `Er` must derive from `error`.
    template <typename Tp = void, std::derived_from<error> Er = error>
    using result = std::expected<Tp, Er>;


    /// Throws `error` instead of returning it.
    ///
    /// It never returns normally. The `result` return type only exists so the call
    /// can be used in places that expect a `result`, e.g. as the failure branch of
    /// a conditional expression.
    ///
    /// ## Template Parameters
    ///
    /// 1. `Tp` -> The value type of the `result` this call stands in for.
    /// 2. `Er` -> The type of the error to throw.
    ///
    /// ## Parameters
    ///
    /// 1. `error: Er &&` -> The error to throw.
    ///
    /// ## Example
    ///
    /// ```cpp
    /// auto res = *function_that_returns_unexpected().or_lese(throw_error<error>);
    /// ```
    template <typename Tp = void, std::derived_from<error> Er = error>
    auto throw_error(Er &&error) -> result<Tp, Er>
    { throw std::forward<Er>(error); }
}


template <>
struct std::formatter<std::source_location>
{
    constexpr auto parse(auto &ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const std::source_location &src, FormatContext &ctx) const
        -> FormatContext::iterator
    { return std::format_to(ctx.out(), "{}:{}:{}", src.file_name(), src.line(), src.column()); }
};


template <>
struct std::formatter<kei::error>
{
    constexpr auto parse(auto &ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const kei::error &err, FormatContext &ctx) const -> FormatContext::iterator
    { return std::format_to(ctx.out(), "[{}]: {}", err.where(), err.what()); }
};
