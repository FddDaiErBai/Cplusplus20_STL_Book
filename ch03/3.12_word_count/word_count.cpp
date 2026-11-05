// 第 3 章 3.12 用 map 统计词频
// 编译：clang++ -std=c++23 -Wall -Wextra word_count.cpp -o word_count.exe
// 运行：
//   ./word_count.exe                          不带参数：统计程序里写好的一段英文（下面的 poem）
//   ./word_count.exe - < 某个英文文本.txt       带参数 -：统计标准输入里的文字（书上的用法）
//
// ============================================================
// 要做的事：给一段英文，统计每个单词出现了几次，按次数从多到少打印前几名
//   比如 "The cat and the hat." → the: 2，and: 1，cat: 1，hat: 1
//
// 分四步：
//   ① 把文字切成单词。不能简单地按空格切：那样 "hat." 会带着句号，"The" 和 "the" 也会被当成两个词。
//      所以要：只取字母（和数字），去掉标点；再统一转成小写
//   ② 数次数：用 map<string, int>，键是单词，值是次数。每遇到一个单词，它的次数加 1
//   ③ 排序：map 是按单词（键）排序的，而我们想按次数排序。map 的顺序改不了，
//      所以把内容复制到一个 vector<pair<string, int>> 里，再对 vector 按自己的规则排序
//   ④ 打印前 N 个
// ============================================================
#include <algorithm>
#include <cctype>
#include <cstddef>
#include <iostream>
#include <iterator>
#include <map>
#include <print>
#include <ranges>
#include <regex>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

// 爱伦·坡《乌鸦》（1845）的前两节。原文的破折号 — 换成了 --：
// std::regex 处理中文、— 这类非 ASCII 字符不可靠，这一节只处理英文
constexpr std::string_view poem = R"(Once upon a midnight dreary, while I pondered, weak and weary,
Over many a quaint and curious volume of forgotten lore--
While I nodded, nearly napping, suddenly there came a tapping,
As of some one gently rapping, rapping at my chamber door.
"'Tis some visitor," I muttered, "tapping at my chamber door--
Only this and nothing more."
Ah, distinctly I remember it was in the bleak December;
And each separate dying ember wrought its ghost upon the floor.
Eagerly I wished the morrow;--vainly I had sought to borrow
From my books surcease of sorrow--sorrow for the lost Lenore--
For the rare and radiant maiden whom the angels name Lenore--
Nameless here for evermore.)";
// R"( ... )" 叫“原始字符串”：里面的引号、换行都按原样保存，不用转义

// 把字符串转成小写
// std::tolower 的参数如果是负数（char 存了非 ASCII 字节时会是负数）是未定义行为，
// 所以 lambda 的参数写成 unsigned char，让 char 先转成 0~255
std::string to_lower(std::string s)
{
    std::ranges::transform(s, s.begin(), [](unsigned char c)
    {
        return static_cast<char>(std::tolower(c));
    });
    return s;
}

// 第 [3] 部分用：数一数被复制、被移动了几次的“单词”
struct Word
{
    static inline int copies = 0;
    static inline int moves = 0;
    std::string s;

    Word(std::string str) : s{std::move(str)}
    {
    }

    Word(const Word& other) : s{other.s}
    {
        ++copies;
    }

    Word(Word&& other) noexcept : s{std::move(other.s)}
    {
        ++moves;
    }

    Word& operator=(const Word&) = default;
    Word& operator=(Word&&) = default;
    auto operator<=>(const Word&) const = default;
};

int main(int argc, char*[])
{
    std::string text;
    if (argc > 1)
    {
        text.assign(std::istreambuf_iterator<char>{std::cin}, std::istreambuf_iterator<char>{}); // 读入全部输入
    }
    else
    {
        text = poem;
    }

    // ------------------------------------------------------------
    // 1. 书上的做法（四步全部做完）
    //
    // ① 切单词：用“正则表达式”（regex）。正则表达式是一种描述“什么样的文字”的小语言，这里只用到一点：
    //      \w   表示一个“单词字符”：字母 a-z A-Z、数字 0-9、下划线 _
    //      +    表示“前面那个，连续一个或多个”
    //      所以 \w+ 就是“一串连在一起的单词字符”，遇到空格、标点就断开
    //    C++ 字符串里反斜杠是转义符，要写两个 "\\w+" 才表示正则里的 \w+
    //    std::sregex_iterator(开头, 结尾, 正则)：在字符串里一个接一个地找匹配的片段，
    //      *it 是一次匹配，it->str() 是匹配到的文字；默认构造的 sregex_iterator{} 表示“找完了”
    //      （和 3.10 的 istream_iterator 是一样的用法）
    //
    // ② 数次数：书上写的是
    //        auto [map_it, result] = wordmap.try_emplace(word_str, 0);   // 不存在就先放一个 0
    //        auto& [w, count] = *map_it;
    //        ++count;
    //    其实一行 ++wordmap[word]; 就够了（见 [2]）
    //
    // ③ 排序：vector 里放 pair<string, int>，first 是单词，second 是次数
    //    sort 的第二个参数是一个“比较函数”：给两个元素 a、b，返回 true 表示“a 应该排在 b 前面”
    //      次数不同：次数多的在前 → return a.second > b.second;
    //      次数相同：按字母顺序   → return a.first < b.first;
    //
    // ④ 打印前 N 个：for (int limit{5}; const auto& [w, count] : wordvec)
    //    这是 C++20 的“带初始化的 range-for”：分号前面声明一个只在循环里用的变量（和 2.4 节的 if 初始化一样）
    // ------------------------------------------------------------
    std::println("[1] 书上的做法");
    std::map<std::string, int> wordmap;
    std::size_t total_words = 0;
    const std::regex word_re{"\\w+"};

    for (auto it = std::sregex_iterator(text.begin(), text.end(), word_re); it != std::sregex_iterator{}; ++it)
    {
        ++wordmap[to_lower(it->str())];
        ++total_words;
    }

    std::vector<std::pair<std::string, int>> wordvec(wordmap.begin(), wordmap.end()); // 复制到 vector
    std::ranges::sort(wordvec, [](const auto& a, const auto& b)
    {
        if (a.second != b.second)
        {
            return a.second > b.second;
        }
        return a.first < b.first;
    });

    std::println("一共 {} 个单词，其中不同的有 {} 个", total_words, wordmap.size()); // 书上两行都写成了 unique word count
    for (int limit{8}; const auto& [w, count] : wordvec)
    {
        std::println("  {:>2}: {}", count, w);
        if (--limit == 0)
        {
            break;
        }
    }
    std::println("  （也可以写成 for (const auto& [w, count] : wordvec | std::views::take(8))）");

    // ------------------------------------------------------------
    // 2. 数次数：++m[w]
    //    m[w] 的规则（3.5、3.7 节）：w 已经在 map 里 → 返回它的值的引用；
    //    w 不在 → 先插入 {w, 值类型的默认值}，再返回引用。int 的“默认值”是 0（思考题 (1)）
    //    所以 ++m[w]：第一次遇到 w 时插入 0 再加成 1，以后每次加 1。一行就够了
    //
    //    不需要单词按顺序排好的话（反正第 ③ 步还要重新排序），用 unordered_map（3.9 节）通常更快，写法完全一样
    // ------------------------------------------------------------
    std::println("\n[2] ++m[w]");
    std::map<std::string, int> m;
    for (std::string w : {"the", "cat", "the", "hat", "the"})
    {
        ++m[w];
    }
    std::println("{}", m);

    // ------------------------------------------------------------
    // 3. 书上说 ranges::move(wordmap, back_inserter(wordvec)) “使这种传输快速而有效” —— 不对
    //    map 的元素类型是 pair<const string, int>：键是 const 的（3.8 节）。
    //    “移动”是要修改原对象的（把原对象的内存“偷”过来，原对象变空），const 的对象不让改，
    //    所以对 const 对象 std::move 之后，实际调用的还是复制构造函数 —— 名字叫 move，做的是复制（思考题 (2)）
    //    下面用一个会数次数的 Word 当键来验证
    //
    //    真想把键移动出来，可以用 3.8 节的 extract：节点摘下来之后，node.key() 不再是 const 的，可以移动。
    //    （代价是 map 被清空了。单词一般很短，复制其实也很便宜，这里只是说明书上的说法不对）
    // ------------------------------------------------------------
    std::println("\n[3] 从 map 移动到 vector？");
    std::map<Word, int> wm;
    for (std::string w : {"apple", "banana", "cherry", "durian", "elderberry"})
    {
        wm.emplace(w, 1);
    }

    std::vector<std::pair<Word, int>> v1;
    v1.reserve(wm.size()); // 先预留空间，避免 vector 扩容时的移动干扰计数
    Word::copies = 0;
    std::ranges::move(wm, std::back_inserter(v1));
    std::println("ranges::move：5 个元素，复制了 {} 次", Word::copies);

    std::vector<std::pair<Word, int>> v2;
    v2.reserve(wm.size());
    Word::copies = 0;
    while (!wm.empty())
    {
        auto node = wm.extract(wm.begin());
        v2.emplace_back(std::move(node.key()), node.mapped());
    }
    std::println("extract 再移动：5 个元素，复制了 {} 次（map 现在空了：size = {}）", Word::copies, wm.size());

    // ------------------------------------------------------------
    // 4. 只要前几名，可以不用全部排序：std::ranges::partial_sort
    //    partial_sort(v, v.begin() + k, 比较函数)：只保证前 k 个是排好序的“最前面 k 个”，后面的顺序不管
    //    元素很多、只要前几名时，比全部排序省事（比如 [1] 全部排序要把 83 个词都排好，而我们只看前几个）
    // ------------------------------------------------------------
    std::println("\n[4] partial_sort");
    std::vector<std::pair<std::string, int>> v3(wordmap.begin(), wordmap.end());
    const std::size_t k = std::min<std::size_t>(5, v3.size());
    std::ranges::partial_sort(v3, v3.begin() + static_cast<std::ptrdiff_t>(k), [](const auto& a, const auto& b)
    {
        if (a.second != b.second)
        {
            return a.second > b.second;
        }
        return a.first < b.first;
    });
    std::println("前 {} 名：{}", k, v3 | std::views::take(k));

    // ------------------------------------------------------------
    // 5. \w+ 切单词的问题（书上没讲）
    //    "'Tis"、"don't" 里的撇号 ' 不是 \w，所以 'Tis → tis，don't → don 和 t
    //    数字也算 \w：2024 会被当成一个“单词”
    //    下划线也算：snake_case 是一个词
    //    想要不同的规则，就换正则，比如 "[a-zA-Z]+" 只要字母，"[a-zA-Z']+" 把撇号也算进去
    // ------------------------------------------------------------
    std::println("\n[5] 切单词的规则");
    const std::string sample = "Don't panic: it's 2024, use snake_case!";
    for (const char* pattern : {"\\w+", "[a-zA-Z]+", "[a-zA-Z']+"})
    {
        std::regex re{pattern};
        std::vector<std::string> parts;
        for (auto it = std::sregex_iterator(sample.begin(), sample.end(), re); it != std::sregex_iterator{}; ++it)
        {
            parts.push_back(it->str());
        }
        std::println("  {:<12}{}", pattern, parts);
    }
}
