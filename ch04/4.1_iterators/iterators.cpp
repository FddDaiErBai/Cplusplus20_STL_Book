// 第 4 章 4.1 迭代器基础：迭代器是什么、range-for 怎么展开、迭代器的类别、C++20 迭代器概念
// 编译：clang++ -std=c++23 -Wall -Wextra -Wpedantic iterators.cpp -o iterators.exe
//       g++     -std=c++23 -Wall -Wextra -Wpedantic iterators.cpp -o iterators.exe -lstdc++exp
//
// ============================================================
// 迭代器是什么？
//   sort、find、accumulate 这些算法不想关心容器是 vector 还是 list，
//   它们只需要一个“能指向某个元素、能往下走一步、能读出元素”的东西 —— 这就是迭代器。
//   迭代器故意模仿指针的写法：
//       *it          读/写它指向的元素         （指针：*p）
//       ++it         走到下一个元素             （指针：++p）
//       it == other  两个迭代器是不是同一个位置 （指针：p == q）
//   所以原生指针本身就是一种迭代器，算法对数组和容器可以写成同一份代码。
//
// begin() 和 end()：每个容器给出一对迭代器，表示“左闭右开”区间 [begin, end)
//
//       下标：    [0]   [1]   [2]   [3]   [4]
//       元素：     1     2     3     4     5    (这里没有元素)
//                  ^                               ^
//               begin()                          end()
//
//   end() 指向最后一个元素的“后一个位置”，那里没有元素，不能 *end()。为什么这样设计：
//     * 遍历永远写成 for (it = begin; it != end; ++it)，不用特判
//     * 空容器 begin() == end()，循环一次都不进
//     * 能做减法的迭代器：元素个数 = end - begin
//
// 迭代器的类别：不同容器的迭代器“能力”不一样（[4] 打印一张实际的表）
//
//   类别              能做什么                                        例子
//   ----------------  ----------------------------------------------  -----------------------------
//   输入 input        *it 读；++it；只能从头到尾走一遍（单趟）         istream_iterator（从流里读）
//   前向 forward      输入的全部 + 可以多趟遍历                        forward_list、unordered_set
//   双向 bidirectional 前向的全部 + --it 往回走                        list、map、set
//   随机访问 random   双向的全部 + it+n、it-n、it[n]、it2-it1、it1<it2 deque
//     access            （都是 O(1)，一步跳过去，不是一步步走）
//   连续 contiguous   随机访问的全部 + 元素在内存里紧挨着               vector、array、string、原生数组
//   输出 output       *it = x 写；++it；单趟（另一条线：只写不读）       back_inserter、ostream_iterator
//
//   从上到下能力越来越强，下面的类别包含上面类别的全部能力：
//       input → forward → bidirectional → random access → contiguous
//   一个算法只要求它真正需要的最弱的类别。例如 std::find 只要 input（从头读到尾就行），
//   std::reverse 要 bidirectional（要从两头往中间走），std::sort 要 random access（要跳着访问）
//
// C++20 把这些类别写成了概念（concept，1.7 节），在 <iterator> 里：
//       std::input_iterator<It>          std::forward_iterator<It>
//       std::bidirectional_iterator<It>  std::random_access_iterator<It>
//       std::contiguous_iterator<It>     std::output_iterator<It, T>   （能把 T 类型的值写进去）
//   对“整个容器/范围”也有对应的概念，在 <ranges> 里：
//       std::ranges::input_range<R>  ...  std::ranges::random_access_range<R>  std::ranges::contiguous_range<R>
//   它们是由几个更小的概念拼起来的（了解即可）：
//       std::indirectly_readable<It>     能用 *it 读出值
//       std::indirectly_writable<It, T>  能用 *it = t 写入        （书上拼成了 indirectly_writeable，错的）
//       std::weakly_incrementable<It>    能 ++it                   （不要求“相等保持”，[5] 解释）
//       std::incrementable<It>           能 ++it，并且 a == b 时 ++a 和 ++b 也相等（能多趟遍历的基础）
//       std::input_or_output_iterator<It> 能 *it、能 ++it —— 所有迭代器的最低要求
//       std::sentinel_for<S, It>         S 能和 It 比较 ==，用来表示“走到头了”（4.8 节讲哨兵）
//       std::sized_sentinel_for<S, It>   还能用 s - it 在 O(1) 时间算出距离
//   书上这张概念表排版错位了：左边的概念名和右边的描述对不上，以上面这张为准。
// ============================================================
#include <array>
#include <deque>
#include <forward_list>
#include <iterator>
#include <list>
#include <map>
#include <print>
#include <ranges>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

// ------------------------------------------------------------
// [3] 用：一个自己写的“容器”，没有成员函数 begin()/end()，
//     而是在同一个命名空间里写了普通函数 begin(Basket&)、end(Basket&)
// ------------------------------------------------------------
namespace shop
{
    struct Basket
    {
        int items[3]{4, 5, 6};
    };

    int* begin(Basket& b)
    {
        return b.items;
    }

    int* end(Basket& b)
    {
        return b.items + 3;
    }
}

// ------------------------------------------------------------
// [4] 用：判断迭代器类型 It 属于哪个类别
//     注意这里用 if constexpr 是对的：条件 std::contiguous_iterator<It> 只依赖类型 It，
//     编译期就知道真假（对比 1.6、2.6 节的错误：if constexpr 的条件里不能用函数参数）
//     从最强的类别开始判断，第一个满足的就是它“最强能到的类别”
// ------------------------------------------------------------
template <typename It>
constexpr std::string_view category_of()
{
    if constexpr (std::contiguous_iterator<It>)
    {
        return "contiguous    连续";
    }
    else if constexpr (std::random_access_iterator<It>)
    {
        return "random access 随机访问";
    }
    else if constexpr (std::bidirectional_iterator<It>)
    {
        return "bidirectional 双向";
    }
    else if constexpr (std::forward_iterator<It>)
    {
        return "forward       前向";
    }
    else if constexpr (std::input_iterator<It>)
    {
        return "input         输入";
    }
    else if constexpr (std::input_or_output_iterator<It>)
    {
        return "output        输出（只能写）";
    }
    else
    {
        return "不是迭代器";
    }
}

template <typename It>
void show_category(std::string_view name)
{
    std::println("  {:<36} {}", name, category_of<It>());
}

// ------------------------------------------------------------
// [7] 用：书上的 printc —— 要求容器的迭代器是随机访问迭代器
//     问题：约束写成 typename T::iterator，只有“有成员类型 iterator 的类”才可能满足，
//           原生数组 int[5] 根本没有 ::iterator，于是被拒绝了，尽管数组明明能随机访问
// ------------------------------------------------------------
template <typename T>
    requires std::random_access_iterator<typename T::iterator>
void printc_book(const T& c)
{
    for (auto e : c)
    {
        std::print("{} ", e);
    }
    std::println("");
    std::println("  element 0: {}", c[0]);
}

// 改进版：直接约束“范围”本身是随机访问范围 —— 数组、vector、deque、array、string 都行
//   另外书上没检查空容器：空容器的 c[0] 是未定义行为
//   取第 0 个元素写成 std::ranges::begin(c)[0]：随机访问迭代器一定支持 it[n]，
//   而 random_access_range 并不保证容器本身有 operator[]
template <std::ranges::random_access_range R>
void printc(const R& c)
{
    std::print("  ");
    for (const auto& e : c)
    {
        std::print("{} ", e);
    }
    std::println("");
    if (!std::ranges::empty(c))
    {
        std::println("  element 0: {}", std::ranges::begin(c)[0]);
    }
}

// 检查“能不能调用 printc_book”的小概念（requires 表达式要放在模板里，才会在不满足时得到 false 而不是编译错误）
template <typename T>
concept can_printc_book = requires(const T& t) { printc_book(t); };

// ------------------------------------------------------------
// [8] 用：同名函数写两个版本，约束不同，编译器自动选“更具体”的那个
//     random_access_iterator 包含了 forward_iterator 的全部要求（它是“更强”的概念），
//     所以两个都能用时（比如 vector），编译器选随机访问版
//     标准库的 std::advance、std::distance 内部就是这样按类别选不同实现的
// ------------------------------------------------------------
template <std::forward_iterator It>
std::string_view jump_cost(It)
{
    return "一步一步走，O(n)";
}

template <std::random_access_iterator It>
std::string_view jump_cost(It)
{
    return "直接跳过去，O(1)";
}

int main()
{
    // ------------------------------------------------------------
    // [1] 迭代器和指针用起来一模一样
    // ------------------------------------------------------------
    std::println("[1] 迭代器和指针");
    {
        const int a[]{1, 2, 3, 4, 5};
        std::print("  指针遍历数组：  ");
        for (const int* p = a; p != a + 5; ++p)
        {
            std::print("{} ", *p);
        }
        std::println("");

        std::vector<int> v{1, 2, 3, 4, 5};
        std::print("  迭代器遍历 vector：");
        for (auto it = v.begin(); it != v.end(); ++it)
        {
            std::print("{} ", *it);
        }
        std::println("");

        // 迭代器的完整类型名很长，所以平时都写 auto
        //   书上举的例子 std::vector<std::vector<int, std::string>> 是错的：
        //   vector 的第二个模板参数是“分配器”，不是元素类型，应该是 std::vector<std::string>
        std::vector<std::string> words{"apple", "pear"};
        std::vector<std::string>::iterator w_it = words.begin(); // 等价于 auto w_it = words.begin();
        *w_it = "APPLE";                                          // iterator 可以通过 * 修改元素
        std::vector<std::string>::const_iterator c_it = words.cbegin();
        // *c_it = "x";   // 编译错误：const_iterator 只能读，不能改（就像 const int*）
        std::println("  words[0] = {}，通过 const_iterator 读：{}", words[0], *c_it);

        std::vector<int> empty;
        std::println("  空 vector：begin() == end() 是 {}", empty.begin() == empty.end());
    }

    // ------------------------------------------------------------
    // [2] range-for 是怎么展开的
    //   for (const auto& e : v) { 循环体 }
    //   编译器把它改写成（C++20 标准里的规则）：
    //     {
    //         auto&& range_ = v;            // 先把容器绑定到一个引用上（2.4 节讲过它的生命周期问题）
    //         auto begin_ = 起点;
    //         auto end_   = 终点;           // C++17 起 begin_ 和 end_ 可以是不同类型（4.8 节哨兵）
    //         for (; begin_ != end_; ++begin_)
    //         {
    //             const auto& e = *begin_;  // 你写的循环变量，每一圈用 *begin_ 初始化
    //             循环体
    //         }
    //     }
    //   “起点/终点”怎么找（书上说是调用 std::begin()/std::end()，不准确）：
    //     ① 原生数组：range_ 和 range_ + 元素个数，就是两个指针
    //     ② 类有成员函数 begin()/end()：range_.begin() 和 range_.end()
    //     ③ 都不是：调用普通函数 begin(range_)、end(range_)，按 ADL 查找
    //        （ADL：到参数类型所在的命名空间里找同名函数，[3] 演示）
    // ------------------------------------------------------------
    std::println("\n[2] 手动展开 range-for");
    {
        std::vector<int> v{10, 20, 30};
        std::print("  range-for：");
        for (const auto& e : v)
        {
            std::print("{} ", e);
        }
        std::println("");

        std::print("  手动展开：");
        {
            auto&& range_ = v;
            auto begin_ = range_.begin();
            auto end_ = range_.end();
            for (; begin_ != end_; ++begin_)
            {
                const auto& e = *begin_;
                std::print("{} ", e);
            }
        }
        std::println("");

        // 原生数组没有成员函数，range-for 直接用指针
        int arr[]{7, 8, 9};
        std::print("  原生数组：");
        for (int x : arr)
        {
            std::print("{} ", x);
        }
        std::println("  （std::begin(arr) 也能用，返回 int*：{}）", std::begin(arr) == &arr[0]);
    }

    // ------------------------------------------------------------
    // [3] 情况 ③：没有成员 begin()，靠 ADL 找到 shop::begin
    //   basket 的类型是 shop::Basket，所以编译器会去 shop 命名空间里找 begin(basket)
    //   书上说 range-for 调用 std::begin() —— 如果真是那样，这里就编译不过了：
    //     std::begin(basket);   // 编译错误：std::begin(c) 内部调用 c.begin()，Basket 没有这个成员
    //   std::ranges::begin 比 std::begin 聪明：成员找不到时也会按 ADL 找，所以能用
    // ------------------------------------------------------------
    std::println("\n[3] range-for 通过 ADL 找到 shop::begin/shop::end");
    {
        shop::Basket basket;
        std::print("  ");
        for (int x : basket)
        {
            std::print("{} ", x);
        }
        std::println("");
        std::println("  std::ranges::begin(basket) 指向 {}", *std::ranges::begin(basket));
    }

    // ------------------------------------------------------------
    // [4] 实际看看每种迭代器属于哪个类别
    //   两个值得注意的：
    //     * deque 是随机访问但不是连续：它的元素分成好几块存放（3.11 节的图）
    //     * vector<bool> 也不是连续：它把 8 个 bool 压进一个字节，每个元素只占 1 位，
    //       没法给出一个 bool* 指向“第 3 个元素”（思考题 2）
    // ------------------------------------------------------------
    std::println("\n[4] 迭代器类别");
    show_category<int*>("int*（原生指针）");
    show_category<std::vector<int>::iterator>("vector<int>::iterator");
    show_category<std::array<int, 3>::iterator>("array<int,3>::iterator");
    show_category<std::string::iterator>("string::iterator");
    show_category<std::deque<int>::iterator>("deque<int>::iterator");
    show_category<std::vector<bool>::iterator>("vector<bool>::iterator");
    show_category<std::list<int>::iterator>("list<int>::iterator");
    show_category<std::map<int, int>::iterator>("map<int,int>::iterator");
    show_category<std::set<int>::iterator>("set<int>::iterator");
    show_category<std::forward_list<int>::iterator>("forward_list<int>::iterator");
    show_category<std::unordered_set<int>::iterator>("unordered_set<int>::iterator");
    show_category<std::istream_iterator<int>>("istream_iterator<int>");
    show_category<std::back_insert_iterator<std::vector<int>>>("back_insert_iterator<vector<int>>");
    show_category<std::ostream_iterator<int>>("ostream_iterator<int>");
    show_category<std::ranges::iterator_t<std::ranges::iota_view<int, int>>>("views::iota(0, 10) 的迭代器");
    show_category<int>("int");

    // ------------------------------------------------------------
    // [5] 输入迭代器为什么“只能走一遍”
    //   istream_iterator<int> 每次 ++ 都从流里读一个新数，读过的数据就从流里消失了。
    //   复制一个迭代器 b = a，a 和 b 读的还是同一个流：
    //     流里是 "1 2 3"
    //     a 构造时读到 1；b = a 复制，b 也是 1
    //     ++a：从流里读下一个 → a 是 2
    //     ++b：流里下一个已经是 3 了 → b 是 3（而不是 2！）
    //   这就是“不相等保持”：b 是 a 的复制品，但 ++a 和 ++b 得到的东西不一样。
    //   forward 要求复制一份之后各走各的、结果一样（这样才能多趟遍历），istream_iterator 做不到，所以只能是 input
    //
    //   注意一个坑：std::incrementable<istream_iterator<int>> 实际是 true！（下面打印出来）
    //   概念只能检查“语法”：有没有 ++、++ 返回什么类型、能不能复制、能不能 ==。
    //   “a == b 时 ++a 和 ++b 也相等”这种“语义”要求编译器没法检查，只是写在标准里的约定。
    //   istream_iterator 语法上全都有，所以 incrementable 是 true，但它实际上违反了语义要求。
    //   那 forward_iterator<istream_iterator<int>> 怎么做到是 false 的？
    //   forward_iterator 还要求迭代器自己“声明”类别：它里面的类型 iterator_category 必须是
    //   forward_iterator_tag 或更强的标签，而 istream_iterator 声明的是 input_iterator_tag。
    //   换句话说，类别最终靠迭代器自己声明（4.4 节自己写迭代器时要写这些声明）
    // ------------------------------------------------------------
    std::println("\n[5] 输入迭代器只能走一遍");
    {
        std::istringstream in{"1 2 3"};
        std::istream_iterator<int> a{in};
        auto b = a;
        std::println("  一开始    *a = {}，*b = {}", *a, *b);
        ++a;
        ++b;
        std::println("  各 ++ 一次 *a = {}，*b = {}", *a, *b);

        std::vector<int> v{1, 2, 3};
        auto x = v.begin();
        auto y = x;
        ++x;
        ++y;
        std::println("  vector 的迭代器复制后各 ++ 一次：*x = {}，*y = {}（一样，所以是 forward 以上）", *x, *y);
        std::println("  incrementable<istream_iterator<int>>   = {}（只检查语法，所以是 true）",
                     std::incrementable<std::istream_iterator<int>>);
        std::println("  forward_iterator<istream_iterator<int>> = {}（它声明自己是 input_iterator_tag）",
                     std::forward_iterator<std::istream_iterator<int>>);
        static_assert(std::same_as<std::istream_iterator<int>::iterator_category, std::input_iterator_tag>);
    }

    // ------------------------------------------------------------
    // [6] 能力不够的迭代器怎么“跳”：std::next / std::prev / std::advance / std::distance
    //   list 的迭代器是双向的，没有 it + 2：
    //     auto it = l.begin() + 2;   // 编译错误：list 迭代器没有 operator+
    //   要用 std::next(it, 2)：对 list 是循环 ++ 两次（O(n)），对 vector 是直接 + 2（O(1)）
    //     std::next(it, n)     返回往后 n 步的新迭代器（it 本身不变）
    //     std::prev(it, n)     返回往前 n 步的新迭代器（需要双向）
    //     std::advance(it, n)  直接把 it 本身往后挪 n 步（没有返回值）
    //     std::distance(a, b)  从 a 走到 b 要几步（随机访问 O(1)，其它 O(n)）
    //   同理 std::sort(l.begin(), l.end()) 也编译不过：sort 需要随机访问，list 要用成员函数 l.sort()
    // ------------------------------------------------------------
    std::println("\n[6] std::next / prev / advance / distance");
    {
        std::list<int> l{10, 20, 30, 40, 50};
        auto it = std::next(l.begin(), 2);
        std::println("  next(begin, 2) → {}", *it);
        std::println("  prev(end)      → {}", *std::prev(l.end()));
        std::advance(it, -1);
        std::println("  advance(it, -1) 之后 it → {}", *it);
        std::println("  distance(begin, end) = {}", std::distance(l.begin(), l.end()));

        std::vector<int> v{10, 20, 30, 40, 50};
        std::println("  vector 可以直接 v.begin() + 2 → {}", *(v.begin() + 2));
        std::println("  vector 的 end - begin = {}", v.end() - v.begin());
    }

    // ------------------------------------------------------------
    // [7] 用概念约束模板参数
    //   printc_book(lst) 编译错误，因为 list 不是随机访问：
    //     clang：note: because 'typename std::list<int>::iterator' does not satisfy 'random_access_iterator'
    //            note: because 'is_base_of_v<std::random_access_iterator_tag, std::bidirectional_iterator_tag>'
    //                  evaluated to false
    //     g++  ：'std::random_access_iterator_tag' is not a base of 'std::bidirectional_iterator_tag'
    //   两个编译器最后都落在“标签”上：list 迭代器声明自己是 bidirectional_iterator_tag，不是随机访问（[5] 末尾）
    //   而 printc_book(arr) 也编译错误（这是书上的写法的问题）：
    //     clang：note: because substituted constraint expression is ill-formed: type 'int[5]' cannot be used
    //            prior to '::' because it has no members
    //     g++  ：error: 'int [5]' is not a class, struct, or union type
    //   改成约束 std::ranges::random_access_range 之后，数组也能用了
    // ------------------------------------------------------------
    std::println("\n[7] 概念约束");
    {
        std::vector<int> vec{1, 2, 3, 4, 5};
        int arr[]{6, 7, 8, 9, 10};
        std::list<int> lst{1, 2, 3};
        std::print("  printc_book(vec)：");
        printc_book(vec);
        // printc_book(arr);   // 编译错误：int[5] 没有 ::iterator
        // printc_book(lst);   // 编译错误：list 不是随机访问
        printc(vec);
        printc(arr);
        // printc(lst);        // 编译错误：list 不是 random_access_range（这个错误是我们想要的）
        printc(std::deque<int>{});
        std::println("  （空 deque 不打印 element 0）");
        std::println("  list 能用 printc 吗：{}；数组能用 printc_book 吗：{}",
                     std::ranges::random_access_range<std::list<int>>,
                     can_printc_book<int[5]>);
    }

    // ------------------------------------------------------------
    // [8] 两个重载，编译器按约束选“更具体”的那个
    // ------------------------------------------------------------
    std::println("\n[8] 按迭代器类别选重载");
    {
        std::vector<int> v{1, 2, 3};
        std::list<int> l{1, 2, 3};
        std::forward_list<int> f{1, 2, 3};
        std::println("  vector：{}", jump_cost(v.begin()));
        std::println("  list：  {}", jump_cost(l.begin()));
        std::println("  forward_list：{}", jump_cost(f.begin()));
        // jump_cost(std::istream_iterator<int>{});   // 编译错误：input 迭代器两个版本都不满足
    }

    // ------------------------------------------------------------
    // [9] 从迭代器类型得到相关的类型（4.4 节自己写迭代器时会用到）
    //   std::iter_value_t<It>        元素的类型（去掉 const 和引用）
    //   std::iter_reference_t<It>    *it 的类型
    //   std::iter_difference_t<It>   两个迭代器相减的类型（距离），一般是 std::ptrdiff_t
    //   范围版本：std::ranges::range_value_t<R>、std::ranges::iterator_t<R> 等
    // ------------------------------------------------------------
    std::println("\n[9] 迭代器的相关类型");
    {
        using It = std::vector<std::string>::const_iterator;
        static_assert(std::same_as<std::iter_value_t<It>, std::string>);
        static_assert(std::same_as<std::iter_reference_t<It>, const std::string&>);
        static_assert(std::same_as<std::iter_difference_t<It>, std::ptrdiff_t>);
        static_assert(std::same_as<std::ranges::range_value_t<int[3]>, int>);
        std::println("  static_assert 全部通过：vector<string>::const_iterator 的值类型 string，*it 是 const string&");
    }
}
