// 第 3 章 3.7 高效地将元素插入到 map 中 —— 练习
// 编译：clang++ -std=c++23 -Wall -Wextra map_insert_exercise.cpp -o map_insert_exercise.exe
// std::map 不能在编译时使用（要到 C++26），所以这次不用 static_assert，改成运行时检查：
//   补完 TODO，编译运行，所有行都显示“通过”就说明做对了。
#include <cstddef>
#include <map>
#include <print>
#include <string>
#include <string_view>
#include <vector>

void check(bool ok, std::string_view name)
{
    std::println("{} {}", ok ? "通过" : "失败", name);
}

// ============================================================
// 练习 1：每个单词第一次出现的位置
//   返回一个 map：键是单词，值是它“第一次”出现的下标
//   要求：循环里只写一行，用本节学的函数；不要用 find / contains / count 先判断
//   提示：同一个单词再次出现时，什么都不该做
// ============================================================
std::map<std::string, std::size_t> first_index(const std::vector<std::string>& words)
{
    std::map<std::string, std::size_t> result;
    for (std::size_t i = 0; i < words.size(); ++i)
    {
        result.try_emplace(words[i], i);
    }
    return result;
}

void test1()
{
    auto r = first_index({"to", "be", "or", "not", "to", "be"});
    check(r == std::map<std::string, std::size_t>{{"be", 1}, {"not", 3}, {"or", 2}, {"to", 0}}, "练习 1");
}

// ============================================================
// 练习 2：缓存 —— 结果是对的，但构造了太多次
//   Expensive 构造一次代价很大（这里用计数器代替）。get_or_create 想做的事：
//   key 已经在缓存里就直接返回它；不在才构造一个新的放进去。
//   下面的写法返回的结果是对的，但运行后“构造次数”那一项失败。找出原因并修改（只需要改一个地方）
// ============================================================
struct Expensive
{
    static inline int constructed = 0; // 一共构造了多少次
    int id;

    Expensive(int i) : id{i}
    {
        ++constructed;
    }
};

Expensive& get_or_create(std::map<int, Expensive>& cache, int key)
{
    return cache.try_emplace(key, key).first->second;
}

void test2()
{
    std::map<int, Expensive> cache;
    Expensive::constructed = 0;
    bool all_ok = true;
    for (int key : {1, 2, 1, 1, 3, 2})
    {
        if (get_or_create(cache, key).id != key)
        {
            all_ok = false;
        }
    }
    check(all_ok, "练习 2：返回的元素");
    check(cache.size() == 3, "练习 2：缓存里有 3 个元素");
    check(Expensive::constructed == 3, "练习 2：构造次数 = 不同 key 的个数");
    std::println("  （实际构造了 {} 次）", Expensive::constructed);
}

// ============================================================
// 练习 3：找 bug —— 更新价格
//   set_price 想做的事：商品不存在就添加；已经存在就改成新价格。返回 true 表示是新添加的
//   运行后有一项失败。找出原因并修改
//   要求：修改后的函数体只有一行（不用 if，也不用 m[k] = v）
// ============================================================
bool set_price(std::map<std::string, int>& prices, const std::string& name, int price)
{
    return prices.insert_or_assign(name, price).second;
}

void test3()
{
    std::map<std::string, int> prices{{"apple", 5}};
    bool a = set_price(prices, "banana", 3); // 新商品
    bool b = set_price(prices, "apple", 6);  // 涨价
    check(a && !b, "练习 3：返回值");
    check(prices == std::map<std::string, int>{{"apple", 6}, {"banana", 3}}, "练习 3：价格");
}

// ============================================================
// 练习 4（思考题，把答案写在注释里）
//   (1) 示例里 m["Miles"] = "Trumpet"; 这一行被注释掉了。取消注释，编译一次，报错大概说了什么？为什么？
//       换成 std::map<std::string, std::string> 就能编译。这时 m["Miles"] = "Trumpet" 实际做了哪两步？
//   (2) 运行示例，看第 [3] 部分：insert_or_assign 更新已经存在的 Liszt 时，还是打印了
//       “BigThing constructed Organ”。键已经存在，为什么还构造了一个 BigThing？
//       （提示：BigThing 能从 const char* 隐式转换。“赋值”时等号右边是什么类型？）
//   (3) set 没有 try_emplace 和 insert_or_assign。为什么 set 不需要它们？
//
// 你的答案：
//   (1)显示的写出了构造函数，没有默认构造函数
//       改动（补充）：对。自己写了任何构造函数，编译器就不再自动生成默认构造函数。
//         clang 的报错是 no matching constructor for initialization of 'BigThing'（在 pair.h 里）：
//         m["Miles"] 键不存在时要插入 {"Miles", BigThing()}，而 BigThing() 不存在。
//       第二问没答：对 map<string, string>，m["Miles"] = "Trumpet" 分两步：
//         ① m["Miles"]：键不存在 → 插入 {"Miles", ""}（string 默认构造是空串），返回这个值的引用
//         ② = "Trumpet"：再对这个空串赋值
//         所以它是“先造一个默认值，再覆盖”，比 insert_or_assign 多做一步，而且要求值类型能默认构造。
//   (2)不知道
//       改动（补充）：键已存在时，insert_or_assign(k, obj) 做的事就是 it->second = obj;
//         这里 obj 是 "Organ"（const char*），it->second 是 BigThing。BigThing 没有写“用 const char* 赋值”的
//         operator=，只有编译器自动生成的“用另一个 BigThing 赋值”。于是编译器先用 BigThing(const char*)
//         把 "Organ" 隐式转换成一个临时 BigThing（打印就是这时出现的），再把临时对象移动赋值过去。
//         书上 [3] 的 it->second = payload; 也是同一个原因。
//       验证：给 BigThing 加一个 BigThing& operator=(const char* v) { v_ = v; return *this; }，
//         再运行示例，更新 Zappa 和 Liszt 时就都不打印了（我试过，可以自己加上试试）。
//       所以 try_emplace / insert_or_assign 只能保证“键已存在时不构造新元素”，
//         赋值本身要花多少代价，取决于值类型的 operator=。
//   (3)set没有值
//       改动（补充）：对。展开说：set 的元素就是键本身，没有“键之外的值”。
//         try_emplace 的意义是“键已存在时不构造值”——set 没有值，emplace 也只是构造键，没什么可省的；
//         insert_or_assign 的意义是“键已存在时更新值”——set 的元素是 const 的（改了会打乱排序），也没有值可更新。
//         同理 multimap 也没有这两个：它允许键重复，“键已存在”这个情况根本不存在。
// ============================================================

int main()
{
    test1();
    test2();
    test3();
}
