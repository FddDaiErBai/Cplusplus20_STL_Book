// 第 3 章 3.4 常数时间内从未排序的 vector 中删除元素 —— 练习
// 编译：clang++ -std=c++23 -Wall -Wextra quick_delete_exercise.cpp -o quick_delete_exercise.exe
// 用 static_assert 在编译时检查：补完 TODO，去掉断言的注释，能编译通过就说明做对了。
#include <algorithm>
#include <cstddef>
#include <utility>
#include <vector>

// ------------------------------------------------------------
// 练习里用的类型：Tracked 在“自己移动给自己”时会 throw。
// 在编译时执行到 throw，编译器会直接报错（not a constant expression），
// 所以只要你的代码发生了自我移动，static_assert 就编译不过
// ------------------------------------------------------------
struct Tracked
{
    int value;

    constexpr Tracked(int v) : value{v}
    {
    }

    Tracked(const Tracked&) = default;
    Tracked(Tracked&&) = default;
    Tracked& operator=(const Tracked&) = default;

    constexpr Tracked& operator=(Tracked&& other)
    {
        if (this == &other)
        {
            throw "自我移动！";
        }
        value = other.value;
        return *this;
    }

    bool operator==(const Tracked&) const = default;
};

// ============================================================
// 练习 1：更安全的 quick_delete
//   和示例一样，用“最后一个元素补位”的方法删除下标 idx 的元素，额外要求：
//     (a) idx 越界时什么都不做，返回 false；删除成功返回 true
//     (b) 要删的就是最后一个元素时，不能自我移动（直接 pop_back 就行）
//   先写最简单的版本（照抄示例），取消断言的注释，看看哪个断言失败、报错信息是什么，再改
// ============================================================
template <typename T>
constexpr bool quick_delete(std::vector<T>& v, std::size_t idx)
{
    if (idx >= v.size())
    {
        return false;
    }
    if (idx == v.size() - 1)
    {
        v.pop_back();
        return true;
    }

    v[idx] = std::move(v.back());
    v.pop_back();
    return true;
}

constexpr bool test1_middle()
{
    std::vector<Tracked> v{1, 2, 3, 4};
    return quick_delete(v, 1) && v == std::vector<Tracked>{1, 4, 3};
}

constexpr bool test1_last()
{
    std::vector<Tracked> v{1, 2, 3};
    return quick_delete(v, 2) && v == std::vector<Tracked>{1, 2};
}

constexpr bool test1_only()
{
    std::vector<Tracked> v{7};
    return quick_delete(v, 0) && v.empty();
}

constexpr bool test1_out_of_range()
{
    std::vector<Tracked> v{1, 2};
    std::vector<Tracked> empty;
    return !quick_delete(v, 2) && v == std::vector<Tracked>{1, 2} && !quick_delete(empty, 0);
}

static_assert(test1_middle());
static_assert(test1_last());
static_assert(test1_only());
static_assert(test1_out_of_range());

// ============================================================
// 练习 2：找 bug —— 不保持顺序的 erase_if
//   quick_erase_if 想做的事：删除所有满足 pred 的元素，不在乎顺序，返回删除了几个。
//   做法：从前往后看，遇到要删的就用最后一个元素补位。
//   取消断言的注释，test2_even 失败。找出原因并修改
//   提示：在纸上走一遍 {2, 4, 5, 6} 删偶数，每一步写下 i 和 v 的内容
// ============================================================
template <typename T, typename Pred>
constexpr std::size_t quick_erase_if(std::vector<T>& v, Pred pred)
{
    std::size_t count = 0;
    // 改动：原来开头有一个特殊判断 if (i == 0 && v.size() == 1) { v.pop_back(); ++count; continue; }
    //   它没有检查 pred，只剩一个元素时不管满不满足条件都删掉了：{1} 删偶数，结果把 1 删了，返回 1。
    //   题目给的三个断言里，只剩一个元素时它正好都是偶数，所以没测出来（下面新增了断言）。
    // 改动：另外，“要删的正好是最后一个元素”时还是会自我移动（不只是 size == 1 的情况）：
    //   {2, 4, 5, 6} 走到最后剩 {5, 4}，i = 1，v[1] = std::move(v[1])。int 没事，换成 Tracked 就 throw 了。
    // 两个问题都交给练习 1 写好的 quick_delete 处理，这里就不用特殊判断了
    for (std::size_t i = 0; i < v.size();)
    {
        if (pred(v[i]))
        {
            quick_delete(v, i);
            ++count;
        }
        else
        {
            ++i;
        }
    }
    return count;
}

constexpr bool is_even(int x)
{
    return x % 2 == 0;
}

constexpr bool test2_even()
{
    std::vector v{2, 4, 5, 6};
    auto n = quick_erase_if(v, is_even);
    return n == 3 && v == std::vector{5};
}

constexpr bool test2_mixed()
{
    // 顺序不确定，所以用 is_permutation 比较：“元素一样，顺序可以不同”
    std::vector v{1, 2, 3, 4, 5, 6, 7, 8};
    auto n = quick_erase_if(v, is_even);
    return n == 4 && std::ranges::is_permutation(v, std::vector{1, 3, 5, 7});
}

constexpr bool test2_all()
{
    std::vector v{2, 4, 6};
    return quick_erase_if(v, is_even) == 3 && v.empty();
}

static_assert(test2_even());
static_assert(test2_mixed());
static_assert(test2_all());

// 改动（新增）：原写法这两个都编译不过
static_assert([]
{
    std::vector v{1}; // 只剩一个元素，而且不该删
    return quick_erase_if(v, is_even) == 0 && v == std::vector{1};
}());
static_assert([]
{
    std::vector<Tracked> v{1, 2}; // 要删的 2 是最后一个元素：原写法自我移动，throw
    auto n = quick_erase_if(v, [](const Tracked& t)
    {
        return t.value % 2 == 0;
    });
    return n == 1 && v == std::vector<Tracked>{1};
}());

// ============================================================
// 练习 3（思考题，把答案写在注释里）
//   (1) 练习 2 的 bug 是什么？你改了哪里？
//       另外，你改完的版本在“要删的正好是最后一个元素”时，会不会自我移动？
//       （提示：把练习 2 的 int 换成 Tracked 试一试；一定要改的话，可以用练习 1 的函数）
//   (2) 示例第 [2] 部分：10 个元素删第一个，erase 移动了 9 次，quick_delete 只移动 1 次。
//       那如果要一次删掉很多元素（比如 1000 个里删 500 个），quick_erase_if 和 std::erase_if
//       都只需要从头到尾走一遍。这时候 quick_erase_if 还有优势吗？
//       （提示：std::erase_if 遇到要留下的元素就要往前挪；quick_erase_if 什么时候才挪动？）
//   (3) 举一个不能用 quick_delete 的场景（除了“要求元素有序”之外，再想一个）
//       提示：如果程序别的地方保存了“第 5 个元素是张三”这种下标……
//
// 你的答案：
//   (1)删除了第一个元素之后，直接跳到下一个了，不管从末尾移动过来的元素是否满足pred。我将其修改成，只有pred不满足时才跳到下一个元素。不会自我移动。
//       改动（补充）：bug 找得对，改法也对。但“不会自我移动”不对：你的特殊判断只处理了“只剩一个元素”，
//         而“要删的是最后一个、但前面还有别的元素”也会自我移动，比如 {1, 2} 删偶数，i = 1 时
//         v[1] = std::move(v[1])。题目提示让你换成 Tracked 试一试，就是为了发现这个（见上面新增的断言）。
//   (2)遇到不满足pred的时候
//       改动（补充）：反了。quick_erase_if 是在“遇到要删的”时才挪动（把最后一个搬过来补位），
//         std::erase_if 是在“遇到要留下的”时挪动（从第一个被删的位置开始，后面每个要留的都往前挪一格）。
//         所以：quick_erase_if 移动次数 ≈ 删除的个数；std::erase_if 移动次数 ≈ 第一个被删元素后面要留下的个数。
//           1000 个只删开头 1 个：erase_if 移动 999 次，quick 移动 1 次 —— quick 优势巨大
//           1000 个删 500 个、均匀分布：erase_if 约 500 次，quick 约 500 次 —— 差不多，优势基本没了
//         而且 std::erase_if 保持顺序、从前往后顺序读写内存（对缓存友好），所以批量删除时一般直接用 std::erase_if；
//         quick_delete 的主场是“每次只删一个”。
//   (3)不知道
//       改动（补充）：先说明一下，我给的提示不太准确 —— 外面保存的下标，用 erase 删也会失效（后面的全部往前挪一格），
//         这不是 quick_delete 独有的问题。准确地说：
//         erase 删掉下标 i 后，i 后面所有元素的下标都变了；
//         quick_delete 删掉下标 i 后，只有“原来最后那个元素”的下标变了（从 size-1 变成 i）。
//         所以如果外面保存了下标，quick_delete 反而更好维护：只要把搬家的那一个元素的下标改成 i 就行。
//         游戏引擎里管理大量对象就是这么做的（swap-and-pop + 更新被搬动对象的下标）。
//       真正不能用 quick_delete 的场景，基本都是“位置本身有含义”：
//         - 排好序的 vector，后面要用 binary_search / lower_bound 查找（换一个元素进来，顺序就乱了）
//         - 队列、历史记录、撤销列表这类“先后顺序就是数据”的东西
//         - 正在用范围 for 遍历这个 vector 的时候（不管哪种删法，边遍历边删都会出问题）
// ============================================================

int main()
{
}
