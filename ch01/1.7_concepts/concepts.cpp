// 第 1 章 1.7 概念（concept）和约束（constraint）
// 编译：clang++ -std=c++23 -Wall -Wextra concepts.cpp -o concepts.exe
#include <concepts>
#include <print>
#include <string>
#include <string_view>
#include <vector>

// ------------------------------------------------------------
// 1. 问题：普通模板什么类型都收
// ------------------------------------------------------------
template <typename T>
T old_arg42(const T& arg)
{
    return arg + 42;
}
// old_arg42("7") 能编译通过：T 是 const char*，"指针 + 42" 是合法的指针运算，
// 结果指向字符串后面不知道什么地方的内存 —— 编译器不报错，运行时出事。

// ------------------------------------------------------------
// 2. 定义一个概念：concept 名字 = 条件;
//    条件是一个编译时的 bool，可以用 && || ! 组合
// ------------------------------------------------------------
template <typename T>
concept Numeric = std::integral<T> || std::floating_point<T>; // 两个都是 <concepts> 里的标准概念

static_assert(Numeric<int>);
static_assert(Numeric<double>);
static_assert(!Numeric<const char*>);
static_assert(!Numeric<std::string>);

// ------------------------------------------------------------
// 3. 用概念约束模板：四种写法，效果完全一样，选你觉得顺眼的
// ------------------------------------------------------------
// 写法 1：requires 子句写在 template 下面
template <typename T>
    requires Numeric<T>
T arg42_a(const T& arg)
{
    return arg + 42;
}

// 写法 2：概念直接代替 typename（最常用）
template <Numeric T>
T arg42_b(const T& arg)
{
    return arg + 42;
}

template <typename T>
T arg42_b_n(const T& arg)
{
    return arg + 42;
}

// 写法 3：requires 子句写在函数参数列表后面
template <typename T>
T arg42_c(const T& arg)
    requires Numeric<T>
{
    return arg + 42;
}

// 写法 4：简化函数模板，概念写在 auto 前面
// 注意：书上写的是 Numeric auto&，少了 const，arg42(5) 这种传字面量的调用会编译失败
auto arg42_d(const Numeric auto& arg)
{
    return arg + 42;
}

// ------------------------------------------------------------
// 4. requires 表达式（书里没讲，但最实用）：检查“这段代码能不能编译”
//    requires(参数) { 要求1; 要求2; }
// ------------------------------------------------------------
template <typename T>
concept HasSize = requires(const T& c) {
    c.size();                                      // 简单要求：c.size() 这个表达式要合法
    { c.size() } -> std::convertible_to<std::size_t>; // 复合要求：还要求结果能转成 size_t
};

static_assert(HasSize<std::string>);
static_assert(HasSize<std::vector<int>>);
static_assert(!HasSize<int>);

void print_size(const HasSize auto& c)
{
    std::println("size = {}", c.size());
}

// 用 requires 表达式检测“这个函数能不能这样调用”：
// 不加约束的 old_arg42 对 const char* 也“能调用”，加了约束的就不能
template <typename T>
concept CanCallOld = requires(T x) { old_arg42(x); };
template <typename T>
concept CanCallNew = requires(T x) { arg42_b(x); };

static_assert(CanCallOld<const char*>);  // 能调用 —— 这就是问题所在
static_assert(!CanCallNew<const char*>); // 不能调用 —— 编译时就拦住了

// ------------------------------------------------------------
// 5. 按概念重载：同名函数，根据类型满足哪个概念选择不同版本
//    注意：几个版本的参数写法要一致（都按值传 auto），否则编译器会说“有歧义”
// ------------------------------------------------------------
constexpr std::string_view describe(std::integral auto)
{
    return "整数";
}

constexpr std::string_view describe(std::floating_point auto)
{
    return "浮点数";
}

constexpr std::string_view describe(auto) // 没有约束：其他所有类型
{
    return "其他";
}

int main()
{
    std::println("arg42_a(1)   = {}", arg42_a(1));
    std::println("arg42_b(1.5) = {}", arg42_b(1.5));
    std::println("arg42_c(2)   = {}", arg42_c(2));
    std::println("arg42_d(3)   = {}", arg42_d(3));
    // arg42_b("7"); // 取消注释：编译失败，错误信息会说 const char* 不满足 Numeric
    // arg42_b_n(std::string{"7"});

    print_size(std::string{"hello"});
    print_size(std::vector<int>{1, 2, 3});
    // print_size(42);            // 取消注释：编译失败，int 不满足 HasSize

    std::println("42    -> {}", describe(42));
    std::println("3.14  -> {}", describe(3.14));
    std::println("\"hi\"  -> {}", describe("hi"));
}
