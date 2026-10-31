// 第 3 章 3.4 常数时间内从未排序的 vector 中删除元素（“用最后一个元素补位”）
// 编译：clang++ -std=c++23 -Wall -Wextra quick_delete.cpp -o quick_delete.exe
#include <algorithm>
#include <cstddef>
#include <print>
#include <string>
#include <utility> // std::move 在这里（书上说在 <algorithm>，不对，见第 4 部分）
#include <vector>

// ------------------------------------------------------------
// 1. 书上的 quick_delete：把最后一个元素移动到要删除的位置，再删掉最后一个
//    v[idx] 原来的值被覆盖掉了（等于删除了），最后一个元素“搬家”到了 idx，
//    pop_back 删掉的是已经被搬空的末尾 —— 只动了一个元素，所以是 O(1)
//    代价：元素的顺序变了
// ------------------------------------------------------------
template <typename T>
void quick_delete(T& v, std::size_t idx)
{
    if (idx < v.size())
    {
        v[idx] = std::move(v.back());
        v.pop_back();
    }
}

// 书上的迭代器版本。只检查了 it < v.end()，没有检查 it >= v.begin()；
// 而且 it 必须是这个 v 的迭代器，传别的 vector 的迭代器进来，比较本身就是未定义行为
template <typename T>
void quick_delete(T& v, typename T::iterator it)
{
    if (it < v.end())
    {
        *it = std::move(v.back());
        v.pop_back();
    }
}

// ------------------------------------------------------------
// 2. 数一数到底挪动了几次：Counter 每被移动赋值一次，计数加 1
// ------------------------------------------------------------
struct Counter
{
    inline static int moves = 0; // 所有 Counter 对象共用的计数器
    int value;

    Counter(int v) : value{v}
    {
    }

    Counter(const Counter&) = default;
    Counter(Counter&&) = default;
    Counter& operator=(const Counter&) = default;

    Counter& operator=(Counter&& other) noexcept
    {
        ++moves;
        value = other.value;
        return *this;
    }

    bool operator==(const Counter&) const = default;
};

std::vector<Counter> make_counters(int n)
{
    std::vector<Counter> v;
    for (int i = 0; i < n; ++i)
    {
        v.emplace_back(i);
    }
    return v;
}

void print_counters(std::string_view title, const std::vector<Counter>& v)
{
    std::print("{:<26}", title);
    for (const auto& c : v)
    {
        std::print("{} ", c.value);
    }
    std::println("（移动了 {} 次）", Counter::moves);
}

int main()
{
    std::println("[1] 书上的例子");
    std::vector v{12, 196, 47, 38, 19};
    std::println("开始：           {}", v);
    quick_delete(v, std::ranges::find(v, 47));
    std::println("删除 47（迭代器）：{}", v); // 19 补到了 47 的位置
    quick_delete(v, 1);
    std::println("删除下标 1：      {}", v); // 38 补到了 196 的位置

    // ------------------------------------------------------------
    // 2. 和 std::erase 对比：删除第一个元素
    //    erase：后面 9 个元素每个往前挪一格，顺序不变，移动 9 次
    //    quick_delete：只把最后一个搬过来，移动 1 次，但 9 跑到了最前面
    // ------------------------------------------------------------
    std::println("\n[2] 移动次数对比（10 个元素，删除第一个）");
    auto a = make_counters(10);
    Counter::moves = 0;
    a.erase(a.begin());
    print_counters("v.erase(v.begin())：", a);

    auto b = make_counters(10);
    Counter::moves = 0;
    quick_delete(b, 0);
    print_counters("quick_delete(v, 0)：", b);

    // ------------------------------------------------------------
    // 3. 删除最后一个元素时，书上的写法会“自己移动给自己”：v[idx] 和 v.back() 是同一个对象
    //    v[last] = std::move(v[last]);
    //    标准库的类型（string、vector……）保证这样做不会出问题，对象只是变成“有效但未指定”的状态，
    //    反正马上就被 pop_back 删掉了，所以书上的写法对标准库类型没毛病。
    //    但自己写的类，移动赋值如果没考虑“自己给自己”，就可能出错（练习 1 会遇到）。
    //    最稳妥的写法：要删的就是最后一个时，直接 pop_back
    // ------------------------------------------------------------
    std::println("\n[3] 删除最后一个元素");
    std::vector<std::string> s{"a", "b", "c"};
    quick_delete(s, 2); // 自我移动，然后 pop_back
    std::println("{}", s);

    // ------------------------------------------------------------
    // 4. std::move 并不移动任何东西
    //    它只是一个类型转换：把 v.back() 转成“右值引用”，意思是“这个对象我不要了，可以把它的内容搬走”。
    //    真正搬内容的是 operator=（选中了移动赋值，而不是复制赋值）。
    //    <utility> 里的 std::move(x) 是这个转换；<algorithm> 里还有一个三个参数的
    //    std::move(first, last, dest)，是把一段元素移动到另一个地方的算法 —— 名字一样，完全是两回事
    // ------------------------------------------------------------
    std::println("\n[4] std::move 只是转换");
    std::string from = "一个很长很长很长很长很长的字符串";
    std::string&& r = std::move(from); // 只是转换，什么都没发生
    std::println("std::move 之后：from = \"{}\"", from);
    std::string to = r; // r 有名字，当作左值用 → 这里是复制，from 还在
    std::println("复制之后：      from = \"{}\"", from);
    to = std::move(from); // 这里才真正移动
    std::println("移动之后：      from = \"{}\"，to = \"{}\"", from, to);
}
