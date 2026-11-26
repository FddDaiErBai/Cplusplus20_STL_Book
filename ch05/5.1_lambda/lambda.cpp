// 第 5 章 5.1 Lambda 表达式：lambda 是什么、编译器把它变成了什么、捕获和闭包
// 编译：clang++ -std=c++23 -Wall -Wextra -Wpedantic lambda.cpp -o lambda.exe
//       g++     -std=c++23 -Wall -Wextra -Wpedantic lambda.cpp -o lambda.exe -lstdc++exp
//
// 文件结构：[1] 到 [7] 从上到下排列，每个编号只出现一次。
//   每一节里依次是：说明、这一节用到的类和函数、一个 sectionN() 函数（运行这一节）。main 依次调用 section1() 到 section7()。
//
// ============================================================
// lambda 表达式 = 写在表达式里的、没有名字的函数。C++11 开始有。
//
//   [捕获列表](参数列表) { 函数体 }
//   例：[](int x) { return x * 2; }
//
//   以前想给算法传一个“小函数”（比如 sort 的比较方式），只能在别处另外定义一个有名字的函数或者写一个类。
//   lambda 让你在用的地方直接写。
//
// 这一节只讲最基本的东西。下一节 5.3 讲完整语法（mutable、[=]、[&]、初始化捕获、模板 lambda、-> 返回类型……）。
//   [1] 写法和用法：存到变量里、当参数传、写完立刻调用
//   [2] 编译器把 lambda 变成了什么：一个类 + 这个类的一个对象（最重要，后面的规则都能从这里推出来）
//   [3] 为什么要“捕获”：lambda 的函数体看不见外面的局部变量
//   [4] 按值捕获 [a]：拷贝一份存进对象，创建的那一刻就定了
//   [5] 按引用捕获 [&a]：存一个引用，看到的、改的都是外面那个变量
//   [6] 闭包：lambda 对象可以比创建它的函数活得久 —— 这时必须按值捕获
//   [7] 没有捕获的 lambda 能转成普通函数指针，有捕获的不能
//
// 书上这一节很短，基本没错，只是有几处没说清楚，在对应的地方补充：
//   · 书上写 f(la)，没说 f 怎么声明：lambda 的类型写不出名字，f 的参数只能是 auto / 模板 / std::function（[1] [2]）
//   · 书上说“闭包是一个允许在自己的词法范围之外使用符号的函数”：更准确的说法见 [6]
//   · 书上只给了 GCC 的报错，这里两个编译器的报错都列出来（[3]）
// ============================================================
#include <algorithm>
#include <cstdlib>
#include <print>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

// ============================================================
// [1] 写法和用法
//   auto la = [] { return "Hello"; };
//     []        捕获列表，这里是空的（[3] 讲）
//     ()        参数列表。没有参数时可以省略，这里就省了
//     { ... }   函数体。返回类型由 return 语句推导，这里是 const char*
//   la 是一个变量，la() 调用它，用起来和函数一样
//
//   ① 存进变量，然后调用：la()
//   ② 传给函数：函数的参数要写 auto（C++20 的“缩写函数模板”，等于 template <typename F> void call_twice(F f)）
//      为什么不能写一个具体类型？因为每个 lambda 的类型都是编译器临时起名的，你写不出来（[2]）
//   ③ 传给另一个 lambda：lb 的参数 a 是 auto，lb(la) 就是在 lb 里面调用 la()
//   ④ 不起名字，直接写在要用的地方：lb([] { return "World"; })
//   ⑤ 写完立刻调用：在 lambda 后面直接加 (实参)
// ============================================================
void call_twice(auto f)
{
    f();
    f();
}

void section1()
{
    std::println("[1] 写法和用法");

    auto la = [] { return "Hello"; };
    std::println("  ① la()                 → {}", la());

    std::println("  ② call_twice(...)：");
    call_twice([] { std::println("       被调用了一次"); });

    auto lb = [](auto a) { return a(); };
    std::println("  ③ lb(la)               → {}", lb(la));
    std::println("  ④ lb(没有名字的 lambda) → {}", lb([] { return "World"; }));
    std::println("  ⑤ 写完立刻调用         → {}", [](int l, int r) { return l + r; }(47, 73));
}

// ============================================================
// [2] 编译器把 lambda 变成了什么
//   写 [](int x) { return x * 2; } 时，编译器做了两件事：
//     ① 生成一个没有名字的类（叫“闭包类型”，closure type），类里有一个 operator()：
//
//          class __编译器起的名字
//          {
//          public:
//              auto operator()(int x) const     ← 默认是 const 成员函数，[4] [5] 会用到这一点
//              {
//                  return x * 2;
//              }
//          };
//
//     ② 在写 lambda 的地方，创建这个类的一个对象（叫“闭包对象”）
//   所以 la(21) 其实就是 la.operator()(21)。
//   这种“重载了 operator() 的类的对象”叫函数对象（第 3 章的 std::greater<> 就是一个），lambda 只是让编译器替你写这个类。
//
//   由此可以推出：
//     · 每个 lambda 表达式都是一个新的类型，哪怕两段代码一字不差
//       → la = lb 编译不过：两个不同的类，没有互相赋值的 operator=
//           clang：error: no viable overloaded '='
//           g++  ：error: no match for 'operator=' (operand types are 'section2()::<lambda(int)>' and 'section2()::<lambda(int)>')
//         注意 g++ 报错里两个类型打印出来的文字一模一样，但它们确实是两个类型（下面 is_same 是 false）
//     · 所以存 lambda 只能用 auto（或者模板参数、decltype，或者 5.5 节的 std::function）
//     · 没有捕获时类里没有数据成员，是个空类。C++ 规定任何对象至少占 1 字节，所以 sizeof 是 1
// ============================================================
template <typename T>
constexpr std::string_view type_name()
{
    return __PRETTY_FUNCTION__;   // clang 和 g++ 都会把 T 是什么写进这个字符串里
}

struct Doubler   // 和下面的 lambda 等价的手写类
{
    int operator()(int x) const
    {
        return x * 2;
    }
};

void section2()
{
    std::println("\n[2] 编译器把 lambda 变成了什么");

    auto la = [](int x) { return x * 2; };
    auto lb = [](int x) { return x * 2; };
    Doubler d{};

    std::println("  la(21) = {}，Doubler{{}}(21) = {}", la(21), d(21));
    std::println("  la.operator()(21) = {}   ← 真的有这个成员函数", la.operator()(21));
    std::println("  la 的类型：{}", type_name<decltype(la)>());
    std::println("  lb 的类型：{}", type_name<decltype(lb)>());
    std::println("  la 和 lb 是同一个类型吗？{}", std::is_same_v<decltype(la), decltype(lb)>);
    std::println("  sizeof(la) = {}，sizeof(Doubler) = {}", sizeof(la), sizeof(d));
    // la = lb;   // 打开这行看报错（上面注释里抄了两个编译器的报错）
    // clang 打印 (lambda at lambda.cpp:行:列)，用位置区分；g++ 打印 section2()::<lambda(int)>，两个看起来一样
}

// ============================================================
// [3] 为什么要“捕获”
//   lambda 的函数体，就是 [2] 里那个类的 operator() 的函数体。
//   一个类的成员函数看不见“别的函数里的局部变量”—— 所以 lambda 体里直接用外面的局部变量会编译错误：
//
//       const char* greeting{"Hello"};
//       auto la = [] { return greeting; };
//
//     clang：error: variable 'greeting' cannot be implicitly captured in a lambda with no capture-default specified
//            （后面的 note 列出了四种改法：按值捕获、按引用捕获、默认按值 [=]、默认按引用 [&]）
//     g++  ：error: 'greeting' is not captured
//            note: the lambda has no capture-default
//
//   把变量名写进 [] 里就行了：[greeting] { return greeting; }
//   这叫“捕获”：把外面的变量带进 lambda 对象里，存成它的数据成员（[4] [5] 具体看怎么存）
//
//   不用捕获也能直接用的东西 —— 它们不是“某个函数的局部变量”，任何函数本来就能直接用：
//     · 全局变量、static 变量（函数里的 static 局部变量也算：它不在栈上，整个程序只有一份）
//     · constexpr 常量（只读它的值时，编译器直接把值替换进去，用不着变量本身）
//     · 类型名、函数名、枚举值
// ============================================================
int g_count = 100;

void section3()
{
    std::println("\n[3] 为什么要“捕获”");

    const char* greeting{"Hello"};
    auto la = [greeting] { return greeting; };
    std::println("  [greeting] {{ return greeting; }} → {}", la());

    static int s_count = 20;
    constexpr int k_limit = 3;
    auto lb = [] { return g_count + s_count + k_limit; };   // [] 是空的
    std::println("  全局 100 + static 20 + constexpr 3，[] 里什么都没写也能用 → {}", lb());
}

// ============================================================
// [4] 按值捕获 [a]
//   [a] 等于类里有一个数据成员 int a，创建 lambda 对象的那一刻，把外面的 a 拷贝进去：
//
//       int a = 1;
//       auto f = [a] { return a * 10; };
//     等价于
//       struct F
//       {
//           int a;
//           int operator()() const { return a * 10; }
//       };
//       F f{a};            ← 拷贝发生在这一行（创建的时候），不是调用 f() 的时候
//
//   所以：
//     · 之后外面的 a 再怎么改，f 里面那份都不变 —— 相当于拍了张快照
//     · operator() 是 const 成员函数，const 成员函数里不能改数据成员，所以 [a] { ++a; } 编译错误：
//         clang：error: cannot assign to a variable captured by copy in a non-mutable lambda
//         g++  ：error: increment of read-only variable 'a'
//     · 在参数列表后面加 mutable，operator() 就不是 const 了，能编译 —— 但改的只是 lambda 里面那份拷贝，
//       外面的 a 不变！（下面演示。mutable 真正的用途 5.3 节讲）
//     · 每捕获一个变量，类里就多一个成员，sizeof 跟着变大
//       （string 的大小两个库不一样：libc++ 是 24，libstdc++ 是 32）
// ============================================================
struct F   // 和 [a] { return a * 10; } 等价的手写类
{
    int a;

    int operator()() const
    {
        return a * 10;
    }
};

void section4()
{
    std::println("\n[4] 按值捕获 [a]");

    int a = 1;
    auto f = [a] { return a * 10; };
    F hand{a};
    a = 5;
    std::println("  创建之后把外面的 a 改成 5：f() = {}，手写的 F = {}（都还是 1 × 10）", f(), hand());

    int outside = 0;
    auto add_one = [outside]() mutable { ++outside; return outside; };
    int r1 = add_one();
    int r2 = add_one();
    std::println("  mutable：两次调用返回 {} {}（改的是 lambda 里那份），外面的 outside 还是 {}", r1, r2, outside);

    int b = 2;
    std::string s = "hello";
    auto g1 = [a, b] { return a + b; };
    auto g2 = [s] { return s.size(); };
    std::println("  g1() = {}，g2() = {}", g1(), g2());
    std::println("  sizeof：[] → {}，[a] → {}，[a, b] → {}，[s] → {}（string 本身 {}）",
                 sizeof([] {}), sizeof(f), sizeof(g1), sizeof(g2), sizeof(s));
}

// ============================================================
// [5] 按引用捕获 [&a]
//   [&a] 等于类里的数据成员是一个引用 int& a，绑定到外面那个 a：
//
//       struct G
//       {
//           int& a;
//           void operator()() const { ++a; }    ← const 函数里也能 ++a！
//       };
//
//   所以：
//     · 读到的永远是外面 a 现在的值，不是快照
//     · 可以修改外面的 a。operator() 还是 const 的，为什么能改？
//         const 成员函数的意思是“不能改数据成员”。数据成员是引用本身，引用本来就不能改绑到别的变量，
//         const 管不到“引用指向的那个 int”。
//         这和 4.8 节的 char* const 是一个道理：指针本身是 const（不能指向别处），指向的 char 可以改。
//     · 引用成员在实现里就是存了一个地址，64 位上 sizeof 是 8
//
//   用哪一种：
//     · 要修改外面的变量，或者变量很大不想拷贝         → 引用 [&a]
//     · 要一份不会再变的快照，或者 lambda 会活得比变量久（[6]） → 值 [a]
// ============================================================
void section5()
{
    std::println("\n[5] 按引用捕获 [&a]");

    int count = 0;
    auto inc = [&count] { ++count; };
    for (int i = 0; i < 5; ++i)
    {
        inc();
    }
    std::println("  inc() 调用 5 次之后 count = {}", count);

    std::vector<int> v{5, 20, 50, 80, 200};
    int threshold = 10;
    auto by_value = [threshold](int x) { return x > threshold; };
    auto by_ref = [&threshold](int x) { return x > threshold; };
    threshold = 60;
    std::println("  创建之后把 threshold 从 10 改成 60，数 v = {{5 20 50 80 200}} 里大于它的：");
    std::println("    [threshold]  → {} 个（还按 10 算）", std::ranges::count_if(v, by_value));
    std::println("    [&threshold] → {} 个（按 60 算）", std::ranges::count_if(v, by_ref));
    std::println("  sizeof：[threshold] → {}，[&threshold] → {}", sizeof(by_value), sizeof(by_ref));
}

// ============================================================
// [6] 闭包（closure）
//   “闭包”：一个函数，带着它创建时周围环境里的变量（就是捕获的东西）一起走。
//   在 C++ 里分清两个词：
//     · lambda 表达式：源代码里写的那一段 [..](..) {..}
//     · 闭包对象：程序运行到那里时，按 [2] 创建出来的那个对象，捕获的东西就存在它里面
//
//   闭包对象可以被返回、存进容器，在创建它的函数已经结束以后再调用。这时：
//     · 按值捕获：没问题，数据就在对象自己里面
//     · 按引用捕获：引用指向的局部变量已经销毁了 → 悬空引用，读它是未定义行为
//
//       auto make_adder_bad(int n)
//       {
//           return [&n](int x) { return x + n; };     ← n 是参数，函数返回时就销毁了
//       }
//
//       clang 编译时就警告：warning: address of stack memory associated with parameter 'n' returned [-Wreturn-stack-address]
//       g++ -O0 不警告。实测 make_adder_bad(5)(1) 打印出 754972945（垃圾值）；g++ -O2 才警告 'n' is used uninitialized
//       （这个文件里不运行它：未定义行为，每次结果都可能不一样，也可能直接崩溃）
//
//   规则：lambda 要离开当前作用域（被返回、存起来以后再用、交给别的线程）→ 按值捕获
//
//   顺便一提：同一个 lambda 表达式不管运行多少次，都是同一个类型（[2] 说的“新类型”是按源代码里的位置算的）。
//   所以 make_adder(5) 和 make_adder(100) 返回的是同一个类型的两个对象，只是里面存的 n 不同，可以放进同一个 vector。
// ============================================================
auto make_adder(int n)
{
    return [n](int x) { return x + n; };
}

void section6()
{
    std::println("\n[6] 闭包");

    auto add5 = make_adder(5);      // make_adder 已经返回了，参数 n 早就没了，但 add5 里面存着一份 5
    auto add100 = make_adder(100);
    std::println("  add5(1) = {}，add100(1) = {}", add5(1), add100(1));

    std::println("  add5 和 add100 是同一个类型吗？{}", std::is_same_v<decltype(add5), decltype(add100)>);
    std::vector<decltype(make_adder(0))> adders{make_adder(1), make_adder(10), make_adder(1000)};
    std::print("  放进同一个 vector，对 7 依次调用：");
    for (const auto& add : adders)
    {
        std::print(" {}", add(7));
    }
    std::println("");
}

// ============================================================
// [7] 没有捕获的 lambda 可以转换成函数指针
//   没有捕获 → 类里没有数据 → operator() 用不到对象里的任何东西。
//   这时编译器额外提供一个“转换成普通函数指针”的功能，lambda 可以用在只接受函数指针的地方。
//
//   有捕获就不行：函数指针只是一段代码的地址，装不下捕获的那些数据。
//       int a = 1;
//       int (*fp)(int) = [a](int x) { return x + a; };
//     clang：error: no viable conversion from '(lambda at ...)' to 'int (*)(int)'
//     g++  ：error: cannot convert 'section7()::<lambda(int)>' to 'int (*)(int)' in initialization
//
//   用处：传给 C 语言风格的函数，比如 C 标准库的 std::qsort（排序，比较函数必须是函数指针）、Windows API 的回调。
//   小技巧：在 lambda 前面加一元 +，+[](int x) {...} 的类型就直接是函数指针
//     （+ 只能用在数字和指针上，编译器只好先把 lambda 转成函数指针）
// ============================================================
void section7()
{
    std::println("\n[7] 没有捕获的 lambda → 函数指针");

    int (*fp)(int) = [](int x) { return x * 3; };
    std::println("  fp(5) = {}", fp(5));

    auto p = +[](int x) { return x * 3; };
    std::println("  +[](int x) {{...}} 的类型：{}", type_name<decltype(p)>());

    int arr[]{40, 10, 30, 20};
    std::qsort(arr, 4, sizeof(int), [](const void* a, const void* b)
    {
        int x = *static_cast<const int*>(a);
        int y = *static_cast<const int*>(b);
        return (x > y) - (x < y);   // qsort 要的约定：小于返回负数，相等返回 0，大于返回正数
    });
    std::println("  std::qsort 用 lambda 当比较函数：{} {} {} {}", arr[0], arr[1], arr[2], arr[3]);
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
}
