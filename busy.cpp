/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 *  ██████╗ ██╗   ██╗███████╗██╗   ██╗
 *  ██╔══██╗██║   ██║██╔════╝╚██╗ ██╔╝
 *  ██████╔╝██║   ██║███████╗ ╚████╔╝ 
 *  ██╔══██╗██║   ██║╚════██║  ╚██╔╝  
 *  ██████╔╝╚██████╔╝███████║   ██║   
 *  ╚═════╝  ╚═════╝ ╚══════╝   ╚═╝   
 * 
 * ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 * 【模块功能】
 * ┌─────────────────┬───────────────────────────────────────────────────────────┐
 * │ 过载控制         │ 提供系统过载和繁忙处理的具体实现                             │
 * ├─────────────────┼───────────────────────────────────────────────────────────┤
 * │ 请求过滤         │ 实现前置过滤和后置过滤机制，优化系统性能                     │
 * ├─────────────────┼───────────────────────────────────────────────────────────┤
 * │ 性能优化         │ 减轻系统压力，提高读取效率                                  │
 * └─────────────────┴───────────────────────────────────────────────────────────┘
 * ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/

#include "constants.h"          // ⟪常量定义⟫
#include "ctrl_disk_obj_req.h"  // ⟪控制器、磁盘、对象、请求相关⟫
#include "data_analysis.h"      // ⟪数据分析相关⟫

/*╔══════════════════════════════ 前置过滤实现 ══════════════════════════════╗
 * ┌──────────────────────────────────────────────────────────────────────┐
 * │ 功能：在请求加入前过滤低频率孤立请求                                    │
 * │ 策略：                                                                │
 * │ 1. 计算标签读取频率顺序                                                │
 * │ 2. 检测请求的邻居状态                                                  │
 * │ 3. 基于等待时间动态调整过滤阈值                                         │
 * └──────────────────────────────────────────────────────────────────────┘
 */
void Controller::pre_filter_req(std::vector<std::pair<int, int>> &reqs)
{
    // ◆ 初始化数据结构
    std::vector<int> tag_order(M+1);                    // ● 标签读取频率次序
    std::unordered_map<int, bool> req_is_alone;         // ● 请求邻居状态表

    // ◆ 计算标签读取频率次序
    auto &order_tag = get_sorted_read_tag(this->timestamp);
    for(int i = 0; i < M; ++i)
    {
        tag_order[order_tag[i]] = i;
    }

    // ◆ 分析请求邻居状态
    for(auto [req_id, obj_id] : reqs)
    {
        int size = OBJECTS[obj_id].size;
        int tag = OBJECTS[obj_id].tag;
        Disk& disk = DISKS[OBJECTS[obj_id].replicas[0].first];
        std::vector<int>& cell_idxs = OBJECTS[obj_id].replicas[0].second;

        // ● 检查每个单元格的邻居
        req_is_alone[req_id] = true;
        for (auto &cell_idx : cell_idxs)
        {
            int prev_cell_idx = cell_idx;
            int next_cell_idx = cell_idx;
            // ● 检查前后7个单元格
            for (int k = 0; k < 7; ++k)
            {
                if(disk.cells[prev_cell_idx].req_ids.size() > 0 or 
                   disk.cells[next_cell_idx].req_ids.size() > 0)
                {
                    req_is_alone[req_id] = false; 
                    break;
                }
                prev_cell_idx = prev_cell_idx == 1 ? 1 : prev_cell_idx - 1;
                next_cell_idx = next_cell_idx == size ? size : next_cell_idx + 1;
            }
            if(not req_is_alone[req_id]) break;
        }
    }

    // ◆ 动态过滤请求
    std::vector<std::pair<int, int>> new_reqs;
    for(int i = 0; i < reqs.size(); ++i)
    {
        // ● 获取磁盘平均等待时间
        Disk& disk = DISKS[OBJECTS[reqs[i].second].replicas[0].first];
        float avg_wait_time = disk.get_avg_wait_time();

        // ● 动态调整过滤阈值
        int m = 12;
        if(avg_wait_time > 43)      m -= 1;
        else if(avg_wait_time > 29) m -= 2;
        else if(avg_wait_time > 18) m -= 3;
        else if(avg_wait_time > 16) m -= 4;
        else if(avg_wait_time > 15) m -= 5;
        else if(avg_wait_time > 14) m -= 6;
        else if(avg_wait_time > 13) m -= 7;
        else if(avg_wait_time > 11) m -= 7;
        else if(avg_wait_time > 10) m -= 8;
        else m = 0;

        // ● 应用过滤条件
        if(req_is_alone[reqs[i].first] and 
           tag_order[OBJECTS[reqs[i].second].tag] < m)
        {
            over_load_reqs.push_back(reqs[i].first);
        }
        else
        {
            new_reqs.push_back(reqs[i]);
        }
    }
    reqs = new_reqs;
}
/*╚═════════════════════════════════════════════════════════════════════════╝*/

/*╔══════════════════════════════ 后置过滤实现 ══════════════════════════════╗
 * ┌──────────────────────────────────────────────────────────────────────┐
 * │ 功能：清理已超时或即将超时的请求                                        │
 * │ 策略：                                                                │
 * │ 1. 检查请求的生存时间                                                  │
 * │ 2. 移除超过104个时间片的请求                                           │
 * │ 3. 更新系统状态和计数器                                                │
 * └──────────────────────────────────────────────────────────────────────┘
 */
void Controller::post_filter_req()
{
    // ◆ 处理即将超时的请求
    while(req_105_idx < req_new_idx)
    {
        // ● 获取请求创建时间
        int birth_time = REQS[req_105_idx % LEN_REQ].timestamp;
        
        // ● 跳过已处理的请求
        if(birth_time == 0) 
        {
            req_105_idx++; 
            continue;
        }
        // ● 移除超时请求
        else if (birth_time + 104 < timestamp)
        {
            busy_reqs.push_back(req_105_idx);
            remove_req(req_105_idx);
            req_105_idx++;
        }
        else break;
    }
}
/*╚═════════════════════════════════════════════════════════════════════════╝*/
