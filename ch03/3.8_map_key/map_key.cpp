// 第 3 章 3.8 高效地修改 map 元素的键（extract 和节点句柄）
// 编译：clang++ -std=c++23 -Wall -Wextra map_key.cpp -o map_key.exe
#include <map>
#include <print>
#include <set>
#include <string>
#include <utility>

using Racermap = std::map<unsigned int, std::string>;

void printm(const Racermap& m)
{
    for (const auto& [rank, racer] : m)
    {
        std::print("{}:{}  ", rank, racer);
    }
    std::println("");
}

// ------------------------------------------------------------
// 书上的 node_swap：交换两个键对应的“值”（或者说交换两个元素的键）
// 注意：这个函数有 bug，某些参数会让 map 丢失元素 —— 练习 2 里找
// ------------------------------------------------------------
template <typename M, typename K>
bool node_swap(M& m, K k1, K k2)
{
    auto node1{m.extract(k1)};
    auto node2{m.extract(k2)};
    if (node1.empty() || node2.empty())
    {
        return false;
    }
    std::swap(node1.key(), node2.key());
    m.insert(std::move(node1));
    m.insert(std::move(node2));
    return true;
}

int main()
{
    // ------------------------------------------------------------
    // 1. map 的键是 const 的：元素的类型是 pair<const Key, T>
    //    为什么不让改？map 内部是一棵按键排好序的树，每个元素放在哪，是由它的键决定的。
    //    直接改了键，元素还待在原来的位置，顺序就乱了，后面的查找都会出错
    // ------------------------------------------------------------
    std::println("[1] 键不能直接改");
    Racermap racers{{1, "Mario"}, {2, "Luigi"}, {3, "Bowser"}, {4, "Peach"}, {5, "Donkey Kong Jr"}};
    auto it = racers.begin();
    // it->first = 47;     // 编译失败：键是 const 的
    it->second = "Mario!"; // 值可以改：值不影响顺序
    printm(racers);

    // ------------------------------------------------------------
    // 2. extract（C++17）：把一个元素从 map 里“摘下来”，变成一个节点句柄（node handle）
    //    摘下来之后，它已经不在树里了，所以可以通过 node.key() 修改键；改完再 insert 回去，
    //    map 会按新的键把它放到正确的位置
    //    整个过程：值（这里是 string）不复制、不移动，也不重新分配内存 —— 只是把树上的节点挂到别的位置
    // ------------------------------------------------------------
    std::println("\n[2] 书上的 node_swap(3, 5)");
    node_swap(racers, 3u, 5u);
    printm(racers);

    auto node = racers.extract(1); // 书上说 extract(1) 这种“字面量”会段错误 —— 错的，见思考题 (1)
    std::println("摘下来的节点：empty() = {}，key() = {}，mapped() = {}", node.empty(), node.key(), node.mapped());
    std::println("摘下之后 map 里还剩 {} 个", racers.size());
    node.key() = 10;
    racers.insert(std::move(node));
    std::println("改成 10 再放回去：");
    printm(racers);

    // ------------------------------------------------------------
    // 3. 插入节点失败（新的键已经存在）时，节点并没有丢：
    //    insert(节点) 返回 insert_return_type，有三个成员：
    //      position：指向那个键的元素（插入成功是新元素，失败是原来就有的那个）
    //      inserted：成功没有
    //      node    ：失败时，节点原封不动地还给你（书上直接 exit(1)，其实还可以改个键再放回去）
    // ------------------------------------------------------------
    std::println("\n[3] 插入失败");
    auto peach = racers.extract(4);
    peach.key() = 5; // 5 已经有人了
    auto status = racers.insert(std::move(peach));
    std::println("inserted = {}，5 号是 {}，节点还在：{}（{}）",
                 status.inserted, status.position->second, !status.node.empty(), status.node.mapped());
    status.node.key() = 6;
    racers.insert(std::move(status.node));
    printm(racers);

    // ------------------------------------------------------------
    // 4. 元素的地址不变（书上没讲）
    //    因为节点只是被“摘下来再挂上去”，元素本身一直在同一块内存里。
    //    摘下来之前拿到的指针、引用，放回去之后照样能用（摘下来期间不能用）
    //    对比 erase + insert：旧元素被销毁，新元素是新分配的，以前的指针全都失效
    // ------------------------------------------------------------
    std::println("\n[4] 地址不变");
    const std::string* p = &racers.at(2);
    auto luigi = racers.extract(2);
    luigi.key() = 20;
    racers.insert(std::move(luigi));
    std::println("{}，同一个元素：{}", *p, p == &racers.at(20));

    // ------------------------------------------------------------
    // 5. 节点还能在两个 map 之间搬（书上没讲）
    //    书上说 extract 是“map 类及其派生类”的成员 —— 不准确：标准库容器之间没有“派生”关系。
    //    准确地说，所有关联容器都有 extract：map、set、multimap、multiset 和对应的 unordered_ 版本。
    //    键和值类型相同的两个容器之间，可以直接把节点从一个搬到另一个（不复制元素）
    //    merge（C++17）：把另一个容器里所有“这边还没有的键”整个搬过来，有冲突的留在原处
    //    set 的节点没有 key()/mapped()，只有 value()
    // ------------------------------------------------------------
    std::println("\n[5] 在容器之间搬节点");
    Racermap retired;
    retired.insert(racers.extract(5));
    std::print("racers：");
    printm(racers);
    std::print("retired：");
    printm(retired);

    std::map<int, std::string> a{{1, "x"}, {2, "y"}};
    std::map<int, std::string> b{{2, "Y"}, {3, "Z"}};
    a.merge(b);
    std::println("merge 之后 a = {}，b = {}（键 2 冲突，留在 b 里）", a, b);

    std::set<std::string> s{"apple", "banana"};
    auto sn = s.extract("apple");
    sn.value() = "cherry"; // set 的元素平时也是 const 的，摘下来才能改
    s.insert(std::move(sn));
    std::println("set：{}", s);
}
