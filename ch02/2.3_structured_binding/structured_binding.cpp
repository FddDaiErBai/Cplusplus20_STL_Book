// 第 2 章 2.3 结构化绑定（structured binding）
// 编译：clang++ -std=c++23 -Wall -Wextra structured_binding.cpp -o structured_binding.exe
// 再用 -std=c++26 编译一次，可以看到第 7 部分的 C++26 新特性
#include <array>
#include <cstddef>
#include <map>
#include <print>
#include <string>
#include <tuple>
#include <utility>

// 第 6 部分用：成员是 private 的类
class Point3
{
public:
    constexpr Point3(int x, int y, int z) : x_{x}, y_{y}, z_{z}
    {
    }

    // 结构化绑定会调用成员函数 get<0>()、get<1>()、get<2>()
    template <std::size_t I>
    constexpr int get() const
    {
        if constexpr (I == 0) // if constexpr 是 2.6 节的内容：编译时选分支
        {
            return x_;
        }
        else if constexpr (I == 1)
        {
            return y_;
        }
        else
        {
            return z_;
        }
    }

private:
    int x_;
    int y_;
    int z_;
};

// 告诉标准库：Point3 有 3 个元素，每个元素都是 int
template <>
struct std::tuple_size<Point3> : std::integral_constant<std::size_t, 3>
{
};

template <std::size_t I>
struct std::tuple_element<I, Point3>
{
    using type = int;
};

struct DivResult
{
    long quo;
    long rem;
};

DivResult int_div(long num, long denom)
{
    return {num / denom, num % denom};
}

int main()
{
    // ------------------------------------------------------------
    // 1. 基本用法：pair、tuple、C 数组、std::array、struct 都能拆
    //    书上第一个例子 things_pair<int,int> {47, 9}; auto [this, that] 有两个错：
    //    类型写错了位置，而且 this 是关键字，不能当名字
    // ------------------------------------------------------------
    std::println("[1] 基本用法");
    std::pair<int, int> things{47, 9};
    auto [first, second] = things;
    std::println("pair:   {} {}", first, second);

    std::tuple<int, double, std::string> tup{1, 2.7, "three"};
    auto [ti, td, ts] = tup;
    std::println("tuple:  {} {} {}", ti, td, ts);

    int carray[]{1, 2, 3};
    auto [c1, c2, c3] = carray;
    std::println("C 数组: {} {} {}", c1, c2, c3);

    auto [quo, rem] = int_div(47, 5); // struct：按成员声明的顺序拆
    std::println("struct: quotient {}, remainder {}", quo, rem);

    // ------------------------------------------------------------
    // 2. 真相：编译器先创建一个看不见的对象，[ ] 里的名字是这个对象成员的别名
    //    auto / auto& / const auto& 修饰的是那个看不见的对象，不是每个名字
    //    （书上说“为每个变量分配相应的类型”，不准确：这些名字不是独立的变量）
    // ------------------------------------------------------------
    std::println("\n[2] auto 和 auto&");
    std::array<int, 3> nums{1, 2, 3};

    auto [x, y, z] = nums; // 看不见的对象 = nums 的副本
    x = 100;
    std::println("auto  改 x 之后 nums = {}", nums); // nums 不变

    auto& [rx, ry, rz] = nums; // 看不见的对象 = nums 的引用
    rx = 100;
    std::println("auto& 改 rx 之后 nums = {}", nums); // nums[0] 变了

    const auto& [cx, cy, cz] = nums; // 只读引用：不复制，也不能改
    // cx = 1; // 编译失败
    std::println("const auto&: {} {} {}", cx, cy, cz);

    // 绑定到函数返回的临时对象：auto 和 const auto& 都可以，auto& 不行
    // auto& [q2, r2] = int_div(10, 3); // 编译失败：非 const 引用不能绑定临时对象

    // ------------------------------------------------------------
    // 3. 最常用的场景：遍历 map
    // ------------------------------------------------------------
    std::println("\n[3] map");
    std::map<std::string, int> stock{{"apple", 3}, {"banana", 5}};
    for (const auto& [name, count] : stock) // 用 const auto&，不然每次循环都复制一个 pair
    {
        std::println("{}: {}", name, count);
    }
    for (auto& [name, count] : stock) // 要改值就用 auto&（key 是 const 的，改不了）
    {
        count *= 10;
    }
    std::println("每个乘 10 之后: {}", stock);

    // insert 返回 pair<迭代器, 是否插入成功>，用绑定拆开比写 .first.second 清楚得多
    auto [it, inserted] = stock.insert({"apple", 99});
    std::println("插入 apple: inserted = {}, 现有值 = {}", inserted, it->second);

    // ------------------------------------------------------------
    // 4. C++20：lambda 可以捕获绑定的名字（C++17 不行）
    // ------------------------------------------------------------
    auto [w, h] = std::pair{3, 4};
    auto area = [w, h]
    {
        return w * h;
    };
    std::println("\n[4] lambda 捕获绑定: area = {}", area());

    // ------------------------------------------------------------
    // 5. 不能写具体类型，个数必须正好对上
    // ------------------------------------------------------------
    // int [a, b] = things;        // 编译失败：只能写 auto
    // auto [a, b, c] = things;    // 编译失败：pair 只有 2 个元素

    // ------------------------------------------------------------
    // 6. 自定义类型：成员是 private 时不能直接拆，
    //    需要提供 tuple_size、tuple_element 和 get<I>()（书上没讲，pair 和 tuple 就是这样支持绑定的）
    // ------------------------------------------------------------
    auto [px, py, pz] = Point3{1, 2, 3};
    std::println("\n[6] Point3: {} {} {}", px, py, pz);

    // ------------------------------------------------------------
    // 7. C++26 新特性（要用 -std=c++26 编译）
    // ------------------------------------------------------------
#if __cplusplus > 202302L
    // _ 占位符：不需要的元素用 _ 跳过，可以写好几个
    auto [_, middle, _] = std::tuple{1, 2, 3};
    std::println("\n[7] C++26 占位符 _: middle = {}", middle);

    // 绑定包：...elems 一次拿到所有元素（只能在模板里用，泛型 lambda 也算模板）
    auto sum_all = [](const auto& t)
    {
        auto [... elems] = t;
        return (elems + ...);
    };
    std::println("    绑定包求和: {}", sum_all(std::tuple{1, 2, 3, 4}));
#endif
}
