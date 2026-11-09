// 第 4 章 4.3 创建可迭代范围 —— 练习
// 编译：clang++ -std=c++23 -Wall -Wextra -Wpedantic iterable_range_exercise.cpp -o iterable_range_exercise.exe
//       g++     -std=c++23 -Wall -Wextra -Wpedantic iterable_range_exercise.cpp -o iterable_range_exercise.exe -lstdc++exp
// 运行时检查：补完 TODO，编译运行，所有行都显示“通过”就说明做对了。
// 先看示例 iterable_range.cpp：开头的表格、[3] 停不下来的范围、[4] 只写 ==、[6] 转发成员容器。
#include <algorithm>
#include <array>
#include <cstddef>
#include <print>
#include <ranges>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

void check(bool ok, std::string_view name)
{
    std::println("{} {}", ok ? "通过" : "失败", name);
}

// 用 range-for 把范围里的数收集到 vector 里。最多收 limit 个：收满了说明停不下来，就不再收了
template <typename R>
std::vector<int> collect(const R& r, std::size_t limit = 100)
{
    std::vector<int> out;
    for (int x : r)
    {
        if (out.size() == limit)
        {
            break;
        }
        out.push_back(x);
    }
    return out;
}

using V = std::vector<int>;

// ============================================================
// 练习 1：Python 的 range(start, stop, step)
//   StepRange r{start, stop, step} 依次生成 start, start+step, start+2*step, ……，不包括 stop
//     StepRange{0, 10, 3}   → 0 3 6 9
//     StepRange{10, 0, -3}  → 10 7 4 1          （step 是负数就往下数）
//     StepRange{0, 10}      → 0 1 2 ... 9        （step 默认 1）
//     StepRange{5, 1}       → 空的               （往上数到不了 1）
//   step 不会是 0（不用处理）
//
//   示例 [3] 说过：用“值 != 终点值”判断结束，碰到 {0, 10, 3} 就停不下来（0 3 6 9 12 …… 永远不等于 10）。
//   所以这里换个思路：迭代器里存“第几个”（index），而不是存值：
//
//     StepRange{0, 10, 3}：一共 count = 4 个
//       index：   0    1    2    3    4
//       值：      0    3    6    9   （没有）
//                 ^                   ^
//              begin()              end()      begin() 的 index 是 0，end() 的 index 是 count
//
//     *it  = start + index * step
//     ++it ：index 加 1
//     ==   ：比较 index（C++20 只写 == 就行，示例 [4]；可以直接 = default）
//
//   第一步：在构造函数里算出 count（一共几个）
//     step > 0：如果 stop <= start，count = 0；否则 count = (stop - start + step - 1) / step
//               例：{0, 10, 3}：(10 - 0 + 3 - 1) / 3 = 12 / 3 = 4
//                   {0, 9, 3} ：(9 - 0 + 3 - 1) / 3 = 11 / 3 = 3    → 0 3 6
//     step < 0：反过来想：把 start 和 stop 对调、step 取正，就是上面的情况
//               例：{10, 0, -3}：等于算 (10 - 0 + 3 - 1) / 3 = 4    → 10 7 4 1
//   第二步：写 iterator：存哪些成员、构造函数、*、++、==，由你决定
//   第三步：begin()、end()
//   现在的代码是能编译的“空壳”：== 永远返回 true，所以 begin() == end()，什么都不生成
// ============================================================
class StepRange
{
    int start_{};
    int step_{};
    int count_{};

public:
    StepRange(int start, int stop, int step = 1)
        : start_{start}, step_{step}
    {
        if (step > 0)
        {
            if (stop <= start)
            {
                count_ = 0;
            }
            else
            {
                count_ = (stop - start + step - 1) / step;
            }
        }
        else
        {
            if (start <= stop)
            {
                count_ = 0;
            }
            else
            {
                count_ = (start - stop - step - 1) / (-step);
            }
        }
    }

    int size() const
    {
        return count_;
    }

    class iterator
    {
        int start_{};
        int step_{};
        int index_{};

    public:
        explicit iterator(int start, int step, std::ptrdiff_t index = std::ptrdiff_t{})
            : start_{start}, step_{step}, index_{index}
        {}

        int operator*() const
        {
            // 改动（补充）：小地方。index_ 是 ptrdiff_t（long long），start_ + index_ * step_ 算出来是 long long，
            //   return 时又悄悄转回 int。加 -Wconversion 编译，clang 会警告“implicit conversion loses integer precision”。
            //   count_ 本来就是 int，index_ 也用 int 就行，类型统一，不用来回转换。
            return start_ + index_ * step_;
        }

        iterator& operator++()
        {
            ++index_;
            return *this;
        }

        bool operator==(const iterator& other) const
        {
            return index_ == other.index_;
        }
    };

    iterator begin() const
    {
        return iterator{start_, step_, {}};
    }

    iterator end() const
    {
        return iterator{start_, step_, count_};
    }
};

void test_exercise1()
{
    check(collect(StepRange{0, 10, 3}) == V{0, 3, 6, 9}, "练习 1：{0, 10, 3} → 0 3 6 9");
    check(collect(StepRange{0, 9, 3}) == V{0, 3, 6}, "练习 1：{0, 9, 3} → 0 3 6");
    check(collect(StepRange{0, 5}) == V{0, 1, 2, 3, 4}, "练习 1：{0, 5} → 0 1 2 3 4（step 默认 1）");
    check(collect(StepRange{10, 0, -3}) == V{10, 7, 4, 1}, "练习 1：{10, 0, -3} → 10 7 4 1");
    check(collect(StepRange{3, -3, -2}) == V{3, 1, -1}, "练习 1：{3, -3, -2} → 3 1 -1");
    check(collect(StepRange{-5, 5, 5}) == V{-5, 0}, "练习 1：{-5, 5, 5} → -5 0");
    check(collect(StepRange{5, 5}).empty(), "练习 1：{5, 5} → 空");
    check(collect(StepRange{5, 1}).empty(), "练习 1：{5, 1} → 空（往上数到不了）");
    check(collect(StepRange{1, 5, -1}).empty(), "练习 1：{1, 5, -1} → 空（往下数到不了）");
    check(StepRange{0, 10, 3}.size() == 4 && StepRange{10, 0, -3}.size() == 4 && StepRange{5, 1}.size() == 0,
          "练习 1：size()");

    const StepRange r{1, 8, 2};
    check(collect(r) == V{1, 3, 5, 7} && collect(r) == V{1, 3, 5, 7}, "练习 1：同一个 const 对象遍历两遍，结果一样");
}

// ============================================================
// 练习 2：书架 Shelf —— 转发成员容器的迭代器（示例 [6]）
//   书架有 5 个格子（std::array<std::string, 5>），按顺序往里放书，count_ 记录放了几本。
//   请加上 begin()/end()，让 range-for 只遍历“放了书的格子”：
//
//     slots_：  [0] "C++"  [1] "STL"  [2] "Rust"  [3] ""  [4] ""      count_ = 3
//                ^                                 ^                 ^
//             begin()                           end() 应该在这     slots_.end() 在这（不对，会多出两个空字符串）
//
//   要求：
//     * const 和非 const 各一对 begin()/end()（示例 [6]），返回类型可以写 auto
//     * 返回 slots_ 本身的迭代器（这样 std::ranges::sort 之类的算法也能直接用）
//   下面三个 helper 用 if constexpr 判断 Shelf 能不能遍历：
//   还没写 begin/end 时它们什么都不做，所以这个文件现在也能编译。helper 不用改。
// ============================================================
class Shelf
{
    std::array<std::string, 5> slots_{};
    std::size_t count_{0};

public:
    bool add(std::string title)
    {
        if (count_ == slots_.size())
        {
            return false;
        }
        slots_[count_] = std::move(title);
        ++count_;
        return true;
    }

    std::size_t size() const
    {
        return count_;
    }

    auto begin()
    {
        return slots_.begin();
    }

    auto end()
    {
        return slots_.begin() + static_cast<std::ptrdiff_t>(count_);
    }

    auto begin() const
    {
        return slots_.begin();
    }

    auto end() const
    {
        return slots_.begin() + static_cast<std::ptrdiff_t>(count_);
    }
};

// 在 const 对象上遍历，收集所有书名
template <typename S>
std::vector<std::string> titles_of(const S& s)
{
    std::vector<std::string> out;
    if constexpr (std::ranges::range<const S>)
    {
        for (const auto& t : s)
        {
            out.push_back(t);
        }
    }
    return out;
}

// 在非 const 对象上遍历，给每个书名后面加 "!"
template <typename S>
void shout(S& s)
{
    if constexpr (std::ranges::range<S>)
    {
        for (auto& t : s)
        {
            t += "!";
        }
    }
}

// 用标准库的算法排序
template <typename S>
void sort_shelf(S& s)
{
    if constexpr (std::ranges::random_access_range<S>)
    {
        std::ranges::sort(s);
    }
}

void test_exercise2()
{
    using SV = std::vector<std::string>;
    check(std::ranges::range<Shelf> && std::ranges::range<const Shelf>, "练习 2：Shelf 和 const Shelf 都能遍历");

    Shelf s;
    s.add("STL");
    s.add("Rust");
    s.add("C++");
    check(titles_of(s) == SV{"STL", "Rust", "C++"}, "练习 2：只遍历放了书的 3 个格子");
    shout(s);
    check(titles_of(s) == SV{"STL!", "Rust!", "C++!"}, "练习 2：非 const 遍历可以修改");
    sort_shelf(s);
    check(titles_of(s) == SV{"C++!", "Rust!", "STL!"}, "练习 2：std::ranges::sort 能用，只排放了书的格子");

    Shelf empty;
    check(titles_of(empty).empty(), "练习 2：空书架");

    Shelf full;
    for (std::string t : {"a", "b", "c", "d", "e"})
    {
        full.add(t);
    }
    check(!full.add("f") && titles_of(full) == SV{"a", "b", "c", "d", "e"}, "练习 2：放满 5 本，第 6 本放不进去");
}

// ============================================================
// 练习 3：找 bug
//   Evens{from, to} 生成 [from, to) 里的所有偶数：
//     Evens{0, 10} → 0 2 4 6 8
//     Evens{1, 6}  → 2 4
//   迭代器里存的是值，每次 ++ 加 2，用 == 比较值（和示例 [1] 的 Seq 一样的思路）。
//   运行后会看到有几个检查“失败”，而且收集到了 100 个数（collect 的上限）—— 也就是停不下来了。
//   找出原因并修好。要求：迭代器还是存值、比较值（不要改成练习 1 那样存 index），只改 Evens 的构造函数。
//   提示：示例 [3] —— 从 begin() 开始一次次 +2，能不能“正好等于” end()？
//         把失败的那几个例子的 begin、end 的值写出来看看
// ============================================================
int round_up_even(int x) // 向上取到偶数：4 → 4，3 → 4，-3 → -2
{
    return x % 2 == 0 ? x : x + 1;
}

class Evens
{
    int first_{};
    int last_{};

public:
    Evens(int from, int to)
        : first_{round_up_even(from)}, last_{round_up_even(to)}
    {
        if (from >= to)
        {
            first_ = last_;
        }
    }

    class iterator
    {
        int value_{};

    public:
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

        bool operator==(const iterator&) const = default;
    };

    iterator begin() const
    {
        return iterator{first_};
    }

    iterator end() const
    {
        return iterator{last_};
    }
};

void test_exercise3()
{
    check(collect(Evens{0, 10}) == V{0, 2, 4, 6, 8}, "练习 3：{0, 10} → 0 2 4 6 8");
    check(collect(Evens{1, 6}) == V{2, 4}, "练习 3：{1, 6} → 2 4");
    check(collect(Evens{0, 9}) == V{0, 2, 4, 6, 8}, "练习 3：{0, 9} → 0 2 4 6 8");
    check(collect(Evens{-3, 3}) == V{-2, 0, 2}, "练习 3：{-3, 3} → -2 0 2");
    check(collect(Evens{4, 4}).empty(), "练习 3：{4, 4} → 空");
    check(collect(Evens{3, 3}).empty(), "练习 3：{3, 3} → 空");
    check(collect(Evens{8, 2}).empty(), "练习 3：{8, 2} → 空");
    // 改动：多加几个边界情况（你的写法全部通过）：区间里一个偶数都没有、负数区间
    check(collect(Evens{3, 4}).empty() && collect(Evens{1, 2}).empty() && collect(Evens{-1, 0}).empty(),
          "练习 3：{3, 4}、{1, 2}、{-1, 0} → 空（区间里没有偶数）");
    check(collect(Evens{-4, -3}) == V{-4} && collect(Evens{-5, -1}) == V{-4, -2}, "练习 3：负数区间");
    std::println("  （{{0, 9}} 收集到了 {} 个数）", collect(Evens{0, 9}).size());
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
// （1）练习 1 里你只写了 operator==，range-for 展开后用的却是 begin_ != end_，为什么能编译？
//      反过来，如果只写 operator!=（书上的写法），it == r.end() 能编译吗？（可以改一下试试，再改回来）
// 你的答案：可以转化为!(begin_ == end_)。反过来不可以。
// 改动（补充）：对。规则是 C++20 定的，只有固定的几种改写：
//     a != b  →  !(a == b)
//     b == a  →  a == b             （参数对调也行）
//     a < b、a > b、a <= b、a >= b  →  用 a <=> b 改写（1.5 节）
//   没有任何一条是“用 != 改写出 ==”，所以只写 != 的时候，it == r.end() 编译不过。
//   结论：C++20 起写 ==，不要写 !=。
//
// （2）示例 [1] 的 Seq::operator* 返回的是 T（值），不是 T&。对 Seq<int> r{1, 4}，下面三种写法哪些能编译？为什么？
//        for (auto v : r)          for (auto& v : r)          for (const auto& v : r)
//      提示：按示例 [2] 展开，循环变量那一行分别是 auto v = *begin_;  auto& v = *begin_;  const auto& v = *begin_;
//            *begin_ 得到的是一个临时的 int
// 你的答案：for (auto v : r)和for (const auto& v : r)可以。
// 改动（补充）：对，补上为什么：
//   auto v = *begin_;         把临时的 int 复制一份，当然可以
//   auto& v = *begin_;        非 const 的引用不能绑定到临时值上。编译器报错：
//                               non-const lvalue reference to type 'int' cannot bind to a temporary of type 'int'
//                             这条规则是为了防止“改了一个马上就要消失的东西”：改了也没用
//   const auto& v = *begin_;  const 引用可以绑定到临时值，而且会把临时值的寿命延长到 v 的作用域结束
//   还有一种 auto&& v 也能用（万能引用：绑到什么都行），标准库的 ranges 算法内部就是这么写的
//   所以写模板代码遍历“不知道是什么”的范围时，用 const auto& 或 auto&&，不要用 auto&
//
// （3）练习 2 的 const 版本写成 auto begin() const { return slots_.begin(); }，
//      返回的是 std::array<std::string, 5>::iterator 还是 const_iterator？为什么？
//      这保证了 titles_of(const S& s) 里的什么事情？
//      可以用这一行验证（放到 test_exercise2 里）：
//        static_assert(std::same_as<decltype(std::declval<const Shelf&>().begin()), std::array<std::string, 5>::const_iterator>);
//      （std::declval<const Shelf&>() 的意思是“假装有一个 const Shelf&”，只能用在 decltype 里面）
// 你的答案：返回的是const_iterator。因为加了const。只读。
// 改动（补充）：对，把“因为加了 const”说具体一点：
//   const 成员函数里，this 指向的是 const Shelf，所以成员 slots_ 也是 const 的。
//   std::array 有两个 begin()：普通版本返回 iterator，const 版本返回 const_iterator。
//   对 const 的 slots_ 调用 begin()，只能选到 const 版本，auto 就推导成 const_iterator。
//   这保证了：titles_of(const S& s) 里遍历时，*it 是 const std::string&，想改书名会编译错误。
//   反过来，如果 const 版本返回的是 iterator，别人拿到一个 const Shelf& 也能改里面的书名，const 就形同虚设了。
//   那行 static_assert 我放进去试过，能编译通过。
//
// ============================================================
