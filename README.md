# meta

## 简介

这是一个尝试利用 C++26 静态反射实现的反射库。

## 特点

没有特点。

## 食用指南

### 准备工作

0. Star 本仓库
1. 准备好一个[clang-p2996 编译器](https://github.com/bloomberg/clang-p2996/tree/p2996)
2. 把 meta.hpp 和 span.hpp 拷到你的项目里

### 已实现功能

获取类型信息：

```cpp
using namespace valloside;

constexpr auto r_str = meta::reflect<std::string>(); // constexpr 可以去掉
r_str.for_each_field([](const meta::object &field) {
    std::println("type: {}, name: {}", field.get_type().get_name(), field.get_name());
    return true; // false 表示中断遍历
});
```

输出:

```
type: __rep, name: __rep_
type: __compressed_pair_padding<__rep, true>, name: __padding1_903_
type: allocator<char>, name: __alloc_
type: __compressed_pair_padding<allocator<char>, true>, name: __padding2_903_
```

range-for 也是支持的:

```cpp
for (auto &&field : r_str.get_fields_info()) {
    // ...
}
```

动态反射：

```cpp
struct foo {
    char a;
    int  b : 8;
    int  c : 2;
    float z;
};

using namespace valloside;

constexpr auto r_foo = meta::reflect<foo>();
foo  obj{};
auto dyn_obj = obj | r_str; // 等价于 meta::reflect(obj)
```

动态修改字段：

```cpp
auto z = dyn_obj.field("z");
z = 3.14f; // 目前要求右操作数必须严格对应原始类型，否则行为未定义
std::println("z: {}", obj.z); // z: 3.14
z = 6.28f;
std::println("z: {}", obj.z); // z: 6.28
```

输出：

```
z: 3.14
z: 6.28
```

位域也是支持的：

```cpp
auto bit_c = dyn_obj.field("c");
bit_c = 0;
std::println("c: {}", (int)obj.c); // c: 0
bit_c = 2;
std::println("c: {}", (int)obj.c); // c: -2
```

输出：

```
c: 0
c: -2
```

转回普通对象：
```cpp
foo &f = dyn_obj.as<foo>();
```

成员函数的操作仍在实现中。
