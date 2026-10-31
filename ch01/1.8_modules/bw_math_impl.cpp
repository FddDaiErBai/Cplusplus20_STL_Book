// 第 1 章 1.8 模块 —— 模块实现单元（implementation unit）
// 开头写 module bw_math;（没有 export），表示“这个文件属于 bw_math 模块”
// 它自动能看到接口单元里的所有东西，包括没导出的 secret()
module bw_math;

import std;

namespace bw
{
    double average(const std::vector<double>& v)
    {
        return std::ranges::fold_left(v, 0.0, std::plus{}) / v.size();
    }
}
