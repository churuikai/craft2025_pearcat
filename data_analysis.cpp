#include "constants.h"
#include "data_analysis.h"
#include "debug.h"
#include "controller.h"
#include "disk_obj_req.h"
#include <cmath>
#include <algorithm>
#include <climits>
#include <numeric>
// 定义细粒度的频率数据结构
std::vector<std::vector<std::vector<int>>> FRE;
const int FINE_GRANULARITY = 1800;                    // 细粒度为100时间片，实验表明细粒度效果更差。。。
std::vector<std::vector<std::vector<int>>> FRE_FINE; // [tag][fine_slice_idx][op_type]
std::vector<std::vector<int>> SORTED_READ_TAGS;      // [timestamp][tag_index]，预计算的排序标签


// 细粒度的频率获取函数 （op_type: 0删除，1写入，2读取）
int get_freq_fine(int tag, int timestamp, int op_type)
{

    int fine_slice_idx = (timestamp - 1) / FINE_GRANULARITY + 1;
    return FRE_FINE[tag][fine_slice_idx][op_type];
}

// 获取排序后的当前TIME读频率的tag
std::vector<int> &get_sorted_read_tag(int timestamp)
{
    int fine_slice_idx = (timestamp - 1) / FINE_GRANULARITY + 1;
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
            (void)scanf("%d", &FRE[tag_id][i][0]);
    }
    // 读取写入频率数据
    for (int tag_id = 1; tag_id <= M; ++tag_id)
    {
        for (int i = 1; i <= (T - 1) / FRE_PER_SLICING + 1; ++i)
            (void)scanf("%d", &FRE[tag_id][i][1]);
    }
    // 读取读取频率数据
    for (int tag_id = 1; tag_id <= M; ++tag_id)
    {
        for (int i = 1; i <= (T - 1) / FRE_PER_SLICING + 1; ++i)
            (void)scanf("%d", &FRE[tag_id][i][2]);
    }
    // 插值得到细粒度频率数据
    for (int tag_id = 1; tag_id <= M; ++tag_id)
    {
        for (int op_type = 0; op_type < 3; ++op_type)
        {
            int coarse_slices = (T - 1) / FRE_PER_SLICING + 1;

            // 对每个细粒度时间点进行插值
            for (int fine_idx = 1; fine_idx < fine_slices; ++fine_idx)
            {
                // 当前细粒度时间点对应的实际时间戳
                int timestamp = (fine_idx - 1) * FINE_GRANULARITY + 1;
                
                // 如果细粒度的时间粒度正好是粗粒度的整数倍，可以直接取粗粒度的值
                if (FINE_GRANULARITY % FRE_PER_SLICING == 0 && 
                    (timestamp - 1) % FRE_PER_SLICING == 0) {
                    int coarse_idx = (timestamp - 1) / FRE_PER_SLICING + 1;
                    if (coarse_idx <= (T - 1) / FRE_PER_SLICING + 1) {
                        FRE_FINE[tag_id][fine_idx][op_type] = FRE[tag_id][coarse_idx][op_type];
                        continue;
                    }
                }
                // 计算在粗粒度上的位置（浮点数）
                double coarse_pos = (timestamp - 1) / (double)FRE_PER_SLICING + 1;
                
                // 找到对应的粗粒度区间的左右两个点
                int left_idx = std::max(1, (int)floor(coarse_pos));
                int right_idx = std::min((T - 1) / FRE_PER_SLICING + 1, left_idx + 1);
                
                // 检查边界情况
                if (left_idx == right_idx) {
                    // 在边界处，直接使用对应的粗粒度值
                    FRE_FINE[tag_id][fine_idx][op_type] = FRE[tag_id][left_idx][op_type];
                } else {
                    // 计算在两点之间的位置比例
                    double ratio = coarse_pos - left_idx;
                    
                    // 使用线性插值
                    double left_val = FRE[tag_id][left_idx][op_type];
                    double right_val = FRE[tag_id][right_idx][op_type];
                    int interpolated_value = static_cast<int>(round(left_val * (1 - ratio) + right_val * ratio));
                    
                    // 确保值非负
                    FRE_FINE[tag_id][fine_idx][op_type] = std::max(0, interpolated_value);
                }
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
        for (int i = 1; i <= (T - 1) / FRE_PER_SLICING + 1; ++i) {
            count += FRE[tag_id][i][2];
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
    
    // 使用更好的相似度计算方法 - 余弦相似度
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

// 获取磁盘的统计信息
DiskStatsInfo Controller::get_disk_stats(int disk_id) {
    DiskStatsInfo stats;
    stats.total_blocks = 0;
    stats.max_block_size = 0;
    stats.min_block_size = INT_MAX;
    stats.avg_block_size = 0;
    stats.fragmentation_count = 0;
    
    Disk &disk = DISKS[disk_id];
    
    std::vector<int> block_sizes; // 用于存储所有块的大小
    
    // 遍历所有正常分区
    for (int tag = 1; tag <= M; tag++) {
        for (int size = 1; size <= 5; size++) {
            for (const auto &part : disk.get_parts(tag, size)) {
                // 创建新的分区统计信息
                PartStatsInfo part_stats;
                part_stats.tag = tag;
                part_stats.size = size;
                
                std::vector<int> part_block_sizes; // 该分区的块大小
                
                FreeBlock *current = part.free_list_head;
                while (current != nullptr) {
                    int block_size = current->end - current->start + 1;
                    part_stats.total_blocks++;
                    part_block_sizes.push_back(block_size);
                    
                    // 更新分区统计的最大最小值
                    part_stats.max_block_size = std::max(part_stats.max_block_size, block_size);
                    part_stats.min_block_size = std::min(part_stats.min_block_size, block_size);
                    
                    // 更新分区碎片大小分布
                    part_stats.fragment_size_distribution[block_size]++;
                    
                    // 更新整个磁盘的统计信息
                    stats.total_blocks++;
                    block_sizes.push_back(block_size);
                    stats.max_block_size = std::max(stats.max_block_size, block_size);
                    stats.min_block_size = std::min(stats.min_block_size, block_size);
                    stats.fragment_size_distribution[block_size]++;
                    
                    if (current->next != nullptr) {
                        // 如果当前块与下一个块不连续，增加碎片计数
                        if (current->end + 1 < current->next->start) {
                            part_stats.fragmentation_count++;
                            stats.fragmentation_count++;
                        }
                    }
                    
                    current = current->next;
                }
                
                // 计算分区的平均值，忽略最大值
                if (part_stats.total_blocks > 1) {
                    // 移除最大值
                    int max_size = part_stats.max_block_size;
                    auto it = std::find(part_block_sizes.begin(), part_block_sizes.end(), max_size);
                    if (it != part_block_sizes.end()) {
                        part_block_sizes.erase(it);
                        
                        // 计算剩余块的平均大小
                        double sum = std::accumulate(part_block_sizes.begin(), part_block_sizes.end(), 0.0);
                        part_stats.avg_block_size = sum / part_block_sizes.size();
                    }
                } else if (part_stats.total_blocks == 1) {
                    // 只有一个块时，平均值就是该块的大小
                    part_stats.avg_block_size = part_block_sizes[0];
                }
                
                // 只有当分区有空闲块时才添加到统计中
                if (part_stats.total_blocks > 0) {
                    stats.part_stats[tag].push_back(part_stats);
                }
            }
        }
    }
    
    // 统计冗余区
    for (const auto &part : disk.get_parts(17, 1)) {
        // 创建新的分区统计信息
        PartStatsInfo part_stats;
        part_stats.tag = 17; // 冗余区标签
        part_stats.size = 1;
        
        std::vector<int> part_block_sizes; // 该分区的块大小
        
        FreeBlock *current = part.free_list_head;
        while (current != nullptr) {
            int block_size = current->end - current->start + 1;
            part_stats.total_blocks++;
            part_block_sizes.push_back(block_size);
            
            // 更新分区统计的最大最小值
            part_stats.max_block_size = std::max(part_stats.max_block_size, block_size);
            part_stats.min_block_size = std::min(part_stats.min_block_size, block_size);
            
            // 更新分区碎片大小分布
            part_stats.fragment_size_distribution[block_size]++;
            
            // 更新整个磁盘的统计信息
            stats.total_blocks++;
            block_sizes.push_back(block_size);
            stats.max_block_size = std::max(stats.max_block_size, block_size);
            stats.min_block_size = std::min(stats.min_block_size, block_size);
            stats.fragment_size_distribution[block_size]++;
            
            if (current->next != nullptr) {
                // 如果当前块与下一个块不连续，增加碎片计数
                if (current->end + 1 < current->next->start) {
                    part_stats.fragmentation_count++;
                    stats.fragmentation_count++;
                }
            }
            
            current = current->next;
        }
        
        // 计算分区的平均值，忽略最大值
        if (part_stats.total_blocks > 1) {
            // 移除最大值
            int max_size = part_stats.max_block_size;
            auto it = std::find(part_block_sizes.begin(), part_block_sizes.end(), max_size);
            if (it != part_block_sizes.end()) {
                part_block_sizes.erase(it);
                
                // 计算剩余块的平均大小
                double sum = std::accumulate(part_block_sizes.begin(), part_block_sizes.end(), 0.0);
                part_stats.avg_block_size = sum / part_block_sizes.size();
            }
        } else if (part_stats.total_blocks == 1) {
            // 只有一个块时，平均值就是该块的大小
            part_stats.avg_block_size = part_block_sizes[0];
        }
        
        // 只有当分区有空闲块时才添加到统计中
        if (part_stats.total_blocks > 0) {
            stats.part_stats[17].push_back(part_stats);
        }
    }
    
    // 计算整个磁盘的平均值，忽略最大值
    if (stats.total_blocks > 1) {
        // 移除最大值
        int max_size = stats.max_block_size;
        auto it = std::find(block_sizes.begin(), block_sizes.end(), max_size);
        if (it != block_sizes.end()) {
            block_sizes.erase(it);
            
            // 计算剩余块的平均大小
            double sum = std::accumulate(block_sizes.begin(), block_sizes.end(), 0.0);
            stats.avg_block_size = sum / block_sizes.size();
        }
    } else if (stats.total_blocks == 1) {
        // 只有一个块时，平均值就是该块的大小
        stats.avg_block_size = block_sizes[0];
    }
    
    return stats;
}

void process_verify(Controller &controller) {
    // 验证所有磁盘
    int total_inconsistencies = 0;
    for (int disk_id = 1; disk_id <= N; disk_id++) {
        Disk &disk = controller.DISKS[disk_id];
   
        int disk_inconsistencies = 0;
        
        // 遍历所有分区（除备份区外）
        for (int tag = 1; tag <= M; tag++) {
            for (int size = 1; size <= 5; size++) {
                for (auto &part : disk.get_parts(tag, size)) {
                    // 使用新的验证函数
                    disk_inconsistencies += part.verify_free_list_consistency(&disk);
                    disk_inconsistencies += part.verify_free_list_integrity();
                }
            }
        }
        
        // 检查冗余区
        for (auto &part : disk.get_parts(17, 1)) {
            disk_inconsistencies += part.verify_free_list_consistency(&disk);
            disk_inconsistencies += part.verify_free_list_integrity();
        }
        
        if (disk_inconsistencies > 0) {
            info("磁盘", disk_id, "存在", disk_inconsistencies, "处链表不一致");
            assert(false);
        }
        total_inconsistencies += disk_inconsistencies;
    }

    // 输出详细统计信息
    info("=============================================================");
    info("TIME: ", controller.timestamp, " 磁盘碎片信息...(只输出前1个)");
    info("=============================================================");
    
    for (int disk_id = 1; disk_id <= 1; disk_id++) {
        // 使用新的磁盘统计函数
        DiskStatsInfo stats = controller.get_disk_stats(disk_id);
        
        info("======================== 磁盘", disk_id, "统计信息 ========================");
        
        // 按标签输出每个分区的信息
        for (int tag = 1; tag <= 17; tag++) {
            if(tag > M && tag != 17) continue;
            // 如果该标签下有分区统计信息
            if (stats.part_stats.find(tag) != stats.part_stats.end() && !stats.part_stats[tag].empty()) {
      
                for (const auto& part_stats : stats.part_stats[tag]) {
                    // 输出基本信息
                    std::stringstream ss;
                    ss << "tag:" << tag << " size:" << part_stats.size 
                       << " 平均(去最大):" << std::fixed << std::setprecision(2) << part_stats.avg_block_size
                       << " 最大:" << part_stats.max_block_size
                       << " 最小:" << part_stats.min_block_size
                       << " 碎片数:" << part_stats.fragmentation_count;
          
                    // 统计碎片大小1-10的具体数量
                    std::stringstream ss_small;
                    ss_small << "小碎片分布(1-5): ";
                    for (int size = 1; size <= 5; size++) {
                        int count = part_stats.fragment_size_distribution.count(size) > 0 ? 
                                    part_stats.fragment_size_distribution.at(size) : 0;
                        ss_small << size << "：" << count << " ";
                    }
                    info(tag, ss.str(), ss_small.str());
                }
            }
        }
    }
}

