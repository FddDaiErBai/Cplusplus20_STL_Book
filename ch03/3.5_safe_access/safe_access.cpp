// 第 3 章 3.5 安全地访问 vector 元素（[] 和 at()）
// 编译：clang++ -std=c++23 -Wall -Wextra safe_access.cpp -o safe_access.exe
// 运行：./safe_access.exe        正常部分
//       ./safe_access.exe oob    额外执行一次越界的 v[5]（练习的思考题 (1)）
#include <cstdio>
#include <map>
#include <print>
#include <ranges>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

// ------------------------------------------------------------
// 6. at() 在编译时越界：抛异常 = 编译失败。所以练习里可以用 static_assert 抓越界
// ------------------------------------------------------------
constexpr int third(const std::vector<int>& v)
{
    return v.at(2);
}

static_assert(third({1, 2, 3}) == 3);
// static_assert(third({1, 2}) == 0); // 编译失败：note 里能看到 at(2) 越界

int main(int argc, char* argv[])
{
    std::vector v{19, 71, 47, 192, 4004};

    // ------------------------------------------------------------
    // 1. [] 不检查，at() 检查，越界时抛出 std::out_of_range
    //    异常信息（what()）每个标准库不一样，书上的是 g++ 的：
    //      g++  ："vector::_M_range_check: __n (which is 5) >= this->size() (which is 5)"
    //      clang（libc++）："vector"  —— 就这一个词，什么都没说
    //    所以 what() 只适合打印给人看，程序不要靠它的内容做判断
    // ------------------------------------------------------------
    std::println("[1] [] 和 at()");
    std::println("v[2] = {}，v.at(2) = {}", v[2], v.at(2));
    try
    {
        v.at(5) = 2001;
        std::println("这一行不会执行");
    }
    catch (const std::out_of_range& e)
    {
        std::println("捕获到 out_of_range：{}", e.what());
    }
    std::println("v 没被改坏：{}", v);

    // ------------------------------------------------------------
    // 2. 下标是 size_t（无符号）。传一个 int -1 进去，会先变成 size_t 的最大值
    //    18446744073709551615（2.3 节 make_commas 的同一个道理），然后才和 size() 比较
    //    g++ 的异常信息里能直接看到这个数
    // ------------------------------------------------------------
    std::println("\n[2] 负数下标");
    int i = -1;
    try
    {
        std::println("{}", v.at(i));
    }
    catch (const std::out_of_range& e)
    {
        std::println("at(-1) 也越界了：{}", e.what());
    }
    std::println("-1 变成 size_t 是 {}", static_cast<std::size_t>(i));

    // ------------------------------------------------------------
    // 3. 越界的 []：书上说会“悄悄读到 0”“悄悄写进去”，那是 2021 年左右的 g++。现在：
    //      clang 默认                         ：悄悄读到一个垃圾值（未定义行为，结果没有保证）
    //      clang -D_LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_FAST ：直接崩溃（Illegal instruction）
    //      g++ 15+ 不开优化（-O0）            ：断言失败，程序终止（默认开了 _GLIBCXX_ASSERTIONS）
    //      g++ -O2                            ：断言没开，悄悄读到垃圾值
    //    C++26 把这叫做“标准库加固”（hardening）：[] 越界不再是未定义行为，而是必须被发现
    //    （2.2 节 span 也讲过同样的内容）
    // ------------------------------------------------------------
    if (argc > 1 && std::string_view{argv[1]} == "oob")
    {
        std::println("\n[3] 越界的 v[5]");
        std::fflush(stdout); // 如果下一行崩溃，确保上面的内容已经输出了
        std::println("v[5] = {}", v[5]);
    }

    // ------------------------------------------------------------
    // 4. front() / back() 在空 vector 上同样是未定义行为，而且它们没有“带检查”的版本
    //    先用 empty() 判断
    // ------------------------------------------------------------
    std::println("\n[4] front / back");
    std::vector<int> empty;
    if (!empty.empty())
    {
        std::println("{}", empty.front());
    }
    else
    {
        std::println("空的，不能 front()");
    }

    // ------------------------------------------------------------
    // 5. 最安全的下标是“不用下标”
    //    书上说 at() 应该是首选。更好的做法是：能不用下标就不用，没有下标就不会越界
    //      遍历全部          → 范围 for
    //      既要元素又要下标  → C++23 views::enumerate
    //      查找、求和……      → 算法（第 5 章）
    //    at() 适合下标来自“外面”的时候：用户输入、文件、网络……你没法保证它是对的
    // ------------------------------------------------------------
    std::println("\n[5] 不用下标");
    int total = 0;
    for (int x : v)
    {
        total += x;
    }
    std::println("总和 {}", total);
    for (auto [idx, x] : std::views::enumerate(v))
    {
        std::print("v[{}]={} ", idx, x);
    }
    std::println("");

    // ------------------------------------------------------------
    // 7. map 也有 [] 和 at()，但 map 的 [] 更危险：找不到的键会被“插入”（3.7 节细讲）
    //    只想查、不想改的时候用 at() 或 find()
    // ------------------------------------------------------------
    std::println("\n[7] map");
    std::map<std::string, int> ages{{"Tom", 20}};
    try
    {
        std::println("{}", ages.at("Jerry"));
    }
    catch (const std::out_of_range& e)
    {
        std::println("ages.at(\"Jerry\") 抛出异常：{}", e.what());
    }
    if (auto it = ages.find("Jerry"); it != ages.end()) // 2.4 节的 if 初始化
    {
        std::println("Jerry {} 岁", it->second);
    }
    else
    {
        std::println("find 没找到 Jerry，map 没变：{}", ages);
    }
}
