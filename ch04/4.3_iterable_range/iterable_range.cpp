// 第 4 章 4.3 创建可迭代范围：自己写一个能放进 range-for 的类
// 编译：clang++ -std=c++23 -Wall -Wextra -Wpedantic iterable_range.cpp -o iterable_range.exe
//       g++     -std=c++23 -Wall -Wextra -Wpedantic iterable_range.cpp -o iterable_range.exe -lstdc++exp
//
// ============================================================
// 要做的事：写一个类 Seq，让下面的代码能运行：
//     Seq<int> r{100, 105};
//     for (auto v : r) { ... }      // 依次得到 100 101 102 103 104
//
// range-for 需要什么（4.1 示例 [2] 讲过展开规则）：
//     auto begin_ = r.begin();      ① 类要有 begin() 和 end()
//     auto end_   = r.end();
//     for (; begin_ != end_; ++begin_)   ② 迭代器要支持 != 和前置 ++
//     {
//         auto v = *begin_;              ③ 迭代器要支持 *
//     }
//   只要这几样，别的都不需要（不需要后置 it++、不需要 ==、不需要 --）
//
// Seq 是一个“生成器”：100 到 104 这些数并没有存在任何地方，
// Seq 对象里只存了两个数 start_ = 100、end_ = 105，迭代器里只存了一个“当前的数”：
//
//     begin() 返回 iterator{100}，end() 返回 iterator{105}
//     *it  ：返回 it 里存的数
//     ++it ：把 it 里存的数加 1
//     it != end：比较两个迭代器里存的数
//
//     it 里的数    it != end ？      *it
//     100          100 != 105 是     100
//     101          101 != 105 是     101
//     ...
//     104          104 != 105 是     104
//     105          105 != 105 否     （循环结束）
//
//   要点：循环靠 != 停下来，所以 ++ 若干次之后必须“正好等于” end，否则永远停不下来（[3]）
//   标准库里早就有现成的：std::views::iota(100, 105)（[8]），这里是学它的原理
// ============================================================
#include <algorithm>
#include <print>
#include <ranges>
#include <string>
#include <string_view>
#include <vector>

// ------------------------------------------------------------
// [1] 书上的 Seq
//   迭代器是写在 Seq 里面的类（“嵌套类”），完整的类型名是 Seq<int>::iterator
//   书上第一段代码把返回类型写成 iterator<T>，是错的：iterator 不是模板，就叫 iterator；
//   而且 iterator 要先在类里声明，后面的 begin() 才能用它当返回类型
// ------------------------------------------------------------
template <typename T>
class Seq
{
    T start_{};
    T end_{};

public:
    Seq(T start, T end)
        : start_{start}, end_{end}
    {
    }

    class iterator
    {
        T value_{};

    public:
        // explicit：不允许 Seq<int>::iterator it = 5; 这种隐式转换，必须写 iterator{5}
        // 书上默认参数写 = 0，要求 T 能从 0 构造；写 T position = T{} 更通用（[3] 的 Seq2）
        explicit iterator(T position = 0)
            : value_{position}
        {
        }

        T operator*() const
        {
            return value_; // 返回的是一个值（复制品），不是引用：这个数本来就不存在别的地方（思考题 2）
        }

        iterator& operator++()
        {
            ++value_;
            return *this;
        }

        bool operator!=(const iterator& other) const
        {
            return value_ != other.value_;
        }
    };

    iterator begin() const
    {
        return iterator{start_};
    }

    iterator end() const
    {
        return iterator{end_};
    }
};

// ------------------------------------------------------------
// [3] 用：最多打印 limit 个，防止停不下来的循环一直跑
// ------------------------------------------------------------
template <typename R>
void print_first(std::string_view name, const R& r, int limit)
{
    std::print("  {:<22}", name);
    int n = 0;
    for (auto v : r)
    {
        if (n == limit)
        {
            std::print("……停不下来");
            break;
        }
        std::print("{} ", v);
        ++n;
    }
    if (n == 0)
    {
        std::print("（空）");
    }
    std::println("");
}

// ------------------------------------------------------------
// [3][4] 改进版 Seq2：
//   * 构造时如果 end < start，就把 end 改成 start，变成空范围（标准库的 iota 是要求调用者保证 start <= end）
//   * 默认参数 T{} 代替 0
//   * C++20：只写 operator==（甚至直接 = default），!= 自动就有了（[4]）
// ------------------------------------------------------------
template <typename T>
class Seq2
{
    T start_{};
    T end_{};

public:
    Seq2(T start, T end)
        : start_{start}, end_{end < start ? start : end}
    {
    }

    class iterator
    {
        T value_{};

    public:
        explicit iterator(T position = T{})
            : value_{position}
        {
        }

        T operator*() const
        {
            return value_;
        }

        iterator& operator++()
        {
            ++value_;
            return *this;
        }

        // = default：编译器自动生成，逐个比较所有成员（这里只有 value_）
        bool operator==(const iterator&) const = default;
    };

    iterator begin() const
    {
        return iterator{start_};
    }

    iterator end() const
    {
        return iterator{end_};
    }
};

// ------------------------------------------------------------
// [6] 实际中最常见的情况：类里面已经有一个标准容器，
//     begin()/end() 直接“转发”成员容器的 begin()/end()，不用自己写迭代器
//   要写两对：
//     const 版本：在 const Playlist& 上遍历时用（只读）
//     非 const 版本：在普通 Playlist 上遍历时用（可以通过迭代器修改元素）
//   返回类型写 auto，让编译器推导（const 版本里 songs_ 是 const 的，推导出来是 const_iterator，思考题 3）
//   好处：得到的是 vector 真正的迭代器，标准库的算法全都能直接用（[7] 对比 Seq）
// ------------------------------------------------------------
class Playlist
{
    std::vector<std::string> songs_;

public:
    void add(std::string song)
    {
        songs_.push_back(std::move(song));
    }

    auto begin()
    {
        return songs_.begin();
    }

    auto end()
    {
        return songs_.end();
    }

    auto begin() const
    {
        return songs_.begin();
    }

    auto end() const
    {
        return songs_.end();
    }
};

void print_playlist(const Playlist& p) // const 引用：用的是 const 版本的 begin()/end()
{
    std::print("  ");
    for (const auto& s : p)
    {
        std::print("{} | ", s);
    }
    std::println("");
}

int main()
{
    // ------------------------------------------------------------
    // [1] 书上的例子
    //   Seq r{100, 110} 不写 <int> 也行：类模板参数推导（2.5 节 CTAD），从 100 推导出 T = int
    // ------------------------------------------------------------
    std::println("[1] 书上的 Seq");
    {
        Seq<int> r{100, 110};
        std::print("  ");
        for (auto v : r)
        {
            std::print("{} ", v);
        }
        std::println("");

        Seq r2{'a', 'f'}; // T 推导为 char
        std::print("  ");
        for (auto c : r2)
        {
            std::print("{} ", c);
        }
        std::println("");

        Seq<int>::iterator it = r.begin(); // 完整的类型名（平时写 auto 就行）
        std::println("  *r.begin() = {}", *it);
        // Seq<int>::iterator it2 = 5;     // 编译错误：构造函数是 explicit 的
    }

    // ------------------------------------------------------------
    // [2] 手动展开 range-for，看清楚 Seq 被调用了哪几个函数
    //   书上又写成了 std::begin(container)，规则见 4.1 示例 [2]：类有成员 begin() 就用成员
    // ------------------------------------------------------------
    std::println("\n[2] 手动展开");
    {
        Seq<int> r{1, 4};
        auto begin_ = r.begin();      // Seq::begin()
        auto end_ = r.end();          // Seq::end()
        std::print("  ");
        for (; begin_ != end_; ++begin_) // iterator::operator!=、iterator::operator++
        {
            auto v = *begin_;         // iterator::operator*
            std::print("{} ", v);
        }
        std::println("");
    }

    // ------------------------------------------------------------
    // [3] 陷阱：++ 若干次之后永远“正好等于” end 不了
    //   Seq<int>{110, 100}：从 110 往上加，永远到不了 100，
    //     一直加到 int 的最大值后溢出（有符号整数溢出是未定义行为），程序就乱了
    //   Seq<double>{0.5, 3.0}：0.5 1.5 2.5 3.5 ……永远不会正好等于 3.0
    //   书上完全没提。下面每个最多打印 6 个
    //   Seq2 修好了第一个（end < start 时变成空范围），但第二个还是停不下来：
    //   用“值 != 终点值”判断结束，只在“从起点一步步 +1 正好能走到终点”时才对。
    //   彻底的办法是迭代器里数“第几个”，而不是比较值（练习 1）
    // ------------------------------------------------------------
    std::println("\n[3] 停不下来的范围");
    print_first("Seq{100, 104}", Seq{100, 104}, 6);
    print_first("Seq{110, 100}", Seq{110, 100}, 6);
    print_first("Seq{0.5, 3.0}", Seq{0.5, 3.0}, 6);
    print_first("Seq2{110, 100}", Seq2{110, 100}, 6);
    print_first("Seq2{0.5, 3.0}", Seq2{0.5, 3.0}, 6);

    // ------------------------------------------------------------
    // [4] C++20 的比较运算符“改写”
    //   C++20 起，写了 a == b，编译器遇到 a != b 时会自动改写成 !(a == b)
    //   所以 Seq2 只写了 operator==，range-for 里的 != 照样能用
    //   反过来不行：只写 operator!=（书上的写法），a == b 是编译错误，编译器不会把 == 改写成 !=
    //     Seq<int>::iterator a{1}, b{1};
    //     a == b;    // clang：error: invalid operands to binary expression
    //   所以 C++20 起应该写 ==，不要只写 !=（4.4 节会看到，标准库算法也需要 ==）
    // ------------------------------------------------------------
    std::println("\n[4] 只写 == 就有 !=");
    {
        Seq2<int>::iterator a{1};
        Seq2<int>::iterator b{2};
        std::println("  a == b：{}，a != b：{}（!= 是编译器用 == 改写出来的）", a == b, a != b);
    }

    // ------------------------------------------------------------
    // [5] 迭代器也可以只是一个指针
    //   类里如果是原生数组，begin()/end() 直接返回指针就行：指针本身就是迭代器（4.1 示例 [1]）
    // ------------------------------------------------------------
    std::println("\n[5] 用指针当迭代器");
    {
        struct Dice
        {
            int faces[6]{1, 2, 3, 4, 5, 6};

            const int* begin() const
            {
                return faces;
            }

            const int* end() const
            {
                return faces + 6;
            }
        };

        Dice d;
        std::print("  ");
        for (int f : d)
        {
            std::print("{} ", f);
        }
        std::println("");
    }

    // ------------------------------------------------------------
    // [6] 转发成员容器的 begin()/end()
    // ------------------------------------------------------------
    std::println("\n[6] 转发成员容器的迭代器");
    {
        Playlist p;
        p.add("Yesterday");
        p.add("Bohemian");
        p.add("Africa");
        print_playlist(p);

        for (auto& s : p) // 非 const 版本：auto& 拿到的是 std::string&，可以修改
        {
            s += "!";
        }
        std::ranges::sort(p); // vector 的迭代器是随机访问的，sort 直接能用
        print_playlist(p);
        auto it = std::ranges::find(p, "Bohemian!");
        std::println("  find 找到：{}", it != p.end());
    }

    // ------------------------------------------------------------
    // [7] Seq 能放进 range-for，但标准库还不认它是“范围”
    //   std::ranges::range<R> 要求 begin() 返回的东西是一个“迭代器”（至少 input_or_output_iterator），
    //   而那要求迭代器有 difference_type 类型、支持后置 it++ 等，Seq::iterator 都没有
    //   所以 std::ranges::max(Seq{1, 5})、std::minmax_element(r.begin(), r.end()) 都编译不过：
    //     clang：note: because 'Seq<int>' does not satisfy 'input_range'
    //   下一节 4.4 就是把这些补上
    // ------------------------------------------------------------
    std::println("\n[7] 标准库认不认");
    {
        using It = Seq<int>::iterator;
        std::println("  Seq<int>   ：input_or_output_iterator {}，ranges::range {}",
                     std::input_or_output_iterator<It>, std::ranges::range<Seq<int>>);
        std::println("  Playlist   ：ranges::range {}，random_access_range {}",
                     std::ranges::range<Playlist>, std::ranges::random_access_range<Playlist>);
    }

    // ------------------------------------------------------------
    // [8] 标准库现成的生成器：std::views::iota（<ranges>）
    //   iota(a, b) 生成 a, a+1, ..., b-1，迭代器是随机访问的，算法全都能用
    //   要隔几个取一个，C++23 有 views::stride(n)：每 n 个取第一个
    //   实际写代码时优先用它们，自己写 Seq 是为了理解原理
    // ------------------------------------------------------------
    std::println("\n[8] std::views::iota");
    {
        std::print("  iota(100, 105)：");
        for (int v : std::views::iota(100, 105))
        {
            std::print("{} ", v);
        }
        std::println("");

        std::print("  iota(0, 10) | stride(3)：");
        for (int v : std::views::iota(0, 10) | std::views::stride(3))
        {
            std::print("{} ", v);
        }
        std::println("");
        std::println("  ranges::max(iota(1, 5)) = {}", std::ranges::max(std::views::iota(1, 5)));
    }
}
