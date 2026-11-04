// 第 3 章 3.11 用 deque 写一个简单的 RPN 计算器 —— 练习
// 编译：clang++ -std=c++23 -Wall -Wextra rpn_exercise.cpp -o rpn_exercise.exe
// 运行时检查（deque 不能在编译时使用）：补完 TODO，编译运行，所有行都显示“通过”就说明做对了。
// 先看示例 rpn.cpp 开头的“背景知识”：RPN 是什么、怎么用栈来算。
#include <cstddef>
#include <charconv>
#include <deque>
#include <optional>
#include <print>
#include <ranges>
#include <string_view>
#include <vector>

void check(bool ok, std::string_view name)
{
    std::println("{} {}", ok ? "通过" : "失败", name);
}

// ------------------------------------------------------------
// 两个现成的工具函数，直接用就行
// ------------------------------------------------------------

// 把整个字符串转成数字；不是一个完整的数字（比如 "+"、"x"、"1.2.3"、""）就返回 nullopt
// （和示例里 RPN::parse_number 一样）
std::optional<double> parse_number(std::string_view s)
{
    double v{};
    auto [end, ec] = std::from_chars(s.data(), s.data() + s.size(), v);
    if (ec != std::errc{} || end != s.data() + s.size())
    {
        return std::nullopt;
    }
    return v;
}

// 按空格把算式切成一项一项："3 2  +" → {"3", "2", "+"}（多余的空格会被跳过）
std::vector<std::string_view> tokens(std::string_view s)
{
    std::vector<std::string_view> result;
    for (auto part : s | std::views::split(' '))
    {
        if (!part.empty())
        {
            result.emplace_back(part.begin(), part.end());
        }
    }
    return result;
}

// ============================================================
// 练习 1：算出一整个 RPN 算式
//   eval_rpn("3 2 + 3 *") 返回 15。支持 + - * / 四种运算符，数字可以是负数和小数。
//   算式有错时返回 std::nullopt，有错指的是：
//     - 出现了既不是数字、也不是 + - * / 的东西（比如 "x"）
//     - 遇到运算符时，栈里不够两个数（比如 "1 +"）
//     - 全部算完，栈里剩下的不是正好一个数（比如 "2 3" 剩两个；"" 一个都没有）
//   要求：用 std::deque<double> 当栈
//   提示：
//     - for (auto t : tokens(expr)) 一项一项地处理；parse_number(t) 判断是不是数字
//     - 运算符只有一个字符，t 是 "+" 时 t == "+" 为 true（string_view 可以直接和 "+" 比较）
//     - 先拿出来的是右边的数（示例开头“背景知识 2”最后一段）
// ============================================================
std::optional<double> eval_rpn(std::string_view expr)
{
    std::deque<double> stack{};
    for (const auto& i : tokens(expr))
    {
        if (auto v = parse_number(i))
        {
            stack.push_front(*v);
            continue;
        }
        if (stack.size() < 2)
        {
            return std::nullopt;
        }
        const double right{stack.front()};
        stack.pop_front();
        const double left{stack.front()};
        stack.pop_front();
        if (i == "+")
        {
            stack.push_front(left + right);
        }
        else if (i == "-")
        {
            stack.push_front(left - right);
        }
        else if (i == "*")
        {
            stack.push_front(left * right);
        }
        else if (i == "/")
        {
            stack.push_front(left / right);
        }
        else // 改动：不认识的运算符要报错。原来这里什么都不做，两个数被拿走就丢了
        {
            return std::nullopt;
        }
    }
    if (stack.size() != 1)
    {
        return std::nullopt;
    }
    return stack.front();
}

// 改动（补充）：整体思路完全对，顺序（先拿 right）也对。但有一个漏洞：遇到 "x" 这种不认识的东西时，
//   if / else if 四个分支都不成立，什么都没做 —— 可是两个数已经 pop 掉了，于是它们悄悄消失，循环继续。
//   原来的测试 "2 x +" 之所以通过，是因为走到 x 时栈里只有 1 个数，被 size() < 2 那里拦下了，是碰巧。
//   换成 "5 1 2 x"：走到 x 时栈是 2 1 5，拿走 2 和 1 之后丢掉，栈里剩一个 5，最后返回了 5，而不是 nullopt。
//   我在 test1 里加了这一项，你原来的代码会显示“失败”。修法：最后加一个 else { return std::nullopt; }。
//   更稳妥的写法是“先确认是认识的运算符，再动栈”，像示例里的 RPN::apply_operator 那样先 find，再 pop。

void test1()
{
    check(eval_rpn("3 2 + 3 *") == 15.0, "练习 1：(3 + 2) * 3");
    check(eval_rpn("9 6 * 2 3 * +") == 60.0, "练习 1：9 * 6 + 2 * 3");
    check(eval_rpn("10 4 -") == 6.0, "练习 1：10 - 4（顺序）");
    check(eval_rpn("1 2 /") == 0.5, "练习 1：1 / 2（顺序）");
    check(eval_rpn("5 1 2 + 4 * + 3 -") == 14.0, "练习 1：5 + (1 + 2) * 4 - 3");
    check(eval_rpn("-3 4 *") == -12.0, "练习 1：负数");
    check(eval_rpn("  7  ") == 7.0, "练习 1：只有一个数");
    check(!eval_rpn("2 3").has_value(), "练习 1：剩下两个数");
    check(!eval_rpn("1 +").has_value(), "练习 1：不够两个数");
    check(!eval_rpn("2 x +").has_value(), "练习 1：不认识的 x");
    check(!eval_rpn("5 1 2 x").has_value(), "练习 1：不认识的 x（栈里有足够的数时）"); // 改动：新加的测试
    check(!eval_rpn("").has_value(), "练习 1：空算式");
}

// ============================================================
// 练习 2：找 bug —— 运算顺序
//   apply_op 把栈顶的两个数拿出来，按 op 计算，结果放回栈顶。st 的 front 是栈顶。
//   测试里的 Stack{4, 10} 表示 front 是 4、后面是 10，也就是“先放入了 10，再放入了 4”，
//   相当于算式 “10 4” 之后的栈。
//   运行后，+ 和 * 通过了，- 和 / 却失败了。找出原因并修改（思考题 (2) 会问为什么 + 和 * 没事）
// ============================================================
using Stack = std::deque<double>;

bool apply_op(Stack& st, char op)
{
    if (st.size() < 2)
    {
        return false;
    }
    double a = st.front();
    st.pop_front();
    double b = st.front();
    st.pop_front();
    switch (op)
    {
        case '+':
            st.push_front(b + a);
            break;
        case '-':
            st.push_front(b - a);
            break;
        case '*':
            st.push_front(b * a);
            break;
        case '/':
            st.push_front(b / a);
            break;
        default:
            st.push_front(b); // 不认识的运算符：把两个数按原样放回去
            st.push_front(a);
            return false;
    }
    return true;
}

// 改动（补充）：正确。把 a - b 改成 b - a，a、b 的含义不变（a 是先拿出来的栈顶 = 右边的数），
//   所以 default 里“按原样放回去”的顺序也不用动，5 项全过。
//   如果换一种改法 —— 把前面两次读取对调（先读到的叫 b），而 default 里没跟着改，
//   放回去的顺序就反了，最后一项会失败。你这种改法正好避开了。
//   + 和 * 也写成 b + a、b * a，和 - / 保持同一个顺序，读起来更一致，好习惯。

void test2()
{
    Stack s1{4, 10};
    check(apply_op(s1, '+') && s1 == Stack{14}, "练习 2：10 + 4");
    Stack s2{4, 10};
    check(apply_op(s2, '*') && s2 == Stack{40}, "练习 2：10 * 4");
    Stack s3{4, 10};
    check(apply_op(s3, '-') && s3 == Stack{6}, "练习 2：10 - 4");
    std::println("  （得到 {}）", s3);
    Stack s4{2, 8, 99};
    check(apply_op(s4, '/') && s4 == Stack{4, 99}, "练习 2：8 / 2，下面的 99 不受影响");
    std::println("  （得到 {}）", s4);
    Stack s5{2, 8};
    check(!apply_op(s5, '?') && s5 == Stack{2, 8}, "练习 2：不认识的运算符，栈不变");
}

// ============================================================
// 练习 3：最近几次的结果（deque 两头都用）
//   计算器想记住“最近的 capacity 个结果”，类似手机上“最近通话”：
//     add(x)  ：记下一个新结果。如果记录数超过了 capacity，就把最旧的那个丢掉
//     undo()  ：撤销最新的那个结果，并返回它；一个都没有时返回 std::nullopt
//     items() ：从旧到新返回所有记录（已经写好了）
//   约定：d_ 的 front 是最旧的，back 是最新的
//   提示：新的从后面进，旧的从前面出 —— deque 两头都能快速地放入、拿出（示例 [4] [5]）
// ============================================================
class Recent
{
public:
    explicit Recent(std::size_t capacity) : capacity_{capacity}
    {
    }

    void add(double x)
    {
        if (capacity_ > d_.size())
        {
            d_.push_back(x);
        }
        else if (capacity_ != 0)
        {
            d_.push_back(x);
            d_.pop_front();
        }
    }

    std::optional<double> undo()
    {
        if (d_.empty())
        {
            return std::nullopt;
        }
        std::optional<double> pop_back_copy{d_.back()};
        d_.pop_back();
        return pop_back_copy;
    }

    std::vector<double> items() const
    {
        return {d_.begin(), d_.end()};
    }

private:
    std::deque<double> d_;
    std::size_t capacity_;
};

// 改动（补充）：正确，capacity 为 0 的情况也想到了。两点可以更简单：
//   (1) add 可以写成“先放进去，多了再丢”，不用分情况，capacity 为 0 也自然成立：
//           d_.push_back(x);
//           if (d_.size() > capacity_)
//           {
//               d_.pop_front();
//           }
//   (2) undo 里的局部变量用 double 就行：double x = d_.back(); d_.pop_back(); return x;
//       return 时会自动把 double 包装成 optional。
//       另外提一句：要返回的局部变量不要加 const。对 double 无所谓，但如果是 std::string、vector 这种，
//       return 一个 const 局部变量时，编译器没法“移动”它，只能复制一份（移动要修改原对象，const 不让改）。

void test3()
{
    Recent r{3};
    r.add(1);
    r.add(2);
    check(r.items() == std::vector<double>{1, 2}, "练习 3：没满");
    r.add(3);
    r.add(4);
    check(r.items() == std::vector<double>{2, 3, 4}, "练习 3：满了，丢掉最旧的 1");
    std::println("  （得到 {}）", r.items());

    check(r.undo() == 4.0 && r.items() == std::vector<double>{2, 3}, "练习 3：撤销最新的 4");
    r.add(5);
    r.add(6);
    check(r.items() == std::vector<double>{3, 5, 6}, "练习 3：再加两个");

    bool ok = r.undo() == 6.0 && r.undo() == 5.0 && r.undo() == 3.0 && !r.undo().has_value();
    check(ok && r.items().empty(), "练习 3：全部撤销，再撤销返回 nullopt");

    Recent zero{0};
    zero.add(1);
    check(zero.items().empty(), "练习 3：capacity 为 0，什么都不记");
}

// ============================================================
// 练习 4（思考题，把答案写在注释里）
//   (1) 运行示例，看 [2] 里书上的计算器：先输入 "5"，再输入 "+"，栈变成了 "0 5"。
//       对照 BookRPN 的 pop_get2 和 optor，说说这个 0 是怎么来的？5 为什么还在？
//       另外，输入 "-5" 时，栈什么都没变 —— 它走了哪条路，为什么被忽略了？
//   (2) 练习 2 的 bug，为什么 + 和 * 测不出来，只有 - 和 / 测得出来？
//   (3) 练习 3 如果把 d_ 换成 std::vector<double>，add 里“丢掉最旧的”要怎么写？
//       假设 capacity 是 100 万，记录已经满了，这时每 add 一次，vector 要挪动多少个元素？deque 呢？
//       （提示：示例 [4] 的两张图：vector 删掉最前面一个，后面的元素要怎么办？）
//
// 你的答案：
//   (1)pop_get在没有两个数的时候返回了两个0，最后0+0,0被放入了栈顶。整个过程没有把5移走。-5不被numeric当做数字，走了opter(s)这条路。
//       改动（补充）：对。补完“-5”的后半段：进了 optor 之后，opmap.find("-5") 找不到，
//         直接 return zero_，没有碰栈 —— 所以栈不变，调用者也不知道出了错（问题 ②）。
//   (2)+ *满足交换律，不影响。
//       改动（补充）：对。10 + 4 和 4 + 10 一样，所以顺序弄反了也看不出来。
//         这也是写测试的教训：只测 + 和 *，这个 bug 永远发现不了。测试要挑“做错了结果就会不一样”的例子。
//   (3)d_.erase(d_.begin()),100万-1次，得看deque内存断开几次.
//       改动（补充）：vector 部分对：删掉第一个，后面 999999 个元素都要往前挪一格。
//         记录满了之后每 add 一次都要挪 99 万多个；再 add 100 万次，就是大约 1 万亿次挪动。
//         deque 部分不对：pop_front 一个元素都不挪，跟“断开几次”没有关系。
//         回看示例 [4] 的图：deque 只是把第一块里最前面那个元素销毁，再把“开头”的位置往后移一格；
//         如果第一块因此空了，就把这一块释放掉、从目录里去掉。其他元素原地不动。
//         所以 deque 的 pop_front 和 push_back 一样，不管里面有多少元素，花的时间都差不多（O(1)）。
//         “断开”只影响 d[i] 要多绕一步去目录里找块，不影响两头的放入和拿出。
// ============================================================

int main()
{
    test1();
    test2();
    test3();
}
