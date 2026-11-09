// 第 4 章 4.4 让自己写的迭代器和标准库兼容：迭代器特性（traits）与 C++20 迭代器概念
// 编译：clang++ -std=c++23 -Wall -Wextra -Wpedantic iterator_traits.cpp -o iterator_traits.exe
//       g++     -std=c++23 -Wall -Wextra -Wpedantic iterator_traits.cpp -o iterator_traits.exe -lstdc++exp
//
// 文件结构：[1] 到 [7] 从上到下排列，每个编号只出现一次。
//   每一节里依次是：说明、这一节用到的类和函数、一个 sectionN() 函数（运行这一节）。main 依次调用 section1() 到 section7()。
//
// ============================================================
// 问题：4.3 的 Seq 能放进 range-for，但标准库的算法不认它：
//     Seq<int> r{100, 110};
//     std::minmax_element(r.begin(), r.end());   // 编译错误
//     std::ranges::max(r);                       // 编译错误
//   range-for 只需要 *、++、!=。算法要的更多：它们要先“打听”迭代器的情况 ——
//   元素是什么类型？两个迭代器的距离用什么类型？能不能往回走？能不能多趟遍历？
//
// 标准库里有“两套”打听的办法，自己写的迭代器两套都要照顾到：
//
//   老的一套（C++98 起）：std::iterator_traits<It>     （<iterator>）
//     用的人：std::minmax_element、std::max_element、std::prev、std::distance、vector(first, last) ……
//             也就是不带 ranges:: 的老算法
//     做法：iterator_traits<It> 去读迭代器类里的几个类型别名（using xxx = ...;）：
//       iterator_category   类别“标签”，比如 std::forward_iterator_tag（[5] 讲标签）
//       value_type          元素类型
//       difference_type     距离类型，一般是 std::ptrdiff_t
//       reference           *it 的类型
//       pointer             it-> 的类型
//     为什么要多一层 iterator_traits，而不是直接读 It::value_type？
//       因为指针也是迭代器，但 int* 不是类，里面不能写 using。
//       iterator_traits<int*> 是标准库专门为指针写好的版本，直接给出 value_type = int 等等。
//
//   新的一套（C++20）：概念 std::forward_iterator<It> 等（4.1 讲过）
//     用的人：std::ranges:: 的算法（ranges::max、ranges::minmax_element ……）、views（filter、reverse ……）
//     做法：主要检查“操作”能不能写：*it、++it、it++、it == it2、默认构造 It{} ……
//           类型只看 value_type、difference_type 两个，再加一个可选的 iterator_concept 标签
//
// 要两套都能用，迭代器需要（[4] 是我一个个删掉试出来的表）：
//
//   要加的东西            缺了会怎样
//   --------------------  ---------------------------------------------------------------------
//   value_type            input_iterator 就不成立了（不知道元素类型）
//   difference_type       连最低的 input_or_output_iterator 都不成立
//   后置 it++             连最低的 input_or_output_iterator 都不成立（书上漏了这个！[2]）
//   ==                    forward_iterator 不成立；书上 4.3 只写了 !=，C++20 写 == 就够了（4.3 节讲过）
//   默认构造 iterator()   forward_iterator 不成立（只能是 input）
//   iterator_concept      C++20 的类别标签。可以不写：不写时，按操作判断（有 -- 就是双向……）
//   iterator_category     老算法看的类别标签。不写的话，老算法会把它当成 input 迭代器（[6]）
//   reference、pointer    可以不写，iterator_traits 会自己推导。书上写的 reference = const T& 是错的（[7]）
//
//   [3] 的完整版 Seq 就是按这张表写的，自己写迭代器时照着它抄
// ============================================================
#include <algorithm>
#include <cstddef>
#include <forward_list>
#include <iterator>
#include <list>
#include <print>
#include <ranges>
#include <string_view>
#include <type_traits>
#include <vector>

// ============================================================
// [1] 4.3 的 Seq（已经按 C++20 写成了 ==），什么都没加：range-for 能用，标准库不认
//   std::minmax_element(r.begin(), r.end())（老的一套）：
//     clang：error: static assertion failed due to requirement
//            'std::is_convertible<std::__nat, std::forward_iterator_tag>::value': std::minmax_element requires a ForwardIterator
//            （__nat 是 libc++ 内部的“不是类型”，意思是 iterator_traits 打听不到类别）
//     g++  ：居然能编译运行 —— libstdc++ 的 minmax_element 不检查类别，只管用 ++ 和 *。
//            书上说“某些系统能编译，Clang 不行”就是这个现象。但不能指望它
//   std::ranges::max(r)（新的一套）两个编译器都报错：
//     clang：note: because 'Seq43<int> &' does not satisfy 'input_range'
//     g++  ：the required expression 'std::ranges::_Cpo::begin(__t)' is invalid
// ============================================================
template <typename T>
class Seq43
{
    T start_{};
    T end_{};

public:
    Seq43(T start, T end)
        : start_{start}, end_{end}
    {
    }

    class iterator
    {
        T value_{};

    public:
        explicit iterator(T position = T{})
            : value_{position}
        {
        }

        T operator*() const
        {
            return value_;
        }

        iterator& operator++()
        {
            ++value_;
            return *this;
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

void section1()
{
    std::println("[1] 4.3 的 Seq：range-for 能用");
    Seq43<int> r{100, 105};
    std::print("  ");
    for (auto v : r)
    {
        std::print("{} ", v);
    }
    std::println("");
    std::println("  但 std::ranges::range<Seq43<int>> 是 {}", std::ranges::range<Seq43<int>>);
}

// ============================================================
// [2] 书上的版本：加了 6 个类型别名和 ==，但是没有后置 it++
//   老算法能用，但 C++20 概念一个都不满足
//   原因：没有后置 it++。C++20 的 weakly_incrementable（所有迭代器的最低要求）要求 it++ 能写
//   所以书上的 static_assert(ranges::forward_range<Seq<int>>) 和 ranges::minmax_element(r) 都编译不过：
//     clang：note: because 'SeqBook<int> &' does not satisfy 'input_range'
//     g++  ：the required expression 'std::ranges::_Cpo::begin(__t)' is invalid
//     （ranges::begin(r) 要求返回的东西是迭代器，SeqBook 的迭代器不算，于是 begin 都“不能调用”）
//   老算法 std::minmax_element 只读 iterator_traits 里的类型别名，内部只用 ++it，所以能用
//   （书上的 GitHub 代码里可能有后置 ++，但书里印出来的代码没有）
// ============================================================
template <typename T>
class SeqBook
{
    T start_{};
    T end_{};

public:
    SeqBook(T start, T end)
        : start_{start}, end_{end}
    {
    }

    class iterator
    {
        T value_{};

    public:
        using iterator_concept = std::forward_iterator_tag;
        using iterator_category = std::forward_iterator_tag;
        using value_type = std::remove_cv_t<T>;
        using difference_type = std::ptrdiff_t;
        using pointer = const T*;   // 这两个和 operator* 对不上，[7] 细讲
        using reference = const T&;

        explicit iterator(T position = T{})
            : value_{position}
        {
        }

        T operator*() const
        {
            return value_;
        }

        iterator& operator++()
        {
            ++value_;
            return *this;
        }

        bool operator==(const iterator& other) const
        {
            return value_ == other.value_;
        }
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

void section2()
{
    std::println("\n[2] 书上的版本");
    SeqBook<int> r{100, 110};
    auto [min_it, max_it] = std::minmax_element(r.begin(), r.end());
    std::println("  std::minmax_element（老算法）：{} - {}", *min_it, *max_it);
    std::println("  std::ranges::forward_range<SeqBook<int>> 是 {}", std::ranges::forward_range<SeqBook<int>>);
}

// ============================================================
// [3] 推荐的完整版本：老算法、ranges 算法、views 全都能用
//   书上的 printc 约束写的是 forward_iterator<typename T::iterator>：
//   和 4.1 一样的问题，原生数组没有 ::iterator。约束 std::ranges::forward_range 更好（下面的 printc）
// ============================================================
template <typename T>
class Seq
{
    T start_{};
    T end_{};

public:
    Seq(T start, T end)
        : start_{start}, end_{end}
    {
    }

    class iterator
    {
        T value_{};

    public:
        // 习惯上把这些类型别名放在 public 的最前面，一眼就能看到
        using iterator_concept = std::forward_iterator_tag;  // 给 C++20 概念看：我是 forward
        using iterator_category = std::forward_iterator_tag; // 给老算法（iterator_traits）看：我是 forward
        using value_type = T;                                // 元素类型
        using difference_type = std::ptrdiff_t;              // 距离类型
        // reference、pointer 不写，iterator_traits 自己推导：reference = decltype(*it) = T，pointer = void

        iterator() = default; // 默认构造：forward 要求（[4] 的表）

        explicit iterator(T position)
            : value_{position}
        {
        }

        T operator*() const
        {
            return value_;
        }

        iterator& operator++() // 前置 ++it：先加，返回加完的自己
        {
            ++value_;
            return *this;
        }

        iterator operator++(int) // 后置 it++：参数 int 只是用来区分前置和后置，不用它
        {                        // 先存一份旧的，再加，返回旧的那份（复制品，所以返回类型不带 &）
            iterator old = *this;
            ++*this;             // 直接调用上面的前置 ++，不重复写一遍
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

template <std::ranges::forward_range R>
void printc(const R& r)
{
    std::print("  ");
    for (auto v : r)
    {
        std::print("{} ", v);
    }
    std::println("");
}

void section3()
{
    std::println("\n[3] 完整版 Seq");
    Seq<int> r{100, 110};
    static_assert(std::ranges::forward_range<Seq<int>>);

    auto [a, b] = std::minmax_element(r.begin(), r.end());
    std::println("  std::minmax_element：        {} - {}", *a, *b);
    auto [mn, mx] = std::ranges::minmax_element(r);
    std::println("  std::ranges::minmax_element：{} - {}", *mn, *mx);
    std::println("  std::ranges::max：           {}", std::ranges::max(r));
    std::println("  std::ranges::count_if 偶数： {}", std::ranges::count_if(r, [](int x) { return x % 2 == 0; }));

    std::vector<int> v(r.begin(), r.end()); // vector 的这个构造函数也是“老的一套”，要打听类别
    std::println("  vector(r.begin(), r.end())： {}", v);

    auto threes = r | std::views::filter([](int x) { return x % 3 == 0; }) | std::ranges::to<std::vector>();
    std::println("  views::filter 3 的倍数：     {}", threes);

    printc(r);
    int arr[]{1, 2, 3};
    printc(arr); // 书上的 printc 不能接受数组（4.1 节讲过）
}

// ============================================================
// [4] 一个个删掉试：每样东西缺了，C++20 概念会怎样
//   我写了 5 个迭代器，都从 [3] 的完整版开始，每个只删掉一样东西，结果（两个编译器一样）：
//
//     删掉                 input_or_output  input   forward  bidirectional
//     （什么都不删）        true             true    true     false（没有 --）
//     value_type           true             false   false    false
//     difference_type      false            false   false    false
//     后置 it++             false            false   false    false
//     默认构造              true             true    false    false
//     == 换成只写 !=        true             true    false    false
//
//   下面打印的是 [1][2][3] 三个实际的类型
//   （ioi = input_or_output_iterator，所有迭代器的最低要求）
// ============================================================
template <typename It>
void concept_row(std::string_view name)
{
    std::println("  {:<16} {:<5} {:<5} {:<5} {:<5}", name, std::input_or_output_iterator<It>, std::input_iterator<It>,
                 std::forward_iterator<It>, std::bidirectional_iterator<It>);
}

void section4()
{
    std::println("\n[4] 满足哪些 C++20 概念");
    std::println("  {:<16} {:<5} {:<5} {:<5} {:<5}", "", "ioi", "input", "fwd", "bidi");
    concept_row<Seq43<int>::iterator>("[1] 4.3 的 Seq");
    concept_row<SeqBook<int>::iterator>("[2] 书上的");
    concept_row<Seq<int>::iterator>("[3] 完整版");
}

// ============================================================
// [5] iterator_traits 和标签分派
//   iterator_traits<int*> 是标准库专门给指针写的，所以指针不用（也没法）写类型别名
//
//   老的一套怎么用类别标签？—— 函数重载，参数是标签类型（叫“标签分派”）
//   标签是空的结构体，它们之间是继承关系：
//       input_iterator_tag
//         └ forward_iterator_tag
//             └ bidirectional_iterator_tag
//                 └ random_access_iterator_tag
//                     └ contiguous_iterator_tag      （C++20 新加）
//   下面的 pick 只有 input、bidirectional 两个重载：
//     传一个 forward_iterator_tag 进来：没有正好匹配的重载，就转换成它的父类 input_iterator_tag，选第一个
//     传一个 random_access_iterator_tag 进来：两个都能接受，选离它近的父类 bidirectional
//   C++20 之前，std::distance、std::advance 就是这么按类别选实现的
//
//   vector 迭代器在老的一套里只是 random_access：contiguous_iterator_tag 是 C++20 才加的，
//   只出现在 iterator_concept 里（老代码不认识这个新标签）
// ============================================================
std::string_view pick(std::input_iterator_tag)
{
    return "input 版本";
}

std::string_view pick(std::bidirectional_iterator_tag)
{
    return "bidirectional 版本";
}

template <typename It>
std::string_view pick_for()
{
    using Tag = typename std::iterator_traits<It>::iterator_category; // 打听 It 的类别
    return pick(Tag{});                                               // 造一个空的标签对象传进去，靠重载选函数
}

// 判断 iterator_traits 给出的类别标签（[6] 也用）
template <typename It>
std::string_view traits_category()
{
    using C = typename std::iterator_traits<It>::iterator_category;
    if constexpr (std::is_same_v<C, std::random_access_iterator_tag>)
    {
        return "random_access_iterator_tag";
    }
    else if constexpr (std::is_same_v<C, std::bidirectional_iterator_tag>)
    {
        return "bidirectional_iterator_tag";
    }
    else if constexpr (std::is_same_v<C, std::forward_iterator_tag>)
    {
        return "forward_iterator_tag";
    }
    else if constexpr (std::is_same_v<C, std::input_iterator_tag>)
    {
        return "input_iterator_tag";
    }
    else
    {
        return "其它";
    }
}

void section5()
{
    std::println("\n[5] iterator_traits 和标签");
    static_assert(std::same_as<std::iterator_traits<int*>::value_type, int>);
    static_assert(std::same_as<std::iterator_traits<int*>::difference_type, std::ptrdiff_t>);
    std::println("  int*                    的类别标签：{}", traits_category<int*>());
    std::println("  vector<int>::iterator   的类别标签：{}", traits_category<std::vector<int>::iterator>());
    std::println("  list<int>::iterator     的类别标签：{}", traits_category<std::list<int>::iterator>());
    std::println("  完整版 Seq<int>::iterator 的类别标签：{}", traits_category<Seq<int>::iterator>());

    std::println("  pick：forward_list → {}", pick_for<std::forward_list<int>::iterator>()); // forward 标签 → 父类 input
    std::println("  pick：list         → {}", pick_for<std::list<int>::iterator>());
    std::println("  pick：vector       → {}", pick_for<std::vector<int>::iterator>());          // random_access → 最近的父类 bidirectional
    std::println("  pick：int*         → {}", pick_for<int*>());
}

// ============================================================
// [6] 只照顾一套的后果：BackSeq 有 -- 能往回走，但没写任何标签
//   新的一套全都能用：没写 iterator_concept 时，C++20 按操作判断，有 -- 就是双向
//   老的一套出问题：没写 iterator_category，iterator_traits 只能自己推导，
//     而 *it 返回的是值（int）不是引用（int&），老规矩里这种迭代器最多算 input，于是推导成 input_iterator_tag
//   结果老算法把它当成只能往前走一遍的迭代器：
//     std::prev(r.end())：
//       clang（libc++）：编译错误 static assertion failed: Attempt to prev(it) with a non-bidirectional iterator
//       g++（libstdc++）：能编译，但走的是 input 版本，运行时断言失败直接终止：Assertion '__n >= 0' failed
//     std::max_element(r.begin(), r.end())：
//       clang：编译错误 std::max_element requires a ForwardIterator
//       g++  ：能用
//   所以两个标签都写上最省心（书上就是这么做的）。
//   （冷知识：严格按老规矩，*it 返回值的迭代器只能声明成 input。标准库的 views::iota 就是
//     iterator_concept = random_access_iterator_tag、iterator_category = input_iterator_tag。
//     实际上声明成 forward 在两个库里都能正常用，书上也是这样写的，学习阶段不用管这个区别。）
// ============================================================
class BackSeq
{
    int start_{};
    int end_{};

public:
    BackSeq(int start, int end)
        : start_{start}, end_{end}
    {
    }

    class iterator
    {
        int value_{};

    public:
        using value_type = int;
        using difference_type = std::ptrdiff_t;

        iterator() = default;

        explicit iterator(int position)
            : value_{position}
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

void section6()
{
    std::println("\n[6] 没写标签的 BackSeq");
    BackSeq r{1, 6};
    std::println("  C++20：bidirectional_iterator {}，views::reverse：{}", std::bidirectional_iterator<BackSeq::iterator>,
                 r | std::views::reverse | std::ranges::to<std::vector>());
    std::println("  std::ranges::prev(r.end()) → {}（新的一套，没问题）", *std::ranges::prev(r.end()));
    std::println("  老的一套 iterator_traits 推导出的类别：{}", traits_category<BackSeq::iterator>());
    using IotaIt = std::ranges::iterator_t<std::ranges::iota_view<int, int>>;
    std::println("  对比 views::iota 的迭代器：iterator_category 是 {}", traits_category<IotaIt>());
}

// ============================================================
// [7] 书上写的 reference = const T&、pointer = const T* 是错的
//   *it 实际返回的是 T（一个值），不是 const T&。
//   C++20 的 std::iter_reference_t<It> 直接看 decltype(*it)，不看这个别名，所以得到 int；
//   老的 iterator_traits<It>::reference 照抄别名，得到 const int& —— 两套说法不一样，后者是假的。
//   要写就写 using reference = T;（和 operator* 一致），或者干脆不写，让 iterator_traits 推导（[3] 的做法）
// ============================================================
void section7()
{
    std::println("\n[7] reference 别名");
    using BookIt = SeqBook<int>::iterator;
    using GoodIt = Seq<int>::iterator;
    static_assert(std::same_as<std::iter_reference_t<BookIt>, int>);                 // 真实的 *it 类型
    static_assert(std::same_as<std::iterator_traits<BookIt>::reference, const int&>); // 别名说的（不对）
    static_assert(std::same_as<std::iterator_traits<GoodIt>::reference, int>);        // 不写别名，推导出来是对的
    static_assert(std::same_as<std::iterator_traits<GoodIt>::pointer, void>);
    std::println("  static_assert 全部通过：书上的 reference 别名是 const int&，但 *it 实际是 int");
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
