// 第 1 章 1.4 安全地比较不同类型的整数 —— 练习
// 编译：clang++ -std=c++23 -Wall -Wextra cmp_exercise.cpp -o cmp_exercise.exe
// 这次只用 static_assert 验证，能编译通过就说明做对了，不需要运行。
// 注意：这个文件现在编译会有 1 个 -Wsign-compare 警告，它指向的就是练习 2 的 bug。
#include <limits>
#include <type_traits>
#include <utility>

// ============================================================
// 练习 1（预测）：把 ??? 换成你的答案（true / false / 数字），再去掉注释
// 先自己想，不要急着编译。全部能编译通过，说明你都猜对了。
// ============================================================
static_assert(static_cast<unsigned>(-1) == 4294967295u); // 提示：unsigned 是 32 位  改动：加 u，两边都是 unsigned
static_assert(std::cmp_less(-1, 0u) == true);
static_assert(std::cmp_equal(-1, static_cast<unsigned>(-1)) == false);
static_assert(std::cmp_greater(std::numeric_limits<unsigned>::max(), -1) == true);
static_assert(std::in_range<unsigned char>(255) == true);
static_assert(std::in_range<unsigned char>(256) == false);
static_assert(std::in_range<int>(3'000'000'000LL) == false);

// ============================================================
// 练习 2（修 bug）：判断余额够不够付款
//   balance 是余额，可能是负数（欠款）；cost 是价格，不会是负数
//   下面的写法看起来没问题，但余额是 -5 时，居然判断“付得起” 3 元的东西
// 步骤：
//   1. 先只取消注释第三个 static_assert，编译看看它失败，确认 bug 存在
//   2. 修好函数，让四个断言都通过，并且编译时没有警告
// ============================================================
constexpr bool can_afford(int balance, unsigned cost)
{
    return std::cmp_less_equal(cost, balance); // 改动：bug 已修好，删掉了 TODO
}

static_assert(can_afford(100, 30u));
static_assert(!can_afford(10, 30u));
static_assert(!can_afford(-5, 3u)); // 欠款时不能付款
static_assert(can_afford(30, 30u)); // 刚好够

// ============================================================
// 练习 3（仿写）：参考 cmp.cpp 里的 my_cmp_less，写出 my_cmp_equal
//   结构完全一样，三个分支，想清楚每个分支里“有符号的一边是负数”时该返回什么
// ============================================================
template <class T, class U>
constexpr bool my_cmp_equal(T t, U u) noexcept
{
    // 改动：删掉了 TODO；前两个分支也改成单行，和第三个分支保持一致
    using UT = std::make_unsigned_t<T>;
    using UU = std::make_unsigned_t<U>;
    if constexpr (std::is_signed_v<T> == std::is_signed_v<U>)
    {
        return t == u;
    }
    else if constexpr (std::is_signed_v<T>)
    {
        return t < 0 ? false : static_cast<UT>(t) == u;
    }
    else
    {
        return u < 0 ? false : t == static_cast<UU>(u);
    }
}

static_assert(!my_cmp_equal(-1, static_cast<unsigned>(-1))); // 普通的 == 会认为它们相等
static_assert(!my_cmp_equal(static_cast<unsigned>(-1), -1)); // 参数反过来也要对
static_assert(my_cmp_equal(5, 5u));
static_assert(my_cmp_equal(5u, 5));
static_assert(my_cmp_equal(-3, -3));
static_assert(my_cmp_equal(0, 0u));

// ============================================================
// 练习 4（思考题，把答案写在注释里）
//   (1) my_cmp_less 里，为什么执行到 UT(t) < u 时把 t 转成无符号是安全的？
//   (2) short a = -1; unsigned short b = 1;  a < b 的结果是 true 还是 false？
//       和 int / unsigned 的情况一样吗？为什么？（提示：搜索“整数提升”）
//   (3) 为什么标准规定 cmp_less 不接受 char 和 bool？
//
// 你的答案：
//   (1)应该和使用static_cast<UT>(t)一样
//       改动（补充）：这一点没错，但安全的原因是前面的 t < 0 已经把负数排除了，
//       能执行到 UT(t) 时 t 一定 >= 0，非负数转成同样大小的无符号类型，值不会变。
//   (2)不懂
//       改动（补充）：结果是 true，和 int / unsigned 不一样。
//       整数提升：比 int 小的类型（char、short、unsigned short、bool）参与运算前先转成 int，
//       int 能装下所有 unsigned short 的值，所以两边都变成 int，-1 < 1 正常比较。
//       int 和 unsigned 大小相同，不会提升，只能统一成 unsigned，-1 就变成了 4294967295。
//       所以这个坑只出现在 int 及更大的类型和它们的无符号版本之间（比如 int 和 size_t）。
//   (3)不懂
//       改动（补充）：bool 表示真假、char 表示字符，拿来和数字比较多半是写错了；
//       而且 char 是有符号还是无符号由平台决定（x86 上通常有符号，很多 ARM 上无符号），
//       没法保证“安全比较”。所以标准直接让它们编译失败，确实要比较就先 static_cast<int>(c)。
// ============================================================

int main()
{}
