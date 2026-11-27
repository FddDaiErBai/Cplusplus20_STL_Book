// 第 5 章 5.3 Lambda 的完整语法：捕获的各种写法、mutable、模板 lambda、返回类型、说明符
// 编译：clang++ -std=c++23 -Wall -Wextra -Wpedantic lambda_syntax.cpp -o lambda_syntax.exe
//       g++     -std=c++23 -Wall -Wextra -Wpedantic lambda_syntax.cpp -o lambda_syntax.exe -lstdc++exp
//
// 文件结构：[1] 到 [8] 从上到下排列，每个编号只出现一次。
//   每一节里依次是：说明、这一节用到的类和函数、一个 sectionN() 函数（运行这一节）。main 依次调用 section1() 到 section8()。
//
// ============================================================
// 5.1 节讲了：lambda = 编译器生成的一个类 + 这个类的一个对象；[a] 存一份拷贝，[&a] 存一个引用。
// 这一节把 lambda 能写的每一部分都过一遍。完整的样子：
//
//   [捕获列表] <模板参数>(参数列表) 说明符 -> 返回类型 { 函数体 }
//      必须       可选      可选       可选     可选        必须
//
//   说明符有：mutable、constexpr、consteval、static（C++23）、noexcept
//
//   最短的 lambda：[] {}  —— 什么都不捕获、什么都不做
//
//   [1] 哪些部分可以省略；书上关于 () 的说法在 C++23 里已经过时
//   [2] 默认捕获 [=] 和 [&]：只捕获用到的变量；和显式捕获混着写的规则
//   [3] mutable：每次调用接着上一次的状态；复制 lambda 也复制状态；算法拿到的是副本
//   [4] 初始化捕获 [名字 = 表达式]：自己起名字、自己算初值、把东西“移动”进 lambda
//   [5] 在类的成员函数里：成员变量不能直接捕获，要捕获 this 或 *this
//   [6] 参数：auto 参数（泛型 lambda）和 C++20 的模板 lambda []<typename T>
//   [7] 返回类型：自动推导的规则（会丢掉引用！）、-> 指定返回类型
//   [8] constexpr、noexcept、static
//
// 书上的错误（都是小错，在对应的地方会再提一次）：
//   · auto two = [] { return "two"; }        少了分号
//   · []<template T>(T v)                      应该是 []<typename T>(T v)，写 template 编译不过（[6]）
//   · [] (int a, int b) { return a + b };      return 语句少了分号，应该是 { return a + b; }
//   · “[=] 将捕获 lambda 作用域中的所有符号”    不对，只捕获函数体里真正用到的变量（[2] 用 sizeof 验证）
//   · “有说明符或 -> 时必须写 ()，GCC 允许省略是不正确的”  C++23 起已经允许省略了（[1]）
//   · “wuto 返回类型”是 auto，“弱 lambda 满足要求”是“若”
// ============================================================
#include <algorithm>
#include <memory>
#include <print>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

// ============================================================
// [1] 哪些部分可以省略
//   · 捕获列表 [] 和函数体 {} 不能省，其它都可以
//   · 没有参数时 () 可以省：[] { return 1; } 和 []() { return 1; } 一样
//   · 书上说：只要写了说明符（mutable 等）或 -> 返回类型，就必须写 ()，[] mutable {...} 是错的。
//     这是 C++20 的规定。C++23（提案 P1102）把它改了：没有参数时，() 什么时候都可以省。
//     用 -std=c++20 编译 [n = 0] mutable { ... } 两个编译器只给警告：
//       clang：warning: lambda without a parameter clause is a C++23 extension
//       g++  ：warning: parameter declaration before lambda declaration specifiers only optional with '-std=c++23'
//     这个文件用 -std=c++23，两种写法都行。为了旧代码也能编译，下面还是习惯性地写上 ()。
// ============================================================
void section1()
{
    std::println("[1] 哪些部分可以省略");

    auto shortest = [] {};
    shortest();

    auto one = []() { return "one"; };
    auto two = [] { return "two"; };
    auto three = [] -> const char* { return "three"; };   // C++23：有 -> 也能省 ()
    std::println("  {} {} {}", one(), two(), three());
}

// ============================================================
// [2] 默认捕获 [=] 和 [&]
//   [=]：函数体里用到的外部变量，全部按值捕获
//   [&]：函数体里用到的外部变量，全部按引用捕获
//   “用到的”是关键：没用到的变量不会被捕获。下面用 sizeof 验证（a、b、c 都是 int）：
//
//     写法                         捕获了什么          sizeof
//     [=] { return a; }            一个 int            4
//     [=] { return a + b + c; }    三个 int            12
//     [&] { return a; }            一个引用（8 字节）   8
//     [&] { return a + b + c; }    三个引用            24
//
//   默认捕获可以和显式捕获混着写，默认的那个必须写在最前面，后面写“例外”：
//     [=, &total]   其它按值，total 按引用
//     [&, limit]    其它按引用，limit 按值
//   例外必须和默认的方式不一样，否则 clang 报错（g++ 只警告）：
//     [=, a]   clang：error: '&' must precede a capture when the capture default is '='
//     [&, &a]  clang：error: '&' cannot precede a capture when the capture default is '&'
//   同一个变量也不能写两次：[a, &a]  clang：error: 'a' can appear only once in a capture list
//
//   建议：尽量写显式捕获。[&] 很方便，但 lambda 被返回或存起来的时候，看不出来它偷偷引用了哪些局部变量（5.1 节 [6] 的悬空问题）。
// ============================================================
void section2()
{
    std::println("\n[2] 默认捕获 [=] 和 [&]");

    int a = 1;
    int b = 2;
    int c = 3;
    auto v1 = [=] { return a; };
    auto v3 = [=] { return a + b + c; };
    auto r1 = [&] { return a; };
    auto r3 = [&] { return a + b + c; };
    std::println("  sizeof：[=] 用 1 个 → {}，[=] 用 3 个 → {}，[&] 用 1 个 → {}，[&] 用 3 个 → {}",
                 sizeof(v1), sizeof(v3), sizeof(r1), sizeof(r3));

    std::vector<int> v{1, 2, 3, 4};
    int total = 0;
    int factor = 10;
    std::ranges::for_each(v, [=, &total](int x) { total += x * factor; });   // factor 按值，total 按引用
    std::println("  [=, &total]：total = {}（1+2+3+4 再乘 10）", total);
}

// ============================================================
// [3] mutable
//   5.1 节说过：operator() 默认是 const 成员函数，不能改按值捕获的成员；加 mutable 就能改了。
//   这一节看它真正的用途：让 lambda 有“自己的状态”，每次调用接着上一次。
//
//       auto counter = [n = 0]() mutable { return ++n; };     ← [n = 0] 是 [4] 讲的初始化捕获，意思是“成员 n，初值 0”
//       counter()  → 1
//       counter()  → 2      n 存在 counter 这个对象里，调用之间一直保留着
//
//   要注意的两件事：
//   ① 复制一个 lambda，状态也跟着复制。复制出来以后，两个对象各走各的：
//       counter 已经是 2 了，auto copy = counter; 之后 copy() → 3，counter() → 3（不是 4）
//   ② 标准算法按值接收函数对象（拿到的是一个副本）。所以
//       std::ranges::for_each(v, tally);
//     算法改的是副本里的 n，调用完以后 tally 里的 n 还是 0。
//     想拿到结果：ranges::for_each 会把用过的那个副本还回来，在返回值的 .fun 里
//
//   顺便提醒：std::println("{} {}", f(), f()) 里两个 f() 谁先调用，C++ 没有规定（实测 clang 从左到右，g++ 从右到左）。
//   有状态的 lambda 要分开写成两句。
// ============================================================
void section3()
{
    std::println("\n[3] mutable");

    auto counter = [n = 0]() mutable { return ++n; };
    int first = counter();
    int second = counter();
    std::println("  counter() 两次：{} {}", first, second);

    auto copy = counter;
    int from_copy = copy();
    int from_counter = counter();
    std::println("  auto copy = counter; 然后 copy() → {}，counter() → {}（各走各的）", from_copy, from_counter);

    std::vector<int> v{5, 6, 7};
    auto tally = [n = 0](int) mutable { return ++n; };
    auto result = std::ranges::for_each(v, tally);
    int after = tally(0);
    int used = result.fun(0);
    std::println("  for_each 处理 3 个元素以后：tally(0) → {}（tally 自己一次都没被调用过），result.fun(0) → {}（算法用的那个副本）", after, used);
}

// ============================================================
// [4] 初始化捕获 [名字 = 表达式]（C++14）
//   在捕获列表里新建一个成员，名字自己起，初值是任意表达式。成员的类型按 auto 的规则推导。
//
//   ① [n = 0]          不需要外面有变量 n，lambda 自己的状态（[3] 用的就是这个）
//   ② [len = s.size()] 捕获的是计算的结果，不是 s 本身
//   ③ [s = std::move(s)]  把 s 移动进 lambda，不拷贝（5.1 节思考题 2(c) 的答案）
//        std::move(s) 只是把 s 标记成“可以被偷走”，string 的移动构造把内部那块内存的指针交给新成员。
//        之后外面的 s 变成空串（标准只说“有效但未指定”，实际两个库都是空串）
//   ④ [&r = x]         给外面的 x 起个别名 r，按引用捕获
//   ⑤ 只能移动、不能复制的东西（比如 std::unique_ptr）只能用 ③ 的写法捕获。
//      这时 lambda 也变成只能移动、不能复制的了：
//        编译器给 lambda 生成拷贝构造函数时，要拷贝每个成员。unique_ptr 的拷贝构造是 delete 的，所以 lambda 的也被 delete
//          clang：error: call to implicitly-deleted copy constructor of '(lambda at ...)'
//          g++  ：error: use of deleted function 'section4()::<lambda()>::<lambda>(const section4()::<lambda()>&)'
//        auto b = std::move(a); 可以
// ============================================================
void section4()
{
    std::println("\n[4] 初始化捕获");

    std::string s = "a long long text";
    auto size_only = [len = s.size()] { return len; };
    std::println("  ② [len = s.size()] → {}，sizeof = {}（只存了一个 size_t）", size_only(), sizeof(size_only));

    std::string big(1000, 'x');
    const char* before = big.data();
    auto owner = [text = std::move(big)] { return text.data(); };
    std::println("  ③ [text = std::move(big)]：lambda 里 data() 的地址和移动前一样？{}，外面的 big 现在是 \"{}\"",
                 owner() == before, big);

    int x = 5;
    auto bump = [&r = x] { ++r; };
    bump();
    bump();
    std::println("  ④ [&r = x] 调用两次以后 x = {}", x);

    auto ptr = std::make_unique<int>(42);
    auto holder = [p = std::move(ptr)] { return *p; };
    std::println("  ⑤ [p = std::move(ptr)]：holder() = {}，外面的 ptr 是空的？{}", holder(), ptr == nullptr);
    // auto copy = holder;              // 打开看报错（上面抄了）
    auto moved = std::move(holder);     // 移动可以
    std::println("     移动给 moved 以后 moved() = {}，lambda 能复制吗？{}", moved(), std::is_copy_constructible_v<decltype(moved)>);
}

// ============================================================
// [5] 在类的成员函数里写 lambda
//   成员变量 id_ 不是局部变量，不能直接写进捕获列表：
//     [id_] { return id_; }
//       clang：error: class member 'id_' cannot appear in capture list as it is not a variable
//       g++  ：error: capture of non-variable 'Widget::id_'
//   在成员函数里写 id_，其实是 this->id_ 的简写。所以要捕获的是 this：
//
//     写法        存了什么                      之后对象被修改            对象被销毁以后再调用
//     [this]      this 指针（8 字节）            看到新值                  悬空指针，未定义行为
//     [*this]     整个对象的一份拷贝（C++17）     还是创建时的值（快照）     没问题
//     [id = id_]  只拷贝需要的那个成员            快照                      没问题
//
//   [=] 在 C++20 以前会悄悄捕获 this（指针，不是对象！），名字叫“按值”其实是按引用，很容易出错，C++20 起已弃用：
//       clang：warning: implicit capture of 'this' with a capture default of '=' is deprecated
//       g++  ：warning: implicit capture of 'this' via '[=]' is deprecated in C++20
//   要写就写清楚：[=, this] 或 [=, *this]
// ============================================================
class Widget
{
    int id_ = 1;

public:
    void set_id(int id)
    {
        id_ = id;
    }

    auto live()
    {
        return [this] { return id_; };
    }

    auto snapshot()
    {
        return [*this] { return id_; };
    }

    auto just_id()
    {
        return [id = id_] { return id; };
    }
};

void section5()
{
    std::println("\n[5] 在类的成员函数里");

    Widget w;
    auto l1 = w.live();
    auto l2 = w.snapshot();
    auto l3 = w.just_id();
    w.set_id(9);
    std::println("  id 从 1 改成 9 以后：[this] → {}，[*this] → {}，[id = id_] → {}", l1(), l2(), l3());
}

// ============================================================
// [6] 参数：泛型 lambda 和模板 lambda
//   ① 参数写 auto（C++14）：叫泛型 lambda。编译器生成的 operator() 是一个模板：
//        [](auto l, auto r) { return l + r; }
//        等价于  template <typename T1, typename T2> auto operator()(T1 l, T2 r) const { return l + r; }
//      注意每个 auto 是独立的类型参数，所以 (1, 2.5) 也能调用：T1 = int，T2 = double
//
//   ② 模板 lambda（C++20）：在 [] 后面直接写模板参数，不用写 template 关键字：
//        []<typename T>(T a, T b) { ... }
//      书上写成了 []<template T>(T v)，编译不过：
//        clang：error: expected '<' after 'template'
//        g++  ：error: expected '<' before 'T'
//      什么时候需要它（auto 做不到的）：
//        · 要求两个参数是同一个类型：(T a, T b) 用 (1, 2.5) 调用 → 推导冲突，不能调用
//        · 要拿到类型的名字：[]<typename T>(const std::vector<T>& v) 能直接用 T，auto 要写 typename std::decay_t<decltype(v)>::value_type
// ============================================================
void section6()
{
    std::println("\n[6] 参数：泛型 lambda 和模板 lambda");

    auto add = [](auto l, auto r) { return l + r; };
    std::println("  auto 参数：add(1, 2.5) = {}，add(string, \"b\") = {}", add(1, 2.5), add(std::string("a"), "b"));

    auto same_add = []<typename T>(T a, T b) { return a + b; };
    std::println("  模板 lambda：same_add(1, 2) = {}", same_add(1, 2));
    std::println("  same_add(1, 2.5) 能调用吗？{}（T 推导出 int 和 double，冲突）",
                 std::is_invocable_v<decltype(same_add), int, double>);
    std::println("  add(1, 2.5) 能调用吗？{}", std::is_invocable_v<decltype(add), int, double>);

    auto first_or = []<typename T>(const std::vector<T>& v, T fallback) { return v.empty() ? fallback : v.front(); };
    std::println("  first_or({{}}, 7) = {}，first_or({{3, 4}}, 7) = {}", first_or(std::vector<int>{}, 7), first_or(std::vector<int>{3, 4}, 7));
}

// ============================================================
// [7] 返回类型
//   不写 -> 时，返回类型按 auto 的规则从 return 推导。两个要注意的地方：
//   ① 有多个 return，推导出的类型必须完全一样：
//        [](bool big) { if (big) return 1; return 2.5; }      int 和 double，编译错误
//          clang：error: return type 'double' must match previous return type 'int' when lambda expression has unspecified explicit return type
//          g++  ：error: inconsistent types 'int' and 'double' deduced for lambda return type
//      写 -> double 就行了，return 1 会转换成 1.0
//   ② auto 推导会去掉引用。v[0] 的类型是 int&，但 [&v] { return v[0]; } 返回的是 int（一份拷贝）。
//      想返回引用（让调用的人能改元素），要写 -> int&，或者 -> auto&，或者 -> decltype(auto)（原样保留表达式的类型）
//      当然，返回引用时要保证被引用的东西还活着（这里 v 在外面，没问题）
// ============================================================
void section7()
{
    std::println("\n[7] 返回类型");

    auto pick = [](bool big) -> double
    {
        if (big)
        {
            return 1;
        }
        return 2.5;
    };
    std::println("  -> double：pick(true) = {}，pick(false) = {}", pick(true), pick(false));

    std::vector<int> v{1, 2, 3};
    auto copy_of_first = [&v] { return v[0]; };
    auto ref_to_first = [&v] -> int& { return v[0]; };
    auto same_as_expr = [&v] -> decltype(auto) { return v[0]; };
    std::println("  不写 ->：返回 int 吗？{}", std::is_same_v<decltype(copy_of_first()), int>);
    std::println("  -> int&：返回 int& 吗？{}", std::is_same_v<decltype(ref_to_first()), int&>);
    std::println("  -> decltype(auto)：返回 int& 吗？{}", std::is_same_v<decltype(same_as_expr()), int&>);
    ref_to_first() = 100;
    std::println("  ref_to_first() = 100 以后 v[0] = {}", v[0]);
}

// ============================================================
// [8] constexpr、noexcept、static
//   ① constexpr：C++17 起，只要函数体满足 constexpr 函数的要求，lambda 自动就是 constexpr 的，可以在编译时调用。
//      写上 constexpr 的作用是“要求”它必须满足：不满足就编译错误，而不是悄悄变成普通函数。
//      consteval（C++20）：只能在编译时调用
//   ② noexcept：承诺不抛异常。可以用 noexcept(表达式) 在编译时查询
//   ③ static（C++23）：没有捕获的 lambda 可以写 static，operator() 变成静态成员函数，调用时不需要传 this，省一点点开销。
//      写了 static 就不能有捕获，也不能再写 mutable
// ============================================================
void section8()
{
    std::println("\n[8] constexpr、noexcept、static");

    auto square = [](int n) { return n * n; };            // 没写 constexpr
    static_assert(square(5) == 25);                        // 也能在编译时调用
    constexpr auto cube = [](int n) constexpr { return n * n * n; };
    static_assert(cube(3) == 27);
    std::println("  square(5) 和 cube(3) 都在编译时算过了（static_assert 通过）：{} {}", square(5), cube(3));

    auto safe = [](int n) noexcept { return n + 1; };
    std::println("  noexcept(safe(1)) = {}，noexcept(square(1)) = {}", noexcept(safe(1)), noexcept(square(1)));

    auto plus_one = [](int n) static { return n + 1; };
    std::println("  static lambda：plus_one(1) = {}", plus_one(1));
}

int main()
{
    section1();
    section2();
    section3();
    section4();
    section5();
    section6();
    section7();
    section8();
}
