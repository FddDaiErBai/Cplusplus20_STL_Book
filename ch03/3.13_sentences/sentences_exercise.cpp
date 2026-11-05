// 第 3 章 3.13 用“vector 的 vector”按长度给句子排序 —— 练习
// 编译：clang++ -std=c++23 -Wall -Wextra sentences_exercise.cpp -o sentences_exercise.exe
// 运行时检查：补完 TODO，编译运行，所有行都显示“通过”就说明做对了。
// 先看示例 sentences.cpp 开头的说明和那张“vector 的 vector”的图。
#include <algorithm>
#include <cstddef>
#include <functional>
#include <iterator>
#include <numeric>
#include <print>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

void check(bool ok, std::string_view name)
{
    std::println("{} {}", ok ? "通过" : "失败", name);
}

using Sentence = std::vector<std::string>;

// ============================================================
// 练习 1：切句子
//   (a) ends_sentence(word)：这个单词是不是句子的结尾。规则：
//         先去掉末尾的引号和右括号（" ' )，可能有好几个），剩下的最后一个字符是 . ! ? 之一，就是句尾
//         "fine!" → 是     "\"Stop!\"" → 是（去掉 " 之后是 !）    "(really?)" → 是
//         "3.14" → 不是    "\"Hi\"" → 不是    "\"\"" → 不是（去完什么都不剩）
//       提示：std::string_view 有 remove_suffix(1)（去掉最后一个字符）、back()（最后一个字符）、empty()；
//             std::string_view{"\"')"}.contains(c) 判断 c 是不是这三个字符之一（示例里的 is_eos 用了同样的写法）
//   (b) split_sentences(text)：按示例 [1] 的方法把文字切成句子，用 ends_sentence 判断句尾。
//       要求：结果里不能有空句子；最后一句没有句尾标点也要算一句
//       提示：std::istringstream in{std::string{text}}; 然后 for (std::string s; in >> s;) 一个个读单词
// ============================================================
bool ends_sentence(std::string_view word)
{
    while (!word.empty() && std::string_view{"\"')"}.contains(word.back())) // 改动：先判断 !empty()，再取 back()
    {
        word.remove_suffix(1);
    }
    return !word.empty() && std::string_view{".!?"}.contains(word.back());

}

// 改动（补充）：思路对，但 while 的条件原来是 contains(word.back()) && !word.empty()，顺序反了。
//   && 是从左往右算的：先执行了 word.back()，再检查 empty。word 为空时调用 back() 是未定义行为（3.5 节）。
//   ends_sentence("") 和 ends_sentence("\"\"")（两个引号去掉之后为空）都会走到这里。
//   实际结果：
//     clang 默认设置：读到了旁边内存里的某个字节，碰巧不是引号，12 项全过 —— 看起来没问题，其实是运气
//     g++（-O0 默认开着检查）：第一项都没打印出来就终止了：
//         string_view:291: ... back() const ...: Assertion 'this->_M_len > 0' failed.
//     clang 加 -D_LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_FAST：Illegal instruction，同样终止
//   把两个条件对调就好了：!word.empty() 为 false 时，&& 不会再算右边（“短路”），back() 就不会被调用。
//   下面那个 if 你写的顺序是对的 —— 同一个道理，两处要一致。
//   另外，最后的 if/return true/return false 可以直接写成 return !word.empty() && ...;

std::vector<Sentence> split_sentences(std::string_view text)
{
    std::vector<Sentence> sentences{Sentence{}};
    std::istringstream in{std::string{text}};
    for (std::istream_iterator<std::string> it{in}, end{}; it != end; ++it)
    {
        sentences.back().push_back(*it);
        if (ends_sentence(*it))
        {
            sentences.emplace_back();
        }
    }
    if (sentences.back().empty())
    {
        sentences.pop_back();
    }
    return sentences;
}

// 改动（补充）：正确。用 istream_iterator 也可以，记得加了 <iterator>，很好。

void test1()
{
    bool ok = ends_sentence("fine!") && ends_sentence("\"Stop!\"") && ends_sentence("(really?)")
        && ends_sentence("end.");
    check(ok, "练习 1：ends_sentence 认出句尾");
    ok = !ends_sentence("3.14") && !ends_sentence("\"Hi\"") && !ends_sentence("\"\"") && !ends_sentence("")
        && !ends_sentence("word");
    check(ok, "练习 1：ends_sentence 不误判");

    auto r1 = split_sentences("He said \"Stop!\" Then he left. Pi is 3.14 now");
    check(r1 == std::vector<Sentence>{{"He", "said", "\"Stop!\""}, {"Then", "he", "left."}, {"Pi", "is", "3.14", "now"}},
          "练习 1：切成 3 句");
    std::println("  （得到 {}）", r1);

    check(split_sentences("Yes.") == std::vector<Sentence>{{"Yes."}}, "练习 1：一句，句尾后面没有空句子");
    check(split_sentences("").empty() && split_sentences("   ").empty(), "练习 1：没有单词");
}

// ============================================================
// 练习 2：找 bug —— 句子预览
//   preview(s, limit)：把句子的前 limit 个单词用空格连起来；如果后面还有单词没显示，加上 " ..."。
//     preview({"a", "b", "c", "d"}, 2) == "a b ..."
//     preview({"a", "b", "c", "d"}, 4) == "a b c d"      （正好显示完，不加 ...）
//     preview({"a", "b", "c", "d"}, 0) == "..."          （一个都不显示，只有 ...，前面没有空格）
//   下面是照书上的写法改的，运行后有一项失败。找出原因并修改
//   提示：limit 是 0 时，--left 会变成多少？（2.3、3.5 节：size_t 是无符号的）
// ============================================================
std::string preview(const Sentence& s, std::size_t limit)
{
    std::string out;
    std::size_t left = limit;
    for (const auto& w : s)
    {
        if (!out.empty())
        {
            out += ' ';
        }
        if (left > 0)
        {
            out += w;
            if (--left == 0)
            {
                if (s.size() > limit)
                {
                    out += " ...";
                }
                break;
            }
        }
        else
        {
            out += "...";
            break;
        }
    }
    return out;
}

// 改动（补充）：能通过，原因也找对了（limit 为 0 时 --left 从 0 减成 18446744073709551615，永远等不到 0）。
//   不过修得有点绕：每个单词都先判断要不要加空格，再判断 left，“...”分成了两处来加。
//   更直接的写法是“先检查，再加单词”，一个计数器 shown 从 0 往上数，不做减法也就不会回绕：
//       std::string out;
//       std::size_t shown = 0;
//       for (const auto& w : s)
//       {
//           if (shown == limit)                        // 已经显示够了，后面还有单词
//           {
//               out += out.empty() ? "..." : " ...";
//               break;
//           }
//           if (!out.empty())
//           {
//               out += ' ';
//           }
//           out += w;
//           ++shown;
//       }
//       return out;
//   无符号数做减法之前，先想一想“会不会减到 0 以下”—— 这是你在 2.3、3.5 节也遇到过的问题。

void test2()
{
    const Sentence s{"a", "b", "c", "d"};
    check(preview(s, 2) == "a b ...", "练习 2：显示 2 个");
    check(preview(s, 4) == "a b c d" && preview(s, 9) == "a b c d", "练习 2：全部显示");
    check(preview({}, 3).empty(), "练习 2：空句子");
    check(preview(s, 0) == "...", "练习 2：limit 为 0");
    std::println("  （preview(s, 0) 得到 \"{}\"）", preview(s, 0));
}

// ============================================================
// 练习 3：只排“编号”，不动句子
//   rank_by_length(sentences) 返回句子的编号（下标），按句子的单词数从多到少排；单词数一样的，编号小的在前。
//     比如 sentences 的单词数是 [3, 5, 3, 1]  →  返回 [1, 0, 2, 3]
//   sentences 是 const 的，不能排序它本身。这种“排编号”的做法在实际中很常用：原数据不动，
//   要按好几种顺序看时，各排一份编号就行。
//   提示：
//     - 先做一个 vector<std::size_t> idx(sentences.size())，再用 std::iota(idx.begin(), idx.end(), 0)
//       填成 0, 1, 2, ...（<numeric>，iota 就是“从 0 开始一个个往上数”）
//     - 排序 idx：比较函数收到两个编号 i、j，要比较的是 sentences[i].size() 和 sentences[j].size()。
//       lambda 要用到外面的 sentences，写 [&] 把它“捕获”进来
//     - “单词数一样，编号小的在前”：用 stable_sort（示例 [3]）；或者在比较函数里，单词数一样时比较 i < j
//     - 也可以用 3.6 节的“投影”：std::ranges::stable_sort(idx, std::greater{}, 投影函数)
// ============================================================
std::vector<std::size_t> rank_by_length(const std::vector<Sentence>& sentences)
{
    std::vector<std::size_t> idx(sentences.size());
    std::ranges::iota(idx, 0uz);
    std::ranges::stable_sort(idx, std::greater{}, [&](std::size_t i)
    {
        return sentences[i].size();
    });
    return idx;
}

// 改动（补充）：正确，stable_sort + 投影，最简洁的写法。ranges::iota 是 C++23 的，两个编译器都支持。
//   两个小地方：
//     - lambda 参数写成 std::size_t：不带 std:: 的 size_t 能用，是因为头文件碰巧也在全局放了一份
//     - iota(idx, 0) 的 0 是 int，填进 size_t 时每次都要转换一下；写 0uz（C++23，size_t 类型的 0）更准确
//   我试过把 stable_sort 换成 sort：40 个句子那一项，两个编译器都会失败。

void test3()
{
    const std::vector<Sentence> ss{{"a", "b", "c"}, {"a", "b", "c", "d", "e"}, {"x", "y", "z"}, {"q"}};
    auto r = rank_by_length(ss);
    check(r == std::vector<std::size_t>{1, 0, 2, 3}, "练习 3：按单词数排编号");
    std::println("  （得到 {}）", r);

    std::vector<Sentence> many;
    for (std::size_t i = 0; i < 40; ++i)
    {
        many.push_back(Sentence(i * 7 % 4 + 1, "w")); // 40 个句子，单词数只有 1~4，很多一样长的
    }
    auto r2 = rank_by_length(many);
    bool ok = r2.size() == 40;
    for (std::size_t k = 1; ok && k < r2.size(); ++k)
    {
        std::size_t a = many[r2[k - 1]].size();
        std::size_t b = many[r2[k]].size();
        ok = a > b || (a == b && r2[k - 1] < r2[k]);
    }
    check(ok, "练习 3：40 个句子，一样长的编号从小到大");
    check(rank_by_length({}).empty(), "练习 3：没有句子");
}

// ============================================================
// 练习 4（思考题，把答案写在注释里）
//   (1) 运行示例，看 [2]：书上的 is_eos 把 "Pi is about 3.14 today." 切成了哪几段？为什么？
//       改进版也把 "Mr. Smith agreed." 切错了，错在哪？
//   (2) 示例 [3]：stable_sort 比 sort 多保证了什么？
//       书上示例的输出里有两个 16 个单词的句子，用的是 sort。换一个编译器，它们的先后顺序还一定一样吗？
//   (3) 示例 [4]：排序后，每个句子用的还是原来那块内存（打印出来的编号只是换了顺序）。
//       sort 交换两个句子时，句子里的单词（string）被复制了吗？交换两个 vector 实际上交换的是什么？
//       （提示：示例 [4] 的注释；3.4 节“移动”）
//
// 你的答案：
//   (1)切成了两句话，将3.14中间的.识别成了句尾。Mr.不应该单独一句。
//       改动（补充）：对。整段一共被切成 4 段：[Pi is about 3.14] [today.] [Mr.] [Smith agreed.]
//         Mr. 的问题：它以点结尾，看起来和句尾一模一样。只看字符分不出来，要有一张“缩写表”才行。
//   (2)单词数一样，编号小的在前。不一样。
//       改动（补充）：对，准确地说是“不一定一样”：sort 对一样长的句子不做任何保证，
//         同一个编译器可能碰巧每次都一样，换一个编译器（或者换一个版本）就可能变。示例 [3] 两个编译器就不一样。
//         stable_sort 保证的更一般的说法是：比较函数认为“一样”的元素，保持排序前的先后顺序。
//   (3)没有，移动了数据在哪，用了多少，容量多少这些指针。
//       改动（补充）：对。所以交换两个句子的代价和句子多长无关，一个 1000 个单词的句子也只是挪 3 个指针。
//         对比：如果是复制，每个 string 都要重新分配内存、复制字符。
// ============================================================

int main()
{
    test1();
    test2();
    test3();
}
