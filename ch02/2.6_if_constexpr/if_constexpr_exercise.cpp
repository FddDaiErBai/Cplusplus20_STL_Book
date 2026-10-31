// 第 2 章 2.6 if constexpr —— 练习
// 编译：clang++ -std=c++23 -Wall -Wextra if_constexpr_exercise.cpp -o if_constexpr_exercise.exe
// 用 static_assert 在编译时检查：补完 TODO，去掉断言的注释，能编译通过就说明做对了。
#include <array>
#include <climits> // 改动：INT_MIN 在这里；<cmath> 不用 abs 了，删掉
#include <cstddef>
#include <string_view>
#include <type_traits>

// ============================================================
// 练习 1：length_of —— 不同的类型，不同的“长度”
//   用一条 if constexpr / else if constexpr 链实现：
//     整数类型        -> 十进制位数（不算负号）：12345 -> 5，0 -> 1，-42 -> 2
//     有 .size() 的   -> v.size()
//     C 数组          -> 数组的长度（提示：std::is_array_v<T> 和 std::extent_v<T>）
//     其他类型        -> static_assert(false, "...")，报错信息自己写
//   提示：
//     - 位数可以用循环：每次除以 10，直到变成 0。想想 0 怎么处理才能得到 1（do-while 很合适）
//     - 参数是 const T&，传 C 数组时 T 就是数组类型本身（比如 int[5]），不会退化成指针（2.5 节）
// ============================================================
template <typename T>
constexpr std::size_t length_of(const T& v)
{
    if constexpr (std::is_integral_v<T>)
    {
        std::size_t n{0};
        //std::abs(v); // c++23不是支持abs为constexpr函数了吗？为什么用不了？
        // 改动（回答）：标准是这么规定的（P0533），但两个库都还没实现。检查办法是 1.6 节的特性宏
        //   __cpp_lib_constexpr_cmath，clang 和 g++ 都没有定义它。
        //   g++ 下 std::abs(int) 碰巧能在编译时用，是因为 g++ 把 abs 当内置函数直接算了，不是库实现了；
        //   换成 std::abs(-5L) 照样报错。clang 用的是 mingw 的 C 函数 abs，完全不能在编译时用。
        // 改动：原来的 int v_abs{(v < 0 ? -v : v)} 有三个问题：
        //   ① T = long long 时，把 long long 塞进 int，大括号初始化不允许变窄，clang 直接报错
        //   ② T = unsigned 时同理（下面新增的断言能看到）
        //   ③ v = INT_MIN 时，-v 超出 int 范围（int 的负数比正数多一个），编译时计算会报“溢出”
        // 其实根本不需要取绝对值：负数除以 10 也是向 0 取整，-42 -> -4 -> 0，一样是 2 位。
        // 用 T 类型保存，把循环条件从 > 0 改成 != 0，就同时解决了三个问题
        T x{v};
        do
        {
            x = x / 10;
            ++n;
        }
        while (x != 0);
        return n;
    }
    else if constexpr (requires { v.size(); })
    {
        return v.size();
    }
    else if constexpr (std::is_array_v<T>)
    {
        return std::extent_v<T>;
    }
    else
    {
        static_assert(false, "length_of 不支持这个类型");
    }
}

inline constexpr int arr[5]{};
inline constexpr std::array<int, 4> nums{1, 2, 3, 4};

static_assert(length_of(12345) == 5);
static_assert(length_of(0) == 1);
static_assert(length_of(-42) == 2);
static_assert(length_of(std::string_view{"abc"}) == 3);
static_assert(length_of(nums) == 4);
static_assert(length_of(arr) == 5);
static_assert(length_of("hello") == 6); // 字符串字面量是 const char[6]，结尾还有一个 '\0'
// static_assert(length_of(3.14) == 0);    // 只取消注释看一眼报错信息，然后注释回去
static_assert(length_of(12345678901LL) == 11); // 改动（新增）：原写法 clang 报错（变窄）
static_assert(length_of(5u) == 1);             // 改动（新增）：原写法 clang 报错（变窄）
static_assert(length_of(INT_MIN) == 10);       // 改动（新增）：原写法两个编译器都报错（-v 溢出）

// ============================================================
// 练习 2：找 bug（1）
//   size_or_value 想做的事：整数返回它本身，有 .size() 的返回 size()，其他类型返回 -1。
//   先只取消前两个断言的注释，能编译；再取消第三个，编译失败。
//   读一读报错信息，找出原因并修改（只需要改一个地方）
// ============================================================
template <typename T>
constexpr int size_or_value(const T& v)
{
    if constexpr (std::is_integral_v<T>)
    {
        return v;
    }
    else if constexpr (requires { v.size(); })
    {
        return static_cast<int>(v.size());
    }
    else
    {
        return -1;
    }
}

static_assert(size_or_value(7) == 7);
static_assert(size_or_value(std::string_view{"abcd"}) == 4);
static_assert(size_or_value(3.5) == -1);

// ============================================================
// 练习 3：找 bug（2）
//   positive_value<T>() 想做的事：如果 T 有一个静态成员 value，并且 value > 0，就返回 value；
//   其他情况（没有 value、或者 value <= 0）都返回 0。
//   取消断言的注释后，最后一个断言编译失败。找出原因并修改（参考示例第 6 部分）
// ============================================================
struct Five
{
    static constexpr int value = 5;
};

struct MinusThree
{
    static constexpr int value = -3;
};

template <typename T>
constexpr int positive_value()
{
    if constexpr (requires { T::value; })
    {
        if constexpr (T::value > 0)
        {
            return T::value;
        }
        else
        {
            return 0;
        }
    }
    else
    {
        return 0;
    }
}

static_assert(positive_value<Five>() == 5);
static_assert(positive_value<MinusThree>() == 0);
static_assert(positive_value<int>() == 0);

// ============================================================
// 练习 4（思考题，把答案写在注释里）
//   (1) 练习 2 你改了哪里？为什么原来的写法在 T = double 时编译失败，而 T = int 时没问题？
//   (2) 下面的函数为什么编译不过？想要“n > 0 时做一件事，否则做另一件事”，应该怎么写？
//           constexpr int sign(int n)
//           {
//               if constexpr (n > 0)
//               {
//                   return 1;
//               }
//               return 0;
//           }
//   (3) 把下面这个函数复制到文件里编译一次（不用运行），看编译器给出的警告。
//       为什么说 if constexpr (std::is_constant_evaluated()) 永远走第一个分支？
//       正确的写法是什么？（示例第 8 部分有一种）
//           constexpr int where()
//           {
//               if constexpr (std::is_constant_evaluated())
//               {
//                   return 1;
//               }
//               else
//               {
//                   return 2;
//               }
//           }
//
// 你的答案：
//   (1)加了constexpr，因为T=int，选择了int分支，其余都舍弃了。
//       改动（补充）：改对了，T = int 的原因也对。还差 T = double 为什么失败：
//         第一个 if constexpr 不成立，丢弃的是第一个分支，else 部分保留下来了。
//         else 里面是普通 if，普通 if 的两个分支都要能编译，于是 v.size() 要对 double 生成 —— 报错。
//         注意 requires { v.size(); } 这个条件本身没问题，它对 double 算出 false；出错的是分支里面的 v.size()。
//       一句话：else if 少了 constexpr，后面的分支就失去了“丢弃”的保护。
//   (2)constexpr int sign(int n)
//           {
//               if constexpr (n > 0)
//               {
//                   return 1;
//               }
//               else
//               {
//                   return 0;
//               }
//
//           }
//       改动（补充）：这样还是编译不过，加 else 不解决问题。原因是条件 n > 0 不是编译时常量：
//         n 是函数参数，constexpr 函数也可以在运行时调用（比如 sign(从键盘读的数)），
//         所以编译器不能把 n 当常量 —— 哪怕这次调用传的是 sign(5) 也不行。
//         if constexpr 的条件必须在“写函数的时候”就能确定，不能取决于调用时传进来的值。
//       正确写法：去掉 constexpr，用普通 if。constexpr 函数里的普通 if 照样能在编译时执行，
//         static_assert(sign(5) == 1) 没问题：
//           constexpr int sign(int n)
//           {
//               if (n > 0)
//               {
//                   return 1;
//               }
//               return 0;
//           }
//       如果真的需要编译时选分支，就得把 n 变成模板参数：template <int N> constexpr int sign() { if constexpr (N > 0) ... }
//       （1.6 节练习你也踩过这个坑：if constexpr 用在了运行时参数上。记住：if constexpr 的条件里只能出现
//         类型、模板参数、constexpr 变量，不能出现函数参数。）
//   (3)std::is_constant_evaluated()会判断当前调用是不是在编译期，放在constexpr if里面，肯定是编译期，为true。正确写法是std::is_constant_evaluated()要放在普通if里面
//       改动（补充）：正确。C++23 还可以写 if consteval { ... } else { ... }（示例第 8 部分），
//         它不需要条件，也就不会写错，推荐用这个。
// ============================================================


int main()
{
}
