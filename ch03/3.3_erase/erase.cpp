// 第 3 章 3.3 使用擦除函数从容器中删除元素（C++20 std::erase / std::erase_if）
// 编译：clang++ -std=c++23 -Wall -Wextra erase.cpp -o erase.exe
#include <algorithm>
#include <format>
#include <list>
#include <map>
#include <print>
#include <ranges>
#include <set>
#include <string>
#include <string_view>
#include <vector>

// 打印 size 和所有元素（书上的 printc；C++23 可以直接 println("{}", v) 打印整个容器）
void printc(std::string_view title, const auto& c)
{
    std::println("{:<28} size {}: {}", title, c.size(), c);
}

int main()
{
    // ------------------------------------------------------------
    // 1. C++20 以前：erase-remove 惯用法
    //    std::remove 不会真的删除，它只是把“要留下的元素”往前挪，容器的 size 不变，
    //    返回“留下的元素”后面的位置。然后再调用容器自己的 erase 把后面那段删掉
    // ------------------------------------------------------------
    std::println("[1] erase-remove");
    std::vector v{5, 1, 5, 2};
    auto it = std::remove(v.begin(), v.end(), 5);
    printc("remove 之后（还没 erase）", v);
    std::println("留下了 {} 个，后面 {} 个是“剩下的”", it - v.begin(), v.end() - it);
    v.erase(it, v.end());
    printc("erase 之后", v);

    // 书上说后面那段是“未定义的值”，不准确：标准说的是“有效但未指定”（valid but unspecified）
    // 意思是：这些对象还是好的（可以赋值、可以销毁），只是不保证是什么值。
    // int 一般就是原来的值没动；string 的内容被“移走”了，一般会变成空字符串
    std::vector<std::string> words{"apple", "x", "banana", "x"};
    auto it2 = std::remove(words.begin(), words.end(), "x");
    std::println("string 版本 remove 之后：{}，留下 {} 个", words, it2 - words.begin());

    // ------------------------------------------------------------
    // 2. C++20：一步完成，而且返回删掉了几个（书上没提返回值）
    // ------------------------------------------------------------
    std::println("\n[2] std::erase / std::erase_if");
    std::vector v2{0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    auto n1 = std::erase(v2, 5);
    printc(std::format("erase(v, 5) 删了 {} 个", n1), v2);
    auto n2 = std::erase_if(v2, [](int x) { return x % 2 == 0; });
    printc(std::format("erase_if(偶数) 删了 {} 个", n2), v2);

    // string 也有（书上漏了）：定义在 <string> 里
    std::string s{"h e l l o"};
    std::erase(s, ' ');
    std::println("去掉空格：{}", s);

    // ------------------------------------------------------------
    // 3. list 的版本不是 erase-remove（书上说“只是 erase-remove 的包装器”，对 list 不对）
    //    链表删除元素只需要改指针，不用挪动其他元素，所以 std::erase(list) 调用的是成员函数 list::remove
    //    好处：没被删除的元素一个都没动过，指向它们的迭代器、引用都还有效
    // ------------------------------------------------------------
    std::println("\n[3] list");
    std::list l{0, 1, 2, 3, 4, 5};
    auto three = std::next(l.begin(), 3); // 指向 3
    std::erase_if(l, [](int x) { return x % 2 == 0; });
    printc("erase_if(偶数)", l);
    std::println("之前保存的迭代器还指向：{}", *three); // 还是 3；换成 vector 的话这个迭代器已经指错位置了

    // ------------------------------------------------------------
    // 4. 关联容器只有 erase_if，没有 std::erase（原因见练习的思考题）
    //    按键删除用成员函数 m.erase(key)
    // ------------------------------------------------------------
    std::println("\n[4] map / set");
    std::map<int, std::string> m{{1, "uno"}, {2, "dos"}, {3, "tres"}, {4, "cuatro"}, {5, "cinco"}};
    std::erase_if(m, [](const auto& p) { return p.first % 2 == 0; }); // p 是 pair<const int, string>
    std::println("erase_if(键是偶数)：{}", m);
    m.erase(3);
    std::println("m.erase(3)：        {}", m);
    std::set st{3, 1, 4, 1, 5, 9, 2, 6};
    std::erase_if(st, [](int x) { return x > 4; });
    std::println("set erase_if(>4)：  {}", st);

    // ------------------------------------------------------------
    // 5. 坑：std::erase(v, v[0])  —— “删掉所有和第一个元素相等的”
    //    erase 的第二个参数是 const 引用，引用的就是 v[0] 这个位置。
    //    挪动元素的时候 v[0] 被覆盖了，要比较的“值”也跟着变了，结果出错
    // ------------------------------------------------------------
    std::println("\n[5] erase(v, v[0]) 的坑");
    std::vector v5{1, 2, 1, 3};
    std::erase(v5, v5[0]);
    printc("erase(v, v[0])（错）", v5); // 期望 {2, 3}，实际 {2, 1, 3}
    std::vector v6{1, 2, 1, 3};
    std::erase(v6, auto(v6[0])); // C++23 auto(x)：先复制一份，再传进去；C++20 可以写 int first = v6[0];
    printc("erase(v, auto(v[0]))（对）", v6);

    // ------------------------------------------------------------
    // 6. 自己写循环删除时的正确写法：it = c.erase(it)
    //    erase 之后 it 失效了，不能再 ++it；erase 会返回“被删元素的下一个位置”，用它接着走。
    //    std::erase_if 对 map / set 就是这样实现的
    // ------------------------------------------------------------
    std::println("\n[6] 手写循环");
    std::map<std::string, int> stock{{"apple", 3}, {"banana", 0}, {"cherry", 0}, {"durian", 7}};
    for (auto i = stock.begin(); i != stock.end();)
    {
        if (i->second == 0)
        {
            i = stock.erase(i); // 删掉了：i 直接跳到下一个
        }
        else
        {
            ++i;                // 没删：正常往下走
        }
    }
    std::println("去掉库存为 0 的：{}", stock);

    // ------------------------------------------------------------
    // 7. ranges 版本的 remove 返回一段 subrange（“剩下的”那段），可以直接解包
    // ------------------------------------------------------------
    std::println("\n[7] ranges::remove");
    std::vector v7{1, 0, 2, 0, 3};
    auto [first, last] = std::ranges::remove(v7, 0);
    v7.erase(first, last);
    printc("ranges::remove + erase", v7);
}
