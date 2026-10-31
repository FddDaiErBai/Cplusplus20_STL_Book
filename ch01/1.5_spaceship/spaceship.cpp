// 第 1 章 1.5 三向比较运算符 <=>（宇宙飞船运算符）
// 编译：clang++ -std=c++23 -Wall -Wextra spaceship.cpp -o spaceship.exe
#include <algorithm>
#include <compare>
#include <limits>
#include <print>
#include <vector>

// 把比较结果转成文字，方便打印（ordering 对象不能直接 println）
constexpr const char* to_str(std::partial_ordering o)
{
    if (o < 0)
    {
        return "less";
    }
    if (o > 0)
    {
        return "greater";
    }
    if (o == 0)
    {
        return "equal";
    }
    return "unordered";
}

// ------------------------------------------------------------
// 1. 旧写法：六个运算符要一个一个写
// ------------------------------------------------------------
struct OldNum
{
    int a;
    constexpr bool operator==(const OldNum& rhs) const { return a == rhs.a; }
    constexpr bool operator!=(const OldNum& rhs) const { return a != rhs.a; }
    constexpr bool operator<(const OldNum& rhs) const { return a < rhs.a; }
    constexpr bool operator>(const OldNum& rhs) const { return a > rhs.a; }
    constexpr bool operator<=(const OldNum& rhs) const { return a <= rhs.a; }
    constexpr bool operator>=(const OldNum& rhs) const { return a >= rhs.a; }
};

// ------------------------------------------------------------
// 2. 新写法：一行 = default，六个运算符全有了
// ------------------------------------------------------------
struct Num
{
    int a;
    auto operator<=>(const Num&) const = default; // 默认的 <=> 还会顺带生成默认的 ==
};

static_assert(Num{7} < Num{42});
static_assert(Num{42} > Num{7});
static_assert(Num{7} == Num{7});
static_assert(Num{7} != Num{42});
static_assert(Num{7} <= Num{7});
static_assert(Num{42} >= Num{7});

// ------------------------------------------------------------
// 3. 默认的 <=> 按“成员声明的顺序”逐个比较（像查字典）
//    先比第一个成员，不相等就出结果；相等再比第二个……
// ------------------------------------------------------------
struct Date
{
    int year; // 年放最前面，比较结果就是时间先后
    int month;
    int day;
    auto operator<=>(const Date&) const = default;
};

struct BadDate
{
    int day; // 日放最前面：先比日，结果就错了
    int month;
    int year;
    auto operator<=>(const BadDate&) const = default;
};

static_assert(Date{2023, 12, 31} < Date{2024, 1, 1});        // 正确
static_assert(BadDate{31, 12, 2023} > BadDate{1, 1, 2024}); // 错了：2023 年反而“更大”

// ------------------------------------------------------------
// 4. 自己写 <=>：分数比较
// ------------------------------------------------------------
// 书里的写法：转成 double 再比。看起来没问题，但 double 精度有限
struct FracD
{
    long long n;
    long long d;
    constexpr double dbl() const { return static_cast<double>(n) / d; }
    constexpr auto operator<=>(const FracD& rhs) const { return dbl() <=> rhs.dbl(); }
    constexpr bool operator==(const FracD& rhs) const { return dbl() == rhs.dbl(); }
};

// 两个不同的分数，差值约 2e-19，比 double 能分辨的精度还小，被判成了“相等”
static_assert(FracD{2147483646, 2147483647} == FracD{2147483645, 2147483646});

// 更好的写法：交叉相乘，只用整数运算（假设分母都是正数）
// a/b 和 c/d 比较  ==>  a*d 和 c*b 比较
struct Frac
{
    long long n;
    long long d;
    // 返回 weak_ordering：10/15 和 2/3 “等价”，但它们的成员值并不一样（下面会讲）
    constexpr std::weak_ordering operator<=>(const Frac& rhs) const { return n * rhs.d <=> rhs.n * d; }
    // 自己写了 <=>，编译器就不会生成 ==，要自己写
    constexpr bool operator==(const Frac& rhs) const { return n * rhs.d == rhs.n * d; }
};

static_assert(Frac{2147483646, 2147483647} != Frac{2147483645, 2147483646}); // 正确区分
static_assert(Frac{10, 15} == Frac{2, 3});
static_assert(Frac{5, 3} > Frac{2, 3});

int main()
{
    // 5. <=> 的返回值：不是 int，而是一个“比较结果”对象，拿它和 0 比较
    int a{7};
    int b{42};
    int c{7};
    std::println("7 <=> 42  : {}", to_str(a <=> b));
    std::println("42 <=> 7  : {}", to_str(b <=> a));
    std::println("7 <=> 7   : {}", to_str(a <=> c));

    // 6. 浮点数返回 partial_ordering，因为 NaN（非数字）和谁都“没法比”
    double nan{std::numeric_limits<double>::quiet_NaN()};
    std::println("1.0 <=> 2.0 : {}", to_str(1.0 <=> 2.0));
    std::println("NaN <=> 1.0 : {}", to_str(nan <=> 1.0));
    std::println("NaN == NaN  : {}", nan == nan); // 连自己都不等于自己

    // 7. 有了 <=>，std::sort 直接就能排序
    std::vector<Date> dates{{2024, 3, 1}, {2023, 12, 31}, {2024, 1, 15}};
    std::sort(dates.begin(), dates.end());
    for (const auto& d : dates)
    {
        std::println("{}-{:02}-{:02}", d.year, d.month, d.day);
    }
}
