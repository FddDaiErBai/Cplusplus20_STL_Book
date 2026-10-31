// 第 1 章 1.3 constexpr —— 编译时使用 vector 和 string
// 编译：clang++ -std=c++23 -Wall -Wextra constexpr.cpp -o constexpr.exe
#include <algorithm>
#include <array>
#include <numeric>
#include <print>
#include <string>
#include <vector>

// ------------------------------------------------------------
// 1. constexpr 函数：能在编译时算，也能在运行时算
// ------------------------------------------------------------
constexpr int square(int x)
{
	return x * x;
}

// ------------------------------------------------------------
// 2. C++20 新能力：constexpr 函数里可以用 string 和 vector
// ------------------------------------------------------------
constexpr auto use_string()
{
	std::string str{"string"};
	return str.size(); // 返回的是 size_t，不是 string 本身
}

constexpr auto use_vector()
{
	std::vector<int> vec{1, 2, 3, 4, 5};
	return std::accumulate(vec.begin(), vec.end(), 0); // 算法也能在编译时用
}

constexpr std::vector<int> make_vector()
{
	return {1, 2, 3, 4, 5};
}

// ------------------------------------------------------------
// 3. 实用套路：编译时用 vector 计算，结果拷进 std::array 带到运行时
//    例子：编译时求出 n 以内的所有质数
// ------------------------------------------------------------
constexpr std::vector<int> primes_below(int n)
{
	std::vector<int> result;
	for (int i = 2; i < n; ++i)
	{
		bool is_prime = std::none_of(result.begin(), result.end(), [i](int p) { return i % p == 0; });
		if (is_prime)
		{
			result.push_back(i);
		}
	}
	return result;
}

template <int N>
constexpr auto primes_array()
{
	constexpr auto count = primes_below(N).size(); // 第一次算：只要个数（vector 用完就释放）
	std::array<int, count> arr{};
	auto v = primes_below(N); // 第二次算：拿到内容
	std::copy(v.begin(), v.end(), arr.begin());
	return arr; // array 不需要动态内存，可以带到运行时
}

int main()
{
	// 1. 编译时计算：结果在编译期就已确定，static_assert 可以检查
	constexpr int a = square(5);
	static_assert(a == 25);

	// 1. 同一个函数也能在运行时调用（参数不是常量时）
	int input = 7;
	int b = square(input);
	std::println("square(5) = {}，square({}) = {}", a, input, b);

	// 2. string / vector 在编译时使用
	constexpr auto len = use_string();
	constexpr auto sum = use_vector();
	static_assert(len == 6);
	static_assert(sum == 15);
	std::println("use_string() = {}，use_vector() = {}", len, sum);

	// 2. 编译时分配的内存必须在编译时释放：
	// constexpr auto vec = make_vector();       // 错误！vector 的内存不能“漏”到运行时
	constexpr auto size = make_vector().size(); // 正确：只把 size 这个数字带出来
	std::println("make_vector().size() = {}", size);

	// 3. 编译时算好的质数表
	constexpr auto primes = primes_array<50>();
	static_assert(primes.size() == 15);
	std::println("50 以内的质数：{}", primes); // C++23 可以直接打印容器
}
