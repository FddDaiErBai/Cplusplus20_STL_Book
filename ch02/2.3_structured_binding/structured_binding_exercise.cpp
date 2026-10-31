// 第 2 章 2.3 结构化绑定 —— 练习
// 编译：clang++ -std=c++23 -Wall -Wextra structured_binding_exercise.cpp -o structured_binding_exercise.exe
// 用 static_assert 在编译时检查：补完 TODO，去掉断言的注释，能编译通过就说明做对了。
// 注意：constexpr 的结构化绑定要到 C++26 才有，所以断言里用“立即调用的 lambda”[] { ... }() 包了一层，
//       lambda 里面就可以随便写绑定了。
#include <algorithm>
#include <cstddef>
#include <functional>
#include <print>
#include <span>
#include <string_view>
#include <type_traits> // std::conditional_t、std::integral_constant
#include <utility>
#include <vector>

// ============================================================
// 练习 1：一次返回多个结果
//   summarize 返回 Summary：最小值、最大值、总和
//   要求：用 std::ranges::minmax(s) 求最小最大值，它返回一个有 min 和 max 两个成员的结构体，
//         用结构化绑定把它拆开；总和用 std::ranges::fold_left
//   约定：s 不为空
// ============================================================
struct Summary
{
    int min;
    int max;
    int sum;
};

constexpr Summary summarize(std::span<const int> s)
{
    auto [s_min, s_max] = std::ranges::minmax(s);
    return {s_min, s_max, std::ranges::fold_left(s, 0, std::plus{})};
}

static_assert([]
{
    auto [lo, hi, total] = summarize(std::vector{3, 1, 4, 1, 5, 9, 2, 6});
    return lo == 1 && hi == 9 && total == 31;
}());
static_assert([]
{
    auto [lo, hi, total] = summarize(std::vector{-7});
    return lo == -7 && hi == -7 && total == -7;
}());

// ============================================================
// 练习 2：找 bug
//   shift_to_origin 想把所有点平移，使最小的 x 和最小的 y 都变成 0。
//   例：{(3, 5), (4, 7)} -> {(0, 0), (1, 2)}
//   代码能编译，但是结果不对。找出原因并修改（只需要改一个字符）
// ============================================================
struct Point
{
    int x;
    int y;

    constexpr bool operator==(const Point&) const = default;
};

constexpr void shift_to_origin(std::vector<Point>& pts)
{
    const int min_x = std::ranges::min(pts, {}, &Point::x).x;
    const int min_y = std::ranges::min(pts, {}, &Point::y).y;
    for (auto& [x, y] : pts)
    {
        x -= min_x;
        y -= min_y;
    }
}

static_assert([]
{
    std::vector<Point> pts{{3, 5}, {4, 7}, {6, 5}};
    shift_to_origin(pts);
    return pts == std::vector<Point>{{0, 0}, {1, 2}, {3, 0}};
}());

// ============================================================
// 练习 3：用结构化绑定遍历“名字-分数”对，返回分数最高的名字
//   要求：for 循环里用结构化绑定拆开 pair，不要写 .first / .second
//   约定：v 不为空；分数相同时返回先出现的那个
// ============================================================
constexpr std::string_view top_scorer(std::span<const std::pair<std::string_view, int>> v)
{
    auto [best_name, best_score] = v[0];
    for (const auto& [name,score] : v)
    {
        if (score > best_score)
        {
            best_score = score;
            best_name = name;
        }
    }
    return best_name;
}

static_assert(top_scorer(std::vector<std::pair<std::string_view, int>>{{"Tom", 80}, {"Amy", 95}, {"Bob", 70}}) == "Amy");
static_assert(top_scorer(std::vector<std::pair<std::string_view, int>>{{"Tom", 80}, {"Amy", 80}}) == "Tom");

// ============================================================
// 练习 4：让 private 成员的类支持结构化绑定
//   让 auto [name, score] = student; 能用：name 的类型是 std::string_view，score 的类型是 int
//   参考示例第 6 部分的 Point3。和 Point3 不同的是，这里两个元素的类型不一样：
//     - get<I>() 的返回类型可以写 auto，让每个分支自己决定
//     - tuple_element 要分别说明第 0 个和第 1 个的类型（可以写两个特化，也可以用 std::conditional_t）
// ============================================================
class Student
{
public:
    constexpr Student(std::string_view name, int score) : name_{name}, score_{score}
    {
    }

    constexpr std::string_view name() const
    {
        return name_;
    }

    constexpr int score() const
    {
        return score_;
    }

    template <std::size_t I>
    constexpr auto get() const
    {
        if constexpr (I == 0)
        {
            return name();
        }
        else
        {
            return score();
        }
    }

private:
    std::string_view name_;
    int score_;
};

template <>
struct std::tuple_size<Student> : std::integral_constant<std::size_t, 2>
{};

template <>
struct std::tuple_element<0, Student>
{
    using type = std::string_view;
};

template <>
struct std::tuple_element<1, Student>
{
    using type = int;
};

static_assert([]
{
    auto [name, score] = Student{"Amy", 95};
    return name == "Amy" && score == 95;
}());

// ============================================================
// 练习 5（思考题，把答案写在注释里）
//   (1) 下面这段代码输出什么？为什么？
//         std::pair<int, int> p{1, 2};
//         auto& [a, b] = p;
//         auto [c, d] = p;
//         a = 10;
//         c = 20;
//         std::println("{} {} {} {}", p.first, a, c, d);
//   (2) 标准库的 std::div(47, 5) 返回 std::div_t，它有 quot（商）和 rem（余数）两个成员。
//       为什么 auto [q, r] = std::div(47, 5); 是不可移植的写法？（提示：C 语言标准规定了这两个成员的顺序吗？）
//       练习 1 里，std::ranges::minmax 的结果用绑定拆开就没问题，区别在哪？
//   (3) 书上的 make_commas 里有一句 for (int l = s.length() - 3; l > 0; l -= 3)。
//       当数字小于 100（s.length() 是 1 或 2）时，s.length() - 3 是多少？赋给 int l 之后又是多少？
//       这段代码碰巧能正确运行，为什么说它写得不好？（回想 1.4 节）
//
// 你的答案：
//   (1)10 10 20 2 auto&的结构化绑定可以修改原本对象的值
//       改动（补充）：正确。另一半原因：auto [c, d] 的看不见的对象是 p 的副本，c 是副本的 first，
//       改 c 不影响 p；d 是副本的 second，复制时 p.second 是 2，所以 d 是 2。
//   (2)不知道
//       改动（补充）：结构化绑定按成员“声明的顺序”拆：第一个名字对应第一个成员。
//       C 语言标准只说 div_t 里有 quot 和 rem 两个成员，明确写了“顺序不限”（in either order）。
//       我们的 mingw 恰好是 quot 在前，所以能用；换一个平台，q 拿到的可能是余数，而且不会有任何报错。
//       std::ranges::minmax 返回的 min_max_result 是 C++ 标准定义的，规定了 min 在前、max 在后，所以拆开是安全的。
//       结论：只有顺序有保证的类型（pair、tuple、array、你自己写的 struct、标准规定了成员顺序的类型）才用绑定，
//       否则老老实实写 .quot / .rem。
//   (3)不懂
//       改动（补充）：s.length() 返回 size_t（无符号）。数字是 47 时 s.length() 是 2：
//         2 - 3 在无符号里不会变成 -1，而是绕回最大值 18446744073709551615（2 的 64 次方减 1）
//         再赋给 int l，大数装不下，C++20 起规定按二进制截断，结果是 -1（C++20 之前结果由编译器决定）
//         l = -1，l > 0 不成立，循环一次都不执行，结果“碰巧”正确。
//       写得不好的原因：正确性依赖“绕回 + 截断”两次意外，任何一步不按预期结果就错；
//       而且 -Wall -Wextra 不会警告（要加 -Wconversion 才有）。这正是 1.4 节说的有符号/无符号混用问题。
//       安全的写法，全程用 size_t，并且先比较再减，不会出现负数：
//         for (std::size_t l = s.length(); l > 3;)
//         {
//             l -= 3;
//             s.insert(l, ",");
//         }
// ============================================================

int main()
{}
