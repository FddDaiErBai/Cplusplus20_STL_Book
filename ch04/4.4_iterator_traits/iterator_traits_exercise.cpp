// 第 4 章 4.4 让迭代器和标准库兼容 —— 练习
// 编译：clang++ -std=c++23 -Wall -Wextra -Wpedantic iterator_traits_exercise.cpp -o iterator_traits_exercise.exe
//       g++     -std=c++23 -Wall -Wextra -Wpedantic iterator_traits_exercise.cpp -o iterator_traits_exercise.exe -lstdc++exp
// 运行时检查：补完 TODO，编译运行，所有行都显示“通过”就说明做对了。
// 先看示例 iterator_traits.cpp 开头的表（要加哪些东西），以及 [3] 的完整版 Seq。
#include <algorithm>
#include <cstddef>
#include <deque>
#include <forward_list>
#include <iterator>
#include <list>
#include <print>
#include <ranges>
#include <sstream>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

void check(bool ok, std::string_view name)
{
    std::println("{} {}", ok ? "通过" : "失败", name);
}

using V = std::vector<int>;

// 用 range-for 收集，最多 100 个（防止停不下来）
template <typename R>
V collect(const R& r)
{
    V out;
    for (int x : r)
    {
        if (out.size() == 100)
        {
            break;
        }
        out.push_back(x);
    }
    return out;
}

// 老的一套（iterator_traits）认为 It 至少是 Tag 这个类别吗？
//   iterator_traits 打听不到类别时，这里是 false，不会编译错误
template <typename It, typename Tag>
concept legacy_at_least = requires { typename std::iterator_traits<It>::iterator_category; }
                          && std::derived_from<typename std::iterator_traits<It>::iterator_category, Tag>;

// ============================================================
// 练习 1：倒计时 Countdown{n} 生成 n, n-1, ..., 1
//   下面的迭代器是 4.3 那种写法，只能用在 range-for 里。
//   请照着示例 [3] 的完整版 Seq，把它改成两套（老算法、C++20）都能用的 forward 迭代器：
//     ① 两个标签：iterator_concept、iterator_category，都是 std::forward_iterator_tag
//     ② value_type、difference_type
//     ③ 默认构造函数
//     ④ 后置 it++：返回加之前的旧值（注意这里“加”其实是 --value_，往下数）
//     ⑤ 把 operator!= 换成 operator==
//   改 iterator 里面就行，Countdown 本身不用改。
//   下面的 helper 用 if constexpr 判断“能不能用”，不能用就返回 -1 或空的，所以这个文件现在也能编译。helper 不用改。
// ============================================================
class Countdown
{
    int from_{};

public:
    explicit Countdown(int from)
        : from_{from < 0 ? 0 : from}
    {
    }

    class iterator
    {
        int value_{};

    public:
        using iterator_concept = std::forward_iterator_tag;
        using iterator_category = std::forward_iterator_tag;
        using value_type = int;
        using difference_type = std::ptrdiff_t;

        iterator() = default;

        explicit iterator(int v)
            : value_{v}
        {
        }

        int operator*() const
        {
            return value_;
        }

        iterator& operator++()
        {
            --value_;
            return *this;
        }

        iterator operator++(int)
        {
            iterator old = *this;
            ++*this;
            return old;
        }

        bool operator==(const iterator&) const = default;
    };

    iterator begin() const
    {
        return iterator{from_};
    }

    iterator end() const
    {
        return iterator{0};
    }
};

// 老算法 std::minmax_element，返回 {最小, 最大}
template <typename R>
std::pair<int, int> old_minmax(const R& r)
{
    using It = decltype(r.begin());
    if constexpr (legacy_at_least<It, std::forward_iterator_tag>)
    {
        auto [a, b] = std::minmax_element(r.begin(), r.end());
        if (a == r.end())
        {
            return {0, 0};
        }
        return {*a, *b};
    }
    return {-1, -1};
}

// 老的一套：vector 的 (first, last) 构造函数
template <typename R>
V old_to_vector(const R& r)
{
    using It = decltype(r.begin());
    if constexpr (legacy_at_least<It, std::input_iterator_tag>)
    {
        return V(r.begin(), r.end());
    }
    return {};
}

// 新的一套：std::ranges::max
template <typename R>
int new_max(const R& r)
{
    if constexpr (std::ranges::forward_range<const R>)
    {
        return std::ranges::max(r);
    }
    return -1;
}

// 新的一套：views::filter 取偶数
template <typename R>
V new_evens(const R& r)
{
    if constexpr (std::ranges::forward_range<const R>)
    {
        return r | std::views::filter([](int x) { return x % 2 == 0; }) | std::ranges::to<V>();
    }
    return {};
}

// 后置 it++ 返回的是不是旧值：对 Countdown{5}，old = it++ 之后，*old 应该是 5，*it 应该是 4
template <typename R>
bool postfix_ok(const R& r)
{
    auto it = r.begin();
    if constexpr (requires { { it++ } -> std::same_as<decltype(it)>; })
    {
        auto old = it++;
        return *old == 5 && *it == 4;
    }
    return false;
}

void test_exercise1()
{
    using It = Countdown::iterator;
    check(collect(Countdown{5}) == V{5, 4, 3, 2, 1}, "练习 1：range-for 照样能用");
    check(std::forward_iterator<It>, "练习 1：std::forward_iterator（新的一套）");
    check(legacy_at_least<It, std::forward_iterator_tag>, "练习 1：iterator_traits 认为是 forward（老的一套）");
    check(postfix_ok(Countdown{5}), "练习 1：后置 it++ 返回旧值");
    check(old_minmax(Countdown{5}) == std::pair{1, 5}, "练习 1：std::minmax_element");
    check(old_to_vector(Countdown{4}) == V{4, 3, 2, 1}, "练习 1：vector(first, last)");
    check(new_max(Countdown{7}) == 7, "练习 1：std::ranges::max");
    check(new_evens(Countdown{7}) == V{6, 4, 2}, "练习 1：views::filter");
    check(collect(Countdown{0}).empty() && old_to_vector(Countdown{-3}).empty() && old_minmax(Countdown{0}) == std::pair{0, 0},
          "练习 1：Countdown{0}、Countdown{-3} 是空的");
}

// ============================================================
// 练习 2：标签分派 —— 自己写 std::advance（C++20 之前标准库就是这么写的，示例 [5]）
//   my_advance(it, n)：把迭代器 it 往后挪 n 步（n 是负数就往前挪）
//   my_advance 已经写好了：它从 iterator_traits 打听类别标签，造一个标签对象，交给 advance_impl 的重载去选：
//
//     advance_impl(it, n, input_iterator_tag)          ：只能往前走。n >= 0 就 ++it 走 n 次；n < 0 什么都不做
//     advance_impl(it, n, bidirectional_iterator_tag)  ：n >= 0 就 ++it 走 n 次；n < 0 就 --it 走 -n 次
//     advance_impl(it, n, random_access_iterator_tag)  ：一步到位 it += n;
//
//   每个版本一开始把 last_used 设成自己的名字（"input" / "bidirectional" / "random_access"），检查时看选中了哪个。
//   现在只有 input 版本（函数体也还没写），所以所有迭代器都会走到它。
//   TODO：
//     1. 写 input 版本的函数体
//     2. 在 input 版本下面、my_advance 上面，加另外两个重载（顺序要紧：my_advance 只能看见写在它前面的重载）
//   注意：不要写 forward_iterator_tag 的版本，看看 forward_list 会被分到哪里（思考题 2）
// ============================================================
std::string_view last_used = "";

template <typename It>
void advance_impl(It& it, std::ptrdiff_t n, std::input_iterator_tag)
{
    last_used = "input";
    if (n >= 0)
    {
        for (std::ptrdiff_t count{}; count != n; ++count, ++it)
        {}
    }
}

// TODO：bidirectional_iterator_tag 和 random_access_iterator_tag 两个版本写在这里

template <typename It>
void advance_impl(It& it, std::ptrdiff_t n, std::bidirectional_iterator_tag)
{
    last_used = "bidirectional";
    while (n > 0)
    {
        ++it;
        --n;
    }

    while (n < 0)
    {
        --it;
        ++n;
    }
}

template <typename It>
void advance_impl(It& it, std::ptrdiff_t n, std::random_access_iterator_tag)
{
    last_used = "random_access";
    it += n;
}

template <typename It>
void my_advance(It& it, std::ptrdiff_t n)
{
    advance_impl(it, n, typename std::iterator_traits<It>::iterator_category{});
}

void test_exercise2()
{
    std::forward_list<int> fl{1, 2, 3, 4, 5};
    auto f = fl.begin();
    my_advance(f, 3);
    check(*f == 4 && last_used == "input", "练习 2：forward_list 往后 3 步 → 4，用 input 版本");
    my_advance(f, -2);
    check(*f == 4, "练习 2：forward_list 不能往回走，n < 0 什么都不做");

    std::list<int> l{1, 2, 3, 4, 5};
    auto li = l.begin();
    my_advance(li, 4);
    check(*li == 5 && last_used == "bidirectional", "练习 2：list 往后 4 步 → 5，用 bidirectional 版本");
    my_advance(li, -3);
    check(*li == 2 && last_used == "bidirectional", "练习 2：list 往回 3 步 → 2");

    std::vector<int> v{10, 20, 30, 40, 50};
    auto vi = v.begin();
    my_advance(vi, 4);
    check(*vi == 50 && last_used == "random_access", "练习 2：vector 往后 4 步 → 50，用 random_access 版本");
    my_advance(vi, -3);
    check(*vi == 20 && last_used == "random_access", "练习 2：vector 往回 3 步 → 20");

    std::deque<int> d{7, 8, 9};
    auto di = d.begin();
    my_advance(di, 2);
    check(*di == 9 && last_used == "random_access", "练习 2：deque → random_access 版本");

    int arr[]{1, 2, 3, 4};
    int* p = arr;
    my_advance(p, 3);
    check(*p == 4 && last_used == "random_access", "练习 2：原生指针 → random_access 版本");

    std::istringstream in{"10 20 30"};
    std::istream_iterator<int> ii{in};
    my_advance(ii, 2);
    check(*ii == 30 && last_used == "input", "练习 2：istream_iterator → input 版本");

    auto zero = v.begin();
    my_advance(zero, 0);
    check(*zero == 10, "练习 2：走 0 步不动");

    // 改动（补充）：自己写的迭代器也能走标签分派：练习 1 的 Countdown 标签是 forward，没有 forward 版本，分到 input
    Countdown cd{5};
    auto ci = cd.begin();
    my_advance(ci, 2);
    check(*ci == 3 && last_used == "input", "练习 2：Countdown（forward 标签）往后 2 步 → 3，用 input 版本");
}

// ============================================================
// 练习 3：找 bug
//   Odds{limit} 生成小于 limit 的所有正奇数：Odds{10} → 1 3 5 7 9
//   它写了 -- ，本来想做成双向迭代器，可以倒着遍历。但是运行后：
//     * bidirectional_iterator 是 false，views::reverse 用不了，std::prev 也用不了
//     * 后置 it-- 的检查失败
//   一共两个问题（其中一个要改两行），都在 iterator 里面。
//   提示：示例 [6] —— 类别最后由谁说了算？后置 it-- 应该返回减之前的值还是减之后的？对照一下后置 it++
// ============================================================
class Odds
{
    int last_{};

public:
    explicit Odds(int limit)
        : last_{std::max(1, limit % 2 != 0 ? limit : limit + 1)} // 第一个 >= limit 的奇数，至少是 1
    {
    }

    class iterator
    {
        int value_{};

    public:
        using iterator_concept = std::bidirectional_iterator_tag;
        using iterator_category = std::bidirectional_iterator_tag;
        using value_type = int;
        using difference_type = std::ptrdiff_t;

        iterator() = default;

        explicit iterator(int v)
            : value_{v}
        {
        }

        int operator*() const
        {
            return value_;
        }

        iterator& operator++()
        {
            value_ += 2;
            return *this;
        }

        iterator operator++(int)
        {
            iterator old = *this;
            ++*this;
            return old;
        }

        iterator& operator--()
        {
            value_ -= 2;
            return *this;
        }

        iterator operator--(int)
        {
            iterator old = *this;
            --*this;
            return old;
        }

        bool operator==(const iterator&) const = default;
    };

    iterator begin() const
    {
        return iterator{1};
    }

    iterator end() const
    {
        return iterator{last_};
    }
};

// 新的一套：倒着收集
template <typename R>
V reversed(const R& r)
{
    if constexpr (std::ranges::bidirectional_range<const R>)
    {
        return r | std::views::reverse | std::ranges::to<V>();
    }
    return {};
}

// 老的一套：std::prev(end) 取最后一个
template <typename R>
int old_last(const R& r)
{
    using It = decltype(r.begin());
    if constexpr (legacy_at_least<It, std::bidirectional_iterator_tag>)
    {
        return *std::prev(r.end());
    }
    return -1;
}

void test_exercise3()
{
    check(collect(Odds{10}) == V{1, 3, 5, 7, 9} && collect(Odds{9}) == V{1, 3, 5, 7}, "练习 3：往前遍历");
    check(collect(Odds{1}).empty() && collect(Odds{0}).empty() && collect(Odds{2}) == V{1}, "练习 3：边界");
    check(std::bidirectional_iterator<Odds::iterator>, "练习 3：std::bidirectional_iterator（新的一套）");
    check(reversed(Odds{10}) == V{9, 7, 5, 3, 1}, "练习 3：views::reverse → 9 7 5 3 1");
    check(old_last(Odds{10}) == 9 && old_last(Odds{8}) == 7, "练习 3：std::prev(end)（老的一套）");

    Odds r{10};
    auto it = std::ranges::next(r.begin(), 2); // 指向 5
    auto old = it--;
    check(*old == 5 && *it == 3, "练习 3：old = it-- 之后，old 是 5，it 是 3");

    // 改动（补充）：修好的 Odds 标签是 bidirectional，练习 2 的 my_advance 能从 end() 往回走
    auto back = r.end(); // Odds{10} 的 end() 存的是 11
    my_advance(back, -2);
    check(*back == 7 && last_used == "bidirectional", "练习 3：my_advance(end, -2) → 7，用 bidirectional 版本");
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
// 思考题
// （1）练习 1 做完以后，只把 iterator() = default; 这一行注释掉，编译运行，哪几个检查从“通过”变成了“失败”？
//      由此说说：默认构造是老的一套要的，还是新的一套要的？（看完记得改回来）
// 你的答案：：std::forward_iterator（新的一套）std::ranges::max views::filter 失败了。新一套需要默认构造。
// 改动（补充）：对，和我跑出来的一样，就是这 3 个。补上为什么老的一套没反应：
//   老的一套根本不检查：iterator_traits 只是把你写的 iterator_category 照抄过去，你写 forward 它就信 forward。
//   （其实 C++17 的标准文字里，forward 迭代器也要求能默认构造，但那只是写在纸上，编译器不检查。）
//   新的一套由编译器检查：forward_iterator 要求 std::regular（能复制、能比较、能默认构造），
//   少了默认构造，概念就是 false，于是 ranges::max、views::filter 都不接受它。
//   为什么 forward 要默认构造：算法里常要先声明一个“空的”迭代器，后面再赋值，比如 It best; ... best = it;
//
// （2）练习 2 里没有写 forward_iterator_tag 的版本，forward_list 却能用，它被分到了哪个版本？为什么？
//      vector 的标签是 random_access_iterator_tag，它其实也能转换成 bidirectional_iterator_tag 和 input_iterator_tag，
//      三个重载都能接受它，编译器为什么选了 random_access 那个？
//      提示：示例 [5] 的标签继承关系图
// 你的答案：被分到了input_iterator_tag，因为forward_iterator_tag是input_iterator_tag的子类，可以隐式转换成父类。
// 选择距离自己最近的父类。
// 改动（补充）：forward_list 对。vector 那一问说得再准一点：
//   vector 的标签本身就是 random_access_iterator_tag，和第三个重载“完全一样”，不需要任何转换；
//   另外两个要先把它转换成父类才能用。重载的规则是：完全一样 > 转成父类；都要转成父类时，越近的父类越好。
//   所以 vector 选 random_access，contiguous 标签（如果传进来）选 random_access，forward 标签选 input。
//   这也是为什么三个版本都写成“只管自己能做的事”：input 版本会被所有更强的迭代器当成后备。
//
// （3）后置 it++ 为什么返回 iterator（复制品）而不是 iterator&（引用）？
//      如果写成下面这样，会出什么问题？（可以放进练习 1 编译试试，看编译器说什么，再改回来）
//        iterator& operator++(int)
//        {
//            iterator old = *this;
//            ++*this;
//            return old;
//        }
// 你的答案： non-const lvalue reference to type 'iterator' cannot bind to a temporary of type 'iterator'
// 非const的左值引用不能绑定这个iterator类型的亡值。
// 改动（补充）：实际编译、把报错贴上来，这点很好。“亡值”也说对了：C++23 规定 return 一个局部变量时，
//   把它当成亡值（准备被移走的值），所以 iterator& 绑不上去，报错（g++ 的说法是 cannot bind ... to an rvalue）。
//   但这只是“编译器怎么拦住你”，题目问的“为什么要返回复制品”还差一步：
//   old 是函数里的局部变量，函数一返回它就销毁了。返回它的引用，调用者拿到的是一个指向已经销毁的东西的引用（悬空引用）。
//   我用 C++20 试过：两个编译器都只给警告，照样编译通过，运行时是未定义行为：
//     clang：warning: reference to stack memory associated with local variable 'old' returned
//     g++  ：warning: reference to local variable 'old' returned
//   C++23 的新规则顺带把它变成了编译错误。
//   而且“旧值”除了 old 之外哪儿都没有了（迭代器本身已经加过了），所以只能复制一份返回。
//   前置 ++it 可以返回 iterator&，是因为它返回的是 *this，迭代器本身在函数返回后还活着。
//
// ============================================================
