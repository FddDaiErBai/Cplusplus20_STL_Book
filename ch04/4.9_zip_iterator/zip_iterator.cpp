// 第 4 章 4.9 zip 迭代器适配器：把两个序列“拉链”在一起，一次拿到一对
// 编译：clang++ -std=c++23 -Wall -Wextra -Wpedantic zip_iterator.cpp -o zip_iterator.exe
//       g++     -std=c++23 -Wall -Wextra -Wpedantic zip_iterator.cpp -o zip_iterator.exe -lstdc++exp
//
// 文件结构：[1] 到 [7] 从上到下排列，每个编号只出现一次。
//   每一节里依次是：说明、这一节用到的类和函数、一个 sectionN() 函数（运行这一节）。main 依次调用 section1() 到 section7()。
//
// ============================================================
// zip（拉链）：两个序列按位置配对：
//     a：  1     2     3
//     b：  6     7     8
//     zip：(1,6) (2,7) (3,8)
//   两个序列一样长时很简单。不一样长时，以短的为准：走到任何一个的结尾就停（Python 的 zip 也是这样）
//
// 这一节做的事：
//   [1] 书上的 zip_iterator（又是“既是范围又是迭代器”的写法）
//   [2] 书上写法的问题：只能 range-for、只能同一种容器、不能 const、改不了元素、存了 6 个迭代器……
//   [3] 两个不一样长：书上的 == 为什么用 ||
//   [4] C++20 风格的自己写：Zip 范围 + 迭代器 + 哨兵，支持任意两种容器，*it 返回引用对
//   [5] 标准库现成的：C++23 std::views::zip（实际写代码用它）
//   [6] views::zip 的几个用法：一起排序、转成 map、zip_transform、enumerate、adjacent
//   [7] 书上 There's more：zip 进 map
// ============================================================
#include <algorithm>
#include <cstddef>
#include <functional>
#include <iterator>
#include <list>
#include <map>
#include <print>
#include <ranges>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

// ============================================================
// [1] 书上的 zip_iterator<T>：T 是容器类型，两个容器必须是同一种类型
//   成员：ita_、itb_ 是“当前位置”，另外还存了 4 个：两个容器的 begin、两个容器的 end
//     公开的构造函数 zip_iterator(a, b)：从两个容器构造，6 个迭代器全填上（这个对象当“范围”用）
//     私有的构造函数 zip_iterator(ita, itb)：只填 ita_、itb_（begin()、end() 用它造“迭代器”）
//   *it 返回 std::pair<val_t, val_t>：两个元素的复制品
//   range-for 里用 auto [a, b] 拆开 pair（结构化绑定，C++17）
//   书上的输出：zipped: [Bob, Dylan] [John, Williams] [Joni, Mitchell]
//   下面的代码是书上的原样（只调整了格式），问题在 [2] 讲
// ============================================================
template <typename T>
class zip_iterator
{
    using val_t = typename T::value_type;
    using ret_t = std::pair<val_t, val_t>;
    using it_t = typename T::iterator;

    it_t ita_{};
    it_t itb_{};
    // for begin() and end() objects
    it_t ita_begin_{};
    it_t itb_begin_{};
    it_t ita_end_{};
    it_t itb_end_{};

    // private constructor for begin() and end() objects
    zip_iterator(it_t ita, it_t itb)
        : ita_{ita}, itb_{itb}
    {
    }

public:
    using iterator_concept = std::forward_iterator_tag;
    using iterator_category = std::forward_iterator_tag;
    using value_type = std::pair<val_t, val_t>;
    using difference_type = long int;
    using pointer = const val_t*;
    using reference = const val_t&;

    zip_iterator(T& a, T& b)
        : ita_{a.begin()}, itb_{b.begin()}, ita_begin_{ita_}, itb_begin_{itb_}, ita_end_{a.end()}, itb_end_{b.end()}
    {
    }

    zip_iterator& operator++()
    {
        ++ita_;
        ++itb_;
        return *this;
    }

    bool operator==(const zip_iterator& o) const
    {
        return ita_ == o.ita_ || itb_ == o.itb_;
    }

    bool operator!=(const zip_iterator& o) const
    {
        return !operator==(o);
    }

    ret_t operator*() const
    {
        return {*ita_, *itb_};
    }

    zip_iterator begin() const
    {
        return zip_iterator(ita_begin_, itb_begin_);
    }

    zip_iterator end() const
    {
        return zip_iterator(ita_end_, itb_end_);
    }
};

void section1()
{
    std::println("[1] 书上的 zip_iterator");
    std::vector<std::string> vec_a{"Bob", "John", "Joni"};
    std::vector<std::string> vec_b{"Dylan", "Williams", "Mitchell"};
    std::print("  zipped: ");
    for (auto [a, b] : zip_iterator(vec_a, vec_b))
    {
        std::print("[{}, {}] ", a, b);
    }
    std::println("");
}

// ============================================================
// [2] 书上写法的问题
//   (a) 没有后置 ++、没有默认构造（私有构造函数不算），所以连最低的迭代器概念都不满足：
//       ranges 算法、views 一个都用不了，只能 range-for。和 4.4 节书上的 Seq 是同一个毛病
//   (b) 两个容器必须是同一种类型：zip_iterator(vector<int>, vector<double>) 编译不过：
//         clang：error: no viable constructor or deduction guide for deduction of template arguments of 'zip_iterator'
//         g++  ：error: no matching function for call to 'zip_iterator(std::vector<int>&, std::vector<double>&)'
//       实际最常用的恰恰是不同类型配对：编号 + 名字、名字 + 分数
//   (c) 不能用 const 容器：构造函数参数是 T&，而且用的是 T::iterator，const vector 给不出来。报错同上
//   (d) *it 返回 pair 的复制品：range-for 里改 a、b，改的是复制品，原来的容器没变（下面演示）
//   (e) reference = const val_t&、pointer = const val_t*：*it 实际返回的是 pair<val_t, val_t>，
//       不是 const val_t&，和 4.4、4.6 节一样是错的
//   (f) 只写 val_t、it_t 这种 T::value_type、T::iterator，原生数组不能用（4.1 节）
//   (g) 一个对象存了 6 个迭代器：既当范围又当迭代器，每次复制迭代器都要复制 6 个（下面打印了大小）。
//       分成“范围（存两个容器）”和“迭代器（存两个位置）”两个类就清楚了（[4]）
//   (h) 只写了一个 == 再写 != 去调用它：C++20 不用写 !=
//   (i) difference_type = long int：Windows 上 long 是 32 位，应该用 std::ptrdiff_t（4.6 节溢出的教训）
// ============================================================
void section2()
{
    std::println("\n[2] 书上写法的问题");
    using Z = zip_iterator<std::vector<std::string>>;
    std::println("  std::ranges::range {}，input_or_output_iterator {}，default_initializable {}", std::ranges::range<Z>,
                 std::input_or_output_iterator<Z>, std::default_initializable<Z>);
    std::println("  sizeof(zip_iterator) = {}，一个 vector 迭代器是 {}", sizeof(Z), sizeof(std::vector<std::string>::iterator));

    std::vector<std::string> a{"Bob", "John"};
    std::vector<std::string> b{"Dylan", "Williams"};
    for (auto [x, y] : zip_iterator(a, b))
    {
        x += "!"; // 改的是 pair 里的复制品
        (void)y;
    }
    std::println("  range-for 里给 x 加 \"!\" 之后，a 还是：{}", a);
}

// ============================================================
// [3] 两个不一样长：书上的 == 为什么用 ||
//   range-for 每次比较 it != end，也就是 !(it == end)。书上的 ==：
//     ita_ == end 的 ita_ || itb_ == end 的 itb_
//   任何一个走到了自己的结尾，就算“相等”，循环停下。所以以短的为准，不会越界
//   如果写成 &&：要两个同时到结尾才停。短的那个先到结尾后还会继续 ++、继续 *，越界，未定义行为
//   代价：这个 == 不是真正的“相等”。两个迭代器只要有一边位置一样就“相等”，
//     比如 (位置 0, 位置 5) == (位置 0, 位置 9) 也是 true。这不满足“相等”应有的性质（练习 3 会遇到）。
//     [4] 的做法：迭代器之间的 == 老老实实比较；“到头了没有”交给哨兵去判断，在哨兵的 == 里用 ||
// ============================================================
void section3()
{
    std::println("\n[3] 不一样长");
    std::vector<std::string> a{"Bob", "John", "Joni"};
    std::vector<std::string> c{"x"};
    std::print("  a 有 3 个，c 有 1 个：");
    for (auto [x, y] : zip_iterator(a, c))
    {
        std::print("[{}, {}] ", x, y);
    }
    std::println("（以短的为准）");
}

// ============================================================
// [4] C++20 风格：Zip 范围 + ZipIt 迭代器 + ZipEnd 哨兵
//   Zip<A, B>：范围，只存两个容器的指针（A*、B*）。A、B 可以是不同类型，可以是 const 的，可以是原生数组
//   ZipIt<I1, I2>：迭代器，只存两个位置 i1_、i2_
//     *it  返回 std::pair<std::iter_reference_t<I1>, std::iter_reference_t<I2>>：
//          两个“引用”组成的 pair（比如 pair<int&, std::string&>），所以通过它能改原来的元素
//     ==   迭代器之间：比较 i1_（两个位置是一起走的，比一个就够了）
//   ZipEnd<I1, I2>：哨兵（4.6、4.8 节），存两个容器的 end
//     it == 哨兵：i1_ == e1 || i2_ == e2   —— 以短的为准，“||”放在这里就不会破坏迭代器之间的 ==
//   类别：iterator_concept 写 forward（C++20 按操作判断，ranges、views 都能用）；
//     iterator_category 写 input：*it 返回的是临时的 pair，不是一个真正的引用，老规矩里只能算 input（4.4 节 [6] 的冷知识）。
//     标准库的 views::zip 也是这么做的
//   用 std::ranges::begin、std::ranges::end、std::ranges::iterator_t：原生数组也能用（4.1 节）
// ============================================================
template <typename I1, typename I2>
struct ZipEnd
{
    I1 e1{};
    I2 e2{};
};

template <typename I1, typename I2>
class ZipIt
{
    I1 i1_{};
    I2 i2_{};

public:
    using iterator_concept = std::forward_iterator_tag;
    using iterator_category = std::input_iterator_tag;
    using value_type = std::pair<std::iter_value_t<I1>, std::iter_value_t<I2>>;
    using difference_type = std::ptrdiff_t;

    ZipIt() = default;

    ZipIt(I1 i1, I2 i2)
        : i1_{i1}, i2_{i2}
    {
    }

    std::pair<std::iter_reference_t<I1>, std::iter_reference_t<I2>> operator*() const
    {
        return {*i1_, *i2_};
    }

    ZipIt& operator++()
    {
        ++i1_;
        ++i2_;
        return *this;
    }

    ZipIt operator++(int)
    {
        ZipIt old = *this;
        ++*this;
        return old;
    }

    bool operator==(const ZipIt& o) const
    {
        return i1_ == o.i1_;
    }

    bool operator==(const ZipEnd<I1, I2>& e) const
    {
        return i1_ == e.e1 || i2_ == e.e2;
    }
};

template <std::ranges::forward_range A, std::ranges::forward_range B>
class Zip
{
    A* a_;
    B* b_;

    using I1 = std::ranges::iterator_t<A>;
    using I2 = std::ranges::iterator_t<B>;

public:
    Zip(A& a, B& b)
        : a_{&a}, b_{&b}
    {
    }

    ZipIt<I1, I2> begin() const
    {
        return {std::ranges::begin(*a_), std::ranges::begin(*b_)};
    }

    ZipEnd<I1, I2> end() const
    {
        return {std::ranges::end(*a_), std::ranges::end(*b_)};
    }
};

void section4()
{
    std::println("\n[4] C++20 风格的 Zip");
    std::vector<int> ids{1, 2, 3, 4};
    std::vector<std::string> names{"Bob", "John", "Joni"};
    Zip z{ids, names}; // 类模板参数推导：A = vector<int>，B = vector<string>
    std::println("  forward_range {}，ranges::distance（以短的为准）{}", std::ranges::forward_range<decltype(z)>,
                 std::ranges::distance(z));

    std::print("  不同类型：");
    for (auto [id, name] : z)
    {
        std::print("{}:{} ", id, name);
    }
    std::println("");

    for (auto [id, name] : z)
    {
        name += "!"; // name 是 std::string&，改的是 names 里的元素
        (void)id;
    }
    std::println("  通过引用修改之后 names：{}", names);

    const std::list<double> scores{9.5, 8.0};
    int arr[]{7, 8, 9};
    Zip z2{arr, scores}; // 原生数组 + const list
    std::print("  原生数组 + const list：");
    for (auto [n, s] : z2)
    {
        std::print("{}:{} ", n, s);
    }
    std::println("");
    std::println("  views::transform 取第一个：{}", z | std::views::transform([](auto p) { return p.first; }));
}

// ============================================================
// [5] 标准库现成的：C++23 std::views::zip（<ranges>）—— 实际写代码就用它
//   std::views::zip(a, b, c, ...)：任意多个范围，以最短的为准
//   *it 是 std::tuple<A&, B&, ...>：引用组成的 tuple，所以 auto [x, y] 拆出来的 x、y 也是引用，能改原来的元素
//     （注意这和 [2] 书上的不同：书上返回复制品的 pair，改了也没用）
//   类别：所有范围里最弱的那个。两个 vector → random_access，能 size()；有一个 list → 只到 bidirectional
// ============================================================
void section5()
{
    std::println("\n[5] std::views::zip");
    std::vector<int> ids{1, 2, 3, 4};
    std::vector<std::string> names{"Bob", "John", "Joni"};
    std::list<double> scores{9.5, 8.0, 7.5};

    std::println("  zip(ids, names, scores)：{}", std::views::zip(ids, names, scores));

    for (auto [id, name] : std::views::zip(ids, names))
    {
        name += "?";
        (void)id;
    }
    std::println("  通过 auto [id, name] 修改之后 names：{}", names);

    auto z = std::views::zip(ids, names);
    static_assert(std::same_as<std::ranges::range_reference_t<decltype(z)>, std::tuple<int&, std::string&>>);
    std::println("  两个 vector：random_access {}，size() = {}", std::ranges::random_access_range<decltype(z)>, z.size());
    auto zl = std::views::zip(ids, scores);
    std::println("  vector + list：bidirectional {}，random_access {}", std::ranges::bidirectional_range<decltype(zl)>,
                 std::ranges::random_access_range<decltype(zl)>);
}

// ============================================================
// [6] views::zip 的几个常用法（C++23）
//   一起排序：std::ranges::sort(std::views::zip(k, v))：按 k 排序，v 跟着一起换位置
//     （tuple 按第一个元素比较，第一个一样再比第二个）
//   转成 map：std::views::zip(keys, values) | std::ranges::to<std::map>()
//   zip_transform(f, a, b)：对每一对调用 f(a[i], b[i])，比如两个数组对应相加
//   enumerate(r)：和 0 1 2 …… 配对，相当于 zip(iota(0), r)，得到 (下标, 元素)
//   adjacent<2>(r)：和“自己的下一个”配对：(r[0], r[1]) (r[1], r[2]) ……
// ============================================================
void section6()
{
    std::println("\n[6] 常用法");
    std::vector<int> k{3, 1, 2};
    std::vector<char> v{'c', 'a', 'b'};
    std::ranges::sort(std::views::zip(k, v));
    std::println("  按 k 一起排序：k = {}，v = {}", k, v);

    std::vector<std::string> names{"Dylan", "Williams", "Mitchell"};
    std::vector<int> ids{1, 2, 3};
    auto m = std::views::zip(names, ids) | std::ranges::to<std::map>();
    std::println("  zip | to<map>：{}", m);

    std::println("  zip_transform(plus, ids, {{10, 20, 30}})：{}", std::views::zip_transform(std::plus{}, ids, std::vector{10, 20, 30}));
    std::println("  enumerate(names)：{}", std::views::enumerate(names));
    std::println("  adjacent<2>(ids)：{}", ids | std::views::adjacent<2>);
}

// ============================================================
// [7] 书上 There's more：zip 进 map
//   try_emplace(key, value)：key 不存在才插入（3.7 节讲过）
//   C++23 直接写 std::views::zip(vec_a, vec_b) | std::ranges::to<std::map>()（[6]）
// ============================================================
void section7()
{
    std::println("\n[7] zip 进 map");
    std::vector<std::string> vec_a{"Bob", "John", "Joni"};
    std::vector<std::string> vec_b{"Dylan", "Williams", "Mitchell"};
    std::map<std::string, std::string> name_map{};
    for (auto [a, b] : zip_iterator(vec_a, vec_b))
    {
        name_map.try_emplace(a, b);
    }
    std::println("  name_map: {}", name_map);
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
