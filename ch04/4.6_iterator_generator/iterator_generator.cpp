// 第 4 章 4.6 创建迭代器生成器：边遍历边算出下一个值，不用容器存
// 编译：clang++ -std=c++23 -Wall -Wextra -Wpedantic iterator_generator.cpp -o iterator_generator.exe
//       g++     -std=c++23 -Wall -Wextra -Wpedantic iterator_generator.cpp -o iterator_generator.exe -lstdc++exp
//
// 文件结构：[1] 到 [7] 从上到下排列，每个编号只出现一次。
//   每一节里依次是：说明、这一节用到的类和函数、一个 sectionN() 函数（运行这一节）。main 依次调用 section1() 到 section7()。
//
// ============================================================
// “生成器”：一个迭代器（或范围），它的值不存在任何容器里，而是每次 ++ 的时候现算出来。
//   4.3 的 Seq（100 101 102 ……）就是最简单的生成器。这一节做斐波那契数列：
//     1 1 2 3 5 8 13 21 34 55 ……   每个数是前两个数的和
//   迭代器里只需要存“前两个数” a、b：
//       a   b         ++ 之后：新 b = a + b，新 a = 旧 b
//       0   1    *it = 1
//       1   1    *it = 1
//       1   2    *it = 2
//       2   3    *it = 3
//   不管要生成多少个，内存里永远只有两个数。
//
// 写生成器要回答的核心问题：“什么时候停？”这一节有三种答案：
//   [1] 书上的：数“已经生成了几个”，end() 造一个“计数已满”的对象，== 比较计数
//   [5] C++20：end() 返回一个专门的“哨兵”类型 std::default_sentinel_t，由迭代器自己判断停没停
//   [6] 不停：end() 返回 std::unreachable_sentinel，范围是无限长的，用的时候再 views::take(n) 截断
// ============================================================
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <print>
#include <ranges>
#include <type_traits>
#include <vector>
#if __has_include(<generator>)
#include <generator>
#endif

// ============================================================
// [1] 书上的 fib_generator（加上了书上“There's more”里的类型别名）
//   特别的地方：这个类“既是范围又是迭代器”：
//     它有 begin()/end()，所以能放进 range-for（是范围）
//     它有 *、++、==，所以 begin() 返回的“它自己”能当迭代器用
//   begin()：返回 *this（自己）。range-for 里 auto begin_ = r.begin(); 会复制一份，遍历的是复制品
//   end()  ：造一个新的 fib_generator，把它的 count_ 设成 stop_，当作“结束”的标记
//   ==     ：只比较 count_（已经生成了几个）。begin_ 每次 ++ 计数加 1，加到 stop_ 时就等于 end()
//
//   书上的小问题（下面的代码保留了书上的写法，用注释标出来）：
//     * operator!= 不用写：C++20 写了 == 就自动有 !=（4.3 节）
//     * end() 返回 const fib_generator（返回值加 const）：没有用，还会妨碍移动。返回 fib_generator 就行
//     * reference = const fib_t&、pointer = const fib_t*：和 4.4 节一样是错的，*it 返回的是值
//     * fib_t = unsigned long：Windows 上只有 32 位，第 48 个数就装不下了（[3]）
//     * size() 应该是 const 成员函数
// ============================================================
class fib_generator
{
    using fib_t = unsigned long;
    fib_t stop_{};
    fib_t count_{0};
    fib_t a_{0};
    fib_t b_{1};

    constexpr void do_fib()
    {
        const fib_t old_b = b_;
        b_ += a_;
        a_ = old_b;
    }

public:
    using iterator_concept = std::forward_iterator_tag;
    using iterator_category = std::forward_iterator_tag;
    using value_type = std::remove_cv_t<fib_t>;
    using difference_type = std::ptrdiff_t;
    using pointer = const fib_t*;   // 书上的，错的（见上面）
    using reference = const fib_t&; // 书上的，错的

    explicit fib_generator(fib_t stop = 0) // 不带参数时就是默认构造：forward 迭代器要求（4.4 节）
        : stop_{stop}
    {
    }

    fib_t operator*() const
    {
        return b_;
    }

    constexpr fib_generator& operator++()
    {
        do_fib();
        ++count_;
        return *this;
    }

    fib_generator operator++(int)
    {
        auto temp{*this};
        ++*this;
        return temp;
    }

    bool operator!=(const fib_generator& o) const // 书上的，C++20 不用写
    {
        return count_ != o.count_;
    }

    bool operator==(const fib_generator& o) const
    {
        return count_ == o.count_;
    }

    const fib_generator& begin() const
    {
        return *this;
    }

    const fib_generator end() const // 书上的，返回值的 const 没用
    {
        auto sentinel = fib_generator();
        sentinel.count_ = stop_;
        return sentinel;
    }

    fib_t size() // 书上的，应该加 const
    {
        return stop_;
    }
};

void section1()
{
    std::println("[1] 书上的 fib_generator");
    std::print("  fib_generator(10)：");
    for (auto v : fib_generator(10))
    {
        std::print("{} ", v);
    }
    std::println("");
}

// ============================================================
// [2] “既是范围又是迭代器”带来的两个怪现象
//   (a) begin() 返回的是“当前状态”：对生成器自己 ++ 两次，再遍历，就从第 3 个数开始，只剩 3 个
//       g(5)：count_ 已经是 2，end() 的 count_ 是 5，所以只走 3 步
//   (b) == 只比较 count_：两个毫不相干的生成器 a(10)、b(20)，各 ++ 一次，a == b 是 true
//       作为“和 end() 比较”这没问题，但它说明 == 的意思其实是“生成了一样多个”，而不是“是同一个位置”
//   所以更常见的写法是把“范围”和“迭代器”分成两个类（[5] 的做法，也是 4.3、4.4 的 Seq 的做法）：
//     范围里存“参数”（生成几个），迭代器里存“进度”（a、b），各管各的
// ============================================================
void section2()
{
    std::println("\n[2] 既是范围又是迭代器");
    fib_generator g(5);
    ++g;
    ++g;
    std::print("  g(5) 先 ++ 两次再遍历：");
    for (auto v : g)
    {
        std::print("{} ", v);
    }
    std::println("");

    fib_generator a(10);
    fib_generator b(20);
    ++a;
    ++b;
    std::println("  a(10)、b(20) 各 ++ 一次：a == b 是 {}", a == b);

    std::print("  同一个 fib_generator(5) 遍历两遍：");
    fib_generator f(5);
    for (auto v : f)
    {
        std::print("{} ", v);
    }
    std::print("| ");
    for (auto v : f) // begin() 返回的是复制品，f 本身没被改，所以第二遍一样
    {
        std::print("{} ", v);
    }
    std::println("");
}

// ============================================================
// [3] 书上没提的溢出：unsigned long 在 Windows 上是 32 位
//   Linux（书上用的）：unsigned long 是 64 位。Windows（两个编译器都是）：32 位，最大 4294967295
//   第 47 个斐波那契数 2971215073 还装得下，第 48 个 4807526976 就装不下了
//   无符号整数溢出不是错误，也没有警告：它“绕回来”（对 2^32 取余），算出一个看起来正常的错数
//   办法：用宽度固定的 std::uint64_t（<cstdint>），任何平台都是 64 位，能撑到第 93 个
//   规律：要“固定位数”的整数时，用 std::int32_t、std::uint64_t 这类，不要用 long、unsigned long
// ============================================================
void section3()
{
    std::println("\n[3] 溢出");
    std::println("  sizeof(unsigned long) = {}，sizeof(std::uint64_t) = {}", sizeof(unsigned long), sizeof(std::uint64_t));
    unsigned long last = 0;
    for (auto v : fib_generator(50))
    {
        last = v;
    }
    std::println("  fib_generator(50) 的最后一个：{}（错的，第 50 个应该是 12586269025）", last);
}

// ============================================================
// [4] 书上的“There's more”：加上类型别名后，标准库的算法、views 都能用
//   书上的 printc(x, "squared:") 输出的是 "squared:: "：printc 自己会加 ": "，描述里不该再写冒号
//   这一次书上写了后置 it++，所以 C++20 概念是满足的（4.4 节书上的 Seq 就漏了）
// ============================================================
void section4()
{
    std::println("\n[4] 配合算法和 views");
    std::println("  forward_range {}，sized_range {}", std::ranges::forward_range<fib_generator>,
                 std::ranges::sized_range<fib_generator>);
    fib_generator fib(10);
    auto squared = std::views::transform(fib, [](unsigned long x) { return x * x; });
    std::println("  views::transform 平方：{}", squared);
    std::println("  std::max_element（老算法）：{}", *std::max_element(fib.begin(), fib.end()));
    std::println("  ranges::to<vector>：      {}", fib | std::ranges::to<std::vector>());
}

// ============================================================
// [5] C++20 的哨兵：end() 不必是迭代器
//   书上为了表示“结束”，在 end() 里专门造了一个完整的 fib_generator，只为了用它的 count_。
//   C++20 允许 end() 返回另一种类型，叫“哨兵”（sentinel）：它不需要能 *、能 ++，只需要能和迭代器 == 比较。
//   标准库现成的哨兵类型 std::default_sentinel_t（<iterator>），对象是 std::default_sentinel，里面什么都没有。
//   “停没停”由迭代器自己判断：迭代器里存着还剩几个，剩 0 个时，it == std::default_sentinel 就是 true
//
//     FibN 范围：存“生成几个”n_
//       begin() → FibN::iterator{n_}        迭代器存 a、b 和 left_（还剩几个）
//       end()   → std::default_sentinel     什么都不存
//     it == default_sentinel  ⇔  it.left_ == 0
//
//   谁能用：
//     range-for：能（C++17 起 begin、end 可以是不同类型）
//     ranges 算法、views：能（它们本来就是按“迭代器 + 哨兵”设计的）
//     老算法 std::max_element(first, last)、vector(first, last)：不能，要求两个参数类型一样：
//       clang：error: no matching function for call to 'max_element'
//       g++  ：error: no matching function for call to 'max_element(FibN::iterator, std::default_sentinel_t)'
//     办法：r | std::views::common，把它包装成 begin、end 类型一样的范围（common_range）
//       包装后的迭代器是 std::common_iterator，它“要么装着一个迭代器，要么装着一个哨兵”
//       注意：迭代器要写 iterator_category，否则 clang（libc++）会把 common_iterator 当成 input，
//       std::max_element 还是报 requires a ForwardIterator（g++ 不检查）
// ============================================================
class FibN
{
    int n_{};
    
public:
    explicit FibN(int n)
        : n_{n < 0 ? 0 : n}
    {
    }

    class iterator
    {
        std::uint64_t a_{0};
        std::uint64_t b_{1};
        int left_{0};

    public:
        using iterator_concept = std::forward_iterator_tag;
        using iterator_category = std::forward_iterator_tag;
        using value_type = std::uint64_t;
        using difference_type = std::ptrdiff_t;

        iterator() = default;

        explicit iterator(int left)
            : left_{left}
        {
        }

        std::uint64_t operator*() const
        {
            return b_;
        }

        iterator& operator++()
        {
            const auto old_b = b_;
            b_ += a_;
            a_ = old_b;
            --left_;
            return *this;
        }

        iterator operator++(int)
        {
            iterator old = *this;
            ++*this;
            return old;
        }

        bool operator==(const iterator&) const = default; // forward 迭代器要求迭代器之间能比较

        bool operator==(std::default_sentinel_t) const // 和哨兵比较：还剩 0 个就是结束了
        {
            return left_ == 0;
        }
    };

    iterator begin() const
    {
        return iterator{n_};
    }

    std::default_sentinel_t end() const
    {
        return std::default_sentinel;
    }
};

void section5()
{
    std::println("\n[5] 哨兵 default_sentinel");
    FibN r{10};
    std::print("  range-for：");
    for (auto v : r)
    {
        std::print("{} ", v);
    }
    std::println("");
    std::println("  forward_range {}，common_range（begin、end 同类型）{}", std::ranges::forward_range<FibN>,
                 std::ranges::common_range<FibN>);
    std::println("  ranges::max：{}，ranges::distance：{}", std::ranges::max(r), std::ranges::distance(r));
    // std::max_element(r.begin(), r.end());  // 编译错误：两个参数类型不一样
    auto c = r | std::views::common;
    std::println("  views::common 之后 std::max_element：{}", *std::max_element(c.begin(), c.end()));
    std::vector<std::uint64_t> v(c.begin(), c.end());
    std::println("  views::common 之后 vector(first, last)：{}", v);

    std::uint64_t last = 0;
    for (auto x : FibN{50})
    {
        last = x;
    }
    std::println("  FibN{{50}} 的最后一个：{}（uint64_t，对了）", last);
}

// ============================================================
// [6] 无限生成器：std::unreachable_sentinel
//   斐波那契数列本来就没有尽头。干脆不在生成器里决定“生成几个”，让 end() 返回 std::unreachable_sentinel：
//   它和任何迭代器比较都是 false，永远到不了，所以范围是无限长的。
//   用的时候再截断：
//     views::take(n)           ：只要前 n 个
//     views::take_while(条件)  ：条件第一次不成立就停（注意不是 filter：filter 会一直往后找，永远停不下来）
//   好处：生成器只管“怎么算下一个”，“要多少”交给使用者，同一个生成器怎么截断都行
//   不能对无限范围用 ranges::max、ranges::distance、ranges::to：它们要走到 end，永远走不到
// ============================================================
class Fib
{
public:
    class iterator
    {
        std::uint64_t a_{0};
        std::uint64_t b_{1};

    public:
        using iterator_concept = std::forward_iterator_tag;
        using iterator_category = std::forward_iterator_tag;
        using value_type = std::uint64_t;
        using difference_type = std::ptrdiff_t;

        std::uint64_t operator*() const
        {
            return b_;
        }

        iterator& operator++()
        {
            const auto old_b = b_;
            b_ += a_;
            a_ = old_b;
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
        return iterator{};
    }

    std::unreachable_sentinel_t end() const
    {
        return std::unreachable_sentinel;
    }
};

void section6()
{
    std::println("\n[6] 无限生成器 unreachable_sentinel");
    std::println("  forward_range {}，sized_range {}", std::ranges::forward_range<Fib>, std::ranges::sized_range<Fib>);
    std::println("  Fib{{}} | take(10)：            {}", Fib{} | std::views::take(10));
    std::println("  Fib{{}} | take_while(< 1000)：  {}", Fib{} | std::views::take_while([](auto x) { return x < 1000; }));
    std::println("  偶数里的前 5 个（filter + take）：{}",
                 Fib{} | std::views::filter([](auto x) { return x % 2 == 0; }) | std::views::take(5));
    std::println("  第 93 个（uint64_t 能装下的最后一个）：{}", *std::ranges::next(Fib{}.begin(), 92));
}

// ============================================================
// [7] 不想自己写迭代器时：标准库现成的生成器
//   views::iota(1)：1 2 3 …… 无限的整数（4.3 节），配合 transform 就能生成“第 n 个是 f(n)”的数列
//     但斐波那契每个数依赖前两个，不是 f(n) 的形式，iota + transform 写不出来
//   C++23 std::generator（<generator>）：用“协程”写生成器，像写普通循环一样，每次 co_yield 交出一个值：
//       std::generator<std::uint64_t> fib()
//       {
//           std::uint64_t a = 0, b = 1;
//           while (true)
//           {
//               co_yield b;            // 交出 b，函数在这里“暂停”，下次 ++ 时从这里接着运行
//               auto old_b = b; b += a; a = old_b;
//           }
//       }
//     g++（libstdc++）有；clang 用的 libc++ 现在还没有 std::generator，所以下面用 #if 判断。
//     这一行的输出两个编译器不一样
//     协程是第 10 章以后的内容，这里只看个样子：它的迭代器是 input（只能走一遍），比自己写的弱
// ============================================================
#if defined(__cpp_lib_generator)
std::generator<std::uint64_t> fib_coroutine()
{
    std::uint64_t a = 0;
    std::uint64_t b = 1;
    while (true)
    {
        co_yield b;
        const auto old_b = b;
        b += a;
        a = old_b;
    }
}
#endif

void section7()
{
    std::println("\n[7] 现成的生成器");
    auto squares = std::views::iota(1) | std::views::transform([](int n) { return n * n; });
    std::println("  iota(1) | transform(n*n) | take(6)：{}", squares | std::views::take(6));
#if defined(__cpp_lib_generator)
    std::println("  std::generator | take(10)：{}", fib_coroutine() | std::views::take(10));
#else
    std::println("  这个标准库还没有 std::generator（libc++）");
#endif
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
