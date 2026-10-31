// 第 1 章 1.8 模块 —— 练习：使用模块
// 改 stats.cppm 和 stats_impl.cpp，然后把下面的断言和 main 里的代码取消注释，两个编译器都能编译通过就说明做对了。
//
// clang++（std.pcm 在示例里已经编译过了，这里直接用）：
//   clang++ -std=c++23 -Wall -Wextra -fprebuilt-module-path=. --precompile stats.cppm -o stats.pcm
//   clang++ -std=c++23 -Wall -Wextra -fprebuilt-module-path=. stats.pcm stats_impl.cpp stats_exercise.cpp -o stats_exercise.exe
//
// g++：
//   g++ -std=c++23 -fmodules --compile-std-module -Wall -Wextra stats.cppm stats_impl.cpp stats_exercise.cpp -o stats_exercise_g.exe -lstdc++exp
import stats;
import std;

// 检测工具（已经写好，不用改）：stats::mean(vector<T>) 能不能调用
template <typename T>
concept CanMean = requires(std::vector<T> v) { stats::mean(v); };

// TODO 1
static_assert(stats::version == 2);

// TODO 2
static_assert(stats::Number<int>);
static_assert(!stats::Number<bool>);

// TODO 3
static_assert(stats::sum(std::vector{1, 2, 3, 4}) == 10.0);
static_assert(stats::mean(std::vector{1, 2, 3, 4}) == 2.5);
static_assert(stats::variance(std::vector{2, 4, 4, 4, 5, 5, 7, 9}) == 4.0);
static_assert(CanMean<int>);
static_assert(CanMean<double>);
static_assert(!CanMean<bool>); // 加 Number 约束之前，这个会失败
static_assert(!CanMean<std::string>);

int main()
{
    // TODO 4：预期输出 n=8 mean=5 sd=2
    std::println("{}", stats::report({2, 4, 4, 4, 5, 5, 7, 9}));

    // stats::square(3.0); // 这一行要一直保持注释：取消注释应该编译失败，因为 square 没有导出
}

// ============================================================
// 练习 5（思考题，把答案写在注释里）
//   (1) 示例 module_test.cpp 里，书上用 cout 输出 5.79，std::println 却输出 5.789999999999999。
//       哪个才是 1.23 + 4.56 在 double 里真正的值？两种输出方式的规则有什么不同？（回想 1.5 节 Frac 的问题）
//   (2) 把本文件的 import std; 删掉，只留 import stats;，编译一次。std::vector 和 std::println 还能用吗？为什么？
//       如果想让使用者 import stats 就自动拿到 std，stats.cppm 里该怎么写？你觉得这样做好不好？
//   (3) 用 clang++ 的命令来说：只改了 stats_impl.cpp 里 report 的实现，哪几条命令要重新执行？
//       如果改的是 stats.cppm 呢？
//   (4) 先删掉 gcm.cache 文件夹，再把 g++ 命令里的 stats.cppm 挪到最后面编译一次，会怎样？为什么？
//       不删 gcm.cache 直接这样编译，又会怎样？这说明了什么风险？
//
// 你的答案：
//   (1)后者，保留几位小数不一样
//       改动（补充）：方向对，但严格说两个都不是“真正的值”。double 里存的精确值是
//       5.789999999999999147348717087880...（用 %.30f 打印出来的），两种输出规则不同：
//         cout 默认只保留 6 位有效数字，四舍五入成 5.79；
//         std::println 输出“能唯一还原这个 double 的最短写法”，也就是 5.789999999999999。
//       5.79 其实是另一个 double（5.790000000000000035...），所以 1.23 + 4.56 == 5.79 是 false。
//       cout 的输出看着整齐，却把这个差别藏起来了。这和 1.5 节 Frac 用 double 比较出错是同一个原因。
//   (2)不能，import std只在内部可见，export import std;
//       改动（补充）：正确。“好不好”：一般不推荐。使用者用到了 std，却看不出 std 是从哪来的；
//       哪天 stats 不再需要 std，删掉这一行，使用者的代码就会编译失败。使用者自己写 import std; 更清楚。
//       export import 适合“打包模块”：比如一个 mylib 模块只做 export import mylib.math; export import mylib.io;
//   (3)不懂
//       改动（补充）：clang 的命令一共 3 条：①生成 std.pcm ②生成 stats.pcm ③编译 .cpp 并链接。
//       只改 stats_impl.cpp：只重新执行 ③。接口没变，stats.pcm 还是原来的，不用重新生成。
//       改了 stats.cppm：先执行 ② 重新生成 stats.pcm，再执行 ③。不执行 ②，③ 用的还是旧的接口。
//       ① 只有换编译器版本或换 -std 选项时才要重做。
//       （我们的 ③ 是一条命令编译所有 .cpp，所以三个文件总会一起重新编译。真实项目里每个 .cpp 单独 -c 成 .o，
//         只改实现文件时只有 stats_impl.o 需要重新编译 —— 这就是示例里说的“改实现不用重新编译使用者”。）
//   (4)我只用了clang
//       改动（补充）：我用 g++ 试过了：
//       删掉 gcm.cache 再把 stats.cppm 放到最后：编译失败，报 failed to read compiled module。
//         g++ 按顺序编译，编译 stats_impl.cpp 时 stats 模块还没编译，找不到 stats.gcm。
//       不删 gcm.cache：能编译通过，但用的是 gcm.cache 里上一次留下的旧 stats.gcm。
//         风险：如果你刚改了 stats.cppm，用的却是旧接口，会出现莫名其妙的错误，甚至编译通过但结果不对。
//       所以编译顺序很重要，大项目要交给 CMake（3.28 以上）这样的构建工具去排顺序。
// ============================================================
