// 第 3 章 3.10 使用 set 对输入进行排序和筛选 —— 练习
// 编译：clang++ -std=c++23 -Wall -Wextra set_words_exercise.cpp -o set_words_exercise.exe
// 运行时检查：补完 TODO，编译运行，所有行都显示“通过”就说明做对了。
// 为了方便测试，函数都从 std::istream& 读，测试时传 istringstream；换成 std::cin 就是读键盘输入。
#include <algorithm>
#include <iterator>
#include <optional>
#include <print>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

void check(bool ok, std::string_view name)
{
    std::println("{} {}", ok ? "通过" : "失败", name);
}

// ============================================================
// 练习 1：按长度排序的单词表
//   读入所有单词，去掉重复的，按“短的在前；一样长的按字母顺序”排好，放进 vector 返回
//   要求：
//     - 用 std::set<std::string, ByLength>，ByLength 自己写（示例 [4] 的 CaseLess 是同一种写法）
//     - 读入用 istream_iterator（或 views::istream），不要自己写 while (in >> w) 循环
//   提示：set 用一对迭代器就能构造（示例 [2]）；vector 也是
// ============================================================
struct ByLength
{
    bool operator()(const std::string& a, const std::string& b) const
    {
        if (a.size() == b.size())
        {
            return a < b;
        }
        return a.size() < b.size();
    }
};

std::vector<std::string> by_length(std::istream& in)
{
    std::set<std::string, ByLength> word(std::istream_iterator<std::string>{in}, std::istream_iterator<std::string>{});

    return {word.begin(), word.end()};
}

void test1()
{
    std::istringstream in1{"pear fig apple fig kiwi date"};
    auto r1 = by_length(in1);
    check(r1 == std::vector<std::string>{"fig", "date", "kiwi", "pear", "apple"}, "练习 1：排序 + 去重");
    std::println("  （得到 {}）", r1);

    std::istringstream in2{""};
    check(by_length(in2).empty(), "练习 1：没有输入");
}

// ============================================================
// 练习 2：找 bug —— 编号排序
//   输入是一串编号（整数），sorted_ids 想返回去重之后、从小到大排好的编号。
//   运行后两项都失败。找出原因并修改
//   提示：打印出来看看顺序。set<std::string> 是按什么排的？"7" 和 "007" 是同一个字符串吗？
// ============================================================
std::vector<int> sorted_ids(std::istream& in)
{
    std::set<int> s{std::istream_iterator<int>{in}, std::istream_iterator<int>{}};
    return {s.begin(), s.end()};
}

// 改动（补充）：正确。原来的 bug 有两个，根源是同一个 —— set 是按“键的类型”的规则排序、去重的：
//   (1) set<std::string> 按字符比较："100" 的第一个字符 '1' 小于 "7" 的 '7'，所以 100 排在 7 前面
//   (2) "7"、"007"、"07" 是三个不同的字符串，set 当然认为它们不重复；stoi 之后才都变成 7，已经晚了
//   改成直接读 int：>> 读整数时 "007" 就是 7，排序、去重都按数值来，两个问题一起解决。

void test2()
{
    std::istringstream in1{"42 7 100 7 9 42"};
    auto r1 = sorted_ids(in1);
    check(r1 == std::vector{7, 9, 42, 100}, "练习 2：从小到大");
    std::println("  （得到 {}）", r1);

    std::istringstream in2{"7 007 07"};
    auto r2 = sorted_ids(in2);
    check(r2 == std::vector{7}, "练习 2：7、007、07 是同一个编号");
    std::println("  （得到 {}）", r2);
}

// ============================================================
// 练习 3：第一个重复的单词
//   从输入里一个一个读单词，返回第一个“第二次出现”的单词；没有重复就返回 std::nullopt。
//   要求：
//     - 找到之后立刻返回，后面的单词一个都不要再读（测试会检查流里剩下的内容）
//     - 不要用 find / contains / count —— 用 insert 的返回值判断（3.7 节）
// ============================================================
std::optional<std::string> first_repeat(std::istream& in)
{
    std::set<std::string> s{};
    std::string word{};
    while (in >> word)
    {
        if (!s.insert(word).second)
        {
            return word;
        }
    }
    return std::nullopt;
}

// 改动（补充）：正确，用 insert(...).second 判断，找到就返回，没有多读。
//   这一题没有要求用 istream_iterator，while (in >> word) 完全可以。用迭代器也不会多读：
//       for (std::istream_iterator<std::string> it{in}, end{}; it != end; ++it)
//       {
//           if (!s.insert(*it).second) return *it;
//       }
//   istream_iterator 只在“构造”和“++”时读，return 时没有执行 ++，所以下一个单词还留在流里。

void test3()
{
    std::istringstream in1{"to be or not to be"};
    auto r1 = first_repeat(in1);
    std::string next;
    in1 >> next;
    check(r1 == "to", "练习 3：找到 to");
    check(next == "be", "练习 3：后面的单词没有被读掉");

    std::istringstream in2{"a b c b a"};
    check(first_repeat(in2) == "b", "练习 3：找到 b（不是 a）");

    std::istringstream in3{"x y z"};
    check(!first_repeat(in3).has_value(), "练习 3：没有重复");

    std::istringstream in4{""};
    check(!first_repeat(in4).has_value(), "练习 3：没有输入");
}

// ============================================================
// 练习 4（思考题，把答案写在注释里）
//   (1) 示例 [3] 从 "5 3 5 1 x 4 2" 读整数，只读到了 {1, 3, 5}。后面的 4 和 2 为什么没读进来？
//       nums.clear() 是干什么的？clear 之后接着读，为什么读到的是 "x"，而不是 "4"？
//   (2) 书上写的是 inserter(words, words.end())，要传一个位置。可是 set 里的元素放在哪，是 set 自己按大小决定的，
//       这个位置有什么用？看示例 [5] 最后一行：按从小到大的顺序插入时，insert(end(), 值) 为什么只比较了一两次？
//       如果输入是乱序的，这个位置还有帮助吗？
//   (3) 练习 1 里，如果 ByLength 只写了 return a.size() < b.size();
//       "pear fig apple fig kiwi date" 会得到什么？（可以改了试试）
//       没有人写过 ==，为什么 set 认为 "kiwi" 和 "pear" 是重复的？（提示：3.8 节练习 2 的批注）
//
// 你的答案：
//   (1)迭代器会变成end()。清除失败的状态的吧。x之前没读成功。
//       改动（补充）：三点都对，把因果连起来：
//         ① 读到 x 时，nums >> int 失败：x 不是数字。失败的读取不会“吃掉” x，它还留在流里
//         ② 流被打上“失败”标记（failbit）。之后所有的 >> 都直接失败，不会再去看后面的内容 ——
//            所以 4 和 2 根本没有被尝试读取，istream_iterator 也就变成了 end，循环结束
//         ③ clear() 把失败标记清掉，流恢复正常；接着读，第一个碰到的还是那个没被吃掉的 x
//       （键盘输入时也一样：要 int 的地方打了字母，程序后面所有的 cin >> 都会“跳过”，常见的新手 bug）
//   (2)决定什么时候结束吗？指定位置了。有帮助。
//       改动（补充）：三问都不对。这个位置是一个“提示”（hint），跟结束无关（结束是由 it == end 决定的）。
//         insert(提示, 值) 会先检查：值是不是正好该放在“提示”的前面？也就是 前一个元素 < 值 < 提示处的元素。
//           - 猜对了：直接挂在那里，只比较一两次，不用从树根一层层往下找
//           - 猜错了：退回到普通的 insert，从树根开始找 —— 白白多比较了一两次
//         按从小到大插入时，每个新值都比已有的全部大，正好该放在末尾，所以 end() 次次都猜对：
//           clang：insert(值) 比较 5544778 次，insert(end(), 值) 只有 99999 次（每个 1 次）
//         乱序时 end() 几乎总是猜错，没有帮助，反而略慢。我用 10 万个随机数试过：
//           clang：insert(值) 2320365 次，insert(end(), 值) 2420158 次 —— 多了约 10 万次（每个白比较 1 次）
//           g++  ：1755005 次 vs 1854889 次，同样多了约 10 万次
//         那 inserter 为什么非要一个位置？因为 inserter 是通用的：给 vector、list 用时，位置就是“插在哪”，
//           是必须的；给 set 用时就只是个提示。没有不要位置的版本，书上就随手填了 end()。
//   (3)a.size() < b.size()比较方式不严格。
//       改动（补充）：方向不对。“只比长度”恰恰是一个合法的比较方式（标准里的说法叫 strict weak ordering，
//         正好有“严格”这个词），set 能正常工作，只是它比你想要的粗糙。实际结果（两个编译器一样）：
//             {"fig", "pear", "apple"}        ← kiwi 和 date 不见了
//         原因：set 判断“是不是同一个元素”不用 ==，而是用比较函数问两次：
//             !(a < b) && !(b < a)   → a 不比 b 小，b 也不比 a 小，那就认为它们“等价”
//         ByLength("kiwi", "pear") 是 false（4 不小于 4），ByLength("pear", "kiwi") 也是 false，
//         所以 set 认为 kiwi 和 pear 是同一个元素，插入 kiwi 失败（date 同理）；先来的 pear 留下了。
//         这和 3.8 节说的一样：map/set 只认比较函数，同样的规则在示例 [4] 里让 "Apple" 和 "apple" 合并。
//         所以写比较函数时要想清楚：哪些东西你希望被当成“同一个”？你写的“长度一样再比字母”就是在说：
//         只有完全相同的字符串才算同一个。
// ============================================================

int main()
{
    test1();
    test2();
    test3();
}
