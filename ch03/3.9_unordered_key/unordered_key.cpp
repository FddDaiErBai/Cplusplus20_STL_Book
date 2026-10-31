// 第 3 章 3.9 用自定义类型作为 unordered_map 的键（相等比较 + 哈希函数）
// 编译：clang++ -std=c++23 -Wall -Wextra unordered_key.cpp -o unordered_key.exe
#include <cstddef>
#include <cstdint>
#include <functional>
#include <algorithm>
#include <map>
#include <print>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

struct Coord
{
    int x{};
    int y{};

    // 书上手写了 lhs.x == rhs.x && lhs.y == rhs.y。C++20 直接 = default（1.5 节），逐个成员比较
    bool operator==(const Coord&) const = default;
};

// ------------------------------------------------------------
// 书上的哈希：x + y。能用，但很差（第 [3] 部分）
// 书上说特化“必须在 std 命名空间中”，要写 namespace std { template<> struct hash<Coord> {...}; }
// C++17 起可以直接在外面写 std::hash<Coord>，不用打开 namespace std
// ------------------------------------------------------------
template <>
struct std::hash<Coord>
{
    std::size_t operator()(const Coord& c) const
    {
        return static_cast<std::size_t>(c.x) + static_cast<std::size_t>(c.y);
    }
};

using Coordmap = std::unordered_map<Coord, int>;

void print_Coordmap(const Coordmap& m)
{
    for (const auto& [key, value] : m)
    {
        std::print("{{ ({}, {}): {} }} ", key.x, key.y, value);
    }
    std::println("");
}

// ------------------------------------------------------------
// 3. 好一点的哈希：把 x 放在高 32 位、y 放在低 32 位，拼成一个 64 位的数
//    不同的 (x, y) 一定得到不同的结果（int 是 32 位，size_t 是 64 位，正好放得下）
//    先转成 uint32_t：负数直接转成 64 位会把高 32 位全填成 1，就和 x 混在一起了
// ------------------------------------------------------------
struct GoodHash
{
    std::size_t operator()(const Coord& c) const
    {
        const auto hx = static_cast<std::size_t>(static_cast<std::uint32_t>(c.x));
        const auto hy = static_cast<std::size_t>(static_cast<std::uint32_t>(c.y));
        return hx << 32 | hy;
    }
};

// 数一数 == 被调用了多少次：查找时，同一个桶里的元素要一个一个用 == 比较
struct CountingEqual
{
    static inline long long calls = 0;

    bool operator()(const Coord& a, const Coord& b) const
    {
        ++calls;
        return a == b;
    }
};

// 用 n×n 的网格填满一个 unordered_set，再把每个点查找一次，打印统计信息
template <typename Hash>
void grid_test(std::string_view name, int n)
{
    std::unordered_set<Coord, Hash, CountingEqual> s;
    for (int x = 0; x < n; ++x)
    {
        for (int y = 0; y < n; ++y)
        {
            s.insert({x, y});
        }
    }
    std::size_t max_bucket = 0;
    std::size_t used_buckets = 0;
    for (std::size_t b = 0; b < s.bucket_count(); ++b)
    {
        max_bucket = std::max(max_bucket, s.bucket_size(b));
        if (s.bucket_size(b) > 0)
        {
            ++used_buckets;
        }
    }
    CountingEqual::calls = 0;
    for (int x = 0; x < n; ++x)
    {
        for (int y = 0; y < n; ++y)
        {
            (void)s.contains({x, y});
        }
    }
    std::println("{:<8}{} 个元素，{} 个桶，用到 {} 个，最挤的桶有 {} 个，查找 {} 次共调用 == {} 次",
                 name, s.size(), s.bucket_count(), used_buckets, max_bucket, s.size(), CountingEqual::calls);
}

// 4. 把哈希作为模板参数传进去：不用特化 std::hash
//    C++20 起，没有捕获的 lambda 可以默认构造，所以 decltype(lambda) 也能当哈希类型
constexpr auto lambda_hash = [](const Coord& c)
{
    return std::hash<int>{}(c.x) * 31 + std::hash<int>{}(c.y);
};

int main()
{
    // ------------------------------------------------------------
    // 1. 书上的例子
    //    unordered_map 找一个键分两步：
    //      ① 用哈希函数把键变成一个数，决定它在哪个“桶”（bucket）里 —— 一下子就跳到那个桶
    //      ② 在这个桶里，用 == 一个一个比较，找到真正相等的那个
    //    所以键类型需要两样东西：哈希函数 + 相等比较。少一个都编译不过
    // ------------------------------------------------------------
    std::println("[1] 书上的例子");
    Coordmap m{{{0, 0}, 1}, {{0, 1}, 2}, {{2, 1}, 3}};
    print_Coordmap(m);
    Coord k{0, 1};
    std::println("m.at({{0, 1}}) = {}", m.at(k));

    // ------------------------------------------------------------
    // 2. 遍历顺序没有保证
    //    书上的输出是 (2,1) (0,1) (0,0)。这个顺序取决于哈希函数、桶的个数、标准库的实现 ——
    //    换个编译器、换个版本就可能不一样，程序不能依赖它。
    //    比如桶的个数：同样 3 个元素，clang（libc++）是 5 个桶，g++（libstdc++）是 13 个桶
    // ------------------------------------------------------------
    std::println("\n[2] 桶");
    std::println("bucket_count = {}，每个键在哪个桶：", m.bucket_count());
    for (const auto& [key, value] : m)
    {
        std::println("  ({}, {}) → 哈希值 {} → 第 {} 号桶", key.x, key.y, std::hash<Coord>{}(key), m.bucket(key));
    }

    // ------------------------------------------------------------
    // 3. 书上的 x + y 是个很差的哈希：(0,1) (1,0) (2,-1) …… 所有 x + y 相同的点哈希值都一样
    //    100×100 的网格一万个点，x + y 只有 0~198 这 199 种结果 → 最多只能用到 199 个桶，
    //    平均每个桶挤 50 个，每次查找都要在桶里比较几十次 —— 哈希表退化成了链表
    //    结果仍然是对的，只是慢（思考题 (1)）。桶的个数两个标准库不一样，但 == 的调用次数一样
    // ------------------------------------------------------------
    std::println("\n[3] 哈希函数的好坏");
    grid_test<std::hash<Coord>>("x + y", 100);
    grid_test<GoodHash>("拼接", 100);

    // ------------------------------------------------------------
    // 4. 不特化 std::hash，而是把哈希类型作为第 3 个模板参数传进去（书上最后提了一句）
    //    书上说“可以向模板形参传递一个函数” —— 不准确：传的是一个“类型”（有 operator() 的类），不是函数
    //    特化 std::hash：所有地方默认都能用，适合“这个类型就该这么哈希”
    //    模板参数：只影响这一个容器，适合同一个类型在不同场合用不同的哈希
    // ------------------------------------------------------------
    std::println("\n[4] 哈希作为模板参数");
    std::unordered_map<Coord, int, GoodHash> m2{{{0, 0}, 1}, {{0, 1}, 2}};
    std::unordered_set<Coord, decltype(lambda_hash)> s2{{1, 2}, {2, 1}, {1, 2}};
    std::println("m2.size() = {}，s2.size() = {}（{{1, 2}} 重复了）", m2.size(), s2.size());

    // ------------------------------------------------------------
    // 5. 书上说“(0,1) 既不大于也不小于 (1,0)，所以不能用有序 map” —— 不对
    //    map 需要的只是“某种一致的排序规则”，不需要有几何意义。
    //    比如“先比 x，x 相同再比 y”（字典序），C++20 一行 = default 的 <=> 就是这个规则（1.5 节）
    //    选 map 还是 unordered_map，看的是要不要“有序遍历/范围查找”，以及性能，而不是键“能不能比大小”
    // ------------------------------------------------------------
    std::println("\n[5] 用 map 也可以");
    struct OrderedCoord
    {
        int x{};
        int y{};

        auto operator<=>(const OrderedCoord&) const = default;
    };
    std::map<OrderedCoord, int> om{{{2, 1}, 3}, {{0, 1}, 2}, {{0, 0}, 1}};
    for (const auto& [key, value] : om)
    {
        std::print("({}, {}): {}  ", key.x, key.y, value);
    }
    std::println("   ← 有序：先按 x，再按 y");
}
