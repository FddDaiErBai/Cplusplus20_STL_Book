// 第 4 章 4.7 反向迭代器：std::reverse_iterator、rbegin/rend、base()
// 编译：clang++ -std=c++23 -Wall -Wextra -Wpedantic reverse_iterators.cpp -o reverse_iterators.exe
//       g++     -std=c++23 -Wall -Wextra -Wpedantic reverse_iterators.cpp -o reverse_iterators.exe -lstdc++exp
//
// 文件结构：[1] 到 [7] 从上到下排列，每个编号只出现一次。
//   每一节里依次是：说明、这一节用到的类和函数、一个 sectionN() 函数（运行这一节）。main 依次调用 section1() 到 section7()。
//
// ============================================================
// 反向迭代器 std::reverse_iterator<It>（<iterator>）：把一个迭代器包起来，方向反过来：
//     对它 ++，实际是对里面的迭代器 --；对它 --，实际是 ++
//   怎么得到：
//     容器的成员函数    v.rbegin()、v.rend()（const 版本 v.crbegin()、v.crend()）
//     非成员函数        std::rbegin(c)、std::rend(c)：容器和原生数组都能用
//     自己包            std::make_reverse_iterator(it)
//   条件：里面的迭代器至少是双向的（要能 --）。forward_list 没有 rbegin（[5]）
//
//   最重要、也最容易错的一点（书上没讲，[3][4]）：
//     反向迭代器里存的那个迭代器（叫 base()），指向的是“它看到的元素”的后一个位置：
//
//       位置：      begin                                   end
//                    ↓                                       ↓
//       元素：      [1]     [2]     [3]     [4]     [5]
//                                                    ↑       ↑
//                                          *rbegin() 看到 5   rbegin().base() 存的是 end
//                    ↑
//       *?          rend().base() 存的是 begin（rend 看到的是 1 的前面，那里没有元素，不能 *）
//
//     规则：*r 等于 *std::prev(r.base())
// ============================================================
#include <algorithm>
#include <cstddef>
#include <forward_list>
#include <iterator>
#include <list>
#include <print>
#include <ranges>
#include <string>
#include <string_view>
#include <vector>

// ============================================================
// [1] 原生数组：std::begin、std::end，std::rbegin、std::rend
//   书上把 std::begin(array) 叫做“迭代器适配器”，不准确：对原生数组，std::begin 返回的就是普通的 int*，
//     什么都没包；std::rbegin(array) 才真的造了一个适配器 std::reverse_iterator<int*>
//   书上写的“for 循环的输出”，代码其实是 while 循环
//   *it++：先取 *it，再把 it 往后挪（后置 ++ 返回旧值，4.4 节）。对反向迭代器，“往后”就是往数组开头走
// ============================================================
void section1()
{
    std::println("[1] 原生数组");
    int array[]{1, 2, 3, 4, 5};

    auto it = std::begin(array);
    auto end_it = std::end(array);
    std::print("  std::begin..std::end：  ");
    while (it != end_it)
    {
        std::print("{} ", *it++);
    }
    std::println("");

    auto rit = std::rbegin(array);
    auto rend_it = std::rend(array);
    std::print("  std::rbegin..std::rend：");
    while (rit != rend_it)
    {
        std::print("{} ", *rit++);
    }
    std::println("");

    static_assert(std::same_as<decltype(std::begin(array)), int*>);                      // 就是指针
    static_assert(std::same_as<decltype(std::rbegin(array)), std::reverse_iterator<int*>>); // 这才是适配器
    std::println("  std::begin(array) 的类型是 int*，std::rbegin(array) 的类型是 std::reverse_iterator<int*>");
}

// ============================================================
// [2] 书上的 printr：倒着打印任何双向范围
//   用 std::rbegin(c) 而不是 c.rbegin()：这样原生数组也能用（和 4.1 节说的 std::begin 一个道理）
//   书上把局部变量起名叫 rbegin、rend，和 std::rbegin 同名。因为调用时写了 std::，不会混，但最好别这样起名
//   C++20 更简单：for (auto x : c | std::views::reverse)
// ============================================================
void printr(const auto& c, const std::string_view s = "")
{
    if (s.size())
    {
        std::print("  {}: ", s);
    }
    for (auto it = std::rbegin(c); it != std::rend(c); ++it)
    {
        std::print("{} ", *it);
    }
    std::println("");
}

void section2()
{
    std::println("\n[2] printr");
    int array[]{1, 2, 3, 4, 5};
    std::vector<int> v{1, 2, 3, 4, 5};
    std::list<std::string> l{"a", "b", "c"};
    printr(array, "rev c-array");
    printr(v, "rev vector");
    printr(l, "rev list");

    std::print("  views::reverse: ");
    for (auto x : v | std::views::reverse)
    {
        std::print("{} ", x);
    }
    std::println("");
}

// ============================================================
// [3] 反向迭代器是怎么实现的：base() 和“差一个位置”
//   std::reverse_iterator 里只存一个普通迭代器 current（用 r.base() 取出来）。它的几个运算符相当于：
//       *r   ：auto tmp = current; --tmp; return *tmp;     看的是 current 的前一个
//       ++r  ：--current;
//       --r  ：++current;
//       r1 == r2：比较 current
//   为什么不干脆让 current 直接指向“看到的那个元素”？
//     rbegin() 要看到最后一个元素，rend() 要表示“第一个元素的前面”。
//     “第一个元素的前面”这个位置不存在（v.begin() - 1 是未定义行为，原生数组的 arr - 1 也是）。
//     错开一个以后：rbegin() 存 end()，rend() 存 begin()，两个都是合法的位置
//   代价：每次 *r 都要复制一份再 --，比普通迭代器多一点点工作
// ============================================================
void section3()
{
    std::println("\n[3] base()");
    std::vector<int> v{1, 2, 3, 4, 5};
    auto r = v.rbegin();
    std::println("  *v.rbegin() = {}，v.rbegin().base() == v.end() 是 {}", *r, r.base() == v.end());
    std::println("  v.rend().base() == v.begin() 是 {}", v.rend().base() == v.begin());

    auto three = std::find(v.rbegin(), v.rend(), 3);
    std::println("  反向找 3：*r = {}，*r.base() = {}（差一个），*std::prev(r.base()) = {}", *three, *three.base(),
                 *std::prev(three.base()));
}

// ============================================================
// [4] 反向找到以后，怎么换回普通迭代器（书上没讲，实际最常用）
//   很多成员函数只接受普通迭代器，比如 v.erase(pos)。反向迭代器要先换回来：
//     r 看到的元素  =  std::prev(r.base())   或者写成   std::next(r).base()
//   两种写法一样。例子：删掉最后一个 2
//     auto r = std::find(v.rbegin(), v.rend(), 2);
//     v.erase(r.base());               // 错！删掉的是 r.base() 指向的元素，也就是 2 后面那个
//     v.erase(std::prev(r.base()));    // 对
//   如果没找到，r == v.rend()，r.base() 是 v.begin()，再 prev 就越界了，所以要先判断
//   C++23 的 std::ranges::find_last(v, 2) 直接返回最后一个 2 的位置（普通迭代器），不用换（返回一个 subrange，.begin() 就是位置）
// ============================================================
void section4()
{
    std::println("\n[4] 换回普通迭代器");
    std::vector<int> v{1, 2, 3, 2, 1, 2, 5};
    auto r = std::find(v.rbegin(), v.rend(), 2);
    if (r != v.rend())
    {
        v.erase(std::prev(r.base()));
    }
    std::println("  {{1 2 3 2 1 2 5}} 删掉最后一个 2：{}", v);

    std::vector<int> w{1, 2, 3, 2, 1};
    auto last = std::ranges::find_last(w, 2).begin();
    std::println("  ranges::find_last：最后一个 2 的下标是 {}", last - w.begin());
}

// ============================================================
// [5] 只有双向迭代器能反过来
//   forward_list 只能往前走，没有 rbegin：
//     std::rbegin(fl)：
//       clang：error: no matching function for call to 'rbegin'
//       g++  ：error: no matching function for call to 'rbegin(std::forward_list<int>&)'
//     硬写 std::reverse_iterator<std::forward_list<int>::iterator> 再 *：
//       clang：static assertion failed ... reverse_iterator<It> requires It to be a bidirectional iterator.
//       g++  ：error: no match for 'operator--'
//   自己写的迭代器，只要是双向的（4.4 节的 BackSeq 那样，有 --），make_reverse_iterator、views::reverse 都能用
// ============================================================
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

void section5()
{
    std::println("\n[5] 自己写的双向迭代器也能反过来");
    Up u{1, 6};
    std::vector<int> v(std::make_reverse_iterator(u.end()), std::make_reverse_iterator(u.begin()));
    std::println("  make_reverse_iterator：{}", v);
    std::println("  views::reverse：       {}", u | std::views::reverse);
    std::println("  bidirectional_range<forward_list<int>> 是 {}", std::ranges::bidirectional_range<std::forward_list<int>>);
}

// ============================================================
// [6] 反向迭代器的几个小用法（书上没讲）
//   排成从大到小：std::sort(v.rbegin(), v.rend())。sort 照常排“从小到大”，但它看到的是倒过来的 v，
//     所以 v 倒过来是从小到大，v 本身就是从大到小。也可以 std::ranges::sort(v, std::greater{})
//   回文：前一半和“倒过来的前一半”比较：std::equal(s.begin(), s.begin() + s.size() / 2, s.rbegin())
//   倒过来的字符串：std::string(s.rbegin(), s.rend())
//   v 是随机访问的，反向迭代器也是：v.rend() - v.rbegin() 是元素个数，v.rbegin()[1] 是倒数第二个
//   crbegin()：const 版本，*it 是 const int&，不能改
// ============================================================
void section6()
{
    std::println("\n[6] 小用法");
    std::vector<int> v{3, 1, 4, 1, 5};
    std::sort(v.rbegin(), v.rend());
    std::println("  sort(rbegin, rend)：{}", v);

    std::string s{"stressed"};
    std::println("  {:?} 倒过来：{:?}", s, std::string(s.rbegin(), s.rend()));

    std::string_view p{"level"};
    std::println("  {:?} 是回文：{}", p, std::equal(p.begin(), p.begin() + p.size() / 2, p.rbegin()));

    std::println("  v.rend() - v.rbegin() = {}，v.rbegin()[1] = {}", v.rend() - v.rbegin(), v.rbegin()[1]);
    static_assert(std::same_as<decltype(*v.crbegin()), const int&>);
}

// ============================================================
// [7] views::reverse 和 reverse_iterator 的关系
//   v | std::views::reverse 造出来的 view，迭代器就是 std::reverse_iterator<vector<int>::iterator>
//   再 reverse 一次，views::reverse 很聪明：它不会套两层，而是直接还原成原来的范围（ref_view<vector<int>>）
//   写代码时：只是倒着遍历，用 views::reverse；要找到位置再去 erase / insert，用 rbegin + base()（[4]）
// ============================================================
void section7()
{
    std::println("\n[7] views::reverse");
    std::vector<int> v{1, 2, 3};
    auto r = v | std::views::reverse;
    static_assert(std::same_as<std::ranges::iterator_t<decltype(r)>, std::reverse_iterator<std::vector<int>::iterator>>);
    auto rr = v | std::views::reverse | std::views::reverse;
    static_assert(std::same_as<decltype(rr), std::ranges::ref_view<std::vector<int>>>);
    std::println("  reverse 一次：{}，reverse 两次：{}（类型就是 ref_view<vector<int>>，没有套两层）", r, rr);
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
