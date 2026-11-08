// 第 4 章 4.1 迭代器基础 —— 练习
// 编译：clang++ -std=c++23 -Wall -Wextra -Wpedantic iterators_exercise.cpp -o iterators_exercise.exe
//       g++     -std=c++23 -Wall -Wextra -Wpedantic iterators_exercise.cpp -o iterators_exercise.exe -lstdc++exp
// 运行时检查：补完 TODO，编译运行，所有行都显示“通过”就说明做对了。
// 先看示例 iterators.cpp 开头的类别表，以及 [4] 打印出来的每种容器的类别。
#include <deque>
#include <forward_list>
#include <iterator>
#include <list>
#include <numeric>
#include <print>
#include <ranges>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

void check(bool ok, std::string_view name)
{
    std::println("{} {}", ok ? "通过" : "失败", name);
}

// ============================================================
// 练习 1：自己写 std::distance
//   my_distance(first, last) 返回从 first 走到 last 要走几步。
//     * It 是随机访问迭代器（vector、deque、指针……）：直接 return last - first;   一步算出，O(1)
//     * 其它迭代器（list、forward_list、istream_iterator……）：不能相减，
//       只能 ++first 一步一步走，数走了几步，直到 first == last                    O(n)
//   用 if constexpr 选分支，条件写 std::random_access_iterator<It>（示例 [4] 的 category_of 就是这么写的）
//   这里用 if constexpr 是对的：条件只依赖类型 It，编译期就知道
//   返回类型 std::iter_difference_t<It> 是“距离类型”（示例 [9]），一般就是 std::ptrdiff_t
// ============================================================
template <std::input_iterator It>
std::iter_difference_t<It> my_distance(It first, It last)
{
    if constexpr (std::random_access_iterator<It>)
    {
        return last - first;
    }
    else
    {
        std::iter_difference_t<It> count{};
        for (; first != last; ++first, ++count)
        {}
        return count;
    }
}

void test_exercise1()
{
    std::vector<int> v{1, 2, 3, 4, 5};
    std::list<int> l{1, 2, 3, 4, 5};
    std::forward_list<int> f{1, 2, 3};
    int arr[]{1, 2, 3, 4};
    std::vector<int> empty;
    check(my_distance(v.begin(), v.end()) == 5, "练习 1：vector");
    check(my_distance(l.begin(), l.end()) == 5, "练习 1：list");
    check(my_distance(std::next(l.begin()), std::prev(l.end())) == 3, "练习 1：list 中间一段");
    check(my_distance(f.begin(), f.end()) == 3, "练习 1：forward_list");
    check(my_distance(arr, arr + 4) == 4, "练习 1：原生数组的指针");
    check(my_distance(empty.begin(), empty.end()) == 0, "练习 1：空的");

    std::istringstream in{"5 6 7"};
    check(my_distance(std::istream_iterator<int>{in}, std::istream_iterator<int>{}) == 3,
          "练习 1：istream_iterator（只能一步一步走）");
}

// ============================================================
// 练习 2：给函数加上正确的约束
//   下面三个函数的函数体已经写好了，但都没有约束（template <typename R> 什么都收）。
//   请把每个 typename 换成合适的概念，写法例如：template <std::ranges::forward_range R>
//   能用的范围概念（从弱到强）：
//     std::ranges::input_range → forward_range → bidirectional_range → random_access_range → contiguous_range
//   规则：选“函数体真正需要的、最弱的”那个。
//     太弱：不能用的容器也放进来了（比如 forward_list 进了 last_of，到函数体里才报一堆错）
//     太强：能用的容器被拒绝了（比如 first_ptr 要求 contiguous 是对的，但 middle_of 要求 contiguous 就把 deque 拒了）
//   不要学书上写 std::random_access_iterator<typename R::iterator>：原生数组没有 ::iterator（示例 [7]）
//
//   last_of(r)  ：最后一个元素。函数体用了 std::prev(end)，要往回走一步
//   middle_of(r)：中间那个元素。函数体用了 begin(r)[n]，要直接跳到第 n 个
//   first_ptr(r)：指向第一个元素的指针。函数体用了 std::ranges::data(r)，元素要在内存里紧挨着
//
//   下面的 can_xxx 是检查“能不能调用 xxx(r)”的小概念（和示例 [7] 的 can_printc_book 一样），不用改
// ============================================================
template <std::ranges::bidirectional_range R>
std::ranges::range_value_t<R> last_of(const R& r)
{
    return *std::prev(std::ranges::end(r));
}

template <std::ranges::random_access_range R>
std::ranges::range_value_t<R> middle_of(const R& r)
{
    return std::ranges::begin(r)[std::ranges::size(r) / 2];
}

template <std::ranges::contiguous_range R>
const std::ranges::range_value_t<R>* first_ptr(const R& r)
{
    return std::ranges::data(r);
}

template <typename R>
concept can_last_of = requires(const R& r) { last_of(r); };

template <typename R>
concept can_middle_of = requires(const R& r) { middle_of(r); };

template <typename R>
concept can_first_ptr = requires(const R& r) { first_ptr(r); };

void test_exercise2()
{
    using std::deque, std::forward_list, std::list, std::set, std::string, std::vector;

    check(can_last_of<vector<int>> && can_last_of<int[3]> && can_last_of<string> && can_last_of<deque<int>>
              && can_last_of<list<int>> && can_last_of<set<int>>,
          "练习 2：last_of 接受 vector、数组、string、deque、list、set");
    check(!can_last_of<forward_list<int>>, "练习 2：last_of 拒绝 forward_list（不能往回走）");

    check(can_middle_of<vector<int>> && can_middle_of<int[3]> && can_middle_of<string> && can_middle_of<deque<int>>
              && can_middle_of<vector<bool>>,
          "练习 2：middle_of 接受 vector、数组、string、deque、vector<bool>");
    check(!can_middle_of<list<int>> && !can_middle_of<set<int>> && !can_middle_of<forward_list<int>>,
          "练习 2：middle_of 拒绝 list、set、forward_list（不能跳）");

    check(can_first_ptr<vector<int>> && can_first_ptr<int[3]> && can_first_ptr<string>,
          "练习 2：first_ptr 接受 vector、数组、string");
    check(!can_first_ptr<deque<int>> && !can_first_ptr<vector<bool>> && !can_first_ptr<list<int>>,
          "练习 2：first_ptr 拒绝 deque、vector<bool>、list（元素不是紧挨着的）");

    vector<int> v{1, 2, 3, 4, 5};
    list<int> l{7, 8, 9};
    deque<int> d{10, 20, 30, 40};
    int arr[]{4, 5, 6};
    check(last_of(v) == 5 && last_of(l) == 9 && last_of(arr) == 6, "练习 2：last_of 的值");
    check(middle_of(v) == 3 && middle_of(d) == 30 && middle_of(string{"abc"}) == 'b', "练习 2：middle_of 的值");
    check(first_ptr(v) == v.data() && first_ptr(arr) == &arr[0], "练习 2：first_ptr 的值");
}

// ============================================================
// 练习 3：找 bug
//   average(first, last) 求平均值，约束是 std::input_iterator，也就是说它声称“输入迭代器也能用”。
//   std::accumulate(first, last, 0.0)：从 0.0 开始，把 [first, last) 里的每个元素加上去，返回总和（<numeric>）
//   运行后会看到：vector、list 的检查通过，istringstream 的检查失败。
//   找出原因并修好（保持 std::input_iterator 约束不变，istream_iterator 也要能用）。
//   提示：示例 [5] —— 输入迭代器能走几遍？这个函数把 [first, last) 走了几遍？
// ============================================================
template <std::input_iterator It>
double average(It first, It last)
{
    if constexpr (!std::forward_iterator<It>)
    {
        if (first == last)
        {
            return 0.0;
        }
        std::iter_difference_t<It> count{};
        double sum{};
        for (; first != last; ++first)
        {
            sum += *first;
            ++count;
        }
        return sum / static_cast<double>(count);
    }
    else
    {
        // 改动（补充）：这个分支是对的，但其实不需要。上面那个一趟循环对所有迭代器都能用（forward 也是 input），
        //   只留那个循环、去掉 if constexpr 就够了。对 list 来说，distance + accumulate 还要从头走两遍，一趟循环只走一遍。
        //   if constexpr 分两种写法，适合的是“强的迭代器有更快的做法”，比如练习 1：随机访问 O(1)，其它 O(n)。
        //   这里两个分支都是 O(n)，分开写只是多了代码。
        auto n = std::distance(first, last);
        if (n == 0)
        {
            return 0.0;
        }
        double sum = std::accumulate(first, last, 0.0);
        return sum / static_cast<double>(n);
    }
}

double average_of_text(std::string text)
{
    std::istringstream in{text};
    return average(std::istream_iterator<int>{in}, std::istream_iterator<int>{});
}

void test_exercise3()
{
    std::vector<int> v{1, 2, 3, 4};
    std::list<int> l{2, 4};
    check(average(v.begin(), v.end()) == 2.5, "练习 3：vector");
    check(average(l.begin(), l.end()) == 3.0, "练习 3：list");
    check(average(v.end(), v.end()) == 0.0, "练习 3：空的");
    check(average_of_text("1 2 3 4") == 2.5, "练习 3：istringstream \"1 2 3 4\"");
    check(average_of_text("10") == 10.0, "练习 3：istringstream \"10\"");
    check(average_of_text("") == 0.0, "练习 3：istringstream 空字符串");
}

// ============================================================
// 思考题 3 用：运行后看输出（不用改）
//   流里是 "1 2 3"，b 是 a 的复制品，然后 a、b 各 ++ 一次
// ============================================================
void q3_demo()
{
    std::istringstream in{"1 2 3"};
    std::istream_iterator<int> a{in};
    auto b = a;
    ++a;
    ++b;
    std::println("思考题 3：*a = {}，*b = {}，a == b 是 {}", *a, *b, a == b);
}

int main()
{
    test_exercise1();
    std::println("");
    test_exercise2();
    std::println("");
    test_exercise3();
    std::println("");
    q3_demo();
}

// ============================================================
// 思考题
// （1）std::list<int> l{1, 2, 3, 4, 5};
//      auto it = l.begin() + 2;            为什么编译不过？应该怎么写？代价是 O(1) 还是 O(n)？
//      std::sort(l.begin(), l.end());      为什么也编译不过？list 要怎么排序？
// 你的答案：list不支持随机访问。std::next(l,2)。O(n)。
// sort要求随机访问。使用成员函数l.sort()
// 改动（补充）：std::next 的参数是迭代器，不是容器：std::next(l.begin(), 2)。
//   sort 要随机访问，是因为它要跳着访问：直接取中间元素 first + (last - first) / 2、
//   用 it2 - it1 算区间多长。list 只能一步步走，做这些都是 O(n)，sort 就慢了。
//   所以标准干脆用约束拒绝它。l.sort() 是归并排序，只改节点之间的指针，不需要跳。
//
// （2）示例 [4] 里 vector<bool> 的迭代器是“随机访问”，却不是“连续”。为什么？
//      提示：“连续”意味着可以拿一个 bool* 指针指向第 n 个元素。vector<bool> 里一个元素占多大？
//      可以写一行 static_assert(std::same_as<std::iter_reference_t<std::vector<bool>::iterator>, bool&>);
//      编译试试，看看 *it 到底是不是 bool&（编译器的报错会告诉你它实际是什么类型）
// 你的答案：vector<bool>把 8 个 bool 压进一个字节，每个元素只占 1 位。
// 改动（补充）：还差“所以呢”这一步。指针最小只能指向一个字节，没法指向字节里的某一位，
//   所以拿不到 bool* 去指向第 n 个元素，也就不可能是连续的。*it 也给不出 bool&（引用和指针一样，只能指向整个字节）。
//   *it 返回的是一个“代理对象”，它记住“哪个字节的第几位”，对它赋值时再去改那一位。
//   提示里那行 static_assert 的报错会显示出代理对象的类型：
//     clang（libc++）：std::__bit_reference<std::vector<bool>>
//     g++（libstdc++）：std::_Bit_reference
//   那为什么还是随机访问？第 n 个元素就在第 n / 8 个字节的第 n % 8 位，一次就能算出来，是 O(1) 的。
//
// （3）运行程序，把最后一行“思考题 3：……”的输出贴在这里。
//      输出：思考题 3：*a = 2，*b = 3，a == b 是 true
//      a == b 打印的是 true 还是 false？既然 a == b，为什么 *a 和 *b 不一样？
//      由此猜一猜：两个 istream_iterator 的 == 比较的到底是什么？
// 你的答案：true。a==b比较的是迭代器类型吧？
// 改动（补充）：true 对。但比较的不是类型：类型是编译期的事，a 和 b 都是 istream_iterator<int>，
//   类型不同的话，== 根本编译不过，不会运行到 false。
//   实际比较的是“读的是不是同一个流”：每个 istream_iterator 里存着一个指向流的指针，== 就是比较这两个指针。
//   a 和 b 读的都是 in，所以 a == b，跟它们手里存的数（2 和 3）无关。
//   反过来，两个不同的流里都是 "7"，*x 和 *y 都是 7，x == y 却是 false。（我验证过）
//   结束迭代器 std::istream_iterator<int>{} 里的指针是空的。读失败时，迭代器会把自己的指针也设成空的，
//   于是就和结束迭代器相等了。所以 for (; first != last; ++first) 能在读完时停下来。
//   流里是 "1 2" 时，每次 ++ 之前 a == end 依次是：false（a 存着 1）、false（存着 2）、true（再读失败了）
//   注意：已经等于 end 之后不能再 ++。clang 上会直接崩溃（Segmentation fault），这和 *v.end() 一样是未定义行为。
//
// ============================================================
