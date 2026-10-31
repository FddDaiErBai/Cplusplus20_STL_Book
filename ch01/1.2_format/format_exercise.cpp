// 第 1 章 1.2 格式化文本 —— 练习
// 编译：clang++ -std=c++23 -Wall -Wextra format_exercise.cpp -o format_exercise.exe
// 这个文件现在就能编译运行，把每个 TODO 补完，让输出和注释里的“期望输出”一致。
#include <format>
#include <print>
#include <string>
#include <vector>

// ============================================================
// 练习 1（热身）：只改格式字符串，不改参数
// 格式说明：{ 序号 : 填充 对齐 宽度 .精度 类型 }
// ============================================================
void exercise1()
{
	std::println("[{:03}]", 7);           // 期望：[007]       改动：{:0>3} -> {:03}，数字补零要用这个，负数才正确
	std::println("[{:.2f}]", 3.14159);     // 期望：[3.14]
	std::println("[{:#X}]", 255);          // 期望：[0XFF]
	std::println("[{:*^8}]", "hi");        // 期望：[***hi***]
	std::println("[{:.2e}]", 1234567.891); // 期望：[1.23e+06]
	std::println("[{:0>3}] [{:03}]", -7, -7); // 改动：新增，对比两种写法 -> [0-7] [-07]
}

// ============================================================
// 练习 2（实用）：打印一张对齐的价格表
// 期望输出（列宽：名称 12 左对齐，单价 8，数量 6，小计 10，都右对齐）：
//
// Item           Price   Qty  Subtotal
// apple           3.50     4     14.00
// banana          0.25    12      3.00
// watermelon     12.80     1     12.80
// ------------------------------------
// Total                          29.80
//
// 提示：分隔线可以用 std::println("{:-<36}", "") 打印，想想为什么
// ============================================================
struct Item
{
	std::string name;
	double price;
	int qty;
};

void exercise2()
{
	std::vector<Item> items{{"apple", 3.5, 4}, {"banana", 0.25, 12}, {"watermelon", 12.8, 1}};

	// 改动：去掉了 {: <12} 里的空格，默认填充字符就是空格
	std::println("{:<12}{:>8}{:>6}{:>10}", "Item", "Price", "Qty", "Subtotal");

	double total{};
	for (const auto& it : items)
	{
		double sub = it.price * it.qty;
		total += sub;
		std::println("{:<12}{:>8.2f}{:>6}{:>10.2f}", it.name, it.price, it.qty, sub);
	}

	std::println("{:-<36}", "");
	std::println("{:<12}{:>24.2f}", "Total", total);
}

// ============================================================
// 练习 3（进阶）：让 Frac 支持两种格式
//   {}   或 {:d}  ->  5/3
//   {:f}          ->  1.667   （保留 3 位小数）
//   {:x}          ->  编译报错（不支持的格式）
//
// 思路：
//   1. 给 formatter 加一个成员变量 mode，记住用户要哪种格式
//   2. parse 里：ctx.begin() 指向冒号后的第一个字符
//      - 如果是 'd' 或 'f'，存进 mode，迭代器往后走一格
//      - 走完后，迭代器必须指向 '}'（或已到 ctx.end()），否则 throw std::format_error("...")
//      - 返回这个迭代器
//   3. format 里：根据 mode 选择输出方式
//      注意 n / d 是整数除法，要先转成 double
// ============================================================
struct Frac
{
	long n;
	long d;
};

template <>
struct std::formatter<Frac>
{
	char mode{'d'}; // 'd' 输出分数，'f' 输出小数

	constexpr auto parse(std::format_parse_context& ctx)
	{
		auto it{ctx.begin()};
		// 改动 1：取值前先检查 it != ctx.end()
		// 改动 2：只在遇到 d / f 时修改 mode，遇到 '}' 什么都不做（默认值已经是 'd'）
		if (it != ctx.end() && (*it == 'd' || *it == 'f'))
		{
			mode = *it;
			++it;
		}
		// 改动 3：读完后必须正好停在 '}'，否则 {:x}、{:df} 都在这里报错
		if (it != ctx.end() && *it != '}')
		{
			throw std::format_error("Frac 只支持 {:d} 和 {:f}");
		}
		return it;
	}

	auto format(const Frac& f, std::format_context& ctx) const
	{
		// 改动 4：mode 只可能是 'd' 或 'f'，只需判断一种
		if (mode == 'f')
		{
			// 改动 5：只转换一个操作数，另一个会自动变成 double
			return std::format_to(ctx.out(), "{:.3f}", static_cast<double>(f.n) / f.d);
		}
		return std::format_to(ctx.out(), "{}/{}", f.n, f.d);
	}
};

void exercise3()
{
	Frac f{5, 3};
	std::println("{}", f);   // 期望：5/3
	std::println("{:d}", f); // 期望：5/3
	std::println("{:f}", f); // 期望：1.667
	// std::println("{:x}", f);  // 取消注释后应该“编译失败”
	// std::println("{:df}", f); // 改动：新增，现在也由你的 parse 报错
}

int main()
{
	std::println("--- 练习 1 ---");
	exercise1();
	std::println("--- 练习 2 ---");
	exercise2();
	std::println("--- 练习 3 ---");
	exercise3();
}
