// 第 4 章 4.8 用哨兵遍历长度未知的东西：C 字符串
// 编译：clang++ -std=c++23 -Wall -Wextra -Wpedantic sentinel.cpp -o sentinel.exe
//       g++     -std=c++23 -Wall -Wextra -Wpedantic sentinel.cpp -o sentinel.exe -lstdc++exp
//
// 文件结构：[1] 到 [7] 从上到下排列，每个编号只出现一次。
//   每一节里依次是：说明、这一节用到的类和函数、一个 sectionN() 函数（运行这一节）。main 依次调用 section1() 到 section7()。
//
// ============================================================
// 有些东西事先不知道有多长，只有走到某个“结束记号”才知道到头了。最常见的就是 C 字符串：
//     "string" 在内存里是 7 个 char：'s' 't' 'r' 'i' 'n' 'g' '\0'
//   最后的 '\0'（值是 0 的字符，叫“空字符”）就是结束记号。只拿到一个 const char*，
//   不从头数一遍，就不知道它有多长（strlen 就是从头数到 '\0'）。
//
// 4.6 节已经见过“哨兵”（sentinel）：end() 返回的不是迭代器，而是另一种类型的对象，
//   它只负责回答“迭代器到头了没有”，也就是 it == 哨兵 是 true 还是 false。
//   这一节就用哨兵来遍历 C 字符串：不用先 strlen，走到 '\0' 自然停下。
//
//   [1] 字符串字面量里藏着的 '\0'
//   [2] 书上的写法 cstr_it（只够 range-for 用）
//   [3] 书上写法的问题
//   [4] C++20 的正确写法：迭代器 + 空的哨兵类型，ranges 算法、views 全能用
//   [5] 更省事：指针本身就是迭代器，只写一个哨兵 + std::ranges::subrange
//   [6] 同一个思路的其它“结束记号”：-1 结尾的数组、nullptr 结尾的指针数组
//   [7] 哨兵范围不知道自己的长度：没有 size()，只有 ranges::distance
// ============================================================
#include <algorithm>
#include <cctype>
#include <cstddef>
#include <functional>
#include <iterator>
#include <print>
#include <ranges>
#include <string>
#include <string_view>

// ============================================================
// [1] 字符串字面量 "string" 的类型是 const char[7]：6 个字母 + 1 个 '\0'
//   用 range-for 遍历一个数组，会把 7 个元素全走一遍，'\0' 也在里面（书上的输出最后有个 00）
//   {:02x}：用两位十六进制打印，不够两位前面补 0
// ============================================================
void section1()
{
    std::println("[1] 字符串字面量");
    std::print("  for (char c : \"string\")：");
    for (char c : "string")
    {
        std::print("{:02x} ", c);
    }
    std::println("");
    std::println("  sizeof(\"string\") = {}，std::size(\"string\") = {}（都算上了 '\\0'）", sizeof("string"), std::size("string"));
    std::println("  std::string_view{{\"ab\\0cd\"}}.size() = {}（从 const char* 构造时，遇到 '\\0' 就认为结束了）",
                 std::string_view{"ab\0cd"}.size());
}

// ============================================================
// [2] 书上的 cstr_it：一个类，既是范围又是迭代器（和 4.6 节书上的 fib_generator 一样的套路）
//   哨兵类型 sentinel_t = const char，end() 返回 '\0'
//   range-for 展开后是 for (; begin_ != end_; ++begin_)，所以书上只写了 !=：
//     it != 哨兵  ⇔  s 不是空指针，并且 *s 不是 '\0'
//   参数只写了类型，没写名字：bool operator!=(sentinel_t) const —— 用不到它的值，只要求类型对得上
//   书上的 main 里，"array" 和 "c-string" 打印出来最后都没有 00：遇到 '\0' 就停了，'\0' 自己没有被遍历
// ============================================================
using sentinel_t = const char;
constexpr sentinel_t nullchar = '\0';

class cstr_it
{
    const char* s{};

public:
    explicit cstr_it(const char* str)
        : s{str}
    {
    }

    char operator*() const
    {
        return *s;
    }

    cstr_it& operator++()
    {
        ++s;
        return *this;
    }

    bool operator!=(sentinel_t) const
    {
        return s != nullptr && *s != nullchar;
    }

    cstr_it begin() const
    {
        return *this;
    }

    char end() const // 书上写的是 sentinel_t end() const，也就是返回 const char，g++ 会警告（[3] (b)）
    {
        return nullchar;
    }
};

void print_cstr(const char* s)
{
    std::print("  {}: ", s);
    for (char c : cstr_it(s))
    {
        std::print("{:02x} ", c);
    }
    std::println("");
}

void section2()
{
    std::println("\n[2] 书上的 cstr_it");
    const char carray[]{"array"};
    print_cstr(carray);
    const char* cstr{"c-string"};
    print_cstr(cstr);
}

// ============================================================
// [3] 书上写法的问题
//   (a) 哨兵类型用 const char 不好：任何 char 都能当“哨兵”传进来，比如 it != 'x' 也能编译，
//       而且函数根本不看参数的值，结果和 it != '\0' 一样。int 也能隐式转成 char，it != 65 也能编译。
//       哨兵应该是一个专门的、空的类型（[4] 的 cstr_end），这样只有它能和迭代器比较
//   (b) 书上 end() 的返回类型是 sentinel_t，也就是 const char：返回值上的 const 没有意义，g++ 加 -Wextra 会警告：
//         warning: type qualifiers ignored on function return type [-Wignored-qualifiers]
//       所以上面 [2] 的代码里，我把 end() 的返回类型改成了 char
//   (c) 只写了 !=：C++20 不会用 != 改写出 ==（4.3 节），it == '\0' 编译不过：
//         clang：error: invalid operands to binary expression ('cstr_it' and 'char')
//         g++  ：error: no match for 'operator==' (operand types are 'cstr_it' and 'char')
//       C++20 起应该写 ==
//   (d) 没有后置 ++、没有 difference_type 等（4.4 节的表），所以它连最低的迭代器概念都不满足，
//       ranges 算法、views 一个都用不了。书上说“这是基于范围的 for 循环所必需的最小值”，确实只够 range-for
//   (e) 和 4.6 节一样，“既是范围又是迭代器”：同一个对象既当 begin 又被复制，容易混乱
// ============================================================
void section3()
{
    std::println("\n[3] 书上写法的问题");
    cstr_it it("ab");
    std::println("  it != 'x' 是 {}，it != 65 是 {}（随便一个 char 都能当哨兵）", it != 'x', it != 65);
    std::println("  std::ranges::range<cstr_it> 是 {}，input_or_output_iterator<cstr_it> 是 {}", std::ranges::range<cstr_it>,
                 std::input_or_output_iterator<cstr_it>);
}

// ============================================================
// [4] C++20 的写法：范围 CStr + 迭代器 CStr::iterator + 空的哨兵类型 cstr_end
//   cstr_end 是一个空结构体，什么都不存，只用它的“类型”：
//     迭代器里写 bool operator==(cstr_end) const { return p_ == nullptr || *p_ == '\0'; }
//   C++20 会自动补上 !=，还会把 cstr_end{} == it 对调成 it == cstr_end{}（4.3 节）
//   迭代器按 4.4 节的表写全：类型别名、默认构造、后置 ++、迭代器之间的 ==
//   end() 返回 cstr_end{}，类型和 begin() 不一样，所以：
//     ranges 算法、views：都能用（std::sentinel_for<cstr_end, CStr::iterator> 是 true）
//     老算法 std::find(first, last, x)：不能用，要求两个参数同类型（4.6 节，可以用 views::common 包一下）
//   *it 返回 const char&（指向字符串里的那个字符），不是复制品：字符本来就在内存里存着，可以给引用
// ============================================================
struct cstr_end
{
};

class CStr
{
    const char* p_{};

public:
    explicit CStr(const char* p)
        : p_{p}
    {
    }

    class iterator
    {
        const char* p_{};

    public:
        using iterator_concept = std::forward_iterator_tag;
        using iterator_category = std::forward_iterator_tag;
        using value_type = char;
        using difference_type = std::ptrdiff_t;

        iterator() = default;

        explicit iterator(const char* p)
            : p_{p}
        {
        }

        const char& operator*() const
        {
            return *p_;
        }

        iterator& operator++()
        {
            ++p_;
            return *this;
        }

        iterator operator++(int)
        {
            iterator old = *this;
            ++*this;
            return old;
        }

        bool operator==(const iterator&) const = default;

        bool operator==(cstr_end) const
        {
            return p_ == nullptr || *p_ == '\0';
        }
    };

    iterator begin() const
    {
        return iterator{p_};
    }

    cstr_end end() const
    {
        return {};
    }
};

void section4()
{
    std::println("\n[4] C++20 的写法：CStr + cstr_end");
    CStr s{"Hello, world"};
    std::println("  forward_range {}，common_range（begin、end 同类型）{}，sentinel_for {}", std::ranges::forward_range<CStr>,
                 std::ranges::common_range<CStr>, std::sentinel_for<cstr_end, CStr::iterator>);
    std::println("  ranges::distance：{}", std::ranges::distance(s));
    std::println("  ranges::count_if 大写字母：{}",
                 std::ranges::count_if(s, [](char c) { return std::isupper(static_cast<unsigned char>(c)) != 0; }));
    auto comma = std::ranges::find(s, ',');
    std::println("  ranges::find ',' 之后的字符：{:?}", *std::ranges::next(comma));
    auto upper = s | std::views::transform([](char c) { return static_cast<char>(std::toupper(static_cast<unsigned char>(c))); });
    std::println("  views::transform 变大写：{:?}", upper | std::ranges::to<std::string>());
    std::println("  CStr{{nullptr}} 的长度：{}（空指针当成空字符串）", std::ranges::distance(CStr{nullptr}));
    // std::isupper、std::toupper 的参数要先转成 unsigned char：char 可能是负数（比如 UTF-8 的 'é' 是 c3 a9），
    // 直接传负数给它们是未定义行为
}

// ============================================================
// [5] 更省事的写法：const char* 本来就是迭代器（而且是连续的），只缺一个“哨兵”
//   只写一个空的哨兵类型 + 一个 == 运算符（参数是指针和哨兵），然后用 std::ranges::subrange(指针, 哨兵) 组成一个范围
//     struct nul {};
//     bool operator==(const char* p, nul) { return *p == '\0'; }
//   == 写在类外面（普通函数），因为左边是指针，指针不是类，不能给它加成员函数
//   std::ranges::subrange(first, last)：把“一个迭代器 + 一个哨兵（或者另一个迭代器）”包成一个范围（<ranges>）
//   好处：迭代器是现成的 const char*，连续的，所以这个范围是 contiguous_range，比 [4] 的 forward 还强
//   C++26 会有现成的 std::ranges::null_sentinel，两个编译器现在都还没有
// ============================================================
struct nul
{
};

bool operator==(const char* p, nul)
{
    return *p == '\0';
}

void section5()
{
    std::println("\n[5] 指针 + 哨兵 + subrange");
    const char* s = "hello";
    auto r = std::ranges::subrange(s, nul{});
    std::print("  range-for：");
    for (char c : r)
    {
        std::print("{}", c);
    }
    std::println("");
    std::println("  sentinel_for<nul, const char*> {}，contiguous_range {}", std::sentinel_for<nul, const char*>,
                 std::ranges::contiguous_range<decltype(r)>);
    std::println("  ranges::to<std::string>：{:?}", r | std::ranges::to<std::string>());

    // 不写任何类也行：无限长的范围 + take_while（4.6 节的 unreachable_sentinel）
    auto tw = std::ranges::subrange(s, std::unreachable_sentinel) | std::views::take_while([](char c) { return c != '\0'; });
    std::println("  subrange(s, unreachable_sentinel) | take_while(c != '\\0')：{:?}", tw | std::ranges::to<std::string>());
}

// ============================================================
// [6] 同一个思路：任何“用一个特殊值表示结束”的数据都能这样遍历
//   以 -1 结尾的 int 数组：{3, 1, 4, -1, 9}，-1 后面的 9 不算
//   以 nullptr 结尾的指针数组（main 的 argv、环境变量 environ 都是这种格式）
//   +a 的意思：数组前面加一元 +，把数组变成指向第一个元素的指针（int[5] → int*），让人一眼看出“这里传的是指针”。
//     其实不加也能编译：subrange 的构造函数按值接收参数，数组按值传参时自动变成指针（两个编译器我都试过）
//   std::ranges::fold_left(r, 0, std::plus{})：C++23，从 0 开始把每个元素加起来（相当于 accumulate 的 ranges 版本）
// ============================================================
struct minus_one
{
};

bool operator==(const int* p, minus_one)
{
    return *p == -1;
}

struct null_entry
{
};

bool operator==(const char* const* p, null_entry)
{
    return *p == nullptr;
}

void section6()
{
    std::println("\n[6] 其它结束记号");
    int a[]{3, 1, 4, -1, 9};
    auto ints = std::ranges::subrange(+a, minus_one{});
    std::println("  {{3, 1, 4, -1, 9}} 到 -1 为止：{}，和是 {}", ints, std::ranges::fold_left(ints, 0, std::plus{}));

    const char* const names[]{"ann", "bob", "cy", nullptr};
    auto ns = std::ranges::subrange(+names, null_entry{});
    std::println("  以 nullptr 结尾的名字表：{}，一共 {} 个", ns, std::ranges::distance(ns));
}

// ============================================================
// [7] 哨兵范围不知道自己有多长
//   std::ranges::size(r)：要求“不用遍历就能知道长度”（sized_range）。哨兵范围做不到，编译错误：
//     clang：error: no matching function for call to object of type 'const __size::__fn'
//     g++  ：error: no match for call to '(const std::ranges::__access::_Size) (std::ranges::subrange<const char*, nul, ...>&)'
//   std::ranges::distance(r)：能用，但要从头走到 '\0' 一个一个数，是 O(n)，和 strlen 一样
//   这正是哨兵的意义：不用先花 O(n) 算出长度再遍历，而是边走边看，只走一遍
//   反过来，如果要多次用长度，先算一次存起来（或者干脆转成 std::string / std::string_view）
// ============================================================
template <typename R>
concept has_size = requires(R& r) { std::ranges::size(r); };

void section7()
{
    std::println("\n[7] 没有 size()，只有 distance");
    using R = decltype(std::ranges::subrange(std::declval<const char*>(), nul{}));
    std::println("  sized_range：subrange(指针, nul) {}，CStr {}，std::string {}", std::ranges::sized_range<R>,
                 std::ranges::sized_range<CStr>, std::ranges::sized_range<std::string>);
    std::println("  能不能调用 ranges::size：subrange(指针, nul) {}，std::string {}", has_size<R>, has_size<std::string>);
    const char* empty = "";
    std::println("  空字符串的 ranges::distance：{}", std::ranges::distance(std::ranges::subrange(empty, nul{})));
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
