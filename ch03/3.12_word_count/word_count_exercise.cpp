// 第 3 章 3.12 用 map 统计词频 —— 练习
// 编译：clang++ -std=c++23 -Wall -Wextra word_count_exercise.cpp -o word_count_exercise.exe
// 运行时检查：补完 TODO，编译运行，所有行都显示“通过”就说明做对了。
// 先看示例 word_count.cpp 开头的说明：统计词频分哪四步。
#include <algorithm>
#include <cctype>
#include <cstddef>
#include <map>
#include <print>
#include <regex>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

void check(bool ok, std::string_view name)
{
    std::println("{} {}", ok ? "通过" : "失败", name);
}

// ============================================================
// 练习 1：数单词
//   返回一个 map：键是单词（全部小写），值是出现的次数。
//   “单词”的规则：连在一起的英文字母 a-z、A-Z。其他字符（空格、标点、数字、撇号）都算分隔符。
//     比如 "Don't stop 42 times" 里的单词是 don、t、stop、times
//   两种做法任选：
//     (a) 正则表达式：示例 [5] 里 "[a-zA-Z]+" 就是这个规则，用法照抄示例 [1] 的 sregex_iterator 循环
//     (b) 自己一个字符一个字符地看：是字母就接到当前单词后面；不是字母时，如果当前单词不空，就记下它并清空。
//         注意最后一个单词后面可能没有分隔符（比如 "the end"），循环结束后还要再记一次
//   计数用 ++counts[单词]（示例 [2]）。转小写：std::tolower，参数先转成 unsigned char（示例里的 to_lower）
//   如果用正则：参数是 std::string_view，而示例里的 sregex_iterator 只能用在 std::string 上。两个办法：
//     - 先 std::string s{text}; 复制一份，再照抄示例的写法
//     - 或者换成 std::cregex_iterator(text.data(), text.data() + text.size(), re)
//       （c 表示“用 const char* 指针表示范围”），it->str() 的用法不变
// ============================================================
std::map<std::string, int> count_words(std::string_view text)
{
    std::map<std::string, int> wordmap{};
    const std::regex re{"[a-zA-Z]+"};
    for (auto it = std::cregex_iterator(text.data(), text.data() + text.size(), re); it != std::cregex_iterator{}; ++it)
    {
        std::string s{it->str()};
        std::ranges::transform(s, s.begin(), [](unsigned char c)
        {
            return static_cast<char>(std::tolower(c));
        });
        ++wordmap[s];
    }
    return wordmap;
}

// 改动（补充）：正确，cregex_iterator 直接用在 string_view 上，省掉了一次复制。
//   tolower 的参数写成 unsigned char、结果 static_cast 回 char，两个细节都对。

void test1()
{
    auto r1 = count_words("The cat and the hat. THE END");
    check(r1 == std::map<std::string, int>{{"and", 1}, {"cat", 1}, {"end", 1}, {"hat", 1}, {"the", 3}},
          "练习 1：大小写、标点、最后一个单词");
    std::println("  （得到 {}）", r1);

    auto r2 = count_words("Don't stop: 42 apples, 7 APPLES!");
    check(r2 == std::map<std::string, int>{{"apples", 2}, {"don", 1}, {"stop", 1}, {"t", 1}}, "练习 1：撇号和数字");
    std::println("  （得到 {}）", r2);

    check(count_words("").empty() && count_words(" ... 123 ").empty(), "练习 1：没有单词");
}

// ============================================================
// 练习 2：找 bug —— 前 k 名
//   top_words 想返回出现次数最多的 k 个单词：次数多的在前，次数一样的按字母顺序。
//   单词总数不到 k 个时，有几个返回几个。
//   运行后两项都失败。这里有两个 bug，找出来并修改
//   提示：打印出来看看。sort 的比较函数返回 true 表示“a 排在 b 前面”；
//         v.resize(n) 在 n 比 v.size() 大时会做什么？
// ============================================================
using WordCount = std::pair<std::string, int>;

std::vector<WordCount> top_words(const std::map<std::string, int>& counts, std::size_t k)
{
    std::vector<WordCount> v(counts.begin(), counts.end());
    std::ranges::sort(v, [](const WordCount& a, const WordCount& b)
    {
        if (a.second != b.second)
        {
            return a.second > b.second;
        }
        return a.first < b.first;
    });
    if (k < v.size())
    {
        v.resize(k); // 改动：去掉了这里的 return v;，和下面那句重复了
    }
    return v;
}

// 改动（补充）：两个 bug 都找到了：
//   (1) a.second < b.second 是“次数少的排前面”，改成 > 才是从多到少
//   (2) resize(k) 在 k 比元素个数大时会“补齐”：用默认值 {"", 0} 填满到 k 个，
//       于是 top_words(counts, 10) 得到 4 个真单词 + 6 个 {"", 0}。只在 k 更小时才截断，就对了
//   也可以一行写：v.resize(std::min(k, v.size()));

void test2()
{
    const std::map<std::string, int> counts{{"and", 2}, {"cat", 1}, {"hat", 1}, {"the", 3}};

    auto r1 = top_words(counts, 2);
    check(r1 == std::vector<WordCount>{{"the", 3}, {"and", 2}}, "练习 2：前 2 名");
    std::println("  （得到 {}）", r1);

    auto r2 = top_words(counts, 10);
    check(r2 == std::vector<WordCount>{{"the", 3}, {"and", 2}, {"cat", 1}, {"hat", 1}}, "练习 2：不到 10 个，全部返回");
    std::println("  （得到 {}）", r2);
}

// ============================================================
// 练习 3：按次数分组 —— 不用 sort 的另一种思路
//   把“单词 → 次数”反过来，变成“次数 → 这个次数的所有单词”：
//     {and: 2, cat: 1, hat: 1, the: 3}  →  {1: [cat, hat], 2: [and], 3: [the]}
//   每组里的单词按字母顺序排列。
//   提示：
//     - 返回值的类型是 std::map<int, std::vector<std::string>>：键是次数，值是一个 vector
//     - 遍历 counts（map 本来就按单词的字母顺序遍历），把每个单词 push_back 到 result[次数] 里。
//       result[次数] 不存在时，会自动插入一个空 vector（和 ++m[w] 自动插入 0 是同一个道理）
//     - 这样每组里的单词自然就是字母顺序，不用再排序
// ============================================================
std::map<int, std::vector<std::string>> group_by_count(const std::map<std::string, int>& counts)
{
    std::map<int, std::vector<std::string>> result{};
    for (const auto& [word, count] : counts)
    {
        result[count].push_back(word);
    }
    return result;
}

// 改动（补充）：正确，正是预期的写法。result[count] 第一次出现时自动插入空 vector，再 push_back。

void test3()
{
    const std::map<std::string, int> counts{{"and", 2}, {"cat", 1}, {"hat", 1}, {"the", 3}, {"a", 2}};
    auto r = group_by_count(counts);
    using Groups = std::map<int, std::vector<std::string>>;
    check(r == Groups{{1, {"cat", "hat"}}, {2, {"a", "and"}}, {3, {"the"}}}, "练习 3：分组");
    std::println("  （得到 {}）", r);
    check(group_by_count({}).empty(), "练习 3：空的");
}

// ============================================================
// 练习 4（思考题，把答案写在注释里）
//   (1) ++counts[w] 第一次遇到 w 时，counts 里还没有 w，为什么能从 0 开始加？counts[w] 这时做了什么？
//       如果 map 的值类型是 3.7 节那个“没有默认构造函数”的 BigThing，counts[w] 还能用吗？
//   (2) 运行示例，看 [3]：ranges::move 把 5 个元素从 map 搬到 vector，Word 却被复制了 5 次，一次都没移动。
//       都写了 move 了，为什么还是复制？（提示：map 的元素类型是 pair<const Word, int>，
//       “移动”一个对象时要对原对象做什么？const 允许吗？）
//   (3) 练习 3 的结果是按次数从小到大排的（map 按键从小到大）。可是我们想先打印次数多的。
//       说出两种办法：一种不改 map 的类型，只改遍历的方式；另一种改 map 的类型（提示：3.6 节降序用的那个东西）
//
// 你的答案：
//   (1)counts[]在找不到匹配的键的时候，会默认构造。不可以。
//       改动（补充）：对。准确地说：counts[w] 找不到 w 时，插入 {w, int{}}，int{} 就是 0，再返回这个值的引用，
//         ++ 把它加成 1。BigThing 没有默认构造函数，counts[w] 编译不过（3.7 思考题 (1) 的同一个报错），
//         只能用 try_emplace(w, 初始值) 这类需要你给出值的函数。
//   (2)move一个const对象，其实是在调用拷贝。
//       改动（补充）：结论对，补上“为什么”：std::move(x) 只是把 x 转换成右值（3.4 节），
//         对 const Word 得到的是 const Word&&。移动构造函数的参数是 Word&&（非 const），因为它要修改原对象
//         （把原对象的字符串“偷”走，让它变空）。const Word&& 绑定不到 Word&& 上，
//         能接住它的只有复制构造函数的 const Word&，于是调用了复制。编译器不报错，悄悄就复制了。
//   (3)std::ranges::greater
//       改动（补充）：这是第二种办法（改 map 的类型），对：
//           std::map<int, std::vector<std::string>, std::ranges::greater>
//         我两个编译器都试过，能用。更常见的写法是 std::greater<>（或 std::greater<int>），效果一样。
//         第一种办法没答：不改类型，倒着遍历就行：
//           for (const auto& [c, words] : result | std::views::reverse)   // 或者 rbegin() 到 rend()
//         map 是树，从最右边（最大的键）往左走一样快，两种办法都不用额外排序。
//         区别：改类型是“这个 map 永远从大到小”；倒着遍历是“平时从小到大，这一次倒着看”。
// ============================================================

int main()
{
    test1();
    test2();
    test3();
}
