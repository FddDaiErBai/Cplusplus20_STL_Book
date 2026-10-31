// 第 3 章 3.9 用自定义类型作为 unordered_map 的键 —— 练习
// 编译：clang++ -std=c++23 -Wall -Wextra unordered_key_exercise.cpp -o unordered_key_exercise.exe
// 运行时检查：补完 TODO，编译运行，所有行都显示“通过”就说明做对了。
#include <cstddef>
#include <cstdint> // 改动：练习 3 用到了 std::uint32_t，它在 <cstdint> 里（现在能编译只是别的头文件顺带包含了）
#include <functional>
#include <print>
#include <string>
#include <string_view>
#include <unordered_set>

void check(bool ok, std::string_view name)
{
    std::println("{} {}", ok ? "通过" : "失败", name);
}

// ============================================================
// 练习 1：给 Date 写哈希
//   补完下面 std::hash<Date> 的 operator()。要求：2000~2029 年里所有的 (年, 月, 日) 组合
//   （日期不管合不合法，月 1~12，日 1~31）得到的哈希值互不相同
//   提示：示例 [3] 说明了 x + y 为什么不行。想一想怎么让 (2001, 2, 3) 和 (2001, 3, 2) 得到不同的数
//         ——比如把它们当成一个“很多位的数”的不同位
// ============================================================
struct Date
{
    int year;
    int month;
    int day;

    bool operator==(const Date&) const = default;
};

template <>
struct std::hash<Date>
{
    std::size_t operator()(const Date& d) const
    {
        return static_cast<std::size_t>(d.year) * 10000 + static_cast<std::size_t>(d.month) * 100
            + static_cast<std::size_t>(d.day);
    }
};

// 改动（补充）：能通过，思路也对 —— 用 "0" 把年、月、日隔开，拼成一个十进制数。
//   我检查过 1000~2147 年范围内也没有重复。几点补充：
//   (1) 这里写 string 而不是 std::string 也能编译：struct std::hash<Date> 是 std 命名空间的成员，
//       在它里面查名字会先查 std。能编译，但在别处这么写就不行了，建议还是写 std::string
//   (2) 年份大一点就会出错：比如 (2200, 12, 31) 拼成 "2200012031" = 22 亿，超过了 int 的上限（约 21.4 亿），
//       std::stoi 会抛出 std::out_of_range 异常。哈希函数抛异常，插入、查找都会跟着失败
//   (3) 慢：每算一次哈希，要做 3 次 to_string、几次字符串拼接（可能要分配内存），再解析一遍字符串。
//       而 unordered_map 每次插入、查找都要算一次哈希
//   同样是“让年、月、日占不同的十进制位”，直接用乘法就行，不需要字符串：
//       return static_cast<std::size_t>(d.year) * 10000 + static_cast<std::size_t>(d.month) * 100
//            + static_cast<std::size_t>(d.day);
//   (2001, 2, 3) → 20010203，(2001, 3, 2) → 20010302。月和日都小于 100，所以各占两位，不会互相干扰。

void test1()
{
    std::unordered_set<std::size_t> hashes;
    for (int y = 2000; y < 2030; ++y)
    {
        for (int m = 1; m <= 12; ++m)
        {
            for (int d = 1; d <= 31; ++d)
            {
                hashes.insert(std::hash<Date>{}({y, m, d}));
            }
        }
    }
    check(hashes.size() == 30 * 12 * 31, "练习 1：哈希值互不相同");
    std::println("  （{} 个日期，得到 {} 种不同的哈希值）", 30 * 12 * 31, hashes.size());

    std::unordered_set<Date> holidays{{2024, 1, 1}, {2024, 10, 1}, {2024, 1, 1}};
    check(holidays.size() == 2 && holidays.contains({2024, 10, 1}) && !holidays.contains({2024, 1, 10}),
          "练习 1：unordered_set<Date> 能正常使用");
}

// ============================================================
// 练习 2：找 bug —— 不区分大小写的名字
//   Name 的 == 不区分大小写："Bob"、"bob"、"BOB" 是同一个名字。
//   但放进 unordered_set 之后，它们都被当成了不同的元素。找出原因并修改
//   要求：不要改 Name 的 ==，只改 std::hash<Name>
//   提示：unordered_set 先用哈希值决定去哪个桶里找，再在桶里用 == 比较。
//         如果两个“相等”的名字哈希值不一样，会怎样？
// ============================================================
constexpr char lower(char c)
{
    return c >= 'A' && c <= 'Z' ? static_cast<char>(c - 'A' + 'a') : c;
}

struct Name
{
    std::string s;

    bool operator==(const Name& other) const
    {
        if (s.size() != other.s.size())
        {
            return false;
        }
        for (std::size_t i = 0; i < s.size(); ++i)
        {
            if (lower(s[i]) != lower(other.s[i]))
            {
                return false;
            }
        }
        return true;
    }
};

template <>
struct std::hash<Name>
{
    std::size_t operator()(const Name& n) const
    {
        string s{n.s};
        for (auto& i : s)
        {
            i = lower(i);
        }
        return std::hash<std::string>{}(s);
    }
};

void test2()
{
    std::unordered_set<Name> names{{"Bob"}, {"bob"}, {"Amy"}, {"BOB"}, {"amy"}};
    check(names.size() == 2, "练习 2：只剩 2 个名字");
    check(names.contains({"bOb"}), "练习 2：能找到 bOb");
    std::println("  （names.size() = {}）", names.size());
}

// ============================================================
// 练习 3：走过了几个格子
//   一个机器人从 (0, 0) 出发，moves 里每个字符走一步：U 上 (y+1)、D 下 (y-1)、L 左 (x-1)、R 右 (x+1)。
//   返回它一共到过多少个不同的格子（包括起点）
//   要求：
//     - 用 std::unordered_set<Pos, PosHash>，哈希作为模板参数传进去（示例 [4]），不要特化 std::hash<Pos>
//     - PosHash 自己写，要求和练习 1 一样：不要用 x + y 这种容易重复的写法
//   提示：坐标可能是负数
// ============================================================
struct Pos
{
    int x{};
    int y{};

    bool operator==(const Pos&) const = default;
};

struct PosHash
{
    std::size_t operator()(const Pos& p) const
    {
        // static_assert(sizeof(std::size_t) == 8); 这里vs好像代码提示好像是x86
        const auto px = static_cast<std::size_t>(static_cast<std::uint32_t>(p.x));
        const auto py = static_cast<std::size_t>(static_cast<std::uint32_t>(p.y));

        // 改动：按 size_t 的大小分两种情况（理由见下面的注释）
        if constexpr (sizeof(std::size_t) >= 8)
        {
            return px << 32 | py;
        }
        else
        {
            return px * 31 + py;
        }
    }
};

// 改动（补充）：你注意到的问题是真的。VS 的代码提示如果按 x86（32 位）分析，size_t 只有 32 位，
//   px << 32 就是“移动的位数 >= 类型的位数”，这是未定义行为。我用 32 位的 clang 编译了你原来的写法：
//       warning: shift count >= width of type [-Wshift-count-overflow]
//   64 位下没问题（两个编译器都没有警告），所以 static_assert 那行如果打开，在 32 位下会直接编译失败 ——
//   作为“这段代码只支持 64 位”的声明，也是一种合理的做法。
//   上面改成了 if constexpr：sizeof(std::size_t) 是编译时就知道的常量，所以可以当 if constexpr 的条件。
//   （对比 1.6、2.6 节的错误：函数参数是运行时的值，不能用在 if constexpr 里。sizeof 可以）
//   32 位下 size_t 根本放不下两个 32 位的 int，冲突在所难免，只能尽量让结果分散，所以用了 * 31 + y。
//   我用 32 位 clang 编译改过的版本，没有警告了。
//   另外，distinct_visits 里每个 case 都写了一遍 step.insert(position)。可以在 switch 后面统一插入一次；
//   遇到非法字符时，在 default 里 continue 跳过就行。

std::size_t distinct_visits(std::string_view moves)
{
    std::unordered_set<Pos, PosHash> step{};
    Pos position{0, 0};
    step.insert(position);

    for (const auto& i : moves)
    {
        switch (i)
        {
            case 'U':
                position.y += 1;
                break;
            case 'D':
                position.y -= 1;
                break;
            case 'L':
                position.x -= 1;
                break;
            case 'R':
                position.x += 1;
                break;
            default:
                continue;
        }
        step.insert(position);
    }
    return step.size();
}

void test3()
{
    check(distinct_visits("") == 1, "练习 3：原地不动");
    check(distinct_visits("UUUU") == 5, "练习 3：一直往上");
    check(distinct_visits("UDUD") == 2, "练习 3：来回走");
    check(distinct_visits("URDL") == 4, "练习 3：转一圈");
    check(distinct_visits("LLDDRRUU") == 8, "练习 3：负数坐标");
    check(distinct_visits("RULDLDRU") == 7, "练习 3：8 字形，回到起点两次");
}

// ============================================================
// 练习 4（思考题，把答案写在注释里）
//   (1) 把示例里 std::hash<Coord> 的 return 改成 return 0;（所有键的哈希值都一样），编译运行。
//       第 [1] 部分的结果还对吗？第 [3] 部分 “x + y” 那一行，用到几个桶、== 调用了多少次？
//       （把那一行输出贴在这里；编译时会有一个 unused parameter 警告，不用管）
//       为什么说哈希函数写得差，“结果不会错，只是会慢”？
//   (2) 练习 2 说明了一条规则：如果 a == b，那么 hash(a) 必须等于 hash(b)。
//       反过来呢？hash(a) == hash(b) 时，a 一定等于 b 吗？用示例 [3] 里的 x + y 举一个例子
//   (3) 示例 [3] 的“拼接”哈希，一万个点的哈希值全都不同，但输出说只用到了 9220 个桶（g++ 是 8404 个），
//       最挤的桶里有 2 个元素。哈希值都不一样，为什么还会挤到同一个桶里？
//       （提示：示例 [2]，桶只有 bucket_count 个，哈希值却可以是任意大的数）
//
// 你的答案：
//   (1)改编译警告命令才能编译，结果是对的。用到1个，调用5000500次。会在同一个桶里面继续比较
//       改动（补充）：对，次数少抄了一个 0：两个编译器都是 50005000 次（5000 万）。
//         （警告本身不会阻止编译，除非开了“把警告当成错误”，比如 -Werror 或者 VS 的 /WX）
//         这个数是怎么来的：所有元素都在同一个桶里，桶就成了一条链表。查找第 i 个元素，
//         最多要比较 i 次，一万次查找合计 1 + 2 + … + 10000 = 10000 × 10001 / 2 = 50005000 次。
//         对比“拼接”哈希的 10000 次（每次查找比较 1 次），慢了 5000 倍。
//       “结果不会错”：哈希只决定去哪个桶找，最后判断“是不是同一个键”的是 ==。只要 == 写对了，
//         哪怕所有元素都在一个桶里，也一定能找到；只是查找从“直接跳到桶里”变成了“一个一个比过去”，
//         从 O(1) 退化成 O(n)。
//   (2)不一定。就如问题1一样
//       改动（补充）：对。用 x + y 举例：(0, 1) 和 (1, 0) 的哈希值都是 1，但它们不相等。
//         这叫哈希冲突。冲突是不可避免的：哈希值只有 2^64 种，而可能的键（比如所有字符串）多得多。
//         所以规则只有单向的：相等 → 哈希必须相同；哈希相同 → 不一定相等，还要用 == 确认。
//   (3)有的数字溢出了？
//       改动（补充）：不是溢出。x << 32 | y 最大也就用到 64 位里的低 39 位，size_t 完全放得下。
//         原因在于：哈希值可以是很大的数，但桶只有 bucket_count 个（clang 是 12853 个），
//         所以容器会把哈希值再“折算”成桶的编号，通常就是取余：桶号 = 哈希值 % bucket_count。
//         两个不同的哈希值，只要除以 12853 的余数相同，就会进同一个桶。
//         比如哈希值 5 和 12858，5 % 12853 = 5，12858 % 12853 = 5，都进 5 号桶。
//         一万个不同的数放进一万多个桶，有一些撞在一起是正常的；好的哈希只是让每个桶都不太挤（最多 2 个），
//         所以查找还是基本上一次就中（示例里 == 只调用了 10000 次）。
// ============================================================

int main()
{
    test1();
    test2();
    test3();
}
