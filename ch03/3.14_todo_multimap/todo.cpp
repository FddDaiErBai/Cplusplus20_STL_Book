// 第 3 章 3.14 用 multimap 做待办事项列表
// 编译：clang++ -std=c++23 -Wall -Wextra todo.cpp -o todo.exe
//
// ============================================================
// 要做的事：一个待办清单，每件事有一个优先级（数字越大越要紧），按优先级从高到低列出来：
//     2: do homework
//     1: wash dishes
//     0: watch teevee
//     0: read comics
//   优先级会重复（好几件事都是 0），所以不能用 map —— map 的键不能重复，第二个 0 会插入失败（3.7 节）
//
// multimap（多重 map）：和 map 几乎一样，也是按键排好序的树，区别只有一个 —— 键可以重复
//   内部可以想成一个按键排好序的列表，键相同的元素挨在一起：
//       { 0: watch teevee,  0: read comics,  1: wash dishes,  2: do homework }
//         └──── 键都是 0 ────┘
//   键相同的元素，按插入的先后排列：先插入的在前（C++11 起标准保证的）
//
//   和 map 的区别（[3] 里一个个演示）：
//                         map                         multimap
//     键能重复吗          不能                          能
//     insert 返回         pair<迭代器, bool>            迭代器（总是成功，不需要 bool）
//     m[k]、m.at(k)       有                            没有（k 对应好几个值，返回哪一个？）
//     find(k)             那一个                        键为 k 的“某一个”（不一定是第一个）
//     count(k)            0 或 1                        有几个就是几
//     erase(k)            删掉那一个                    键为 k 的全部删掉
//     equal_range(k)      很少用                        最常用：键为 k 的所有元素的范围 [first, second)
// ============================================================
#include <cstddef>
#include <functional>
#include <map>
#include <print>
#include <queue>
#include <ranges>
#include <string>
#include <utility>
#include <vector>

using todomap = std::multimap<int, std::string>;

// 书上的打印函数：用反向迭代器从后往前打印
//   rbegin() 指向最后一个元素，++ 是往前走，rend() 是“第一个元素的前面”（和 begin/end 正好反过来）
//   书上参数写成 todomap&（非 const），只是打印，应该用 const todomap&
void rprint(const todomap& todo)
{
    for (auto it = todo.rbegin(); it != todo.rend(); ++it)
    {
        std::println("  {}: {}", it->first, it->second);
    }
}

int main()
{
    // ------------------------------------------------------------
    // 1. 书上的例子
    //    注意两个优先级为 0 的任务：初始化时 "watch teevee" 写在 "read comics" 前面（先插入），
    //    所以在 multimap 里，从前往后是 watch teevee、read comics。
    //    rprint 从后往前打印，这两个的顺序也跟着反过来了：先打印后插入的 read comics（思考题 (1)）
    //    书上的输出正是这样，但书上没提。对待办清单来说，同样要紧的事，一般希望“先加进来的先做”
    // ------------------------------------------------------------
    std::println("[1] 书上的例子：反向迭代器");
    todomap todo{
        {1, "wash dishes"},
        {0, "watch teevee"},
        {2, "do homework"},
        {0, "read comics"},
    };
    std::println("从前往后（键从小到大）：");
    for (const auto& [p, task] : todo)
    {
        std::println("  {}: {}", p, task);
    }
    std::println("rprint 从后往前：");
    rprint(todo);

    // ------------------------------------------------------------
    // 2. 让 multimap 本身就从大到小排：第三个模板参数传 std::greater<>（3.12 思考题 (3)）
    //    书上说“不可能改变 multimap 的排序顺序” —— 不对，创建的时候就可以指定
    //    这样正着遍历就是优先级从高到低，而同一个优先级里仍然是“先插入的在前”—— 正是待办清单想要的
    // ------------------------------------------------------------
    std::println("\n[2] std::greater<>：正着遍历就是从高到低");
    std::multimap<int, std::string, std::greater<>> todo2{
        {1, "wash dishes"},
        {0, "watch teevee"},
        {2, "do homework"},
        {0, "read comics"},
    };
    for (const auto& [p, task] : todo2)
    {
        std::println("  {}: {}", p, task);
    }

    // ------------------------------------------------------------
    // 3. multimap 的常用操作（开头那张表）
    // ------------------------------------------------------------
    std::println("\n[3] 常用操作");
    std::multimap<int, std::string, std::greater<>> t{
        {2, "homework"}, {1, "dishes"}, {1, "laundry"}, {0, "comics"}, {1, "trash"}
    };

    // insert 总是成功，返回指向新元素的迭代器（map 的 insert 返回的是 pair<迭代器, bool>）
    auto it = t.insert({1, "groceries"});
    std::println("insert 返回的迭代器指向：{}: {}（排在其他优先级 1 的后面）", it->first, it->second);

    // count：有几个
    std::println("优先级 1 的任务有 {} 个", t.count(1));

    // equal_range(k)：返回一对迭代器 [first, second)，正好框住所有键为 k 的元素
    //   first 指向第一个键为 k 的元素，second 指向最后一个的“下一个”（和 begin/end 一样，左闭右开）
    //   可以用结构化绑定（2.3 节）拆开，再用 std::ranges::subrange 包装成一个能 for 的范围
    auto [first, last] = t.equal_range(1);
    std::print("equal_range(1)：");
    for (const auto& [p, task] : std::ranges::subrange(first, last))
    {
        std::print("{}  ", task);
    }
    std::println("");

    // m[k]、m.at(k)：multimap 没有。下面这行编译不过（思考题 (2)）
    // t[1] = "nap";

    // erase(k)：键为 k 的全部删掉，返回删了几个；erase(迭代器)：只删那一个
    std::size_t n = t.erase(1);
    std::println("erase(1) 删掉了 {} 个，剩下：{}", n, t);
    t.erase(t.begin());
    std::println("erase(begin()) 删掉第一个，剩下：{}", t);

    // ------------------------------------------------------------
    // 4. 按“任务名”找：multimap 只能按键（优先级）快速查找。按值找，只能一个一个看
    //    std::ranges::find_if(t, 条件)：从头找第一个满足条件的元素，返回迭代器，找不到返回 end()
    //    找到之后：删掉它用 erase(it)；改它的优先级（也就是改键）用 3.8 节的 extract
    // ------------------------------------------------------------
    std::println("\n[4] 按任务名找、改优先级");
    std::multimap<int, std::string, std::greater<>> t4{
        {2, "homework"}, {1, "dishes"}, {1, "laundry"}, {0, "comics"},
    };
    auto pos = std::ranges::find_if(t4, [](const auto& kv) { return kv.second == "laundry"; });
    if (pos != t4.end())
    {
        auto node = t4.extract(pos); // 摘下来，键就能改了
        node.key() = 3;              // laundry 升到 3
        t4.insert(std::move(node));  // 挂回去，自动排到正确的位置
    }
    std::println("laundry 改成 3 之后：{}", t4);

    // ------------------------------------------------------------
    // 5. 书上提到的 priority_queue（优先队列，<queue>）
    //    专门用来“每次取出最大的那个”：push 放入，top 看最大的，pop 拿掉最大的
    //    书上说它的缺点是没有迭代器：不能遍历，只能看到最上面那一个。想把全部打印出来，只能一个个 pop。
    //    下面先复制一份再 pop，原来那个就不受影响（思考题 (3)）
    //    pair 比较时先比 first，一样再比 second —— 所以优先级一样时，会按任务名的字母顺序“从大到小”出来，
    //    跟谁先放进去无关：下面 read comics 先放进去，却是 watch teevee 先出来（w 比 r 大）
    // ------------------------------------------------------------
    std::println("\n[5] priority_queue");
    std::priority_queue<std::pair<int, std::string>> pq;
    pq.push({1, "wash dishes"});
    pq.push({0, "read comics"}); // read comics 先放进去
    pq.push({2, "do homework"});
    pq.push({0, "watch teevee"});
    std::println("top() = {}: {}，size() = {}", pq.top().first, pq.top().second, pq.size());

    auto copy = pq;
    while (!copy.empty())
    {
        std::println("  {}: {}", copy.top().first, copy.top().second);
        copy.pop();
    }
    std::println("pop 完副本之后，原来的 pq 还有 {} 个", pq.size());
}
