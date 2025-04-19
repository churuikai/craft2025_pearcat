/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 *  ██████╗  █████╗ ████████╗ █████╗      █████╗ ███╗   ██╗ █████╗ ██╗  ██╗   ██╗███████╗██╗███████╗
 *  ██╔══██╗██╔══██╗╚══██╔══╝██╔══██╗    ██╔══██╗████╗  ██║██╔══██╗██║  ╚██╗ ██╔╝██╔════╝██║██╔════╝
 *  ██║  ██║███████║   ██║   ███████║    ███████║██╔██╗ ██║███████║██║   ╚████╔╝ ███████╗██║███████╗
 *  ██║  ██║██╔══██║   ██║   ██╔══██║    ██╔══██║██║╚██╗██║██╔══██║██║    ╚██╔╝  ╚════██║██║╚════██║
 *  ██████╔╝██║  ██║   ██║   ██║  ██║    ██║  ██║██║ ╚████║██║  ██║███████╗██║   ███████║██║███████║
 *  ╚═════╝ ╚═╝  ╚═╝   ╚═╝   ╚═╝  ╚═╝    ╚═╝  ╚═╝╚═╝  ╚═══╝╚═╝  ╚═╝╚══════╝╚═╝   ╚══════╝╚═╝╚══════╝
 * 
 * ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 * 【模块功能】
 * ┌─────────────────┬───────────────────────────────────────────────────────────┐
 * │ 频率分析         │ 处理和分析各类操作的频率数据                                │
 * ├─────────────────┼───────────────────────────────────────────────────────────┤
 * │ 标签排序         │ 计算和维护标签的最优排序                                    │
 * ├─────────────────┼───────────────────────────────────────────────────────────┤
 * │ 相似度计算       │ 实现曲线相似度的计算和标签聚类                              │
 * └─────────────────┴───────────────────────────────────────────────────────────┘
 * ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/

#include "constants.h"          // ⟪常量定义⟫
#include "data_analysis.h"      // ⟪数据分析相关⟫
#include "debug.h"              // ⟪调试工具⟫
#include "ctrl_disk_obj_req.h"  // ⟪控制器、磁盘、对象、请求相关⟫
#include <cmath>                // ⟪数学函数⟫
#include <algorithm>            // ⟪算法函数⟫
#include <climits>              // ⟪系统限制常量⟫
#include <numeric>              // ⟪数值算法⟫

/*╔══════════════════════════════ 全局数据结构 ══════════════════════════════╗*/
// ◇ 频率数据结构
std::vector<std::vector<std::vector<int>>> FRE;              // [tag][slice][op_type] 操作频率
std::vector<std::vector<int>> SORTED_READ_TAGS;              // [timestamp][tag_index] 预排序标签
std::vector<std::vector<int>> OBJ_COUNT;                     // [tag][slice_idx] 对象数量
/*╚═════════════════════════════════════════════════════════════════════════╝*/

/*╔══════════════════════════════ 频率查询接口 ══════════════════════════════╗
 * ┌──────────────────────────────────────────────────────────────────────┐
 * │ 功能：获取指定标签在特定时间点的操作频率                                 │
 * │ 参数：                                                                │
 * │ - tag: 标签ID                                                         │
 * │ - timestamp: 时间戳                                                   │
 * │ - op_type: 操作类型 (0删除，1写入，2读取)                              │
 * └──────────────────────────────────────────────────────────────────────┘
 */
int get_freq(int tag, int timestamp, int op_type)
{
    int slice_idx = (timestamp + FRE_PER_SLICING - 1) / FRE_PER_SLICING;
    return FRE[tag][slice_idx][op_type];
}

/*╔══════════════════════════════ 标签排序接口 ══════════════════════════════╗
 * ┌──────────────────────────────────────────────────────────────────────┐
 * │ 功能：获取指定时间点的已排序标签列表                                     │
 * │ 参数：                                                                │
 * │ - timestamp: 时间戳                                                   │
 * │ 返回：按读取频率排序的标签列表                                          │
 * └──────────────────────────────────────────────────────────────────────┘
 */
std::vector<int> &get_sorted_read_tag(int timestamp)
{
    int slice_idx = std::min((timestamp + FRE_PER_SLICING - 1) / FRE_PER_SLICING, 
                            (int)SORTED_READ_TAGS.size() - 1);
    return SORTED_READ_TAGS[slice_idx];
}

/*╔════════════════════════════ 数据分析初始化 ═══════════════════════════════╗
 * ┌──────────────────────────────────────────────────────────────────────┐
 * │ 功能：初始化并处理系统数据分析                                         │
 * │ 步骤：                                                               │
 * │ 1. 初始化频率和对象数量数据结构                                        │
 * │ 2. 读取各类操作频率数据                                               │
 * │ 3. 计算对象数量变化                                                   │
 * │ 4. 预计算标签排序                                                     │
 * └──────────────────────────────────────────────────────────────────────┘
 */
void process_data_analysis()
{
    // ◆ 初始化数据结构
    FRE.resize(MAX_TAG_NUM + 1, 
               std::vector<std::vector<int>>((MAX_SLICING_NUM + 1) / FRE_PER_SLICING + 1, 
               std::vector<int>(3, 0)));
    OBJ_COUNT.resize(MAX_TAG_NUM + 1, 
                    std::vector<int>((MAX_SLICING_NUM + 1) / FRE_PER_SLICING + 1, 0));

    // ◆ 读取操作频率数据
    for (int tag_id = 1; tag_id <= M; ++tag_id)
    {
        // ● 读取删除频率
        for (int i = 1; i <= (T - 1) / FRE_PER_SLICING + 1; ++i)
            FRE[tag_id][i][0] = DEL_COUNT[tag_id-1][i-1];
        
        // ● 读取写入频率
        for (int i = 1; i <= (T - 1) / FRE_PER_SLICING + 1; ++i)
            FRE[tag_id][i][1] = WRITE_COUNT[tag_id-1][i-1];
        
        // ● 读取读取频率
        for (int i = 1; i <= (T - 1) / FRE_PER_SLICING + 1; ++i)
            FRE[tag_id][i][2] = READ_COUNT[tag_id-1][i-1];
    }

    // ◆ 计算对象数量变化
    for (int tag_id = 1; tag_id <= M; ++tag_id) 
    {
        int cumulative_count = 0;
        for (int i = 1; i <= (T - 1) / FRE_PER_SLICING + 1; ++i) 
        {
            cumulative_count += FRE[tag_id][i][1] - FRE[tag_id][i][0];  // 写入数 - 删除数
            OBJ_COUNT[tag_id][i] = cumulative_count;
        }
    }
    
    // ◆ 预计算标签排序
    int slices = (T - 1) / FRE_PER_SLICING + 2;
    SORTED_READ_TAGS.resize(slices);
    for (int slice_idx = 1; slice_idx < slices; ++slice_idx)
    {
        // ● 初始化标签序列
        std::vector<int> tag_order;
        for (int tag_id = 1; tag_id <= M; ++tag_id)
        {
            tag_order.push_back(tag_id);
        }

        // ● 按读取频率排序
        std::sort(tag_order.begin(), tag_order.end(), 
                 [slice_idx](int a, int b) { 
                     return FRE[a][slice_idx][2] < FRE[b][slice_idx][2]; 
                 });

        SORTED_READ_TAGS[slice_idx] = tag_order;
    }

    // ◆ 计算最终标签顺序
    compute_tag_order();
}

/*╔════════════════════════════ 标签顺序计算 ═══════════════════════════════╗
 * ┌──────────────────────────────────────────────────────────────────────┐
 * │ 功能：根据读取频率曲线相似度计算标签顺序                                 │
 * │ 步骤：                                                                │
 * │ 1. 找出读取次数最多的标签                                              │
 * │ 2. 基于曲线相似度逐个添加标签                                           │
 * │ 3. 更新磁盘的标签顺序                                                  │
 * └──────────────────────────────────────────────────────────────────────┘
 */
void compute_tag_order()
{
    // ◆ 找出最高频标签
    int max_read_tag = 0;
    int max_read_count = 0;
    for (int tag_id = 1; tag_id <= M; ++tag_id) 
    {
        int count = 0;
        for (int i = 1; i <= (T - 1) / FRE_PER_SLICING + 1; ++i) 
        {
            count += FRE[tag_id][i][2];
        }
        if (count > max_read_count) 
        {
            max_read_count = count;
            max_read_tag = tag_id;
        }
    }
    
    // ◆ 构建标签序列
    std::vector<int> tag_order = {START_TAG};
    while (tag_order.size() < static_cast<size_t>(M)) 
    {
        double min_diff_sum = std::numeric_limits<double>::infinity();
        int min_diff_tag = 0;
        
        // ● 获取参考曲线
        std::vector<double> fre_tmp_old;
        for (int i = 1; i <= (T - 1) / FRE_PER_SLICING + 1; ++i) 
        {
            fre_tmp_old.push_back(FRE[tag_order.back()][i][2]);
        }
        
        // ● 寻找最相似标签
        for (int tag_id = 1; tag_id <= M; ++tag_id) 
        {
            if (std::find(tag_order.begin(), tag_order.end(), tag_id) != tag_order.end()) 
            {
                continue;
            }
            
            std::vector<double> fre_tmp_new;
            for (int i = 1; i <= (T - 1) / FRE_PER_SLICING + 1; ++i) 
            {
                fre_tmp_new.push_back(FRE[tag_id][i][2]);
            }
            
            double diff_sum = __compute_similarity(fre_tmp_old, fre_tmp_new, true, true);
            
            if (diff_sum < min_diff_sum) 
            {
                min_diff_sum = diff_sum;
                min_diff_tag = tag_id;
            }
        }
        
        tag_order.push_back(min_diff_tag);
    }

    // ◆ 更新磁盘标签顺序
    TAG_ORDERS.resize(N);
    for (int i = 0; i < N; ++i) 
    {
        TAG_ORDERS[i] = tag_order;
    }

    info("根据读取频率曲线最相似排列的标签顺序：==========================");
    info(tag_order);
}

/*╔════════════════════════════ 曲线归一化实现 ═══════════════════════════════╗
 * ┌──────────────────────────────────────────────────────────────────────┐
 * │ 功能：对输入曲线进行归一化处理                                         │
 * │ 参数：                                                               │
 * │ - curve: 输入曲线数据                                                │
 * │ 返回：归一化后的曲线                                                  │
 * └──────────────────────────────────────────────────────────────────────┘
 */
std::vector<double> __normalize_curve(const std::vector<double> &curve) 
{
    std::vector<double> normalized_curve = curve;
    double max_val = *std::max_element(normalized_curve.begin(), normalized_curve.end());
    if (max_val > 0) 
    {
        for (double& val : normalized_curve) 
        {
            val /= max_val;
        }
    }
    return normalized_curve;
}

/*╔════════════════════════════ 相似度计算实现 ═══════════════════════════════╗
 * ┌──────────────────────────────────────────────────────────────────────┐
 * │ 功能：计算两条曲线的相似度                                             │
 * │ 参数：                                                               │
 * │ - curve1, curve2: 待比较的曲线                                        │
 * │ - normalize_curve1, normalize_curve2: 是否需要归一化                  │
 * │ 返回：相似度得分 (0-2，越小表示越相似)                                 │
 * └──────────────────────────────────────────────────────────────────────┘
 */
double __compute_similarity(const std::vector<double> &curve1, 
                          const std::vector<double> &curve2,
                          bool normalize_curve1, 
                          bool normalize_curve2)
{
    // ◆ 曲线归一化
    std::vector<double> normalized_curve1 = normalize_curve1 ? __normalize_curve(curve1) : curve1;
    std::vector<double> normalized_curve2 = normalize_curve2 ? __normalize_curve(curve2) : curve2;
    
    // ◆ 计算余弦相似度
    double dot_product = 0.0;
    double norm1 = 0.0;
    double norm2 = 0.0;
    
    for (size_t i = 0; i < normalized_curve1.size() && i < normalized_curve2.size(); ++i) 
    {
        dot_product += normalized_curve1[i] * normalized_curve2[i];
        norm1 += normalized_curve1[i] * normalized_curve1[i];
        norm2 += normalized_curve2[i] * normalized_curve2[i];
    }
    
    // ◆ 处理边界情况
    if (norm1 == 0.0 || norm2 == 0.0) 
    {
        return 1.0;  // 曲线全为零时视为不相似
    }
    
    // ◆ 计算最终相似度
    double cosine_similarity = dot_product / (std::sqrt(norm1) * std::sqrt(norm2));
    return 1.0 - cosine_similarity;  // 转换为距离度量
}

/*╔════════════════════════════ 标签相似度查询 ═══════════════════════════════╗
 * ┌──────────────────────────────────────────────────────────────────────┐
 * │ 功能：获取与指定标签相似的标签序列                                      │
 * │ 参数：                                                               │
 * │ - time: 当前时间点                                                   │
 * │ - tag: 参考标签                                                      │
 * │ - mode: 相似度计算模式                                               │
 * │   1: 当前时间                                                        │
 * │   2: 全部时间                                                        │
 * │   3: 当前时间及以后                                                   │
 * └──────────────────────────────────────────────────────────────────────┘
 */
std::vector<int> get_similar_tag_sequence(int time, int tag, int mode) 
{
    std::vector<int> result;
    std::vector<std::pair<int, double>> similarities;
    
    // ◆ 确定时间范围
    int start_slice = 1;
    int end_slice = (T - 1) / FRE_PER_SLICING + 1;
    
    if (mode == 1) 
    {
        int current_slice = (time - 1) / FRE_PER_SLICING + 1;
        start_slice = current_slice;
        end_slice = current_slice;
    } 
    else if (mode == 3) 
    {
        start_slice = (time - 1) / FRE_PER_SLICING + 1;
    }
    
    // ◆ 获取参考曲线
    std::vector<double> ref_curve;
    for (int i = start_slice; i <= end_slice; ++i) 
    {
        ref_curve.push_back(FRE[tag][i][2]);
    }
    
    // 计算每个标签与参考标签的相似度
    for (int other_tag = 1; other_tag <= M; ++other_tag) {
        if (other_tag == tag) continue; // 排除参考标签自身
        
        std::vector<double> other_curve;
        for (int i = start_slice; i <= end_slice; ++i) {
            other_curve.push_back(FRE[other_tag][i][2]);
        }
        
        // 计算相似度，对两条曲线都进行归一化
        double similarity = __compute_similarity(ref_curve, other_curve, true, true);
        similarities.push_back({other_tag, similarity});
    }
    
    // 按相似度排序（升序，因为值越小表示越相似）
    std::sort(similarities.begin(), similarities.end(), 
              [](const auto& a, const auto& b) { return a.second < b.second; });
    
    // 提取排序后的标签
    for (const auto& pair : similarities) {
        result.push_back(pair.first);
    }
    
    return result;
}
