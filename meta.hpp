#pragma once

#include <ranges>
#include <meta>
#include "span.hpp"

namespace valloside::meta {

    class type;
    class any;

    namespace details {

        struct object_info;
        struct type_info;
        template <std::meta::info R>
        consteval auto make_type_info() -> const type_info &;

        // 类型的信息
        struct type_info {
            structural_string_view             m_name;
            structural_span<const object_info> m_fields;
        };

        struct member_offset {
            ptrdiff_t m_bytes{-1};
            ptrdiff_t m_bits{-1};

            constexpr member_offset() = default;
            constexpr member_offset(const std::meta::member_offset &off) : m_bytes(off.bytes), m_bits(off.bits) {}
            constexpr member_offset &operator==(const std::meta::member_offset &off) {
                return m_bytes = off.bytes, m_bits = off.bits, *this;
            }
            constexpr auto operator<=>(const member_offset &) const = default;

            constexpr bool is_valid() const { return m_bytes != -1 && m_bits != -1; }
            constexpr      operator bool() const { return is_valid(); }
        };

        // 变量、数据成员的信息
        struct object_info {
            structural_string_view m_name;
            const type_info       *m_type;
            member_offset          m_offset;
            size_t                 m_size;

            using accessor_t = any (*)(void *obj);
            accessor_t m_accessor = nullptr;
        };

        struct method_info {
            structural_string_view           m_name;
            const object_info               *m_result;
            structural_span<const type_info> m_params;
        };

        struct namespace_info {
            structural_string_view             m_name;
            structural_span<const object_info> m_vars;
            structural_span<const method_info> m_funcs;
        };

        template <std::meta::info R>
        consteval auto _extract_name() -> std::string_view {
            /*
                什么时候没有 identifier 呢？我只发现模板形参、匿名类、匿名联合体没有。
            */
            if constexpr (std::meta::has_identifier(R)) {
                return std::meta::identifier_of(R);
            } else if constexpr (std::meta::is_nonstatic_data_member(R)) { // 匿名成员
                if constexpr (std::meta::is_union_type(type_of(R)))
                    return "(anonymous union)";
                else if constexpr (std::meta::is_class_type(type_of(R)))
                    return "(anonymous class)";
                else
                    return "(anonymous member)";
            } else { // 从模板形参取得的反射会丢失 identifier，例如把 std::string
                     // 传入模板参数， 就拿不到 "string" 这个别名了，但是
                     // display_string_of 可以重建出其原始名称： "basic_string<char,
                     // std::char_traits<char>, std::allocator<char>>"
                return std::meta::display_string_of(R);
            }
        }

        // 从反射句柄获取名称信息
        template <std::meta::info R>
        consteval auto extract_name() -> std::string_view {
            constexpr auto name = _extract_name<R>();
            return {std::define_static_string(name), name.size()};
        }

        template <std::meta::info R>
        consteval auto extract_field_info();

        template <std::meta::info R>
        consteval auto extract_fields() -> std::span<const object_info> {
            constexpr auto R_ = decay(R);
            if constexpr (std::meta::is_class_type(R_) || std::meta::is_union_type(R_)) {
                constexpr auto      fields = std::define_static_array(extract_field_info<R_>());
                constexpr std::span fields_span{fields.begin(), fields.end()};
                return fields_span;
            } else {
                return {};
            }
        }

        template <std::meta::info R>
        consteval auto make_type_info() -> const type_info & {
            constexpr auto type_name = extract_name<R>();
            constexpr auto fields = extract_fields<R>();
            return std::define_static_array(std::views::single(type_info{type_name, fields}))[0];
        }

    } // namespace details

    class object {
        const details::object_info *m_object_info;

      public:
        constexpr object(const details::object_info &info) : m_object_info(&info) {}

        constexpr auto get_name() const -> std::string_view { return m_object_info->m_name; }

        constexpr auto get_type() const -> type;

        constexpr auto get_offset() const { return m_object_info->m_offset; }

        constexpr auto get_size() const { return m_object_info->m_size; }

        constexpr auto get_accessor() const { return m_object_info->m_accessor; }
    };

    class type {
        const details::type_info *m_type_info;

      public:
        constexpr type(const details::type_info &type_info) : m_type_info(&type_info) {}
        constexpr type(nullptr_t) : m_type_info(nullptr) {}

        constexpr auto get_name() const -> std::string_view { return m_type_info->m_name; }

        template <typename Callback>
            requires std::is_invocable_r_v<bool, Callback, const object &>
        constexpr auto for_each_field(Callback &&callback) const {
            for (auto &&field : m_type_info->m_fields.to_span()) {
                if (!callback(object{field}))
                    return false;
            }
            return true;
        }

        constexpr auto get_fields_info() const {
            return m_type_info->m_fields.to_span() |
                   std::views::transform([](auto &&field_impl) { return object{field_impl}; });
        }

        constexpr auto get_field(std::string_view name) const -> std::optional<object> {
            for (auto &&field : m_type_info->m_fields.to_span()) {
                if (field.m_name == name) {
                    return object{field};
                }
            }
            return std::nullopt;
        }

        constexpr bool operator==(const type &other) const {
            /*
                我不太确定合不合理，不清楚标准里有没有相关说明，结果可能未定义，或未指明
                但我暂时找不到其它简单的做法了，反正 clang 这边测试是没问题的
             */
            return m_type_info == other.m_type_info;
        }

        template <std::meta::info R>
            requires(std::meta::is_type(R))
        friend consteval auto reflect();
    };

    constexpr auto object::get_type() const -> type { return type{*m_object_info->m_type}; }

    constexpr struct null_obj_t {
    } null_obj;

    class any;

    /*
        dynamic/runtime object
    */
    class any {
      public:
        enum class value_status : uint8_t {
            empty = 0,
            variable = 1,
            bitfield = 2,
        };

      private:
        using bit_field_getter_t = size_t (*)(void *);
        using bit_field_setter_t = void (*)(void *, size_t);

        struct bit_field_proxy {
            bit_field_getter_t m_getter = nullptr;
            bit_field_setter_t m_setter = nullptr;
        };

        struct value {
            void           *m_ptr;
            bit_field_proxy m_bit_field_proxy;
            value_status    m_status;

            constexpr value(nullptr_t) : m_ptr(nullptr), m_status(value_status::empty) {}
            constexpr value(void *obj) : m_ptr(obj), m_status(value_status::variable) {}
            constexpr value(void *obj, bit_field_proxy proxy) :
                m_ptr(obj), m_bit_field_proxy(proxy), m_status(value_status::bitfield) {}
        };

        type  m_type_info;
        value m_value;

      public:
        constexpr any(null_obj_t = null_obj) : m_type_info(nullptr), m_value(nullptr) {}

        constexpr any(void *obj, const details::type_info &info) : m_type_info(info), m_value(obj) {}
        constexpr any(void *obj, const type &info) : m_type_info(info), m_value(obj) {}
        constexpr any(void *obj, bit_field_getter_t getter, bit_field_setter_t setter, const type &info) :
            m_type_info(info), m_value(obj, bit_field_proxy{getter, setter}) {}

        template <typename T>
        constexpr any(T &obj, const details::type_info &info) :
            m_type_info(info), m_value(const_cast<void *>(static_cast<const void *>(&obj))) {}
        template <typename T>
        constexpr any(T &obj, const type &info) :
            m_type_info(info), m_value(const_cast<void *>(static_cast<const void *>(&obj))) {}
        template <typename T>
        constexpr any(T &obj, bit_field_getter_t getter, bit_field_setter_t setter, const type &info) :
            m_type_info(info),
            m_value(const_cast<void *>(static_cast<const void *>(&obj)), bit_field_proxy{getter, setter}) {}

        auto get_raw_ptr() const -> void * { return m_value.m_ptr; }

        template <typename T>
        auto as() const -> T & {
            switch (m_value.m_status) {
            case value_status::variable:
                return *static_cast<T *>(m_value.m_ptr);
            case value_status::bitfield:
                if constexpr (std::is_fundamental_v<std::remove_const_t<T>>)
                    return m_value.m_bit_field_proxy.m_getter(m_value.m_ptr);
                else
                    throw std::runtime_error{"Reading unsupported type from a bit field"};
            case value_status::empty:
            default:
                throw std::runtime_error{"Reading from null_obj"};
            }
        }

        template <typename T>
        any &operator=(T &&value) {
            switch (m_value.m_status) {
            case value_status::variable:
                *static_cast<T *>(m_value.m_ptr) = value;
                break;
            case value_status::bitfield:
                if constexpr (std::is_fundamental_v<std::decay_t<T>>)
                    m_value.m_bit_field_proxy.m_setter(m_value.m_ptr, value);
                else
                    throw std::runtime_error{"Reading unsupported type from a bit field"};
                break;
            case value_status::empty:
            default:
                throw std::runtime_error{"Reading null_obj"};
            }
            return *this;
        }

        auto field(std::string_view name) const -> any {
            auto field = m_type_info.get_field(name);
            if (field) {
                return field->get_accessor()(m_value.m_ptr);
            } else {
                return null_obj;
            }
        }

        auto get_type_info() const -> type { return m_type_info; }

        // auto method(std::string_view name) const -> any {}

        constexpr bool empty() const { return m_value.m_status != value_status::empty; }
        constexpr      operator bool() const { return empty(); }
        constexpr bool is_bit_field() const { return m_value.m_status == value_status::bitfield; }
    };

    namespace details {

        template <std::meta::info Rclass, size_t N, const type_info *t>
        auto accessor_impl(void *obj) -> any {
            using Class = typename[:Rclass:];
            constexpr auto r_memb =
                define_static_array(nonstatic_data_members_of(Rclass, std::meta::access_context::unchecked()))[N];
            if constexpr (!std::meta::is_bit_field(r_memb))
                return any{static_cast<Class *>(obj)->[:r_memb:], *t};
            else
                return any{
                    obj,
                    [](void *obj) -> size_t {
                        constexpr auto r_memb = define_static_array(
                            nonstatic_data_members_of(Rclass, std::meta::access_context::unchecked())
                        )[N];
                        return static_cast<Class *>(obj)->[:r_memb:];
                    },
                    [](void *obj, size_t value) {
                        constexpr auto r_memb = define_static_array(
                            nonstatic_data_members_of(Rclass, std::meta::access_context::unchecked())
                        )[N];
                        static_cast<Class *>(obj)->[:r_memb:] = value;
                    },
                    *t
                };
        }

        template <std::size_t... Is>
        consteval auto create_constexpr_array(std::integer_sequence<std::size_t, Is...>) {
            return std::array<size_t, sizeof...(Is)>{Is...};
        }

        template <std::meta::info R>
        consteval auto extract_field_info() {
            constexpr auto r_members =
                define_static_array(nonstatic_data_members_of(R, std::meta::access_context::unchecked()));
            std::array<object_info, r_members.size()> fields;
            template for (constexpr auto idx : create_constexpr_array(std::make_index_sequence<r_members.size()>())) {
                constexpr auto  r_memb = r_members[idx];
                constexpr auto &typeinfo = make_type_info<type_of(r_memb)>();
                fields[idx].m_name = extract_name<r_memb>();
                fields[idx].m_type = &typeinfo;
                fields[idx].m_offset = std::meta::offset_of(r_memb);
                fields[idx].m_size = std::meta::size_of(r_memb);
                fields[idx].m_accessor = &accessor_impl<R, idx, &typeinfo>;
            }
            return fields;
        }

    } // namespace details

    template <typename T>
    constexpr any operator|(T &obj, const type &info) {
        return any{obj, info};
    }

    template <std::meta::info R>
        requires(std::meta::is_type(R))
    consteval auto reflect() -> type {
        constexpr auto &t = details::make_type_info<R>();
        return type{t};
    }

    template <typename T>
    consteval auto reflect() -> type {
        return reflect<^^T>();
    }

    template <typename T>
    auto reflect(T &v) -> any {
        return v | reflect<T>();
    }

} // namespace valloside::meta
