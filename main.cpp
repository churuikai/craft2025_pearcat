#include "constants.h"
#include "disk_obj_req.h"
#include "data_analysis.h"
#include "debug.h"
#include <iostream>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <string>
#include <cmath>
#include <cassert>
#include <algorithm>
#include <limits>

#include <cstdio>
#include <cassert>
#include <cstdlib>

Disk DISKS[MAX_DISK_NUM];
Object OBJECTS[MAX_OBJECT_NUM];
Req REQS[LEN_REQ];

    // 同步时间
void Controller::sync(int timestamp) {
        this->timestamp = timestamp;
    }

void process_timestamp(int timestamp, Controller& controller) {

    scanf("%*s%*d");
    // 更新各个磁盘的tokens
    for (int i = 1; i <= N; ++i) {
        DISKS[i].tokens1 = G;
        DISKS[i].tokens2 = G;
    }
    
    // 同步时间
    controller.sync(timestamp);
    TIME = timestamp;


    printf("TIMESTAMP %d\n", timestamp);
    fflush(stdout);
}


int main() {

    // 清理debug和info日志
    init_logs();

    // 读取常量输入
    scanf("%d%d%d%d%d%d", &T, &M, &N, &V, &G, &k);
    G_float = G;

    info("=============================================================");
    info("T: "+ std::to_string(T)+", M: "+ std::to_string(M)+", N: "+ std::to_string(N)+", V: "+ std::to_string(V)+", G: "+ std::to_string(G)+", k: "+ std::to_string(k));

    // 预处理数据分析
    process_data_analysis();

    // 初始化磁盘和对象
    init();

    // 创建文件管理器
    Controller controller;

    // 输出预处理完成
    printf("OK\n");
    fflush(stdout);

    // 处理每个时间片
    for (int timestamp = 1; timestamp <= T + EXTRA_TIME; ++timestamp) {
        // 处理时间戳事件
        process_timestamp(timestamp, controller);
        
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
            process_gc(controller);
        }

    }

    info("=============================================================");
    info("busy_count: ", controller.busy_count);
    info("over_load_count: ", controller.over_load_count);
    info("=============================================================");
    info("OVER");
}