/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 *  ██████╗ ███████╗ █████╗ ██████╗ 
 *  ██╔══██╗██╔════╝██╔══██╗██╔══██╗
 *  ██████╔╝█████╗  ███████║██║  ██║
 *  ██╔══██╗██╔══╝  ██╔══██║██║  ██║
 *  ██║  ██║███████╗██║  ██║██████╔╝
 *  ╚═╝  ╚═╝╚══════╝╚═╝  ╚═╝╚═════╝ 
 * 
 * ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 * 【模块功能】
 * ┌─────────────────┬───────────────────────────────────────────────────────────┐
 * │ 读取调度         │ 实现磁头调度和路径优化                                      │
 * ├─────────────────┼───────────────────────────────────────────────────────────┤
 * │ 请求处理         │ 管理读取请求的执行和完成                                    │
 * ├─────────────────┼───────────────────────────────────────────────────────────┤
 * │ 性能统计         │ 收集和分析读取性能数据                                      │
 * └─────────────────┴───────────────────────────────────────────────────────────┘
 * ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/

/*============================================================================
 * 读取事件处理
 *============================================================================
 * 【代码功能】
 * - 实现对象读取的具体策略
 * - 磁头调度与路径选择优化
 * - 请求完成处理与等待时间统计
 *============================================================================*/

#include "constants.h"          // 常量定义
#include "ctrl_disk_obj_req.h"  // 控制器、磁盘、对象、请求相关
#include "debug.h"              // 调试工具
#include "token_table.h"        // rpj 序列表
#include "data_analysis.h"      // 数据分析相关
#include <cmath>                // 数学函数

/*╔══════════════════════════════ 读取调度模块 ═══════════════════════════════╗*/
/**
 * @brief     处理所有磁盘的读取请求
 * @return    std::pair<std::vector<std::string>, std::vector<int>> 
 *           - first: 磁头操作指令列表
 *           - second: 完成的请求ID列表
 * @details   执行以下步骤:
 * ┌──────────────────────────────────────────────────────────────────────┐
 * │ 1. 遍历所有磁盘                                                       │
 * │ 2. 对每个磁盘的两个磁头执行读取操作                                    │
 * │ 3. 收集所有磁头的操作指令和完成的请求                                  │
 * └──────────────────────────────────────────────────────────────────────┘
 */
std::pair<std::vector<std::string>, std::vector<int>> Controller::read()
{
    std::vector<std::string> ops;
    std::vector<int> completed_reqs;

    // ◆ 遍历所有磁盘执行读取操作
    for (int disk_id = 1; disk_id <= N; ++disk_id)
    {
        // ● 磁头1读取操作
        auto [op1, completed_req1] = DISKS[disk_id].read(1);
        ops.push_back(op1);
        completed_reqs.insert(completed_reqs.end(), completed_req1.begin(), completed_req1.end());

        // ● 磁头2读取操作
        auto [op2, completed_req2] = DISKS[disk_id].read(2);
        ops.push_back(op2);
        completed_reqs.insert(completed_reqs.end(), completed_req2.begin(), completed_req2.end());
    }

    return {ops, completed_reqs};
}

/**
 * @brief     执行单个磁头的读取操作
 * @param     op_id 磁头ID（1或2）
 * @return    std::pair<std::string, std::vector<int>>
 *           - first: 操作指令
 *           - second: 完成的请求ID列表
 * @details   根据当前状态选择最优读取策略:
 * ┌──────────────────────────────────────────────────────────────────────┐
 * │ 1. 获取最佳读取起点                                                   │
 * │ 2. 如果没有可读取的单元，返回空操作                                    │
 * │ 3. 如果读取代价超过令牌数，执行跳转                                    │
 * │ 4. 如果令牌充足，执行最优路径读取                                      │
 * └──────────────────────────────────────────────────────────────────────┘
 */
std::pair<std::string, std::vector<int>> Disk::read(int op_id)
{
    // ◆ 获取磁头相关参数
    int &point = op_id == 1 ? point1 : point2;
    int &tokens = op_id == 1 ? tokens1 : tokens2;
    int &prev_read_token = op_id == 1 ? prev_read_token1 : prev_read_token2;

    // ◆ 寻找最佳读取起点
    int start = _get_best_start(op_id);

    // ◆ 根据起点状态选择操作
    if (start == -1)
    {
        // ● 无可读取单元，返回空操作
        return {"#", std::vector<int>()};
    }
    else if ((start - point + size) % size > tokens)
    {
        // ● 令牌不足，执行跳转
        point = start;
        prev_read_token = 80;
        return {"j " + std::to_string(start), std::vector<int>()};
    }
    else
    {
        // ● 执行最优路径读取
        auto [path, completed_reqs, _] = _read_by_best_path(start, op_id);
        return {path + "#", completed_reqs};
    }
}
/*╚═════════════════════════════════════════════════════════════════════════╝*/

/*╔══════════════════════════════ 路径优化模块 ═══════════════════════════════╗*/
/**
 * @brief     获取最佳读取起点
 * @param     op_id 磁头ID（1或2）
 * @return    int 最佳起点位置，如果没有可读取的单元则返回-1
 * @details   执行以下步骤:
 * ┌──────────────────────────────────────────────────────────────────────┐
 * │ 1. 获取磁头相关参数和分区范围                                          │
 * │ 2. 从当前位置开始遍历寻找最近的有请求的单元                            │
 * │ 3. 如果找不到有请求的单元，返回-1                                      │
 * └──────────────────────────────────────────────────────────────────────┘
 */
int Disk::_get_best_start(int op_id)
{
    // ◆ 获取磁头参数
    int &point = op_id == 1 ? point1 : point2;
    int &tokens = op_id == 1 ? tokens1 : tokens2;
    int &prev_read_token = op_id == 1 ? prev_read_token1 : prev_read_token2;
    
    // ◆ 获取分区范围
    int data_size = op_id == 1 ? data_size1 : data_size2;
    int part_start = op_id == 1 ? 1 : data_size1 + 1;
    int part_end = op_id == 1 ? data_size1 : data_size1 + data_size2;

    // ◆ 寻找最近的有请求单元
    int start = point;
    for (int i = 0; i < data_size; ++i)
    {
        start = start <= part_end ? start : part_start;
        if (!cells[start].req_ids.empty())
        {
            return start;
        }
        start = start + 1;
    }

    return -1;
}

/**
 * @brief     根据最佳路径读取数据
 * @param     start 读取起点
 * @param     op_id 磁头ID（1或2）
 * @return    std::tuple<std::string, std::vector<int>, std::vector<int>>
 *           - first: 读取路径
 *           - second: 完成的请求列表
 *           - third: 占用对象列表
 * @details   使用预计算的token表优化读取策略:
 * ┌──────────────────────────────────────────────────────────────────────┐
 * │ 1. 初始化滑动窗口和序列                                               │
 * │ 2. 根据token表构建最优读取路径                                        │
 * │ 3. 执行读取操作并更新状态                                             │
 * └──────────────────────────────────────────────────────────────────────┘
 */
std::tuple<std::string, std::vector<int>, std::vector<int>> Disk::_read_by_best_path(int start, int op_id)
{
    // ◆ 初始化返回值
    std::string path = "";
    std::vector<int> completed_reqs;
    std::vector<int> occupied_obj;

    // ◆ 获取磁头参数
    int &point = op_id == 1 ? point1 : point2;
    int &tokens = op_id == 1 ? tokens1 : tokens2;
    int &prev_read_token = op_id == 1 ? prev_read_token1 : prev_read_token2;
    int data_size = op_id == 1 ? data_size1 : data_size2;
    int part_start = op_id == 1 ? 1 : data_size1 + 1;
    int part_end = op_id == 1 ? data_size1 : data_size1 + data_size2;

    // ◆ 初始化滑动窗口
    int win_end = point;
    auto fo_seq = EMPTY_SEQUENCE;
    int point_start = point;

    // ◆ 定义读取判断函数
    auto is_read = [&](int cell_idx) -> bool
    {
        return not cells[cell_idx].req_ids.empty() and 
               cell_idx >= part_start and 
               cell_idx <= part_end;
    };

    // ◆ 初始化滑动窗口序列
    bool arrive = false;
    for (int i = 0; i < 13; ++i)
    {
        if (win_end == start) arrive = true;
        update_sequence(fo_seq, is_read(win_end));
        win_end = win_end % size + 1;
    }

    // ◆ 构建最优读取路径
    while (true)
    {
        // ● 获取下一步决策
        auto decision = get_decision(prev_read_token, fo_seq);
        if (tokens - decision.cost < 0) break;

        // ● 执行读取或跳过
        if (decision.is_r)
        {
            path.append(1, 'r');
            _read_cell(point, completed_reqs);
        }
        else
        {
            path.append(1, 'p');
        }

        // ● 更新状态
        tokens -= decision.cost;
        prev_read_token = decision.next_token;
        point = point % size + 1;
        
        // ● 更新序列
        if (win_end == start) arrive = true;
        update_sequence(fo_seq, is_read(win_end));
        win_end = win_end % size + 1;
    }

    return {path, completed_reqs, occupied_obj};
}
/*╚═════════════════════════════════════════════════════════════════════════╝*/

/*╔══════════════════════════════ 请求处理模块 ═══════════════════════════════╗*/
/**
 * @brief     读取单个磁盘单元
 * @param     cell_idx 单元索引
 * @param     completed_reqs 完成的请求列表
 * @details   执行以下步骤:
 * ┌──────────────────────────────────────────────────────────────────────┐
 * │ 1. 检查单元是否有效                                                   │
 * │ 2. 更新请求状态                                                       │
 * │ 3. 处理完成的请求                                                     │
 * │ 4. 清理单元状态                                                       │
 * └──────────────────────────────────────────────────────────────────────┘
 */
void Disk::_read_cell(int cell_idx, std::vector<int>& completed_reqs)
{
    // ◆ 检查单元有效性
    if(cells[cell_idx].req_ids.empty() or cells[cell_idx].obj_id == 0)
    {
        return;
    }
    
    // ◆ 处理单元上的所有请求
    for (int req_id : cells[cell_idx].req_ids)
    {
        // ● 更新请求状态
        controller->REQS[req_id % LEN_REQ].remain_units.erase(cells[cell_idx].unit_id);

        // ● 处理完成的请求
        if (controller->REQS[req_id % LEN_REQ].remain_units.empty())
        {
            completed_reqs.push_back(req_id);
            _update_wait_time_stats(req_id, controller->timestamp);
            controller->OBJECTS[cells[cell_idx].obj_id].req_ids.erase(req_id);
            controller->REQS[req_id % LEN_REQ].clear();
        }
    }

    // ◆ 清理单元状态
    assert(cells[cell_idx].obj_id != 0);
    cells[cell_idx].req_ids.clear();
}
/*╚═════════════════════════════════════════════════════════════════════════╝*/

/*╔══════════════════════════════ 性能统计模块 ═══════════════════════════════╗*/
/**
 * @brief     更新请求等待时间统计数据
 * @param     req_id 请求ID
 * @param     current_timestamp 当前时间戳
 * @details   执行以下步骤:
 * ┌──────────────────────────────────────────────────────────────────────┐
 * │ 1. 计算请求等待时间                                                   │
 * │ 2. 更新统计数据                                                       │
 * │ 3. 维护最近请求队列                                                   │
 * └──────────────────────────────────────────────────────────────────────┘
 */
void Disk::_update_wait_time_stats(int req_id, int current_timestamp)
{
    // ◆ 计算等待时间
    int req_timestamp = controller->REQS[req_id % LEN_REQ].timestamp;
    int wait_time = current_timestamp - req_timestamp;
    
    // ◆ 更新统计数据
    total_wait_time += wait_time;
    recent_wait_times.push_back(wait_time);
    
    // ◆ 维护最近请求队列
    if (recent_wait_times.size() > MAX_RECENT_REQS) 
    {
        total_wait_time -= recent_wait_times.front();
        recent_wait_times.pop_front();
    }
}
/*╚═════════════════════════════════════════════════════════════════════════╝*/