// 第 2 章 2.5 模板参数推导（函数模板推导 + C++17 类模板参数推导 CTAD）
// 编译：clang++ -std=c++23 -Wall -Wextra ctad.cpp -o ctad.exe
#include <algorithm>
#include <array>
#include <concepts>
#include <format>
#include <functional>
#include <memory>
#include <print>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <typeinfo>
#include <utility>
#include <vector>

using namespace std::string_literals; // "abc"s 后缀

// ------------------------------------------------------------
// 工具：在编译时拿到类型的名字（书上用 typeid(T).name()，在 g++/clang 上输出的是 i、PKc 这种缩写）
// 原理：__PRETTY_FUNCTION__ 是 g++/clang 提供的“当前函数的完整签名”，里面带着 T = 什么
//       g++ 的格式："... [with T = int; std::string_view = ...]"   clang 的格式："... [T = int]"
// 看不懂实现没关系，会用就行
// ------------------------------------------------------------
template <typename T>
constexpr std::string_view type_name()
{
    std::string_view s = __PRETTY_FUNCTION__;
    const auto start = s.find("T = ") + 4;
    auto end = s.find(';', start);
    if (end == std::string_view::npos)
    {
        end = s.rfind(']');
    }
    return s.substr(start, end - start);
}

// ------------------------------------------------------------
// 1. 函数模板：从实参推导 T
// ------------------------------------------------------------
template <typename T>
std::string_view by_value(T) // 按值传：数组退化成指针，顶层 const 去掉
{
    return type_name<T>();
}

template <typename T>
std::string_view by_cref(const T&) // 按 const 引用传：数组保持数组
{
    return type_name<T>();
}

template <typename T>
std::string_view by_ref(T&) // 按非 const 引用传：不能接字面量
{
    return type_name<T>();
}

// ------------------------------------------------------------
// 5. 书上的 Thing（书上一会儿叫 Thing 一会儿叫 Things，print 也该是 const）
// ------------------------------------------------------------
template <typename T1, typename T2, typename T3>
class Thing
{
public:
    Thing(T1 p1, T2 p2, T3 p3) : v1{p1}, v2{p2}, v3{p3}
    {
    }

    std::string print() const
    {
        return std::format("{}, {}, {}", type_name<T1>(), type_name<T2>(), type_name<T3>());
    }

private:
    T1 v1;
    T2 v2;
    T3 v3;
};

// ------------------------------------------------------------
// 6. C++20：聚合类（没有构造函数的 struct）也能推导，不用写推导指南
// ------------------------------------------------------------
template <typename T>
struct Point
{
    T x;
    T y;
};

// C++20：别名模板也能推导
template <typename T>
using Vec = std::vector<T>;

// ------------------------------------------------------------
// 7. 推导指南（deduction guide）：告诉编译器“看到这样的构造参数，就推导成这个类型”
// ------------------------------------------------------------
template <typename T>
class Sum
{
public:
    template <typename... Ts>
    Sum(Ts&&... values) : v{(values + ...)} // 折叠表达式：把所有参数加起来
    {
    }

    const T& value() const
    {
        return v;
    }

private:
    T v{};
};

// 参数类型各不相同，T 推导不出来；指南说：T 取所有参数的“公共类型”
template <typename... Ts>
Sum(Ts&&... ts) -> Sum<std::common_type_t<Ts...>>;

// 另一个常见用途：把 const char* 推导成 std::string
template <typename T>
struct Label
{
    T text;
};

Label(const char*) -> Label<std::string>;

int main()
{
    std::println("[1] 函数模板推导");
    std::println("by_value(47)    : T = {}", by_value(47));
    std::println("by_value(47L)   : T = {}", by_value(47L));
    std::println("by_value(47.0)  : T = {}", by_value(47.0));
    std::println("by_value(\"47\")  : T = {}", by_value("47"));  // 数组退化成指针
    std::println("by_cref(\"47\")   : T = {}", by_cref("47"));   // 还是数组，长度 3（包括结尾的 '\0'）
    std::println("by_value(\"47\"s) : T = {}", by_value("47"s)); // std::string
    const int ci = 47;
    std::println("by_value(ci)    : T = {}", by_value(ci)); // 顶层 const 被去掉了
    std::println("by_ref(ci)      : T = {}", by_ref(ci));   // 引用会保留 const
    std::println("typeid 的输出   : {} {}", typeid(int).name(), typeid(const char*).name()); // 书上的方法

    // ------------------------------------------------------------
    // 2. 书上说 f(const T&) 不能接字面量 f(47)，这是错的：const 引用可以绑定临时值
    //    不能接字面量的是 T&（非 const 引用），和 1.7 节思考题 (2) 是同一个道理
    // ------------------------------------------------------------
    std::println("\n[2] by_cref(47)  : T = {}", by_cref(47)); // 可以
    // by_ref(47); // 编译失败：非 const 引用不能绑定字面量

    // ------------------------------------------------------------
    // 3. 推导冲突时要手动指定
    // ------------------------------------------------------------
    // std::max(1, 2.0); // 编译失败：T 一会儿是 int 一会儿是 double，编译器不会帮你选
    std::println("\n[3] std::max<double>(1, 2.0) = {}", std::max<double>(1, 2.0));

    // ------------------------------------------------------------
    // 4. C++17 类模板参数推导（CTAD）：构造对象时不写模板参数
    // ------------------------------------------------------------
    std::println("\n[4] CTAD");
    std::pair p{47, 47.0};
    std::tuple t{9, 17, 2.5};
    std::array a{1, 2, 3}; // 元素类型和长度都推导出来：array<int, 3>
    std::println("pair{{47, 47.0}}     -> {}", type_name<decltype(p)>());
    std::println("tuple{{9, 17, 2.5}}  -> {}", type_name<decltype(t)>());
    std::println("array{{1, 2, 3}}     -> {}", type_name<decltype(a)>());
    // std::array<int> b{1, 2, 3}; // 编译失败：CTAD 要么全推导，要么全写，不能只写一半

    // 坑 1：字符串字面量推导成 const char*，不是 std::string
    std::pair name_age{"Tom", 20};
    std::println("pair{{\"Tom\", 20}}    -> {}", type_name<decltype(name_age)>());
    std::pair name_age2{"Tom"s, 20}; // 想要 string 就加 s 后缀
    std::println("pair{{\"Tom\"s, 20}}   -> {}", type_name<decltype(name_age2)>());

    // 坑 2：vector 的大括号和小括号
    std::vector v1(5, 1); // 5 个 1
    std::vector v2{5, 1}; // 两个元素：5 和 1
    std::vector v3{v1};   // 不是“装着一个 vector 的 vector”，而是 v1 的副本！
    std::println("vector(5, 1) = {}, vector{{5, 1}} = {}", v1, v2);
    std::println("vector{{v1}}  -> {}", type_name<decltype(v3)>());

    // ------------------------------------------------------------
    // 5. 自己写的类模板也能推导
    // ------------------------------------------------------------
    Thing thing{1, 47.0, "three"};
    std::println("\n[5] Thing{{1, 47.0, \"three\"}} -> {}", thing.print());

    // ------------------------------------------------------------
    // 6. 聚合类和别名模板的推导（C++20，书上没讲）
    // ------------------------------------------------------------
    Point pt{1.5, 2.5};
    Vec vec{1, 2, 3};
    std::println("\n[6] Point{{1.5, 2.5}} -> {}", type_name<decltype(pt)>());
    std::println("    Vec{{1, 2, 3}}    -> {}", type_name<decltype(vec)>());

    // ------------------------------------------------------------
    // 7. 推导指南
    // ------------------------------------------------------------
    Sum s1{1u, 2.0, 3, 4.0f};
    Sum s2{"abc"s, "def"};
    std::println("\n[7] Sum{{1u, 2.0, 3, 4.0f}} -> {}，值 {}", type_name<decltype(s1)>(), s1.value());
    std::println("    Sum{{\"abc\"s, \"def\"}}  -> 值 {}", s2.value());
    // Sum s3{s1}; // 编译失败！见练习的思考题
    Label label{"hello"};
    std::println("    Label{{\"hello\"}}       -> {}", type_name<decltype(label)>());

    // ------------------------------------------------------------
    // 8. 书上说 make_pair、make_tuple “已经过时”，说得太绝对了：
    //    (a) make_unique / make_shared 没法用 CTAD 代替（unique_ptr 不能从裸指针推导）
    //    (b) make_tuple 会把 std::ref(x) 变成真正的引用 int&，CTAD 不会（见练习）
    // ------------------------------------------------------------
    auto up = std::make_unique<int>(42);
    std::println("\n[8] make_unique<int>(42) -> {}", *up);
}
