// 第 1 章 1.6 <version> 特性测试宏 —— 练习
// 目标：同一份代码，用 C++17 / C++20 / C++23 都能编译，并且输出一样。
// 三种标准都要编译运行一遍：
//   clang++ -std=c++17 -Wall -Wextra version_exercise.cpp -o e17.exe
//   clang++ -std=c++20 -Wall -Wextra version_exercise.cpp -o e20.exe
//   clang++ -std=c++23 -Wall -Wextra version_exercise.cpp -o e23.exe
#include <cstdio>
#include <version>
// 改动：原来用 __has_include 决定 include 什么、用特性宏决定用什么功能，这个思路是对的
//       （比如 g++ 13 根本没有 <print> 文件，直接 include 会报错，__has_include 能防住）。
//       这里改成只用特性宏，是因为宏有定义就说明头文件一定存在，一个判断就同时管了两件事，
//       而且不会 include 用不上的头文件。两种写法都可以。
#if defined(__cpp_lib_print)
#include <print>
#elif defined(__cpp_lib_format)
#include <format>
#include <string>
#endif
// 改动：<utility> 从 C++98 起就有，不用判断；只有用到 cmp_less 时才需要它
#ifdef __cpp_lib_integer_comparison_functions
#include <utility>
#endif

// ============================================================
// 练习 1：根据编译器支持的特性，选择最好的打印方式
//   输出（三种标准都一样）：Hello, Tom! You are 20.
//   并且在前面打印用的是哪种方式，例如：
//     C++23：[std::println] Hello, Tom! You are 20.
//     C++20：[std::format]  Hello, Tom! You are 20.
//     C++17：[printf]       Hello, Tom! You are 20.
//   规则：
//     支持 std::print  -> 用 std::println
//     否则支持 format  -> 用 std::format 拼好字符串，再用 std::fputs 或 std::puts 输出
//     都不支持         -> 用 std::printf
// ============================================================
void greet(const char* name, int age)
{
#ifdef __cpp_lib_print
    // 改动：原来这里打印了两行：一行写死了 "Tom" 和 20，没用参数；另一行又用 format 打印了一遍。
    //       只需要一行 println，并加上 [std::println] 标记
    std::println("[std::println] Hello, {}! You are {}.", name, age);
#elif defined(__cpp_lib_format)
    std::string str{std::format("[std::format]  Hello, {}! You are {}.", name, age)}; // 改动：加上 [std::format] 标记
    std::puts(str.c_str()); // 改动：puts 只接受 const char*，string 要用 .c_str() 转换（原来 C++20 下编译失败）
#else
    std::printf("[printf]       Hello, %s! You are %d.\n", name, age);
#endif
}

// ============================================================
// 练习 2：安全比较，有标准函数就用标准的，没有就用自己写的
//   支持 std::cmp_less（C++20）  -> 直接调用
//   不支持                        -> 用 1.4 节学到的思路自己实现：负数一定更小
// ============================================================
constexpr bool safe_less(int a, unsigned b)
{
#ifdef __cpp_lib_integer_comparison_functions
    return std::cmp_less(a, b);
#else
    // 改动 1：if constexpr 改成普通 if。a 是函数参数，值要到调用时才知道，
    //         if constexpr 的条件必须在编译时就能确定（原来 C++17 下编译失败）
    if (a < 0)
    {
        return true;
    }
    else
    {
        // 改动 2：a < b 是 int 和 unsigned 混合比较，有 -Wsign-compare 警告。
        //         这里 a 已经确定 >= 0，转成 unsigned 是安全的（1.4 节思考题 (1) 的结论）
        return static_cast<unsigned>(a) < b;
    }
#endif
}

static_assert(safe_less(-1, 0u));
static_assert(safe_less(3, 7u));
static_assert(!safe_less(7, 3u));
static_assert(!safe_less(5, 5u));

// ============================================================
// 练习 3（思考题，把答案写在注释里）
//   (1) 第一节课你用 clang++ format.cpp（没加 -std）编译，报了一堆错。
//       用 version.cpp 的输出解释：为什么 <format> 头文件明明存在，std::format 却找不到？
//       这说明检查 __cpp_lib_format 和检查 __has_include(<format>) 哪个更可靠？
//   (2) 同样是 C++23，g++ 的 __cpp_lib_format 是 202304，clang++ 是 202110。这说明什么？
//       什么时候应该写 #if __cpp_lib_format >= 202304L，而不是只写 #ifdef __cpp_lib_format？
//   (3) 不加 -std 时，clang++ 默认是 C++17，g++ 16 默认是 C++20。这对你写编译命令有什么启示？
//
// 你的答案：
//   (1)c++版本不支持format特性， __cpp_lib_format更可靠
//       改动（补充）：正确。更具体地说：不加 -std 时 clang++ 用 C++17，<format> 文件存在所以 include 不报错，
//       但标准库在 C++17 模式下把里面的内容关掉了，所以 std::format 找不到。
//       __has_include 只能说明“文件在”，__cpp_lib_format 才说明“功能能用”。
//   (2)实现的是不同时间通过的提案，依赖某次提案的标准
//       改动（补充）：正确。format 通过后又被修订过几次，每次修订宏的值就变大，两个编译器实现到的修订版不同。
//       当你用到的功能是某次修订才加入的，就要用 >= 判断那次修订的值；只用最初就有的功能，#ifdef 就够了。
//   (3)不知道
//       改动（补充）：同一条不带 -std 的命令，换个编译器（甚至同一编译器换个版本）用的标准就不同，
//       结果可能一个能编译、一个不能。所以编译命令里永远显式写上 -std=c++20 / -std=c++23，
//       让结果不依赖编译器的默认值。
// ============================================================

int main()
{
    greet("Tom", 20);
}
