// 第 1 章 1.5 三向比较运算符 <=> —— 练习
// 编译：clang++ -std=c++23 -Wall -Wextra spaceship_exercise.cpp -o spaceship_exercise.exe
// 用 static_assert 验证：补完 TODO，去掉断言的注释，能编译通过就说明做对了。
#include <algorithm>
#include <array>
#include <compare>
#include <string>

// ============================================================
// 练习 1（热身）：软件版本号 主版本.次版本.修订号，例如 1.2.10
//   要求：用一行 = default 让 Version 支持全部六个比较运算符
//   想一想：三个成员应该按什么顺序声明？
// ============================================================
struct Version
{
    int major;
    int minor;
    int patch;
    auto operator<=>(const Version&) const = default;
};

static_assert(Version{1, 2, 10} > Version{1, 2, 9}); // 注意：用花括号初始化时，值是按成员顺序填的
static_assert(Version{2, 0, 0} > Version{1, 99, 99});
static_assert(Version{1, 0, 0} == Version{1, 0, 0});
static_assert(Version{1, 0, 0} != Version{1, 0, 1});

// ============================================================
// 练习 2（自己写 <=>）：考试排名
//   排名规则：分数高的排前面；分数相同时，按名字字母顺序（A 在 B 前面）
//   “排前面”就是“更小”，这样 std::sort 排出来就是排名顺序
//   例如：{"Bob", 90} < {"Amy", 80}   （Bob 分高，排前面）
//         {"Amy", 90} < {"Bob", 90}   （同分，Amy 字母在前）
// 提示：
//   1. 默认的 <=> 不行：它会先比 name，而且分数是从小到大
//   2. 先比分数，不相等就直接返回；相等再比名字
//      分数要“从大到小”，想想 score <=> rhs.score 和 rhs.score <=> score 有什么区别
//   3. std::string 本身就支持 <=>
//   4. 要在 static_assert 里用，函数得是 constexpr（默认的 <=> 自动就是）
//   5. 写完 <=> 后，先只取消注释第三个断言（==），看看会发生什么
// ============================================================
struct Student
{
    std::string name;
    int score;

    constexpr std::strong_ordering operator<=>(const Student& rhs) const
    {
        if (auto cmp = (rhs.score <=> score); cmp != 0)
        {
            return cmp;
        }
        return name <=> rhs.name;
    }

    // 改动：去掉了 constexpr（= default 的 == 自动就是 constexpr，写上也不算错）和多余的空行
    bool operator==(const Student& rhs) const = default;
};

static_assert(Student{"Bob", 90} < Student{"Amy", 80});
static_assert(Student{"Amy", 90} < Student{"Bob", 90});
static_assert(Student{"Amy", 90} == Student{"Amy", 90});

constexpr bool ranking_ok()
{
    std::array<Student, 3> list{{{"Cat", 80}, {"Bob", 90}, {"Amy", 90}}};
    std::sort(list.begin(), list.end());
    return list[0].name == "Amy" && list[1].name == "Bob" && list[2].name == "Cat";
}

static_assert(ranking_ok());

// ============================================================
// 练习 3（思考题，把答案写在注释里）
//   (1) 练习 2 写完 <=> 后，为什么 == 还是编译不过？有哪两种办法解决？
//   (2) 示例 spaceship.cpp 里，Frac 的 <=> 为什么返回 weak_ordering 而不是 strong_ordering？
//       （提示：Frac{10, 15} 和 Frac{2, 3} “相等”，但它们真的一模一样吗？）
//   (3) 为什么 double 的 <=> 返回的是 partial_ordering？
//
// 你的答案：
//   (1)重载<=>可以default，或者自己在写一个==
//       改动（补充）：原因是只有 = default 的 <=> 才会顺带生成 ==，自己写的 <=> 不会。
//       但 Student 的排名规则和默认不同，<=> 不能 default，所以两种办法是：
//       ① bool operator==(const Student&) const = default;（你用的就是这个，逐个成员比较）
//       ② 自己手写 ==。
//       为什么这样设计：== 往往有更快的写法，比如两个字符串长度不同就能直接判定不相等，
//       而 <=> 必须逐字符比出大小。所以标准不拿你的 <=> 去凑 ==。
//       另外要保证 == 和 <=> 的判断一致（<=> 说相等时 == 也要说相等），否则排序、查找会出怪问题。
//   (2)成员n d的值不一样
//       改动（补充）：正确。strong_ordering 的“相等”意味着两个对象可以互相替换、完全一样；
//       10/15 和 2/3 只是值相等，分子分母不同（比如打印出来就不一样），所以只能算“等价”，用 weak。
//   (3)浮点数是nan和谁都没法比
//       改动（补充）：正确。只要存在“没法比”的值，就不是每一对都能分出大小，所以只能是 partial。
//       另外 0.0 和 -0.0 比较是相等的，但它们其实是两个不同的值，这也是为什么不能用 strong。
// ============================================================

int main()
{}
