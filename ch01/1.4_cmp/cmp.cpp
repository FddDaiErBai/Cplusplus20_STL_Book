// 第 1 章 1.4 安全地比较不同类型的整数：std::cmp_less 等
// 编译：clang++ -std=c++23 -Wall -Wextra cmp.cpp -o cmp.exe
#include <print>
#include <type_traits>
#include <utility>
#include <vector>

// 书中给出的 cmp_less 参考实现（加 my_ 前缀，避免和标准库重名）
// if constexpr 是 2.6 节的内容，这里先理解成“编译时就选好走哪个分支的 if”
template <class T, class U>
constexpr bool my_cmp_less(T t, U u) noexcept
{
    using UT = std::make_unsigned_t<T>; // T 对应的无符号类型，例如 int -> unsigned int
    using UU = std::make_unsigned_t<U>;
    if constexpr (std::is_signed_v<T> == std::is_signed_v<U>)
    {
        return t < u; // 两边符号相同：直接比，不会出问题
    }
    else if constexpr (std::is_signed_v<T>)
    {
        return t < 0 ? true : UT(t) < u; // t 有符号：负数一定更小；非负数转成无符号再比
    }
    else
    {
        return u < 0 ? false : t < UU(u); // u 有符号：u 是负数，t 不可能比它小
    }
}

static_assert(my_cmp_less(-3, 7u));
static_assert(!my_cmp_less(7u, -3));
static_assert(my_cmp_less(1, 2));

int main()
{
    int x{-3};
    unsigned y{7};

    // 1. 陷阱：写 x < y 时，编译器会先把 x 转成 unsigned 再比较，相当于下面这样
    //    （直接写 x < y 结果一样，加了 -Wall -Wextra 会出现 -Wsign-compare 警告）
    unsigned converted{static_cast<unsigned>(x)};
    std::println("-3 转成 unsigned 后是 {}（十六进制 {:#x}）", converted, converted);
    std::println("所以 x < y 的结果是 {}", converted < y);

    // 2. C++20 的解决方法：<utility> 里的六个比较函数
    std::println("cmp_equal(x, y)         = {}", std::cmp_equal(x, y));         // x == y
    std::println("cmp_not_equal(x, y)     = {}", std::cmp_not_equal(x, y));     // x != y
    std::println("cmp_less(x, y)          = {}", std::cmp_less(x, y));          // x <  y
    std::println("cmp_less_equal(x, y)    = {}", std::cmp_less_equal(x, y));    // x <= y
    std::println("cmp_greater(x, y)       = {}", std::cmp_greater(x, y));       // x >  y
    std::println("cmp_greater_equal(x, y) = {}", std::cmp_greater_equal(x, y)); // x >= y

    // 3. 最常见的场景：int 下标和 v.size()（无符号的 size_t）比较
    std::vector<int> v{10, 20, 30};
    int i{-1};
    std::println("cmp_less({}, v.size()) = {}", i, std::cmp_less(i, v.size()));

    // 4. 书里没讲的 C++20 函数 in_range<T>(x)：x 能不能原样放进 T 类型
    std::println("in_range<unsigned char>(200) = {}", std::in_range<unsigned char>(200));
    std::println("in_range<unsigned char>(300) = {}", std::in_range<unsigned char>(300));
    std::println("in_range<unsigned char>(-1)  = {}", std::in_range<unsigned char>(-1));

    // 5. 限制：不接受 bool 和字符类型，下面两行取消注释都会编译失败
    // std::cmp_less('a', 98);
    // std::cmp_less(true, 2);
}
