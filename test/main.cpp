#include <meta>
#include <print>
#include "meta.hpp"
#include "test.h"

using namespace valloside;

constexpr void test0() {
    constexpr auto string = meta::reflect<std::string>();
    constexpr auto vec_int = meta::reflect<std::vector<int>>();
    constexpr auto string2 = meta::reflect<std::string>();

    auto string3 = get();

    static_assert(string != vec_int);
    static_assert(string == string2);
    std::println("string == string3: {}", string == string3);
}

void test1() {
    constexpr auto string = meta::reflect<std::string>();
    std::println("name of std::string: {}", string.get_name());
    string.for_each_field([](const meta::object &field) {
        std::println("type: {}, name: {}", field.get_type().get_name(), field.get_name());
        return true;
    });
    constexpr auto alloc = string.get_field("__alloc_");
    if constexpr (alloc)
        std::println("__alloc found!: {}", alloc.value().get_name());
    constexpr auto fields = string.get_fields_info();
    for (auto &&field : fields) {
        // ...
    }
    static_assert(fields[0].get_type().get_name() == "__rep");
}

namespace nspc {
    template <typename T>
    struct Vector2D_ {
      public:
        T x;
        T y;

        Vector2D_() = default;
    };

    using Vector2D = Vector2D_<float>;
} // namespace nspc

void test2() {
    auto           r_vec = meta::reflect<nspc::Vector2D>();
    nspc::Vector2D obj{};
    obj.x = 16;
    obj.y = 16;

    auto dyn_obj = obj | r_vec;
    auto res = dyn_obj.as<nspc::Vector2D>();
    std::println("{}", res.x);
    auto y = dyn_obj.field("y");
    std::println("y: {}", obj.y);
    y = 2.17f;
    std::println("y: {}", obj.y);
}

struct foo {
    char  a;
    int   b : 8;
    int   c : 2;
    float z;
};

void test3() {
    auto r_foo = meta::reflect<foo>();
    foo  obj{};
    auto dyn_obj = meta::reflect(obj);
    auto bit_c = dyn_obj.field("c");
    std::println("c: {}", (int)obj.c);
    bit_c = 2;
    std::println("c: {}", (int)obj.c);
    auto z = dyn_obj.field("z");
    z = 3.14f;
    std::println("z: {}", obj.z);
    z = 6.28f;
    std::println("z: {}", obj.z);
    foo &f = dyn_obj.as<foo>();
    std::println("z: {}", f.z);
}

int main() {
    test0();
    test1();
    test2();
    test3();
}