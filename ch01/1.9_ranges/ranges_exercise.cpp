// 第 1 章 1.9 范围和视图 —— 练习
// 编译：clang++ -std=c++23 -Wall -Wextra ranges_exercise.cpp -o ranges_exercise.exe
// 所有函数都是 constexpr，用 static_assert 在编译时检查：补完 TODO，去掉断言的注释，能编译通过就说明做对了。
// 要求：每个函数都用视图和范围算法来写，不要写 for 循环。
#include <algorithm>
#include <functional>
#include <print>
#include <ranges>
#include <string_view>
#include <vector>

namespace ranges = std::ranges;
namespace views = std::views;

// ============================================================
// 练习 1：1 到 n 之间所有奇数的平方和
//   例：n = 5 -> 1 + 9 + 25 = 35
//   提示：iota 生成数字 -> filter 留下奇数 -> transform 求平方 -> ranges::fold_left 求和
//         注意 iota(a, b) 不包括 b
// ============================================================
constexpr int sum_of_odd_squares(int n)
{
    auto num{
        views::iota(1, n + 1) | views::filter([](const int i)
        {
            return i % 2 == 1;
        }) | views::transform([](const int i)
        {
            return i * i;
        })
    };
    return ranges::fold_left(num, 0, std::plus{});
}

static_assert(sum_of_odd_squares(5) == 35);
static_assert(sum_of_odd_squares(6) == 35);
static_assert(sum_of_odd_squares(0) == 0);

// ============================================================
// 练习 2：k 的前 n 个正倍数，放进 vector 返回
//   例：k = 7, n = 3 -> {7, 14, 21}
//   要求：用无限序列 iota(1) 来做，不要自己算上限
//   提示：lambda 要用到参数 k，记得写捕获 [k]；最后用 ranges::to 把视图变成 vector
// ============================================================
constexpr std::vector<int> first_multiples(int k, int n)
{
    return {
        views::iota(1) | views::take(n) | views::transform([k](const int i)
        {
            return i * k;
        }) | ranges::to<std::vector>()
    };
}

static_assert(first_multiples(7, 3) == std::vector{7, 14, 21});
static_assert(first_multiples(1, 5) == std::vector{1, 2, 3, 4, 5});
static_assert(first_multiples(5, 0).empty());

// ============================================================
// 练习 3：前 k 个元素不动，把剩下的元素按从小到大排序
//   例：{5, 4, 3, 2, 1}, k = 2 -> {5, 4, 1, 2, 3}
//   这就是书上那句“sort(v.begin() + 5, v.end())”真正做的事
//   提示：示例第 4 部分。k 比元素个数还多时会怎样？试试看需不需要特别处理
// ============================================================
constexpr std::vector<int> sort_all_but_first(std::vector<int> v, int k)
{
    if (k < 0)
    {
        return v;
    }
    ranges::sort(v | views::drop(k));
    return v;
}

static_assert(sort_all_but_first({5, 4, 3, 2, 1}, 2) == std::vector{5, 4, 1, 2, 3});
static_assert(sort_all_but_first({3, 1, 2}, 0) == std::vector{1, 2, 3});
static_assert(sort_all_but_first({3, 1, 2}, 5) == std::vector{3, 1, 2});

// ============================================================
// 练习 4：点积：a[0]*b[0] + a[1]*b[1] + ...，长度不同时只算到短的那个结束
//   例：{1, 2, 3} · {4, 5, 6} = 4 + 10 + 18 = 32
//   提示：views::zip 把两个范围配对，每个元素是一个 tuple，lambda 里可以用结构化绑定拆开：
//         [](auto t) { auto [x, y] = t; ... }
// ============================================================
constexpr int dot(const std::vector<int>& a, const std::vector<int>& b)
{
    auto vec{
        views::zip(a, b) | views::transform([](auto t)
        {
            auto [x,y] = t;
            return x * y;
        })
    };
    return ranges::fold_left(vec, 0, std::plus{});
}

static_assert(dot({1, 2, 3}, {4, 5, 6}) == 32);
static_assert(dot({1, 2, 3}, {4, 5}) == 14);
static_assert(dot({}, {1, 2}) == 0);

// ============================================================
// 练习 5：按分数从高到低排名，返回名字列表
//   提示：ranges::sort 的第 2 个参数是比较方式（从大到小用 ranges::greater{}），第 3 个参数是投影；
//         排好之后用 transform 取出名字，再 ranges::to
//   （1.5 节你用自定义 <=> 做过排名，对比一下哪种写法更简单）
// ============================================================
struct Student
{
    std::string_view name;
    int score;
};

constexpr std::vector<std::string_view> ranking(std::vector<Student> students)
{
    ranges::sort(students, ranges::greater{}, &Student::score);
    return students | views::transform(&Student::name) | ranges::to<std::vector>();
}

static_assert(
    ranking({{"Tom", 80}, {"Amy", 95}, {"Bob", 70}, {"Eve", 60}}) == std::vector<std::string_view>{
        "Amy", "Tom", "Bob", "Eve"
    });

// ============================================================
// 练习 6（思考题，把答案写在注释里）
//   (1) 示例第 2 部分里，transform | filter 时 3 和 4 的平方各算了两次，1 和 2 只算了一次。为什么？
//       如果 transform 里的函数很耗时，怎么调整顺序可以避免重复计算？（提示：本练习第 1 题的顺序）
//   (2) 把下面 main 里 print_all 那两行取消注释编译一次（取消一行试一行）。
//       为什么 take 的视图可以传给 const auto&，filter 的视图不行？print_all 的参数应该怎么改？
//   (3) 示例里的 views::iota(0, v.size()) 为什么编译失败？除了 0uz，还有什么改法？
//       如果是 C++20（没有 uz 后缀）呢？
//
// 你的答案：
//   (1)transform是惰性求值，不会保存结果。第一次filter判断成功，不会拿到值，只会记住这个位置，第二次for才会通过transform拿到值。可以先filter在transform
//       改动（补充）：基本正确，有一处要更准确：filter 判断时其实“拿到了值”——它必须取出平方结果交给
//       lambda 判断 > 5，判断完就丢掉了，只记住位置；for 循环再取值时 transform 又算一次。
//       1 和 2 没通过筛选，for 循环不会取它们的值，所以只算一次。
//       先 filter 再 transform 的前提是：筛选条件能用原来的值写出来。如果条件必须看计算后的结果（比如
//       “平方 > 5”），就没法调换顺序，这时可以先用 ranges::to 把 transform 的结果存进 vector 再筛选。
//   (2)不知道
//       改动（补充）：filter_view 的 begin() 要从头找第一个满足条件的元素，这可能要走很远（O(n)）。
//       标准要求 begin() 平均下来是 O(1)，所以 filter_view 第一次调用 begin() 时会把结果缓存在自己身上，
//       之后直接用。缓存就是修改自己，所以 begin() 不是 const 函数，const 的 filter_view 无法遍历。
//       take_view 没有这个问题：它的 begin() 就是原容器的 begin()，不用缓存，所以有 const 版本。
//       改法：参数写成 auto&&（转发引用），const 和非 const、变量和临时对象都能接，见下面的 print_all。
//       经验：写接受“任意范围”的函数时，参数用 auto&& 或 R&&，不要用 const auto&。
//   (3)0是int v.size()是size_t static_cast<size_t>
//       改动（补充）：正确。static_cast 在 C++20 里就能用，所以 C++20 的问题你也答了。其他改法：
//         views::iota(std::size_t{0}, v.size())          —— 和 static_cast 一样，下标是 size_t
//         views::iota(0, static_cast<int>(v.size()))     —— 反过来转，下标是 int，循环里和 int 运算更方便
//         views::iota(0u, v.size())                      —— 规则只要求“同为无符号”，unsigned 和 size_t 也行
//       C++26 还会加一个 views::indices(v.size())，专门解决这个问题。
// ============================================================

// 改动：参数从 const auto& 改成 auto&&（思考题 (2)），下面 main 里两行都能编译了
void print_all(auto&& r)
{
    for (const auto& x : r)
    {
        std::print("{} ", x);
    }
    std::println("");
}

int main()
{
    std::vector<int> v{1, 2, 3, 4, 5, 6};
    print_all(v | views::take(3));                                     // 改动：取消注释
    print_all(v | views::filter([](int i) { return i % 2 == 0; }));    // 改动：取消注释
}
