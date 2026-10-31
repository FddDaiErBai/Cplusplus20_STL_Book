// 第 2 章 2.4 if 和 switch 语句中初始化变量（C++17），以及范围 for 的初始化（C++20）
// 编译：clang++ -std=c++23 -Wall -Wextra if_switch_init.cpp -o if_switch_init.exe
#include <map>
#include <print>
#include <string>
#include <string_view>
#include <vector>

// 一个会报告“创建”和“销毁”的类，用来观察变量什么时候被销毁
struct Tracer
{
    std::string_view name;

    explicit Tracer(std::string_view n) : name{n}
    {
        std::println("    + {} 创建", name);
    }

    ~Tracer()
    {
        std::println("    - {} 销毁", name);
    }
};

// 模拟书上 SQLite 那种“用输出参数返回结果、用返回值表示错误码”的老式 C 接口
struct Handle
{
    int id;
};

int legacy_open(const char* name, Handle** out) // 返回 0 表示成功
{
    static Handle h{42};
    if (name[0] == '\0')
    {
        return 1;
    }
    *out = &h;
    return 0;
}

int main()
{
    const std::string artist{"Jimi Hendrix"};

    // ------------------------------------------------------------
    // 1. 以前：pos 在 if 外面声明，if 结束后它还在，容易和后面的同名变量冲突
    //    现在：if (初始化; 条件)，pos 只在 if 和 else 里可见
    // ------------------------------------------------------------
    std::println("[1] if 初始化");
    if (std::size_t pos = artist.find("Jimi"); pos != std::string::npos)
    {
        std::println("找到了，位置 {}", pos);
    }
    else
    {
        std::println("没找到，pos = {}", pos); // else 里也能用 pos
    }
    // std::println("{}", pos); // 编译失败：出了 if，pos 就不存在了

    // ------------------------------------------------------------
    // 2. 最常见的用法：map 查找、插入
    // ------------------------------------------------------------
    std::println("\n[2] map");
    std::map<std::string, int> stock{{"apple", 3}};
    if (auto it = stock.find("apple"); it != stock.end())
    {
        std::println("apple 有 {} 个", it->second);
    }
    // 和 2.3 的结构化绑定一起用
    if (auto [it, inserted] = stock.insert({"banana", 5}); inserted)
    {
        std::println("插入了 {}", it->first);
    }

    // ------------------------------------------------------------
    // 3. 初始化的变量在整个 if/else 结束时销毁
    // ------------------------------------------------------------
    std::println("\n[3] 销毁时机");
    if (Tracer t{"t"}; t.name.size() > 5)
    {
        std::println("    if 分支");
    }
    else
    {
        std::println("    else 分支");
    }
    std::println("    if 语句之后");

    // ------------------------------------------------------------
    // 4. switch 初始化
    // ------------------------------------------------------------
    std::println("\n[4] switch 初始化");
    switch (char c = artist[0]; c) // 书上写的 Default: 首字母大写了，应该是 default:
    {
    case 'J':
        std::println("以 J 开头");
        break;
    default:
        std::println("以 {} 开头", c);
        break;
    }

    // ------------------------------------------------------------
    // 5. 书上的 lock_guard 用法，以及一个大坑：变量一定要起名字
    //    lock_guard 创建时加锁、销毁时解锁，这里用 Tracer 代替，看得更清楚
    // ------------------------------------------------------------
    std::println("\n[5] 有名字 vs 没名字");
    if (Tracer guard{"有名字的锁"}; true)
    {
        std::println("    if 里面：锁还在");
    }
    if (Tracer{"没名字的锁"}; true) // 没有名字 = 临时对象，这条初始化语句一结束就销毁了
    {
        std::println("    if 里面：锁已经没了！");
    }
    // 真实代码：if (std::lock_guard<std::mutex>{m}; cond) 编译能过、没有警告，但根本没锁住

    // ------------------------------------------------------------
    // 6. 书上 SQLite 的例子写错了，编译不过：
    //      if (sqlite3_stmt** stmt, auto rc = sqlite3_prepare_v2(db, sql, -1, &_stmt, nullptr); !rc)
    //    初始化部分只能写一条语句，一条声明里不能既有 sqlite3_stmt** 又有 auto；
    //    而且声明的是 stmt，用的却是 _stmt
    //    正确写法：初始化部分声明输出参数，条件部分调用函数
    // ------------------------------------------------------------
    std::println("\n[6] 老式接口");
    if (Handle* h = nullptr; legacy_open("test.db", &h) == 0)
    {
        std::println("打开成功，handle id = {}", h->id);
    }
    else
    {
        std::println("打开失败");
    }

    // ------------------------------------------------------------
    // 7. C++20：范围 for 也能初始化：for (初始化; 元素 : 范围)
    //    典型用法：需要一个下标计数器，但不想让它泄漏到循环外面
    // ------------------------------------------------------------
    std::println("\n[7] 范围 for 初始化");
    std::vector<std::string> names{"Tom", "Amy", "Bob"};
    for (int i = 1; const auto& name : names)
    {
        std::println("第 {} 个：{}", i, name);
        ++i;
    }
    // 另一个用途：先把临时对象存起来，再遍历它的成员，避免悬空（见 range_for_lifetime.cpp）

    // ------------------------------------------------------------
    // 8. C++23：初始化部分还可以写 using 类型别名
    // ------------------------------------------------------------
    if (using Map = std::map<std::string, int>; !Map{}.empty())
    {
        std::println("不会执行");
    }
}
