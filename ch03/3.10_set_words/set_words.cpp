// 第 3 章 3.10 使用 set 对输入进行排序和筛选（istream_iterator + inserter）
// 编译：clang++ -std=c++23 -Wall -Wextra set_words.cpp -o set_words.exe
// 运行：
//   ./set_words.exe                       不带参数：[1] 用程序里写好的一句话代替键盘输入
//   echo "a a b this that" | ./set_words.exe -     带参数 -：[1] 从标准输入读（书上的用法）
#include <algorithm>
#include <cstddef>
#include <iostream>
#include <iterator>
#include <print>
#include <ranges>
#include <set>
#include <sstream>
#include <string>
#include <vector>

using input_it = std::istream_iterator<std::string>;

// 书上的写法，只是把 cin 换成了参数：任何输入流都行（cin、文件、字符串流）
std::set<std::string> read_words(std::istream& in)
{
    std::set<std::string> words;
    input_it it{in};
    input_it end{};
    std::copy(it, end, std::inserter(words, words.end()));
    return words;
}

// 不区分大小写的“小于”（第 [4] 部分）
struct CaseLess
{
    static char lower(char c)
    {
        return c >= 'A' && c <= 'Z' ? static_cast<char>(c - 'A' + 'a') : c;
    }

    bool operator()(const std::string& a, const std::string& b) const
    {
        return std::ranges::lexicographical_compare(a, b, {}, lower, lower);
    }
};

// 数一数“小于”被调用了多少次（第 [5] 部分）
struct CountingLess
{
    static inline long long calls = 0;

    bool operator()(int a, int b) const
    {
        ++calls;
        return a < b;
    }
};

void compare_cost(std::size_t n, int distinct)
{
    // 生成 n 个数，只有 distinct 种不同的值（乱序）
    std::vector<int> data(n);
    for (std::size_t i = 0; i < n; ++i)
    {
        data[i] = static_cast<int>(i * 7919 % static_cast<std::size_t>(distinct));
    }

    CountingLess::calls = 0;
    std::set<int, CountingLess> s(data.begin(), data.end());
    long long set_calls = CountingLess::calls;

    CountingLess::calls = 0;
    std::vector<int> v = data;
    std::ranges::sort(v, CountingLess{});
    auto [first, last] = std::ranges::unique(v);
    v.erase(first, last);
    long long vec_calls = CountingLess::calls;

    std::println("  {:>6} 个数，{:>6} 种：set 最多存 {:>6} 个，比较 {:>7} 次；vector 先存全部 {:>6} 个，比较 {:>7} 次",
                 n, distinct, s.size(), set_calls, data.size(), vec_calls);
}

int main(int argc, char*[])
{
    // ------------------------------------------------------------
    // 1. 书上的例子：读入一串单词，去掉重复的，按顺序打印
    //    set 的特点：元素就是键（3.7 思考题 (3)），自动排好序，重复的插不进去
    // ------------------------------------------------------------
    std::println("[1] 书上的例子");
    std::set<std::string> words;
    if (argc > 1)
    {
        words = read_words(std::cin);
    }
    else
    {
        std::istringstream in{"a a a b c this that this foo foo foo"};
        words = read_words(in);
    }
    for (const std::string& w : words)
    {
        std::print("{} ", w);
    }
    std::println("");

    // ------------------------------------------------------------
    // 2. 这几行在做什么
    //    istream_iterator<string> it{in}：把输入流包装成迭代器。*it 是当前读到的单词，++it 读下一个
    //      （内部就是 in >> word，所以按空白分词）
    //    istream_iterator<string> end{}：默认构造的是“流结束迭代器”。读失败（输入结束）时，it 会变得
    //      和 end 相等 —— 书上说“这个迭代器将与等价于 cin 迭代器”，说反了也说乱了：是 it 变得等于 end
    //    inserter(words, words.end())：一个“输出迭代器”，往它身上赋值 = 调用 words.insert(位置, 值)
    //    std::copy 把 [it, end) 一个个赋值给 inserter，于是每个单词都被 insert 进 set
    //
    //    其实不需要 copy + inserter，set 自己就能接受一对迭代器；C++23 还能一句话写完
    // ------------------------------------------------------------
    std::println("\n[2] 更简单的写法");
    std::istringstream in2{"pear apple pear fig"};
    std::set<std::string> w2;
    w2.insert(input_it{in2}, input_it{}); // 范围插入
    std::println("insert(first, last)：{}", w2);

    std::istringstream in3{"pear apple pear fig"};
    auto w3 = std::views::istream<std::string>(in3) | std::ranges::to<std::set>(); // C++23
    std::println("views::istream | ranges::to：{}", w3);

    // ------------------------------------------------------------
    // 3. istream_iterator 的几个细节（书上没讲）
    //    (a) 构造的时候就已经读了第一个值 —— 从 cin 构造时，程序会停在这一行等你输入
    //    (b) 什么类型都能读，只要能用 >> 读：istream_iterator<int> 读整数
    //    (c) 遇到读不了的内容（比如在读 int 时遇到 x），流进入失败状态，迭代器就等于 end 了，
    //        后面的内容不会再读（思考题 (1)）
    // ------------------------------------------------------------
    std::println("\n[3] 读整数");
    std::istringstream nums{"5 3 5 1 x 4 2"};
    std::set<int> ns(std::istream_iterator<int>{nums}, std::istream_iterator<int>{});
    std::println("从 \"5 3 5 1 x 4 2\" 读到：{}", ns);
    nums.clear(); // 清掉失败状态，才能接着读
    std::string rest;
    nums >> rest;
    std::println("clear() 之后接着读，下一个单词是：{}", rest);

    // ------------------------------------------------------------
    // 4. “排好序”是按什么排的？（书上没讲）
    //    std::string 的 < 是逐个字符比较字符编码：大写字母（65~90）都排在小写字母（97~122）前面，
    //    标点也算单词的一部分（因为只按空白分词）
    //    想要别的顺序，就给 set 第二个模板参数传一个“小于”（和 3.9 传哈希一样，传的是类型）
    //    注意：用 CaseLess 时，set 认为 "Apple" 和 "apple" 是同一个元素（思考题 (2)）
    // ------------------------------------------------------------
    std::println("\n[4] 排序规则");
    const char* text = "banana Apple cherry apple Banana cherry, Cherry";
    std::istringstream in4{text};
    std::set<std::string> plain(input_it{in4}, input_it{});
    std::println("默认（区分大小写）：{}", plain);

    std::istringstream in5{text};
    std::set<std::string, CaseLess> nocase(input_it{in5}, input_it{});
    std::println("CaseLess：        {}", nocase);
    std::println("CaseLess 里 contains(\"BANANA\") = {}", nocase.contains("BANANA"));

    // ------------------------------------------------------------
    // 5. 只想“去重 + 排序”，不一定要用 set
    //    另一种常见做法：全部放进 vector，sort，再 unique + erase（3.3 节的套路）
    //    (a) 比较次数：两者是同一个量级（都是 n × log 级别）。谁多谁少取决于标准库 ——
    //        clang（libc++）上 vector 少，g++（libstdc++）上 set 少，正好相反
    //    (b) 内存：set 只存不同的元素；vector 要先把 n 个全存下来。重复很多、数据又大时，这是 set 的优势
    //    (c) set 每个元素单独分配一个树节点，vector 是一整块连续内存 —— 所以没什么重复时，vector 通常更快
    //    不需要有序，就用 unordered_set（3.9 节）
    //
    //    最后两行：按从小到大的顺序插入 0~99999。insert(值) 每次都从树根往下找；
    //    insert(s.end(), 值) 多给了一个“位置提示”：提示对了（新值正好该放在末尾），只要比较一两次（思考题 (2)）
    // ------------------------------------------------------------
    std::println("\n[5] set 和 vector + sort + unique（数“小于”调用的次数）");
    compare_cost(100000, 100);
    compare_cost(100000, 100000);

    std::set<int, CountingLess> no_hint;
    std::set<int, CountingLess> with_hint;
    CountingLess::calls = 0;
    for (int i = 0; i < 100000; ++i)
    {
        no_hint.insert(i);
    }
    long long no_hint_calls = CountingLess::calls;
    CountingLess::calls = 0;
    for (int i = 0; i < 100000; ++i)
    {
        with_hint.insert(with_hint.end(), i);
    }
    std::println("  有序插入 100000 个：insert(值) 比较 {} 次，insert(end(), 值) 比较 {} 次",
                 no_hint_calls, CountingLess::calls);
}
