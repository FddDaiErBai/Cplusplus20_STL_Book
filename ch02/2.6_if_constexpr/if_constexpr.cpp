// 第 2 章 2.6 if constexpr —— 在编译时选择分支
// 编译：clang++ -std=c++23 -Wall -Wextra if_constexpr.cpp -o if_constexpr.exe
#include <format>
#include <print>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

// ------------------------------------------------------------
// 1. 书上的例子：T 是指针就解引用，否则直接返回
// ------------------------------------------------------------
template <typename T>
auto value_of(const T v)
{
    if constexpr (std::is_pointer_v<T>)
    {
        return *v;
    }
    else
    {
        return v;
    }
}

// ------------------------------------------------------------
// 2. 换成普通 if 为什么不行
//    普通 if 是在运行时判断的，编译器不知道会走哪个分支，所以两个分支都必须能编译。
//    T = int 时 *v 编译失败，哪怕这个分支永远不会执行。
//    if constexpr 在编译时就判断好了，没被选中的分支叫“被丢弃的分支”，编译器不会为当前的 T 生成它
// ------------------------------------------------------------
template <typename T>
auto value_of_bad(const T v)
{
    if (std::is_pointer_v<T>)
    {
        return *v;
    }
    else
    {
        return v;
    }
}

// ------------------------------------------------------------
// 3. 被丢弃的分支里的 return 不参与 auto 返回类型的推导
//    T = int** 时选中第一个分支：return deref_all(int*)，继续递归，直到不是指针为止。
//    第二个分支 return v（类型 int**）被丢弃了，所以不会和 int 冲突
// ------------------------------------------------------------
template <typename T>
auto deref_all(const T v)
{
    if constexpr (std::is_pointer_v<T>)
    {
        return deref_all(*v);
    }
    else
    {
        return v;
    }
}

// ------------------------------------------------------------
// 4. 多个分支：else if constexpr 链
//    注意：每个 else if 后面都要写 constexpr！只写 else if 就变回了普通 if（见练习 2）
//    C++20 起，条件里可以直接写 requires 表达式（1.7 节），检查“这个写法能不能编译”
//    最后的 else 里写 static_assert(false, "...")：所有分支都不匹配时，给出清楚的报错信息
// ------------------------------------------------------------
template <typename T>
std::string describe(const T& v)
{
    if constexpr (std::is_same_v<T, bool>) // bool 也算整数类型，所以要排在 is_arithmetic 前面
    {
        return v ? "布尔值 true" : "布尔值 false";
    }
    else if constexpr (std::is_arithmetic_v<T>)
    {
        return std::format("数字 {}", v);
    }
    else if constexpr (std::is_pointer_v<T>)
    {
        return "指针，指向" + describe(*v);
    }
    else if constexpr (requires { v.size(); })
    {
        return std::format("有 {} 个元素的容器", v.size());
    }
    else
    {
        static_assert(false, "describe 不支持这个类型");
    }
}

// ------------------------------------------------------------
// 5. static_assert(false) 以前不能这么写
//    C++23 之前的规则：static_assert(false) 不依赖 T，编译器在“还没确定 T”的时候就会检查它，
//    于是不管走不走这个分支都报错。当时的办法是造一个依赖 T 的 false：
//        template <typename>
//        inline constexpr bool always_false = false;
//        ...
//        static_assert(always_false<T>, "不支持这个类型");
//    C++23（P2593）改了规则，并且作为缺陷修复，-std=c++17 下也能用（clang 17+、g++ 13+）。
//    老代码里经常能看到 always_false、dependent_false 这种写法，知道是干什么的就行
// ------------------------------------------------------------

// ------------------------------------------------------------
// 6. && 不能“保护”后面的表达式
//    想写：“T 有 value_type 成员，并且 value_type 是整数”
//        if constexpr (requires { typename T::value_type; } && std::is_integral_v<typename T::value_type>)
//    T = int 时编译失败：&& 只是在“求值”时短路，但编译器要先把整个条件生成出来，
//    生成 int::value_type 这一步就已经报错了。正确的写法是拆成两层 if constexpr：
// ------------------------------------------------------------
template <typename T>
constexpr bool has_integral_elements()
{
    if constexpr (requires { typename T::value_type; })
    {
        if constexpr (std::is_integral_v<typename T::value_type>)
        {
            return true;
        }
    }
    return false;
}

static_assert(has_integral_elements<std::vector<int>>());
static_assert(!has_integral_elements<std::vector<double>>());
static_assert(!has_integral_elements<int>());

// ------------------------------------------------------------
// 7. 被丢弃的分支只在模板里才“不生成”
//    (a) 分支里的代码还是要先被解析：语法错误、和 T 无关的错误照样报错
//    (b) 不在模板里时，两个分支都会被完整检查，if constexpr 只剩下“不执行”的作用：
//            void f()
//            {
//                if constexpr (false)
//                {
//                    undeclared(); // 编译失败：undeclared 没有声明
//                }
//            }
//    (c) 条件必须是编译时常量。函数参数不是常量，哪怕在 constexpr 函数里也不是：
//            constexpr int g(int n)
//            {
//                if constexpr (n > 0) // 编译失败：n 不是常量表达式
//                ...
// ------------------------------------------------------------

// ------------------------------------------------------------
// 8. 容易混淆：if constexpr 和 if consteval（C++23）
//    if constexpr (条件)：按编译时已知的条件选分支
//    if consteval      ：判断“当前这次调用是不是在编译时执行”
//    C++20 用 std::is_constant_evaluated() 做同样的判断，但它要放在普通 if 里：
//        if constexpr (std::is_constant_evaluated()) // 错！永远是 true（见思考题 (3)）
// ------------------------------------------------------------
constexpr std::string_view where()
{
    if consteval
    {
        return "编译时";
    }
    else
    {
        return "运行时";
    }
}

int main()
{
    std::println("[1] value_of");
    int x{47};
    int* px{&x};
    int** ppx{&px};
    std::println("value_of(x)  = {}", value_of(x));
    std::println("value_of(px) = {}", value_of(px));
    // value_of_bad(x); // 编译失败：普通 if 的两个分支都要能编译，int 不能用 *

    std::println("\n[3] deref_all(ppx) = {}", deref_all(ppx)); // int** -> int* -> int

    std::println("\n[4] describe");
    std::println("{}", describe(true));
    std::println("{}", describe(3.5));
    std::println("{}", describe(px));
    std::println("{}", describe(std::string{"hello"}));
    std::println("{}", describe(std::vector{1, 2, 3}));
    // describe(nullptr); // 编译失败，报错信息里能看到 "describe 不支持这个类型"

    std::println("\n[8] if consteval");
    constexpr std::string_view a = where(); // constexpr 变量：一定在编译时计算
    std::string_view b = where();           // 普通变量：在运行时计算
    std::println("a = {}，b = {}", a, b);
}
