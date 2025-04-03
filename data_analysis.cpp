#include "constants.h"
#include "data_analysis.h"
#include "debug.h"
#include <cmath>
#include <algorithm>
// 定义细粒度的频率数据结构
std::vector<std::vector<std::vector<int>>> FRE;
const int FINE_GRANULARITY = 100;                    // 细粒度为100时间片
std::vector<std::vector<std::vector<int>>> FRE_FINE; // [tag][fine_slice_idx][op_type]
std::vector<std::vector<int>> SORTED_READ_TAGS;      // [timestamp][tag_index]，预计算的排序标签


// 细粒度的频率获取函数 （op_type: 0删除，1写入，2读取）
int get_freq_fine(int tag, int timestamp, int op_type)
{
    int fine_slice_idx = (timestamp - 1) / FINE_GRANULARITY + 1;
    return FRE_FINE[tag][fine_slice_idx][op_type];
}

// 获取排序后的当前TIME读频率的tag
std::vector<int> &get_sorted_read_tag()
{
    int fine_slice_idx = (TIME - 1) / FINE_GRANULARITY + 1;
    return SORTED_READ_TAGS[fine_slice_idx];
}


void process_data_analysis()
{

    // 初始化频率数据
    FRE.resize(MAX_TAG_NUM + 1, std::vector<std::vector<int>>((MAX_SLICING_NUM + 1) / FRE_PER_SLICING + 1, std::vector<int>(3, 0)));

    // 初始化细粒度频率数据
    int fine_slices = (T - 1) / FINE_GRANULARITY + 2;
    FRE_FINE.resize(MAX_TAG_NUM + 1, std::vector<std::vector<int>>(fine_slices, std::vector<int>(3, 0)));

    // 读取删除频率数据
    for (int tag_id = 1; tag_id <= M; ++tag_id)
    {
        for (int i = 1; i <= (T - 1) / FRE_PER_SLICING + 1; ++i)
            scanf("%d", &FRE[tag_id][i][0]);
    }
    // 读取写入频率数据
    for (int tag_id = 1; tag_id <= M; ++tag_id)
    {
        for (int i = 1; i <= (T - 1) / FRE_PER_SLICING + 1; ++i)
            scanf("%d", &FRE[tag_id][i][1]);
    }
    // 读取读取频率数据
    for (int tag_id = 1; tag_id <= M; ++tag_id)
    {
        for (int i = 1; i <= (T - 1) / FRE_PER_SLICING + 1; ++i)
            scanf("%d", &FRE[tag_id][i][2]);
    }
    // 使用三次样条插值得到细粒度频率数据
    for (int tag_id = 1; tag_id <= M; ++tag_id)
    {
        for (int op_type = 0; op_type < 3; ++op_type)
        {
            int coarse_slices = (T - 1) / FRE_PER_SLICING + 1;

            // 提取原始数据点
            std::vector<int> original_data(coarse_slices + 1);
            for (int i = 1; i <= coarse_slices; ++i)
            {
                original_data[i] = FRE[tag_id][i][op_type];
            }

            // 计算样条插值系数
            std::vector<double> a(coarse_slices), b(coarse_slices), c(coarse_slices + 1), d(coarse_slices);
            __compute_spline_coefficients(original_data, a, b, c, d, coarse_slices);

            // 对每个细粒度时间点进行插值
            for (int fine_idx = 1; fine_idx < fine_slices; ++fine_idx)
            {
                // 当前细粒度时间点对应的实际时间戳
                int timestamp = (fine_idx - 1) * FINE_GRANULARITY + 1;

                // 计算在粗粒度上的位置（浮点数）
                double coarse_pos = (timestamp - 1) / (double)FRE_PER_SLICING + 1;

                // 找到对应的粗粒度区间
                int i = std::min(coarse_slices - 1, std::max(1, (int)coarse_pos));

                // 使用样条插值
                FRE_FINE[tag_id][fine_idx][op_type] = __spline_interpolate(a, b, c, d, coarse_pos, i);
            }
        }
    }
    // 预计算每个时间点的排序标签
    SORTED_READ_TAGS.resize(fine_slices);
    for (int fine_idx = 1; fine_idx < fine_slices; ++fine_idx)
    {
        std::vector<int> tag_order;
        for (int tag_id = 1; tag_id <= M; ++tag_id)
        {
            tag_order.push_back(tag_id);
        }

        // 按照读取频率（op_type=2）排序
        std::sort(tag_order.begin(), tag_order.end(), [fine_idx](int a, int b)
                  { return FRE_FINE[a][fine_idx][2] < FRE_FINE[b][fine_idx][2]; });

        SORTED_READ_TAGS[fine_idx] = tag_order;
    }

    // 计算标签顺序
    compute_tag_order();

}

//计算标签顺序
void compute_tag_order()
{
    // 读取次数最大的标签
    int max_read_tag = 0;
    int max_read_count = 0;
    for (int tag_id = 1; tag_id <= M; ++tag_id) {
        int count = 0;
        for (int i = 0; i < M + 1; ++i) {
            count += FRE[tag_id][i][0];
        }
        if (count > max_read_count) {
            max_read_count = count;
            max_read_tag = tag_id;
        }
    }
    // debug(max_read_tag);
    std::vector<int> tag_order = {max_read_tag};
    
    // 按读频率曲线最相似排列
    while (tag_order.size() < static_cast<size_t>(M)) {
        double min_diff_sum = std::numeric_limits<double>::infinity();
        int min_diff_tag = 0;
        
        // 获取最后一个标签的频率曲线
        std::vector<double> fre_tmp_old;
        for (int i = 0; i < T / FRE_PER_SLICING + 1; ++i) {
            fre_tmp_old.push_back(FRE[tag_order.back()][i][0]);
        }
        
        // 找出与当前标签曲线最相似的未加入标签
        for (int tag_id = 1; tag_id <= M; ++tag_id) {
            if (std::find(tag_order.begin(), tag_order.end(), tag_id) != tag_order.end()) {
                continue;
            }
            
            // 获取待比较标签的频率曲线
            std::vector<double> fre_tmp_new;
            for (int i = 0; i < T / FRE_PER_SLICING + 1; ++i) {
                fre_tmp_new.push_back(FRE[tag_id][i][0]);
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
    
    // 计算差异总和
    double diff_sum = 0;
    for (size_t i = 0; i < normalized_curve1.size() && i < normalized_curve2.size(); ++i) {
        diff_sum += std::abs(normalized_curve1[i] - normalized_curve2[i]);
    }
    
    return diff_sum;
}

// 三次样条插值辅助函数
void __compute_spline_coefficients(const std::vector<int> &y, std::vector<double> &a, std::vector<double> &b,
                                 std::vector<double> &c, std::vector<double> &d, int n)
{
    std::vector<double> h(n), alpha(n), l(n + 1), mu(n), z(n + 1);

    for (int i = 0; i < n; i++)
    {
        h[i] = 1.0; // 等距采样点，间距为1
        a[i] = y[i];
    }

    // 计算中间变量
    for (int i = 1; i < n; i++)
    {
        alpha[i] = 3.0 * (a[i + 1] - a[i]) / h[i] - 3.0 * (a[i] - a[i - 1]) / h[i - 1];
    }

    // 解三对角矩阵
    l[0] = 1.0;
    mu[0] = 0.0;
    z[0] = 0.0;

    for (int i = 1; i < n; i++)
    {
        l[i] = 2.0 * (h[i - 1] + h[i]) - h[i - 1] * mu[i - 1];
        mu[i] = h[i] / l[i];
        z[i] = (alpha[i] - h[i - 1] * z[i - 1]) / l[i];
    }

    l[n] = 1.0;
    z[n] = 0.0;
    c[n] = 0.0;

    // 回代求解系数
    for (int j = n - 1; j >= 0; j--)
    {
        c[j] = z[j] - mu[j] * c[j + 1];
        b[j] = (a[j + 1] - a[j]) / h[j] - h[j] * (c[j + 1] + 2.0 * c[j]) / 3.0;
        d[j] = (c[j + 1] - c[j]) / (3.0 * h[j]);
    }
}

// 使用三次样条插值计算值
int __spline_interpolate(const std::vector<double> &a, const std::vector<double> &b,
                       const std::vector<double> &c, const std::vector<double> &d,
                       double x, int i)
{
    double dx = x - i;
    double result = a[i] + b[i] * dx + c[i] * dx * dx + d[i] * dx * dx * dx;
    return std::max(0, static_cast<int>(std::round(result)));
}

