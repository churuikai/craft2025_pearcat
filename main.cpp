#include "constants.h"
#include "disk_obj_req.h"
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

std::vector<std::vector<std::vector<int>>> FRE;
int T, M, N, V, G, k, TIME;

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
    // 创建文件管理器

    init_input();
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
    
    return 0;
}