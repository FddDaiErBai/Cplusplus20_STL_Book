// 第 4 章 4.10 随机访问迭代器 —— 练习
// 编译：clang++ -std=c++23 -Wall -Wextra -Wpedantic random_access_exercise.cpp -o random_access_exercise.exe
//       g++     -std=c++23 -Wall -Wextra -Wpedantic random_access_exercise.cpp -o random_access_exercise.exe -lstdc++exp
// 运行时检查：补完 TODO，编译运行，所有行都显示“通过”就说明做对了。
// 先看示例 random_access.cpp 开头的表、[2]（书上三处错误）、[3]（每个运算符的写法）。
#include <algorithm>
#include <compare>
#include <cstddef>
#include <iterator>
#include <list>
#include <print>
#include <ranges>
#include <string_view>
#include <utility>
#include <vector>

void check(bool ok, std::string_view name)
{
    std::println("{} {}", ok ? "通过" : "失败", name);
}

using V = std::vector<int>;

// ============================================================
// 练习 1：Stride —— 每隔 step 个取一个（相当于 C++23 的 views::stride）
//   Stride{v, 2}：v = {0 10 20 30 40 50 60} → 0 20 40 60
//   麻烦的地方在 end：元素个数不一定是 step 的倍数。7 个元素、step 2，取出 4 个：下标 0 2 4 6，
//     end 应该在“第 4 个”的位置，也就是下标 8，已经超出了数组（思考题 1(a)）。
//   所以迭代器里不存指针，存“第几个”i_（一个整数），*it 时才算出真正的位置 base_[i_ * step_]。
//   迭代器存三个东西：base_（数组开头的指针，不变）、i_（第几个，std::ptrdiff_t）、step_（步长）
//     *it         ：base_[i_ * step_]
//     ++it、--it  ：i_ 加减 1
//     it += n     ：i_ += n
//     it2 - it1   ：it2.i_ - it1.i_
//     ==、<=>     ：比较 i_（= default 也行：base_、step_ 都一样，逐个比较的结果就是比较 i_）
//     it[n]       ：*(it + n)
//   Stride::begin() 返回 i_ = 0，end() 返回 i_ = 一共几个（(size + step - 1) / step），已经写好了
//
//   TODO（都在 StrideIt 里），照着示例 [3] 写，开头的表里每一项都要有：
//     ① 类型别名：iterator_concept、iterator_category 都写 random_access_iterator_tag（不是 contiguous：元素不紧挨着）；
//        value_type = int；difference_type = std::ptrdiff_t
//     ② 默认构造
//     ③ *、[]
//     ④ 前置/后置 ++、前置/后置 --
//     ⑤ +=、-=，以及 it + n、n + it、it - n、it2 - it1（用 friend，参数按值，借用 += -=）
//     ⑥ ==、<=>
//   现在只写了构造函数和 *。下面的 helper 用 if constexpr 判断能不能用，helper 不用改。
// ============================================================
class StrideIt
{
    int* base_{};
    std::ptrdiff_t i_{};
    std::ptrdiff_t step_{1};

public:
    using iterator_concept = std::random_access_iterator_tag;
    using iterator_category = std::random_access_iterator_tag;
    using value_type = int;
    using difference_type = std::ptrdiff_t;

    StrideIt() = default;

    StrideIt(int* base, std::ptrdiff_t i, std::ptrdiff_t step)
        : base_{base}, i_{i}, step_{step}
    {
    }

    int& operator*() const
    {
        return base_[i_ * step_];
    }

    int& operator[](difference_type n) const
    {
        return *(*this + n);
    }

    StrideIt& operator++()
    {
        ++i_;
        return *this;
    }

    StrideIt operator++(int)
    {
        StrideIt old = *this;
        ++i_;
        return old;
    }

    StrideIt& operator--()
    {
        --i_;
        return *this;
    }

    StrideIt operator--(int)
    {
        StrideIt old = *this;
        --i_;
        return old;
    }

    StrideIt& operator+=(difference_type n)
    {
        i_ += n;
        return *this;
    }

    StrideIt& operator-=(difference_type n)
    {
        i_ -= n;
        return *this;
    }

    friend StrideIt operator+(StrideIt it, difference_type n)
    {
        return it += n;
    }

    friend StrideIt operator+(difference_type n, StrideIt it)
    {
        return it += n;
    }

    friend StrideIt operator-(StrideIt it, difference_type n)
    {
        return it -= n;
    }

    friend difference_type operator-(const StrideIt it1, const StrideIt it2) // 改动（补充）：按值传也对。只读不改的参数一般写 const StrideIt&，
    {
        //   这里迭代器很小（三个整数），按值复制也不贵，两种都行
        return it1.i_ - it2.i_;
    }

    bool operator==(const StrideIt&) const = default;
    auto operator<=>(const StrideIt&) const = default;
};

class Stride
{
    int* base_{};
    std::ptrdiff_t count_{};
    std::ptrdiff_t step_{1};

public:
    Stride(V& v, std::ptrdiff_t step)
        : base_{v.data()}, count_{(static_cast<std::ptrdiff_t>(v.size()) + step - 1) / step}, step_{step}
    {
    }

    StrideIt begin() const
    {
        return StrideIt{base_, 0, step_};
    }

    StrideIt end() const
    {
        return StrideIt{base_, count_, step_};
    }
};

// 收集（新的一套，ranges::to）
// （helper 都写成模板 S：if constexpr 只有在模板里才会真的“跳过”不成立的那个分支，普通函数里两个分支都要能编译）
template <typename S>
V collect(const S& s)
{
    if constexpr (std::ranges::input_range<const S>)
    {
        return s | std::ranges::to<V>();
    }
    (void)s;
    return {-1};
}

// 倒着收集
template <typename S>
V reversed(const S& s)
{
    if constexpr (std::ranges::bidirectional_range<const S>)
    {
        return s | std::views::reverse | std::ranges::to<V>();
    }
    (void)s;
    return {-1};
}

// 只对“取出来的那些”排序：std::ranges::sort 要求随机访问
template <typename S>
bool sort_picked(const S& s)
{
    if constexpr (std::ranges::random_access_range<const S>)
    {
        std::ranges::sort(s);
        return true;
    }
    (void)s;
    return false;
}

// 跳着访问：it + n、n + it、it[n]、it - n、end - begin、<
template <typename It>
bool jumps_ok(It b, It e)
{
    if constexpr (std::random_access_iterator<It>)
    {
        return *(b + 2) == 40 && *(2 + b) == 40 && b[3] == 60 && *(e - 1) == 60 && e - b == 4 && b - e == -4 && b < e && !(e < b)
            && *(b += 1) == 20 && *(b -= 1) == 0;
    }
    (void)b;
    (void)e;
    return false;
}

// 后置 ++、-- 返回旧值
template <typename It>
bool postfix_ok(It b)
{
    if constexpr (std::bidirectional_iterator<It>)
    {
        auto old = b++;
        bool ok1 = *old == 0 && *b == 20;
        auto old2 = b--;
        return ok1 && *old2 == 20 && *b == 0;
    }
    (void)b;
    return false;
}

void test_exercise1()
{
    check(std::random_access_iterator<StrideIt>, "练习 1：std::random_access_iterator<StrideIt>");
    check(!std::contiguous_iterator<StrideIt>, "练习 1：不是 contiguous（元素不紧挨着）");
    check(std::ranges::sized_range<Stride>, "练习 1：sized_range（随机访问 + end - begin 就知道长度）");

    V v{0, 10, 20, 30, 40, 50, 60};
    Stride s{v, 2};
    check(collect(s) == V{0, 20, 40, 60}, "练习 1：Stride{v, 2} → 0 20 40 60");
    check(reversed(s) == V{60, 40, 20, 0}, "练习 1：views::reverse → 60 40 20 0");
    check(jumps_ok(s.begin(), s.end()), "练习 1：it + n、n + it、it[n]、it - n、end - begin、<、+=、-=");
    check(postfix_ok(s.begin()), "练习 1：后置 ++、-- 返回旧值");

    V w{1, 2, 3, 4, 5, 6};
    V e;
    check(collect(Stride{w, 3}) == V{1, 4} && collect(Stride{w, 1}) == w && collect(Stride{e, 2}).empty(),
          "练习 1：step 3 / step 1 / 空的");

    V u{9, 0, 7, 0, 8, 0, 6};
    bool sorted = sort_picked(Stride{u, 2});
    check(sorted && u == V{6, 0, 7, 0, 8, 0, 9}, "练习 1：ranges::sort 只排取出来的那些，其它位置不动");

    // 改动（补充）：元素个数不是 step 的倍数时 end - 1 是最后一个；通过 Stride 修改元素；二分查找也能用
    V odd{1, 2, 3, 4, 5};
    Stride so{odd, 2};
    check(std::ranges::size(so) == 3 && *(so.end() - 1) == 5, "练习 1：5 个元素 step 2 → 3 个，最后一个是 5");
    V m{0, 10, 20, 30, 40, 50, 60};
    for (int& x : Stride{m, 3})
    {
        x = -x;
    }
    check(m == V{0, 10, 20, -30, 40, 50, -60}, "练习 1：通过 int& 修改取出来的元素");
    V b{1, 9, 3, 9, 5, 9};
    check(std::ranges::binary_search(Stride{b, 2}, 3) && !std::ranges::binary_search(Stride{b, 2}, 9),
          "练习 1：ranges::binary_search（只看 1 3 5）");
}

// ============================================================
// 练习 2：自己写二分查找 my_lower_bound(first, last, x)：返回第一个 >= x 的位置（[first, last) 已经排好序）
//   一种写法（count 表示“还剩几个没排除”）：
//     auto count = last - first;
//     while (count > 0)
//     {
//         auto half = count / 2;
//         auto mid = first + half;
//         if (*mid < x) { first = mid + 1; count -= half + 1; }   // 答案在 mid 右边
//         else          { count = half; }                       // 答案在 mid 或它左边
//     }
//     return first;
//   TODO：
//     ① 把模板参数的约束写对：函数体里用了 last - first、first + half，要求随机访问。
//        现在写的是 std::forward_iterator It，太弱了（4.1 节练习 2 的规则：选“函数体真正需要的、最弱的”）
//     ② 写函数体
//   下面的 can_lb 检查“能不能调用 my_lower_bound”，不用改。
// ============================================================
template <std::random_access_iterator It>
It my_lower_bound(It first, It last, int x)
{
    auto count = last - first;
    while (count > 0)
    {
        auto half = count / 2;
        auto mid = first + half;
        if (*mid < x)
        {
            first = mid + 1;
            count -= half + 1;
        }
        else
        {
            count = half;
        }
    }
    return first;
}

template <typename It>
concept can_lb = requires(It a, It b) { my_lower_bound(a, b, 0); };

void test_exercise2()
{
    V v{10, 20, 20, 30, 50};
    check(*my_lower_bound(v.begin(), v.end(), 20) == 20 && my_lower_bound(v.begin(), v.end(), 20) - v.begin() == 1,
          "练习 2：lower_bound 20 → 下标 1（第一个 20）");
    check(my_lower_bound(v.begin(), v.end(), 25) - v.begin() == 3 && my_lower_bound(v.begin(), v.end(), 5) == v.begin(),
          "练习 2：25 → 下标 3；5 → begin");
    check(my_lower_bound(v.begin(), v.end(), 99) == v.end(), "练习 2：比所有都大 → end");
    V e;
    check(my_lower_bound(e.begin(), e.end(), 1) == e.end(), "练习 2：空的 → end");
    int arr[]{1, 3, 5, 7};
    check(*my_lower_bound(arr, arr + 4, 4) == 5, "练习 2：原生数组（指针也是随机访问迭代器）");
    check(can_lb<V::iterator> && can_lb<int*> && !can_lb<std::list<int>::iterator>, "练习 2：接受 vector、指针，拒绝 list");
}

// ============================================================
// 练习 3：找 bug（两处 bug，每处只改一行。只改这两处，别的地方不要动）
//   Ring 是一个“环形”的下标迭代器：在 0..n-1 里转圈，用来演示。运行后看到：
//     * std::random_access_iterator<RingIt> 是 false（示例 [2] 里书上的问题之一）
//     * a - b 应该是 -3，算出来是一个很大的数
//   提示：示例 [2] 的 (a)(b)(c) —— 看看 it2 - it1 的返回类型、const，看看 n + it 返回什么类型
// ============================================================
class RingIt
{
    std::ptrdiff_t i_{};

public:
    using iterator_concept = std::random_access_iterator_tag;
    using iterator_category = std::random_access_iterator_tag;
    using value_type = std::ptrdiff_t;
    using difference_type = std::ptrdiff_t;

    RingIt() = default;

    explicit RingIt(std::ptrdiff_t i)
        : i_{i}
    {
    }

    std::ptrdiff_t operator*() const
    {
        return i_;
    }

    std::ptrdiff_t operator[](difference_type n) const
    {
        return i_ + n;
    }

    RingIt& operator++()
    {
        ++i_;
        return *this;
    }

    RingIt operator++(int)
    {
        RingIt old = *this;
        ++i_;
        return old;
    }

    RingIt& operator--()
    {
        --i_;
        return *this;
    }

    RingIt operator--(int)
    {
        RingIt old = *this;
        --i_;
        return old;
    }

    RingIt& operator+=(difference_type n)
    {
        i_ += n;
        return *this;
    }

    RingIt& operator-=(difference_type n)
    {
        i_ -= n;
        return *this;
    }

    friend RingIt operator+(RingIt it, difference_type n)
    {
        return it += n;
    }

    friend RingIt operator+(difference_type n, RingIt it)
    {
        return it += n;
    }

    friend RingIt operator-(RingIt it, difference_type n)
    {
        return it -= n;
    }

    friend difference_type operator-(const RingIt& a, const RingIt& b)
    {
        return a.i_ - b.i_;
    }

    bool operator==(const RingIt&) const = default;
    auto operator<=>(const RingIt&) const = default;
};

template <typename It>
bool ring_ok()
{
    if constexpr (std::random_access_iterator<It>)
    {
        It a{2};
        It b{5};
        return std::ranges::distance(a, b) == 3 && *std::ranges::next(a, 2) == 4;
    }
    return false;
}

void test_exercise3()
{
    check(std::random_access_iterator<RingIt>, "练习 3：std::random_access_iterator<RingIt>");
    RingIt a{2};
    RingIt b{5};
    auto d = a - b;
    check(std::cmp_equal(d, -3) && std::cmp_less(d, 0), "练习 3：a - b == -3（有正负。用 1.4 节的 cmp_equal、cmp_less 比较，不会被 size_t 骗）");
    check(*(3 + a) == 5 && *(a + 3) == 5, "练习 3：3 + a、a + 3");
    check(ring_ok<RingIt>(), "练习 3：ranges::distance、ranges::next 能用");
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
// （1）关于练习 1 的 Stride
//   (a) 为什么迭代器里存“下标 i_”而不是直接存指针？存指针的话，Stride{v, 2}（v 有 7 个元素）的 end 指向哪里？这有什么问题？
// 你的答案：防止越界解引用，未定义行为。base_[i_ * step_]，i是指针？没理解。
// 改动（补充）：i_ 不是指针，是一个整数：“这是取出来的第几个”。真正的指针只有 base_（数组开头），*it 时才临时算出 base_ + i_ * step_。
//   如果迭代器里直接存指针 p_：v 有 7 个元素（下标 0..6），step 2 取出 0 2 4 6 四个。
//   end 是“最后一个的后一个”，也就是再跳一步：下标 6 + 2 = 8，p_ = v.data() + 8。
//   问题不是“解引用越界”（end 本来就不解引用），而是：这个指针算出来就已经是未定义行为。
//   C++ 只允许指针指向数组里的元素，或者“最后一个的后一个”（v.data() + 7）。v.data() + 8 超出了这个范围，
//   哪怕只是算出来、不去 * 它，也是未定义行为（和 4.7 节说的 begin() - 1 一样）。
//   存整数 i_ 就没这个问题：i_ = 4 只是个数字，永远不会被拿去造那个越界的指针。
//   (b) Stride 为什么不能声明成 contiguous_iterator_tag？（提示：示例 [4]，“连续”保证了什么？*(it + 1) 和 *(std::to_address(it) + 1) 一样吗？）
// 你的答案：连续保证元素在内存里紧挨着。像stride这种隔着几个数，？*(it + 1)和*(std::to_address(it) + 1) 的结果不一样。
// 改动（补充）：对，而且答到了关键。说完整：to_address(it) 拿到的是 it 指向的元素的真正指针，
//   对这个指针 + 1 是内存里的下一个 int；而 it + 1 是 Stride 的“下一个”，隔了 step 个。
//   连续迭代器保证这两个永远一样，所以 std::span、memcpy 才敢直接拿指针按内存顺序读。
//   Stride 做不到，声明成 contiguous 就是在撒谎：span 会读到中间那些没被选中的元素。
//
// （2）关于练习 2
//   (a) 约束写成 std::forward_iterator 时，list 也能调用 my_lower_bound，函数体里哪一行会出问题？会是编译错误还是运行时错误？
// 你的答案：第一步auto count = last-first就错了。编译错误。
// 改动（补充）：对。为什么是编译错误而不是运行时：约束 forward_iterator 让 list 的迭代器通过了“门口的检查”，
//   但函数体实例化时，list 迭代器没有 operator-，编译器在这里报错。报错位置在函数体里面，一堆模板信息，不好看懂。
//   约束写成 random_access_iterator 后，报错变成调用处的一句“不满足约束”，清楚得多（4.1 节说的“太弱”的坏处）。
//   (b) 标准库的 std::ranges::lower_bound 其实接受 forward 迭代器，它没有 first + half 怎么办？代价是什么？（提示：std::ranges::next，4.1 节）
// 你的答案：一步一步走，开销变大。
// 改动（补充）：对，说具体一点开销变大多少：用 std::ranges::next(first, half) 一步一步走到中间，
//   每次走一半，总共要走 n/2 + n/4 + …… ≈ n 步，移动迭代器变成 O(n)。
//   但比较次数还是 O(log n)：比较很贵（比如比较长字符串）、移动很便宜（list 的指针）时，二分查找仍然划算。
//
// （3）关于示例 [2] 和练习 3
//   (a) it2 - it1 为什么必须返回有符号的类型？返回 size_t 时，算 a - b（a 在 b 前面）会得到什么？为什么？
// 你的答案：元素有先后之别。会得到一个大数，符号溢出了。
// 改动（补充）：前两点对。“符号溢出”这个词不准确，要分清两件事：
//   有符号整数（int、ptrdiff_t）溢出：未定义行为。
//   无符号整数（size_t）算出负数：不是溢出错误，规则规定“绕回来”，对 2^64 取余，-3 变成 2^64 - 3 = 18446744073709551613。
//   这里是第二种：a.i_ - b.i_ 本来是 -3（有符号，没问题），转成 size_t 时绕成了大数。
//   所以要返回有符号的 difference_type：it2 - it1 本来就可能是负的（a 在 b 前面），size_t 根本装不下。
//   (b) 示例 [2] 说书上的迭代器 std::sort 能用、std::ranges::sort 不能用。为什么两个 sort 的态度不一样？（提示：4.4 节的“两套”）
// 你的答案：后者需要随机访问。
// 改动（补充）：两个 sort 都需要随机访问，区别在“怎么判断你是不是随机访问”（4.4 节的两套）：
//   std::sort（老的一套）：只看 iterator_traits 里的 iterator_category 标签。书上写了 random_access_iterator_tag，
//     它就信了，直接用 it - it、it + n。书上的这几个运算符虽然写得不规范（非 const、返回 size_t），
//     但 std::sort 内部调用的方式恰好能编译，所以能用（size_t 的差在内部又转回了有符号，碰巧没出错）。
//   std::ranges::sort（新的一套）：用 std::random_access_iterator 概念逐项检查，包括“在 const 迭代器上做 it - it”
//     “结果是有符号的 difference_type”，书上的三处问题全被查出来，拒绝编译。
//   这正是概念的意义：标签只是一句“自我声明”，概念是真的去检查。
//
// ============================================================
