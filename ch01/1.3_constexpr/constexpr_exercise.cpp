// 第 1 章 1.3 constexpr —— 练习
// 编译：clang++ -std=c++23 -Wall -Wextra constexpr_exercise.cpp -o constexpr_exercise.exe
// 这个文件现在就能编译。补完 TODO 后，把 static_assert 的注释去掉，
// 能编译通过，就说明你的函数在“编译时”算对了 —— 不需要运行程序。
#include <algorithm>
#include <array>
#include <print>
#include <string>
#include <string_view>
#include <vector>

// ============================================================
// 练习 1（热身）：编译时判断回文
//   "level" 正着读反着读一样 -> true
//   "hello"                  -> false
// 要求：在函数里用 std::string 拷一份，再用 std::reverse 反转，最后比较
// ============================================================
constexpr bool is_palindrome(std::string_view s)
{
    std::string s_copy{s};
    std::reverse(s_copy.begin(), s_copy.end());
    return s_copy == s; // 改动：string 可以直接和 string_view 比较，不用再包一层 string_view{...}
}

static_assert(is_palindrome("level"));
static_assert(is_palindrome("abba"));
static_assert(!is_palindrome("hello"));
static_assert(is_palindrome("")); // 空串也算回文

// ============================================================
// 练习 2（套路）：编译时生成斐波那契数列，带到运行时打印
//   前 10 项：0 1 1 2 3 5 8 13 21 34
// 步骤：
//   1. fib_vector(n)：用 std::vector<long long> 算出前 n 项
//   2. fib_array<N>()：把结果拷进 std::array<long long, N>
//      （这里个数就是 N，比示例里的质数表更简单，不用算两遍）
// ============================================================
constexpr std::vector<long long> fib_vector(int n)
{
    // 改动：原来按 n==1 / n==2 / n>2 分三种情况处理（你补上 push_back(0) 后结果已经正确），
    //       这里改成一个循环统一处理，不再有单独的分支可能写错：
    //       a 是当前项，b 是下一项，每次放入 a 后两者往后挪一位
    std::vector<long long> fib{};
    long long a{0};
    long long b{1};
    for (int i = 0; i < n; ++i)
    {
        fib.push_back(a);
        long long next{a + b};
        a = b;
        b = next;
    }
    return fib;
}

// 改动：新增边界测试，0、1、2 这些最小值最容易写错
static_assert(fib_vector(0).empty());
static_assert(fib_vector(1).size() == 1);
static_assert(fib_vector(2)[0] == 0 && fib_vector(2)[1] == 1);

template <int N>
constexpr auto fib_array()
{
    std::array<long long, N> arr{};
    auto vec = fib_vector(N);
    std::copy(vec.begin(), vec.end(), arr.begin());
    return arr;
}

static_assert(fib_array<10>()[9] == 34);
static_assert(fib_array<50>()[49] == 7778742049);

// ============================================================
// 练习 3（思考题，不用写代码，把答案写在下面注释里）
//   (1) 为什么 constexpr auto v = fib_vector(10); 编译失败，
//       而 constexpr auto n = fib_vector(10).size(); 可以？
//   (2) 示例 constexpr.cpp 里，int b = square(input); 是在编译时算的还是运行时算的？为什么？
//   (3) 把练习 2 的 fib_array<10> 改成 fib_array<100>，会发生什么？（提示：long long 最大约 9.2e18）
//
// 你的答案：
//   (1) vector的size算出之后就会销毁
//       改动（补充）：第一种写法想把 vector 本身保存成编译时常量，但它的数据在编译时分配的堆内存里，
//       这块内存到运行时并不存在，所以不允许带出去。第二种只带出一个数字，vector 在编译时就释放了。
//   (2) 运行时计算的，因为参数是变量
//   (3) 有符号溢出
//       改动（补充）：编译时计算遇到溢出会直接编译失败（报 value ... is outside the range of
//       representable values of type 'long long'）。同样的代码在运行时溢出是未定义行为，可能悄悄算错。
// ============================================================

int main()
{
    std::println("--- 练习 1 ---");
    std::println("level: {}", is_palindrome("level"));
    std::println("hello: {}", is_palindrome("hello"));

    std::println("--- 练习 2 ---");
    constexpr auto fib = fib_array<10>();
    std::println("{}", fib);
}
