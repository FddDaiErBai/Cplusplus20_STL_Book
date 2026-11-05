// 第 3 章 3.13 用“vector 的 vector”按长度给句子排序
// 编译：clang++ -std=c++23 -Wall -Wextra sentences.cpp -o sentences.exe
// 运行：
//   ./sentences.exe                         不带参数：处理程序里写好的一段英文（下面的 text）
//   ./sentences.exe - < 某个英文文本.txt      带参数 -：处理标准输入里的文字（书上的用法）
//
// ============================================================
// 要做的事：写文章时，句子太长不好读。给一段英文，找出每个句子有几个单词，按从长到短列出来：
//     12: The vector is generally the most flexible of the ...
//      7: This recipe uses a vector of vectors.
//
// 数据怎么存：“vector 的 vector”—— std::vector<std::vector<std::string>>
//   一个句子 = 一串单词 = vector<string>
//   一段文字 = 一串句子 = vector<vector<string>>
//
//     sentences ──→ [ 句子0 ,  句子1 ,  句子2 ]          外层 vector：每个元素是一个句子
//                      │        │        │
//                      ↓        ↓        ↓
//                   ["Hi."]  ["How", "are", "you?"]  ["Fine."]     内层 vector：句子里的单词
//
//   sentences.size()       句子的个数（3）
//   sentences[1]           第 1 个句子，类型是 vector<string>
//   sentences[1].size()    第 1 个句子有几个单词（3）
//   sentences[1][2]        第 1 个句子的第 2 个单词 "you?"
//   sentences.back()       最后一个句子 —— 读文字时，新单词总是加到“当前正在读的句子”，也就是 back() 上
//
// 分三步：
//   ① 一个单词一个单词地读（>> 按空白分开，标点会粘在单词上："you?"）
//      每个单词加到当前句子 sentences.back() 的末尾；
//      如果这个单词是句子的结尾（带着 . ! ?），就在外层再加一个空句子，后面的单词放进新句子里
//   ② 按句子的单词数从多到少排序 —— 外层 vector 的元素是 vector，排序时比较的是它们的 size()
//   ③ 打印：每个句子只打印前几个单词，太长的用 ... 省略
// ============================================================
#include <algorithm>
#include <cstddef>
#include <cstring>
#include <functional>
#include <iostream>
#include <map>
#include <print>
#include <ranges>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

using Sentence = std::vector<std::string>;

constexpr std::string_view text = R"(It can be useful for a writer to make sure that sentences are not too long.
Let's build a tool that evaluates a text for sentence length. Short is fine!
Choosing the right container is key when using the STL. The vector is generally the most
flexible of the STL containers. Is it? This recipe uses a vector of vectors.)";

// ------------------------------------------------------------
// 书上判断“句子结尾”的函数：单词里任何位置出现 . ! ? 中的一个，就算句尾
//   strchr(字符串, c)：C 语言的函数（<cstring>），在字符串里找字符 c，找到返回指针，找不到返回 nullptr
//   书上参数写成 const string_view&：string_view 本身就很小（一个指针 + 一个长度），直接按值传就行
// ------------------------------------------------------------
bool book_is_eos(std::string_view word)
{
    constexpr const char* end_punct{".!?"};
    for (auto c : word)
    {
        if (std::strchr(end_punct, c) != nullptr)
        {
            return true;
        }
    }
    return false;
}

// 改进：只看“最后一个字符”是不是 . ! ?（[2] 说明为什么）
bool is_eos(std::string_view word)
{
    return !word.empty() && std::string_view{".!?"}.contains(word.back()); // contains：C++23
}

// ① 书上的读法：把输入切成句子
template <typename EosFn>
std::vector<Sentence> split_sentences(std::istream& in, EosFn eos)
{
    std::vector<Sentence> sentences{Sentence{}}; // 一开始就放一个空句子，让 back() 一定存在
    for (std::string s; in >> s;)
    {
        sentences.back().push_back(s);
        if (eos(s))
        {
            sentences.emplace_back(); // 句子结束：在外层加一个新的空句子
        }
    }
    // 最后一个单词如果是句尾，后面会多出一个空句子，删掉
    if (sentences.back().empty())
    {
        sentences.pop_back();
    }
    return sentences;
}

// ③ 打印一个句子：最多 limit 个单词，多了就加 ...
void print_sentence(const Sentence& s, std::size_t limit)
{
    std::print("  {:>2}:", s.size());
    for (const auto& w : s | std::views::take(limit)) // take(limit)：最多取前 limit 个
    {
        std::print(" {}", w);
    }
    std::println("{}", s.size() > limit ? " ..." : "");
}

int main(int argc, char*[])
{
    std::string input{text};
    if (argc > 1)
    {
        input.assign(std::istreambuf_iterator<char>{std::cin}, std::istreambuf_iterator<char>{});
    }

    // ------------------------------------------------------------
    // 1. 书上的做法，三步全部做完
    //    ② 排序：ranges::sort(sentences, 比较函数)。比较函数收到两个句子 l、r，
    //       返回 true 表示 l 排在 r 前面；按单词数从多到少，就是 l.size() > r.size()
    //       （书上写的是 sort(vv_sentences, ...)，少了 ranges::。std::sort 只接受一对迭代器，不接受整个容器）
    //    ③ 书上用一个计数器 limit 每打印一个单词减 1，减到 0 就 break；这里用 views::take，更不容易写错（练习 2）
    // ------------------------------------------------------------
    std::println("[1] 书上的做法");
    std::istringstream in1{input};
    auto sentences = split_sentences(in1, book_is_eos);
    std::ranges::sort(sentences, [](const Sentence& l, const Sentence& r)
    {
        return l.size() > r.size();
    });
    for (const auto& s : sentences)
    {
        print_sentence(s, 8);
    }

    // ------------------------------------------------------------
    // 2. 书上 is_eos 的问题：它检查单词里的“每一个”字符
    //    "3.14" 中间有个点 → 被当成句尾，一个句子被切成了两半
    //    书上自己也承认“若单词中间有标点符号，这个函数将无法正确地分开句子”
    //    只看最后一个字符就好多了。还有一些情况两种写法都处理不了：
    //      "Mr." "e.g." 这类缩写以点结尾，会被误认为句尾（要彻底解决得有一张缩写表）
    //      "\"Stop!\"" 最后一个字符是引号，改进版反而认不出句尾（练习 1 处理）
    // ------------------------------------------------------------
    std::println("\n[2] 判断句尾");
    const std::string tricky = "Pi is about 3.14 today. Mr. Smith agreed.";
    std::istringstream in2a{tricky};
    std::istringstream in2b{tricky};
    std::println("原文：{}", tricky);
    std::println("书上的 is_eos（看每个字符）：{}", split_sentences(in2a, book_is_eos));
    std::println("改进的 is_eos（看最后一个）：{}", split_sentences(in2b, is_eos));

    // ------------------------------------------------------------
    // 3. 一样长的句子谁在前？ranges::sort 不保证（思考题 (2)）
    //    sort 只保证“按比较函数排好”；比较函数认为“一样”的元素（这里是单词数相同的句子），
    //    它们之间谁在前谁在后，sort 不管 —— 可能和原来的顺序不一样。
    //    ranges::stable_sort（stable = 稳定）额外保证：一样的元素保持原来的先后顺序。
    //    下面造 40 个句子，单词数只有 1~4 四种，句子的第一个单词是它原来的编号，排序后看看
    //    （元素很少时，标准库内部用的“插入排序”恰好是稳定的，所以要多一点元素才看得出来）
    //    sort 之后的顺序，两个编译器还不一样（标准库内部的做法不同）；stable_sort 的结果是确定的
    // ------------------------------------------------------------
    std::println("\n[3] sort 和 stable_sort");
    std::vector<Sentence> many;
    for (int i = 0; i < 40; ++i)
    {
        Sentence s(static_cast<std::size_t>(i * 7 % 4 + 1), "w");
        s[0] = std::to_string(i);
        many.push_back(s);
    }
    auto by_size_desc = [](const Sentence& l, const Sentence& r)
    {
        return l.size() > r.size();
    };
    auto first_words_of_length = [](const std::vector<Sentence>& v, std::size_t len)
    {
        std::vector<std::string> ids;
        for (const auto& s : v)
        {
            if (s.size() == len)
            {
                ids.push_back(s[0]);
            }
        }
        return ids;
    };
    auto a = many;
    std::ranges::sort(a, by_size_desc);
    auto b = many;
    std::ranges::stable_sort(b, by_size_desc);
    std::println("4 个单词的句子，原来的编号：");
    std::println("  sort        之后：{}", first_words_of_length(a, 4));
    std::println("  stable_sort 之后：{}", first_words_of_length(b, 4));

    // ------------------------------------------------------------
    // 4. 排序时，句子里的单词被复制了吗？没有
    //    sort 交换两个句子时，用的是移动（3.4 节）。移动一个 vector 只是把它内部的三个指针
    //    （数据在哪、用了多少、容量多少）交给另一个 vector，单词本身待在原来的内存里不动。
    //    所以排 vector<vector<string>> 并不慢：每次交换只是挪几个指针，不管句子有多长。
    //    下面记下每个句子的 data()（内层 vector 存单词的那块内存的地址），排序后看看还是不是那几块
    // ------------------------------------------------------------
    std::println("\n[4] 排序移动的是什么");
    std::istringstream in4{input};
    auto ss = split_sentences(in4, is_eos);
    std::vector<const std::string*> before;
    for (const auto& s : ss)
    {
        before.push_back(s.data());
    }
    std::ranges::stable_sort(ss, by_size_desc);
    std::size_t same = 0;
    for (const auto& s : ss)
    {
        if (std::ranges::find(before, s.data()) != before.end())
        {
            ++same;
        }
    }
    std::println("{} 个句子，排序后仍然使用原来那块内存的有 {} 个", ss.size(), same);
    std::vector<std::ptrdiff_t> from;
    for (const auto& s : ss)
    {
        from.push_back(std::ranges::find(before, s.data()) - before.begin());
    }
    std::println("排序后从前到后，每个句子用的是原来第几个句子的内存：{}", from);

    // ------------------------------------------------------------
    // 5. 书上说“因为需要自定义排序，所以 vector 更合适”—— 不完全对
    //    map/multimap 也能自定义顺序：第三个模板参数传 std::greater<>，键就从大到小（3.12 思考题 (3)）
    //    multimap（多重 map）允许键重复：同一个单词数可以对应好几个句子。
    //    multimap 对重复的键保证“先插入的排在前面”，所以效果和 stable_sort 一样（下一节 3.14 专门讲 multimap）
    //    两种做法都行：vector 是“先全部收集，最后排一次”；multimap 是“每插入一个就放到正确的位置”
    // ------------------------------------------------------------
    std::println("\n[5] 用 multimap");
    std::multimap<std::size_t, Sentence, std::greater<>> by_len;
    std::istringstream in5{input};
    for (auto& s : split_sentences(in5, is_eos))
    {
        by_len.emplace(s.size(), std::move(s));
    }
    for (const auto& [len, s] : by_len)
    {
        print_sentence(s, 8);
    }
}
