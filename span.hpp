#pragma once

#include <cstddef>
#include <string_view>
#include <span>

namespace valloside::meta::details {

    /*
        std::string_view 的数组不能作为 std::define_static_array 的参数，原因是 std::string_view
        不满足一个条件 std::meta::detail::__metafn_is_structural_type，structural type 的具体含义见
        https://en.cppreference.com/w/cpp/language/template_parameters.html
        可以简单理解为能被放进模板参数的类型。所以设计这个类，用于在编译期构造静态数组。
        其实直接存一个裸指针加一个长度也是可以的，封装成类比较方便而已。
    */
    struct structural_string_view {
        const char *m_str;
        size_t      m_len;

        consteval structural_string_view() noexcept : m_str(nullptr), m_len(0) {}
        consteval structural_string_view(const char *str, size_t len) noexcept : m_str(str), m_len(len) {}
        consteval structural_string_view(std::string_view str) noexcept : m_str(str.data()), m_len(str.size()) {}

        constexpr      operator std::string_view() const noexcept { return std::string_view(m_str, m_len); }
        constexpr auto to_string_view() const noexcept { return std::string_view(m_str, m_len); }
    };

    /*
        同 structural_string_view
    */
    template <typename T>
    struct structural_span {
        T     *m_data;
        size_t m_size;

        consteval structural_span() noexcept : m_data(nullptr), m_size(0) {}
        consteval structural_span(T *data, size_t size) noexcept : m_data(data), m_size(size) {}
        consteval structural_span(std::span<T> span) noexcept : m_data(span.data()), m_size(span.size()) {}

        constexpr      operator std::span<T>() const noexcept { return std::span(m_data, m_size); }
        constexpr auto to_span() const noexcept { return std::span(m_data, m_size); }
    };

} // namespace valloside::meta::details