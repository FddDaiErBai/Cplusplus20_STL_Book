// 第 5 章 5.4 算法库中的谓词 —— 练习
// 编译：clang++ -std=c++23 -Wall -Wextra -Wpedantic predicates_exercise.cpp -o predicates_exercise.exe
//       g++     -std=c++23 -Wall -Wextra -Wpedantic predicates_exercise.cpp -o predicates_exercise.exe -lstdc++exp
// 运行时检查：补完 TODO，编译运行，所有行都显示“通过”就说明做对了。
// 先看示例 predicates.cpp 的 [1]（算法怎么接收谓词）、[4]（调用次数）、[5]（返回类型）、[6]（投影）。
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <optional>
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
// 练习 1：预测结果（先自己想，把答案填进 ans_ 变量，再运行看对不对）
//   下面每段代码都在用一个谓词。注意：v 是 {1, 7, 4, 9, 4, 8, 12, 10, 20}，一共 9 个元素。
// ============================================================
int ans_p1 = 9;     // TODO：P1 谓词被调用了几次
int ans_p2 = 2;     // TODO：P2 谓词被调用了几次
int ans_p3 = 7;     // TODO：P3 谓词被调用了几次
bool ans_p4 = true; // TODO：P4 返回 int 的谓词，ranges::count_if 能用吗
bool ans_p5 = true; // TODO：P5 返回 int 的谓词，满足 std::predicate<..., int> 吗

void test1()
{
    const std::vector<int> v{1, 7, 4, 9, 4, 8, 12, 10, 20};

    {
        // P1
        int calls = 0;
        (void)std::ranges::count_if(v, [&calls](int x) { ++calls; return x > 0; });
        check(calls == ans_p1, "练习 1：P1");
    }
    {
        // P2：找第一个大于 5 的
        int calls = 0;
        (void)std::ranges::find_if(v, [&calls](int x) { ++calls; return x > 5; });
        check(calls == ans_p2, "练习 1：P2");
    }
    {
        // P3：有没有大于 10 的
        int calls = 0;
        (void)std::ranges::any_of(v, [&calls](int x) { ++calls; return x > 10; });
        check(calls == ans_p3, "练习 1：P3");
    }
    {
        // P4
        auto parity = [](int x) { return x % 2; };
        check((std::ranges::count_if(v, parity) > 0) == ans_p4, "练习 1：P4");
    }
    {
        // P5
        auto parity = [](int x) { return x % 2; };
        check(std::predicate<decltype(parity), int> == ans_p5, "练习 1：P5");
    }
}

// ============================================================
// 练习 2：写谓词，交给算法
//   每个函数的注释里写清了要求和期望结果。用 ranges 版本（std::ranges::xxx），
//   谓词都写成 lambda。
//
//   ① count_divisible(v, d)：v 里有几个能被 d 整除的数（0 也行）。
//   ② has_negative(v)：v 里有没有负数。用 std::ranges::any_of。
//      空 vector 应该返回 false。
//   ③ first_word_longer(words, min_len)：第一个长度 > min_len 的单词。
//      用 std::ranges::find_if。找到返回 std::optional<std::string_view>，没有返回 std::nullopt。
//      注意 find_if 返回的是迭代器，没找到时它等于 end()，不能直接解引用。
//   ④ sort_by_abs(v)：按绝对值从小到大排序。绝对值相同的情况这个练习里不会出现。
//      用 std::ranges::sort 加一个二元谓词。
//   ⑤ erase_multiples(v, m)：删掉 v 里所有能被 m 整除的数，返回删了几个。
//      用 std::erase_if(v, 谓词)（C++20，第 3 章 3.3 节讲过，它内部就是 remove_if + erase）。
//   ⑥ count_at_least(people, age)：people 里 age 字段 >= 给定值的人数。
//      用 std::ranges::count_if 加投影（示例 [6] ③），谓词只接受一个 int。
// ============================================================
std::size_t count_divisible(const std::vector<int>& v, int d)
{
    // 改动（说明）：对。顺便注意一件事：ranges::count_if 返回的是 std::ptrdiff_t（有符号，64 位上是 long long），
    //   这个函数返回 std::size_t（无符号）。return 时悄悄做了一次“有符号 → 无符号”转换。
    //   -Wall -Wextra 不报，加 -Wsign-conversion 就会报：implicit conversion changes signedness: 'long long' to 'unsigned long long'
    //   这里计数不可能是负数，所以结果没问题；练习 3 里我写的 static_cast<std::size_t>(...) 就是把这个转换明着写出来。
    //   ⑥ count_at_least 也一样。
    return std::ranges::count_if(v, [d](int x)
    {
        return x % d == 0;
    });
}

bool has_negative(const std::vector<int>& v)
{
    return std::ranges::any_of(v, [](int x)
    {
        return x < 0;
    });
}

std::optional<std::string_view> first_word_longer(const std::vector<std::string>& words, std::size_t min_len)
{
    // 改动（说明）：谓词参数写 std::string_view 也对：每个 std::string 元素会隐式转成 string_view，不拷贝字符。
    auto it = std::ranges::find_if(words, [min_len](std::string_view s)
    {
        return s.size() > min_len;
    });

    if (it == words.end())
    {
        return std::nullopt;
    }
    return std::string_view{*it};
}

void sort_by_abs(std::vector<int>& v)
{
    std::ranges::sort(v, [](int a, int b)
    {
        return std::abs(a) < std::abs(b);
    });
}

std::size_t erase_multiples(std::vector<int>& v, int m)
{
    return std::erase_if(v, [m](int x)
    {
        return x % m == 0;
    });
}

struct Person
{
    std::string name;
    int age;
};

std::size_t count_at_least(const std::vector<Person>& people, int age)
{
    return std::ranges::count_if(people, [age](int x)
    {
        return x >= age;
    }, &Person::age);
}

void test2()
{
    std::vector<int> v{12, 7, 30, 9, 4, 8, 25, 10, 20};
    check(count_divisible(v, 4) == 4, "练习 2 ①：{12 7 30 9 4 8 25 10 20} 里能被 4 整除的有 4 个");
    check(count_divisible({}, 3) == 0 && count_divisible({0, 6, 7}, 3) == 2, "练习 2 ①：空的是 0；0 能被 3 整除");

    check(has_negative({1, -2, 3}), "练习 2 ②：{1 -2 3} 有负数");
    check(!has_negative({1, 2, 3}) && !has_negative({}), "练习 2 ②：没有负数、空的都返回 false");

    std::vector<std::string> words{"a", "bee", "cat", "dolphin"};
    auto w1 = first_word_longer(words, 3);
    check(w1.has_value() && *w1 == "dolphin", "练习 2 ③：第一个长度 > 3 的是 dolphin");
    auto w2 = first_word_longer(words, 1);
    check(w2.has_value() && *w2 == "bee", "练习 2 ③：第一个长度 > 1 的是 bee");
    check(!first_word_longer(words, 10).has_value() && !first_word_longer({}, 0).has_value(), "练习 2 ③：找不到、空的都返回 nullopt");
    // 改动：补一项检查。返回的 string_view 指向 words 里那个元素本身（没有拷贝），所以 data() 地址相同
    check(w1.has_value() && w1->data() == words[3].data(), "练习 2 ③：返回的 string_view 指向 words[3] 本身");

    std::vector<int> abs_v{-30, 2, -100, 45, -7};
    sort_by_abs(abs_v);
    check(abs_v == std::vector<int>{2, -7, -30, 45, -100}, "练习 2 ④：按绝对值排 → 2 -7 -30 45 -100");

    std::vector<int> m{3, 4, 6, 9, 10, 12};
    std::size_t erased = erase_multiples(m, 3);
    check(erased == 4 && m == std::vector<int>{4, 10}, "练习 2 ⑤：删掉能被 3 整除的 4 个 → 4 10");
    check(erase_multiples(m, 7) == 0 && m == std::vector<int>{4, 10}, "练习 2 ⑤：没有能删的，返回 0");

    std::vector<Person> people{{"Ada", 36}, {"Bob", 19}, {"Cy", 44}, {"Di", 27}};
    check(count_at_least(people, 30) == 2 && count_at_least(people, 0) == 4 && count_at_least({}, 0) == 0,
          "练习 2 ⑥：>= 30 岁 2 人，>= 0 岁 4 人，空的 0 人");
}

// ============================================================
// 练习 3：找 bug（两处，每处只改一个地方，别的不要动）
//   ① count_above：数 v 里有几个“不小于 limit”的数，顺便把 v 里大于 limit 的元素都压到 limit。
//      结果数是对的，但调用完以后 v 里本来大于 limit 的元素真的被改了 —— 调用者拿到的 v 和传进去的不一样。
//      注意：这个 lambda 的写法在语法上完全合法，编译器不会警告。
//   ② count_logged：数 v 里有几个大于 limit 的数，顺便通过 calls 参数告诉调用者“谓词被调用了几次”。
//      现在 calls 一直是 0。
// ============================================================
std::size_t count_above(std::vector<int>& v, int limit)
{
    return static_cast<std::size_t>(std::ranges::count_if(v, [limit](int x)
    {
        x = std::min(x, limit);
        return x == limit;
    }));
}

std::size_t count_logged(const std::vector<int>& v, int limit, int& calls)
{
    // 改动（说明）：只改了一处 calls → &calls，符合要求。mutable 现在可以去掉了：按引用捕获本来就能改外面的 calls，
    //   mutable 只管按值捕获的成员（5.3 节 [3]）。留着也不出错，只是多余。
    return static_cast<std::size_t>(std::ranges::count_if(v, [limit, &calls](int x) mutable
    {
        ++calls;
        return x > limit;
    }));
}

void test3()
{
    std::vector<int> v{1, 5, 10};
    std::size_t n = count_above(v, 5);
    check(n == 2, "练习 3 ①：{1 5 10} 里不小于 5 的有 2 个");
    check(v == std::vector<int>{1, 5, 10}, "练习 3 ①：调用完 v 必须还是 {1 5 10}");

    std::vector<int> w{1, 5, 10};
    int calls = 0;
    std::size_t m = count_logged(w, 5, calls);
    check(m == 1 && calls == 3, "练习 3 ②：结果 1，谓词被调用 3 次");
    // 改动：补一项检查。再调用一次，calls 接着往上加（3 + 3 = 6），说明改的确实是调用者那个变量
    (void)count_logged(w, 5, calls);
    check(calls == 6, "练习 3 ②：再调用一次，calls 累加到 6");
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
// 1. 书上写 bool (*fp)(int) = is_div4; 而那个 is_div4 是一个结构体（函子）。
//    (a) 为什么普通函数 is_div4 可以赋给函数指针，函子 Div4{} 不行？函数指针里存的是什么，函子对象里存的是什么？
// 你的答案：函数指针只是一个代码地址，可能存有非静态数据成员。
// 改动（补充）：方向对，但两半都没说完整。
//   · 函数指针：只存一个代码地址。调用时直接跳过去，不需要任何对象。
//   · 函子：调用的是成员函数 operator()，成员函数调用必须有一个对象（this）。函子对象可能存数据，即使没有数据，
//     operator() 也是“这个对象的”成员函数，光有代码地址不知道该对哪个对象调用。
//   · 那没有捕获的 lambda 为什么行？因为编译器给它额外生成了一个“转换成函数指针”的转换函数（5.1 节 [7]）。
//     普通的 struct Div4 没有这个转换函数，所以转不了 —— 不是做不到，是没人写（自己在 Div4 里加 using Fn = bool (*)(int); operator Fn() const {...} 也能转，两个编译器都验证过）。
//    (b) 没有捕获的 lambda 能转成函数指针。转换之后传给 count_if，算法收到的类型是什么？和直接传 lambda 比，对编译有什么影响？
// 你的答案：函数指针，没法内联。
// 改动（补充）：对，类型是 bool(*)(int)。更准确地说是“更难内联”：算法拿到的只是一个运行时的地址，
//   所有函数指针都是同一个类型，同一份 count_if<..., bool(*)(int)> 要能处理任何函数，编译器看不到具体调哪一个。
//   直接传 lambda 时，每个 lambda 是独立的类型，count_if 为它单独实例化一份，函数体就在眼前，可以直接展开。
//   （开 -O2 时，如果 count_if 本身被内联进调用处，编译器有时还是能看出指针是常量，再把它内联掉。所以是“不保证”，不是“绝对不能”。）
//
// 2. 示例 [4] 里 count_if 对 9 个元素调用了 9 次谓词，而 sort 对 5 个元素比较了 7 次或 8 次。
//    (a) 前者为什么正好是 9 次？后者为什么标准不给确切数字？
// 你的答案：需要遍历每一个元素。每个算法实现不一样。
// 改动（补充）：对。补一点标准原文的意思：count_if 的规定是“恰好调用 last - first 次谓词”，所以一定是 9。
//   sort 的规定只有复杂度上界：比较次数是 O(n log n)。用哪种排序（libc++ 和 libstdc++ 都是内省排序 + 小规模插入排序，细节不同）
//   标准不管，所以 5 个元素时 clang 8 次、g++ 7 次，都合法。
//    (b) 为什么“在谓词里 ++counter 统计调用次数”这件事本身就不可靠？算法对谓词做了什么？
// 你的答案：因为counter可能是按值传进来的，算法可能对其复制。
// 改动（补充）：对，抓到了关键：算法按值接收谓词，内部还可以随便复制。分两种情况说清楚：
//   · 计数器按值存在谓词里（[n = 0] mutable）：每个副本各数各的，你最后看到的只是某一个副本的数，不是总数 → 不可靠
//   · 按引用捕获外面的计数器（[&calls]，示例 [4] 和练习 3 ② 的写法）：所有副本共用同一个 calls，总数是对的 → 可靠
//   就算总数可靠，“到底调用几次”对 sort 这类算法也是未指定的，所以只能拿来观察，不能让程序逻辑依赖它。
//
// 3. 示例 [5] 和 [6] 里有两个“两个库表现不一样”的地方。
//    (a) 返回 explicit operator bool 的谓词：classic 的 count_if 接受，ranges::count_if 拒绝。原因是什么？
// 你的答案：convertible_to是隐式转换，不能接受explicit函数。
//    (b) 装 unique_ptr 的谓词（不能拷贝）：ranges 一定拒绝，classic 在 clang 上能过、在 g++ 上不能。这说明标准对 classic 算法的实现有什么要求（或者没要求）？
// 你的答案：没什么标准。
// 改动（补充）：标准其实明确说了一句：除非特别说明，算法可以随意复制传进来的函数对象（[algorithms.requirements]）。
//   意思是：“复制谓词”是标准允许的行为。libstdc++ 内部复制了一次 → 要求谓词能拷贝 → 装 unique_ptr 的谓词编译不过；
//   libc++ 恰好没复制 → 能编译。两个都合法。
//   结论：标准没有保证 classic 算法“不复制”，所以想在所有编译器上都能用，谓词必须能拷贝。
//   ranges 算法更进一步，直接用概念 copy_constructible 写进约束里，任何库都会拒绝。
// ============================================================
