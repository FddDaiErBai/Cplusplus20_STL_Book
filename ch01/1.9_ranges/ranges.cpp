// 第 1 章 1.9 范围（range）和视图（view）
// 编译：clang++ -std=c++23 -Wall -Wextra ranges.cpp -o ranges.exe
#include <algorithm>
#include <functional>
#include <map>
#include <print>
#include <ranges>
#include <string>
#include <vector>

namespace ranges = std::ranges;
namespace views = std::views; // std::views 是 std::ranges::views 的别名（书上写成 std::view，少了 s）

struct Person
{
    std::string name;
    int age;
};

int main()
{
    const std::vector<int> nums{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

    // ------------------------------------------------------------
    // 1. 视图的两种写法：视图类 和 视图适配器（用 | 连接）
    //    C++23 的 std::println 可以直接打印范围，不用再写 for 循环
    // ------------------------------------------------------------
    std::println("[1] 基本用法");
    std::println("take_view(nums, 5)    = {}", ranges::take_view(nums, 5));
    std::println("nums | take(5)        = {}", nums | views::take(5));
    std::println("take(5) | reverse     = {}", nums | views::take(5) | views::reverse);
    std::println("reverse | take(5)     = {}", nums | views::reverse | views::take(5)); // 顺序不同，结果不同
    std::println("filter 偶数           = {}", nums | views::filter([](int i) { return i % 2 == 0; }));
    std::println("transform 平方        = {}", nums | views::transform([](int i) { return i * i; }));
    std::println("iota(1, 10)           = {}", views::iota(1, 10));               // [1, 10)，不包括 10
    std::println("iota(1) | take(5)     = {}", views::iota(1) | views::take(5)); // 无限序列，取前 5 个

    // ------------------------------------------------------------
    // 2. 视图是惰性的：创建视图时什么都不算，遍历时才一个一个算
    //    所以无限序列 iota(1) 不会卡死：take(3) 拿够 3 个就停
    // ------------------------------------------------------------
    std::println("\n[2] 惰性求值");
    auto noisy_square = [](int i)
    {
        std::println("    计算 {} 的平方", i);
        return i * i;
    };
    auto lazy = views::iota(1) | views::transform(noisy_square) | views::take(3);
    std::println("  视图创建好了，但还一次都没算");
    for (int x : lazy)
    {
        std::println("  拿到 {}", x);
    }

    // 坑：transform 后面接 filter，通过筛选的元素会被计算两次
    //    （filter 判断时取一次值，for 循环再取一次值，transform 不会把结果存起来）
    std::println("  transform | filter:");
    for (int x : views::iota(1, 5) | views::transform(noisy_square) | views::filter([](int i) { return i > 5; }))
    {
        std::println("  拿到 {}", x);
    }

    // ------------------------------------------------------------
    // 3. 视图不复制数据，它引用原来的容器
    //    改容器，视图看到的也变；通过视图也能改容器
    // ------------------------------------------------------------
    std::println("\n[3] 视图引用原容器");
    std::vector<int> v{1, 2, 3, 4, 5};
    auto first3 = v | views::take(3);
    v[0] = 100;
    std::println("改 v[0] 之后 first3 = {}", first3);
    for (int& x : v | views::reverse | views::take(2)) // 最后两个元素乘 10
    {
        x *= 10;
    }
    std::println("通过视图修改之后 v = {}", v);

    // ------------------------------------------------------------
    // 4. 范围版算法：std::ranges::xxx，直接传容器，不用传 begin/end
    // ------------------------------------------------------------
    std::println("\n[4] 范围算法");
    std::vector<int> s{9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    ranges::sort(s | views::drop(5)); // 跳过前 5 个，只排后面的（书上说“对前 5 个排序”是说反了）
    std::println("只排序 drop(5) 部分   = {}", s);

    // 投影（projection，书上没讲）：告诉算法“按哪个成员比较”，不用自己写比较函数
    std::vector<Person> people{{"Tom", 30}, {"Amy", 25}, {"Bob", 35}};
    ranges::sort(people, {}, &Person::age); // {} 表示用默认的比较方式 ranges::less
    std::println("按年龄排序            = {}", people | views::transform(&Person::name));
    auto oldest = ranges::max(people, {}, &Person::age);
    std::println("年龄最大的人          = {}", oldest.name);

    // ------------------------------------------------------------
    // 5. C++23 新增（书写于 C++20，没有这些）
    // ------------------------------------------------------------
    std::println("\n[5] C++23 新增");
    // ranges::to：把视图变回容器（视图是惰性的，想保存结果就要用它）
    std::vector<int> evens = nums | views::filter([](int i) { return i % 2 == 0; }) | ranges::to<std::vector>();
    std::println("ranges::to            = {}", evens);
    // fold_left：累加（以前要用 <numeric> 里的 std::accumulate，它不接受范围）
    std::println("fold_left 求和        = {}", ranges::fold_left(nums, 0, std::plus{}));
    // enumerate：同时拿到下标和元素
    for (auto [i, name] : views::enumerate(people | views::transform(&Person::name)))
    {
        std::println("  第 {} 名：{}", i + 1, name);
    }
    // zip：几个范围一一配对，长度取最短的
    std::vector<std::string> fruits{"apple", "banana", "cherry"};
    std::println("zip                   = {}", views::zip(fruits, nums));
    // stride：每隔 n 个取一个
    std::println("stride(3)             = {}", nums | views::stride(3));
    // keys / values：取 map（或 pair、tuple 的范围）的第一个 / 第二个成员
    std::map<std::string, int> stock{{"apple", 3}, {"banana", 5}};
    std::println("keys                  = {}", stock | views::keys);
    std::println("values                = {}", stock | views::values);
    // 注意：views::chunk（分组）和 views::slide（滑动窗口）也是 C++23 的，g++ 有，clang 的 libc++ 还没实现

    // ------------------------------------------------------------
    // 6. 常见的坑（书上没讲）
    // ------------------------------------------------------------
    // (a) iota 的两个参数要同为有符号或同为无符号（又是 1.4 节的问题）
    //     views::iota(0, v.size());  // 编译失败：0 是 int，v.size() 是 size_t
    std::println("\n[6] iota(0uz, size)   = {}", views::iota(0uz, v.size())); // uz 后缀（C++23）表示 size_t

    // (b) 范围算法传临时容器，返回的迭代器会“悬空”，标准库用 ranges::dangling 类型拦住你
    //     auto it = ranges::find(std::vector{1, 2, 3}, 2);
    //     *it;  // 编译失败：it 的类型是 ranges::dangling，不能解引用

    // (c) filter_view 不能用 const 遍历（它第一次 begin() 时要把结果缓存起来，需要修改自己）
    //     见练习的思考题
}
