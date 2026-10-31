// 第 3 章 3.6 保持 vector 元素的顺序（有序插入：lower_bound + insert）
// 编译：clang++ -std=c++23 -Wall -Wextra sorted_vector.cpp -o sorted_vector.exe
#include <algorithm>
#include <flat_set>
#include <functional>
#include <list>
#include <print>
#include <string>
#include <string_view>
#include <vector>

using Vstr = std::vector<std::string>;

void psorted(const Vstr& v)
{
    std::println("{:<9}{}", std::ranges::is_sorted(v) ? "sorted:" : "unsorted:", v);
}

// ------------------------------------------------------------
// 1. 书上的 insert_sorted：先用 lower_bound 找位置，再在那里 insert
//    lower_bound(v, x)：第一个“不小于 x”（>= x）的位置。在这里插入，前面都比 x 小，顺序就保持住了
//    lower_bound 是二分查找，只要 O(log n) 次比较；但 insert 要把后面的元素往后挪一格，是 O(n)
// ------------------------------------------------------------
void insert_sorted(Vstr& v, const std::string& s)
{
    const auto pos = std::ranges::lower_bound(v, s);
    v.insert(pos, s);
}

// ------------------------------------------------------------
// 4. 书上 There's more 的“通用版本”
// ------------------------------------------------------------
template <typename C, typename E>
void insert_sorted_any(C& c, const E& e)
{
    const auto pos = std::ranges::lower_bound(c, e);
    c.insert(pos, e);
}

struct Person
{
    std::string name;
    int age;
};

int main()
{
    std::println("[1] 书上的例子");
    Vstr v{"Miles", "Hendrix", "Beatles", "Zappa", "Shostakovich"};
    psorted(v);
    std::ranges::sort(v);
    psorted(v);
    insert_sorted(v, "Ella");
    insert_sorted(v, "Stones");
    psorted(v);

    // ------------------------------------------------------------
    // 2. lower_bound 和 upper_bound：遇到“相等的元素”时区别就出来了
    //    lower_bound：第一个 >= x 的位置 → 插在相等元素的“前面”
    //    upper_bound：第一个 >  x 的位置 → 插在相等元素的“后面”
    //    equal_range：一次拿到这两个位置，中间那段就是所有等于 x 的元素
    //    binary_search：只回答“有没有”
    // ------------------------------------------------------------
    std::println("\n[2] lower_bound / upper_bound");
    std::vector n{10, 20, 20, 20, 30};
    auto lo = std::ranges::lower_bound(n, 20);
    auto hi = std::ranges::upper_bound(n, 20);
    std::println("{}：lower_bound(20) 在下标 {}，upper_bound(20) 在下标 {}，20 一共 {} 个",
                 n, lo - n.begin(), hi - n.begin(), hi - lo);
    auto [first, last] = std::ranges::equal_range(n, 25); // 没有 25：两个位置重合，就是 25 该插入的地方
    std::println("equal_range(25)：[{}, {})，binary_search(25) = {}",
                 first - n.begin(), last - n.begin(), std::ranges::binary_search(n, 25));

    // ------------------------------------------------------------
    // 3. 按某个成员排序：projection（1.9 节）。sort 和 lower_bound 必须用同一种“顺序”
    //    按年龄排好序，插入时用 upper_bound：同岁的人保持“先来的在前”
    // ------------------------------------------------------------
    std::println("\n[3] 按成员排序");
    std::vector<Person> people{{"Tom", 30}, {"Amy", 20}, {"Bob", 30}};
    std::ranges::stable_sort(people, {}, &Person::age); // {} 表示用默认的 std::ranges::less
    Person eve{"Eve", 30};
    people.insert(std::ranges::upper_bound(people, eve.age, {}, &Person::age), eve);
    for (const auto& p : people)
    {
        std::print("{}({}) ", p.name, p.age);
    }
    std::println("   ← Eve 排在同岁的 Tom、Bob 后面");

    // 降序也一样：sort 用 greater，lower_bound 也要用 greater
    std::vector d{50, 40, 20, 10};
    d.insert(std::ranges::lower_bound(d, 30, std::ranges::greater{}), 30);
    std::println("降序插入 30：{}", d);

    // ------------------------------------------------------------
    // 4. 书上说通用版本“将用于其他容器，如 set、deque 和 list”——能编译，但用法不对：
    //    list：迭代器不能跳着走，lower_bound 只能一步一步数过去，比较次数 O(log n)，但走路要 O(n)
    //          list 本来就该自己从头找，或者干脆别用 list
    //    set ：set 本身就是有序的，直接 s.insert(x) 就会放到正确的位置（O(log n)）；
    //          用 ranges::lower_bound 反而是 O(n) 地一步步走。set 要查找请用成员函数 s.lower_bound(x)
    //    规律：容器有同名成员函数时（set::lower_bound、map::find、list::sort……），优先用成员函数
    // ------------------------------------------------------------
    std::println("\n[4] 通用版本用在 list 上");
    std::list l{1, 3, 5};
    insert_sorted_any(l, 4);
    std::println("list：{}", l);

    // ------------------------------------------------------------
    // 5. C++23 std::flat_set：把“有序 vector”直接做成了容器（书上没有）
    //    内部就是一个排好序的 vector，insert 时自动找位置、自动去重
    //    和 set 比：内存连续，遍历和查找更快、更省内存；但插入要挪元素，O(n)
    //    适合“建好之后主要用来查”的数据
    // ------------------------------------------------------------
    std::println("\n[5] std::flat_set");
    std::flat_set<std::string> fs{"Miles", "Hendrix", "Beatles"};
    fs.insert("Ella");
    fs.insert("Miles"); // 已经有了，插不进去
    std::println("{}，contains(\"Ella\") = {}", fs, fs.contains("Ella"));
}
