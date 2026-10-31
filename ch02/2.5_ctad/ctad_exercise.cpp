// 第 2 章 2.5 模板参数推导 —— 练习
// 编译：clang++ -std=c++23 -Wall -Wextra ctad_exercise.cpp -o ctad_exercise.exe
// 用 static_assert 在编译时检查：补完 TODO，去掉断言的注释，能编译通过就说明做对了。
#include <algorithm>
#include <array>
#include <concepts>
#include <functional>
#include <initializer_list>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

using namespace std::string_literals;

// ============================================================
// 练习 1：先猜，再验证
//   把每个 AnsN 的 void 换成你认为的类型，然后取消对应断言的注释。
//   要求：先在脑子里（或纸上）写出答案，再编译验证；猜错了的在思考题里记一下为什么
// ============================================================
template <typename T>
T by_value(T); // 只有声明，没有定义：只用在 decltype 里，不会真的调用

template <typename T>
std::type_identity<T> by_cref(const T&); // type_identity<T>::type 就是 T，用来把推导出的 T“带出来”

inline int x = 0;
inline const std::vector<int> v{1, 2, 3};

using Ans1 = const char*;                             // by_value("hi") 推导出的 T
using Ans2 = char[3];                                 // by_cref("hi") 推导出的 T
using Ans3 = std::pair<const char*, int>;             // std::pair{"Tom", 20}
using Ans4 = std::vector<int>;                        // std::vector{v}
using Ans5 = std::vector<std::vector<int>>;           // std::vector{v, v}
using Ans6 = std::array<double, 2>;                   // std::array{1.5, 2.5}
using Ans7 = std::tuple<std::reference_wrapper<int>>; // std::tuple{std::ref(x)}
using Ans8 = std::tuple<int&>;                        // std::make_tuple(std::ref(x))

static_assert(std::same_as<decltype(by_value("hi")), Ans1>);
static_assert(std::same_as<decltype(by_cref("hi"))::type, Ans2>);
static_assert(std::same_as<decltype(std::pair{"Tom", 20}), Ans3>);
static_assert(std::same_as<decltype(std::vector{v}), Ans4>);
static_assert(std::same_as<decltype(std::vector{v, v}), Ans5>);
static_assert(std::same_as<decltype(std::array{1.5, 2.5}), Ans6>);
static_assert(std::same_as<decltype(std::tuple{std::ref(x)}), Ans7>);
static_assert(std::same_as<decltype(std::make_tuple(std::ref(x))), Ans8>);

// ============================================================
// 练习 2：推导冲突
//   下面两行如果直接写 std::max(3, 7.5) 和 std::min(10u, 4) 都编译不过。
//   用两种不同的方法让它们编译通过：
//     第一行：手动指定模板参数
//     第二行：不指定模板参数，改实参（让两个实参类型一样）
// ============================================================
static_assert(std::max<double>(3, 7.5) == 7.5);
static_assert(std::min(10, 4) == 4);

// ============================================================
// 练习 3：写推导指南
//   Wrapper 是一个简单的包装类。现在 Wrapper w{"hi"} 会推导成 Wrapper<const char*>。
//   写一个推导指南，让它推导成 Wrapper<std::string>
// ============================================================
template <typename T>
class Wrapper
{
public:
    constexpr Wrapper(T v) : value_{v}
    {
    }

    constexpr const T& get() const
    {
        return value_;
    }

private:
    T value_;
};

Wrapper(const char*) -> Wrapper<std::string>;

static_assert(std::same_as<decltype(Wrapper{"hi"}), Wrapper<std::string>>);
static_assert(std::same_as<decltype(Wrapper{42}), Wrapper<int>>); // 其他类型不受影响
static_assert(Wrapper{"hi"}.get() == "hi");

// ============================================================
// 练习 4：参数包 + 推导指南
//   Max 的构造函数接受任意多个参数，保存其中最大的那个。
//   (a) 补完构造函数：用 std::max 的 initializer_list 版本 std::max({a, b, c, ...})
//       提示：参数包展开成 {static_cast<T>(values)...}，先统一转成 T 再比较
//   (b) 写推导指南：T 取所有参数的公共类型（参考示例的 Sum）
// ============================================================
template <typename T>
class Max
{
public:
    template <typename... Ts>
    constexpr Max(Ts... values) : value_{std::max({static_cast<T>(values)...})}
    {}

    constexpr const T& value() const
    {
        return value_;
    }

private:
    T value_{};
};

template <typename... Ts>
Max(Ts...) -> Max<std::common_type_t<Ts...>>;
static_assert(std::same_as<decltype(Max{3, 7.5, 2}), Max<double>>);
static_assert(Max{3, 7.5, 2}.value() == 7.5);
static_assert(Max{4, 9, 1}.value() == 9);
static_assert(Max{-1}.value() == -1);

// ============================================================
// 练习 5（思考题，把答案写在注释里）
//   (1) 练习 1 里你猜错了哪几个？各是为什么？（全对也写一句你觉得最容易错的是哪个）
//   (2) 书上说 f(const T& a) 调用 f(47)“不能编译”。对吗？换成 f(T& a) 呢？为什么？
//   (3) 书上说 make_pair、make_tuple“已经过时了”。根据练习 1 的 Ans7 和 Ans8，
//       说说 make_tuple 和直接写 std::tuple{...} 有什么区别；再举一个 CTAD 代替不了的 make_ 函数
//
// 你的答案：
//   (1)最后两个，完全不懂
//       改动（补充）：你写的答案其实是对的（断言都通过了），下面解释为什么。
//       ① std::ref(x) 是什么：它返回一个 std::reference_wrapper<int> 对象，可以理解成“能装进容器、
//          能复制的引用”。普通引用 int& 不能放进 vector，也不能改指向，reference_wrapper 可以。
//          它里面存着 x 的地址，调用 .get() 就拿到 int&。
//       ② make_tuple 是什么：一个普通的函数模板，作用是“不写类型就造出 tuple”：
//            auto t = std::make_tuple(1, 2.5);   // 等于 std::tuple<int, double> t{1, 2.5};
//          它有一条特殊规则：参数如果是 reference_wrapper<T>，元素类型就变成 T&（自动“拆开”）。
//          所以 make_tuple(std::ref(x)) 是 tuple<int&> —— 这正是 std::ref 存在的意义：告诉 make_tuple“这里要引用”。
//       ③ std::tuple{std::ref(x)} 走的是 CTAD，CTAD 没有这条特殊规则，参数是什么类型就存什么类型，
//          所以是 tuple<reference_wrapper<int>>。
//       两者用起来的区别，见下面新增的两个断言。
//   (2)不对，const修饰的引用可以接左值，纯右值和亡值，47是纯右值。左值引用接左值，f(T& a)不可以过编译
//       改动（补充）：正确，术语也用得很准。
//   (3)同问题1，完全不懂make_pair、make_tuple
//       改动（补充）：先说它们为什么存在。C++17 之前，类模板不能推导，只有函数模板能推导：
//            std::pair<int, double> p(1, 2.5);   // 类模板：类型必须自己写
//            auto p = std::make_pair(1, 2.5);    // 函数模板：编译器帮你推导
//          所以标准库提供了 make_pair、make_tuple 这些“造对象的函数”，专门用来省掉类型。
//          C++17 有了 CTAD，直接写 std::pair p{1, 2.5} 就行，大部分情况下 make_pair 确实用不着了。
//       但说“过时”太绝对：
//          - make_tuple / make_pair 会把 std::ref 拆成真正的引用，CTAD 不会（就是 Ans7 和 Ans8 的区别）
//          - make_unique / make_shared 没法用 CTAD 代替：std::unique_ptr p{new int{42}} 推导不出来，
//            而且 make_unique 更安全（不用自己写 new）
// ============================================================

// 改动（新增）：Ans7 和 Ans8 用起来的区别
static_assert([]
{
    int y = 1;
    auto t = std::make_tuple(std::ref(y)); // tuple<int&>
    std::get<0>(t) = 5;                     // 元素就是 int&，直接赋值，改的就是 y
    return y == 5;
}());
static_assert([]
{
    int y = 1;
    std::tuple t{std::ref(y)}; // tuple<reference_wrapper<int>>
    std::get<0>(t).get() = 5;  // 元素是 reference_wrapper，要先 .get() 拿到 int& 再赋值
    // std::get<0>(t) = 5;     // 编译失败：不能把 int 赋给 reference_wrapper
    return y == 5;
}());

int main()
{
}
