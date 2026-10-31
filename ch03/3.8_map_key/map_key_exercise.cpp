// 第 3 章 3.8 高效地修改 map 元素的键 —— 练习
// 编译：clang++ -std=c++23 -Wall -Wextra map_key_exercise.cpp -o map_key_exercise.exe
// 和 3.7 一样用运行时检查：补完 TODO，编译运行，所有行都显示“通过”就说明做对了。
#include <map>
#include <print>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

void check(bool ok, std::string_view name)
{
    std::println("{} {}", ok ? "通过" : "失败", name);
}

// ============================================================
// 练习 1：给键改名
//   把键 from 改成 to，值保持不变。成功返回 true。以下情况返回 false，而且 m 必须保持原样：
//     - from 不存在
//     - to 已经存在（不能覆盖别人，也不能把 from 弄丢）
//   要求：用 extract，不要用 erase + insert，也不要用 contains / find / count 先判断 to 在不在
//   提示：示例第 [3] 部分 —— 插入失败时，节点会还给你
// ============================================================
bool rename_key(std::map<std::string, int>& m, const std::string& from, const std::string& to)
{
    auto node{m.extract(from)};
    if (node.empty())
    {
        return false;
    }
    node.key() = to;
    if (auto status{m.insert(std::move(node))}; !status.inserted)
    {
        status.node.key() = from;
        m.insert(std::move(status.node));
        return false;
    }
    return true;
}

void test1()
{
    std::map<std::string, int> m{{"Bob", 1}, {"Amy", 2}};
    check(rename_key(m, "Bob", "Robert") && m == std::map<std::string, int>{{"Amy", 2}, {"Robert", 1}},
          "练习 1：正常改名");
    check(!rename_key(m, "Tom", "Tim") && m == std::map<std::string, int>{{"Amy", 2}, {"Robert", 1}},
          "练习 1：from 不存在");
    check(!rename_key(m, "Amy", "Robert") && m == std::map<std::string, int>{{"Amy", 2}, {"Robert", 1}},
          "练习 1：to 已存在");
}

// ============================================================
// 练习 2：找 bug —— 书上的 node_swap
//   交换 k1、k2 两个键对应的元素。正常情况下没问题，但下面两种情况会“弄丢”元素：
//   运行后看哪几项失败，找出原因并修改。要求：
//     - k1 或 k2 不存在，或者 k1 == k2 时，返回 false，m 保持原样
//     - 仍然用 extract 实现
//   提示：在纸上走一遍 node_swap(m, 3, 99)：node1、node2 各是什么？return false 之后 node1 去哪了？
//         也想一想 node_swap(m, 2, 2)
// ============================================================
template <typename M, typename K>
bool node_swap(M& m, K k1, K k2)
{
    auto node1{m.extract(k1)};
    auto node2{m.extract(k2)};
    if (node1.empty() || node2.empty())
    {
        m.insert(std::move(node1));
        m.insert(std::move(node2));
        return false;
    }
    std::swap(node1.key(), node2.key());
    m.insert(std::move(node1));
    m.insert(std::move(node2));
    return true;
}

// 改动（补充）：正确。insert 一个空节点是允许的：什么都不做（返回 inserted = false），所以不用判断哪个是空的。
//   另外，开头的 k1 == k2 判断其实可以去掉：k1 == k2 时，第二次 extract 找不到（已经被第一次摘走了），
//   node2 是空的，正好走进下面的分支，把 node1 放回去，返回 false。我删掉那 4 行试过，四项照样全部通过。
//   而且 map 判断“键相同”用的是比较函数（!(a < b) && !(b < a)），不是 ==。如果用了自定义比较
//   （比如不区分大小写），"abc" == "ABC" 是 false，但 map 认为它们是同一个键。
//   靠 extract 的结果来判断，永远和 map 的看法一致；自己写 == 就可能不一致。

void test2()
{
    using Racermap = std::map<int, std::string>;
    const Racermap start{{1, "Mario"}, {2, "Luigi"}, {3, "Bowser"}};

    Racermap r = start;
    check(node_swap(r, 1, 3) && r == Racermap{{1, "Bowser"}, {2, "Luigi"}, {3, "Mario"}}, "练习 2：正常交换");

    r = start;
    check(!node_swap(r, 3, 99) && r == start, "练习 2：k2 不存在");

    r = start;
    check(!node_swap(r, 99, 3) && r == start, "练习 2：k1 不存在");

    r = start;
    check(!node_swap(r, 2, 2) && r == start, "练习 2：k1 == k2");
}

// ============================================================
// 练习 3：淘汰选手 —— 把节点搬到另一个 map
//   active 是参赛选手（键是编号）。把所有 score < threshold 的选手从 active 搬到 eliminated。
//   要求：Player 一次都不能被复制或移动（下面的计数器会检查）—— 所以只能搬节点
//   提示：(1) 遍历 map 时删除元素，回忆 3.3 节的 it = c.erase(it)
//         (2) extract(it) 之后 it 就失效了，而 extract 不像 erase 那样返回“下一个位置”。怎么办？
// ============================================================
struct Player
{
    static inline int copies = 0; // 复制 + 移动的总次数
    std::string name;
    int score;

    Player(std::string n, int s) : name{std::move(n)}, score{s}
    {
    }

    Player(const Player& other) : name{other.name}, score{other.score}
    {
        ++copies;
    }

    Player(Player&& other) : name{std::move(other.name)}, score{other.score}
    {
        ++copies;
    }

    bool operator==(const Player&) const = default;
};

using Roster = std::map<int, Player>;

void eliminate(Roster& active, Roster& eliminated, int threshold)
{
    for (auto it = active.begin(); it != active.end();)
    {
        if (it->second.score < threshold)
        {
            eliminated.insert(active.extract(it++));
        }
        else
        {
            ++it;
        }
    }
}

// 改动（补充）：正确，extract(it++) 是这个问题的经典写法。为什么安全：
//   it++ 先把 it 挪到下一个元素，再返回“旧的 it”的副本；这些都在调用 extract 之前完成。
//   extract 摘走的是旧位置的元素，只有指向它的迭代器失效，it 已经指向下一个了，不受影响。
//   （map 是树，摘掉一个节点不会影响别的节点 —— 这一点和 vector 不同，vector 删一个，后面的迭代器全失效）
//   写成两行意思更直白：auto next = std::next(it); eliminated.insert(active.extract(it)); it = next;

void test3()
{
    Roster active;
    active.try_emplace(1, "Mario", 90);
    active.try_emplace(2, "Luigi", 40);
    active.try_emplace(3, "Bowser", 30);
    active.try_emplace(4, "Peach", 80);
    active.try_emplace(5, "Toad", 10);
    Roster eliminated;
    eliminated.try_emplace(9, "Wario", 0);

    Player::copies = 0;
    eliminate(active, eliminated, 50);
    int copies = Player::copies; // 先记下来：下面构造用来比较的 map 也会复制

    std::vector<int> a_keys;
    std::vector<int> e_keys;
    for (const auto& [id, p] : active)
    {
        a_keys.push_back(id);
    }
    for (const auto& [id, p] : eliminated)
    {
        e_keys.push_back(id);
    }
    check(a_keys == std::vector{1, 4}, "练习 3：active 剩下 1、4");
    check(e_keys == std::vector{2, 3, 5, 9}, "练习 3：eliminated 是 2、3、5、9");
    check(copies == 0, "练习 3：没有复制或移动 Player");
    std::println("  （复制 + 移动了 {} 次）", copies);
}

// ============================================================
// 练习 4（思考题，把答案写在注释里）
//   (1) 书上说“不能使用字面量引用，因此像 extract(1) 这样的调用会因段错误而使程序崩溃”。
//       示例第 [2] 部分就写了 racers.extract(1)。运行示例，它崩溃了吗？
//       extract 的参数类型是 const key_type&（这里是 const unsigned int&），为什么可以传字面量 1？
//       （提示：void f(const std::string& s); 可以写 f("hello") 吗？）
//   (2) 给键改名也可以不用 extract：
//           auto v = m.at("Bob");
//           m.erase("Bob");
//           m.emplace("Robert", v);
//       假设值的类型是一个有 100 万个元素的 std::vector。这三行和练习 1 的写法相比，多做了哪些事？
//       另外，如果程序别的地方保存了指向 m.at("Bob") 的指针，两种写法之后这个指针还能用吗？（示例第 [4] 部分）
//   (3) 假设 map 允许直接写 it->first = 47。map 是 {1, 2, 3}，把 1 改成 47，map 里的顺序变成什么样？
//       之后 m.find(47) 能找到吗？（提示：find 是从树根开始，比较大小决定往左还是往右走的二分查找）
//
// 你的答案：
//   (1)const的左值引用可以接左值和右值，而字面量是纯右值
//       改动（补充）：对。补一句细节：传 1 时，编译器会创建一个值为 1 的临时对象，让引用绑定到它上面；
//         这个临时对象一直活到整条语句结束，extract 用完它早就返回了，所以不会出问题。
//         示例运行也没有崩溃 —— 书上“段错误”的说法是错的。
//   (2)移动，构造
//       改动（补充）：方向对，但不准确，而且没回答指针那一问。逐行看：
//         auto v = m.at("Bob");      // 复制！at 返回的是引用，auto v 用它构造一个新 vector：
//                                    //   分配一块新内存 + 复制 100 万个元素（不是移动）
//         m.erase("Bob");            // 销毁原来的元素：释放那 100 万个元素的内存，再释放树节点
//         m.emplace("Robert", v);    // 又复制一次！v 是有名字的变量（左值），不会自动移动：
//                                    //   分配树节点 + 再分配一块内存 + 再复制 100 万个元素
//         合计：复制 200 万个元素、多分配 3 次、多释放 2 次（离开作用域时 v 本身也要释放）。
//         最后一行改成 std::move(v) 可以少一次复制，但第一行那次复制还是省不掉。
//         extract 写法：0 次复制，0 次分配 —— 只是把树节点摘下来、换个键、挂回去。
//       指针：erase 写法之后，原来的元素被销毁了，指针悬空，再用就是未定义行为；
//         extract 写法之后，还是同一个元素、同一块内存，指针照样能用（示例 [4] 打印的 true 就是这个）。
//   (3)没看懂
//       改动（补充）：画成树就清楚了。{1, 2, 3} 在 map 内部是一棵二叉搜索树：
//               2
//              / \    ← 注释行的最后一个字符不能是反斜杠，否则下一行会被并进注释，所以这里加了箭头
//             1   3
//         规则：每个节点左边的都比它小，右边的都比它大。遍历 map 就是“左 → 中 → 右”地走，所以是 1 2 3。
//         现在把 1 改成 47，节点不会自己挪位置，还待在 2 的左边：
//               2
//              / \    ←
//            47   3
//         顺序：遍历还是“左 → 中 → 右”，得到 47 2 3 —— 不再有序了。
//         find(47)：从树根 2 开始，47 > 2，往右走 → 到 3，47 > 3，再往右走 → 没有了 → 找不到。
//           47 明明在 map 里，但它待在“左边”，而 find 只会往右边找。
//         更糟的是 insert({47, ...}) 也会成功，于是 map 里有两个 47。
//       我用 const_cast 强行改了键（这是未定义行为，只是演示，千万别这么写），两个编译器的结果都是：
//           {47: 'a', 2: 'b', 3: 'c'}
//           find(47) found: false
//           insert(47) ok: true
//           {47: 'a', 2: 'b', 3: 'c', 47: 'z'}
//         这就是键必须是 const 的原因；要改键，就用 extract 先把节点从树上摘下来。
// ============================================================

int main()
{
    test1();
    test2();
    test3();
}
