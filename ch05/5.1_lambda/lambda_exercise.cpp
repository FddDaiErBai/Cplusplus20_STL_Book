// 第 5 章 5.1 Lambda 表达式 —— 练习
// 编译：clang++ -std=c++23 -Wall -Wextra -Wpedantic lambda_exercise.cpp -o lambda_exercise.exe
//       g++     -std=c++23 -Wall -Wextra -Wpedantic lambda_exercise.cpp -o lambda_exercise.exe -lstdc++exp
// 运行时检查：补完 TODO，编译运行，所有行都显示“通过”就说明做对了。
// 先看示例 lambda.cpp 的 [2]（lambda 就是一个类 + 一个对象）、[4]（按值捕获）、[5]（按引用捕获）、[6]（闭包）。
#include <algorithm>
#include <concepts>
#include <cstddef>
#include <print>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

void check(bool ok, std::string_view name)
{
    std::println("{} {}", ok ? "通过" : "失败", name);
}

// ============================================================
// 练习 1：预测结果（不要运行，先自己想，把数字填进 ans_ 变量里）
//   每一段代码就写在 test1() 里，下面已经算出了真正的结果，check 会把它和你填的比较。
//   先填，再运行看对不对。填错了的，回去对照示例 [4] [5] 想清楚为什么。
// ============================================================
int ans_p1 = 1;      // TODO：P1 里 f() 返回几
int ans_p2 = 2;      // TODO：P2 里 f() 返回几
int ans_p3_call = 3; // TODO：P3 里第二次调用 f() 返回几
int ans_p3_a = 1;    // TODO：P3 最后外面的 a 是几
bool ans_p4 = true;  // TODO：P4 里 r 的类型是不是 int（true / false）
int ans_p5 = 21;     // TODO：P5 里 g() 返回几

void test1()
{
    {
        // P1
        int a = 1;
        auto f = [a] { return a; };
        a = 2;
        check(f() == ans_p1, "练习 1：P1");
    }
    {
        // P2
        int a = 1;
        auto f = [&a] { return a; };
        a = 2;
        check(f() == ans_p2, "练习 1：P2");
    }
    {
        // P3
        int a = 1;
        auto f = [a]() mutable { return ++a; };
        f();
        int second = f();
        check(second == ans_p3_call, "练习 1：P3 第二次调用的返回值");
        check(a == ans_p3_a, "练习 1：P3 外面的 a");
    }
    {
        // P4：注意最后的 (5)
        auto r = [](int x) { return x * 2; }(5);
        check(std::is_same_v<decltype(r), int> == ans_p4, "练习 1：P4 r 的类型");
    }
    {
        // P5
        int a = 1;
        int b = 10;
        auto g = [a, &b] { return a + b; };
        a = 2;
        b = 20;
        check(g() == ans_p5, "练习 1：P5");
    }
}

// ============================================================
// 练习 2：自己写 lambda
//   ① count_above：v 里有几个元素大于 limit。用 std::ranges::count_if(v, 一个 lambda)，一行写完。
//        lambda 要用到 limit，所以要捕获它（按值就够了：只读，而且很小）
//   ② add_all：把 v 里每个元素加到 total 上。用 std::ranges::for_each(v, 一个 lambda)。
//        lambda 要修改外面的 total，该用哪种捕获？
//   ③ twice：一个 lambda，参数是 (auto f, int x)，返回 f(f(x))。现在的写法直接返回 x，改掉。
//   ④ make_multiplier：返回一个 lambda，它接受 int x，返回 x * k。
//        make_multiplier 返回以后 k 就销毁了（示例 [6]），所以要怎么捕获 k？
//   去掉 TODO 那几行里的 (void)，写上真正的代码。
// ============================================================
std::ptrdiff_t count_above(const std::vector<int>& v, int limit)
{
    return std::ranges::count_if(v, [limit](int x)
    {
        return x > limit;
    });
}

void add_all(const std::vector<int>& v, int& total)
{
    std::ranges::for_each(v, [&total](int x)
    {
        total += x;
    });
}

auto twice = [](auto f, int x)
{
    return f(f(x));
};

auto make_multiplier(int k)
{
    return [k](int x)
    {
        return x * k;
    };
}

void test2()
{
    std::vector<int> v{3, 8, 1, 9, 4, 7};
    check(count_above(v, 5) == 3, "练习 2 ①：{3 8 1 9 4 7} 里大于 5 的有 3 个");
    check(count_above(v, 100) == 0 && count_above({}, 0) == 0, "练习 2 ①：没有的时候是 0");

    int total = 0;
    add_all(v, total);
    check(total == 32, "练习 2 ②：3+8+1+9+4+7 = 32");
    add_all({10, 20}, total);
    check(total == 62, "练习 2 ②：再加 10 和 20，接着累加 = 62");

    auto add3 = [](int x) { return x + 3; };
    check(twice(add3, 1) == 7, "练习 2 ③：twice(加 3, 1) = 7");
    check(twice([](int x) { return x * x; }, 3) == 81, "练习 2 ③：twice(平方, 3) = 81");

    auto times3 = make_multiplier(3);
    auto times10 = make_multiplier(10);
    check(times3(7) == 21 && times10(7) == 70, "练习 2 ④：make_multiplier(3)(7) = 21，make_multiplier(10)(7) = 70");
    check(sizeof(times3) == sizeof(int), "练习 2 ④：lambda 里存的是 k 的一份拷贝（大小 = 一个 int）");
}

// ============================================================
// 练习 3：手写 lambda 背后的那个类（示例 [2] [4] [5]）
//   下面这个 lambda：
//
//       int base = 100;
//       std::vector<int> log;
//       auto rec = [base, &log](int x)
//       {
//           log.push_back(x);
//           return base + x;
//       };
//
//   编译器会为它生成一个类。在 Recorder 里写出这个类：
//     · 两个数据成员，名字就叫 base 和 log，先 base 后 log。按值捕获和按引用捕获，成员的类型分别是什么？
//     · 一个 operator()：参数 int x，返回 int，函数体和 lambda 的一样。
//       记住：编译器生成的 operator() 是 const 成员函数。你的也要是 const 的（检查里用 const Recorder 去调用）。
//   写完后 Recorder r{100, log}; 就能和 rec 一样用。
//   （现在 Recorder 是空的，检查会显示“失败”，但能编译。）
// ============================================================
struct Recorder
{
    int base;
    std::vector<int>& log;

    int operator()(int x) const // 改动（说明）：题目要的是 int，写 auto 也对：base + x 推导出来就是 int，和 lambda 的规则一样
    {
        log.push_back(x);
        return base + x;
    }
};

template <typename R>
concept RecorderLike = requires(std::vector<int>& log, const R r, int x) {
    R{100, log};
    { r(x) } -> std::same_as<int>;
};

template <typename R>
bool recorder_ok()
{
    if constexpr (RecorderLike<R>)
    {
        int base = 100;
        std::vector<int> log1;
        std::vector<int> log2;
        auto rec = [base, &log1](int x)
        {
            log1.push_back(x);
            return base + x;
        };
        const R r{base, log2};
        bool same = rec(1) == r(1) && rec(5) == r(5) && r(-100) == 0;
        rec(-100);
        return same && log1 == log2 && log2 == std::vector<int>{1, 5, -100};
    }
    else
    {
        return false;
    }
}

template <typename R>
bool recorder_size_ok()
{
    if constexpr (RecorderLike<R>)
    {
        int base = 0;
        std::vector<int> log;
        auto rec = [base, &log](int x)
        {
            log.push_back(x);
            return base + x;
        };
        return sizeof(R) == sizeof(rec);
    }
    else
    {
        return false;
    }
}

void test3()
{
    check(RecorderLike<Recorder>, "练习 3：Recorder{100, log} 能构造，const Recorder 能用 int 调用并返回 int");
    check(recorder_ok<Recorder>(), "练习 3：和 lambda 的返回值一样，而且往同一种 log 里记录（log 是引用：外面的 vector 变了）");
    check(recorder_size_ok<Recorder>(), "练习 3：sizeof 和 lambda 一样");

    // 改动：补一项检查。复制 Recorder 时，引用成员复制的是“绑定到谁”，不会复制那个 vector。
    //   所以两份 Recorder 往同一个 vector 里记录。lambda 也一样：auto rec2 = rec; 两个对象共用外面那一个 log
    std::vector<int> shared;
    Recorder r1{0, shared};
    Recorder r2 = r1;
    r1(1);
    r2(2);
    check(shared == std::vector<int>{1, 2}, "练习 3：复制出来的 Recorder 和原来的往同一个 vector 里记录");
}

// ============================================================
// 练习 4：找 bug（每个函数里只有一个 [] 写错了，一共只改两处，别的不要动）
//   ① count_char：数字符串里有几个 ch。运行结果总是 0。
//   ② make_checks：做两个判断“至少 1 分”“至少 3 分”的 lambda。结果两个都变成了“至少 3 分”。
//   想一想错在哪，对照示例 [4] [5]：什么时候要快照，什么时候要看到外面的变化。
// ============================================================
int count_char(std::string_view s, char ch)
{
    int count = 0;
    std::ranges::for_each(s, [&count, ch](char c) // 改动（说明）：去掉 mutable 是对的：按引用捕获本来就能改，不需要它
    {
        if (c == ch)
        {
            ++count;
        }
    });
    return count;
}

struct Checks
{
    int at_least_1;   // 有几个 >= 1
    int at_least_3;   // 有几个 >= 3
};

Checks make_checks(const std::vector<int>& scores)
{
    int level = 1;
    auto ge1 = [level](int x)
    {
        return x >= level;
    };
    level = 3;
    auto ge3 = [level](int x)
    {
        return x >= level;
    };
    return {static_cast<int>(std::ranges::count_if(scores, ge1)), static_cast<int>(std::ranges::count_if(scores, ge3))};
}

void test4()
{
    check(count_char("banana", 'a') == 3, "练习 4 ①：banana 里有 3 个 a");
    check(count_char("banana", 'x') == 0 && count_char("", 'a') == 0, "练习 4 ①：没有的时候是 0");

    std::vector<int> scores{0, 1, 2, 3, 4, 5};
    Checks c = make_checks(scores);
    check(c.at_least_1 == 5, "练习 4 ②：{0 1 2 3 4 5} 里 >= 1 的有 5 个");
    check(c.at_least_3 == 3, "练习 4 ②：{0 1 2 3 4 5} 里 >= 3 的有 3 个");
}

int main()
{
    test1();
    test2();
    test3();
    test4();
}

// ============================================================
// 思考题（写在“你的答案：”后面，每一小题都要写）
//
// 1. [x] { ++x; } 编译错误，[&x] { ++x; } 能编译。可是这两个 lambda 的 operator() 都是 const 成员函数。
//    (a) 用示例 [2] 的“类”的写法，分别写出这两个 lambda 对应的类（数据成员 + operator()），
//        然后说明：为什么第一个的 ++x 违反了 const，第二个的不违反？
// 你的答案：int x; int& x;第一个修改了类的数据成员，这是const成员函数不允许的。后者管不到int&指向的对象。
//    (b) [x]() mutable { ++x; } 能编译。调用它 3 次以后，外面的 x 是几？为什么？
// 你的答案：不变，按值捕获的是副本。
// 改动（补充）：外面的 x 不变；lambda 里那份变成 x + 3。闭包对象一直活着，所以每次调用都接着上一次的值往上加（练习 1 的 P3）。
//
// 2. 下面的函数（clang 编译时有警告，g++ -O0 没有）：
//
//        auto make_greeter(std::string name)
//        {
//            return [&name] { return "Hello, " + name; };
//        }
//        auto hi = make_greeter("Tom");
//        std::println("{}", hi());
//
//    (a) hi() 运行的时候，lambda 里的那个引用指向谁？那个东西这时还在吗？
// 你的答案："Tom",不在了。
// 改动（补充）：“不在了”对。但指向的不是 "Tom" 这个字面量，而是参数 name，一个 std::string 对象。
//   这个对象是调用时用 "Tom" 构造出来的，放在 make_greeter 的栈帧里，make_greeter 一返回就析构了。
//   字面量 "Tom" 本身存在程序的只读数据区，整个程序运行期间都在。所以要分清“字面量”和“用它构造出来的 string 对象”。
//    (b) 把 [&name] 改成 [name] 为什么就对了？说清楚 "Tom" 这时存在哪里。
// 你的答案：按值捕获的是副本，lambda对应类的数据成员，std::string name;
//    (c) 如果真的不想拷贝 name（假设它很长），还能怎么写？提示：示例 [6] 下面那段说了什么时候必须按值捕获，
//        这里能不能让 name 被“移动”进 lambda？（不会也没关系，写你的猜想。5.3 节会讲“初始化捕获” [n = ...] 的写法）
// 你的答案：使用全局变量或者static，constexpr。
// 改动（补充）：这三种都不行。
//   · 全局变量和 static：整个程序只有一份。make_greeter("Tom") 和 make_greeter("Ann") 会共用它，后一次调用把前一次的名字盖掉
//   · constexpr：名字是运行时才传进来的参数，不能在编译时确定
//   正确的写法是初始化捕获，直接把参数“移动”进 lambda：
//       return [name = std::move(name)] { return "Hello, " + name; };
//   意思是“lambda 里新建一个成员叫 name，用 std::move(name) 初始化它”。移动 string 只是把内部那块堆内存的指针交过去，不拷贝字符。
//   实测（两个编译器一样）：1000 个字符的 string 移动进 lambda 后，lambda 里 data() 的地址和移动前完全相同，原来的变量变成空串。
//   5.3 节细讲这种写法。
//
// 3. (a) std::qsort 的比较函数只能是函数指针。[](const void* a, const void* b) {...} 能传进去，
//        但如果比较时要用到一个局部变量 bool descending，写成 [descending](const void* a, const void* b) {...} 就传不进去了。为什么？
// 你的答案：函数指针只是一段代码的地址，存不了捕获的数据。
//    (b) auto a = [] { return 1; };  auto b = [] { return 1; };
//        a = b; 编译错误。但是 auto c = a; c = a; 能编译。两个情况的区别在哪？
//        （C++20 起，没有捕获的 lambda 可以默认构造、可以赋值。所以 c = a 能编译，问题不在“lambda 不能赋值”。）
// 你的答案：同一个lambda表达式是同一个类型。
// 改动（补充）：对。补上另一半：a 和 b 是源代码里两段不同的 lambda 表达式，所以是两个不同的类，
//   两个不相关的类之间没有 operator=（就像 struct A 不能赋值给 struct B）。
//   c 是用 auto 从 a 推导出来的，类型就是 decltype(a)，同一个类之间有拷贝赋值，所以 c = a 能编译。
// ============================================================
