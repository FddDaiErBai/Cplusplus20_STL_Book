// 第 4 章 4.7 反向迭代器 —— 练习
// 编译：clang++ -std=c++23 -Wall -Wextra -Wpedantic reverse_iterators_exercise.cpp -o reverse_iterators_exercise.exe
//       g++     -std=c++23 -Wall -Wextra -Wpedantic reverse_iterators_exercise.cpp -o reverse_iterators_exercise.exe -lstdc++exp
// 运行时检查：补完 TODO，编译运行，所有行都显示“通过”就说明做对了。
// 先看示例 reverse_iterators.cpp 开头的图（base() 差一个位置），以及 [3]（reverse_iterator 怎么实现）、[4]（换回普通迭代器）。
#include <algorithm>
#include <cstddef>
#include <functional>
#include <iterator>
#include <list>
#include <print>
#include <ranges>
#include <string>
#include <string_view>
#include <vector>

void check(bool ok, std::string_view name)
{
    std::println("{} {}", ok ? "通过" : "失败", name);
}

using V = std::vector<int>;

// ============================================================
// 练习 1：自己写 std::reverse_iterator —— Rev<It>
//   照着示例 [3] 说的实现：只存一个普通迭代器 current_，
//     *r   ：复制一份 current_，--，返回它指向的元素      （看的是 current_ 的前一个）
//     ++r  ：--current_          --r ：++current_
//     r1 == r2：比较 current_（可以 = default）
//     base()：返回 current_（已经写好了）
//   TODO：
//     ① 类型别名：iterator_concept、iterator_category 都写 std::bidirectional_iterator_tag
//                 （只写了 ++、--，没写 +=、[]，所以就算 It 是随机访问的，Rev 也只能说自己是双向的）
//                 value_type = std::iter_value_t<It>;   difference_type = std::iter_difference_t<It>;
//     ② 默认构造函数
//     ③ std::iter_reference_t<It> operator*() const      返回类型照抄：It 的 *it 是什么类型，这里就是什么类型
//                                                         （vector 是 int&，所以能通过 Rev 修改元素）
//     ④ 前置 ++、后置 ++、前置 --、后置 --
//     ⑤ ==
//   make_rev 已经写好了。下面的 helper 用 if constexpr 判断 Rev 能不能用，helper 不用改。
// ============================================================
template <std::bidirectional_iterator It>
class Rev
{
    It current_{};

public:
    using iterator_category = std::bidirectional_iterator_tag;
    using iterator_concept = std::bidirectional_iterator_tag;
    using value_type = std::iter_value_t<It>;
    using difference_type = std::iter_difference_t<It>;

    Rev() = default;

    explicit Rev(It it)
        : current_{it}
    {
    }

    It base() const
    {
        return current_;
    }

    // 改动（补充）：TODO 注释写完可以删掉了（下面 erase_last 里那个也是）
    std::iter_reference_t<It> operator*() const
    {
        auto temp = current_;
        --temp;
        return *temp;
    }

    Rev& operator++()
    {
        --current_;
        return *this;
    }

    Rev operator++(int)
    {
        auto old = *this;
        ++*this;
        return old;
    }

    Rev& operator--()
    {
        ++current_;
        return *this;
    }

    Rev operator--(int)
    {
        auto old = *this;
        --*this;
        return old;
    }

    bool operator==(const Rev& other) const
    {
        return current_ == other.current_;
    }
};

template <std::bidirectional_iterator It>
Rev<It> make_rev(It it)
{
    return Rev<It>{it};
}

// 用 Rev 倒着收集：[make_rev(end), make_rev(begin)) 交给 vector 的 (first, last) 构造函数（老的一套）
template <typename R>
V reversed_old(const R& r)
{
    using It = std::ranges::iterator_t<const R>;
    if constexpr (std::bidirectional_iterator<Rev<It>>)
    {
        return V(make_rev(std::ranges::end(r)), make_rev(std::ranges::begin(r)));
    }
    return {};
}

// 新的一套：std::ranges::subrange 把一对迭代器变成一个范围，再 ranges::to
template <typename R>
V reversed_new(const R& r)
{
    using It = std::ranges::iterator_t<const R>;
    if constexpr (std::bidirectional_iterator<Rev<It>>)
    {
        return std::ranges::subrange(make_rev(std::ranges::end(r)), make_rev(std::ranges::begin(r))) | std::ranges::to<V>();
    }
    return {};
}

// 用 Rev 往回找 x，返回 Rev 的 base() 指向的元素（找不到返回 -1）
template <typename It>
int found_base(It first, It last, int x)
{
    if constexpr (std::bidirectional_iterator<Rev<It>>)
    {
        auto r = std::find(make_rev(last), make_rev(first), x);
        if (r == make_rev(first) || r.base() == last)
        {
            return -1;
        }
        return *r.base();
    }
    return -1;
}

// 通过 Rev 修改最后一个元素
template <typename It>
bool write_last(It last, int value)
{
    if constexpr (std::bidirectional_iterator<Rev<It>>)
    {
        if constexpr (requires(Rev<It> r) { *r = value; })
        {
            *make_rev(last) = value;
            return true;
        }
    }
    return false;
}

// 后置 ++、-- 和 ==：对 v = {1 2 3 4 5}，r = make_rev(end)，old = r++ 后 *old 是 5、*r 是 4；再 --r，回到 5
template <typename It>
bool ops_ok(It first, It last)
{
    if constexpr (std::bidirectional_iterator<Rev<It>>)
    {
        auto r = make_rev(last);
        auto old = r++;
        bool ok = *old == 5 && *r == 4;
        --r;
        return ok && *r == 5 && r.base() == last;
    }
    (void)first;
    return false;
}

template <typename It>
bool eq_ok(It first, It last)
{
    if constexpr (std::bidirectional_iterator<Rev<It>>)
    {
        return make_rev(first) == make_rev(first) && make_rev(first) != make_rev(last);
    }
    return false;
}

// 一个只有 -- 的自己写的双向迭代器（4.4 节的写法），*it 返回值，不是引用
class Up
{
    int start_{};
    int end_{};

public:
    Up(int start, int end)
        : start_{start}, end_{end}
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
            ++value_;
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
            --value_;
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
        return iterator{start_};
    }

    iterator end() const
    {
        return iterator{end_};
    }
};

void test_exercise1()
{
    using VIt = V::iterator;
    check(std::bidirectional_iterator<Rev<VIt>> && !std::random_access_iterator<Rev<VIt>>,
          "练习 1：Rev<vector::iterator> 是双向的（不是随机访问）");
    check(std::bidirectional_iterator<Rev<std::list<int>::iterator>> && std::bidirectional_iterator<Rev<int*>>,
          "练习 1：Rev<list::iterator>、Rev<int*> 也是双向的");

    V v{1, 2, 3, 4, 5};
    check(reversed_old(v) == V{5, 4, 3, 2, 1}, "练习 1：vector(Rev, Rev) → 5 4 3 2 1（老的一套）");
    check(reversed_new(std::list<int>{7, 8, 9}) == V{9, 8, 7}, "练习 1：subrange(Rev, Rev) | to → 9 8 7（新的一套）");
    int arr[]{10, 20, 30};
    check(reversed_old(arr) == V{30, 20, 10} && reversed_new(Up{1, 4}) == V{3, 2, 1}, "练习 1：原生数组、自己写的 Up");
    check(reversed_old(V{}).empty() && reversed_new(V{42}) == V{42}, "练习 1：空的 / 一个元素");

    check(found_base(v.begin(), v.end(), 3) == 4, "练习 1：往回找 3，base() 指向 4（差一个位置）");
    check(found_base(v.begin(), v.end(), 9) == -1, "练习 1：找不到");

    V w{1, 2, 3};
    check(write_last(w.end(), 99) && w == V{1, 2, 99}, "练习 1：*make_rev(w.end()) = 99 改的是最后一个元素");

    check(ops_ok(v.begin(), v.end()), "练习 1：后置 ++ 返回旧值；-- 往回（往 end 方向）走");
    check(eq_ok(v.begin(), v.end()), "练习 1：==、!=");

    // 改动（补充）：反两次就是正的：Rev<Rev<It>>。你的 Rev 写得通用，套两层也能用
    auto rr_first = make_rev(make_rev(v.end()));   // 外层的 base() 是 make_rev(v.end())，也就是 rbegin
    auto rr_last = make_rev(make_rev(v.begin()));
    check(V(rr_last, rr_first) == V{1, 2, 3, 4, 5}, "练习 1：Rev<Rev<It>> 反两次 → 1 2 3 4 5");
}

// ============================================================
// 练习 2：用 base() 把反向迭代器换回普通迭代器（示例 [4]）
//   不许用 std::ranges::find_last，也不许用 std::string_view 的 rfind / find_last_of 之类的成员函数，
//   要用 rbegin()/rend() + 算法 + base()
//
//   erase_last(v, x)：删掉 v 里最后一个 x，返回 true；没有 x 就什么都不做，返回 false
//     {1, 2, 3, 2, 1}, 2 → {1, 2, 3, 1}
//
//   last_word(s)：s 里用空格分开的最后一个单词（s 只含字母和空格）
//     "hello  world  " → "world"      "one" → "one"      "   " → ""      "" → ""
//   思路：两次反向查找
//     r1 = 从后往前第一个“不是空格”的字符            （单词的最后一个字母）
//     r2 = 从 r1 开始往前，第一个“是空格”的字符       （单词前面的空格，可能找不到 → rend）
//         "hello  world  "
//                ^    ^
//                r2   r1
//     单词就是普通迭代器的区间 [r2.base(), r1.base())。想一想为什么这里不用 prev
//     std::string_view 可以用两个迭代器构造：std::string_view(first, last)
// ============================================================
bool erase_last(V& v, int x)
{
    // TODO
    auto pos = std::find(v.rbegin(), v.rend(), x);
    if (pos != v.rend())
    {
        v.erase(std::prev(pos.base()));
        return true;
    }
    return false;
}

std::string_view last_word(std::string_view s)
{
    auto pos_1 = std::find_if(s.rbegin(), s.rend(), [](char c)
    {
        return c != ' ';
    });
    auto pos_2 = std::find_if(pos_1, s.rend(), [](char c)
    {
        return c == ' ';
    });
    return std::string_view{pos_2.base(), pos_1.base()};
}

void test_exercise2()
{
    V v{1, 2, 3, 2, 1};
    check(erase_last(v, 2) && v == V{1, 2, 3, 1}, "练习 2：erase_last({1 2 3 2 1}, 2) → 1 2 3 1");
    check(erase_last(v, 1) && v == V{1, 2, 3}, "练习 2：再删最后一个 1 → 1 2 3");
    check(!erase_last(v, 9) && v == V{1, 2, 3}, "练习 2：没有 9，什么都不做，返回 false");
    V one{5};
    V empty;
    check(erase_last(one, 5) && one.empty() && !erase_last(empty, 5), "练习 2：只有一个元素 / 空的");

    check(last_word("hello  world  ") == "world", "练习 2：last_word(\"hello  world  \") → \"world\"");
    check(last_word("hello world") == "world" && last_word("one") == "one", "练习 2：末尾没有空格 / 只有一个词");
    check(last_word("  a") == "a" && last_word("a  ") == "a", "练习 2：一个字母");
    check(last_word("   ").empty() && last_word("").empty(), "练习 2：全是空格 / 空的 → \"\"");
    // 改动（补充）：多个单词，确认 r2 停在最后一个单词前面那个空格，没有一直找到开头
    check(last_word("a bc def") == "def" && last_word(" x  yz") == "yz", "练习 2：多个单词");
}

// ============================================================
// 练习 3：找 bug（两个函数各有一个 bug，每个只改一行。只改这两处，别的地方不要动）
//   trim_right(s)：删掉 s 末尾的空格：  "abc  " → "abc"，"a b " → "a b"，"   " → ""
//   sort_desc(v) ：从大到小排序：        {3, 1, 2} → {3, 2, 1}
//   提示：trim_right —— 示例开头的图：要删的是 [第一个空格, end)。r.base() 指向哪里？
//         sort_desc  —— 示例 [6]：sort(rbegin, rend) 为什么是从大到小？这里反了几次？
// ============================================================
void trim_right(std::string& s)
{
    auto r = std::find_if(s.rbegin(), s.rend(), [](char c) { return c != ' '; }); // 从后往前第一个不是空格的
    if (r == s.rend())
    {
        s.clear(); // 全是空格（或者是空的）
        return;
    }
    s.erase(r.base(), s.end());
}

void sort_desc(V& v)
{
    std::ranges::sort(v | std::views::reverse);
}

void test_exercise3()
{
    std::string a{"abc  "};
    std::string b{"a b "};
    std::string c{"   "};
    std::string d{"xyz"};
    trim_right(a);
    trim_right(b);
    trim_right(c);
    trim_right(d);
    check(a == "abc" && b == "a b", "练习 3：trim_right(\"abc  \") → \"abc\"，\"a b \" → \"a b\"");
    check(c.empty() && d == "xyz", "练习 3：trim_right 全是空格 / 末尾没有空格");

    V v{3, 1, 2};
    sort_desc(v);
    check(v == V{3, 2, 1}, "练习 3：sort_desc({3 1 2}) → 3 2 1");
    V w{5, 5, 1, 9};
    sort_desc(w);
    check(w == V{9, 5, 5, 1}, "练习 3：sort_desc({5 5 1 9}) → 9 5 5 1");
}

// ============================================================
// 思考题 2 用：运行后看输出（不用改）
// ============================================================
void q2_demo()
{
    V v{1, 2, 3, 2, 5};
    auto r = std::find(v.rbegin(), v.rend(), 2);
    v.erase(r.base()); // 想删最后一个 2
    std::println("思考题 2：{{1 2 3 2 5}} 用 v.erase(r.base()) 之后：{}", v);
}

int main()
{
    test_exercise1();
    std::println("");
    test_exercise2();
    std::println("");
    test_exercise3();
    std::println("");
    q2_demo();
}

// ============================================================
// 思考题（每个小问都单独回答一行，每一行都要说“为什么”）
// （1）关于 base() 差一个位置
//   (a) v.rbegin().base() 为什么存的是 v.end()，而不是指向最后一个元素？
// 你的答案：C++ 允许迭代器指向最后一个元素的后面，但不允许迭代器越过第一个元素的前面，假如指向最后一个元素，那rend()不就是非法位置了吗？
//   (b) 如果不错开（current_ 直接指向看到的元素），v.rend() 里要存什么？这个位置有什么问题？
// 你的答案：那rend()不就是非法位置了吗？
// 改动（补充）：方向对，(a) 答得很好，有因果。(b) 和 (a) 重复了，要具体说出“存什么”和“哪里非法”：
//   不错开的话，rend() 要表示“第一个元素的前面”，current_ 就得存 v.begin() - 1。
//   对 vector 迭代器，算出 begin() - 1 本身就是未定义行为（哪怕不去 * 它）；
//   对原生数组也一样，arr - 1 不是合法指针。list 的 begin() 再 -- 也是未定义行为。
//   而 end() 是标准专门保证的合法位置（“最后一个的后面”），所以错开一个，两头都落在合法位置上。
//
// （2）运行后看“思考题 2”那一行。
//   (a) 想删最后一个 2，实际删掉的是哪个数？为什么？
// 你的答案：5，base()指向的是看到的元素的后面一个。
//   (b) 如果 v 里根本没有 2，r 等于什么？r.base() 等于什么？这时候写 v.erase(std::prev(r.base())) 会怎样？
// 你的答案：rend()，r.base()=v.begin()，未定义行为。
// 改动（补充）：三个都对。补一句为什么：std::prev(v.begin()) 就是 (b) 里说的“第一个元素的前面”，
//   算出它就已经是未定义行为，再 erase 更不用说。所以练习 2 里你先判断 pos != v.rend() 是必须的，写得对。
//
// （3）关于练习 2 的 last_word 和练习 3 的 trim_right
//   (a) last_word 里单词是 [r2.base(), r1.base())，为什么两边都不用 prev？
//       （提示：r1.base() 指向 r1 看到的字母的“后一个”，作为区间的结尾正好是什么？r2.base() 呢？）
// 你的答案：区间本来就是左闭右开。
// 改动（补充）：方向对，但要说出两头各自为什么正好对上：
//   右边：r1 看到单词的最后一个字母，r1.base() 指向它的后一个。左闭右开的“右开”要的正是“最后一个的后一个”，
//         所以 r1.base() 直接当结尾，不用 prev。
//   左边：r2 看到单词前面的空格，r2.base() 指向空格的后一个，就是单词的第一个字母，正好是“左闭”要的开头。
//         如果前面没有空格（"one"），r2 == rend()，r2.base() 是 begin()，也正好是第一个字母。
//   反过来，erase_last 要的是“r 看到的那个元素本身”，不是区间的端点，所以那里要 prev。
//   (b) std::sort(l.rbegin(), l.rend())，l 是 std::list<int>，能编译吗？为什么？（可以写一行试试，再删掉）
// 你的答案：编译报错了
// 改动（补充）：试了是好的，但没说为什么。std::sort 要随机访问迭代器：它要算 last - first、跳到中间（4.1 节思考题 1）。
//   反向迭代器的类别和里面的迭代器一样：list 的迭代器是双向的，reverse_iterator<list::iterator> 也只是双向的，
//   没有 - 运算。两个编译器的报错都指向这个：
//     clang：invalid operands to binary expression ('std::reverse_iterator<std::__list_iterator<int, void *>>' and ...)
//     g++  ：no match for 'operator-' (operand types are 'std::reverse_iterator<std::_List_iterator<int> >' and ...)
//   反过来，vector 的 rbegin 是随机访问的，所以示例 [6] 的 sort(v.rbegin(), v.rend()) 能用。
//   list 从大到小排序：l.sort(std::greater{});
//
// ============================================================
