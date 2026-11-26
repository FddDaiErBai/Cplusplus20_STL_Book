// 第 4 章 4.10 随机访问迭代器（以及连续迭代器）：最强的一种迭代器，要写哪些运算符
// 编译：clang++ -std=c++23 -Wall -Wextra -Wpedantic random_access.cpp -o random_access.exe
//       g++     -std=c++23 -Wall -Wextra -Wpedantic random_access.cpp -o random_access.exe -lstdc++exp
//
// 文件结构：[1] 到 [7] 从上到下排列，每个编号只出现一次。
//   每一节里依次是：说明、这一节用到的类和函数、一个 sectionN() 函数（运行这一节）。main 依次调用 section1() 到 section7()。
//
// ============================================================
// 随机访问迭代器：在双向迭代器（能 ++、--）的基础上，还能“跳”：
//
//   表达式           结果类型              意思
//   --------------   -------------------   ----------------------------------------
//   it += n          It&                   往后跳 n 个（n 可以是负数）
//   it -= n          It&                   往前跳 n 个
//   it + n、n + it   It                    跳 n 个之后的新迭代器（两种顺序都要支持）
//   it - n           It                    往前跳 n 个的新迭代器
//   it2 - it1        difference_type       两个迭代器之间差几个（有正负！）
//   it[n]            引用                  *(it + n)
//   < > <= >=        bool                  比较位置的前后
//
//   C++20 的 std::random_access_iterator 概念检查上面每一个，而且检查的是 const 的迭代器（下面 [2] 会栽在这里）
//   再强一级：std::contiguous_iterator（连续迭代器）：元素在内存里紧挨着，能拿到真正的指针（[4]）
//
// 书上这一节的完整代码有 700 多行，书里只印了一部分，放在 GitHub 上。这里只用书里印出来的部分。
//   [1] 书上的 Container 和它的迭代器（照书上印的写法）
//   [2] 书上的迭代器其实不是随机访问迭代器：哪几行出了问题
//   [3] 正确的写法：Container<T>，迭代器的每个运算符
//   [4] 连续迭代器：iterator_concept = contiguous_iterator_tag，std::span、ranges::data 都能用
//   [5] const_iterator：一个模板写出 iterator 和 const_iterator 两个
//   [6] 能用上的算法：sort、lower_bound、nth_element ……
//   [7] 第 4 章小结：五种迭代器，各要写什么
// ============================================================
#include <algorithm>
#include <compare>
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <print>
#include <ranges>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

// ============================================================
// [1] 书上的 Container（书里印出来的部分，格式调整过）
//   std::unique_ptr<T[]> c_：智能指针管理一块数组内存，Container 析构时自动 delete[]，不用自己写析构函数
//   std::make_unique<T[]>(n)：分配 n 个元素，每个都“值初始化”（int 是 0，string 是空字符串）
//     书上说 Container(size_t) 是“未初始化的元素”，不准确：都是初始化过的空值，所以打印出来是 [] [] ……
//   迭代器里只存一个 T* ptr_，所有运算符都是对指针做同样的事
//   书上运算符的写法问题很多，下面用注释标出来，[2] 细讲。书上没印出来的运算符（*、->、++、--、[]、+=、-=）按书上的风格补上
// ============================================================
template <typename T>
class BookContainer
{
    std::unique_ptr<T[]> c_{};
    std::size_t n_elements_{};

public:
    class iterator
    {
        T* ptr_;

    public:
        using iterator_concept = std::contiguous_iterator_tag;
        using iterator_category = std::random_access_iterator_tag;
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = T*;
        using reference = T&;

        iterator(T* ptr = nullptr)
            : ptr_{ptr}
        {
        }

        T& operator*() const
        {
            return *ptr_;
        }

        T* operator->() const
        {
            return ptr_;
        }

        iterator& operator++()
        {
            ++ptr_;
            return *this;
        }

        iterator operator++(int)
        {
            iterator old = *this;
            ++ptr_;
            return old;
        }

        iterator& operator--()
        {
            --ptr_;
            return *this;
        }

        iterator operator--(int)
        {
            iterator old = *this;
            --ptr_;
            return old;
        }

        T& operator[](std::size_t n) const
        {
            return ptr_[n];
        }

        iterator& operator+=(std::size_t n)
        {
            ptr_ += n;
            return *this;
        }

        iterator& operator-=(std::size_t n)
        {
            ptr_ -= n;
            return *this;
        }

        auto operator<=>(const iterator& o) const // 书上写的是 const auto operator<=>，返回值上的 const 没用
        {
            return ptr_ <=> o.ptr_;
        }

        bool operator==(const iterator& o) const
        {
            return ptr_ == o.ptr_;
        }

        iterator operator+(const std::size_t n) const
        {
            return iterator(ptr_ + n);
        }

        friend iterator operator+(const std::size_t n, const iterator& o) // 书上返回 const iterator，问题 (c)
        {
            return iterator(o.ptr_ + n);
        }

        iterator operator-(const std::size_t n) // 书上返回 const iterator，并且函数不是 const 的，问题 (b)
        {
            return iterator(ptr_ - n);
        }

        std::size_t operator-(const iterator& o) // 书上返回 const size_t，函数不是 const 的，问题 (a)(b)
        {
            return static_cast<std::size_t>(ptr_ - o.ptr_);
        }
    };

    BookContainer(std::initializer_list<T> l)
        : n_elements_{l.size()}
    {
        c_ = std::make_unique<T[]>(n_elements_);
        std::size_t index{0};
        for (T e : l)
        {
            c_[index++] = e;
        }
    }

    BookContainer(std::size_t sz)
        : n_elements_{sz}
    {
        c_ = std::make_unique<T[]>(n_elements_);
    }

    std::size_t size() const
    {
        return n_elements_;
    }

    const T& operator[](const std::size_t index) const
    {
        return c_[index];
    }

    T& at(const std::size_t index) const // 问题 (d)(e)
    {
        if (index > n_elements_ - 1)
        {
            throw std::out_of_range("Container::at(): index out of range");
        }
        return c_[index];
    }

    iterator begin() const
    {
        return iterator(c_.get());
    }

    iterator end() const
    {
        return iterator(c_.get() + n_elements_);
    }
};

void section1()
{
    std::println("[1] 书上的 Container");
    BookContainer<std::string> x{"one", "two", "three", "four", "five", "six", "seven", "eight", "nine", "ten"};
    std::println("  Container x size: {}", x.size());
    std::print("  ");
    for (auto e : x)
    {
        std::print("{} ", e);
    }
    std::println("");
    std::println("  at(5): {}，[5]: {}，begin + 5: {}，5 + begin: {}，begin += 5: {}", x.at(5), x[5], *(x.begin() + 5),
                 *(5 + x.begin()), *(x.begin() += 5));
    std::print("  views::reverse: ");
    for (auto v : x | std::views::reverse)
    {
        std::print("{} ", v);
    }
    std::println("");

    BookContainer<std::string> y(x.size());
    std::print("  Container y size: {}，元素：", y.size());
    for (auto e : y)
    {
        std::print("[{}] ", e);
    }
    std::println("");
}

// ============================================================
// [2] 书上的迭代器不是 std::random_access_iterator（两个编译器都是 false）
//   书上说“全功能连续/随机访问迭代器”，但按 C++20 的概念检查，它只是双向迭代器。原因：
//   (a) it2 - it1 返回 size_t（无符号数）：
//       begin - end 应该是 -5，size_t 装不下负数，“绕回来”变成 18446744073709551611（4.6 节说过的无符号溢出）。
//       概念要求返回 difference_type（有符号的 std::ptrdiff_t）
//   (b) 两个 operator- 都不是 const 成员函数：
//       概念检查的写法是 requires(const I i, const I j) { j - i; }，在 const 迭代器上调用，非 const 的函数调用不了。
//       （书上其它运算符大多写了 const，就这两个漏了）
//   (c) n + it 返回 const iterator：概念要求 n + it 的类型正好是 I，const I 不算
//   三处都改对以后（我一个一个试过），random_access_iterator 才变成 true。改其中一两处都不够
//   为什么书上能运行？range-for、views::reverse 只要双向；书上的例子没有用到 ranges::sort 这类要随机访问的
//   老算法 std::sort 只看 iterator_category 标签（4.4 节），不检查概念，所以也能用。换成 std::ranges::sort 就编译不过：
//     clang：note: because 'BookContainer<int>::iterator' does not satisfy 'random_access_iterator'
//            note: because '__s - __i' would be invalid: invalid operands to binary expression
//                  ('const BookContainer<int>::iterator' and 'const BookContainer<int>::iterator')   ← 正是 (b)：const 的迭代器调用不了
//     g++  ：error: no match for call to '(const std::ranges::__sort_fn) (BookContainer<int>::iterator, BookContainer<int>::iterator)'
//
//   Container 本身的两个问题：
//   (d) at() 的边界检查 index > n_elements_ - 1：n_elements_ 是 0 时，0 - 1 在 size_t 里是 18446744073709551615，
//       任何 index 都不大于它，于是空容器的 at(0) 不抛异常，直接越界读。应该写 index >= n_elements_
//   (e) at() 是 const 成员函数，却返回 T&：const 的 Container 也能通过 at() 改元素。应该写两个版本（[3]）
// ============================================================
void section2()
{
    std::println("\n[2] 书上的迭代器是什么类别");
    using It = BookContainer<int>::iterator;
    std::println("  bidirectional_iterator {}，random_access_iterator {}，contiguous_iterator {}", std::bidirectional_iterator<It>,
                 std::random_access_iterator<It>, std::contiguous_iterator<It>);
    BookContainer<int> c{1, 2, 3, 4, 5};
    std::println("  c.begin() - c.end() = {}（应该是 -5）", c.begin() - c.end());
    std::size_t zero = 0;
    std::println("  n_elements_ 是 0 时，n_elements_ - 1 = {}", zero - 1);
}

// ============================================================
// [3] 正确的写法：Container<T> 和它的迭代器
//   迭代器写成模板 basic_iterator<U>：U 是 T 时就是 iterator，U 是 const T 时就是 const_iterator（[5]）
//   要点（对照开头的表）：
//     difference_type 用 std::ptrdiff_t（有符号），n 的类型都用 difference_type，不用 size_t
//     + - 写成 friend 函数（书上也是这样写 n + it 的）：
//       friend 写在类里面，但它不是成员函数，是普通函数，两个参数都写出来，所以 it + n、n + it 都好写
//       参数按值传进来（basic_iterator it），改它不影响调用者，直接 return it += n; 就行：+ 借用 +=，不重复写
//     比较：bool operator==(...) const = default; 和 auto operator<=>(...) const = default;
//       = default 会逐个比较成员，这里只有一个指针 p_，正好就是“比较位置”。< > <= >= 都由 <=> 自动得到（1.5 节）
//     所有不改自己的运算符都加 const
//   Container 的 at() 写成 index >= n_，并且有 const 和非 const 两个版本
// ============================================================
template <typename T>
class Container
{
    std::unique_ptr<T[]> c_{};
    std::size_t n_{};

public:
    template <typename U>
    class basic_iterator
    {
        U* p_{};

    public:
        using iterator_concept = std::contiguous_iterator_tag;   // [4]
        using iterator_category = std::random_access_iterator_tag; // 老的一套里最高就是 random_access（4.4 节）
        using value_type = std::remove_cv_t<U>;                   // const_iterator 的 value_type 也是 T，不是 const T
        using difference_type = std::ptrdiff_t;
        using pointer = U*;
        using reference = U&;

        basic_iterator() = default;

        explicit basic_iterator(U* p)
            : p_{p}
        {
        }

        // iterator 能自动变成 const_iterator（反过来不行），[5]
        operator basic_iterator<const U>() const
            requires(!std::is_const_v<U>)
        {
            return basic_iterator<const U>{p_};
        }

        U& operator*() const
        {
            return *p_;
        }

        U* operator->() const // [4]：连续迭代器靠它（或者 std::to_address）拿到真正的指针
        {
            return p_;
        }

        U& operator[](difference_type n) const
        {
            return p_[n];
        }

        basic_iterator& operator++()
        {
            ++p_;
            return *this;
        }

        basic_iterator operator++(int)
        {
            basic_iterator old = *this;
            ++p_;
            return old;
        }

        basic_iterator& operator--()
        {
            --p_;
            return *this;
        }

        basic_iterator operator--(int)
        {
            basic_iterator old = *this;
            --p_;
            return old;
        }

        basic_iterator& operator+=(difference_type n)
        {
            p_ += n;
            return *this;
        }

        basic_iterator& operator-=(difference_type n)
        {
            p_ -= n;
            return *this;
        }

        friend basic_iterator operator+(basic_iterator it, difference_type n)
        {
            return it += n;
        }

        friend basic_iterator operator+(difference_type n, basic_iterator it)
        {
            return it += n;
        }

        friend basic_iterator operator-(basic_iterator it, difference_type n)
        {
            return it -= n;
        }

        friend difference_type operator-(const basic_iterator& a, const basic_iterator& b)
        {
            return a.p_ - b.p_;
        }

        bool operator==(const basic_iterator&) const = default;
        auto operator<=>(const basic_iterator&) const = default;
    };

    using iterator = basic_iterator<T>;
    using const_iterator = basic_iterator<const T>;

    Container(std::initializer_list<T> l)
        : c_{std::make_unique<T[]>(l.size())}, n_{l.size()}
    {
        std::ranges::copy(l, c_.get());
    }

    explicit Container(std::size_t n) // explicit：不让 Container<int> c = 5; 这种写法偷偷造出 5 个元素
        : c_{std::make_unique<T[]>(n)}, n_{n}
    {
    }

    std::size_t size() const
    {
        return n_;
    }

    T& operator[](std::size_t i)
    {
        return c_[i];
    }

    const T& operator[](std::size_t i) const
    {
        return c_[i];
    }

    T& at(std::size_t i)
    {
        if (i >= n_)
        {
            throw std::out_of_range("Container::at(): index out of range");
        }
        return c_[i];
    }

    const T& at(std::size_t i) const
    {
        if (i >= n_)
        {
            throw std::out_of_range("Container::at(): index out of range");
        }
        return c_[i];
    }

    iterator begin()
    {
        return iterator{c_.get()};
    }

    iterator end()
    {
        return iterator{c_.get() + n_};
    }

    const_iterator begin() const
    {
        return const_iterator{c_.get()};
    }

    const_iterator end() const
    {
        return const_iterator{c_.get() + n_};
    }
};

void section3()
{
    std::println("\n[3] 正确的写法");
    using It = Container<int>::iterator;
    std::println("  random_access_iterator {}", std::random_access_iterator<It>);
    Container<int> c{10, 20, 30, 40, 50};
    auto it = c.begin();
    std::println("  *(it + 2) = {}，*(2 + it) = {}，it[3] = {}，*(c.end() - 1) = {}", *(it + 2), *(2 + it), it[3], *(c.end() - 1));
    std::println("  c.end() - c.begin() = {}，c.begin() - c.end() = {}", c.end() - c.begin(), c.begin() - c.end());
    std::println("  c.begin() < c.end() 是 {}", c.begin() < c.end());

    Container<int> empty(std::size_t{0});
    try
    {
        (void)empty.at(0);
        std::println("  空容器 at(0)：没有抛异常（错）");
    }
    catch (const std::out_of_range& e)
    {
        std::println("  空容器 at(0)：抛出 out_of_range：{}", e.what());
    }
}

// ============================================================
// [4] 连续迭代器（C++20 的 std::contiguous_iterator）
//   比随机访问再多一条：元素在内存里紧挨着，能从迭代器拿到真正的指针（std::to_address(it)，靠 operator-> 实现）。
//   要声明出来：iterator_concept = std::contiguous_iterator_tag。只看操作是分不出“随机访问”和“连续”的：
//     4.1 节说过的 vector<bool>、deque 都是随机访问但不是连续，操作写得再全也不能自动算连续
//   有了它，容器就是 contiguous_range，可以：
//     std::ranges::data(c)：拿到第一个元素的指针
//     std::span<int>(c)：不复制，直接“看”这一块内存（C++20，<span>）
//     传给只接受指针的 C 函数（memcpy 之类）
// ============================================================
void section4()
{
    std::println("\n[4] 连续迭代器");
    Container<int> c{5, 1, 4, 2, 3};
    std::println("  contiguous_iterator {}，contiguous_range {}，sized_range {}", std::contiguous_iterator<Container<int>::iterator>,
                 std::ranges::contiguous_range<Container<int>>, std::ranges::sized_range<Container<int>>);
    std::println("  std::to_address(c.begin()) == &c[0] 是 {}", std::to_address(c.begin()) == &c[0]);
    std::span<int> s{c};
    s[0] = 50; // 通过 span 改的就是 c 里的元素
    std::println("  std::span<int>(c)：{}，改 s[0] 之后 c[0] = {}", s, c[0]);
}

// ============================================================
// [5] const_iterator
//   const 的容器（比如函数参数 const Container<int>&）只能调用 const 版本的 begin()，返回 const_iterator：*it 是 const T&，不能改
//   iterator 和 const_iterator 除了“*it 是 T& 还是 const T&”以外完全一样，所以写成一个模板 basic_iterator<U>，
//   U 分别是 T 和 const T，不用把 100 多行写两遍
//   iterator 应该能变成 const_iterator（只是多一道限制，和 4.8 节 char* 能变 const char* 一个道理），反过来不行。
//   [3] 里的 operator basic_iterator<const U>() 就是这个转换；requires(!std::is_const_v<U>) 让它只在 iterator 里出现
// ============================================================
void section5()
{
    std::println("\n[5] const_iterator");
    Container<int> c{1, 2, 3};
    const Container<int>& cr = c;
    static_assert(std::same_as<decltype(*cr.begin()), const int&>);
    static_assert(std::same_as<decltype(*c.begin()), int&>);
    Container<int>::const_iterator ci = c.begin(); // iterator → const_iterator：可以
    std::println("  const_iterator：*ci = {}，是 random_access {}，能往里写（output_iterator）{}", *ci,
                 std::random_access_iterator<Container<int>::const_iterator>, std::output_iterator<Container<int>::const_iterator, int>);
    std::println("  const_iterator 能变成 iterator 吗：{}", std::convertible_to<Container<int>::const_iterator, Container<int>::iterator>);
}

// ============================================================
// [6] 随机访问迭代器能用上的算法（这些都要求随机访问，list 用不了）
//   std::ranges::sort、std::ranges::nth_element（找第 n 小，O(n)）、std::ranges::lower_bound（二分查找，O(log n)）
//   为什么要随机访问：二分查找每次要跳到中间 first + (last - first) / 2。随机访问一步到，O(1)；
//     双向迭代器只能一步一步走，跳到中间本身就是 O(n)，二分就没意义了（练习 2 自己写一个）
// ============================================================
void section6()
{
    std::println("\n[6] 能用上的算法");
    Container<int> c{50, 10, 40, 20, 30};
    std::ranges::nth_element(c, c.begin() + 2);
    std::println("  nth_element 第 2 小（下标从 0 开始）：{}", c[2]);
    std::ranges::sort(c);
    std::println("  ranges::sort：{}", c);
    auto lb = std::ranges::lower_bound(c, 25);
    std::println("  ranges::lower_bound(25)：第一个 >= 25 的是 {}，下标 {}", *lb, lb - c.begin());
    std::println("  views::reverse | take(2)：{}", c | std::views::reverse | std::views::take(2));
}

// ============================================================
// [7] 第 4 章小结：五种迭代器（从弱到强，每一种都包含前一种的全部要求）
//
//   类别             比前一种多要求的                       例子
//   ---------------  -------------------------------------  ---------------------------------------
//   input            *it 读、++it、it++、能和 end 比较      istream_iterator（4.5）、std::generator（4.6）
//   forward          默认构造、== 正常、能多次遍历           forward_list、4.4 的 Seq、4.8 的 CStr
//   bidirectional    --it、it--                             list、map、set、4.7 能反过来的那些
//   random_access    += -= + - 、it - it、[]、< >           deque、4.9 的 views::zip（两个 vector）、本节
//   contiguous       元素紧挨着，to_address 拿到指针         vector、array、string、原生数组、本节
//
//   自己写迭代器时（4.4 节的清单）：两个标签、value_type、difference_type、默认构造、后置 ++、==，
//     再按想要的类别补上对应的运算符。写完用 static_assert(std::random_access_iterator<It>) 之类检查一下，
//     不要像书上一样只靠 range-for 能跑就认为写对了
//   实际工作中：存数据就用 vector、array，它们的迭代器早就写好了；自己写迭代器主要是做“生成器”（4.6）、
//     “视图”（4.9 的 zip）这类不存数据、或者包装别人数据的东西
// ============================================================
void section7()
{
    std::println("\n[7] 小结：各种迭代器的类别");
    auto row = [](std::string_view name, auto tag) {
        using It = decltype(tag);
        std::println("  {:<28} input {:<5} fwd {:<5} bidi {:<5} ra {:<5} contig {}", name, std::input_iterator<It>,
                     std::forward_iterator<It>, std::bidirectional_iterator<It>, std::random_access_iterator<It>,
                     std::contiguous_iterator<It>);
    };
    row("BookContainer::iterator", BookContainer<int>::iterator{});
    row("Container::iterator", Container<int>::iterator{});
    row("std::vector<int>::iterator", std::vector<int>::iterator{});
    row("int*", static_cast<int*>(nullptr));
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
