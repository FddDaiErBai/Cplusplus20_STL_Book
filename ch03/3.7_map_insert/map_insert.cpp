// 第 3 章 3.7 高效地将元素插入到 map 中（emplace、try_emplace、insert_or_assign）
// 编译：clang++ -std=c++23 -Wall -Wextra map_insert.cpp -o map_insert.exe
#include <map>
#include <print>
#include <string>
#include <unordered_map>
#include <utility>

// 书上的“大对象”：构造时打印一行，用来观察它被构造了几次
struct BigThing
{
    std::string v_;

    BigThing(const char* v) : v_{v}
    {
        std::println("  BigThing constructed {}", v_);
    }
};

using Mymap = std::map<std::string, BigThing>;

void printm(const Mymap& m)
{
    for (const auto& [k, v] : m)
    {
        std::print("[{}:{}] ", k, v.v_);
    }
    std::println("");
}

int main()
{
    // ------------------------------------------------------------
    // 1. 往 map 里放元素的几种写法
    //    m[k] = v        ：k 不存在就先插入一个“默认值”，再把 v 赋值过去；k 存在就直接覆盖
    //    insert({k, v})  ：k 存在就什么都不做
    //    emplace(k, v)   ：同上，参数直接拿去构造元素（书上说的“完美转发”）
    //    try_emplace(k, v)：同上，但 k 存在时“连 v 都不构造”（C++17）
    //    m[k] 要求值类型能默认构造：BigThing 没有默认构造函数，所以 m["x"] = "y" 编译不过
    // ------------------------------------------------------------
    std::println("[1] 几种插入写法");
    std::map<std::string, std::string> s;
    s["Miles"] = "Trumpet";
    s.insert({"Hendrix", "Guitar"}); // 书上写 pair<string,string>(...)，用 {} 就够了
    s.emplace("Krupa", "Drums");
    s.try_emplace("Zappa", "Guitar");
    std::println("{}", s);

    Mymap m;
    // m["Miles"] = "Trumpet"; // 编译失败：BigThing 没有默认构造函数（思考题 (1)）
    m.emplace("Miles", "Trumpet");
    m.emplace("Hendrix", "Guitar");
    m.try_emplace("Krupa", "Drums");
    m.try_emplace("Zappa", "Guitar");
    printm(m);

    // ------------------------------------------------------------
    // 2. 书上的重点：键已经存在时
    //    emplace     ：先把整个元素（键 + 值）构造出来，再去查键，发现重复，扔掉 → 白构造了一次
    //    insert({..}) ：{"Zappa", "X"} 在调用 insert 之前就已经构造成一个 pair 了 → 同样白构造
    //    try_emplace ：键是单独的参数，先拿键去查，重复就直接返回，值根本不构造
    //    细节：如果 emplace 的第一个参数“正好是” std::string（键的类型），两个标准库都会先查再构造，
    //          不会白构造。但这只是库自己做的优化，标准没有保证；这里传的是 "Hendrix"（const char*），就没有这个优化
    // ------------------------------------------------------------
    std::println("\n[2] 键重复时");
    std::println("emplace(Hendrix, Singer)：");
    m.emplace("Hendrix", "Singer");
    std::println("insert({{Hendrix, Singer}})：");
    m.insert({"Hendrix", "Singer"});
    std::println("try_emplace(Zappa, Composer)：");
    m.try_emplace("Zappa", "Composer");
    printm(m);

    // ------------------------------------------------------------
    // 3. 返回值：pair<iterator, bool>
    //    second：插入成功没有；first：指向这个键的元素（新插入的，或者原来就有的）
    //    书上用它实现了“没有就插入，有就更新”。C++17 有现成的函数：insert_or_assign
    // ------------------------------------------------------------
    std::println("\n[3] 返回值 / 插入或更新");
    if (auto [it, success] = m.try_emplace("Zappa", "Composer"); !success) // 2.3 结构化绑定 + 2.4 if 初始化
    {
        std::println("Zappa 已存在，原来的值是 {}，更新：", it->second.v_);
        it->second = "Composer";
    }
    std::println("insert_or_assign(Liszt, Piano)：");
    auto [it2, inserted2] = m.insert_or_assign("Liszt", "Piano"); // 不存在 → 插入
    std::println("  inserted = {}，{} 现在是 {}", inserted2, it2->first, it2->second.v_);
    std::println("insert_or_assign(Liszt, Organ)：");
    auto [it3, inserted3] = m.insert_or_assign("Liszt", "Organ"); // 已存在 → 赋值
    std::println("  inserted = {}，{} 现在是 {}", inserted3, it3->first, it3->second.v_);
    printm(m);

    // ------------------------------------------------------------
    // 4. try_emplace 的另一个保证（书上没讲）：插入失败时，传进去的参数不会被“搬走”
    //    emplace 失败时，你 std::move 进去的东西可能已经被搬进那个白构造的元素，然后跟着一起扔掉了
    //    （下面这种写法两个标准库都会搬走；换成别的参数形式，有的库会、有的不会 —— 标准没有保证）
    //    insert({k, std::move(x)}) 更不用说，pair 在调用前就构造好了，x 一定被搬走
    // ------------------------------------------------------------
    std::println("\n[4] 失败时参数还在不在");
    std::map<std::string, std::string> roles{{"Hendrix", "Guitar"}};
    std::string a = "a long string that does not fit in SSO buffer";
    std::string b = a;
    roles.emplace("Hendrix", std::move(a));
    roles.try_emplace("Hendrix", std::move(b));
    std::println("emplace 失败后 a = \"{}\"", a);
    std::println("try_emplace 失败后 b = \"{}\"", b);

    // ------------------------------------------------------------
    // 5. 小结
    //    只想“没有才插入”         → try_emplace（键存在时什么都不做，也不碰参数）
    //    想“没有就插入，有就覆盖” → insert_or_assign（比 m[k] = v 好：不要求默认构造，还告诉你是插入还是覆盖）
    //    unordered_map 也有这两个函数；set、multimap 没有（思考题 (3)）
    //    注意：try_emplace("Zappa", ...) 还是要先用 "Zappa" 构造一个 std::string 键才能去查
    //          （书上的 Note 说的就是这个）；C++26 允许直接拿 "Zappa" 去查，省掉这次构造
    // ------------------------------------------------------------
    std::println("\n[5] unordered_map");
    std::unordered_map<std::string, BigThing> u;
    u.try_emplace("Miles", "Trumpet");
    u.try_emplace("Miles", "Flugelhorn"); // 不打印：没有构造
    std::println("u.size() = {}", u.size());
}
