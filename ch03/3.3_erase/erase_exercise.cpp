// 第 3 章 3.3 使用擦除函数从容器中删除元素 —— 练习
// 编译：clang++ -std=c++23 -Wall -Wextra erase_exercise.cpp -o erase_exercise.exe
// 练习 1~3 用 static_assert 在编译时检查：补完 TODO，去掉断言的注释，能编译通过就说明做对了。
// 练习 4 用的 map 不能在编译时用，需要运行一次看输出。
#include <algorithm>
#include <cstddef>
#include <map>
#include <print>
#include <string>
#include <vector>

// ============================================================
// 练习 1：用 C++20 以前的写法（erase-remove）删除所有等于 value 的元素，返回删除了几个
//   要求：用 std::remove 和 v.erase 两步完成，不用 std::erase
//   提示：std::remove 返回的迭代器后面那段就是要删除的，它的长度就是删除的个数
// ============================================================
constexpr std::size_t remove_value_old(std::vector<int>& v, int value)
{
    auto it = std::remove(v.begin(), v.end(), value);
    const auto size = static_cast<std::size_t>(v.end() - it);
    v.erase(it, v.end());
    return size;
}

constexpr bool test1()
{
    std::vector v{1, 5, 2, 5, 5};
    auto n = remove_value_old(v, 5);
    return n == 3 && v == std::vector{1, 2};
}

constexpr bool test1_none()
{
    std::vector v{1, 2, 3};
    auto n = remove_value_old(v, 9);
    return n == 0 && v == std::vector{1, 2, 3};
}

static_assert(test1());
static_assert(test1_none());

// ============================================================
// 练习 2：找 bug（1）
//   remove_like_first 想做的事：删除所有和第一个元素相等的元素，返回删除了几个；v 是空的就返回 0。
//   取消断言的注释，第一个断言失败。找出原因并修改
// ============================================================
constexpr std::size_t remove_like_first(std::vector<int>& v)
{
    if (v.empty())
    {
        return 0;
    }
    return std::erase(v, auto(v.front()));
}

constexpr bool test2()
{
    std::vector v{1, 2, 1, 3};
    auto n = remove_like_first(v);
    return n == 2 && v == std::vector{2, 3};
}

constexpr bool test2_empty()
{
    std::vector<int> v;
    return remove_like_first(v) == 0;
}

static_assert(test2());
static_assert(test2_empty());

// ============================================================
// 练习 3：找 bug（2）
//   erase_evens 想做的事：删除所有偶数。
//   要求：保留“自己写循环 + v.erase(it)”的写法，不要换成 std::erase_if（练习的目的是学会这个循环，
//         std::erase_if 对 map、set 内部就是这样写的）
//   取消断言的注释，编译失败。读报错信息，找出原因并修改（参考示例第 6 部分）
// ============================================================
constexpr void erase_evens(std::vector<int>& v)
{
    for (auto it = v.begin(); it != v.end();)
    {
        if ((*it) % 2 == 0)
        {
            it = v.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

constexpr bool test3()
{
    std::vector v{2, 4, 5, 6};
    erase_evens(v);
    return v == std::vector{5};
}

constexpr bool test3_odd()
{
    std::vector v{1, 3};
    erase_evens(v);
    return v == std::vector{1, 3};
}

static_assert(test3());
static_assert(test3_odd());

// ============================================================
// 练习 4：map + erase_if
//   删除库存少于 min 的商品，返回删除了几种。用 std::erase_if 写（一两行就够了）
//   提示：map 的元素是 pair<const std::string, int>，库存是 .second
//   写完后编译运行，看 main 里打印的是“通过”还是“没通过”
// ============================================================
std::size_t prune(std::map<std::string, int>& stock, int min)
{
    std::size_t n = std::erase_if(stock, [min](const auto& item)
    {
        return item.second < min;
    });
    return n;
}

// ============================================================
// 练习 5（思考题，把答案写在注释里）
//   (1) 运行示例 erase.cpp，看第 [1] 部分 string 版本的输出：
//         words 原来是 {"apple", "x", "banana", "x"}，remove 之后变成了 {"apple", "banana", "", "x"}。
//       为什么第 3 个变成了空字符串，而最后一个 "x" 却原样留着？
//       （提示：remove 是一个一个往前“挪”的，想想 "banana" 是从哪里挪到哪里的）
//   (2) 关联容器（map、set）只有 std::erase_if，没有 std::erase(m, ...)。为什么不需要？
//   (3) 示例第 [3] 部分，list 删除元素后，之前保存的迭代器还能用。
//       如果换成 vector{0, 1, 2, 3, 4, 5}，保存一个指向 3 的迭代器，再 erase_if 删掉偶数，
//       这个迭代器还能用吗？为什么 list 可以、vector 不行？
//
// 你的答案：
//   (1)banana从第三位挪到第二位，而第4位的x不需要挪动
//       改动（补充）：方向对，还差“为什么第 3 位变成空的”。关键是 remove 用的是“移动”，不是“复制”：
//         下标:   0        1      2         3
//         开始: "apple"  "x"   "banana"   "x"
//         ① 下标 0 "apple" 要留，不动
//         ② 下标 1 "x" 要删，跳过
//         ③ 下标 2 "banana" 要留，把它“移动”到下标 1。移动 = 把字符串的内容直接“搬走”，不另外复制一份，
//            所以搬完之后下标 2 剩下一个被掏空的 string，一般就是空字符串 ""
//         ④ 下标 3 "x" 要删，跳过 —— 没人碰过它，所以原样留着
//       移动比复制快（不用重新分配内存、拷贝字符），代价就是源对象被“掏空”。
//       这就是“有效但未指定”：空字符串也是个合法的 string，只是标准不保证它是什么内容。
//   (2)不知道匹配的是first还是second的值
//       改动（补充）：这个角度有道理（map 的元素是 pair，按值删确实说不清比什么），但主要原因是：
//         关联容器本来就有成员函数 m.erase(key)，按键删除，O(log n)，还返回删了几个。
//         set 的元素就是键，所以 s.erase(5) 就是“删掉所有等于 5 的”；multiset / multimap 会删掉全部同键的。
//         而 std::erase(c, value) 是“从头到尾扫一遍比较”，O(n)，比成员函数慢，所以没必要提供。
//         vector 没有“按值删除”的成员函数，才需要 std::erase。
//   (3)不知道
//       改动（补充）：不能用了。区别在于元素在内存里怎么放：
//         vector：所有元素挨着放在一整块内存里。删掉中间一个，后面的元素要整体往前挪，填上空位。
//           vector{0,1,2,3,4,5} 删掉偶数后变成 {1,3,5}，3 从下标 3 挪到了下标 1。
//           之前保存的迭代器还指着下标 3 的位置，而现在 size 只有 3，下标 3 已经是 end() 了 —— 迭代器失效。
//           标准规定：vector 的 erase 会让“被删位置及其后面”的所有迭代器、引用失效。
//         list：每个元素单独放在一个“节点”里，节点之间用指针连起来。删掉一个节点只是把它前后两个节点的指针
//           接起来，其他节点在内存里一动不动，所以指向它们的迭代器都还有效（只有被删的那个失效）。
//       这也是 3.1 节表格里“list 已知位置时增删 O(1)”的原因。代价是 list 不能用下标、遍历慢（节点分散在内存里）。
// ============================================================

int main()
{
    std::map<std::string, int> stock{{"apple", 3}, {"banana", 0}, {"cherry", 1}, {"durian", 7}};
    auto n = prune(stock, 2);
    bool ok = n == 2 && stock == std::map<std::string, int>{{"apple", 3}, {"durian", 7}};
    std::println("练习 4：{}（删除了 {} 种，剩下 {}）", ok ? "通过" : "没通过", n, stock);
}
