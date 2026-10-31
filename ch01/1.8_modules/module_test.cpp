// 第 1 章 1.8 模块 —— 使用模块
// 模块要先编译，才能被 import，所以编译顺序有要求：先接口单元，再其他文件
//
// clang++（3 步）：
//   第 1 步，把标准库编译成模块 std.pcm（只需要做一次，换 -std 选项才要重做）：
//     clang++ -std=c++23 -Wno-reserved-module-identifier --precompile C:/Dev/llvm-mingw/share/libc++/v1/std.cppm -o std.pcm
//   第 2 步，编译接口单元，生成 bw_math.pcm（编译好的模块接口）：
//     clang++ -std=c++23 -Wall -Wextra -fprebuilt-module-path=. --precompile bw_math.cppm -o bw_math.pcm
//   第 3 步，编译其余文件并链接（bw_math.pcm 也要写上，它里面有 hello() 等函数的代码）：
//     clang++ -std=c++23 -Wall -Wextra -fprebuilt-module-path=. bw_math.pcm bw_math_impl.cpp module_test.cpp -o module_test.exe
//
// g++（1 步，接口单元要写在前面）：
//     g++ -std=c++23 -fmodules --compile-std-module -Wall -Wextra bw_math.cppm bw_math_impl.cpp module_test.cpp -o module_test.exe -lstdc++exp
//   g++ 会把编译好的模块放进 gcm.cache 文件夹
import bw_math;
import std; // C++23 标准库模块：一行拿到整个标准库（书上的 std.core 是 MSVC 早期的实验写法）

// 宏不会从模块里泄漏出来
#ifdef BW_SECRET
#error "BW_SECRET 泄漏了"
#endif

int main()
{
    double f = bw::add(1.23, 4.56);
    int i = bw::add(7, 42);
    std::string s = bw::add<std::string>("one ", "two");

    std::println("double: {}", f); // 书上用 cout 输出 5.79，这里会输出 5.789999999999999，为什么？
    std::println("int: {}", i);
    std::println("string: {}", s);

    std::println("safe_sqrt(2) = {}", bw::safe_sqrt(2.0));
    std::println("answer()     = {}", bw::answer());
    std::println("average      = {}", bw::average({1.0, 2.0, 3.0, 4.0}));

    bw::Point p{3, 4};
    std::println("Point        = ({}, {})", p.x, p.y);

    hello(); // 全局命名空间里的函数

    // bw::secret(); // 取消注释：编译失败，secret 没有导出
}
