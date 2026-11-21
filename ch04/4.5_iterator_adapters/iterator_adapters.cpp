// 第 4 章 4.5 用迭代器适配器填充容器：插入迭代器、流迭代器、反向迭代器、移动迭代器
// 编译：clang++ -std=c++23 -Wall -Wextra -Wpedantic iterator_adapters.cpp -o iterator_adapters.exe
//       g++     -std=c++23 -Wall -Wextra -Wpedantic iterator_adapters.cpp -o iterator_adapters.exe -lstdc++exp
//
// 文件结构：[1] 到 [11] 从上到下排列，每个编号只出现一次。
//   每一节里依次是：说明、这一节用到的函数、一个 sectionN() 函数（运行这一节）。main 依次调用 section1() 到 section11()。
//
// ============================================================
// 什么是“迭代器适配器”：一个类，用起来像迭代器（能写 *it、++it、*it = x），但背后做的是别的事。
//   比如 back_inserter(v)：对它写 *it = 5，实际做的是 v.push_back(5)。
//   这样，所有“往迭代器里写”的算法（copy、transform、copy_if ……）就都能用来往容器里“加”元素了。
//
// 分四类（都在 <iterator> 里）：
//
//   类别         函数（造出适配器）             对它做 *it = x 时实际做的事      [n]
//   ----------  ------------------------------  ------------------------------  -------
//   插入迭代器   std::back_inserter(c)           c.push_back(x)                  [2]
//                std::front_inserter(c)          c.push_front(x)                 [3]
//                std::inserter(c, pos)           c.insert(pos, x)，pos 往后挪    [4]
//   流迭代器     std::ostream_iterator<T>(os)    os << x                         [6]
//                std::istream_iterator<T>(is)    （反过来：读）*it 是 is >> 读到的值   [7][8]
//   反向迭代器   c.rbegin()、std::rbegin(arr)    ++ 变成往回走                    [9]
//   移动迭代器   std::make_move_iterator(it)     *it 得到右值，复制变成“移动”     [10]（书上没讲）
//
// 怎么读算法的第三个参数：std::copy(first, last, out) 内部就是
//     for (; first != last; ++first) { *out = *first; ++out; }
//   out 是普通迭代器时：覆盖 out 指向的已有元素（[1]）
//   out 是 back_inserter 时：*out = x 变成 push_back(x)（[2]）
// ============================================================
#include <algorithm>
#include <deque>
#include <iterator>
#include <list>
#include <print>
#include <ranges>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

// ============================================================
// [1] std::copy 不会分配空间：目标里必须已经有足够的元素
//   书上：deque<int> d2(d1.size()); 先造 5 个 0，copy 再把它们覆盖掉
//   书上说“尝试不分配空间的 vector，会出现段错误”：不一定。往不存在的位置写是未定义行为，
//     可能崩溃，也可能悄悄写坏别的内存、看起来“正常”，这比崩溃更糟
//   要“加”而不是“覆盖”，就用 [2] 的插入迭代器
// ============================================================
void section1()
{
    std::println("[1] copy 只覆盖，不分配");
    std::deque<int> d1{1, 2, 3, 4, 5};
    std::deque<int> d2(d1.size()); // 圆括号：5 个元素，都是 0（花括号 {5} 会是“一个元素 5”）
    std::println("  d2 copy 之前：{}", d2);
    std::copy(d1.begin(), d1.end(), d2.begin());
    std::println("  d2 copy 之后：{}", d2);
    // std::vector<int> empty;
    // std::copy(d1.begin(), d1.end(), empty.begin()); // 未定义行为：empty 里一个位置都没有
}

// ============================================================
// [2] back_inserter：每次赋值调用一次 push_back
//   std::back_inserter(c) 返回一个 std::back_insert_iterator<C> 对象，里面只存了一个指向容器 c 的指针
//   它的几个运算符：
//     it = x    → c.push_back(x)        真正干活的只有这个
//     *it       → 返回 it 自己           什么都不做
//     ++it、it++ → 返回 it 自己           什么都不做
//   为什么 * 和 ++ 什么都不做还要写？因为算法里写的是 *out = x; ++out;（或者 *out++ = x;），
//   这几个运算符必须“能写”，算法才能编译。练习 3 你自己写一个
//   哪些容器能用：有 push_back 的都行（vector、deque、list、string）
// ============================================================
void section2()
{
    std::println("\n[2] back_inserter");
    std::deque<int> d1{1, 2, 3, 4, 5};
    std::deque<int> d2{1, 2, 3, 4, 5};
    std::copy(d1.begin(), d1.end(), std::back_inserter(d2));
    std::println("  d2 after back_inserter：{}", d2);

    std::vector<int> v;
    auto out = std::back_inserter(v); // 类型是 std::back_insert_iterator<std::vector<int>>
    *out = 10;                        // v.push_back(10)
    ++out;                            // 什么都不做
    out = 20;                         // 不写 * 也一样：= 才是干活的
    *out++ = 30;                      // 算法里常见的写法：先 out++（什么都不做），再 *（什么都不做），再 = 30
    std::println("  手动用：{}", v);

    // ranges 版本的算法也能用，copy_if、transform 也一样（第三个参数都是“往哪写”）
    std::ranges::copy_if(d1, std::back_inserter(v), [](int x) { return x % 2 == 0; });
    std::ranges::transform(d1, std::back_inserter(v), [](int x) { return x * 100; });
    std::println("  copy_if 偶数、transform ×100 之后：{}", v);
}

// ============================================================
// [3] front_inserter：每次赋值调用一次 push_front，所以顺序是反的
//   先插 47，再把 73 插到 47 前面，再把 114 插到最前面……最后插的 54 在最前
//   只有有 push_front 的容器能用：deque、list、forward_list。vector 没有 push_front：
//     std::front_inserter(vec) 再 copy：
//       clang：error: no member named 'push_front' in 'std::vector<int>'
//       g++  ：error: 'class std::vector<int>' has no member named 'push_front'
//   想放到前面又保持原来的顺序：用 [4] 的 inserter(c, c.begin())，或者 [5] 的 prepend_range
// ============================================================
void section3()
{
    std::println("\n[3] front_inserter");
    std::deque<int> d2{1, 2, 3, 4, 5};
    std::deque<int> d3{47, 73, 114, 138, 54};
    std::copy(d3.begin(), d3.end(), std::front_inserter(d2));
    std::println("  d2 after front_inserter：{}", d2);
}

// ============================================================
// [4] inserter(c, pos)：从 pos 这个位置开始插入，顺序不变
//   它里面存着容器指针和一个位置 iter。每次 it = x 做的是：
//       iter = c.insert(iter, x);   // 插到 iter 前面，insert 返回新元素的位置
//       ++iter;                     // 挪到新元素后面，下一个就插在它后面
//   所以插进去的元素保持原来的顺序（和 front_inserter 不一样）
//   书上没提醒的坑：deque、vector 在中间插入后，原来的迭代器（下面的 pos）全部失效，不能再用了
//     （inserter 自己内部用的是 insert 返回的新迭代器，所以它没事）
//   inserter 也能用在 set 上：set 没有 push_back，但有 insert(位置提示, 值)，位置只是“提示”，
//     set 还是会放到排好序的地方。这是把一堆数复制进 set 的常用写法
// ============================================================
void section4()
{
    std::println("\n[4] inserter");
    std::deque<int> d1{1, 2, 3, 4, 5};
    std::deque<int> d2{54, 138, 114, 73, 47};
    auto pos = d2.begin() + 2;
    std::copy(d1.begin(), d1.end(), std::inserter(d2, pos));
    std::println("  d2 在第 2 个位置插入 d1：{}", d2);
    // 这里之后 pos 已经失效，不能再 *pos 或者再拿它插入

    std::set<int> s{5, 1};
    std::ranges::copy(std::vector{3, 1, 4, 1, 5}, std::inserter(s, s.end()));
    std::println("  copy 进 set（自动排序、去重）：{}", s);
}

// ============================================================
// [5] 书上没讲：只是“把一段东西加进容器”，C++23 有更直接的成员函数
//   v.append_range(r)            加到末尾        （相当于 copy + back_inserter）
//   v.insert_range(pos, r)       从 pos 插入     （相当于 copy + inserter）
//   d.prepend_range(r)           加到前面，顺序不变（deque、list 有；vector 没有）
//   r | std::ranges::to<std::vector>()   直接造一个新容器（3.x 节用过）
//   C++23 之前：v.insert(v.end(), first, last);
//   为什么更好：它们一次就知道要加多少个，vector 只需要扩容一次；
//     back_inserter 是一个一个 push_back，vector 可能要扩容很多次（思考题 1 实际数一数）
//   插入迭代器真正有用的地方：配合 copy_if、transform 这类“边算边写”的算法，事先不知道会写几个
// ============================================================
void section5()
{
    std::println("\n[5] C++23 的 append_range、insert_range、prepend_range");
    std::vector<int> src{7, 8, 9};
    std::vector<int> v{1, 2};
    v.append_range(src);
    std::println("  v.append_range：             {}", v);
    v.insert_range(v.begin() + 1, src);
    std::println("  v.insert_range(begin + 1)：  {}", v);
    std::deque<int> d{1, 2};
    d.prepend_range(src);
    std::println("  d.prepend_range（顺序不变）：{}", d);
    auto rev = src | std::views::reverse | std::ranges::to<std::vector>();
    std::println("  ranges::to 造新的：          {}", rev);
}

// ============================================================
// [6] ostream_iterator：每次赋值做一次 os << x
//   std::ostream_iterator<int>(os)          不加分隔符：12345（书上的输出就是这样挤在一起）
//   std::ostream_iterator<int>(os, ", ")    每个后面加分隔符 —— 包括最后一个后面：1, 2, 3, 4, 5,
//     最后多一个分隔符是它的老毛病（练习 2 的 join 要处理这个）
//   这里写到 std::ostringstream（写到字符串里的流，<sstream>）而不是 cout，方便把结果打印出来看清楚
//   C++23 打印一个范围更简单：std::println("{}", v) 输出 [1, 2, 3]；"{:n}" 去掉方括号：1, 2, 3（没有多余的逗号）
// ============================================================
void section6()
{
    std::println("\n[6] ostream_iterator");
    std::deque<int> d1{1, 2, 3, 4, 5};

    std::ostringstream os1;
    std::ranges::copy(d1, std::ostream_iterator<int>(os1));
    std::println("  不加分隔符：  {:?}", os1.str()); // {:?} 打印带引号的字符串，看得出首尾有没有多余的东西

    std::ostringstream os2;
    std::ranges::copy(d1, std::ostream_iterator<int>(os2, ", "));
    std::println("  分隔符 \", \"：{:?}", os2.str());

    std::println("  C++23 {{}}：   {}", d1);
    std::println("  C++23 {{:n}}： {:n}", d1);
}

// ============================================================
// [7] istream_iterator：从流里一个一个读（用的是 >>，所以会跳过空白）
//   std::istream_iterator<T>(is)   造出来时就先读一个；*it 是读到的值；++it 再读下一个
//   std::istream_iterator<T>()     默认构造的是“结束迭代器”（哨兵）
//   书上写的 istream_adapter 是笔误，标准库里没有这个名字
//   什么时候和结束迭代器相等：某次读失败的时候（4.1 思考题 3 讲过它比较的是流指针）。读失败有两种：
//     到了流的结尾；或者读到的东西不是 T，比如读 int 碰到 "x"。第二种情况后面的内容就读不到了
//   书上从 cin 读，这里用 std::istringstream（从字符串读的流），不用手动输入
//
//   注意“最麻烦的解析”（most vexing parse）：
//     std::vector<int> v(std::istream_iterator<int>(in), std::istream_iterator<int>());   // 这是函数声明！
//   编译器把它当成“声明了一个叫 v 的函数”。用花括号 std::istream_iterator<int>{in} 就没有这个问题
// ============================================================
void section7()
{
    std::println("\n[7] istream_iterator");
    std::istringstream words{"this is not a haiku"};
    std::vector<std::string> vs;
    std::copy(std::istream_iterator<std::string>{words}, std::istream_iterator<std::string>{}, std::back_inserter(vs));
    std::println("  读单词：{}", vs);

    std::istringstream nums{"1 2 x 3"};
    std::vector<int> vi(std::istream_iterator<int>{nums}, std::istream_iterator<int>{}); // vector 的 (first, last) 构造
    std::println("  读 \"1 2 x 3\"：{}，碰到 x 就停了，nums.fail() 是 {}", vi, nums.fail());

    // C++20：std::views::istream<T>(is)，直接放进 range-for
    std::istringstream more{"10 20 30"};
    std::print("  views::istream：");
    for (int x : std::views::istream<int>(more))
    {
        std::print("{} ", x);
    }
    std::println("");
}

// ============================================================
// [8] 书上没讲：istreambuf_iterator 读“原样的字符”
//   istream_iterator<char> 用 >> 读 char，会跳过空格、换行：读 "a b\nc" 只得到 "abc"
//   istreambuf_iterator<char> 直接从流的缓冲区拿字符，一个不落，也更快
//   常见用法：把整个文件读进一个 string
//     std::ifstream f{"a.txt"};
//     std::string all(std::istreambuf_iterator<char>{f}, std::istreambuf_iterator<char>{});
// ============================================================
void section8()
{
    std::println("\n[8] istreambuf_iterator");
    std::istringstream a{"a b\nc"};
    std::string s1(std::istream_iterator<char>{a}, std::istream_iterator<char>{});
    std::istringstream b{"a b\nc"};
    std::string s2(std::istreambuf_iterator<char>{b}, std::istreambuf_iterator<char>{});
    std::println("  istream_iterator<char>：   {:?}", s1);
    std::println("  istreambuf_iterator<char>：{:?}", s2);
}

// ============================================================
// [9] 反向迭代器：++ 变成往回走
//   容器的 rbegin()、rend()：rbegin() 指向最后一个元素，rend() 指向“第一个元素的前面”
//   原生数组没有成员函数，用 std::rbegin(arr)、std::rend(arr)
//   C++20 更常用 std::views::reverse
//   4.7 节专门讲反向迭代器（它是怎么实现的、base() 为什么差一个位置）
// ============================================================
void section9()
{
    std::println("\n[9] 反向迭代器");
    std::deque<int> d1{1, 2, 3, 4, 5};
    std::print("  rbegin..rend：");
    for (auto it = d1.rbegin(); it != d1.rend(); ++it)
    {
        std::print("{} ", *it);
    }
    std::println("");

    int arr[]{10, 20, 30};
    std::vector<int> v(std::rbegin(arr), std::rend(arr));
    std::println("  std::rbegin(数组)：{}", v);
    std::println("  views::reverse：   {}", d1 | std::views::reverse);
}

// ============================================================
// [10] 书上没讲：move_iterator，把“复制”变成“移动”
//   std::make_move_iterator(it)：*it 得到的是右值（std::string&&），
//   于是用它构造或赋值时，调用的是移动构造 / 移动赋值：直接把字符串的内存“拿走”，不复制
//   移动之后，源里的字符串处于“有效但未指定”的状态：只能重新赋值或销毁，不要再读它的值
//     （实际上两个库都把它变成了空字符串，下面能看到，但标准不保证）
//   ranges 版本：std::ranges::move(src, std::back_inserter(dst))，效果一样
// ============================================================
void section10()
{
    std::println("\n[10] move_iterator");
    std::vector<std::string> src{"alpha", "beta", "a string that is too long for small string optimization"};
    std::vector<std::string> dst(std::make_move_iterator(src.begin()), std::make_move_iterator(src.end()));
    std::println("  dst：{}", dst);
    std::println("  src：{}（被移走了）", src);
}

// ============================================================
// [11] 这些适配器在 C++20 概念里是什么类别（接 4.4 节）
//   插入迭代器、ostream_iterator 是“输出迭代器”：只能往里写（*it = x），不能读
//     std::output_iterator<It, int>：It 能接收 int
//     C++20 起 back_insert_iterator 的 difference_type 是 std::ptrdiff_t（C++17 是 void），
//     所以满足 weakly_incrementable，ranges::copy 才接受它（4.4 节的表：缺 difference_type 连最低要求都不满足）
//   istream_iterator、istreambuf_iterator 是 input：只能走一遍
//   反向迭代器、移动迭代器：底下的迭代器是什么类别，它就是什么类别
// ============================================================
template <typename It>
void concept_row(std::string_view name)
{
    std::println("  {:<36} {:<6} {:<6} {:<6} {:<6}", name, std::output_iterator<It, int>, std::input_iterator<It>,
                 std::forward_iterator<It>, std::random_access_iterator<It>);
}

void section11()
{
    std::println("\n[11] 适配器的类别");
    std::println("  {:<36} {:<6} {:<6} {:<6} {:<6}", "", "output", "input", "fwd", "ra");
    concept_row<std::back_insert_iterator<std::vector<int>>>("back_insert_iterator<vector<int>>");
    concept_row<std::ostream_iterator<int>>("ostream_iterator<int>");
    concept_row<std::istream_iterator<int>>("istream_iterator<int>");
    concept_row<std::istreambuf_iterator<char>>("istreambuf_iterator<char>");
    concept_row<std::reverse_iterator<std::vector<int>::iterator>>("reverse_iterator<vector::iterator>");
    concept_row<std::move_iterator<std::vector<int>::iterator>>("move_iterator<vector::iterator>");
    static_assert(std::same_as<std::iter_difference_t<std::back_insert_iterator<std::vector<int>>>, std::ptrdiff_t>);
    // 反向迭代器的 output 是 true：*it 是 int&，能写。移动迭代器的 *it 是 int&&，不能往里写
}

int main()
{
    section1();
    section2();
    section3();
    section4();
    section5();
    section6();
    section7();
    section8();
    section9();
    section10();
    section11();
}
