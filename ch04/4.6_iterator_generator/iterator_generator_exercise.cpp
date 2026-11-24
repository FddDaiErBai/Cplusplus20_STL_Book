// 第 4 章 4.6 迭代器生成器 —— 练习
// 编译：clang++ -std=c++23 -Wall -Wextra -Wpedantic iterator_generator_exercise.cpp -o iterator_generator_exercise.exe
//       g++     -std=c++23 -Wall -Wextra -Wpedantic iterator_generator_exercise.cpp -o iterator_generator_exercise.exe -lstdc++exp
// 运行时检查：补完 TODO，编译运行，所有行都显示“通过”就说明做对了。
// 先看示例 iterator_generator.cpp：[1]（书上的写法）、[3]（溢出）、[5]（default_sentinel）、[6]（unreachable_sentinel）。
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <print>
#include <ranges>
#include <string_view>
#include <vector>

void check(bool ok, std::string_view name)
{
    std::println("{} {}", ok ? "通过" : "失败", name);
}

using V64 = std::vector<std::uint64_t>;

// 用 range-for 收集，最多 1000 个（防止停不下来）
template <typename R>
V64 collect(const R& r)
{
    V64 out;
    for (auto x : r)
    {
        if (out.size() == 1000)
        {
            break;
        }
        out.push_back(x);
    }
    return out;
}

// 老的一套（iterator_traits）认为 It 至少是 Tag 这个类别吗？（4.4 练习用过）
template <typename It, typename Tag>
concept legacy_at_least = requires { typename std::iterator_traits<It>::iterator_category; }
                          && std::derived_from<typename std::iterator_traits<It>::iterator_category, Tag>;

// ============================================================
// 练习 1：Collatz 序列（“3n+1 猜想”），用 default_sentinel 判断结束（照着示例 [5] 的 FibN）
//   从 n 开始：偶数就除以 2，奇数就乘 3 加 1，一直到 1 为止（1 也要生成）
//     Collatz{6}  → 6 3 10 5 16 8 4 2 1
//     Collatz{1}  → 1
//     Collatz{27} → 27 82 41 124 …… 一共 112 个数，最大的是 9232
//   这里没法事先知道一共有几个，所以迭代器里存一个 done_：“已经生成过 1 了吗”
//     *it   ：返回 value_（已经写好了）
//     ++it  ：如果 value_ 是 1，说明 1 已经生成过了，把 done_ 设成 true；否则按规则算出下一个 value_
//     it == std::default_sentinel ：就是 done_
//   TODO（都在 iterator 里面）：
//     ① 四个类型别名：iterator_concept、iterator_category（都是 forward）、value_type、difference_type
//     ② 默认构造函数
//     ③ 前置 ++ 的函数体（现在什么都不做），再加上后置 ++
//     ④ 迭代器之间的 ==（可以 = default）
//     ⑤ 改好和哨兵比较的 ==（现在永远返回 true，所以什么都不生成）
//   Collatz 本身、begin()、end() 不用改。下面的 helper 用 if constexpr 判断能不能用，helper 不用改。
// ============================================================
class Collatz
{
    std::uint64_t start_{};

public:
    explicit Collatz(std::uint64_t start)
        : start_{start < 1 ? 1 : start}
    {
    }

    class iterator
    {
        std::uint64_t value_{1};
        bool done_{true};

    public:
        using iterator_concept = std::forward_iterator_tag;
        using iterator_category = std::forward_iterator_tag;
        using value_type = std::uint64_t;
        using difference_type = std::ptrdiff_t;

        iterator() = default;

        explicit iterator(std::uint64_t start)
            : value_{start}, done_{false}
        {
        }

        std::uint64_t operator*() const
        {
            return value_;
        }

        iterator& operator++()
        {
            if (value_ == 1)
            {
                done_ = true;
            }
            else
            {
                if (value_ % 2 == 0)
                {
                    value_ /= 2;
                }
                else
                {
                    value_ = 3 * value_ + 1;
                }
            }
            return *this;
        }

        iterator operator++(int)
        {
            iterator old = *this;
            ++*this;
            return old;
        }

        bool operator==(const iterator&) const = default;

        bool operator==(std::default_sentinel_t) const
        {
            return done_;
        }
    };

    iterator begin() const
    {
        return iterator{start_};
    }

    std::default_sentinel_t end() const
    {
        return std::default_sentinel;
    }
};

// 新的一套：ranges::distance（一共几个）
template <typename R>
long long new_length(const R& r)
{
    if constexpr (std::ranges::forward_range<const R>)
    {
        return std::ranges::distance(r);
    }
    return -1;
}

// 新的一套：ranges::max（空范围不能求 max，先判断）
template <typename R>
std::uint64_t new_max(const R& r)
{
    if constexpr (std::ranges::forward_range<const R>)
    {
        if (std::ranges::empty(r))
        {
            return 0;
        }
        return std::ranges::max(r);
    }
    return 0;
}

// 老的一套：先 views::common 变成 begin、end 同类型，再用 std::max_element（示例 [5]）
template <typename R>
std::uint64_t old_max(const R& r)
{
    if constexpr (std::ranges::forward_range<const R>)
    {
        auto c = r | std::views::common;
        if constexpr (legacy_at_least<decltype(c.begin()), std::forward_iterator_tag>)
        {
            auto it = std::max_element(c.begin(), c.end());
            return it == c.end() ? 0 : *it;
        }
    }
    return 0;
}

// 新的一套：views::filter 取奇数
template <typename R>
V64 new_odds(const R& r)
{
    if constexpr (std::ranges::forward_range<const R>)
    {
        return r | std::views::filter([](std::uint64_t x) { return x % 2 == 1; }) | std::ranges::to<V64>();
    }
    return {};
}

void test_exercise1()
{
    using It = Collatz::iterator;
    check(collect(Collatz{6}) == V64{6, 3, 10, 5, 16, 8, 4, 2, 1}, "练习 1：Collatz{6} → 6 3 10 5 16 8 4 2 1");
    check(collect(Collatz{1}) == V64{1} && collect(Collatz{0}) == V64{1}, "练习 1：Collatz{1}、Collatz{0} → 1");
    check(std::forward_iterator<It>, "练习 1：std::forward_iterator");
    check(std::sentinel_for<std::default_sentinel_t, It>, "练习 1：default_sentinel 是它的哨兵（sentinel_for）");
    check(!std::ranges::common_range<Collatz>, "练习 1：不是 common_range（begin、end 类型不同）");
    check(new_length(Collatz{27}) == 112 && new_max(Collatz{27}) == 9232, "练习 1：Collatz{27}：112 个，最大 9232");
    check(old_max(Collatz{7}) == 52, "练习 1：views::common + std::max_element（老的一套）");
    check(new_odds(Collatz{6}) == V64{3, 5, 1}, "练习 1：views::filter 奇数 → 3 5 1");
}

// ============================================================
// 练习 2：无限生成器 Powers{base}：1, base, base², base³ ……（示例 [6]）
//   iterator 已经差不多写好了，TODO：
//     ① 前置 ++ 的函数体：value_ 乘以 base_
//     ② Powers::end()：现在返回 begin()（所以是空的）。改成返回 std::unreachable_sentinel，让范围变成无限长
//        （返回类型也要改，看示例 [6]）
//     ③ first_powers(base, n)：前 n 个，用 views::take，结果用 std::ranges::to<V64>() 变成 vector
//     ④ powers_below(base, limit)：所有小于 limit 的，用 views::take_while
//   ③④ 的函数体都只要一行。base 保证 >= 2（base 是 1 的话 powers_below 永远停不下来：1 1 1 ……）
// ============================================================
class Powers
{
    std::uint64_t base_{};

public:
    explicit Powers(std::uint64_t base)
        : base_{base}
    {
    }

    class iterator
    {
        std::uint64_t value_{1};
        std::uint64_t base_{1};

    public:
        using iterator_concept = std::forward_iterator_tag;
        using iterator_category = std::forward_iterator_tag;
        using value_type = std::uint64_t;
        using difference_type = std::ptrdiff_t;

        iterator() = default;

        explicit iterator(std::uint64_t base)
            : base_{base}
        {
        }

        std::uint64_t operator*() const
        {
            return value_;
        }

        iterator& operator++()
        {
            value_ *= base_;
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
        return iterator{base_};
    }

    std::unreachable_sentinel_t end() const // TODO
    {
        return std::unreachable_sentinel;
    }
};

V64 first_powers(std::uint64_t base, std::size_t n)
{
    // TODO
    return Powers{base} | std::views::take(n) | std::ranges::to<V64>();
}

V64 powers_below(std::uint64_t base, std::uint64_t limit)
{
    // TODO
    return Powers{base} | std::views::take_while([limit](auto x)
    {
        return x < limit;
    }) | std::ranges::to<V64>();
}

void test_exercise2()
{
    check(std::same_as<decltype(Powers{2}.end()), std::unreachable_sentinel_t>, "练习 2：end() 返回 unreachable_sentinel_t");
    check(std::ranges::forward_range<Powers> && !std::ranges::sized_range<Powers>, "练习 2：forward_range，但没有 size（无限长）");
    check(collect(Powers{2}).size() == 1000, "练习 2：range-for 停不下来（collect 收满 1000 个）");

    Powers p{3};
    auto it = p.begin();
    ++it;
    ++it;
    check(*it == 9, "练习 2：Powers{3} 的第 3 个是 9");

    check(first_powers(3, 5) == V64{1, 3, 9, 27, 81}, "练习 2：first_powers(3, 5) → 1 3 9 27 81");
    check(first_powers(2, 0).empty() && first_powers(10, 1) == V64{1}, "练习 2：first_powers 0 个 / 1 个");
    check(powers_below(10, 1000) == V64{1, 10, 100}, "练习 2：powers_below(10, 1000) → 1 10 100");
    check(powers_below(2, 20) == V64{1, 2, 4, 8, 16}, "练习 2：powers_below(2, 20) → 1 2 4 8 16");
    check(powers_below(2, 1).empty() && powers_below(2, 2) == V64{1}, "练习 2：powers_below 边界（< limit，不含 limit）");
}

// ============================================================
// 练习 3：找 bug
//   tri_generator(n) 照着书上 fib_generator 的写法（既是范围又是迭代器，示例 [1]），
//   生成前 n 个三角形数：1 3 6 10 15 ……（第 k 个是 1 + 2 + …… + k）
//     step_ 是“下一次要加几”，sum_ 是当前的数：
//       sum_ = 1，step_ = 1 → ++ → step_ = 2，sum_ = 3 → ++ → step_ = 3，sum_ = 6 ……
//   运行后看到：什么都生成不出来。修好一个以后，n 大的时候最后一个数又不对。
//   一共两个 bug，每个改一行。
//   提示：示例 [1] —— end() 造出来的“结束标记”和 begin() 哪里一样、哪里不一样？== 比较的是哪个成员？
//         示例 [3] —— 第 400 个三角形数是 80200，tri_t 装得下吗？
// ============================================================
class tri_generator
{
    using tri_t = std::uint32_t; // 改动（补充）：对。uint32_t 最大 4294967295，第 400 个 80200 装得下（第 92681 个以后才溢出）。
    //   原来的 unsigned short 最大只有 65535，第 362 个（65703）就溢出了
    tri_t stop_{};
    tri_t count_{0};
    tri_t step_{1};
    tri_t sum_{1};

public:
    using iterator_concept = std::forward_iterator_tag;
    using iterator_category = std::forward_iterator_tag;
    using value_type = tri_t;
    using difference_type = std::ptrdiff_t;

    explicit tri_generator(tri_t stop = 0)
        : stop_{stop}
    {
    }

    tri_t operator*() const
    {
        return sum_;
    }

    tri_generator& operator++()
    {
        ++step_;
        sum_ += step_;
        ++count_;
        // 改动：删掉了你加的 --stop_;（题目说的两个 bug 不包括这里，这一行是多出来的第三处修改）
        //   结束的标记是 end() 造出来的“count_ == stop_ 的对象”：count_ 往上数，数到 stop_ 就结束，stop_ 不应该变。
        //   你的写法在 range-for 里碰巧没出问题：end() 一开始就造好了，后面 --stop_ 改的是 begin_ 这个复制品的 stop_，
        //   不影响已经造好的 end。所以原来的检查都通过。
        //   但先对生成器自己 ++ 再遍历就错了：g(5) ++ 两次后 count_ = 2，stop_ 却被减成了 3，
        //   end() 的 count_ 是 3，只走 1 步，只得到 6。正确的是 6 10 15（还剩 3 个，和示例 [2] 的 fib 一样）。
        //   下面新加的检查测的就是这个。看“思考题 1”那一行也能看出来：之前是“6 ；”，现在是“6 10 15 ；”
        return *this;
    }

    tri_generator operator++(int)
    {
        auto old{*this};
        ++*this;
        return old;
    }

    bool operator==(const tri_generator& o) const
    {
        return count_ == o.count_;
    }

    const tri_generator& begin() const
    {
        return *this;
    }

    tri_generator end() const
    {
        auto sentinel = tri_generator();
        sentinel.count_ = stop_;
        return sentinel;
    }
};

void test_exercise3()
{
    check(collect(tri_generator(5)) == V64{1, 3, 6, 10, 15}, "练习 3：tri_generator(5) → 1 3 6 10 15");
    check(collect(tri_generator(0)).empty() && collect(tri_generator(1)) == V64{1}, "练习 3：0 个 / 1 个");
    auto big = collect(tri_generator(400));
    check(big.size() == 400 && !big.empty() && big.back() == 80200, "练习 3：tri_generator(400)：400 个，最后一个是 80200");
    check(std::ranges::forward_range<tri_generator> && new_max(tri_generator(10)) == 55, "练习 3：ranges::max(tri_generator(10)) → 55");

    // 改动（补充）：先对生成器自己 ++ 两次再遍历，应该剩下后 3 个（书上的写法就是这样，示例 [2]）
    tri_generator g(5);
    ++g;
    ++g;
    check(collect(g) == V64{6, 10, 15}, "练习 3：g(5) 先 ++ 两次再遍历 → 6 10 15");
}

// ============================================================
// 思考题 1 用：运行后看输出（不用改）。书上的 fib_generator 的写法，换成三角形数
//   等练习 3 修好以后再看这一行
// ============================================================
void q1_demo()
{
    tri_generator g(5);
    ++g;
    ++g;
    std::print("思考题 1：g(5) 先 ++ 两次，遍历得到：");
    int shown = 0;
    for (auto v : g)
    {
        if (++shown > 20) // 练习 3 没修好时停不下来，最多打印 20 个
        {
            std::print("……停不下来");
            break;
        }
        std::print("{} ", v);
    }
    std::println("；遍历完以后 *g = {}", *g);
}

int main()
{
    test_exercise1();
    std::println("");
    test_exercise2();
    std::println("");
    test_exercise3();
    std::println("");
    q1_demo();
}

// ============================================================
// 思考题（每个小问都单独回答一行）
// （1）运行后看“思考题 1”那一行（练习 3 修好以后）。
//   (a) g(5) 先 ++ 两次再遍历，得到了几个数？为什么不是 5 个？（提示：begin() 返回的是什么？end() 的 count_ 是几？）
// 你的答案：begin()返回的是对象本身，是当前的状态。++两次之后，会从第三个开始。
// 改动（补充）：“从第三个开始”对，但题目问的“得到了几个、为什么不是 5 个”没答。
//   得到 3 个：6 10 15。g 自己 ++ 两次后，count_ = 2；end() 造出来的结束标记 count_ = stop_ = 5。
//   从 2 数到 5 只要走 3 步，所以只剩 3 个。生成器“已经用掉”的两个不会再算一遍。
//   （你加了 --stop_ 以后这里只得到 1 个，见练习 3 的改动）
//   (b) 遍历完以后 *g 是几？range-for 走完了，为什么 g 自己没变？（提示：示例 [2]，按 4.1 节的规则展开 range-for）
// 你的答案：6。
//auto&& range = g;
//auto begin = range.begin();
//auto end = range.end();
//
//for (; begin != end; ++begin)
//{
//    auto v = *begin;
//    std::println("{}", v);
//}
// 改动（补充）：6 对，展开也写对了。但“为什么 g 没变”要从展开里指出是哪一行：
//   auto begin = range.begin();   begin() 返回的是 const tri_generator&（g 本身的引用），
//                                 但左边写的是 auto，不是 auto&：auto 会把引用去掉，复制出一个新对象。
//   后面 ++begin 改的都是这个复制品，g 从头到尾没被碰过，所以 *g 还是 6。
//   这也是为什么示例 [2] 里同一个 fib_generator 能遍历两遍、两遍结果一样。
//
// （2）练习 1 的 Collatz 用的是 default_sentinel。
//   (a) std::max_element(r.begin(), r.end()) 为什么编译不过，而 std::ranges::max(r) 可以？
// 你的答案：前者是老算法。要求两个参数类型一样。后者可以，本来就是为此而设计的。
//   (b) views::common 做了什么，让 std::max_element 又能用了？
// 你的答案：把他们都包装成std::common_iterator。
// 改动（补充）：对，再说一句 common_iterator 是什么：它是一个“二选一”的类型，里面要么装着一个 Collatz::iterator，
//   要么装着一个 default_sentinel。begin() 和 end() 都返回 common_iterator，类型一样了，老算法就能接受。
//   两个 common_iterator 比较时，如果一个装的是迭代器、一个装的是哨兵，它就转去调用你写的 operator==(default_sentinel_t)。
//   (c) Collatz 为什么不能像示例 [5] 的 FibN 那样“数着个数”停下来，而要用 done_？
// 你的答案：没理解这个问题。
// 改动（补充）：问的是：FibN 的迭代器里存 left_（还剩几个），每次 ++ 减 1，减到 0 就结束。Collatz 为什么不这样做？
//   因为 FibN 的“生成几个”是用的人给的：FibN{10} 就是 10 个，一开始就知道。
//   Collatz{n} 一共几个数，不把整个序列算一遍就不知道（27 要走 111 步，28 只要 18 步，没有公式）。
//   “每个数最后都会到 1”本身就是一个没有被证明的猜想，所以只能边走边看：走到 1 就停。
//   那为什么不直接写 it == default_sentinel 就是 value_ == 1？
//   因为 range-for 每次先比较、再 *it：value_ 一变成 1，比较就说“结束了”，1 自己就没有被生成。
//   要做到“生成了 1 之后才结束”，就要多一个 done_ 记住“1 已经交出去过了”，在 1 之后的那次 ++ 才设成 true。
//
// （3）练习 2 的 Powers 是无限长的。（这一题只想、不要运行，运行的话程序停不下来）
//   (a) powers_below 如果把 views::take_while(x < limit) 换成 views::filter(x < limit)，还能停下来吗？为什么？
// 你的答案：不能，filter会一直找。
//   (b) std::ranges::max(Powers{2}) 会怎样？为什么 ranges::max 不能用在无限范围上，而 views::take 可以？
// 你的答案：他们需要走到end。后者不需要。
// 改动（补充）：方向对，补上“会怎样”和 take 为什么不需要：
//   ranges::max 要看过每一个元素才知道谁最大，它会一直 ++ 直到 it == end。unreachable_sentinel 和谁比都是 false，
//   所以程序永远不会结束（卡住，不报错）。Powers{2} 乘到 2^63 后再乘 2 会溢出变成 0，之后永远是 0，也停不下来。
//   views::take(n) 不需要 end：它自己数着，走了 n 步就当作结束，根本不去和原来的 end 比较。
//   而且 views::take 只是“包装”，造出来时一个元素都不算，真正遍历时才一个一个算（叫“惰性”）。
//   这和 ranges::max 是算法、一调用就要立刻算出结果，完全不同。
//
// ============================================================
