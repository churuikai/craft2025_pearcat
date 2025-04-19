/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 *  ████████╗ ██████╗ ██╗  ██╗███████╗███╗   ██╗   ██╗  ██╗
 *  ╚══██╔══╝██╔═══██╗██║ ██╔╝██╔════╝████╗  ██║   ██║  ██║
 *     ██║   ██║   ██║█████╔╝ █████╗  ██╔██╗ ██║   ███████║
 *     ██║   ██║   ██║██╔═██╗ ██╔══╝  ██║╚██╗██║   ██╔══██║
 *     ██║   ╚██████╔╝██║  ██╗███████╗██║ ╚████║   ██║  ██║
 *     ╚═╝    ╚═════╝ ╚═╝  ╚═╝╚══════╝╚═╝  ╚═══╝   ╚═╝  ╚═╝
 * 
 * ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 * 【模块功能】
 * ┌─────────────────┬───────────────────────────────────────────────────────────┐
 * │ 令牌转换         │ 提供令牌状态转换和索引映射功能                              │
 * ├─────────────────┼───────────────────────────────────────────────────────────┤
 * │ 决策优化         │ 实现最优读写策略的动态规划预计算                            │
 * ├─────────────────┼───────────────────────────────────────────────────────────┤
 * │ 序列管理         │ 支持滑动窗口序列的高效更新和查询                            │
 * └─────────────────┴───────────────────────────────────────────────────────────┘
 * ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/

#pragma once
#include <array>
#include <functional>

/*╔══════════════════════════════ 令牌转换函数 ═══════════════════════════════╗*/
/**
 * @brief     获取下一个令牌状态
 * @param     token 当前令牌值
 * @return    下一个令牌值
 * @details   令牌状态转换规则:
 * ┌──────────────────────────────────────────────────────────────────────┐
 * │ ● 80 → 64 → 52 → 42 → 34 → 28 → 23 → 19 → 16                         │
 * │ ● 16保持不变，其他无效值返回0                                          │
 * └──────────────────────────────────────────────────────────────────────┘
 */
inline constexpr int get_next_token(int token) {
    switch (token) {
        case 80: return 64;
        case 64: return 52;
        case 52: return 42;
        case 42: return 34;
        case 34: return 28;
        case 28: return 23;
        case 23: return 19;
        case 19: return 16;
        case 16: return 16;
        default: return 0;
    }
}

/**
 * @brief     将令牌值转换为索引
 * @param     token 令牌值
 * @return    对应的索引值[0-8]
 * @details   令牌映射规则:
 * ┌──────────────────────────────────────────────────────────────────────┐
 * │ ● 16→0, 19→1, 23→2, 28→3, 34→4                                       │
 * │ ● 42→5, 52→6, 64→7, 80→8                                             │
 * └──────────────────────────────────────────────────────────────────────┘
 */
inline constexpr int token_to_index(int token) {
    switch (token) {
        case 16: return 0;
        case 19: return 1;
        case 23: return 2;
        case 28: return 3;
        case 34: return 4;
        case 42: return 5;
        case 52: return 6;
        case 64: return 7;
        case 80: return 8;
        default: return 0;
    }
}

/**
 * @brief     将索引转换为令牌值
 * @param     index 索引值[0-8]
 * @return    对应的令牌值
 * @details   索引映射规则:
 * ┌──────────────────────────────────────────────────────────────────────┐
 * │ ● 0→16, 1→19, 2→23, 3→28, 4→34                                       │
 * │ ● 5→42, 6→52, 7→64, 8→80                                             │
 * └──────────────────────────────────────────────────────────────────────┘
 */
inline constexpr int index_to_token(int index) {
    switch (index) {
        case 0: return 16;
        case 1: return 19;
        case 2: return 23;
        case 3: return 28;
        case 4: return 34;
        case 5: return 42;
        case 6: return 52;
        case 7: return 64;
        case 8: return 80;
        default: return 0;
    }
}
/*╚═════════════════════════════════════════════════════════════════════════╝*/

/*╔══════════════════════════════ 决策结构定义 ═══════════════════════════════╗*/
/**
 * @brief     磁头操作决策结构
 * @details   包含当前位置的最优操作选择:
 * ┌──────────────────────────────────────────────────────────────────────┐
 * │ ● is_r: true表示读取(r)操作，false表示跳过(p)操作                      │
 * │ ● cost: 执行当前操作的token消耗                                       │
 * │ ● next_token: 操作后的token状态                                       │
 * └──────────────────────────────────────────────────────────────────────┘
 */
struct RP {
    bool is_r;         // 当前操作选择（true为r，false为p）
    int cost;          // 当前位置的token消耗
    int next_token;    // 执行此操作后的token状态
};

/**
 * @brief     初始化令牌决策预计算表
 * @return    完整的预计算决策表
 * @details   动态规划预计算步骤:
 * ┌──────────────────────────────────────────────────────────────────────┐
 * │ ● 遍历9种可能的prev_token值                                           │
 * │ ● 遍历8192(2^13)种可能的序列组合                                      │
 * │ ● 计算每种组合下r和p操作的最优选择                                     │
 * │ ● 记录最优决策的cost和next_token                                      │
 * └──────────────────────────────────────────────────────────────────────┘
 */
inline auto init_sequence_table() {
    // 9种可能的prev_token值，2^13种可能的序列组合
    using DecisionTable = std::array<std::array<RP, 8192>, 9>;
    DecisionTable table{};
    
    for (int token_idx = 0; token_idx < 9; ++token_idx) {
        int prev_token = index_to_token(token_idx);
        
        // 遍历所有可能的13位序列（0表示不需读取，1表示需要读取）
        for (int seq = 0; seq < 8192; ++seq) {
            // 当前位置是否需要读取
            bool curr_needs_read = seq & 1;
            
            // 存储所有可能的操作序列及其总消耗
            std::array<int, 2> min_costs = {1000000, 1000000}; // [r操作总消耗, p操作总消耗]
            std::array<int, 2> final_tokens = {0, 0}; // 对应的最终token值
            
            // 递归函数计算所有可能的操作序列组合
            std::function<void(int, int, int, int)> search = [&](int pos, int curr_token, int total_cost, int operations) {
                // 已处理完所有13个位置
                if (pos == 13) {
                    // 检查是否是最优解
                    int op_idx = (operations >> 0) & 1; // 第0位的操作类型
                    if (total_cost < min_costs[op_idx]) {
                        min_costs[op_idx] = total_cost;
                        final_tokens[op_idx] = curr_token;
                    }
                    return;
                }
                
                // 检查当前位置是否需要读取
                bool need_read = (seq >> pos) & 1;
                
                if (need_read) {
                    // 必须使用r操作
                    // 计算r操作的token消耗
                    int r_cost = get_next_token(curr_token);
                    
                    // 新的token状态是本次消耗
                    search(pos + 1, r_cost, total_cost + r_cost, operations | (1 << pos));
                } else {
                    // 可以使用r或p
                    // 尝试r操作
                    int r_cost = get_next_token(curr_token);
                    
                    // 新的token状态是本次消耗
                    search(pos + 1, r_cost, total_cost + r_cost, operations | (1 << pos));
                    
                    // 尝试p操作
                    search(pos + 1, 80, total_cost + 1, operations);
                }
            };
            
            // 计算当前位置的r操作cost
            int r_cost = get_next_token(prev_token);
            
            // 从第0个位置开始搜索
            if (curr_needs_read) {
                // 当前位置必须用r
                search(1, r_cost, r_cost, 1);
                table[token_idx][seq] = {true, r_cost, r_cost};
            } else {
                // 当前位置可以用r或p，尝试两种可能
                search(1, r_cost, r_cost, 1); // r操作
                search(1, 80, 1, 0); // p操作
                
                // 选择总消耗更小的操作
                if (min_costs[0] <= min_costs[1]) {
                    // p操作更优
                    table[token_idx][seq] = {false, 1, 80}; // p操作后token状态固定为80
                } else {
                    // r操作更优
                    table[token_idx][seq] = {true, r_cost, r_cost}; // r操作后token状态为r的消耗值
                }
            }
        }
    }
    
    return table;
}
/*╚═════════════════════════════════════════════════════════════════════════╝*/

/*╔══════════════════════════════ 全局操作函数 ═══════════════════════════════╗*/
// 全局预计算表
inline const auto RP_TABLE = init_sequence_table();

/**
 * @brief     高效更新滑动窗口序列
 * @param     seq 当前序列
 * @param     new_bit 新的位值
 * @details   序列更新规则:
 * ┌──────────────────────────────────────────────────────────────────────┐
 * │ ● 序列右移一位，丢弃最低位                                             │
 * │ ● 在最高位(第12位)添加新值                                             │
 * │ ● 保持序列长度为13位不变                                               │
 * └──────────────────────────────────────────────────────────────────────┘
 */
inline void update_sequence(uint16_t& seq, bool new_bit) {
    // 右移一位，丢弃最低位，在最高位添加新值
    seq = ((seq >> 1) | ((new_bit ? 1 : 0) << 12));
}

// 初始空序列常量
inline const uint16_t EMPTY_SEQUENCE = 0;

/**
 * @brief     查询最优决策
 * @param     prev_token 前一状态的令牌值
 * @param     sequence 当前滑动窗口序列
 * @return    最优决策结构体
 * @details   决策查询步骤:
 * ┌──────────────────────────────────────────────────────────────────────┐
 * │ ● 将prev_token转换为索引                                              │
 * │ ● 使用sequence的低13位作为查询键                                       │
 * │ ● 返回预计算表中对应的最优决策                                         │
 * └──────────────────────────────────────────────────────────────────────┘
 */
inline const RP& get_decision(int prev_token, uint16_t sequence) {
    return RP_TABLE[token_to_index(prev_token)][sequence & 0x1FFF]; // 确保只使用低13位
}
/*╚═════════════════════════════════════════════════════════════════════════╝*/

