// 第 3 章 3.5 安全地访问 vector 元素 —— 练习
// 编译：clang++ -std=c++23 -Wall -Wextra safe_access_exercise.cpp -o safe_access_exercise.exe
// 用 static_assert 在编译时检查：补完 TODO，去掉断言的注释，能编译通过就说明做对了。
// 提示：在编译时执行到 at() 越界，编译器会报错，note 里能看到 at(多少)，这个数字很有用
#include <cstddef>
#include <vector>

// ============================================================
// 练习 1：get_or —— 支持负数下标（像 Python 一样）
//   idx >= 0：从前往后数，0 是第一个
//   idx <  0：从后往前数，-1 是最后一个，-2 是倒数第二个
//   越界（不管哪个方向）就返回 fallback
//   要求：idx 是有符号的 std::ptrdiff_t；不能越界访问（用 [] 或 at() 都行，但要先判断好）
//   提示：v.size() 是无符号的 size_t。有符号和无符号直接比较、相加，负数会先变成一个巨大的正数，
//         所以先把 size 转成有符号的：auto n = static_cast<std::ptrdiff_t>(v.size());
//         （C++20 也可以写 std::ssize(v)，直接得到有符号的长度）
// ============================================================
constexpr int get_or(const std::vector<int>& v, std::ptrdiff_t idx, int fallback)
{
    if (idx >= 0)
    {
        if (idx > std::ssize(v) - 1)
        {
            return fallback;
        }
        return v[static_cast<std::size_t>(idx)];
    }

    if (idx < -std::ssize(v))
    {
        return fallback;
    }
    return v[static_cast<std::size_t>(std::ssize(v) + idx)];
}
// 改动（另一种写法，供参考）：先把负数下标“换算”成正数，两个方向就只需要一次检查
//     const auto n = std::ssize(v);
//     if (idx < 0)
//     {
//         idx += n;          // -1 -> n-1，-n -> 0，比 -n 还小的仍然是负数
//     }
//     if (idx < 0 || idx >= n)
//     {
//         return fallback;
//     }
//     return v[static_cast<std::size_t>(idx)];

inline constexpr int fb = -999;

static_assert(get_or({10, 20, 30}, 0, fb) == 10);
static_assert(get_or({10, 20, 30}, 2, fb) == 30);
static_assert(get_or({10, 20, 30}, 3, fb) == fb);
static_assert(get_or({10, 20, 30}, -1, fb) == 30);
static_assert(get_or({10, 20, 30}, -3, fb) == 10);
static_assert(get_or({10, 20, 30}, -4, fb) == fb);
static_assert(get_or({}, 0, fb) == fb);
static_assert(get_or({}, -1, fb) == fb);

// ============================================================
// 练习 2：找 bug（1）—— 倒着复制
//   reversed 想返回一个倒序的新 vector。取消断言的注释，编译失败。
//   (a) 读 clang 的报错，note 里 at(...) 括号里的数是多少？它是怎么来的？
//       （用 g++ 编译的话，不取消注释也会看到一个警告，读一读）
//   (b) 修改：保留“用 std::size_t 下标倒着走”的写法（不要改成 int，也不要用 views::reverse），
//       让三个断言都通过。注意空 vector 也要正确
// ============================================================
constexpr std::vector<int> reversed(const std::vector<int>& v)
{
    std::vector<int> out;
    for (std::size_t i = v.size(); i >= 1; --i)
    {
        out.push_back(v.at(i - 1));
    }
    return out;
}
// 改动（补充 (a)）：clang 的 note 是 at(18446744073709551615)。来历：
//   空 vector 时 v.size() - 1 = 0 - 1，size_t 不能是负数，于是“绕回”到最大值 2^64 - 1；
//   非空时也一样：i 减到 0 之后再 --i，又变成这个最大值，而 i >= 0 永远成立（g++ 的警告说的就是这个）。
// 你的改法（i 从 size 开始，条件 i >= 1，用 i - 1 当下标）完全正确，空 vector 时循环一次都不进。
// 常见的另一种写法是把“减 1”放进条件里：for (std::size_t i = v.size(); i-- > 0;)
//   先比较 i > 0，再减 1，循环体里直接用 i。意思和你的一样，看到别人这么写能认出来就行。

static_assert(reversed({1, 2, 3}) == std::vector{3, 2, 1});
static_assert(reversed({7}) == std::vector{7});
static_assert(reversed({}).empty());

// ============================================================
// 练习 3：找 bug（2）—— 最后 n 个的和
//   last_n_sum 想返回最后 n 个元素的和；n 比元素个数还多时，返回全部元素的和。
//   取消断言的注释，最后两个断言失败。这次没有越界、没有报错，只是结果不对 —— 比崩溃更难发现。
//   找出原因并修改
// ============================================================
constexpr int last_n_sum(const std::vector<int>& v, std::size_t n)
{
    int sum = 0;
    if (n >= v.size())
    {
        for (const auto& i : v)
        {
            sum += i;
        }
        return sum;
    }
    for (std::size_t i = v.size() - n; i < v.size(); ++i)
    {
        sum += v.at(i);
    }
    return sum;
}
// 改动（补充）：正确。原因是 v.size() - n 在 n > size 时同样“绕回”成巨大的数，
//   i < v.size() 一开始就不成立，循环一次都没执行，返回 0 —— 不崩溃，只是结果错。
// 两段循环其实可以合成一段：先算出起点，不够减就从 0 开始
//     const std::size_t start = n >= v.size() ? 0 : v.size() - n;
//     for (std::size_t i = start; i < v.size(); ++i) ...
// 规律：size_t 做减法之前，先确认“被减数 >= 减数”。

static_assert(last_n_sum({1, 2, 3, 4}, 2) == 7);
static_assert(last_n_sum({1, 2, 3, 4}, 0) == 0);
static_assert(last_n_sum({1, 2, 3, 4}, 4) == 10);
static_assert(last_n_sum({1, 2, 3, 4}, 5) == 10);
static_assert(last_n_sum({1, 2, 3, 4}, 100) == 10);

// ============================================================
// 练习 4（思考题，把答案写在注释里）
//   (1) 用下面的命令运行示例的越界部分，把输出贴在这里（一行就行）：
//           ./safe_access.exe oob
//       书上说越界的 [] 读到的是 0。你读到的是什么？
//       为什么说“悄悄读到一个看起来正常的值”比“程序直接崩溃”更糟糕？
//   (2) 下面三行执行完，m.size() 是多少？m["b"] 是多少？
//           std::map<std::string, int> m{{"a", 1}};
//           int x = m["b"];
//           int y = m["c"] + 1;
//       另外，如果 m 是 const std::map<std::string, int>&，还能写 m["a"] 吗？为什么？
//   (3) 书上说 at() 应该是首选。看下面这个循环：
//           for (std::size_t i = 0; i < v.size(); ++i)
//           {
//               total += v.at(i);
//           }
//       这里的 at() 检查有必要吗？有更好的写法吗？什么情况下 at() 才真正有用？
//
// 你的答案：
//   (1)我读到的也是0，错误被隐藏起来了
//       改动（补充）：对。展开说：崩溃会立刻告诉你“这里有 bug”，还能定位到哪一行；
//         而读到 0 这种看起来正常的值，程序会带着错误数据继续跑，可能很久以后在完全不相关的地方才出问题，
//         甚至一直不出问题，直到换了编译器、换了优化级别（用 g++ 编译同一份代码，-O0 直接断言失败，-O2 读到垃圾值）。
//         如果是写入越界，还会悄悄改坏别的变量 —— 这就是很多安全漏洞的来源。
//   (2)不懂你和我讲讲
//       改动（补充）：map 的 m[key] 规定是：键存在就返回它的值；键不存在，就先插入一个 {key, 值类型的默认值}，再返回它。
//         int 的默认值是 0。所以：
//           std::map<std::string, int> m{{"a", 1}};   // m = {a:1}
//           int x = m["b"];                            // "b" 不存在 → 插入 {b:0}，x = 0
//           int y = m["c"] + 1;                        // "c" 不存在 → 插入 {c:0}，y = 1
//         执行完 m.size() 是 3，m["b"] 是 0。只是“读一下”，map 就多了两个元素。
//         这个设计是为了写起来方便：m["apple"] += 1 不用先判断存不存在（3.12 节词频统计就靠这个）。
//       const map 不能用 []：因为 [] 可能要插入元素，也就是可能修改 map，而 const 对象不允许修改，
//         所以标准干脆没给 map 提供 const 版本的 operator[]，编译直接报错。
//         const map 只能用 m.at("a")（找不到抛异常）或 m.find("a")（找不到返回 end()）。
//       对比 vector：vector 的 [] 有 const 版本，因为它从不插入，越界就是未定义行为。
//   (3)没必要，范围for，需要访问外来的，不确定是否边界安全。
//       改动（补充）：正确。循环条件 i < v.size() 已经保证了 i 不会越界，at() 再检查一遍是重复的。
// ============================================================

int main()
{
}
