// 第 4 章 4.5 迭代器适配器 —— 练习
// 编译：clang++ -std=c++23 -Wall -Wextra -Wpedantic iterator_adapters_exercise.cpp -o iterator_adapters_exercise.exe
//       g++     -std=c++23 -Wall -Wextra -Wpedantic iterator_adapters_exercise.cpp -o iterator_adapters_exercise.exe -lstdc++exp
// 运行时检查：补完 TODO，编译运行，所有行都显示“通过”就说明做对了。
// 先看示例 iterator_adapters.cpp 开头的表，以及 [2]（back_inserter 的几个运算符）、[4]（inserter）、[6][7][8]（流迭代器）。
#include <algorithm>
#include <cstddef>
#include <deque>
#include <iterator>
#include <list>
#include <print>
#include <ranges>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

void check(bool ok, std::string_view name)
{
    std::println("{} {}", ok ? "通过" : "失败", name);
}

using V = std::vector<int>;

// ============================================================
// 练习 1：插入迭代器
//   每个函数的函数体只写一个算法调用（加上 return），不要写循环，不要直接调用 push_back / push_front / insert。
//   可以用的算法：std::copy、std::copy_if、std::transform，或者它们的 std::ranges:: 版本
//
//   evens_appended(dst, src)        ：把 src 里的偶数按顺序加到 dst 后面，返回 dst
//                                     （dst 是按值传进来的复制品，改它不影响调用者）
//   reversed_copy(src)              ：返回一个 deque，元素是 src 倒过来。不许用 std::reverse、views::reverse、rbegin
//                                     提示：示例 [3] 里哪个插入迭代器会把顺序弄反？
//   unique_sorted(src)              ：返回一个 set，里面是 src 去重、排好序的结果（示例 [4]）
//   squares_inserted(v, pos, src)   ：在 v 的第 pos 个位置插入 src 里每个数的平方（顺序不变），返回 v
//                                     例：v = {1, 2, 3}，pos = 1，src = {4, 5} → {1, 16, 25, 2, 3}
// ============================================================
V evens_appended(V dst, const V& src)
{
    std::ranges::copy_if(src, std::back_inserter(dst), [](int x)
    {
        return x % 2 == 0;
    });
    return dst;
}

std::deque<int> reversed_copy(const V& src)
{
    std::deque<int> out;
    std::ranges::copy(src, std::front_inserter(out));
    return out;
}

std::set<int> unique_sorted(const V& src)
{
    std::set<int> out;
    std::ranges::copy(src, std::inserter(out, out.end()));
    return out;
}

V squares_inserted(V v, std::size_t pos, const V& src)
{
    auto pos_iterator = v.begin() + static_cast<std::ptrdiff_t>(pos);
    std::ranges::transform(src, std::inserter(v, pos_iterator), [](int x)
    {
        return x * x;
    });
    return v;
}

void test_exercise1()
{
    check(evens_appended(V{9}, V{1, 2, 3, 4, 6}) == V{9, 2, 4, 6}, "练习 1：evens_appended({9}, {1 2 3 4 6}) → 9 2 4 6");
    check(evens_appended(V{}, V{1, 3}).empty() && evens_appended(V{5}, V{}) == V{5}, "练习 1：evens_appended 没有偶数 / src 是空的");

    check(reversed_copy(V{1, 2, 3}) == std::deque<int>{3, 2, 1}, "练习 1：reversed_copy({1 2 3}) → 3 2 1");
    check(reversed_copy(V{}).empty(), "练习 1：reversed_copy 空的");

    check(unique_sorted(V{3, 1, 4, 1, 5, 9, 2, 6, 5}) == std::set<int>{1, 2, 3, 4, 5, 6, 9}, "练习 1：unique_sorted");
    check(unique_sorted(V{}).empty(), "练习 1：unique_sorted 空的");

    check(squares_inserted(V{1, 2, 3}, 1, V{4, 5}) == V{1, 16, 25, 2, 3}, "练习 1：squares_inserted 插在中间");
    check(squares_inserted(V{1, 2}, 0, V{3}) == V{9, 1, 2} && squares_inserted(V{1, 2}, 2, V{3}) == V{1, 2, 9},
          "练习 1：squares_inserted 插在最前、最后");
}

// ============================================================
// 练习 2：流迭代器
//   parse_ints(text)  ：把 text 里的整数读进 vector。碰到不是整数的东西就停：
//                         "3 1 4" → {3, 1, 4}，"1 2 x 3" → {1, 2}，"" → {}
//                         用 std::istringstream + std::istream_iterator<int>（示例 [7]）
//   word_count(text)  ：数 text 里有几个单词（用空白分开的）。"  to be\n or  not " → 4
//                         用 std::istream_iterator<std::string> 和 std::distance（4.1 节）
//   join(v, sep)      ：把 v 的元素用 sep 连起来，最后面不要多出 sep：
//                         join({1, 2, 3}, ", ") → "1, 2, 3"，join({7}, "-") → "7"，join({}, ", ") → ""
//                         用 std::ostringstream + std::ostream_iterator<int>（示例 [6]）。
//                         难点：ostream_iterator 会在每个元素后面都加 sep，包括最后一个。想想怎么让最后一个不加
//                         （不要先拼好再删掉末尾的 sep，也不要用 std::format 的 {:n}）
// ============================================================
V parse_ints(const std::string& text)
{
    V num{};
    std::istringstream txt{text};
    std::copy(std::istream_iterator<int>{txt}, std::istream_iterator<int>{}, std::back_inserter(num));
    return num;
}

std::size_t word_count(const std::string& text)
{
    std::istringstream txt{text};
    std::istream_iterator<std::string> first{txt};
    std::istream_iterator<std::string> last{};
    return static_cast<std::size_t>(std::distance(first, last));
}

std::string join(const V& v, const char* sep)
{
    std::ostringstream os;
    if (!v.empty())
    {
        std::copy(v.begin(), std::prev(v.end()), std::ostream_iterator<int>(os, sep));
        os << v.back();
    }
    return os.str();
}

void test_exercise2()
{
    check(parse_ints("3 1 4") == V{3, 1, 4}, "练习 2：parse_ints(\"3 1 4\")");
    check(parse_ints("  -5\n10\t7 ") == V{-5, 10, 7}, "练习 2：parse_ints 负数和各种空白");
    check(parse_ints("1 2 x 3") == V{1, 2} && parse_ints("x 1").empty() && parse_ints("").empty(),
          "练习 2：parse_ints 碰到不是整数的就停 / 空字符串");

    check(word_count("  to be\n or  not ") == 4, "练习 2：word_count → 4");
    check(word_count("") == 0 && word_count("   \n ") == 0 && word_count("one") == 1, "练习 2：word_count 空的 / 全是空白 / 一个词");

    check(join(V{1, 2, 3}, ", ") == "1, 2, 3", "练习 2：join({1 2 3}, \", \") → \"1, 2, 3\"");
    check(join(V{7}, "-") == "7", "练习 2：join 一个元素，没有分隔符");
    check(join(V{}, ", ").empty(), "练习 2：join 空的 → \"\"");
    check(join(V{-1, 0, 10}, "") == "-1010", "练习 2：join 分隔符是空的");
}

// ============================================================
// 练习 3：自己写一个输出迭代器 Pusher<C>（照着示例 [2] 说的 back_insert_iterator 的几个运算符）
//   pusher(c) 造一个 Pusher，对它写 *it = x 就调用 c.push_back(x)。要求：
//     ① 成员：一个指向容器的指针 C* c_（已经写好了）
//     ② 类型别名：using difference_type = std::ptrdiff_t;      C++20 的 ranges::copy 要（示例 [11]）
//                  using iterator_category = std::output_iterator_tag;  老算法看的标签
//     ③ Pusher& operator=(const typename C::value_type& x)：调用 c_->push_back(x)，返回 *this
//     ④ operator*()：返回 *this（Pusher&），什么都不做
//     ⑤ 前置 ++ 和后置 ++：都返回 *this（Pusher&），什么都不做
//   现在是空壳：下面的 helper 用 if constexpr 判断 Pusher 能不能用，不能用就什么都不做，所以这个文件现在也能编译。
//   helper 不用改。
// ============================================================
template <typename C>
class Pusher
{
    C* c_;

public:
    using iterator_concept = std::output_iterator_tag; // 改动（补充）：这一行不需要。iterator_concept 只给 input 及以上的概念用，
                                                       //   output_iterator 根本不看它。不过写了也没坏处
    using iterator_category = std::output_iterator_tag;
    // 改动：原来是 using value_type = C;
    //   value_type 的意思是“*it 读出来的元素是什么类型”，不是“容器是什么类型”。写成 C，就是在说
    //   “从 Pusher<vector<int>> 能读出一个 vector<int>”，这是假的。新的一套会照抄它：
    //     std::iter_value_t<Pusher<V>> 得到 std::vector<int>（我两个编译器都试过）
    //   输出迭代器根本不能读，所以标准的 back_insert_iterator 写的是 value_type = void，
    //   这时 iter_value_t 根本不存在（下面新加的检查测的就是这个）。不写 value_type 也可以
    //   （老的 iterator_traits 这次没被骗到：你没写 reference，它就不读你的别名，自己推导成 void）
    using value_type = void;
    using difference_type = std::ptrdiff_t;

    explicit Pusher(C& c)
        : c_{&c}
    {
    }

    Pusher& operator=(const typename C::value_type& x)
    {
        c_->push_back(x);
        return *this;
    }

    Pusher& operator*()
    {
        return *this;
    }

    Pusher& operator++()
    {
        return *this;
    }

    Pusher& operator++(int)
    {
        return *this;
    }
};

template <typename C>
Pusher<C> pusher(C& c)
{
    return Pusher<C>{c};
}

// 改动（补充）：检查“iter_value_t<It> 存在吗”，用来测上面的 value_type
template <typename It>
concept has_iter_value = requires { typename std::iter_value_t<It>; };

// 新的一套：std::ranges::copy
template <typename C, typename R>
void new_copy(const R& src, C& dst)
{
    if constexpr (std::output_iterator<Pusher<C>, std::ranges::range_value_t<R>>)
    {
        std::ranges::copy(src, pusher(dst));
    }
}

// 老的一套：std::transform（每个数 ×10）
template <typename C, typename R>
void old_transform(const R& src, C& dst)
{
    if constexpr (std::output_iterator<Pusher<C>, std::ranges::range_value_t<R>>)
    {
        std::transform(src.begin(), src.end(), pusher(dst), [](const auto& x) { return x * 10; });
    }
}

// 手动用：*it++ = x
template <typename C>
void manual(C& dst)
{
    if constexpr (std::output_iterator<Pusher<C>, int>)
    {
        auto it = pusher(dst);
        *it++ = 1;
        ++it;
        *it = 2;
        it = 3;
    }
}

void test_exercise3()
{
    check(std::output_iterator<Pusher<V>, int>, "练习 3：std::output_iterator<Pusher<V>, int>");
    check(!std::input_iterator<Pusher<V>>, "练习 3：不是 input 迭代器（不能读）");

    V v{0};
    new_copy(V{1, 2, 3}, v);
    check(v == V{0, 1, 2, 3}, "练习 3：std::ranges::copy");

    std::list<int> l;
    old_transform(V{1, 2}, l);
    check(l == std::list<int>{10, 20}, "练习 3：std::transform 写进 list");

    V m;
    manual(m);
    check(m == V{1, 2, 3}, "练习 3：手动 *it++ = 1; ++it; *it = 2; it = 3;");

    // 改动（补充）：两个新检查
    check(!has_iter_value<Pusher<V>> && !has_iter_value<std::back_insert_iterator<V>>,
          "练习 3：和 back_insert_iterator 一样，没有 iter_value_t（输出迭代器不能读）");
    std::string str{"ab"};
    new_copy(std::string{"cd"}, str);
    check(std::output_iterator<Pusher<std::string>, char> && str == "abcd", "练习 3：Pusher<std::string> 也能用");
}

// ============================================================
// 练习 4：找 bug
//   下面两个函数各有一个 bug，运行后对应的检查会失败。找出原因并修好，每个只需要改一行。
//   prepend(d, front)：把 front 放到 d 的最前面，保持 front 原来的顺序：
//                       d = {1, 2}，front = {7, 8, 9} → {7, 8, 9, 1, 2}
//   char_count(text) ：数 text 一共有几个字符，空格、换行都算：
//                       "a b\nc" → 5
//   提示：示例 [3]、[4]、[8]
// ============================================================
void prepend(std::deque<int>& d, const V& front)
{
    std::ranges::copy(front, std::inserter(d, d.begin()));
}

std::size_t char_count(const std::string& text)
{
    std::istringstream in{text};
    return static_cast<std::size_t>(std::distance(std::istreambuf_iterator<char>{in}, std::istreambuf_iterator<char>{}));
}

void test_exercise4()
{
    std::deque<int> d{1, 2};
    prepend(d, V{7, 8, 9});
    check(d == std::deque<int>{7, 8, 9, 1, 2}, "练习 4：prepend → 7 8 9 1 2");
    std::deque<int> e;
    prepend(e, V{4, 5});
    check(e == std::deque<int>{4, 5}, "练习 4：prepend 到空的 deque");

    check(char_count("abc") == 3, "练习 4：char_count(\"abc\") → 3");
    check(char_count("a b\nc") == 5, "练习 4：char_count(\"a b\\nc\") → 5");
    check(char_count(" \n\t ") == 4 && char_count("") == 0, "练习 4：char_count 全是空白 / 空的");
}

// ============================================================
// 思考题 1 用：运行后看输出（不用改）
//   往空 vector 里加 1000 个数，数一数 capacity()（已经分配的位置数）变了几次，也就是重新分配了几次内存
// ============================================================
template <typename F>
int count_reallocs(F add_all)
{
    V v;
    int count = 0;
    auto cap = v.capacity();
    add_all(v, [&] {
        if (v.capacity() != cap)
        {
            ++count;
            cap = v.capacity();
        }
    });
    return count;
}

void q1_demo()
{
    const V src(1000, 7);
    int a = count_reallocs([&](V& v, auto after_each) {
        for (int x : src)
        {
            *std::back_inserter(v) = x; // 等于 v.push_back(x)
            after_each();
        }
    });
    int b = count_reallocs([&](V& v, auto after_each) {
        v.append_range(src); // C++23 之前写 v.insert(v.end(), src.begin(), src.end());
        after_each();
    });
    std::println("思考题 1：back_inserter 一个一个加：重新分配 {} 次；append_range：{} 次", a, b);
}

// ============================================================
// 思考题 3 用：运行后看输出（不用改）
// ============================================================
void q3_demo()
{
    std::istringstream in{"1 2 x 3"};
    V v(std::istream_iterator<int>{in}, std::istream_iterator<int>{});
    std::println("思考题 3：读到 {}，in.fail() 是 {}", v, in.fail());
}

int main()
{
    test_exercise1();
    std::println("");
    test_exercise2();
    std::println("");
    test_exercise3();
    std::println("");
    test_exercise4();
    std::println("");
    q1_demo();
    q3_demo();
}

// ============================================================
// 思考题
// （1）运行后看“思考题 1”那一行：两种写法重新分配内存的次数为什么差这么多？
//      append_range 是怎么做到只分配一次的？（提示：它拿到的是整个 src，事先知道什么？）
//      如果非要用 back_inserter，怎样也只分配一次？（提示：vector 有一个成员函数可以提前要好位置）
// 你的答案：没有重复申请资源。预先知道src需要申请分配多少资源。提前reserve。
// 改动（补充）：三点都对，“为什么差这么多”再说具体一点：
//   back_inserter 每次只给 vector 一个数，vector 不知道后面还有多少。位置满了就重新分配：
//   要一块更大的内存（两个库都是原来的 2 倍）、把旧元素全搬过去、释放旧的。
//   容量依次是 1 2 4 8 …… 512 1024，从 0 到 1024 正好变了 11 次，就是输出里的 11。
//   append_range 拿到的是整个 src，事先知道 src.size() = 1000，一次要够 1000 个位置，只分配 1 次。
//   用 back_inserter 时先 v.reserve(v.size() + src.size());，后面的 push_back 就不用再分配了。
//   注意：reserve 只要位置，不造元素，size() 不变；和 [1] 里 deque<int> d2(5) 先造 5 个 0 不一样。
//
// （2）你在练习 3 里写的 operator* 和 operator++ 什么都不做，只返回 *this。
//      std::copy 内部写的是 *out = *first; ++out;，把这两句里 out 是 Pusher 时每一步调用了你的哪个函数写出来，
//      说说为什么 * 和 ++ 可以什么都不做。
//      另外：4.4 思考题 3 说后置 it++ 要返回复制品、不能返回引用，这里后置 ++ 返回 Pusher&，为什么没问题？
// 你的答案：operator=和operator++()。
// 改动（补充）：少了一个 operator*，另外两个小问也没答。完整的过程：
//     *out = *first;   先算 *out：调用 operator*()，返回 out 自己（Pusher&）
//                      再对这个返回值 = *first：调用 operator=(const int&)，里面 push_back
//     ++out;           调用前置 operator++()，返回 out 自己，什么都不做
//   为什么 * 和 ++ 可以什么都不做：普通迭代器要靠 ++ 挪到下一个位置，下一次才写到新地方；
//   Pusher 根本没有“位置”，“写到哪”由 push_back 决定，永远是容器的末尾。每次 push_back 完，
//   末尾自动就往后挪了，所以 ++ 不需要做任何事。* 也一样，只是为了让 *out = x 能编译。
//   后置 ++ 返回 Pusher& 为什么没问题：4.4 那个错误是返回了局部变量 old 的引用，函数结束 old 就没了。
//   这里返回的是 *this，也就是调用者手里的 out 本身，函数返回后它还活着，引用不会悬空。
//   而且 Pusher 加之前和加之后完全一样，没有“旧值”需要单独保存，所以也不需要复制品。
//   （标准的 back_insert_iterator 后置 ++ 返回的也是引用）
//
// （3）运行后看“思考题 3”那一行：读 "1 2 x 3" 为什么只读到 1 和 2？
//      读到 x 的时候发生了什么，为什么这时候 istream_iterator 和结束迭代器 std::istream_iterator<int>{} 相等了？
//      （提示：示例 [7]，4.1 思考题 3 讲过 == 比较的是什么）
//      如果想跳过 x 接着读到 3，大概要对 in 做哪两件事？（不用写代码，说思路；可以查 clear 和 ignore）
// 你的答案：读到了不是int的东西。
// 改动（补充）：这只答了第一问的一半，后面两问没答。完整的过程：
//   读 1、读 2 都成功。下一次 in >> n 碰到 x，x 不能变成 int，读取失败，in 被设上 failbit（in.fail() 是 true），
//   x 还留在流里没被拿走。
//   istream_iterator 每次 ++ 读完都会检查流；发现读失败，就把自己存的流指针设成空的。
//   结束迭代器 std::istream_iterator<int>{} 的流指针本来就是空的，两个空指针一比较就相等了，循环停下。
//   所以 3 根本没机会读：不是读不到 3，是在 x 那里就已经“结束”了。
//   想接着读到 3 要做两件事：
//     1. in.clear()：清掉 failbit。流处于失败状态时，之后所有读取都直接失败
//     2. 把 x 拿掉：in.ignore(...) 跳过一些字符（或者读进一个 std::string 扔掉）
//   然后再造一个新的 istream_iterator<int>{in} 接着读。我试过 clear + ignore 到下一个空格，能读出 [1, 2, 3]。
//
// ============================================================
