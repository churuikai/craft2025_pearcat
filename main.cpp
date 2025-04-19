/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 *  ██████╗ ███████╗ █████╗ ██████╗  ██████╗ █████╗ ████████╗
 *  ██╔══██╗██╔════╝██╔══██╗██╔══██╗██╔════╝██╔══██╗╚══██╔══╝
 *  ██████╔╝█████╗  ███████║██████╔╝██║     ███████║   ██║   
 *  ██╔═══╝ ██╔══╝  ██╔══██║██╔══██╗██║     ██╔══██║   ██║   
 *  ██║     ███████╗██║  ██║██║  ██║╚██████╗██║  ██║   ██║   
 *  ╚═╝     ╚══════╝╚═╝  ╚═╝╚═╝  ╚═╝ ╚═════╝╚═╝  ╚═╝   ╚═╝   
 *                                                        
 * ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 * 【代码组织结构】
 * ┌─────────────────┬───────────────────────────────────────────────────────────┐
 * │ 头文件引入       │ 包含常量定义、数据分析、控制器与磁盘对象、调试工具等模块       │
 * ├─────────────────┼───────────────────────────────────────────────────────────┤
 * │ 函数声明         │ 声明处理各类事件的函数接口                                  │
 * ├─────────────────┼───────────────────────────────────────────────────────────┤
 * │ 主函数(main)     │ 初始化系统并执行时间循环，处理各类事件                       │
 * ├─────────────────┼───────────────────────────────────────────────────────────┤
 * │ 全局变量         │ 存储两轮交互所需的数据和指针                                │
 * └─────────────────┴───────────────────────────────────────────────────────────┘
 * 
 * 【事件处理函数】
 * ┌─────────────────────────┬───────────────────────────────────────────────────┐
 * │ process_data            │ 处理初始化数据和参数                                │
 * ├─────────────────────────┼───────────────────────────────────────────────────┤
 * │ process_incremental_info│ 处理第二轮增量信息                                  │
 * ├─────────────────────────┼───────────────────────────────────────────────────┤
 * │ process_timestamp       │ 处理时间戳更新                                     │
 * ├─────────────────────────┼───────────────────────────────────────────────────┤
 * │ process_delete          │ 处理对象删除事件                                   │
 * ├─────────────────────────┼───────────────────────────────────────────────────┤
 * │ process_write           │ 处理对象写入事件                                    │
 * ├─────────────────────────┼───────────────────────────────────────────────────┤
 * │ process_read            │ 处理对象读取事件                                    │
 * ├─────────────────────────┼───────────────────────────────────────────────────┤
 * │ process_busy            │ 处理系统繁忙事件和过载请求                          │
 * ├─────────────────────────┼───────────────────────────────────────────────────┤
 * │ process_gc              │ 处理垃圾回收事件                                    │
 * └─────────────────────────┴───────────────────────────────────────────────────┘
 * ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/

#include "constants.h"          // ⟪常量定义⟫
#include "data_analysis.h"      // ⟪数据分析相关⟫
#include "ctrl_disk_obj_req.h"  // ⟪控制器、磁盘、对象、请求相关⟫
#include "debug.h"              // ⟪调试工具⟫

/*╔══════════════════════════════ 函数声明 ═══════════════════════════════╗*/
void process_data(Controller &controller);                    // ◆ 处理输入信息
void process_incremental_info(Controller &controller, int);   // ◆ 处理第二轮增量信息
void process_timestamp(Controller &controller, int);          // ◆ 处理时间戳事件
void process_delete(Controller &controller);                  // ◆ 处理删除事件
void process_write(Controller &controller);                   // ◆ 处理写入事件
void process_read(Controller &controller);                    // ◆ 处理读取事件
void process_busy(Controller &controller);                    // ◆ 处理繁忙事件
void process_gc(Controller &controller);                      // ◆ 处理垃圾回收事件
/*╚═════════════════════════════════════════════════════════════════════════╝*/

/*╔══════════════════════════════ 主函数 ══════════════════════════════════╗*/
int main() 
{
    // ▶ debug 模式下创建 debug 和 info 日志文件
    init_logs();

    // ▶ 创建对象、请求、磁盘的交互控制器
    Controller controller;
    // ▶ 读取常量数据并进行数据分析
    process_data(controller);
    // ▶ 初始化磁盘
    controller.disk_init();

    printf("OK\n");
    fflush(stdout);

    for (int timestamp = 1; timestamp <= (T + EXTRA_TIME)*2; ++timestamp) 
    {
        // ▶ 处理时间戳事件
        process_timestamp(controller, timestamp);

        // ▶ 处理删除事件
        process_delete(controller);

        // ▶ 处理写入事件
        process_write(controller);

        // ▶ 处理读取事件
        process_read(controller);

        // ▶ 处理繁忙事件
        process_busy(controller);

        // ▶ 处理垃圾回收事件, 每1800时间片执行一次
        if (controller.timestamp % 1800 == 0)  process_gc(controller);

        // ▶ 处理第二轮开始的增量信息
        if(timestamp == T + EXTRA_TIME) 
        {
            // ● 更新控制器
            controller = Controller();
            // ● 处理增量信息
            process_incremental_info(controller, timestamp);
            // ● 初始化磁盘
            controller.disk_init();
        }
    }

    // ▶ debug 模式下打印运行日志
    info("=============================================================");
    info("busy_count: ", controller.busy_count);
    info("over_load_count: ", controller.over_load_count);
    info("=============================================================");
    info("OVER");
}
/*╚═════════════════════════════════════════════════════════════════════════╝*/


/*╔═════════════════ 全局变量 - 用于存储第一轮交互数据，供第二轮使用 ═════════════════╗*/
std::vector<int> INPUT;         // ◇ 存储所有输入数据
int INPUT_POINTER = 0;          // ◇ 当前读取位置指针
/*╚═══════════════════════════════════════════════════════════════════════════════╝*/


/*╔══════════════════════════════ 输入处理函数 ═══════════════════════════╗*/
/**
 * @brief     统一处理输入逻辑
 * @tparam    Type 输入数据类型
 * @return    Type 读取的数据
 * @details   根据当前轮次自动选择从标准输入或缓存读取数据
 * ┌──────────────────────────────────────────────────────────────────────┐
 * │ 第一轮：直接从标准输入读取并保存到缓存                               │
 * │ 第二轮：从之前保存的缓存中读取                                       │
 * └──────────────────────────────────────────────────────────────────────┘
 */
template<typename Type>
Type get_input(Controller &controller) 
{
    Type value;
    // ◆ 第一轮交互
    if(controller.timestamp_real <= T + EXTRA_TIME) 
    {
        scanf("%d", &value);
        INPUT.push_back(value);
    } 
    // ◆ 第二轮交互
    else 
    {
        value = INPUT[INPUT_POINTER++];
    }
    return value;
}
/*╚════════════════════════════════════════════════════════════════════════╝*/

/*╔════════════════════════════ 数据处理函数实现 ═══════════════════════════╗*/
/**
 * @brief     处理初始化数据输入
 * @param     controller 控制器对象
 * @details   读取系统常量参数并进行数据预处理分析
 * ┌──────────────────────────────────────────────────────────────────────┐
 * │ 读取系统关键参数：T、M、N、V、G、k                                      │
 * │ 记录日志并执行预处理                                                   │
 * └──────────────────────────────────────────────────────────────────────┘
 */
void process_data(Controller &controller) 
{
    // ◆ 读取系统常量参数
    scanf("%d%d%d%d%d%d%d", &T, &M, &N, &V, &G, &k1, &k2);

    // ◆ 记录参数信息到日志
    info("=============================================================");
    info("T:", T, "M:", M, "N:", N, "V:", V, "G:", G, "k1:", k1, "k2:", k2);

    // ◆ 预处理数据分析
    process_data_analysis();
}

/**
 * @brief     处理第二轮交互的增量信息
 * @param     controller 控制器对象
 * @param     timestamp 当前时间戳
 * @details   读取并处理第二轮新增的对象信息
 * ┌──────────────────────────────────────────────────────────────────────┐
 * │ 读取增量对象数量并处理每个对象的信息                                    │
 * └──────────────────────────────────────────────────────────────────────┘
 */
void process_incremental_info(Controller &controller, int timestamp) 
{
    int n_incremental;
    scanf("%d", &n_incremental);
    
    // ◆ 读取每个增量对象信息
    for (int i = 0; i < n_incremental; ++i) 
    {
        int obj_id, tag;
        scanf("%d%d", &obj_id, &tag);
    }
}

/**
 * @brief     处理时间戳事件
 * @param     controller 控制器对象
 * @param     timestamp 当前时间戳
 * @details   更新磁盘令牌数和控制器时间信息
 * ┌──────────────────────────────────────────────────────────────────────┐
 * │ 重置所有磁盘的令牌数                                                   │
 * │ 更新控制器时间信息                                                     │
 * │ 输出当前时间戳                                                        │
 * └──────────────────────────────────────────────────────────────────────┘
 */
void process_timestamp(Controller &controller, int timestamp) 
{
    scanf("%*s%*d");
    
    // ◆ 重置所有磁盘的令牌数
    for (int i = 1; i <= N; ++i) 
    {
        controller.DISKS[i].tokens1 = G;
        controller.DISKS[i].tokens2 = G;
        controller.DISKS[i].K = controller.timestamp_real <= T + EXTRA_TIME ? k1 : k2;
    }
    
    // ◆ 更新控制器时间信息
    controller.timestamp_real = timestamp;
    controller.timestamp = (timestamp-1) % (T + EXTRA_TIME) + 1;
    
    // ◆ 输出当前时间戳
    printf("TIMESTAMP %d\n", (timestamp-1) % (T + EXTRA_TIME) + 1);
    fflush(stdout);
}

/**
 * @brief     处理删除事件
 * @param     controller 控制器对象
 * @details   处理对象删除请求并输出被中断的请求
 * ┌──────────────────────────────────────────────────────────────────────┐
 * │ 获取删除请求数量                                                      │
 * │ 读取要删除的文件ID                                                    │
 * │ 执行删除操作并收集被中断的请求                                         │
 * │ 输出被中断的请求                                                      │
 * └──────────────────────────────────────────────────────────────────────┘
 */
void process_delete(Controller &controller) 
{
    // ◆ 获取删除请求数量
    int n_delete = get_input<int>(controller);

    // ◆ 无删除请求时直接返回
    if (n_delete == 0) 
    {
        printf("0\n");
        fflush(stdout);
        return;
    }

    // ◆ 读取要删除的文件ID
    std::vector<int> delete_ids(n_delete);
    for (int i = 0; i < n_delete; ++i) 
    {
        delete_ids[i] = get_input<int>(controller);
    }

    // ◆ 执行删除操作并收集被中断的请求
    std::vector<int> aborted_requests;
    for (int obj_id : delete_ids) 
    {
        auto reqs = controller.delete_obj(obj_id);
        aborted_requests.insert(aborted_requests.end(), reqs.begin(), reqs.end());
    }
    
    // ◆ 输出被中断的请求
    printf("%d\n", (int)aborted_requests.size());
    for (int req_id : aborted_requests) 
    {
        printf("%d\n", req_id);
    }
    fflush(stdout);
}

/**
 * @brief     处理写入事件
 * @param     controller 控制器对象
 * @details   处理对象写入请求并输出写入结果
 * ┌──────────────────────────────────────────────────────────────────────┐
 * │ 获取写入请求数量                                                      │
 * │ 处理每个写入请求并输出结果                                             │
 * └──────────────────────────────────────────────────────────────────────┘
 */
void process_write(Controller &controller) 
{
    // ◆ 获取写入请求数量
    int n_write = get_input<int>(controller);
    
    if (n_write == 0) return;

    // ◆ 处理每个写入请求
    for (int i = 0; i < n_write; ++i) 
    {
        // ● 读取对象信息
        int obj_id = get_input<int>(controller);
        int obj_size = get_input<int>(controller);
        int tag = get_input<int>(controller);
        
        // ● 执行写入操作
        Object *obj = controller.write(obj_id, obj_size, tag);
        
        // ● 输出写入结果
        printf("%d\n", obj_id);
        for (const auto &[disk_id, cell_idxs] : obj->replicas) 
        {
            printf("%d ", disk_id);
            for (size_t j = 0; j < cell_idxs.size(); ++j) 
            {
                printf(" %d", cell_idxs[j]);
            }
            printf("\n");
        }
    }
    fflush(stdout);
}

/**
 * @brief     处理读取事件
 * @param     controller 控制器对象
 * @details   处理对象读取请求并输出读取结果
 * ┌──────────────────────────────────────────────────────────────────────┐
 * │ 获取读取请求数量                                                      │
 * │ 收集读取请求                                                          │
 * │ 执行请求处理流程：前置过滤、添加请求、执行读取、后置过滤                  │
 * │ 输出磁头操作和完成的请求                                               │
 * └──────────────────────────────────────────────────────────────────────┘
 */
void process_read(Controller &controller) 
{
    // ◆ 获取读取请求数量
    int n_read = get_input<int>(controller);

    // ◆ 收集读取请求
    std::vector<std::pair<int, int>> reqs;
    for (int i = 0; i < n_read; ++i) 
    {
        int req_id = get_input<int>(controller);
        int obj_id = get_input<int>(controller);
        reqs.push_back({req_id, obj_id});
    }

    // ◆ 请求处理流程
    controller.pre_filter_req(reqs);                                  // ● 前置过滤
    for(auto [req_id, obj_id] : reqs) 
    {   // ● 添加请求
        controller.add_req(req_id, obj_id);
    }
    auto [disk_operations, completed_requests] = controller.read();  // ● 执行读取
    controller.post_filter_req();                                    // ● 后置过滤
    
    // ◆ 输出磁头操作
    for (const auto &op : disk_operations) 
    {
        printf("%s\n", op.c_str());
    }

    // ◆ 输出完成的请求
    printf("%d\n", (int)completed_requests.size());
    for (int req_id : completed_requests) 
    {
        printf("%d\n", req_id);
    }
    fflush(stdout);
}

/**
 * @brief     处理繁忙事件
 * @param     controller 控制器对象
 * @details   处理并输出被过滤的请求信息
 * ┌──────────────────────────────────────────────────────────────────────┐
 * │ 获取过滤的请求数量                                                     │
 * │ 输出过滤结果                                                          │
 * │ 清理并更新统计信息                                                     │
 * └──────────────────────────────────────────────────────────────────────┘
 */
void process_busy(Controller &controller) 
{
    // ◆ 获取过滤的请求数量
    int n_over_load = controller.over_load_reqs.size();  // ● 主动过滤的超载请求
    int n_busy = controller.busy_reqs.size();            // ● 被动过滤的繁忙请求

    // ◆ 输出过滤结果
    printf("%d\n", n_busy + n_over_load);
    
    for (int i = 0; i < n_busy; i++) 
    {
        printf("%d\n", controller.busy_reqs[i]);
    }
    for (int i = 0; i < n_over_load; i++) 
    {
        printf("%d\n", controller.over_load_reqs[i]);
    }
    fflush(stdout);

    // ◆ 清理并更新统计信息
    controller.over_load_reqs.clear();
    controller.busy_reqs.clear();
    controller.busy_count += n_busy;
    controller.over_load_count += n_over_load;
}

/**
 * @brief     处理垃圾回收事件
 * @param     controller 控制器对象
 * @details   对每个磁盘执行垃圾回收操作
 * ┌──────────────────────────────────────────────────────────────────────┐
 * │ 处理垃圾回收事件输入                                                   │
 * │ 对每个磁盘执行垃圾回收并输出结果                                        │
 * └──────────────────────────────────────────────────────────────────────┘
 */
void process_gc(Controller &controller) 
{
    if(controller.timestamp_real <= T + EXTRA_TIME) 
    {
        (void)scanf("%*s%*s");
    }
    printf("GARBAGE COLLECTION\n");

    // ◆ 对每个磁盘执行垃圾回收
    for (int i = 1; i <= N; i++) 
    {
        auto gc_pairs = controller.DISKS[i].gc();
        printf("%d\n", (int)gc_pairs.size());
        for (auto &pair : gc_pairs) 
        {
            printf("%d %d\n", pair.first, pair.second);
        }
    }
    fflush(stdout);
}
/*╚═════════════════════════════════════════════════════════════════════════╝*/