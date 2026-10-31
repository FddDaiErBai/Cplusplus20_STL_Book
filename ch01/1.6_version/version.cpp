// 第 1 章 1.6 <version> 头文件 —— 特性测试宏
// 用三种标准分别编译运行，对比输出：
//   clang++ -std=c++17 -Wall -Wextra version.cpp -o v17.exe
//   clang++ -std=c++20 -Wall -Wextra version.cpp -o v20.exe
//   clang++ -std=c++23 -Wall -Wextra version.cpp -o v23.exe
// 注意：这个文件要能在 C++17 下编译，所以只用 printf 打印，不用 std::print
#include <cstdio>
#include <version> // 库特性测试宏（__cpp_lib_ 开头）都在这里

int main()
{
    // ------------------------------------------------------------
    // 1. __cplusplus：当前用的是哪个 C++ 标准
    //    201703L = C++17，202002L = C++20，202302L = C++23
    // ------------------------------------------------------------
    std::printf("__cplusplus = %ld\n\n", static_cast<long>(__cplusplus));

    // ------------------------------------------------------------
    // 2. 语言特性宏：编译器自己定义，不需要 #include
    // ------------------------------------------------------------
    std::printf("[语言特性]\n");
#ifdef __cpp_concepts
    std::printf("  __cpp_concepts                         = %ld\n", static_cast<long>(__cpp_concepts));
#else
    std::printf("  __cpp_concepts                         : 不支持\n");
#endif
#ifdef __cpp_impl_three_way_comparison
    std::printf("  __cpp_impl_three_way_comparison        = %ld\n", static_cast<long>(__cpp_impl_three_way_comparison));
#else
    std::printf("  __cpp_impl_three_way_comparison        : 不支持\n");
#endif

    // ------------------------------------------------------------
    // 3. 库特性宏：定义在 <version> 里（也会随对应的头文件一起出现）
    //    值是“年月”，表示标准委员会通过这个特性（或它的某次修订）的时间
    // ------------------------------------------------------------
    std::printf("\n[库特性]\n");
#ifdef __cpp_lib_format
    std::printf("  __cpp_lib_format                       = %ld\n", static_cast<long>(__cpp_lib_format));
#else
    std::printf("  __cpp_lib_format                       : 不支持\n");
#endif
#ifdef __cpp_lib_print
    std::printf("  __cpp_lib_print                        = %ld\n", static_cast<long>(__cpp_lib_print));
#else
    std::printf("  __cpp_lib_print                        : 不支持\n");
#endif
#ifdef __cpp_lib_constexpr_vector
    std::printf("  __cpp_lib_constexpr_vector             = %ld\n", static_cast<long>(__cpp_lib_constexpr_vector));
#else
    std::printf("  __cpp_lib_constexpr_vector             : 不支持\n");
#endif
#ifdef __cpp_lib_integer_comparison_functions
    std::printf("  __cpp_lib_integer_comparison_functions = %ld\n", static_cast<long>(__cpp_lib_integer_comparison_functions));
#else
    std::printf("  __cpp_lib_integer_comparison_functions : 不支持\n");
#endif
#ifdef __cpp_lib_three_way_comparison
    std::printf("  __cpp_lib_three_way_comparison         = %ld\n", static_cast<long>(__cpp_lib_three_way_comparison));
#else
    std::printf("  __cpp_lib_three_way_comparison         : 不支持\n");
#endif

    // ------------------------------------------------------------
    // 4. __has_include：只能说明“这个头文件存在”，不能说明“里面的功能能用”
    // ------------------------------------------------------------
    std::printf("\n[__has_include]\n");
#if __has_include(<format>)
    std::printf("  <format> 头文件存在\n");
#else
    std::printf("  <format> 头文件不存在\n");
#endif
}
