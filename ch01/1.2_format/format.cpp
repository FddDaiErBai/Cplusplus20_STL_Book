// 第 1 章 1.2 格式化文本：std::format / std::print 练习
#include <format>
#include <numbers>
#include <print>
#include <string>

// 自定义类型：分数
struct Frac
{
	long n;
	long d;
};

// 让 std::format 认识 Frac
template <>
struct std::formatter<Frac>
{
	// 不支持额外的格式说明，直接返回起点
	constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }

	// 注意：新版标准库要求 format() 是 const 成员函数（书中版本没有 const）
	auto format(const Frac& f, std::format_context& ctx) const
	{
		return std::format_to(ctx.out(), "{}/{}", f.n, f.d);
	}
};

int main()
{
	std::string who{"everyone"};
	int ival{42};
	double pi{std::numbers::pi};

	std::print("Hello, {}!\n", who); // 基本占位符
	std::print("Hello {1} {0}\n", ival, who); // 指定参数顺序
	std::print("[{:.<10}]\n", ival); // 左对齐，用 . 填充
	std::print("[{:.>10}]\n", ival); // 右对齐
	std::print("[{:.^10}]\n", ival); // 居中
	std::print("{:.5}\n", pi); // 5 位有效数字
	std::print("{:#x} {:#b}\n", 255, 5); // 十六进制 / 二进制

	std::string s = std::format("Frac: {}", Frac{5, 3});
	std::println("{}", s);
}
