#include "constants.h"
#include "data_analysis.h"
#include "debug.h"
#include "controller.h"
#include "disk_obj_req.h"
#include <cmath>
#include <algorithm>
#include <climits>
#include <numeric>
// 定义频率数据结构
std::vector<std::vector<std::vector<int>>> FRE;
// 移除细粒度常量和数据结构
std::vector<std::vector<int>> SORTED_READ_TAGS;      // [timestamp][tag_index]，预计算的排序标签

// token数量
// std::vector<int> TOKEN_NUM;

// 定义对象数量数据结构
std::vector<std::vector<int>> OBJ_COUNT;             // [tag][slice_idx]粗粒度对象数量

// int get_token(int timestamp)
// {
//     int slice_idx = (timestamp + FRE_PER_SLICING - 1) / FRE_PER_SLICING;
//     assert(slice_idx < TOKEN_NUM.size() and slice_idx > 0);
//     return TOKEN_NUM[slice_idx];
// }

// 频率获取函数 （op_type: 0删除，1写入，2读取）
int get_freq(int tag, int timestamp, int op_type)
{
    int slice_idx = (timestamp + FRE_PER_SLICING - 1) / FRE_PER_SLICING;
    return FRE[tag][slice_idx][op_type];
}

// 获取排序后的当前时间读频率的tag
std::vector<int> &get_sorted_read_tag(int timestamp)
{
    int slice_idx = std::min((timestamp + FRE_PER_SLICING - 1) / FRE_PER_SLICING, (int)SORTED_READ_TAGS.size() - 1);
    return SORTED_READ_TAGS[slice_idx];
}

void process_data_analysis()
{
    // 初始化频率数据
    FRE.resize(MAX_TAG_NUM + 1, std::vector<std::vector<int>>((MAX_SLICING_NUM + 1) / FRE_PER_SLICING + 1, std::vector<int>(3, 0)));
    
    // 初始化对象数量数据结构
    OBJ_COUNT.resize(MAX_TAG_NUM + 1, std::vector<int>((MAX_SLICING_NUM + 1) / FRE_PER_SLICING + 1, 0));


    // 读取删除频率数据
    for (int tag_id = 1; tag_id <= M; ++tag_id)
    {
        for (int i = 1; i <= (T - 1) / FRE_PER_SLICING + 1; ++i)
            // (void)scanf("%d", &FRE[tag_id][i][0]);
            FRE[tag_id][i][0] = DEL_COUNT[tag_id-1][i-1];
    }
    // 读取写入频率数据
    for (int tag_id = 1; tag_id <= M; ++tag_id)
    {
        for (int i = 1; i <= (T - 1) / FRE_PER_SLICING + 1; ++i)
            // (void)scanf("%d", &FRE[tag_id][i][1]);
            FRE[tag_id][i][1] = WRITE_COUNT[tag_id-1][i-1];
    }
    // 读取读取频率数据
    for (int tag_id = 1; tag_id <= M; ++tag_id)
    {
        for (int i = 1; i <= (T - 1) / FRE_PER_SLICING + 1; ++i)
            // (void)scanf("%d", &FRE[tag_id][i][2]);
            FRE[tag_id][i][2] = READ_COUNT[tag_id-1][i-1];
    }

    // 读取token数量
    // TOKEN_NUM.resize((T + 105 + FRE_PER_SLICING - 1) / FRE_PER_SLICING + 1);
    // for (int i = 1; i <= (T + 105 + FRE_PER_SLICING - 1) / FRE_PER_SLICING; ++i)
    //     (void)scanf("%d", &TOKEN_NUM[i]);
    
    // 计算粗粒度对象数量
    for (int tag_id = 1; tag_id <= M; ++tag_id) {
        int cumulative_count = 0;
        for (int i = 1; i <= (T - 1) / FRE_PER_SLICING + 1; ++i) {
            // 累加写入数量并减去删除数量
            cumulative_count += FRE[tag_id][i][1] - FRE[tag_id][i][0];
            OBJ_COUNT[tag_id][i] = cumulative_count;
        }
    }
    
    // 预计算每个时间点的排序标签
    int slices = (T - 1) / FRE_PER_SLICING + 2;
    SORTED_READ_TAGS.resize(slices);
    for (int slice_idx = 1; slice_idx < slices; ++slice_idx)
    {
        std::vector<int> tag_order;
        for (int tag_id = 1; tag_id <= M; ++tag_id)
        {
            tag_order.push_back(tag_id);
        }

        // 按照读取频率（op_type=2）排序
        std::sort(tag_order.begin(), tag_order.end(), [slice_idx](int a, int b)
                  { return FRE[a][slice_idx][2] < FRE[b][slice_idx][2]; });

        SORTED_READ_TAGS[slice_idx] = tag_order;
    }

    // 选择使用哪种方法计算标签顺序
    compute_tag_order(); // 原有方法：根据读取频率曲线相似度排列
    // compute_alternating_tag_order(); // 新方法：交替使用读取频率相似度和对象数量差异
}

//计算标签顺序
void compute_tag_order()
{
    // 读取次数最大的标签
    int max_read_tag = 0;
    int max_read_count = 0;
    for (int tag_id = 1; tag_id <= M; ++tag_id) {
        int count = 0;
        for (int i = 1; i <= (T - 1) / FRE_PER_SLICING + 1; ++i) {
            count += FRE[tag_id][i][2];
        }
        if (count > max_read_count) {
            max_read_count = count;
            max_read_tag = tag_id;
        }
    }
    
    std::vector<int> tag_order = {START_TAG};
    
    // 按读频率曲线最相似排列
    while (tag_order.size() < static_cast<size_t>(M)) {
        double min_diff_sum = std::numeric_limits<double>::infinity();
        int min_diff_tag = 0;
        
        // 获取最后一个标签的频率曲线
        std::vector<double> fre_tmp_old;
        for (int i = 1; i <= (T - 1) / FRE_PER_SLICING + 1; ++i) {
            fre_tmp_old.push_back(FRE[tag_order.back()][i][2]);
        }
        
        // 找出与当前标签曲线最相似的未加入标签
        for (int tag_id = 1; tag_id <= M; ++tag_id) {
            if (std::find(tag_order.begin(), tag_order.end(), tag_id) != tag_order.end()) {
                continue;
            }
            
            // 获取待比较标签的频率曲线
            std::vector<double> fre_tmp_new;
            for (int i = 1; i <= (T - 1) / FRE_PER_SLICING + 1; ++i) {
                fre_tmp_new.push_back(FRE[tag_id][i][2]);
            }
            
            // 使用新参数计算相似度，两条曲线都需要归一化
            double diff_sum = __compute_similarity(fre_tmp_old, fre_tmp_new, true, true);
            
            if (diff_sum < min_diff_sum) {
                min_diff_sum = diff_sum;
                min_diff_tag = tag_id;
            }
        }
        
        tag_order.push_back(min_diff_tag);
    }

    TAG_ORDERS.resize(N);
    for (int i = 0; i < N; ++i) {
        TAG_ORDERS[i] = tag_order;
    }

    info("根据读取频率曲线最相似排列的标签顺序：==========================");
    info(tag_order);
}

// 归一化曲线函数
std::vector<double> __normalize_curve(const std::vector<double> &curve) 
{
    std::vector<double> normalized_curve = curve;
    double max_val = *std::max_element(normalized_curve.begin(), normalized_curve.end());
    if (max_val > 0) {
        for (double& val : normalized_curve) {
            val /= max_val;
        }
    }
    return normalized_curve;
}

// 计算两条曲线相似度辅助函数
double __compute_similarity(const std::vector<double> &curve1, const std::vector<double> &curve2, bool normalize_curve1, bool normalize_curve2)
{
    // 根据参数决定是否对两条曲线进行归一化
    std::vector<double> normalized_curve1 = normalize_curve1 ? __normalize_curve(curve1) : curve1;
    std::vector<double> normalized_curve2 = normalize_curve2 ? __normalize_curve(curve2) : curve2;
    
    // 余弦相似度
    double dot_product = 0.0;
    double norm1 = 0.0;
    double norm2 = 0.0;
    
    for (size_t i = 0; i < normalized_curve1.size() && i < normalized_curve2.size(); ++i) {
        dot_product += normalized_curve1[i] * normalized_curve2[i];
        norm1 += normalized_curve1[i] * normalized_curve1[i];
        norm2 += normalized_curve2[i] * normalized_curve2[i];
    }
    
    // 避免除以零
    if (norm1 == 0.0 || norm2 == 0.0) {
        return 1.0; // 如果有一条曲线全为零，则认为它们不相似
    }
    
    // 余弦相似度，值域为[-1,1]，值越大表示越相似
    double cosine_similarity = dot_product / (std::sqrt(norm1) * std::sqrt(norm2));
    
    // 转换为距离度量，值域为[0,2]，值越小表示越相似
    return 1.0 - cosine_similarity;
}

// 获取与指定标签读取频率相似的标签序列
// time: 当前时间点
// tag: 参考标签
// mode: 相似度计算模式 1:当前时间 2:全部时间 3:当前时间及以后
// 返回按相似度排序的标签序列（不包含参考标签本身）
std::vector<int> get_similar_tag_sequence(int time, int tag, int mode) 
{
    std::vector<int> result;
    std::vector<std::pair<int, double>> similarities;
    
    // 计算时间范围
    int start_slice = 1;
    int end_slice = (T - 1) / FRE_PER_SLICING + 1;
    
    if (mode == 1) {
        // 当前时间的相似度
        int current_slice = (time - 1) / FRE_PER_SLICING + 1;
        start_slice = current_slice;
        end_slice = current_slice;
    } else if (mode == 3) {
        // 当前时间及以后的相似度
        start_slice = (time - 1) / FRE_PER_SLICING + 1;
    }
    
    // 获取参考标签的读取频率曲线
    std::vector<double> ref_curve;
    for (int i = start_slice; i <= end_slice; ++i) {
        ref_curve.push_back(FRE[tag][i][2]); // 2表示读取操作
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
