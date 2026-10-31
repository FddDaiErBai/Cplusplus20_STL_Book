// 第 1 章 1.7 概念和约束 —— 练习
// 编译：clang++ -std=c++23 -Wall -Wextra concepts_exercise.cpp -o concepts_exercise.exe
// 用 static_assert 验证：补完 TODO，去掉断言的注释，能编译通过就说明做对了。
#include <concepts>
#include <numeric>
#include <string>
#include <string_view>
#include <vector>

// ============================================================
// 练习 1：定义概念 Number，并用它约束 average
//   Number：整数或浮点数，但不包括 bool（还记得 1.4 节 cmp_less 也不收 bool 吗？）
//   提示：std::integral<bool> 是 true；判断两个类型相同用 std::same_as<A, B>
// ============================================================
template <typename T>
concept Number = std::floating_point<T> || (std::integral<T> && (!std::same_as<T, bool>));

static_assert(Number<int>);
static_assert(Number<double>);
static_assert(Number<long long>);
static_assert(!Number<bool>);
static_assert(!Number<const char*>);
static_assert(!Number<std::string>);

template <Number T>
constexpr double average(const std::vector<T>& v)
{
    return std::accumulate(v.begin(), v.end(), 0.0) / v.size();
}

// 检测工具（已经写好，不用改）：average(vector<T>) 能不能调用
template <typename T>
concept CanAverage = requires(std::vector<T> v) { average(v); };

static_assert(average(std::vector{1, 2, 3, 4}) == 2.5);
static_assert(CanAverage<int>);
static_assert(CanAverage<double>);
static_assert(!CanAverage<bool>); // 加约束之前，这个会失败：bool 也能“求平均”
static_assert(!CanAverage<std::string>);

// ============================================================
// 练习 2：用 requires 表达式定义概念 HasArea
//   要求：类型有一个成员函数 area()，并且返回值能转换成 double
//   提示：复合要求 { 表达式 } -> std::convertible_to<double>;
// ============================================================
struct Circle
{
    double r;
    constexpr double area() const { return 3.14159 * r * r; }
};

struct Square
{
    int a;
    constexpr int area() const { return a * a; } // 返回 int，int 能转成 double，所以也算
};

struct Point // 没有 area()
{
    int x;
    int y;
};

struct Label // 有 area()，但返回的是字符串
{
    std::string area() const { return "big"; }
};

template <typename T>
concept HasArea = requires(const T& t)
{
    // 改动：删掉了 t.area(); 这一行。下面的复合要求已经包含“t.area() 必须合法”，再写一遍是重复的
    //       （示例里 HasSize 两种都写，只是为了分别演示两种写法）
    { t.area() } -> std::convertible_to<double>;
};

static_assert(HasArea<Circle>);
static_assert(HasArea<Square>);
static_assert(!HasArea<Point>);
static_assert(!HasArea<Label>);

// ============================================================
// 练习 3：按概念重载 kind()
//   有符号整数   -> "signed"     （提示：std::signed_integral）
//   无符号整数   -> "unsigned"   （提示：std::unsigned_integral）
//   浮点数       -> "floating"
//   其他所有类型 -> "other"
//   注意：几个版本的参数写法要一致，参考示例 concepts.cpp 的 describe
// ============================================================
constexpr std::string_view kind(std::signed_integral auto)
{
    return "signed";
}

constexpr std::string_view kind(std::unsigned_integral auto)
{
    return "unsigned";
}

constexpr std::string_view kind(std::floating_point auto)
{
    return "floating";
}

constexpr std::string_view kind(auto)
{
    return "other";
}

static_assert(kind(-5) == "signed");
static_assert(kind(5u) == "unsigned");
static_assert(kind(2.5) == "floating");
static_assert(kind("hi") == "other");

// 改动（新增）：两个容易意外的结果。标准概念把 bool 和 char 也当成整数：
//   bool 是整数且没有符号 -> unsigned_integral；char 在 x86 上有符号 -> signed_integral（ARM 上可能是 unsigned）
//   想把它们排除，就要像练习 1 的 Number 那样自己加条件
static_assert(kind(true) == "unsigned");
static_assert(kind('a') == "signed");

// ============================================================
// 练习 4（思考题，把答案写在注释里）
//   (1) 不加约束的 old_arg42("7") 为什么能编译通过？实际发生了什么？
//   (2) 书上写的是 auto arg42(Numeric auto& arg)。用它调用 arg42(5) 会怎样？为什么？怎么改？
//       （可以在示例 concepts.cpp 里把 arg42_d 的 const 去掉，试试 arg42_d(3)）
//   (3) 把示例 concepts.cpp 里 main 的 arg42_b("7"); 取消注释编译一次，
//       再写一个不加约束的版本用 std::string 调用，对比两次报错。哪个更容易看懂？为什么？
//
// 你的答案：
//   (1)const chat* +42是指针运算，不过是未定义行为
//       改动（补充）：正确（chat 是 char 的笔误）。编译器不拦，是因为普通模板只检查 arg + 42 这个写法
//       对 const char* 是否合法 —— 指针加整数是合法的。"7" 只有 2 个字节，+42 指到了数组外面，
//       读那里就是未定义行为。概念的作用就是让你把“哪些类型才有意义”告诉编译器。
//   (2)编译失败，5是字面量，改为const Numeric auto&
//       改动（补充）：正确。准确的原因是：非 const 的引用只能绑定到“有名字的变量”（左值），
//       5 是一个临时值（右值），不能绑定；const 引用可以绑定临时值，所以加 const 就好了。
//       int x = 5; arg42(x); 这样传变量，不加 const 也能编译。
//   (3)写了约束的更容易看懂，报错直接提示了不满足约束
//       改动（补充）：正确。还有一个区别是报错的位置：不加约束时，错误报在模板函数内部（arg + 42 那一行），
//       并列出一堆标准库里不相关的候选函数；加了约束，错误报在你调用的那一行，并一层层说明哪个条件没满足。
// ============================================================

int main()
{
}
