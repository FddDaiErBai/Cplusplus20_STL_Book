// 第 5 章 5.4 算法库中的谓词：把 lambda 交给 count_if、find_if、sort 这些算法
// 编译：clang++ -std=c++23 -Wall -Wextra -Wpedantic predicates.cpp -o predicates.exe
//       g++     -std=c++23 -Wall -Wextra -Wpedantic predicates.cpp -o predicates.exe -lstdc++exp
//
// 文件结构：[1] 到 [6] 从上到下排列，每个编号只出现一次。
//   每一节里依次是：说明、这一节用到的类和函数、一个 sectionN() 函数（运行这一节）。main 依次调用 section1() 到 section6()。
//
// ============================================================
// 谓词（predicate）：一个“拿元素当参数、回答是或否”的东西。算法的很多参数都是谓词：
//
//   一元谓词 pred(x) → 真/假      用在“看每个元素”的算法里
//   二元谓词 comp(a, b) → 真/假   用在“比较两个元素”的算法里
//
//   算法                                        谓词      例子
//   ----------------------------------------    -------   -----------------------------------
//   count_if、find_if、remove_if、partition     一元      v 里有几个偶数；第一个负数在哪
//   all_of、any_of、none_of                    一元      v 是不是全是正数
//   sort、stable_sort、max_element、minmax      二元      “a 排在 b 前面吗”
//   binary_search、lower_bound                 二元      搜索时怎么比较（默认 <）
//
//   谓词可以是普通函数、函子（重载了 operator() 的类的对象）、lambda。
//   这一节讲清楚三件事：算法怎么接收它、它会被调用多少次、对它的返回类型和可复制性有什么要求。
//
//   [1] 算法是模板：谓词的类型直接作为模板参数，不需要转成函数指针
//   [2] 三种写法：函数、函子、lambda；书上的几处错误
//   [3] 用捕获带上上下文：工厂函数返回“带着参数”的谓词
//   [4] 谓词会被调用多少次？会被复制吗？—— 谓词必须是“纯”的
//   [5] 返回类型：bool 只是习惯；classic 算法和 ranges 算法要求不一样
//   [6] 可复制性、取反 std::not_fn、C++20 的投影（projection）
//
// 书上的错误：
//   · “lambda 表达式具有与函数指针相同的类型”：不对。lambda 的类型是编译器生成的、独一无二的类，
//     只是“没有捕获时可以转换”成函数指针（[2] 验证）
//   · “函数、函子、lambda 的工作方式一致”，然后写 bool (*fp)(int) = is_div4; 其中 is_div4 是那个结构体：
//     函数可以、没有捕获的 lambda 可以、函子不行（[2] 有报错）
//   · “函数指针的类型表示，为调用函数操作符的指针 void (*) ()”：这句翻译不通，忽略
// ============================================================
#include <algorithm>
#include <cmath>
#include <functional>
#include <print>
#include <string>
#include <type_traits>
#include <vector>

// ============================================================
// [1] 算法是模板：谓词的类型直接作为模板参数
//   经典算法的样子（标准库里的声明，简化过）：
//      template <typename InputIt, typename UnaryPredicate>
//      difference_type count_if(InputIt first, InputIt last, UnaryPredicate pred);
//
//   UnaryPredicate 是一个模板参数，你传什么类型，它就实例化成什么类型。
//   传一个 lambda，算法的参数 pred 就是一个 lambda 对象（5.1 节说的那个类的对象），
//   函数体里 pred(*it) 就是直接调用它的 operator()，和调用普通成员函数一样。
//
//   所以：
//     · 不需要转成函数指针，不会多一层间接调用，编译器看得到函数体，可以内联
//     · 每个不同的 lambda（类型不同）都会让算法生成一份新的机器码（模板实例化）
//     · 反过来，用函数指针时算法只知道“有个地址”，一般没法内联
//
//   C++20 的 ranges 版本更好写：第一个参数直接给容器，不用写 begin()/end()：
//      std::count_if(v.begin(), v.end(), pred)     →  std::ranges::count_if(v, pred)
//
//   常见的算法和它们的谓词（v 是 vector<int>）：
//     ranges::count_if(v, pred)       数一数几个元素满足
//     ranges::find_if(v, pred)        第一个满足的位置（迭代器）
//     ranges::all_of / any_of / none_of
//     ranges::remove_if(v, pred)      把满足的挪到后面（要配合 erase 真正删除，第 3 章 3.3 节）
//     ranges::sort(v, comp)           comp 是二元谓词：“a 该排在 b 前面吗”
// ============================================================
void section1()
{
    std::println("[1] 算法是模板");

    const std::vector<int> v{1, 7, 4, 9, 4, 8, 12, 10, 20};
    auto is_div4 = [](int i) { return i % 4 == 0; };

    std::println("  count_if → {}", std::ranges::count_if(v, is_div4));
    auto it = std::ranges::find_if(v, is_div4);
    std::println("  find_if 找到第一个 {}（位置 {}）", *it, std::distance(v.begin(), it));
    std::println("  all_of 全是 4 的倍数？{}  any_of？{}  none_of？{}",
                 std::ranges::all_of(v, is_div4), std::ranges::any_of(v, is_div4), std::ranges::none_of(v, is_div4));
    std::println("  count_if 传的是 lambda：改成传函数指针 +[](int){{...}} → {}",
                 std::ranges::count_if(v, +[](int i) { return i % 4 == 0; }));

    std::vector<int> w{1, 2, 3, 4, 5, 6};
    auto rest = std::ranges::remove_if(w, [](int x) { return x % 2 == 0; });
    // 返回的是一个子范围：开头到“新的结尾”。元素只是被挪到前面了，vector 的长度没变（第 3 章 3.3 节）
    std::println("  remove_if 之后 w 的长度还是 {}，有效的只有前 {} 个：{}", w.size(), std::ranges::ssize(rest), w[0]);
    w.erase(rest.begin(), rest.end());
    std::println("  erase 以后 w == {{1 3 5}}？{}", w == std::vector<int>{1, 3, 5});
}

// ============================================================
// [2] 三种写法：函数、函子、lambda
//   同样是“能被 4 整除吗”，三种写法都行：
//     普通函数    bool is_div4(int i) { return i % 4 == 0; }
//     函子        struct Div4 { bool operator()(int i) const { return i % 4 == 0; } };   用的时候写 Div4{}
//     lambda      [](int i) { return i % 4 == 0; }
//
//   书上说“lambda 表达式具有与函数指针相同的类型”，这句话是错的。真实关系是：
//     · lambda 的类型是编译器生成的类，叫闭包类型，和 bool(*)(int) 是两个完全不同的类型
//     · 没有捕获的 lambda 可以“转换”成函数指针（5.1 节 [7]），这是转换，不是相同
//   验证（下面打印）：
//     is_same<decltype(lambda), bool(*)(int)>        → false
//     is_convertible<decltype(lambda), bool(*)(int)> → true
//     is_convertible<Div4, bool(*)(int)>             → false（函子转不了）
//
//   书上还写：
//       bool (*fp)(int) = is_div4;                          // 这里的 is_div4 是函数：可以
//       bool (*fp)(int) = [] (int i) { return i % 4 == 0; } // 没有捕获：可以
//   但书上前一段里 is_div4 又定义成了结构体。结构体对象不能赋给函数指针：
//     clang：error: no viable conversion from 'Div4' to 'bool (*)(int)'
//     g++  ：error: cannot convert 'Div4' to 'bool (*)(int)' in initialization
//   （书上还把 bool (*fp)(int) 声明了两次，同一个作用域里这是重定义，也编译不过。）
//
//   结论：函子必须有对象才能调用（它可能带数据），函数指针只是一个代码地址，两者的表示不同，不能互转。
// ============================================================
bool is_div4(int i)
{
    return i % 4 == 0;
}

struct Div4
{
    bool operator()(int i) const
    {
        return i % 4 == 0;
    }
};

void section2()
{
    std::println("\n[2] 三种写法");

    const std::vector<int> v{1, 7, 4, 9, 4, 8, 12, 10, 20};
    auto lambda = [](int i) { return i % 4 == 0; };
    bool (*fp)(int) = lambda;   // 没有捕获，可以转换

    std::println("  函数 {}，函子 {}，lambda {}，函数指针 {}",
                 std::count_if(v.begin(), v.end(), is_div4), std::count_if(v.begin(), v.end(), Div4{}),
                 std::count_if(v.begin(), v.end(), lambda), std::count_if(v.begin(), v.end(), fp));
    std::println("  lambda 和 bool(*)(int) 是同一个类型吗？{}", std::is_same_v<decltype(lambda), bool (*)(int)>);
    std::println("  lambda 能转成函数指针吗？{}", std::is_convertible_v<decltype(lambda), bool (*)(int)>);
    std::println("  函子能转成函数指针吗？{}", std::is_convertible_v<Div4, bool (*)(int)>);

    int captured = 4;
    auto with_capture = [captured](int i) { return i % captured == 0; };
    std::println("  有捕获的 lambda 能转成函数指针吗？{}（捕获的数据放不进函数指针）",
                 std::is_convertible_v<decltype(with_capture), bool (*)(int)>);
}

// ============================================================
// [3] 用捕获带上上下文
//   谓词常常需要“阈值”“除数”这类参数。函数指针和普通函数做不到（只能写死或用全局变量），
//   函子要做到得写个类存成员，lambda 只要捕获一下：
//
//      auto is_div_by(int divisor)
//      {
//          return [divisor](int i) { return i % divisor == 0; };
//      }
//
//   每次调用 is_div_by 都返回一个“带着自己那份 divisor”的 lambda 对象。
//   注意它们都是同一个类型（同一个 lambda 表达式，5.1 节 [6]），只是里面的 divisor 不同，可以放进同一个 vector。
// ============================================================
auto is_div_by(int divisor)
{
    return [divisor](int i) { return i % divisor == 0; };
}

void section3()
{
    std::println("\n[3] 用捕获带上上下文");

    const std::vector<int> v{1, 7, 4, 9, 4, 8, 12, 10, 20};
    for (int d : {3, 4, 5})
    {
        std::println("  numbers divisible by {}: {}", d, std::ranges::count_if(v, is_div_by(d)));
    }

    std::println("  is_div_by(3) 和 is_div_by(4) 是同一个类型吗？{}",
                 std::is_same_v<decltype(is_div_by(3)), decltype(is_div_by(4))>);
    std::println("  sizeof = {}（只存了一个 int）", sizeof(is_div_by(3)));
}

// ============================================================
// [4] 谓词会被调用多少次？会被复制吗？
//   实测（v 有 9 个元素，{1, 7, 4, 9, 4, 8, 12, 10, 20}）：
//
//     算法           调用次数   原因
//     ------------   --------   ------------------------------------------
//     count_if       9          每个元素都要问一次
//     find_if        3          找到第一个 4（下标 2）就停
//     all_of         1          第一个元素 1 就不满足，立刻返回 false
//     any_of         3          前两个不是，第三个 4 满足，立刻返回 true
//     remove_if      9          每个元素都要判断要不要留下
//     sort（5 个元素） 7 或 8   比较次数没有规定，两个库就不一样
//
//   “调用几次”大部分是未指定的，标准只给上界（比如 count_if 恰好 n 次是实现的必然，
//   而 sort 用哪种排序算法、比较几次，标准不管）。
//
//   由此得出两条规矩：
//   ① 谓词要“纯”：只看参数，不改外面任何东西。否则每次调用顺序不同，结果就不同
//   ② 谓词不要改自己的状态：算法按值接收谓词，可能复制它。
//      std::sort 内部会复制比较函数；你看到的计数只是“某一个副本的”，不是总数
//   下面 [5] 的例子里，passing a stateful predicate 得到的结果，和调用次数一样是“未指定”的。
// ============================================================
void section4()
{
    std::println("\n[4] 谓词被调用几次");

    std::vector<int> v{1, 7, 4, 9, 4, 8, 12, 10, 20};
    int calls = 0;
    auto counted = [&calls](int x)
    {
        ++calls;
        return x % 4 == 0;
    };

    int n = std::ranges::count_if(v, counted);
    std::println("  count_if → {}，谓词被调用了 {} 次", n, calls);

    calls = 0;
    auto it = std::ranges::find_if(v, counted);
    std::println("  find_if → 找到 {}，调用了 {} 次（提前停）", *it, calls);

    calls = 0;
    bool all = std::ranges::all_of(v, counted);
    std::println("  all_of → {}，调用了 {} 次", all, calls);

    calls = 0;
    bool any = std::ranges::any_of(v, counted);
    std::println("  any_of → {}，调用了 {} 次", any, calls);

    std::vector<int> w{5, 3, 9, 1, 7};
    calls = 0;
    std::ranges::sort(w, [&calls](int a, int b)
    {
        ++calls;
        return a < b;
    });
    std::println("  sort 5 个元素，比较了 {} 次（标准没规定次数；clang 这里 8 次、g++ 7 次，换个库就变）", calls);
}

// ============================================================
// [5] 返回类型有什么要求
//   习惯上谓词返回 bool，但算法只关心“能不能当条件用”，所以返回 int 也行：
//      [](int i) { return i % 4; }   非 0 为真
//   这一点 classic 算法和 ranges 算法不一样：
//
//     谓词返回                      classic（count_if 等）          ranges（ranges::count_if）
//     ---------------------------   ----------------------------   ------------------------------
//     bool                         可以                           可以
//     int                          可以（非 0 为真）               可以（隐式转 bool）
//     带 explicit operator bool 的类 可以（if 的上下文转换允许 explicit）  不行！C++20 概念要求“隐式”转换
//
//   原因：ranges 算法的谓词参数受 std::indirect_unary_predicate 约束，它要求结果满足
//   std::predicate → __boolean_testable → convertible_to<结果, bool>。
//   convertible_to 用的是“隐式”转换，explicit 的转换函数不算。
//   报错很长，关键几行：
//     clang：note: because 'Explicit' does not satisfy '__boolean_testable_impl'
//            note: because 'convertible_to<Explicit, bool>' evaluated to false
//     g++  ：error: could not convert 'main()::Explicit' to 'bool'
//            • explicit conversion function was not considered
//
//   用概念可以在编译时检查一个东西是不是谓词：
//      static_assert(std::predicate<decltype(pred), int>);
// ============================================================
struct Truthy
{
    explicit operator bool() const
    {
        return true;
    }
};

void section5()
{
    std::println("\n[5] 返回类型");

    const std::vector<int> v{1, 7, 4, 9, 4, 8, 12, 10, 20};
    auto int_pred = [](int i) { return i % 4 == 0 ? 1 : 0; };
    std::println("  返回 int：classic {}，ranges {}", std::count_if(v.begin(), v.end(), int_pred), std::ranges::count_if(v, int_pred));

    auto explicit_pred = [](int) { return Truthy{}; };
    std::println("  explicit operator bool：classic {}（能编译，全部算真）", std::count_if(v.begin(), v.end(), explicit_pred));
    std::println("  同一个谓词满足 std::predicate 吗？{}（ranges 算法会拒绝它）", std::predicate<decltype(explicit_pred), int>);
    std::println("  返回 int 的满足吗？{}，返回 bool 的满足吗？{}",
                 std::predicate<decltype(int_pred), int>, std::predicate<decltype([](int i) { return i % 2 == 0; }), int>);
}

// ============================================================
// [6] 可复制性、取反、投影
//   ① 可复制：ranges 算法的谓词参数要求 std::copy_constructible（要能拷贝）。
//      用初始化捕获装一个 unique_ptr 的 lambda 不能拷贝（5.3 节 [4]），所以 ranges 版本直接编译不过：
//         clang：note: because '(lambda ...)' does not satisfy 'copy_constructible'
//         g++  ：error: use of deleted function 'main()::<lambda(int)>::<lambda>(const main()::<lambda(int)>&)'
//      classic 版本在这一点上两个库不一样（实测）：
//         libc++（clang）  能编译，返回 5：直接把参数移动进去，内部不再复制
//         libstdc++（g++） 编译不过：内部会复制一份谓词（stl_algo.h: use of deleted function ...）
//      这属于“标准没要求具体实现怎么做”，所以不能依赖。想跨库通用：谓词必须能拷贝。
//
//   ② 取反：std::not_fn(pred) 返回一个把结果取反的包装。C++20 删掉了旧的 std::not1 / std::not2：
//         libc++  ：error: no member named 'not1' in namespace 'std'
//         libstdc++：还在，但 clang 和 gcc 都会警告 deprecated: use 'std::not_fn' instead
//
//   ③ 投影（projection）：ranges 算法多了一个“先取出要用的字段”的参数，写在谓词后面。
//          ranges::max_element(people, {}, &Person::age)     ↑ 比较用默认的 <
//          ranges::count_if(people, [](int age) { return age > 25; }, &Person::age)
//      有了它就不用写 [](const Person& a, const Person& b) { return a.age < b.age; } 这种比较函数了。
// ============================================================
struct Person
{
    std::string name;
    int age;
};

void section6()
{
    std::println("\n[6] 可复制性、取反、投影");

    std::vector<int> v{1, 7, 4, 9, 4, 8, 12, 10, 20};
    auto not_div4 = std::not_fn([](int i) { return i % 4 == 0; });
    std::println("  not_fn：不能被 4 整除的有 {} 个", std::ranges::count_if(v, not_div4));

    std::vector<Person> people{{"Ada", 36}, {"Bob", 19}, {"Cy", 44}, {"Di", 27}};
    auto oldest = std::ranges::max_element(people, {}, &Person::age);
    std::println("  投影：max_element 按 age 取最大的 → {} 岁（{}）", oldest->age, oldest->name);
    std::println("  投影 + 谓词：count_if 先取 age 再判断 → {} 人超过 25 岁",
                 std::ranges::count_if(people, [](int age) { return age > 25; }, &Person::age));
}

int main()
{
    section1();
    section2();
    section3();
    section4();
    section5();
    section6();
}
