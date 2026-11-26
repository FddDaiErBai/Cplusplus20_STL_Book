// 第 4 章 4.8 哨兵 —— 练习
// 编译：clang++ -std=c++23 -Wall -Wextra -Wpedantic sentinel_exercise.cpp -o sentinel_exercise.exe
//       g++     -std=c++23 -Wall -Wextra -Wpedantic sentinel_exercise.cpp -o sentinel_exercise.exe -lstdc++exp
// 运行时检查：补完 TODO，编译运行，所有行都显示“通过”就说明做对了。
// 先看示例 sentinel.cpp 的 [4]（迭代器 + 空的哨兵类型）、[5]（指针 + 哨兵 + subrange）、[6]（其它结束记号）。
#include <algorithm>
#include <cstddef>
#include <functional>
#include <iterator>
#include <print>
#include <ranges>
#include <string>
#include <string_view>
#include <vector>

void check(bool ok, std::string_view name)
{
    std::println("{} {}", ok ? "通过" : "失败", name);
}

using V = std::vector<int>;

// ============================================================
// 练习 1：第一行（照着示例 [5]）
//   first_line(text)：text 是 C 字符串，返回第一行的内容（不含 '\n'）。没有 '\n' 就是整个字符串
//     "abc\ndef" → "abc"      "abc" → "abc"      "\nxyz" → ""      "" → ""
//   TODO：
//     ① 写好 line_end 的 ==：p 指向 '\n' 或者 '\0' 时就是“到头了”（两个都要判断，想一想为什么）
//     ② first_line：std::ranges::subrange(text, line_end{}) | std::ranges::to<std::string>()，一行就够
//   line_end 结构体本身已经写好了（空的）。
// ============================================================
struct line_end
{
};

bool operator==(const char* p, line_end)
{
    return *p == '\n' || *p == '\0';
}

std::string first_line(const char* text)
{
    return std::ranges::subrange(text, line_end{}) | std::ranges::to<std::string>();
}

void test_exercise1()
{
    check(std::sentinel_for<line_end, const char*>, "练习 1：sentinel_for<line_end, const char*>");
    check(first_line("abc\ndef") == "abc", "练习 1：first_line(\"abc\\ndef\") → \"abc\"");
    check(first_line("abc") == "abc", "练习 1：没有换行 → 整个字符串");
    check(first_line("\nxyz").empty() && first_line("").empty(), "练习 1：第一行是空的 / 整个是空的");
    check(first_line("hello world\n\n") == "hello world", "练习 1：后面有多个换行");
    // 改动（补充）：第二行不会被读到；一行里有空格、逗号也照样读
    check(first_line("a, b\nc, d") == "a, b", "练习 1：只取第一行，第二行不读");
}

// ============================================================
// 练习 2：链表（照着示例 [4]）
//   Node 是一个最简单的单向链表节点：一个值 + 指向下一个节点的指针，最后一个节点的 next 是 nullptr。
//   只拿到头节点 head，不走一遍就不知道有几个节点 —— 和 C 字符串一样，用哨兵：走到 nullptr 就停。
//
//     Chain{head} 是范围，begin() 返回 Chain::iterator{head}，end() 返回 chain_end{}（空的哨兵类型）
//     iterator 里只存一个 Node* p_：
//       *it        ：返回 p_->value 的引用（int&），这样可以通过迭代器修改节点里的值
//       ++it       ：p_ = p_->next
//       it == chain_end{} ：p_ == nullptr
//
//   TODO（都在 Chain::iterator 里）：
//     ① 类型别名：iterator_concept、iterator_category 都是 forward；value_type = int；difference_type = std::ptrdiff_t
//     ② 默认构造函数
//     ③ int& operator*() const（已经写好了：返回 int&，所以 *it = 50 改的是节点里的值）
//     ④ 前置 ++、后置 ++
//     ⑤ 迭代器之间的 ==（= default）
//     ⑥ 和哨兵比较的 ==（现在永远返回 true，所以什么都遍历不到）
//   chain_end、Chain 本身、begin()、end() 已经写好了。下面的 helper 用 if constexpr 判断能不能用，helper 不用改。
// ============================================================
struct Node
{
    int value{};
    Node* next{};
};

struct chain_end
{
};

class Chain
{
    Node* head_{};

public:
    explicit Chain(Node* head)
        : head_{head}
    {
    }

    class iterator
    {
        Node* p_{};

    public:
        using iterator_concept = std::forward_iterator_tag;
        using iterator_category = std::forward_iterator_tag;
        using value_type = int;
        using difference_type = std::ptrdiff_t;

        iterator() = default;

        explicit iterator(Node* p)
            : p_{p}
        {
        }

        int& operator*() const // ③ 已经写好了
        {
            return p_->value;
        }

        iterator& operator++()
        {
            p_ = p_->next;
            return *this;
        }

        iterator operator++(int)
        {
            iterator old = *this;
            ++*this;
            return old;
        }

        bool operator==(const iterator&) const = default;

        bool operator==(chain_end) const
        {
            return p_ == nullptr;
        }
    };

    iterator begin() const
    {
        return iterator{head_};
    }

    chain_end end() const
    {
        return {};
    }
};

// 用 range-for 收集，最多 100 个（防止停不下来）
V collect(const Chain& c)
{
    V out;
    for (int x : c)
    {
        if (out.size() == 100)
        {
            break;
        }
        out.push_back(x);
    }
    return out;
}

// 新的一套：ranges::distance
template <typename R>
long long length(const R& r)
{
    if constexpr (std::ranges::forward_range<const R>)
    {
        return std::ranges::distance(r);
    }
    return -1;
}

// 新的一套：ranges::fold_left 求和
template <typename R>
int sum(const R& r)
{
    if constexpr (std::ranges::forward_range<const R>)
    {
        return std::ranges::fold_left(r, 0, std::plus{});
    }
    return -1;
}

// 新的一套：找到第一个 x，改成 y（通过迭代器修改节点）
template <typename R>
bool replace_first(const R& r, int x, int y)
{
    if constexpr (std::ranges::forward_range<const R>)
    {
        auto it = std::ranges::find(r, x);
        if (it == std::ranges::end(r))
        {
            return false;
        }
        *it = y;
        return true;
    }
    return false;
}

// 新的一套：views::filter 取偶数
template <typename R>
V evens(const R& r)
{
    if constexpr (std::ranges::forward_range<const R>)
    {
        return r | std::views::filter([](int x) { return x % 2 == 0; }) | std::ranges::to<V>();
    }
    return {};
}

// 后置 ++ 返回旧值：old = it++ 之后，*old 是第一个，*it 是第二个
template <typename R>
bool postfix_ok(const R& r, int first, int second)
{
    if constexpr (std::ranges::forward_range<const R>)
    {
        auto it = r.begin();
        auto old = it++;
        return *old == first && *it == second;
    }
    (void)r;
    (void)first;
    (void)second;
    return false;
}

void test_exercise2()
{
    // 3 → 8 → 5 → 2 → nullptr
    Node n4{2, nullptr};
    Node n3{5, &n4};
    Node n2{8, &n3};
    Node n1{3, &n2};
    Chain c{&n1};

    check(collect(c) == V{3, 8, 5, 2}, "练习 2：range-for → 3 8 5 2");
    check(std::forward_iterator<Chain::iterator> && std::sentinel_for<chain_end, Chain::iterator>,
          "练习 2：forward_iterator，chain_end 是它的哨兵");
    check(!std::ranges::sized_range<Chain>, "练习 2：不是 sized_range（不知道自己有多长）");
    check(length(c) == 4 && sum(c) == 18, "练习 2：ranges::distance → 4，fold_left 求和 → 18");
    check(evens(c) == V{8, 2}, "练习 2：views::filter 偶数 → 8 2");
    check(replace_first(c, 5, 50) && n3.value == 50 && !replace_first(c, 99, 0), "练习 2：通过迭代器把 5 改成 50（改的是节点本身）");

    Chain empty{nullptr};
    check(collect(empty).empty() && length(empty) == 0 && sum(empty) == 0, "练习 2：空链表（head 是 nullptr）");

    check(postfix_ok(c, 3, 8), "练习 2：后置 ++ 返回旧值");

    // 改动（补充）：思考题 1(a) 说的“空结构体当哨兵的好处”：Chain::iterator 只能和 chain_end 比较，
    //   拿一个 nullptr、一个 int 去比都编译不过（书上 const char 哨兵的 it != 'x' 就能编译）
    check(!std::equality_comparable_with<Chain::iterator, std::nullptr_t> && !std::equality_comparable_with<Chain::iterator, int>,
          "练习 2：迭代器不能和 nullptr、int 比较，只能和 chain_end 比较");
}

// ============================================================
// 练习 3：找 bug（两个函数各有一个 bug，每个只改一行。只改这两处，别的地方不要动）
//   first_field(s)：s 里第一个逗号之前的内容。没有逗号就是整个字符串：
//     "ab,cd" → "ab"      "abc" → "abc"
//   first_stmt(s) ：s 里第一个分号之前的内容。没有分号就是整个字符串：
//     "x=1;y=2" → "x=1"   "x=1" → "x=1"
//     运行后看到 sentinel_for 那一项失败、first_stmt 什么都没返回。
//   提示：first_field —— 没有逗号的时候，迭代器会停在哪里？
//         first_stmt  —— 传进来的是 const char*，== 的第一个参数是什么类型？const char* 能变成它吗？
//   测试里没有直接用 "abc"，而是用了 buf：{'a','b','c','\0','x',','}。'\0' 后面还有字符，
//     这样 bug 的效果是确定的（多读到 "\0x"），不会读到数组外面去（那是未定义行为）
// ============================================================
struct comma
{
};

bool operator==(const char* p, comma)
{
    return *p == ',' || *p == '\0';
}

std::string first_field(const char* s)
{
    return std::ranges::subrange(s, comma{}) | std::ranges::to<std::string>();
}

struct semicolon
{
};

bool operator==(const char* p, semicolon)
{
    return *p == ';' || *p == '\0';
}

// 从 s 开始，一直读到哨兵 S 说“到头了”。S 不是 const char* 的哨兵时返回空字符串（所以这个文件现在也能编译）
template <typename S>
std::string read_until(const char* s)
{
    if constexpr (std::sentinel_for<S, const char*>)
    {
        return std::ranges::subrange(s, S{}) | std::ranges::to<std::string>();
    }
    (void)s;
    return "";
}

std::string first_stmt(const char* s)
{
    return read_until<semicolon>(s);
}

void test_exercise3()
{
    check(first_field("ab,cd") == "ab" && first_field(",x").empty(), "练习 3：first_field(\"ab,cd\") → \"ab\"");
    const char buf[]{'a', 'b', 'c', '\0', 'x', ','};
    check(first_field(buf) == "abc", "练习 3：first_field 没有逗号 → 整个字符串 \"abc\"");

    check(std::sentinel_for<semicolon, const char*>, "练习 3：sentinel_for<semicolon, const char*>");
    check(first_stmt("x=1;y=2") == "x=1" && first_stmt("x=1") == "x=1" && first_stmt("").empty(),
          "练习 3：first_stmt(\"x=1;y=2\") → \"x=1\"，没有分号 → 整个");
    // 改动（补充）：分号、逗号在最前面
    check(first_stmt(";x").empty() && first_field(",").empty() && first_field("").empty(), "练习 3：开头就是分隔符 / 空的");
}

int main()
{
    test_exercise1();
    std::println("");
    test_exercise2();
    std::println("");
    test_exercise3();
}

// ============================================================
// 思考题（每个小问都单独回答一行，每一行都要说“为什么”）
// （1）关于哨兵类型
//   (a) 书上的哨兵类型是 const char，示例 [3] 说 it != 'x' 也能编译。改用一个空的结构体（cstr_end、chain_end）有什么好处？
// 你的答案：什么都不用存，只是告诉是不是到末尾了。
// 改动（补充）：这说的是“空结构体是什么”，题目问的是“比 const char 好在哪”。好处是：只有它能当哨兵。
//   哨兵是 const char 时，任何能变成 char 的东西（'x'、65、一个 int 变量）都能和迭代器比较，
//   写错了编译器也不报错，结果还和 it != '\0' 一样，bug 藏起来了。
//   cstr_end、chain_end 是专门的类型，别的类型都变不成它，写错了就是编译错误。
//   练习 2 里新加了一个检查：Chain::iterator 和 nullptr、int 都不能比较。
//   (b) chain_end 里什么都没存，operator==(chain_end) 的参数连名字都没写。那“到头了没有”这个信息存在哪里？
// 你的答案：什么都不存只是用这个类型，告诉是不是到末尾了。
// 改动（补充）：问的是“信息存在哪里”，答案是：存在迭代器里。
//   chain_end 只是让编译器选中 operator==(chain_end) 这个函数，真正的判断是函数体里的 p_ == nullptr，
//   用的是迭代器自己的 p_。哨兵没有值，所以参数也不用起名字。
//   C 字符串也一样：判断的是迭代器（指针）指向的字符是不是 '\0'。
//
// （2）关于长度
//   (a) 练习 2 的 Chain 为什么不提供 size()？ranges::distance(c) 是怎么算出 4 的，花的时间和节点个数是什么关系？
// 你的答案：哨兵范围不知道自己有多长。从头到尾一个一个数。O(n)
// 改动（补充）：对。补一句：size() 按约定应该是 O(1) 的（sized_range 的要求），Chain 做不到，
//   所以干脆不提供，让用的人看到 distance 就知道要走一遍。真想要 O(1) 的 size，就得在 Chain 里另外存一个计数。
//   (b) 练习 1 的 line_end 为什么 '\n' 和 '\0' 都要判断？只判断 '\n'，对 "abc" 会发生什么？（练习 3 的 first_field 就是这个 bug）
// 你的答案：停不下来。
// 改动（补充）：还要说“停不下来会怎样”：只判断 '\n'，"abc" 里没有 '\n'，走到 '\0' 也不停，
//   继续往 '\0' 后面读。字符串字面量后面是什么内存不知道，越界读是未定义行为：
//   可能读到一堆乱码碰巧遇到 '\n' 停下，也可能读到不能读的地方崩溃。
//   所以练习 3 的测试用的是 buf {'a','b','c','\0','x',','}：在数组里提前放了一个 ','，bug 的效果才是确定的。
//
// （3）关于示例 [5][6] 的写法
//   (a) operator==(const char* p, nul) 为什么必须写在类外面，不能写成 nul 或者指针的成员函数？
//       （提示：写成 nul 的成员函数 bool operator==(const char*) const 其实也能用 —— 为什么？想想 C++20 会把 p == nul{} 怎么改写，4.3 节）
// 你的答案：const char*是内置类型，无法为它定义成员函数。
// 改动（补充）：对，指针不是类，不能有成员函数。提示里的后半问没答：写成 nul 的成员函数也能用，为什么？
//   struct nul { bool operator==(const char* p) const { ... } };   这个函数处理的是 nul{} == p（nul 在左边）。
//   而 subrange 内部写的是 it == sentinel（指针在左边）。C++20 找不到“指针在左边”的版本时，
//   会把参数对调，试 sentinel == it，于是用上了成员函数（4.3 节说的改写规则：b == a → a == b）。
//   我试过，两个编译器都能编译运行。C++17 没有这个改写，就必须写在类外面。
//   (b) 练习 3 的 first_stmt：operator==(char* p, semicolon) 的 char* 改成 const char* 就好了。
//       为什么 const char* 不能转成 char*，反过来 char* 却能转成 const char*？（想想 const 是在保护什么）
// 你的答案：不能修改指针的指向，但是可以修改指针指向的内容。
// 改动（补充）：说反了。const char* p 是“指向 const char 的指针”：
//   *p = 'X';    不能改指向的内容。clang：error: read-only variable is not assignable
//   p = "xyz";   可以改指针的指向
//   “不能改指向”的写法是 char* const p（const 写在 * 的右边）。读法：从右往左读，const char* 读作“指针，指向 const char”。
//   为什么 const char* 不能变成 char*：变了以后就能通过它修改原本说好不能改的内容，const 的保护就没了：
//     const char* s = "abc";  char* p = s;  *p = 'X';   // 字符串字面量是只读的，改它是未定义行为
//     clang：error: cannot initialize a variable of type 'char *' with an lvalue of type 'const char *'
//     g++  ：error: invalid conversion from 'const char*' to 'char*'
//   反过来 char* 变 const char* 只是“多加一道限制”，不会让谁能改原本不能改的东西，所以允许。
//   first_stmt 传进来的是 const char*，== 只接受 char*，转不过去，于是 sentinel_for 是 false。
//
// ============================================================
