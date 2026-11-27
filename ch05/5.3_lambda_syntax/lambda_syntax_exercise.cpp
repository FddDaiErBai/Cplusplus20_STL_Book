// 第 5 章 5.3 Lambda 的完整语法 —— 练习
// 编译：clang++ -std=c++23 -Wall -Wextra -Wpedantic lambda_syntax_exercise.cpp -o lambda_syntax_exercise.exe
//       g++     -std=c++23 -Wall -Wextra -Wpedantic lambda_syntax_exercise.cpp -o lambda_syntax_exercise.exe -lstdc++exp
// 运行时检查：补完 TODO，编译运行，所有行都显示“通过”就说明做对了。
// 先看示例 lambda_syntax.cpp 的 [2]（默认捕获）、[3]（mutable 和复制）、[4]（初始化捕获）、[5]（this）、[6]（模板 lambda）、[7]（返回类型）。
#include <algorithm>
#include <cstddef>
#include <memory>
#include <print>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

void check(bool ok, std::string_view name)
{
    std::println("{} {}", ok ? "通过" : "失败", name);
}

// ============================================================
// 练习 1：预测结果（先自己想，把答案填进 ans_ 变量，再运行看对不对）
// ============================================================
std::size_t ans_q1 = 8; // TODO：Q1 sizeof(f) 是几（int 是 4 字节）
int ans_q2 = 2;         // TODO：Q2 最后那次 c() 返回几
// 改动：Q2 的正确答案是 11。一步一步算（注意 c() 本身也被调用了一次）：
//   c 里的 n = 10。
//   c()        → 返回 10，c 的 n 变成 11（n++ 先返回旧值再加）
//   auto d = c → 复制，d 里的 n 也是 11
//   d()        → 返回 11，d 的 n 变成 12
//   d()        → 返回 12，d 的 n 变成 13
//   c()        → c 里的 n 还是 11，返回 11（c 只被调用过这一次返回值非零的）
// 你答的 2 大概是只算了 d 的两次调用、又忘了 n++ 是先返回旧值。复制 lambda 时状态跟着复制，之后两个对象各走各的（示例 [3] ①）。
int ans_q3 = 0;         // TODO：Q3 for_each 之后，sum 是几
int ans_q4_this = 50;   // TODO：Q4 a() 返回几
int ans_q4_star = 1;    // TODO：Q4 b() 返回几
int ans_q5 = 0;         // TODO：Q5 get() 的返回类型是不是 int&（是填 1，不是填 0）

struct Box
{
    int value = 1;

    auto by_this()
    {
        return [this] { return value; };
    }

    auto by_copy()
    {
        return [*this] { return value; };
    }
};

void test1()
{
    {
        // Q1：注意 [=] 只捕获“用到的”
        int a = 1;
        int b = 2;
        int c = 3;
        (void)c;
        auto f = [=] { return a + b; };
        check(sizeof(f) == ans_q1, "练习 1：Q1");
    }
    {
        // Q2：注意这里是 n++（先返回旧值，再加 1）
        auto c = [n = 10]() mutable { return n++; };
        c();
        auto d = c;
        d();
        d();
        check(c() == ans_q2, "练习 1：Q2");
    }
    {
        // Q3：lambda 里的 sum 是按值捕获的（mutable），外面的 sum 呢？
        std::vector<int> v{1, 2, 3};
        int sum = 0;
        std::ranges::for_each(v, [sum](int x) mutable { sum += x; });
        check(sum == ans_q3, "练习 1：Q3");
    }
    {
        // Q4
        Box box;
        auto a = box.by_this();
        auto b = box.by_copy();
        box.value = 50;
        check(a() == ans_q4_this, "练习 1：Q4 [this]");
        check(b() == ans_q4_star, "练习 1：Q4 [*this]");
    }
    {
        // Q5
        std::vector<int> v{1, 2, 3};
        auto get = [&v] { return v[0]; };
        check(ans_q5 == (std::is_same_v<decltype(get()), int&> ? 1 : 0), "练习 1：Q5");
    }
}

// ============================================================
// 练习 2：自己写
//   ① make_counter(start, step)：返回一个 lambda，第一次调用返回 start，之后每次加 step。
//        make_counter(5, 3)：5 8 11 14 ……
//      用初始化捕获 + mutable（示例 [3] [4]）。
//
//   ② make_reader(data)：data 是 std::unique_ptr<std::vector<int>>。
//      返回一个 lambda，参数 std::size_t i，返回 (*指针)[i]。
//      unique_ptr 不能复制，[data] 编译不过，要把它“移动”进 lambda（示例 [4] ⑤）。
//
//   ③ clamp_all：一个模板 lambda []<typename T>(std::vector<T>& v, T lo, T hi)，
//      把 v 里小于 lo 的改成 lo、大于 hi 的改成 hi，返回改了几个（int）。
//      lo、hi 必须和元素是同一个类型 T：clamp_all(vector<int>, 0, 1.5) 应该调用不了（示例 [6] ②）。
//      现在写的是 auto 参数，任何类型都能传进来，并且什么都不做。改成模板 lambda 并写出函数体。
//
//   ④ make_accessor(v)：返回一个 lambda，参数 std::size_t i，返回 v[i] 的引用，调用的人能通过它修改 v。
//      现在的写法返回的是拷贝（示例 [7] ②），改掉。
//
//   检查里用 if constexpr 判断能不能调用，所以这个文件现在就能编译。
// ============================================================
auto make_counter(int start, int step)
{
    return [start, step, n = -1]()mutable
    {
        ++n;
        return start + step * n;
    };
}

auto make_reader(std::unique_ptr<std::vector<int>> data)
{
    return [ptr=std::move(data)](std::size_t i)
    {
        return (*ptr)[i];
    };
}

// TODO ③
auto clamp_all = []<typename T>(std::vector<T>& v, T lo, T hi)
{
    int count{0};
    for (auto& i : v)
    {
        i = i > hi ? (++count, hi) : i < lo ? (++count, lo) : i;
    }
    return count;
};

auto make_accessor(std::vector<int>& v)
{
    // TODO ④
    return [&v](std::size_t i)-> int&
    {
        return v[i];
    };
}

template <typename F>
bool write_through(F f, std::vector<int>& v)
{
    if constexpr (std::is_lvalue_reference_v<decltype(f(std::size_t{1}))>)
    {
        f(1) = 50;
        return v == std::vector<int>{1, 50, 3};
    }
    else
    {
        return false;
    }
}

void test2()
{
    auto c = make_counter(5, 3);
    int c1 = c();
    int c2 = c();
    int c3 = c();
    check(c1 == 5 && c2 == 8 && c3 == 11, "练习 2 ①：make_counter(5, 3) → 5 8 11");
    auto d = make_counter(0, -1);
    int d1 = d();
    int d2 = d();
    check(d1 == 0 && d2 == -1, "练习 2 ①：make_counter(0, -1) → 0 -1");
    // 改动：补一项检查。两个 make_counter 各有一份状态，互不影响（示例 [3] ①）
    auto e1 = make_counter(10, 5);
    auto e2 = make_counter(100, 1);
    int e1a = e1();
    int e2a = e2();
    int e1b = e1();
    check(e1a == 10 && e2a == 100 && e1b == 15, "练习 2 ①：两个 counter 的状态互不影响");

    auto r = make_reader(std::make_unique<std::vector<int>>(std::vector<int>{7, 8, 9}));
    check(r(0) == 7 && r(2) == 9, "练习 2 ②：reader(0) = 7，reader(2) = 9");
    check(!std::is_copy_constructible_v<decltype(r)>, "练习 2 ②：lambda 里存着 unique_ptr，所以 lambda 不能复制");

    std::vector<int> v{-5, 3, 12, 7, 20};
    if constexpr (std::is_invocable_v<decltype(clamp_all), std::vector<int>&, int, int>)
    {
        int changed = clamp_all(v, 0, 10);
        check(changed == 3 && v == std::vector<int>{0, 3, 10, 7, 10}, "练习 2 ③：clamp 到 [0, 10] → 0 3 10 7 10，改了 3 个");
    }
    std::vector<double> dv{0.5, 2.5};
    if constexpr (std::is_invocable_v<decltype(clamp_all), std::vector<double>&, double, double>)
    {
        int changed = clamp_all(dv, 1.0, 2.0);
        check(changed == 2 && dv == std::vector<double>{1.0, 2.0}, "练习 2 ③：double 也能用");
    }
    check(!std::is_invocable_v<decltype(clamp_all), std::vector<int>&, int, double>, "练习 2 ③：clamp_all(vector<int>, 0, 1.5) 调用不了");
    // 改动：补两项检查。都在范围内时一个都不改；空 vector 也不能出错
    std::vector<int> in_range{2, 5, 8};
    std::vector<int> none;
    check(clamp_all(in_range, 0, 10) == 0 && in_range == std::vector<int>{2, 5, 8}, "练习 2 ③：都在范围内 → 改 0 个，元素不动");
    check(clamp_all(none, 0, 10) == 0 && none.empty(), "练习 2 ③：空 vector → 0");

    std::vector<int> w{1, 2, 3};
    check(write_through(make_accessor(w), w), "练习 2 ④：make_accessor(w)(1) = 50 能改到 w");
    // 改动：补一项检查，确认返回的确实是引用而不是拷贝
    std::vector<int> acc_v{1, 2, 3};
    auto acc = make_accessor(acc_v);
    check(std::is_same_v<decltype(acc(std::size_t{0})), int&>, "练习 2 ④：返回类型正好是 int&");
}

// ============================================================
// 练习 3：找 bug（两处，每处只改一个地方，别的不要动）
//   ① Thermostat::remember_target()：返回一个 lambda，记住“调用 remember_target 那一刻”的目标温度。
//      之后目标温度改了，这个 lambda 也应该还是返回原来的值。但现在它返回的是新值。
//   ② assign_ids：给物品编号，编号从 1 开始连续递增。分两批编号，第二批应该接着第一批往下编（4 5 6），
//      结果第二批又从 1 开始了。错在 assign_ids 的参数上。
// ============================================================
class Thermostat
{
    int target_ = 20;

public:
    void set_target(int t)
    {
        target_ = t;
    }

    auto remember_target()
    {
        return [*this]
        {
            return target_;
        };
    }
};

void assign_ids(std::vector<int>& ids, std::size_t count, auto& next_id)
{
    for (std::size_t i = 0; i < count; ++i)
    {
        ids.push_back(next_id());
    }
}

void test3()
{
    Thermostat t;
    auto saved = t.remember_target();
    t.set_target(25);
    check(saved() == 20, "练习 3 ①：改成 25 以后，记住的还是 20");

    std::vector<int> ids;
    auto next_id = [n = 0]() mutable { return ++n; };
    assign_ids(ids, 3, next_id);
    assign_ids(ids, 3, next_id);
    check(ids == std::vector<int>{1, 2, 3, 4, 5, 6}, "练习 3 ②：两批编号 1 2 3 4 5 6");
}

int main()
{
    test1();
    test2();
    test3();
}

// ============================================================
// 思考题（写在“你的答案：”后面，每一小题都要写）
//
// 1. 练习 2 ② 的 lambda 里存着一个 unique_ptr。
//    (a) 用 5.1 节“lambda 就是一个类”的说法解释：为什么这个 lambda 不能复制？
// 你的答案：如果要复制的话，编译器会给这个类生成拷贝构造函数，拷贝每一个成员，但是unique_ptr的拷贝构造是delete的。
//    (b) auto a = make_reader(...);  auto b = std::move(a);  之后再调用 a(0) 会怎样？为什么？
//        （提示：a 里的 unique_ptr 被移动走以后是什么值？）
// 你的答案： unique_ptr 被移动走后是一个有效但未定义，clang和gcc好像都是空值。
// 改动（补充）：对了一半。unique_ptr 移动以后不是“未指定”，标准保证它变成空（get() == nullptr，转成 bool 是 false）。
//   所以 a 里那个成员现在是空指针。a(0) 会执行 (*空指针)[0] —— 解引用空指针，未定义行为。
//   我实测过（clang + AddressSanitizer）：
//     ERROR: AddressSanitizer: access-violation on unknown address 0x000000000000
//     HINT: address points to the zero page.
//   编译时不报错，看代码也看不出来，只有运行时才崩。
//
// 2. 练习 3 ① 你改成了一种写法。
//    (a) 如果 Thermostat 对象先被销毁了，再调用 remember_target() 返回的 lambda，原来的 [this] 和你改的写法各会怎样？
// 你的答案：原来的写法会未定义行为，改后的不影响，因为是拷贝的对象副本。
//    (b) 在成员函数里写 [=] { return target_; }，看起来像“按值捕获”，实际上捕获的是什么？所以它和 [this] 还是 [*this] 一样？
// 你的答案：c++20以前会悄悄捕获this指针。
// 改动（补充）：对。补上后半问：捕获的是 this 指针，所以效果和 [this] 一样，不是 [*this]。
//   名字叫“按值”，存的却是指针 —— 对象一改它就看到新值，对象一销毁它就悬空。正因为容易误会，C++20 起这种隐式捕获被弃用：
//     clang：warning: implicit capture of 'this' with a capture default of '=' is deprecated
//     g++  ：warning: implicit capture of 'this' via '[=]' is deprecated in C++20
//
// 3. std::vector<int> v{1, 2, 3};
//    auto get = [&v] { return v[0]; };
//    (a) get() = 100; 能编译吗？为什么？（提示：get() 返回的是什么，是变量还是临时值）
// 你的答案：临时值。
// 改动（补充）：结论对，但漏了“能不能编译”：不能编译。
//   get() 返回的是 int（一份拷贝），是一个临时值，没有名字、没有固定的存放位置，不能当左值赋值：
//     clang：error: expression is not assignable
//     g++  ：error: lvalue required as left operand of assignment
//   想通过返回的东西改 v 里的元素，得像练习 2 ④ 那样写 -> int&，那时返回的是 v[0] 本身，可以赋值。
//    (b) int& r = get(); 能编译吗？const int& r = get(); 呢？说出区别的原因。
// 你的答案：后者可以，const修饰的左值引用可以接亡值。
// 改动（补充）：结论对（int& r = get() 编译不过，const int& r = get() 可以），但“亡值”这个词用错了：
//   get() 返回的 int 是“纯右值”（prvalue），不是“亡值”（xvalue）。
//     纯右值：算出来的临时值，比如字面量 47、函数按值返回的结果 get()
//     亡值  ：本来有名字的东西被标成“可以被偷走”，比如 std::move(x)、static_cast<int&&>(x)
//   int& 只能接左值；const int& 接纯右值时会“延长生命期”：临时值不会立刻销毁，而是一直活到这个引用消失。
// ============================================================
