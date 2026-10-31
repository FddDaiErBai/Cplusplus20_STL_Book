// 第 3 章 3.6 保持 vector 元素的顺序 —— 练习
// 编译：clang++ -std=c++23 -Wall -Wextra sorted_vector_exercise.cpp -o sorted_vector_exercise.exe
// 用 static_assert 在编译时检查：补完 TODO，去掉断言的注释，能编译通过就说明做对了。
#include <algorithm>
#include <functional>
#include <string_view>
#include <vector>

// ============================================================
// 练习 1：不重复的有序插入
//   v 已经从小到大排好序，而且没有重复元素。把 x 插入到正确的位置，保持有序；
//   如果 x 已经存在，就不插入。插入了返回 true，没插入返回 false
//   要求：只用一次 std::ranges::lower_bound（不要再调用 find、binary_search 之类的）
//   提示：lower_bound 返回第一个 >= x 的位置。拿到这个位置后，怎么判断“x 已经在里面了”？
//         想一想 x 比所有元素都大的时候，这个位置是哪里
// ============================================================
constexpr bool insert_unique(std::vector<int>& v, int x)
{
    const auto pos = std::ranges::lower_bound(v, x);
    if (pos != v.end() && *pos == x)
    {
        return false;
    }
    v.insert(pos, x);
    return true;
}

// 改动（补充）：完全正确，边界也处理对了。可以再精简：insert(end(), x) 本身就等于 push_back，
//   所以 end 不用单独处理，只要把“检查 end”和“检查相等”用 && 连起来（&& 左边不成立时右边不会执行）：
//     const auto pos = std::ranges::lower_bound(v, x);
//     if (pos != v.end() && *pos == x)
//     {
//         return false;
//     }
//     v.insert(pos, x);
//     return true;

constexpr bool test1()
{
    std::vector v{10, 20, 30};
    bool a = insert_unique(v, 25);  // 中间
    bool b = insert_unique(v, 20);  // 已存在
    bool c = insert_unique(v, 5);   // 最前面
    bool d = insert_unique(v, 99);  // 最后面
    return a && !b && c && d && v == std::vector{5, 10, 20, 25, 30, 99};
}

constexpr bool test1_empty()
{
    std::vector<int> v;
    return insert_unique(v, 7) && !insert_unique(v, 7) && v == std::vector{7};
}

static_assert(test1());
static_assert(test1_empty());

// ============================================================
// 练习 2：按优先级插入任务，同优先级“先来的排前面”
//   tasks 已经按 priority 从小到大排好序。把 t 插入到正确的位置，要求：
//     - 按 priority 保持从小到大
//     - priority 相同的，先插入的排在前面（新来的排在同优先级的最后）
//   要求：用 projection（&Task::priority），不要自己写 lambda 比较
//   提示：示例第 [2]、[3] 部分。lower_bound 和 upper_bound 用哪个？
// ============================================================
struct Task
{
    std::string_view name;
    int priority;

    bool operator==(const Task&) const = default;
};

constexpr void add_task(std::vector<Task>& tasks, const Task& t)
{
    tasks.insert(std::ranges::upper_bound(tasks, t.priority, {}, &Task::priority), t);
}

constexpr bool test2()
{
    std::vector<Task> tasks;
    add_task(tasks, {"写报告", 2});
    add_task(tasks, {"回邮件", 1});
    add_task(tasks, {"开会", 2});
    add_task(tasks, {"喝水", 1});
    add_task(tasks, {"修 bug", 0});
    return tasks == std::vector<Task>{{"修 bug", 0}, {"回邮件", 1}, {"喝水", 1}, {"写报告", 2}, {"开会", 2}};
}

static_assert(test2());

// ============================================================
// 练习 3：找 bug —— 降序的排行榜
//   scores 从大到小排好序。add_score 想把 s 插入到正确的位置，保持从大到小。
//   取消断言的注释，编译失败。找出原因并修改（只需要改一行）
// ============================================================
constexpr void add_score(std::vector<int>& scores, int s)
{
    scores.insert(std::ranges::lower_bound(scores, s, std::ranges::greater{}), s);
}

constexpr bool test3()
{
    std::vector scores{90, 70, 50};
    add_score(scores, 80);
    add_score(scores, 40);
    add_score(scores, 100);
    return scores == std::vector{100, 90, 80, 70, 50, 40};
}

static_assert(test3());

// ============================================================
// 练习 4（思考题，把答案写在注释里）
//   (1) 有 n 个乱序的数，想得到一个有序的 vector。两种做法：
//         A：每来一个数就 insert_sorted 一次
//         B：全部 push_back，最后 std::ranges::sort 一次
//       哪个快？大概是多少（用 O(...) 表示，提示：insert 一次是 O(n)，sort 一次是 O(n log n)）
//       什么情况下只能用 A？
//   (2) 练习 1 里，如果判断“x 已存在”时不检查 pos != v.end()，哪个断言会出问题？会出什么问题？
//       （可以真的删掉这个检查，编译一次看看报错，再改回来）
//   (3) 示例第 [3] 部分插入 Eve 时用的是 upper_bound。如果换成 lower_bound，输出会变成什么样？
//       （可以真的改一下，运行看看）
//
// 你的答案：
//   (1)我不会算时间复杂度
//       改动（补充）：O(...) 只是粗略地说“数据量 n 变大时，工作量按什么规律变大”：
//         O(n)：n 翻倍，工作量翻倍；O(n²)：n 翻倍，工作量变 4 倍；O(n log n)：比 O(n) 多一点点。
//         A：第 k 次插入，平均要挪 k/2 个元素。总共 1/2 + 2/2 + … + n/2 ≈ n²/4 次移动 → O(n²)
//         B：push_back n 次是 O(n)，sort 一次 O(n log n)，合起来 O(n log n)
//         代入 n = 100 万：A 约 2.5×10^11 次移动，B 约 2×10^7 次比较，差一万倍 —— B 快得多。
//       只能用 A 的情况：数据是一个一个来的，而且两次插入之间就要用到“有序”（比如每来一个数就要查一次排名）。
//         如果是“先全部收集完，再一起用”，就用 B。
//   (2)第一个，解引用了一个end()
//       改动（补充）：原因对，但两个断言都会失败：
//         test1()：插入 99 时，99 比所有元素都大，pos == end()，*pos 越界
//         test1_empty()：空 vector 里 begin() == end()，第一次插入 7 时 pos 就是 end()
//       clang 的报错分别是 “read of object outside its lifetime” 和 “dereferencing a null pointer”。
//   (3)我这里是一样的
//       改动（补充）：改成 lower_bound 后，输出应该是 Amy(20) Eve(30) Tom(30) Bob(30)，Eve 跑到同岁的人前面了。
//         lower_bound 找第一个 age >= 30 的位置，也就是 Tom 前面；upper_bound 找第一个 age > 30 的位置，也就是末尾。
//         你看到“一样”，多半是改了 .cpp 但没有重新编译，运行的还是旧的 exe（目录里 sorted_vector.exe 的时间比 .cpp 早）。
//         改完代码一定要先编译再运行。
// ============================================================

int main()
{
}
