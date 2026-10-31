// 第 1 章 1.8 模块 —— 练习：模块实现单元
module stats;

import std;

namespace stats
{
    // ============================================================
    // TODO 4（续）：实现 report，返回这样的字符串（用 std::format）：
    //     n=8 mean=5 sd=2
    //   n 是元素个数，mean 是平均数，sd 是标准差（方差开平方，用 std::sqrt）
    //   提示：实现单元能直接用接口单元里的所有东西，包括没导出的
    // ============================================================
    std::string report(const std::vector<double>& v)
    {
        return std::format("n={} mean={} sd={}", v.size(), mean(v), std::sqrt(variance(v)));
    }
}
