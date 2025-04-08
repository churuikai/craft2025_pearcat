#include "constants.h"
#include "data_analysis.h"
#include "io.h"
#include "controller.h"
#include "disk_obj_req.h"
#include "verify.h"
#include "debug.h"


int main() {

    // 清理debug和info日志
    init_logs();

    // 创建文件管理器
    Controller controller;

    // 读取常量输入
    scanf("%d%d%d%d%d%d", &T, &M, &N, &V, &G, &k);
    G_float = G;

    info("=============================================================");
    info("T: "+ std::to_string(T)+", M: "+ std::to_string(M)+", N: "+ std::to_string(N)+", V: "+ std::to_string(V)+", G: "+ std::to_string(G)+", k: "+ std::to_string(k));

    // 预处理数据分析
    process_data_analysis();

    // 初始化磁盘
    controller.disk_init();

    printf("OK\n");
    fflush(stdout);

    // 处理每个时间片
    for (int timestamp = 1; timestamp <= T + EXTRA_TIME; ++timestamp) {
        // 处理时间戳事件
        process_timestamp(controller, timestamp);
        
        // 处理删除事件
        process_delete(controller);
        
        // 处理写入事件
        process_write(controller);
        
        // 处理读取事件
        process_read(controller);
        
        // 处理繁忙事件
        process_busy(controller);

        // 处理垃圾回收事件, 每1800时间片执行一次
        if (timestamp % 1800 == 0) {
            debug("timestamp: ", timestamp);
            process_gc(controller);

        }

        // 验证数据结构
        if (timestamp % 10000 == 0) {
            process_verify(controller);
        }
    }

    info("=============================================================");
    info("busy_count: ", controller.busy_count);
    info("over_load_count: ", controller.over_load_count);
    info("=============================================================");
    info("OVER");
}