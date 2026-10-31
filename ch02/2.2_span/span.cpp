// 第 2 章 2.2 std::span —— 连续内存的“视图”
// 编译：clang++ -std=c++23 -Wall -Wextra span.cpp -o span.exe
#include <array>
#include <cstddef>
#include <print>
#include <span>
#include <vector>

// ------------------------------------------------------------
// 1. 问题：C 数组传给函数会退化成指针，长度丢了，只能再传一个参数
// ------------------------------------------------------------
int old_sum(const int* p, std::size_t n) // n 传错了编译器也不知道
{
    int total = 0;
    for (std::size_t i = 0; i < n; ++i)
    {
        total += p[i];
    }
    return total;
}

// ------------------------------------------------------------
// 2. 用 span：指针 + 长度打包在一起
//    只读就用 span<const int>：C 数组、std::array、vector、const 的都能传
//    不用写成模板（书上写成模板，调用时就得写 pspan<int>(...)，见练习思考题）
// ------------------------------------------------------------
int sum(std::span<const int> s)
{
    int total = 0;
    for (int x : s) // span 是范围，可以直接 for
    {
        total += x;
    }
    return total;
}

// 要修改元素就用 span<int>
void double_all(std::span<int> s)
{
    for (int& x : s)
    {
        x *= 2;
    }
}

int main()
{
    int carray[]{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    std::array<int, 3> arr{1, 2, 3};
    std::vector<int> vec{10, 20, 30, 40};

    std::println("[1/2] 传给 span");
    std::println("old_sum(carray, 10) = {}", old_sum(carray, 10));
    std::println("sum(carray) = {}", sum(carray)); // 长度自动带上
    std::println("sum(arr)    = {}", sum(arr));
    std::println("sum(vec)    = {}", sum(vec));
    std::println("sum({{}})     = {}", sum({})); // 空 span

    double_all(vec); // span 不复制数据，改的就是 vec 本身
    std::println("double_all 之后 vec = {}", vec);

    // ------------------------------------------------------------
    // 3. 子 span：first / last / subspan，都不复制数据
    // ------------------------------------------------------------
    std::span<int> s{carray};
    std::println("\n[3] 子 span");
    std::println("size = {}, size_bytes = {}", s.size(), s.size_bytes());
    std::println("first(3)      = {}", s.first(3));
    std::println("last(3)       = {}", s.last(3));
    std::println("subspan(2, 4) = {}", s.subspan(2, 4)); // 从下标 2 开始取 4 个
    std::println("subspan(7)    = {}", s.subspan(7));    // 从下标 7 开始取到最后
    std::println("sum(后一半)   = {}", sum(s.last(5)));

    // 注意：书上的表格说 rbegin 是“指向第一个元素的反向迭代器”，说反了：
    //       rbegin 指向最后一个元素，rend 指向第一个元素的前面
    std::println("*s.rbegin()   = {}", *s.rbegin());

    // ------------------------------------------------------------
    // 4. 固定长度（Extent）：长度写在类型里，编译时就知道
    //    固定长度的 span 只存一个指针，动态长度的还要存长度
    // ------------------------------------------------------------
    std::println("\n[4] 固定长度");
    std::span<int, 10> fixed{carray};
    std::span dyn_from_vec{vec}; // 类模板参数推导：vector 推导出动态长度
    std::span fixed_from_c{carray}; // C 数组推导出固定长度 span<int, 10>
    std::println("sizeof(span<int>)     = {}", sizeof(std::span<int>));
    std::println("sizeof(span<int, 10>) = {}", sizeof(fixed));
    std::println("fixed_from_c.extent   = {}", fixed_from_c.extent);
    std::println("dyn_from_vec.extent == dynamic_extent: {}", dyn_from_vec.extent == std::dynamic_extent);
    auto head = fixed.first<3>(); // 尖括号版本：返回 span<int, 3>，长度也是编译时确定的
    std::println("head.extent           = {}", head.extent);

    // ------------------------------------------------------------
    // 5. const span<int> 仍然能改元素（和 int* const 一样，const 的是 span 本身，不是元素）
    //    想让元素只读，要写 span<const int>
    // ------------------------------------------------------------
    const std::span<int> cs{arr};
    cs[0] = 100; // 能编译
    std::println("\n[5] const span<int> 改了元素：arr = {}", arr);

    // ------------------------------------------------------------
    // 6. 按字节查看：as_bytes 把任何 span 变成 span<const std::byte>
    // ------------------------------------------------------------
    int one = 0x01020304;
    std::print("\n[6] 0x01020304 的字节：");
    for (std::byte b : std::as_bytes(std::span{&one, 1}))
    {
        std::print("{:02x} ", static_cast<unsigned>(b));
    }
    std::println("（低位字节在前：x86 是小端序）");

    // ------------------------------------------------------------
    // 7. 越界：span 的 [] 标准里不检查边界，越界是未定义行为
    //    但现在的编译器可以打开检查（书上没讲）：
    //      g++ 15 起，不开优化（不加 -O）时默认就检查，越界会直接报错退出
    //      clang（libc++）要加 -D_LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_FAST
    //    C++26 新增 s.at(i)，越界抛出异常（目前 g++ 有，clang 还没有）
    // ------------------------------------------------------------
    // std::println("{}", s[10]); // 取消注释，用 g++ 和 clang++ 分别运行，对比结果（思考题 (3)）

    // ------------------------------------------------------------
    // 8. span 不拥有数据，原数据没了 span 就悬空了
    // ------------------------------------------------------------
    // std::span<const int> bad = std::vector{1, 2, 3}; // 临时 vector 在这一行结束就销毁了
    // clang 会警告 -Wdangling-gsl，g++ 不会提示
}
