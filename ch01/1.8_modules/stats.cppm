// 第 1 章 1.8 模块 —— 练习：模块接口单元
// 这个文件原本是一个头文件 stats.h，刚改成模块，还没改完。按 TODO 把它改成一个合格的模块。
// 编译方法见 stats_exercise.cpp 开头
export module stats;

import std;

// ============================================================
// TODO 1：头文件时代，版本号是这样提供给使用者的。但宏不会从模块里导出，
//   import stats 的人看不到 STATS_VERSION。
//   把它改成 stats 命名空间里一个导出的编译期常量 version，值为 2，然后删掉这个宏
// ============================================================

namespace stats
{
    export constexpr int version{2};
    // ============================================================
    // TODO 2：导出 Number 概念（1.7 节练习写过的，已经写好）
    // ============================================================
    export template <typename T>
    concept Number = std::floating_point<T> || (std::integral<T> && !std::same_as<T, bool>);

    // 内部辅助函数：使用者不应该能调用它，保持不导出
    constexpr double square(double x)
    {
        return x * x;
    }

    // ============================================================
    // TODO 3：导出 sum / mean / variance 三个函数，并把 typename T 改成用 Number 约束
    //   导出方式随你：可以一个一个写 export，也可以用 export { } 块
    // ============================================================
    export
    {
        template <Number T>
        constexpr double sum(const std::vector<T>& v)
        {
            double total = 0.0;
            for (const T& x : v)
            {
                total += x;
            }
            return total;
        }

        template <Number T>
        constexpr double mean(const std::vector<T>& v)
        {
            return sum(v) / v.size();
        }

        // 方差：每个数与平均数之差的平方，再求平均
        template <Number T>
        constexpr double variance(const std::vector<T>& v)
        {
            const double m = mean(v);
            double total = 0.0;
            for (const T& x : v)
            {
                total += square(x - m);
            }
            return total / v.size();
        }
    }

    // ============================================================
    // TODO 4：在这里导出一个函数声明（只写声明，不写函数体）：
    //     std::string report(const std::vector<double>& v);
    //   函数体写到实现单元 stats_impl.cpp 里
    // ============================================================
    export std::string report(const std::vector<double>& v);
}
