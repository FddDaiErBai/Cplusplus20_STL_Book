// 第 4 章 4.9 zip —— 练习
// 编译：clang++ -std=c++23 -Wall -Wextra -Wpedantic zip_iterator_exercise.cpp -o zip_iterator_exercise.exe
//       g++     -std=c++23 -Wall -Wextra -Wpedantic zip_iterator_exercise.cpp -o zip_iterator_exercise.exe -lstdc++exp
// 运行时检查：补完 TODO，编译运行，所有行都显示“通过”就说明做对了。
// 先看示例 zip_iterator.cpp 的 [3]（== 为什么用 ||）、[4]（范围 + 迭代器 + 哨兵）、[5][6]（views::zip 和它的用法）。
#include <algorithm>
#include <cstddef>
#include <functional>
#include <iterator>
#include <list>
#include <map>
#include <print>
#include <ranges>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

void check(bool ok, std::string_view name)
{
    std::println("{} {}", ok ? "通过" : "失败", name);
}

using V = std::vector<int>;

// ============================================================
// 练习 1：用标准库的 views::zip 系列（示例 [5][6]），每个函数体只写一条语句（return ...;），不写循环
//   dot(a, b)            ：a[0]*b[0] + a[1]*b[1] + ……，长度不同时以短的为准
//                          提示：views::zip_transform(std::multiplies{}, a, b) 得到每一对的乘积，
//                                再用 std::ranges::fold_left(..., 0, std::plus{}) 加起来
//   count_same(a, b)     ：有几个位置上 a[i] == b[i]（以短的为准）
//                          提示：zip_transform(std::equal_to{}, a, b) 得到一串 true/false，再 std::ranges::count(..., true)
//   to_map(keys, values) ：把两个 vector 配成一个 std::map<std::string, int>
//   sort_together(scores, names)：按 scores 从小到大排序，names 跟着一起换位置（scores 保证没有重复）
//                          提示：示例 [6]。这个函数没有返回值，一条语句就行
// ============================================================
int dot(const V& a, const V& b)
{
    return std::ranges::fold_left(std::views::zip_transform(std::multiplies{}, a, b), 0, std::plus{});
}

long long count_same(const V& a, const V& b)
{
    return std::ranges::count(std::views::zip_transform(std::equal_to{}, a, b), true);
}

std::map<std::string, int> to_map(const std::vector<std::string>& keys, const V& values)
{
    return std::views::zip(keys, values) | std::ranges::to<std::map>();
    // 改动（补充）：to<std::map>() 不写模板参数也对：它从 zip 的元素 tuple<const string&, const int&> 推导出 map<string, int>。
    //   键重复时（下面新加的检查），map 保留先插入的那个，后面的被忽略（和 try_emplace 一样）
}

void sort_together(V& scores, std::vector<std::string>& names)
{
    std::ranges::sort(std::views::zip(scores, names));
}

void test_exercise1()
{
    check(dot(V{1, 2, 3}, V{4, 5, 6}) == 32, "练习 1：dot({1 2 3}, {4 5 6}) → 32");
    check(dot(V{1, 2, 3}, V{10}) == 10 && dot(V{}, V{1, 2}) == 0, "练习 1：dot 长度不同 / 空的");

    check(count_same(V{1, 2, 3, 4}, V{1, 0, 3, 0}) == 2, "练习 1：count_same → 2");
    check(count_same(V{5, 5}, V{5, 5, 5}) == 2 && count_same(V{}, V{}) == 0, "练习 1：count_same 长度不同 / 空的");

    auto m = to_map({"b", "a", "c"}, V{2, 1, 3});
    check(m == std::map<std::string, int>{{"a", 1}, {"b", 2}, {"c", 3}}, "练习 1：to_map");
    check(to_map({"x", "y"}, V{7}) == std::map<std::string, int>{{"x", 7}}, "练习 1：to_map 长度不同，以短的为准");

    V s{70, 95, 80};
    std::vector<std::string> n{"Ann", "Bob", "Cy"};
    sort_together(s, n);
    check(s == V{70, 80, 95} && n == std::vector<std::string>{"Ann", "Cy", "Bob"}, "练习 1：sort_together，names 跟着动");

    // 改动（补充）：键重复时保留第一个
    check(to_map({"a", "a", "b"}, V{1, 2, 3}) == std::map<std::string, int>{{"a", 1}, {"b", 3}}, "练习 1：to_map 键重复，保留第一个");
}

// ============================================================
// 练习 2：自己写 Adjacent —— 每个元素和它的下一个配对（相当于 views::adjacent<2>，示例 [6]）
//     {1, 4, 9, 16} → (1,4) (4,9) (9,16)          只有 1 个或 0 个元素 → 什么都没有
//   照着示例 [4] 的写法：范围 + 迭代器 + 哨兵
//     AdjIt<I> 存两个位置：cur_（当前）、next_（下一个，总是 cur_ 的后一个）
//       *it        ：返回 std::pair<std::iter_reference_t<I>, std::iter_reference_t<I>>{*cur_, *next_}（两个引用组成的 pair）
//       ++it       ：cur_、next_ 各往后走一步
//       it == it2  ：比较 cur_
//       it == 哨兵 ：next_ 走到了范围的 end（这时已经没有“下一个”了）
//     AdjEnd<I>：哨兵，存范围的 end（已经写好了）
//     Adjacent<R>：范围，存一个指向 R 的指针；begin()、end() 已经写好了（begin 里处理了空范围，看看为什么这样写）
//   TODO（都在 AdjIt 里）：
//     ① 类型别名：iterator_concept 写 forward；iterator_category 写 input（和示例 [4] 一样的原因）；
//        value_type = std::pair<std::iter_value_t<I>, std::iter_value_t<I>>；difference_type = std::ptrdiff_t
//     ② 默认构造函数
//     ③ operator*
//     ④ 前置 ++、后置 ++
//     ⑤ 两个 ==（现在和哨兵比较的那个永远返回 true，所以什么都遍历不到）
//   下面的 helper 用 if constexpr 判断能不能用，helper 不用改。
// ============================================================
template <typename I>
struct AdjEnd
{
    I end{};
};

template <typename I>
class AdjIt
{
    I cur_{};
    I next_{};

public:
    using iterator_concept = std::forward_iterator_tag;
    using iterator_category = std::input_iterator_tag;
    using value_type = std::pair<std::iter_reference_t<I>, std::iter_reference_t<I>>;
    using difference_type = std::ptrdiff_t;

    AdjIt() = default;

    AdjIt(I cur, I next)
        : cur_{cur}, next_{next}
    {
    }

    std::pair<std::iter_reference_t<I>, std::iter_reference_t<I>> operator*() const
    {
        return {*cur_, *next_};
    }

    AdjIt& operator++()
    {
        ++cur_;
        ++next_;
        return *this;
    }

    AdjIt operator++(int)
    {
        AdjIt old = *this;
        ++*this;
        return old;
    }

    bool operator==(const AdjIt& other) const
    {
        return cur_ == other.cur_;
    }

    bool operator==(const AdjEnd<I>& e) const
    {
        return next_ == e.end;
    }
};

template <std::ranges::forward_range R>
class Adjacent
{
    R* r_;
    using I = std::ranges::iterator_t<R>;

public:
    explicit Adjacent(R& r)
        : r_{&r}
    {
    }

    AdjIt<I> begin() const
    {
        I first = std::ranges::begin(*r_);
        I second = first;
        if (second != std::ranges::end(*r_)) // 空范围时不能对 end ++，所以 second 就停在 first（也就是 end）
        {
            ++second;
        }
        return {first, second};
    }

    AdjEnd<I> end() const
    {
        return {std::ranges::end(*r_)};
    }
};

// 相邻两个数的差：{1, 4, 9, 16} → 3 5 7
template <typename R>
V diffs(R& r)
{
    if constexpr (std::ranges::forward_range<Adjacent<R>>)
    {
        return Adjacent{r} | std::views::transform([](auto p) { return p.second - p.first; }) | std::ranges::to<V>();
    }
    (void)r;
    return {-1};
}

// 是不是严格递增：每一对都是 first < second
template <typename R>
bool increasing(R& r)
{
    if constexpr (std::ranges::forward_range<Adjacent<R>>)
    {
        return std::ranges::all_of(Adjacent{r}, [](auto p) { return p.first < p.second; });
    }
    (void)r;
    return false;
}

// 通过引用修改：把每一对里的 second 改成 first + second（从前往后做，所以是“前缀和”）
template <typename R>
void prefix_sum(R& r)
{
    if constexpr (std::ranges::forward_range<Adjacent<R>>)
    {
        for (auto [a, b] : Adjacent{r})
        {
            b += a;
        }
    }
    (void)r;
}

// 后置 ++：old = it++ 之后，*old 是第一对，*it 是第二对
template <typename R>
bool postfix_ok(R& r)
{
    if constexpr (std::ranges::forward_range<Adjacent<R>>)
    {
        auto it = Adjacent{r}.begin();
        auto old = it++;
        return (*old).first == 1 && (*old).second == 4 && (*it).first == 4 && (*it).second == 9;
    }
    (void)r;
    return false;
}

void test_exercise2()
{
    V sq{1, 4, 9, 16};
    check(diffs(sq) == V{3, 5, 7}, "练习 2：diffs({1 4 9 16}) → 3 5 7");
    check(std::ranges::forward_range<Adjacent<V>> && !std::ranges::sized_range<Adjacent<V>>, "练习 2：forward_range，不是 sized_range");
    check(postfix_ok(sq), "练习 2：后置 ++ 返回旧值");

    V one{5};
    V empty;
    check(diffs(one).empty() && diffs(empty).empty(), "练习 2：一个元素 / 空的 → 什么都没有");

    std::list<int> l{2, 3, 5, 7};
    V down{3, 2, 1};
    check(increasing(l) && !increasing(down) && increasing(one), "练习 2：increasing（list 也能用）");

    V p{1, 2, 3, 4};
    prefix_sum(p);
    check(p == V{1, 3, 6, 10}, "练习 2：通过引用修改：前缀和 → 1 3 6 10");

    // 改动（补充）：const 容器（*it 是 const int&，pair 里装的是两个 const 引用，只读也能用）、list 上改值
    const V cv{1, 3, 6};
    std::list<int> pl{1, 2, 3};
    prefix_sum(pl);
    check(diffs(cv) == V{2, 3} && pl == std::list<int>{1, 3, 6}, "练习 2：const vector 求差 / list 前缀和");
}

// ============================================================
// 练习 3：找 bug（两个 bug。第一个改一行；第二个是两行的顺序不对，调换一下。只改这两处，别的地方不要动）
//   Twin<T>：把两个同类型的容器配对（书上 zip_iterator 的思路，但范围和迭代器已经分开了）。
//   运行后看到：
//     * add_into（把 b 加到 a 上）之后 a 没有变
//     * old = it++ 之后，*old 不是第一对
//   提示：示例 [2] (d)：*it 返回的是什么？range-for 里的 x 是谁的别名？
//         4.4 节：后置 ++ 应该返回加之前的还是加之后的？
// ============================================================
template <typename T>
class Twin
{
    using ref_t = typename T::reference; // 改动（补充）：对。T::reference 对 vector<int> 就是 int&，pair<int&, int&> 里装的是引用。
    //   名字 val_t 已经名不副实了（它不再是“值类型”），实际代码里应该改名叫 ref_t；
    //   这里题目要求只改一行，保留原名没问题
    using it_t = typename T::iterator;

    T* a_;
    T* b_;

public:
    Twin(T& a, T& b)
        : a_{&a}, b_{&b}
    {
    }

    class iterator
    {
        it_t ia_{};
        it_t ib_{};

    public:
        iterator(it_t ia, it_t ib)
            : ia_{ia}, ib_{ib}
        {
        }

        std::pair<val_t, val_t> operator*() const
        {
            return {*ia_, *ib_};
        }

        iterator& operator++()
        {
            ++ia_;
            ++ib_;
            return *this;
        }

        iterator operator++(int)
        {
            iterator old = *this;
            ++*this;
            return old;
        }

        bool operator==(const iterator& o) const
        {
            return ia_ == o.ia_ || ib_ == o.ib_;
        }
    };

    iterator begin() const
    {
        return {a_->begin(), b_->begin()};
    }

    iterator end() const
    {
        return {a_->end(), b_->end()};
    }
};

void add_into(V& a, V& b)
{
    for (auto [x, y] : Twin{a, b})
    {
        x += y;
    }
}

void test_exercise3()
{
    V a{1, 2, 3};
    V b{10, 20};
    std::print("  Twin：");
    for (auto [x, y] : Twin{a, b})
    {
        std::print("[{}, {}] ", x, y);
    }
    std::println("");

    add_into(a, b);
    check(a == V{11, 22, 3} && b == V{10, 20}, "练习 3：add_into({1 2 3}, {10 20}) → a 变成 11 22 3");

    V c{1, 2, 3};
    V d{4, 5, 6};
    Twin t{c, d};
    auto it = t.begin();
    auto old = it++;
    check((*old).first == 1 && (*old).second == 4 && (*it).first == 2, "练习 3：old = it++ 之后，*old 是 (1, 4)，*it 是 (2, 5)");

    // 改动（补充）：y 也是引用，改 y 改到 b
    V e{1, 2};
    V f{10, 20};
    for (auto [x, y] : Twin{e, f})
    {
        y -= x;
    }
    check(f == V{9, 18}, "练习 3：改 y 也会改到第二个容器");
}

int main()
{
    test_exercise1();
    std::println("");
    test_exercise2();
    std::println("");
    test_exercise3();
}

// ============================================================
// 思考题（每个小问都单独回答一行，每一行都要说“为什么”。先看清问句问的是哪个词：是什么 / 为什么 / 存在哪 / 会怎样）
// （1）关于“以短的为准”
//   (a) 书上 zip_iterator 的 == 用的是 ||。如果换成 &&，两个容器一个 3 个、一个 1 个，会怎样？为什么？
// 你的答案：出现未定义行为。
// 改动（补充）：结论对，“为什么”没写：&& 要两边同时到结尾才算相等。短的那个（1 个）走一步就到了 end，
//   但长的还没到，&& 是 false，循环继续：下一轮 *it 会对短的那个的 end 解引用（读不存在的元素），
//   ++ 又让它越过 end 继续往后走，永远不会再等于 end。读 end、越过 end 都是未定义行为。
//   (b) 示例 [4] 和你的 Adjacent 没有把 || 写在“迭代器之间的 ==”里，而是写在“和哨兵比较的 ==”里。
//       为什么？（提示：示例 [3] 说书上的 == 会让 (位置 0, 位置 5) == (位置 0, 位置 9) 是 true，这有什么不对？）
// 你的答案：没理解问题。
// 改动（补充）：问题是：|| 放在“两个迭代器之间的 ==”里，有什么坏处？
//   “相等”有一条基本的规矩：a == b 并且 b == c，那么 a == c（叫传递性）。书上的 == 不满足：
//     x = (位置 0, 位置 5)，y = (位置 0, 位置 9)，z = (位置 3, 位置 9)
//     x == y：第一个位置一样，true；y == z：第二个位置一样，true；但 x == z：两个都不一样，false
//   标准库的算法默认 == 是正常的（forward_iterator 要求 == 是真正的相等），比如用 == 判断“两个迭代器是同一个位置”，
//   这种 == 会让算法得到错的结论。
//   而“到头了没有”只在和 end 比较时需要，所以把 || 放到和哨兵比较的 == 里：
//   迭代器之间的 == 老老实实比较位置，哨兵的 == 负责“任何一个到头了就停”。两件事分开，各自都对。
//
// （2）关于引用
//   (a) 练习 3 里，for (auto [x, y] : Twin{a, b}) 修改 x，修好前 a 不变、修好后 a 变了。
//       auto 明明是“复制”，为什么修好后改 x 就改到了 a？（提示：auto 复制的是什么？复制品里装着什么？）
// 你的答案：是它的复制品。
// 改动（补充）：只说对了一半。auto 复制的是 *it 返回的那个 pair，复制品是一个新的 pair，这点对。
//   关键是复制品里装着什么：
//     修好前：pair<int, int>，里面装的是两个 int 的复制品，改 x 改的是复制品里的 int，a 不变
//     修好后：pair<int&, int&>，里面装的是两个引用。复制一个 pair 时，引用照样指向原来的元素，
//            所以复制品里的 x 仍然是 a[i] 的别名，改 x 就是改 a[i]
//   一句话：auto 复制的是“盒子”，盒子里装的是引用时，复制出来的盒子里还是同样的引用。
//   views::zip 也是这样：它返回 tuple<int&, string&>，所以示例 [5] 里 auto [id, name] 能改到原来的元素。
//   (b) 示例 [4] 的 ZipIt 和你的 AdjIt，iterator_concept 写 forward，iterator_category 却写 input。为什么？
// 你的答案：因为 std::pair<std::iter_reference_t<I>, std::iter_reference_t<I>>是代理引用，pair并不是引用，不符合传统的前向迭代器的定义。
// 改动（补充）：答得很好，“代理引用”这个词也用对了。补一句为什么新的一套可以写 forward：
//   C++20 的 forward_iterator 不要求 *it 是真正的引用，只看操作（能多次遍历、== 正常、能默认构造），所以写 forward 没问题；
//   老规矩（C++17 的 forward iterator）要求 *it 的类型是 value_type&，pair 临时对象不是，只能声明 input。
//
// （3）关于 views::zip
//   (a) 示例 [5]：views::zip(vector, list) 只是 bidirectional，不是 random_access。为什么 zip 的类别是“最弱的那个”？
// 你的答案：强的迭代器支持弱的特性。
// 改动（补充）：这句话本身对，但没回答“为什么 zip 只能取最弱的”。要从 zip 的操作想：
//   zip 迭代器做 --，就要对里面每个迭代器都 --；做 it + n，就要对每个都 + n。
//   list 的迭代器不能 + n，所以 zip 也没法 + n，只能一步一步走；vector 能做的再多也用不上。
//   每个操作都要“所有成员都能做”才行，所以 zip 能做的就是所有成员都能做的那些，也就是最弱的那个。
//   (b) views::enumerate(r) 相当于 views::zip(views::iota(0), r)。iota(0) 是无限长的，为什么这样配对不会停不下来？
// 你的答案：以短的为准。
// 改动（补充）：对，再说一步：zip 和 end 比较时，任何一个成员到了自己的 end 就算结束（示例 [4] 的 || 放在哨兵里）。
//   iota(0) 永远到不了 end，但 r 会到，r 到头时整个 zip 就停了。所以只要有一个是有限的，zip 就是有限的。
//   反过来，zip 两个无限的（比如 iota(0) 和 iota(100)）就真的停不下来，要再接一个 views::take。
//
// ============================================================
