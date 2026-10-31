// 第 2 章 2.4 补充：范围 for 里的临时对象活多久（书上没讲）
// 这个文件只用 printf，可以用 C++20 和 C++23 分别编译，对比结果（练习的思考题 (2)）：
//   clang++ -std=c++20 -Wall -Wextra range_for_lifetime.cpp -o lifetime20.exe
//   clang++ -std=c++23 -Wall -Wextra range_for_lifetime.cpp -o lifetime23.exe
#include <cstdio>
#include <vector>

struct Box
{
    std::vector<int> data{1, 2, 3};

    ~Box()
    {
        data.assign(3, -1); // 销毁前把数据改成 -1，读到 -1 就说明读的是已经销毁的对象
        std::puts("  ~Box()");
    }

    const std::vector<int>& items() const
    {
        return data;
    }
};

Box make_box()
{
    return {};
}

int main()
{
    std::puts("写法 1：for (int x : make_box().items())");
    for (int x : make_box().items())
    {
        std::printf("  %d\n", x);
    }

    std::puts("写法 2：for (auto box = make_box(); int x : box.items())");
    for (auto box = make_box(); int x : box.items())
    {
        std::printf("  %d\n", x);
    }
}
