// 第 3 章 3.11 用 deque 写一个简单的 RPN（逆波兰）计算器
// 编译：clang++ -std=c++23 -Wall -Wextra rpn.cpp -o rpn.exe
// 运行：
//   ./rpn.exe                              不带参数：运行下面的 [1]~[5] 演示
//   echo "9 6 * 2 3 * +" | ./rpn.exe -     带参数 -：从标准输入读算式，用改进版计算器一步步算
//
// ============================================================
// 背景知识 1：什么是逆波兰表达式（RPN，也叫“后缀表达式”）
//   平时写的 3 + 2 叫“中缀”：运算符在两个数的中间。
//   RPN 把运算符写在两个数的后面：3 2 +
//   好处：不需要括号，也不需要“先乘除后加减”的规则，从左往右一个一个处理就行
//       平时的写法           RPN
//       3 + 2                3 2 +
//       10 - 4               10 4 -
//       (3 + 2) * 3          3 2 + 3 *
//       3 + 2 * 3            3 2 3 * +
//       9 * 6 + 2 * 3        9 6 * 2 3 * +
//
// 背景知识 2：怎么算 —— 用一个“栈”
//   栈（stack）就像一摞盘子：只能往最上面放（push），只能从最上面拿（pop）。后放的先拿出来。
//   从左往右看每一项，只有两条规则：
//     遇到数字   → 放到栈顶
//     遇到运算符 → 从栈顶拿出两个数，算出结果，把结果放回栈顶
//   全部看完，栈里剩下的那一个数就是答案。
//
//   例：9 6 * 2 3 * +          （“栈”这一列：最左边是栈顶）
//     读到   做什么                         栈
//     9      放入                           9
//     6      放入                           6 9
//     *      拿出 6 和 9，算 9 * 6 = 54     54
//     2      放入                           2 54
//     3      放入                           3 2 54
//     *      拿出 3 和 2，算 2 * 3 = 6      6 54
//     +      拿出 6 和 54，算 54 + 6 = 60   60      ← 答案
//
//   注意顺序：先拿出来的是“右边”的数。10 4 - 的意思是 10 - 4：
//     先拿出 4（右边），再拿出 10（左边），算 10 - 4 = 6。顺序反了就成了 4 - 10 = -6（练习 2）
//
// 背景知识 3：用什么容器当栈？
//   这一节用 std::deque（读作 deck，“双端队列”）：两头都能快速地放入和拿出。
//   约定 deque 的 front（最前面）是栈顶：push_front 放入，front 读栈顶，pop_front 拿出。
//   这样从前往后打印 deque，就是“从栈顶到栈底”，和上面表格里的写法一样。
//   deque 内部长什么样、为什么不用 vector 或 std::stack，见 [4] [5]。
// ============================================================
#include <charconv>
#include <cmath>
#include <cstddef>
#include <cctype>
#include <deque>
#include <format>
#include <iostream>
#include <limits>
#include <map>
#include <optional>
#include <print>
#include <ranges>
#include <stack>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

// ------------------------------------------------------------
// 书上的计算器（基本照抄，只修了一个编译错误）。它有好几个问题，[2] 里一个个演示
// ------------------------------------------------------------
class BookRPN
{
public:
    // 处理一项输入：是数字就放入栈；否则当成运算符去算
    double op(const std::string& s)
    {
        if (is_numeric(s))
        {
            double v{std::stod(s, nullptr)}; // stod：把字符串转成 double
            deq_.push_front(v);
            return v;
        }
        else
        {
            return optor(s);
        }
    }

    void clear()
    {
        deq_.clear();
    }

    std::string get_stack_string() const
    {
        std::string s{};
        for (auto v : deq_)
        {
            s += std::format("{} ", v);
        }
        return s;
    }

private:
    std::deque<double> deq_{};
    constexpr static double zero_{0.0};
    constexpr static double inf_{std::numeric_limits<double>::infinity()};

    // 从栈顶拿出两个数，返回 {左边的数, 右边的数}
    std::pair<double, double> pop_get2()
    {
        if (deq_.size() < 2)
        {
            return {zero_, zero_}; // 问题 ③：不够两个数时，假装拿到了两个 0
        }
        double v1{deq_.front()}; // 先拿出来的是右边的数
        deq_.pop_front();
        double v2{deq_.front()};
        deq_.pop_front();
        return {v2, v1};
    }

    // 问题 ①：只认数字和小数点。所以 "-5" 不算数字；"" "." "1.2.3" 却都算数字
    bool is_numeric(const std::string& s)
    {
        for (const char c : s)
        {
            if (c != '.' && !std::isdigit(c))
            {
                return false;
            }
        }
        return true;
    }

    double optor(const std::string& op)
    {
        // 问题 ⑤：这个 map 每次调用 optor 都要重新建一遍（6 个节点、6 个 string）
        std::map<std::string, double (*)(double, double)> opmap{
            {"+", [](double l, double r) { return l + r; }},
            {"-", [](double l, double r) { return l - r; }},
            {"*", [](double l, double r) { return l * r; }},
            {"/", [](double l, double r) { return l / r; }},
            {"^", [](double l, double r) { return std::pow(l, r); }},
            {"%", [](double l, double r) { return std::fmod(l, r); }},
        };
        if (opmap.find(op) == opmap.end()) // 书上第一次写成了 m.end()，编译不过
        {
            return zero_; // 问题 ②：不认识的输入直接忽略，也不告诉调用者
        }
        auto [l, r] = pop_get2();
        if (op == "/" && r == zero_) // 问题 ④：除以 0 一律放入 +∞
        {
            deq_.push_front(inf_);
        }
        else
        {
            deq_.push_front(opmap.at(op)(l, r));
        }
        return deq_.front();
    }
};

// ------------------------------------------------------------
// 改进版计算器
//   op() 返回 std::optional<double>（3.10 练习 3 用过）：
//     成功 → 返回当前栈顶的值
//     失败（输入不认识，或者栈里不够两个数）→ 返回 std::nullopt，而且栈保持原样
// ------------------------------------------------------------
class RPN
{
public:
    std::optional<double> op(std::string_view s)
    {
        if (auto v = parse_number(s))
        {
            stack_.push_front(*v);
            return *v;
        }
        return apply_operator(s);
    }

    void clear()
    {
        stack_.clear();
    }

    std::string stack_string() const
    {
        std::string s;
        for (double v : stack_)
        {
            s += std::format("{} ", v);
        }
        return s;
    }

private:
    std::deque<double> stack_; // front 是栈顶

    // 把整个字符串转成数字；不是一个完整的数字就返回 nullopt
    // std::from_chars（C++17，<charconv>）：从 [开头, 结尾) 读一个数，返回 {读到了哪里, 错误码}
    //   - 认识负数 "-5"、小数 "3.5"、科学计数法 "1e3"
    //   - "" "." "abc" "-" 会失败（错误码不是 errc{}）
    //   - "1.2.3" 只读到 "1.2" 就停了 —— 所以还要检查“读到的位置”是不是字符串的末尾
    //   - 不像 stod 那样抛异常
    static std::optional<double> parse_number(std::string_view s)
    {
        double v{};
        auto [end, ec] = std::from_chars(s.data(), s.data() + s.size(), v);
        if (ec != std::errc{} || end != s.data() + s.size())
        {
            return std::nullopt;
        }
        return v;
    }

    std::optional<double> apply_operator(std::string_view s)
    {
        // 运算符 → 对应的函数。叫“跳转表”：查表就知道该调用哪个函数，不用写一长串 if
        // double (*)(double, double) 是“函数指针”类型：指向一个“接受两个 double、返回 double”的函数。
        //   没有捕获任何东西的 lambda（[] 里是空的）可以自动转换成函数指针，所以能直接放进这个 map
        // static：只在第一次调用时建一次，以后一直用同一个（书上每次都重新建）
        // const：建好以后不会再改
        using Fn = double (*)(double, double);
        static const std::map<std::string_view, Fn> ops{
            {"+", [](double l, double r) { return l + r; }},
            {"-", [](double l, double r) { return l - r; }},
            {"*", [](double l, double r) { return l * r; }},
            {"/", [](double l, double r) { return l / r; }},
            {"^", [](double l, double r) { return std::pow(l, r); }},
            {"%", [](double l, double r) { return std::fmod(l, r); }},
        };

        auto it = ops.find(s);
        if (it == ops.end() || stack_.size() < 2) // 先检查，确定能算了再动栈
        {
            return std::nullopt;
        }
        double right = stack_.front(); // 先拿出来的是右边的数
        stack_.pop_front();
        double left = stack_.front();
        stack_.pop_front();
        stack_.push_front(it->second(left, right)); // it->second 就是那个函数，直接调用
        return stack_.front();
    }
};

// 用书上的计算器跑一串输入，每一步打印栈
void run_book(std::string_view title, const std::vector<std::string>& inputs)
{
    std::println("  {}", title);
    BookRPN rpn;
    for (const auto& s : inputs)
    {
        try
        {
            rpn.op(s);
            std::println("    {:<8}栈：{}", std::format("\"{}\"", s), rpn.get_stack_string());
        }
        catch (const std::exception& e)
        {
            std::println("    {:<8}抛出异常：{}", std::format("\"{}\"", s), e.what());
        }
    }
}

// 用改进版计算器跑一串输入
void run_new(std::string_view title, const std::vector<std::string_view>& inputs)
{
    std::println("  {}", title);
    RPN rpn;
    for (auto s : inputs)
    {
        auto r = rpn.op(s);
        std::println("    {:<8}{}  栈：{}", std::format("\"{}\"", s), r ? "成功" : "拒绝", rpn.stack_string());
    }
}

int main(int argc, char*[])
{
    // 带参数时：从标准输入一项一项地读（按空白分开），用改进版计算器算
    if (argc > 1)
    {
        RPN rpn;
        for (std::string s; std::cin >> s;)
        {
            auto r = rpn.op(s);
            std::println("{:<6}{}栈：{}", s, r ? "" : "（拒绝）", rpn.stack_string());
        }
        return 0;
    }

    // ------------------------------------------------------------
    // 1. 书上的例子：9 6 * 2 3 * +，就是“背景知识 2”那张表
    // ------------------------------------------------------------
    std::println("[1] 书上的例子");
    run_book("9 6 * 2 3 * +", {"9", "6", "*", "2", "3", "*", "+"});

    // ------------------------------------------------------------
    // 2. 书上的计算器有什么问题
    //   ① 判断“是不是数字”的方法太粗糙：
    //        "-5"   ：有个 '-'，不算数字 → 当运算符 → map 里没有 "-5" → 被悄悄忽略。负数根本输不进去
    //        "."    ：算数字 → stod(".") 转不了，抛出 std::invalid_argument 异常，程序没接住就会直接终止
    //        "1.2.3"：算数字 → stod 读到 1.2 就停了，后面的 ".3" 悄悄丢掉
    //   ② 不认识的输入（比如 "x"）直接忽略，调用者不知道出错了
    //   ③ 栈里不够两个数时，pop_get2 返回 {0, 0}，于是 "5 +" 算出了 0 + 0（思考题 (1)）
    //   ④ 除以 0 时一律放入 +∞。其实 double 自己就会处理：1 / 0 = inf，-1 / 0 = -inf，0 / 0 = nan
    //      （nan 是“不是一个数”，有时打印成 -nan，也是 nan）。
    //      书上这样特殊处理，反而把 -1 / 0 算成了 +∞；而 5 % 0 又没管，得到 nan
    //   ⑤ 每次算运算符都重新建一个 map（不影响结果，只是浪费）
    //   另外书上的 std::isdigit(c)：char 是负数时（比如中文的 UTF-8 字节）是未定义行为，
    //   要写成 std::isdigit(static_cast<unsigned char>(c))
    // ------------------------------------------------------------
    std::println("\n[2] 书上计算器的问题");
    run_book("① 负数和奇怪的数字", {"-5", "1.2.3", "."});
    run_book("② ③ 不认识的输入、不够两个数", {"5", "x", "+"});
    run_book("④ 除以 0（书上输不了 -1，用 0 1 - 算出 -1）", {"0", "1", "-", "0", "/", "0", "0", "/", "5", "0", "%"});

    // ------------------------------------------------------------
    // 3. 改进版：同样的输入
    //   用 from_chars 判断数字（负数可以输入了，"1.2.3" 和 "." 被拒绝）
    //   出错时返回 nullopt、栈不变，调用者能知道哪一步出了问题
    //   除以 0 交给 double 自己处理（-1 / 0 是 -inf）
    // ------------------------------------------------------------
    std::println("\n[3] 改进版");
    run_new("① 负数和奇怪的数字", {"-5", "1.2.3", "."});
    run_new("② ③ 不认识的输入、不够两个数", {"5", "x", "+"});
    run_new("④ 除以 0", {"0", "1", "-", "0", "/", "0", "0", "/"});
    run_new("10 4 -（顺序）", {"10", "4", "-"});

    // ------------------------------------------------------------
    // 4. deque 是什么样的
    //   常用操作：push_front / push_back 放入，pop_front / pop_back 拿出，
    //            front / back 读两头，d[i] 按下标访问，size，begin/end 遍历
    //
    //   vector 是一整块连续内存：
    //       [ 1 2 3 4 5 6 7 8 ]
    //     往后面加：有空位就直接放；没空位就分配一块更大的新内存，把所有元素搬过去，旧内存释放
    //     往前面加：所有元素都要往后挪一格 —— 所以 vector 干脆没有 push_front
    //
    //   deque 是很多个固定大小的“块”，再加一张“目录”记着每一块在哪：
    //       目录： [ 块0  块1  块2  块3 ]
    //                 ↓    ↓    ↓    ↓
    //       块：  [ _ _ 1 2 ] [ 3 4 5 6 ] [ 7 8 9 10 ] [ 11 _ _ _ ]
    //     push_front：第一块前面还有空位就直接放；没有就新分配一块，登记到目录最前面
    //     push_back ：同理，往最后一块放
    //     已经在里面的元素一个都不用搬 —— 所以两头放入都很快，而且元素的地址不会变
    //     d[i]：先算出在第几块、块里的第几个，两步就找到了。也很快，只是比 vector 多绕一步
    //   注意：deque 不是一整块连续内存（有的中文书说它是连续的，不对）。
    //   块有多大由标准库决定：下面数一数 10000 个 int 里有几处“断开”，两个编译器不一样
    //     clang（libc++）每块 4096 字节 = 1024 个 int → 9 处；g++（libstdc++）每块 512 字节 = 128 个 int → 78 处
    // ------------------------------------------------------------
    std::println("\n[4] deque 的内存");
    std::deque<int> d{1, 2, 3};
    d.push_front(0);
    d.push_back(4);
    std::println("d = {}，front = {}，back = {}，d[2] = {}", d, d.front(), d.back(), d[2]);
    d.pop_front();
    d.pop_back();
    std::println("pop_front、pop_back 之后：{}", d);

    // 地址会不会变
    std::vector<int> v{42};
    const int* pv = &v[0];
    for (int i = 0; i < 1000; ++i)
    {
        v.push_back(i);
    }
    std::println("vector 加了 1000 个元素后，42 还在原来的地址吗？{}", pv == &v[0]);

    std::deque<int> dq{42};
    const int* pd = &dq[0];
    for (int i = 0; i < 1000; ++i)
    {
        dq.push_back(i);
        dq.push_front(i);
    }
    std::println("deque 前后各加了 1000 个元素后，42 还在原来的地址吗？{}（它现在是 dq[1000]）", pd == &dq[1000]);

    // 数一数有几处不连续：相邻两个元素的地址不是紧挨着的，说明换到了下一块
    std::deque<int> big(10000);
    int gaps = 0;
    for (std::size_t i = 0; i + 1 < big.size(); ++i)
    {
        if (&big[i] + 1 != &big[i + 1])
        {
            ++gaps;
        }
    }
    std::println("10000 个 int 的 deque 里有 {} 处断开（vector 永远是 0 处）", gaps);

    // ------------------------------------------------------------
    // 5. 当栈用，为什么不用 std::stack 或 vector？
    //   std::stack：专门的栈，只有 push、pop、top、size、empty 这几个操作，不能遍历。
    //     想像这个计算器一样“打印整个栈”就做不到（它内部默认就是用 deque 存的）
    //   std::vector：push_back 放入、back 读栈顶、pop_back 拿出 —— 栈顶在后面，当栈用完全没问题，
    //     而且通常比 deque 还快。只是打印时要倒着遍历才是“从栈顶到栈底”
    //   所以只当栈用的话，vector 和 deque 都可以。deque 真正的长处是“两头都要用”的场合，
    //   比如排队：从后面进、从前面出（练习 3）
    // ------------------------------------------------------------
    std::println("\n[5] 其他的栈");
    std::stack<double> st;
    st.push(9);
    st.push(6);
    std::println("std::stack：top() = {}，size() = {}（没有 begin/end，不能遍历）", st.top(), st.size());

    std::vector<double> vs;
    vs.push_back(9);
    vs.push_back(6);
    std::println("vector 当栈：back() = {}，倒着打印：{}", vs.back(), vs | std::views::reverse);
}
