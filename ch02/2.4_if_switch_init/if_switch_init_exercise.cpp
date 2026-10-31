// 第 2 章 2.4 if 和 switch 语句中初始化变量 —— 练习
// 编译：clang++ -std=c++23 -Wall -Wextra if_switch_init_exercise.cpp -o if_switch_init_exercise.exe
// 用 static_assert 在编译时检查：补完 TODO，去掉断言的注释，能编译通过就说明做对了。
#include <algorithm>
#include <array>
#include <charconv>
#include <string_view>
#include <system_error>

// ============================================================
// 练习 1：用 if 初始化改写
//   下面的 price_of 能用，但变量 it 泄漏到了 if 外面。
//   改成 if (初始化; 条件) 的写法，让 it 只在 if 里可见。函数的功能不变
// ============================================================
struct Item
{
    std::string_view name;
    int price;
};

inline constexpr std::array<Item, 3> menu{{{"coffee", 25}, {"tea", 18}, {"juice", 30}}};

constexpr int price_of(std::string_view name) // 找不到返回 -1
{
    if (auto it = std::ranges::find(menu, name, &Item::name); it != menu.end())
    {
        return it->price;
    }
    return -1;
}

static_assert(price_of("tea") == 18);
static_assert(price_of("cola") == -1);

// ============================================================
// 练习 2：把字符串转成整数，失败返回 -1
//   要求：用 std::from_chars（<charconv>，C++23 起可以在编译时用），并且用一条
//         if (auto [ptr, ec] = std::from_chars(...); 条件) 同时完成“转换 + 判断”
//   std::from_chars(开头指针, 结尾指针, 结果变量) 返回一个结构体，两个成员按顺序是：
//         ptr：转换停在了哪里；ec：错误码，成功时等于 std::errc{}
//   “成功”要同时满足：没有错误，并且整个字符串都被转换完了（"12a" 只转换了 "12"，算失败）
//   提示：s.data() 是开头指针，s.data() + s.size() 是结尾指针
// ============================================================
constexpr int parse_int(std::string_view s)
{
    int result{0};
    if (auto [ptr,ec] = std::from_chars(s.data(), s.data() + s.size(), result);
        ec == std::errc{} && ptr == s.data() + s.size())
    {
        return result;
    }
    return -1;
}

static_assert(parse_int("123") == 123);
static_assert(parse_int("0") == 0);
static_assert(parse_int("12a") == -1);
static_assert(parse_int("") == -1);
static_assert(parse_int("abc") == -1);
static_assert(parse_int("99999999999") == -1); // 超出 int 范围：ec 是 std::errc::result_out_of_range

// ============================================================
// 练习 3：用 switch 初始化，按分数给出等级
//   90~100 -> 'A'，80~89 -> 'B'，70~79 -> 'C'，60~69 -> 'D'，0~59 -> 'F'
//   要求：在 switch 的初始化部分算出“十位数”（score / 10），再 switch 它
//   提示：100 / 10 是 10，要和 9 一起归到 'A'。几个 case 可以叠在一起写：
//           case 10:
//           case 9:
//               return 'A';
// ============================================================
constexpr char grade(int score)
{
    switch (int level{(score / 10)}; level)
    {
        case 10:
        case 9:
            return 'A';
        case 8:
            return 'B';
        case 7:
            return 'C';
        case 6:
            return 'D';
        default:
            return 'F';
    }
}

static_assert(grade(100) == 'A');
static_assert(grade(95) == 'A');
static_assert(grade(89) == 'B');
static_assert(grade(70) == 'C');
static_assert(grade(65) == 'D');
static_assert(grade(59) == 'F');
static_assert(grade(0) == 'F');

// ============================================================
// 练习 4：找 bug
//   Guard 创建时 depth 加 1，销毁时减 1（就像 lock_guard 创建时加锁、销毁时解锁）。
//   enter 想在“已经进入”（depth == 1）的状态下执行 if 里面的代码，但 if 里面从来没执行过。
//   找出原因并修改（参考示例第 5 部分）
// ============================================================
struct Guard
{
    int& depth;

    constexpr explicit Guard(int& d) : depth{d}
    {
        ++depth;
    }

    constexpr ~Guard()
    {
        --depth;
    }
};

constexpr bool enter()
{
    int depth = 0;
    if (Guard guard_1{depth}; depth == 1)
    {
        return true;
    }
    return false;
}

static_assert(enter());

// ============================================================
// 练习 5（思考题，把答案写在注释里）
//   (1) 练习 1 改完之后，函数的功能完全没变，那这样改的好处是什么？
//       如果函数里后面还要查另一样东西，两种写法各会怎样？
//   (2) 用 -std=c++20 和 -std=c++23 分别编译运行 range_for_lifetime.cpp（命令在文件开头）。
//       写法 1 在 C++20 下发生了什么？为什么 g++ 和 clang++ 的输出还不一样？C++23 改了什么？
//       写法 2 为什么两个标准下都没问题？
//   (3) for 循环从 C 语言起就有初始化部分，C++17 给 if、switch 加上了，C++20 给范围 for 加上了，
//       但 while 一直没有。如果想要“只在循环里可见的变量 + while 的效果”，可以怎么写？
//
// 你的答案：
//   (1)it不会泄露到外面。不知道
//       改动（补充）：前半句对。后半句的意思是：假如函数里接着还要查 "juice" 的价格——
//         旧写法：第一个 it 还活着，不能再写 auto it = ...（同一个作用域里重复定义，编译失败），
//                 只能起个新名字 it2，或者直接给旧的 it 赋值。查得越多，名字越乱，还容易用错（比如判断了 it2，却用了 it）。
//         新写法：每个 if 有自己的 it，用完就消失，第二个 if 可以放心地再叫 it，互不干扰。
//       一句话：变量的作用域越小，就越不容易被误用。
//   (2)生命周期不同
//       改动（补充）：方向对，具体是这样：
//       写法 1：make_box() 返回一个临时的 Box，items() 返回它内部 vector 的引用。
//         C++20 规定这个临时 Box 在“冒号后面那个表达式”求值完就销毁了，循环体还没开始执行，
//         于是循环遍历的是一个已经销毁的 vector —— 未定义行为。
//         未定义行为的结果没有任何保证，所以两个编译器表现不同：g++ 读到了析构函数写进去的 -1，
//         clang 一个都没打印（读到的长度已经不对了）。换个优化级别结果可能又不一样。g++ 还给了 -Wdangling-reference 警告。
//       C++23 改了规则：冒号后面表达式里产生的临时对象，一直活到整个循环结束，所以写法 1 也安全了。
//       写法 2：box 是初始化部分声明的有名字的变量，它活到整个 for 语句结束，两个标准下都没问题。
//       建议：你没运行这个文件，可以用命令运行一下，亲眼看看 C++20 下的 -1 和 ~Box() 出现的位置。
//   (3)每一次循环都将值传出去？
//       改动（补充）：不需要新语法，用 for 不写第三部分就行，它就是一个“带初始化的 while”：
//         for (int tries = 0; tries < 3;)   // 等价于：int tries = 0; while (tries < 3)，但 tries 不会泄漏
//         {
//             ...
//             ++tries;
//         }
//       另一种办法是在外面套一层大括号：{ int tries = 0; while (tries < 3) { ... } }，大括号结束 tries 就消失了。
// ============================================================

int main()
{}
