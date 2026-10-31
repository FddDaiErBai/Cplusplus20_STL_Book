// 第 2 章 2.2 std::span —— 练习
// 编译：clang++ -std=c++23 -Wall -Wextra span_exercise.cpp -o span_exercise.exe
// 所有函数都是 constexpr，用 static_assert 在编译时检查：补完 TODO，去掉断言的注释，能编译通过就说明做对了。
// （测试数据前面的 inline 只是为了在断言还没取消注释时，不出现“变量未使用”的警告，不用管它）
#include <algorithm>
#include <array>
#include <cstddef>
#include <span>
#include <vector>

// ============================================================
// 练习 1：长度为 k 的连续片段中，和最大的那一段的和
//   例：{1, 3, -1, 5, 2}, k = 2 -> 片段 {1,3} {3,-1} {-1,5} {5,2}，和分别是 4 2 4 7 -> 7
//   要求：用 subspan 取出每一段，交给 sum 求和（sum 已经写好）
//   约定：k 为 0 或者 k 比元素个数还大时，返回 0
// ============================================================
constexpr int sum(std::span<const int> s)
{
    int total = 0;
    for (int x : s)
    {
        total += x;
    }
    return total;
}

constexpr int max_window_sum(std::span<const int> s, std::size_t k)
{
    if (k == 0 || k > s.size())
    {
        return 0;
    }
    int max_sum{sum(s.subspan(0, k))};
    for (std::size_t i = 1; i <= (s.size() - k); ++i)
    {
        max_sum = std::max(max_sum, sum(s.subspan(i, k)));
    }
    return max_sum;
}

inline constexpr int data1[]{1, 3, -1, 5, 2};
static_assert(max_window_sum(data1, 2) == 7);
static_assert(max_window_sum(data1, 1) == 5);
static_assert(max_window_sum(data1, 5) == 10);
static_assert(max_window_sum(data1, 6) == 0);
static_assert(max_window_sum(data1, 0) == 0);
static_assert(max_window_sum(std::vector{-3, -1, -2}, 2) == -3); // 全是负数时，最大的和也是负数

// ============================================================
// 练习 2：通过 span 原地反转元素（会修改原数据，所以参数是 span<int>）
//   要求：自己用下标交换，不要用 std::reverse
//   提示：交换可以用 std::swap（<utility>，已经被 <span> 间接包含了，也可以自己 #include）
// ============================================================
constexpr void reverse_in_place(std::span<int> s)
{
    for (std::size_t i = 0; i < s.size() / 2; ++i)
    {
        std::swap(s[i], s[s.size() - 1 - i]);
    }
}

// 检测工具（已经写好，不用改）：在编译时创建数组、调用函数、检查结果
constexpr bool test_reverse()
{
    int a[]{1, 2, 3, 4, 5};
    reverse_in_place(a);
    std::vector<int> v{1, 2, 3, 4};
    reverse_in_place(std::span{v}.first(2)); // 只反转前两个
    return a[0] == 5 && a[1] == 4 && a[2] == 3 && a[4] == 1 && v == std::vector{2, 1, 3, 4};
}

static_assert(test_reverse());

// ============================================================
// 练习 3：判断 s 是不是以 prefix 开头
//   例：{1, 2, 3, 4} 以 {1, 2} 开头 -> true；以 {2, 3} 开头 -> false
//   要求：用 first 取出 s 的开头一段，再和 prefix 逐个比较（或者用 std::ranges::equal，需要 #include <algorithm>）
//   注意：prefix 比 s 还长时，first 会越界！要先判断
// ============================================================
constexpr bool starts_with(std::span<const int> s, std::span<const int> prefix)
{
    if (prefix.size() > s.size())
    {
        return false;
    }
    return std::ranges::equal(s.first(prefix.size()), prefix);
}

inline constexpr int data2[]{1, 2, 3, 4};
inline constexpr int pre_ok[]{1, 2};
inline constexpr int pre_bad[]{2, 3};
inline constexpr int pre_long[]{1, 2, 3, 4, 5};
static_assert(starts_with(data2, pre_ok));
static_assert(!starts_with(data2, pre_bad));
static_assert(!starts_with(data2, pre_long));
static_assert(starts_with(data2, data2));
static_assert(starts_with(data2, {})); // 空的 prefix：任何序列都以它开头

// ============================================================
// 练习 4：固定长度的 span
//   把下面 dot3 的两个参数类型改成“长度固定为 3 的只读 span”，让长度不是 3 的数组在编译时就被拒绝
//   函数体不用改
// ============================================================
constexpr int dot3(std::span<const int, 3> a, std::span<const int, 3> b)
{
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

// 检测工具（已经写好，不用改）：T 类型的对象能不能传给 dot3
template <typename T>
concept CanDot3 = requires(const T& t) { dot3(t, t); };

inline constexpr int v3a[]{1, 2, 3};
inline constexpr std::array<int, 3> v3b{4, 5, 6};
static_assert(dot3(v3a, v3b) == 32);
static_assert(CanDot3<int[3]>);
static_assert(CanDot3<std::array<int, 3>>);
static_assert(!CanDot3<int[2]>); // 改之前这行会失败：长度 2 也能传进去，a[2] 就越界了
static_assert(!CanDot3<int[4]>);
static_assert(!CanDot3<std::array<int, 4>>);
static_assert(!CanDot3<std::vector<int>>); // vector 的长度编译时不知道，也不能直接传

// ============================================================
// 练习 5（思考题，把答案写在注释里）
//   (1) 书上的 pspan 是模板 template<typename T> void pspan(span<T> s)，调用时写的是 pspan<int>(carray)。
//       把 <int> 去掉会怎样？为什么？（提示：模板参数推导时，不会考虑隐式类型转换）
//       示例里的 sum 为什么不需要写尖括号？
//   (2) 示例里 double_all 的参数是 span<int>。下面两种调用都编译失败，各是为什么？
//         const std::vector<int> cv{1, 2, 3};  double_all(cv);
//         double_all(std::vector{1, 2, 3});
//       但 sum(std::vector{1, 2, 3}) 可以编译，而且是安全的。为什么作为函数参数是安全的，
//       而示例第 8 部分的 std::span<const int> bad = std::vector{1, 2, 3}; 却是悬空的？
//   (3) 把示例第 7 部分 s[10] 那一行取消注释，分别用下面三条命令编译运行，结果有什么不同？
//         g++ -std=c++23 span.cpp -o span_g.exe -lstdc++exp
//         g++ -std=c++23 -O2 span.cpp -o span_g2.exe -lstdc++exp
//         clang++ -std=c++23 span.cpp -o span_c.exe
//       这说明“测试时不报错”能不能证明程序没有越界？
//
// 你的答案：
//   (1)不可以，模板参数推导时，不会考虑隐式类型转换。sum参数已经写为std::span<const int>
//       改动（补充）：正确。展开说：去掉 <int> 后，编译器要拿实参类型 int[10] 去匹配 span<T>，
//       可 int[10] 根本不是 span，匹配不上，T 推导失败（推导只看类型的“形状”，不会先转换再匹配）。
//       写了 <int>，T 已经确定，参数就是普通的 span<int>，int[10] -> span<int> 的隐式转换就可以发生了。
//       sum 不是模板，没有推导这一步，所以直接转换。
//       想保留模板又不写尖括号，可以让调用者先转好：pspan(std::span{carray})，span 自己能推导出元素类型。
//   (2)前者double_all(cv);是因为是const修饰的span传入函数参数为非const类型的函数，后者double_all(std::vector{1, 2, 3});是因为临时构造的vector对象，span不拥有数据，对象临时构造销毁之后，span就悬空了。sum(std::vector{1, 2, 3})的函数参数是const只读
//       改动（补充）：前者方向对，说法要改一下：const 的是 vector，不是 span。span<int> 能修改元素，
//       如果允许它指向 const vector，就能绕过 const 去改 cv，所以被拒绝。
//       后者的原因不是悬空：临时对象会活到整条语句结束（分号处），double_all 执行期间 vector 一直都在。
//       真正的原因是：修改一个马上就要销毁的临时对象没有意义（改完谁也看不到），标准干脆规定
//       span<int> 不能从临时的 vector 构造；span<const int> 只读，没有这个问题，所以允许。
//       “作为函数参数安全、bad 却悬空”的区别在于活多久：函数参数用完时语句还没结束，vector 还在；
//       bad 是变量，这一行结束 vector 就销毁了，bad 却还要继续用。
//   (3)没有测试
//       改动（补充）：我替你跑了一遍：
//         g++（不加 -O）：程序中止，报 Assertion '__idx < size()' failed —— g++ 15 起不优化时默认检查越界
//         g++ -O2：检查关掉了，打印出一个莫名其妙的数（我这次是 224394544），正常退出
//         clang++：打印 0，正常退出，没有任何提示
//       所以“测试时不报错”不能证明没有越界：越界是未定义行为，可能恰好读到一个看着正常的值。
//       调试时要用会检查的编译方式（g++ 不加 -O，或 clang 加 hardening 宏），别依赖“看起来没问题”。
// ============================================================

int main()
{
}
