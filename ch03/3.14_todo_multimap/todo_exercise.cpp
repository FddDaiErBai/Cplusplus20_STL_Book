// 第 3 章 3.14 用 multimap 做待办事项列表 —— 练习
// 编译：clang++ -std=c++23 -Wall -Wextra todo_exercise.cpp -o todo_exercise.exe
// 运行时检查：补完 TODO，编译运行，所有行都显示“通过”就说明做对了。
// 先看示例 todo.cpp 开头那张 map 和 multimap 的对比表。
#include <algorithm> // 改动：find_if 在这里
#include <cstddef>
#include <functional>
#include <iterator>
#include <map>
#include <optional>
#include <print>
#include <ranges>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

void check(bool ok, std::string_view name)
{
    std::println("{} {}", ok ? "通过" : "失败", name);
}

// ============================================================
// 练习 1：待办清单类
//   用 std::multimap<int, std::string, std::greater<>> 存：优先级从高到低，同一优先级先加的在前（示例 [2]）
//     add(p, task)        ：加一个任务
//     pop_next()          ：取出并删除“下一个要做的”—— 优先级最高的里面最先加的那个；清单是空的返回 std::nullopt
//     tasks_at(p)         ：优先级为 p 的所有任务，按加入的先后顺序；没有就返回空的 vector
//     drop(p)             ：删掉优先级为 p 的所有任务，返回删了几个
//     size()              ：一共还有几个任务（已经写好了）
//   提示：
//     - 因为用了 greater<>，“下一个要做的”就是 begin() 指向的那个
//     - tasks_at 用 equal_range（示例 [3]），把 [first, second) 里每个元素的 second（任务名）放进 vector
//     - drop 一行就够：multimap 的 erase(键) 删掉所有这个键的元素，并返回删了几个（示例 [3]）
// ============================================================
class TodoList
{
public:
    void add(int priority, std::string task)
    {
        //todo_.insert({priority, task});
        todo_.emplace(priority, std::move(task));
    }

    // 改动（补充）：能用，但 task 被复制了一次。task 是按值传进来的参数，已经是一份自己的副本了；
    //   可它是有名字的变量（左值），{priority, task} 构造 pair 时不会自动移动，而是又复制一遍（3.8 思考题 (2)）。
    //   写成 todo_.emplace(priority, std::move(task)); 就是移动，不再复制。

    std::optional<std::string> pop_next()
    {
        if (!todo_.empty()) // 改动：去掉了 && !todo_.begin()->second.empty()（理由见函数下面）
        {
            //std::string next{todo_.begin()->second};
            std::string next = std::move(todo_.begin()->second);
            todo_.erase(todo_.begin());
            return next;
        }
        return std::nullopt;
    }

    // 改动（补充）：原来多了一个条件 !todo_.begin()->second.empty()（“任务名不是空字符串”）。
    //   题目没有说任务名不能为空。如果有人 add(5, "")，这个任务排在最前面，pop_next 看到名字是空的，
    //   直接返回 nullopt，而且不删它 —— 它永远堵在最前面，后面的任务一个都取不出来了。
    //   “清单是空的”只需要 todo_.empty() 判断。我在 test1 最后加了一项测试，你原来的代码会显示“失败”。
    //   另外：string next{todo_.begin()->second}; 是复制。反正马上要删掉这个元素，可以“偷”过来：
    //       std::string next = std::move(todo_.begin()->second);   // 值不是 const 的，可以移动（键才是 const）
    //       todo_.erase(todo_.begin());

    std::vector<std::string> tasks_at(int priority) const
    {
        std::vector<std::string> result{};
        auto [first, last] = todo_.equal_range(priority);
        for (const auto& [p, task] : std::ranges::subrange(first, last))
        {
            result.push_back(task);
        }
        return result;
    }

    // 改动（补充）：正确。if (first == last) return {}; 可以去掉：范围是空的时，下面的 for 一次都不执行，
    //   result 本来就是空的。subrange 要 <ranges>，你在开头已经加了，很好。

    std::size_t drop(int priority)
    {
        return todo_.erase(priority);
    }

    std::size_t size() const
    {
        return todo_.size();
    }

private:
    std::multimap<int, std::string, std::greater<>> todo_;
};

void test1()
{
    TodoList t;
    t.add(1, "dishes");
    t.add(3, "homework");
    t.add(1, "laundry");
    t.add(0, "comics");
    t.add(3, "exam");
    t.add(1, "trash");

    check(t.tasks_at(1) == std::vector<std::string>{"dishes", "laundry", "trash"}, "练习 1：tasks_at(1)，按加入的顺序");
    std::println("  （得到 {}）", t.tasks_at(1));
    check(t.tasks_at(2).empty(), "练习 1：tasks_at(2) 没有");

    auto a = t.pop_next();
    auto b = t.pop_next();
    check(a == "homework" && b == "exam" && t.size() == 4, "练习 1：pop_next 先出优先级 3，同级先加的先出");

    check(t.drop(1) == 3 && t.size() == 1, "练习 1：drop(1) 删掉 3 个");
    check(t.drop(1) == 0, "练习 1：再 drop(1) 一个都没有");

    auto c = t.pop_next();
    auto d = t.pop_next();
    check(c == "comics" && !d.has_value() && t.size() == 0, "练习 1：取完了返回 nullopt");

    // 改动：新加的测试 —— 名字为空字符串的任务也是一个任务
    TodoList e;
    e.add(5, "");
    e.add(1, "after");
    auto e1 = e.pop_next();
    auto e2 = e.pop_next();
    check(e1 == "" && e2 == "after" && e.size() == 0, "练习 1：任务名是空字符串");
}

// ============================================================
// 练习 2：找 bug —— 改优先级
//   change_priority(todo, old_p, task, new_p)：把“优先级为 old_p、名字为 task”的那个任务改成优先级 new_p。
//   找到并改好返回 true；找不到（没有这个优先级，或者这个优先级里没有这个名字）返回 false，清单保持不变。
//   改了优先级的任务，在新的优先级里排在最后（相当于刚加进去）
//   运行后有两项失败。找出原因并修改。编译时还有一个 unused parameter 警告 —— 它也是线索
//   提示：同一个优先级可能有好几个任务。find(old_p) 找到的是哪一个？找到之后检查过名字吗？
//         示例开头的表：find(k) 返回键为 k 的“某一个”；想看键为 k 的“全部”，用什么？
// ============================================================
using Todo = std::multimap<int, std::string, std::greater<>>;

bool change_priority(Todo& todo, int old_p, const std::string& tasks, int new_p)
{
    auto [first, last] = todo.equal_range(old_p);
    auto pos{
        std::ranges::find_if(std::ranges::subrange(first, last), [&](const auto& t)
        {
            return t.second == tasks;
        })
    };
    if (pos != last)
    {
        auto node = todo.extract(pos);
        node.key() = new_p;
        todo.insert(std::move(node));
        return true;
    }
    return false;
}

// 改动（补充）：正确。bug 就是原来只用 find(old_p) 拿“某一个”，根本没看名字（所以 task 参数没用到，有警告）。
//   你用 equal_range 圈出这一组，再在组里 find_if 名字 —— 正是要的写法。
//   find_if 在 <algorithm> 里，我在开头补上了。

void test2()
{
    const Todo start{{2, "homework"}, {1, "dishes"}, {1, "laundry"}, {1, "trash"}, {0, "comics"}};

    Todo t1 = start;
    check(change_priority(t1, 2, "homework", 0) && t1 == Todo{{1, "dishes"}, {1, "laundry"}, {1, "trash"}, {0, "comics"}, {0, "homework"}},
          "练习 2：唯一的一个");

    Todo t2 = start;
    check(change_priority(t2, 1, "laundry", 3) && t2 == Todo{{3, "laundry"}, {2, "homework"}, {1, "dishes"}, {1, "trash"}, {0, "comics"}},
          "练习 2：同一优先级里的第二个");
    std::println("  （得到 {}）", t2);

    Todo t3 = start;
    check(!change_priority(t3, 1, "homework", 5) && t3 == start, "练习 2：这个优先级里没有这个名字");
    std::println("  （得到 {}）", t3);

    Todo t4 = start;
    check(!change_priority(t4, 7, "dishes", 0) && t4 == start, "练习 2：没有这个优先级");
}

// ============================================================
// 练习 3：书上那种“从小到大”的 multimap，怎么打印出正确的顺序
//   假设你拿到的是书上那种默认的 std::multimap<int, std::string>（键从小到大，不能改类型）。
//   示例 [1] 说明了：用 rbegin/rend 倒着遍历，优先级是从高到低了，但同一个优先级里的任务也倒过来了。
//   写 in_order(m)：返回所有任务名，优先级从高到低；同一个优先级里，按原来的顺序（先加的在前）
//     {0: watch, 0: read, 1: wash, 2: homework}  →  ["homework", "wash", "watch", "read"]
//   思路：一组一组地处理。从最大的键开始，每次：
//     ① 找到当前还没处理的“最大的键” k
//     ② 用 equal_range(k) 拿到这一组 [first, second)，正着把这一组的任务名放进结果
//     ③ 下一组就是 first 前面的那些元素
//   提示：
//     - 用一个迭代器 end_of_rest 表示“还没处理的部分是 [begin(), end_of_rest)”，一开始是 m.end()
//     - 还没处理的部分里，最大的键就是最后一个元素的键：std::prev(end_of_rest)->first
//       （std::prev(it)：it 往前挪一个的位置，<iterator>）
//     - 处理完一组，end_of_rest = 这一组的 first；end_of_rest == m.begin() 时就全部处理完了
// ============================================================
std::vector<std::string> in_order(const std::multimap<int, std::string>& m)
{
    //if (m.empty())
    //{
    //    return {};
    //}
    std::vector<std::string> result{};
    for (auto end_of_rest{m.end()}; end_of_rest != m.begin();)
    {
        auto [group_first, group_last] = m.equal_range(std::prev(end_of_rest)->first);

        for (auto it{group_first}; it != group_last; ++it)
        {
            result.push_back(it->second);
        }

        end_of_rest = group_first;
    }

    return result;
}

// 改动（补充）：完全正确。开头的 if (m.empty()) return {}; 可以去掉：m 为空时 m.end() == m.begin()，
//   for 一次都不执行，直接返回空的 result。
//   题目说明写得太绕了，这里用图把你的代码走一遍。m = {0: watch, 0: read, 1: wash, 2: homework}：
//
//       位置：   [0]       [1]      [2]      [3]          end
//       元素：  0:watch   0:read   1:wash   2:homework
//
//   第 1 轮：end_of_rest = end
//            prev(end_of_rest) 是 [3]，键 2 → equal_range(2) = [3, end) → 放入 homework
//            end_of_rest = [3]            （[3] 以及后面的都处理完了）
//   第 2 轮：prev([3]) 是 [2]，键 1 → equal_range(1) = [2, 3) → 放入 wash
//            end_of_rest = [2]
//   第 3 轮：prev([2]) 是 [1]，键 0 → equal_range(0) = [0, 2) → 正着放入 watch、read
//            end_of_rest = [0] = begin() → 结束
//   结果：homework, wash, watch, read
//
//   一句话：组和组之间从后往前跳（每次跳到这一组的开头），组里面从前往后读。

void test3()
{
    const std::multimap<int, std::string> m{{1, "wash"}, {0, "watch"}, {2, "homework"}, {0, "read"}};
    auto r = in_order(m);
    check(r == std::vector<std::string>{"homework", "wash", "watch", "read"}, "练习 3：书上的例子");
    std::println("  （得到 {}）", r);

    const std::multimap<int, std::string> m2{{5, "a"}, {5, "b"}, {5, "c"}, {-1, "x"}, {9, "y"}, {-1, "z"}};
    check(in_order(m2) == std::vector<std::string>{"y", "a", "b", "c", "x", "z"}, "练习 3：负数、一组好几个");
    check(in_order({}).empty(), "练习 3：空的");
}

// ============================================================
// 练习 4（思考题，把答案写在注释里）
//   (1) 运行示例，看 [1]：初始化时 watch teevee 写在 read comics 前面，为什么 rprint 先打印 read comics？
//       [2] 用了 greater<>，为什么就变成 watch teevee 在前了？对待办清单来说，哪种更合理？
//   (2) multimap 为什么没有 m[k] 和 m.at(k)？为什么它的 insert 只返回一个迭代器，
//       而 map 的 insert 要返回 pair<迭代器, bool>（3.7 节）？
//   (3) 示例 [5] 里，为了把 priority_queue 的内容全部打印出来，先复制了一份 copy 再一个个 pop。
//       为什么不直接 for (auto& x : pq) 遍历？为什么要先复制？
//       另外，read comics 先放进去，为什么却是 watch teevee 先出来？
//
// 你的答案：
//   (1)因为整体反向遍历了，正着排列，从大到小。后者更合理。
//       改动（补充）：对，再说具体一点：multimap 里同键的元素按插入顺序排，从前往后是 watch、read。
//         rbegin/rend 把整个序列倒过来，不光是“组和组之间”倒了，组里面也倒了，于是 read 在前。
//         greater<> 改变的是“键怎么比大小”，键大的排在前面；同键的元素仍按插入顺序，正着遍历就是 watch、read。
//   (2)一个键可以有很多的值，因为允许重复的键，map的insert可能遇到存在的键，会插入失败。
//       改动（补充）：两半都对。第二半只说了 map 那边，补上 multimap 这边：multimap 的 insert
//         永远成功（键重复也照样插入），没有“失败”这种结果，所以不需要 bool，只返回指向新元素的迭代器。
//   (3)不copy只能看到最大的。
//       改动（补充）：只答对了一点点。三个问题分开说：
//         为什么不能 for 遍历：priority_queue 没有 begin()/end()，range-for 根本编译不过。它只开放 top() 一个元素
//         为什么要复制：想看第二个，只能先 pop 掉第一个。pop 会把元素真的删掉，
//           直接对 pq 一个个 pop 完，pq 就空了。先复制一份，pop 的是副本，原来的 pq 不受影响（示例最后一行：还有 4 个）
//         为什么 watch teevee 先出来：priority_queue 每次给出“最大”的那个。元素是 pair，
//           pair 比较时先比 first，一样再比 second。0 一样，就比 "watch teevee" 和 "read comics"：
//           'w' 比 'r' 大，所以 watch teevee 更“大”，先出来 —— 跟谁先放进去没有关系。
//           所以 priority_queue 不适合“同级先来先做”的待办清单，multimap + greater<> 才适合。
// ============================================================

int main()
{
    test1();
    test2();
    test3();
}
