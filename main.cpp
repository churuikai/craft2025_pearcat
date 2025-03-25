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
int T, M, N, V, G, TIME;

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
        DISKS[i].tokens = G;
    }
    
    // 同步时间
    controller.sync(timestamp);
    TIME = timestamp;

    // 清理长周期的请求
    // if (timestamp % 40 == 0) {
    //     for (int req_id : controller.activate_reqs) {
    //         if (REQS[req_id % LEN_REQ].timestamp + 105 < timestamp) {
    //             controller.activate_reqs.erase(req_id);
    //         // 释放磁盘
    //         for (auto &[disk_id, cell_idxs] : OBJECTS[REQS[req_id % LEN_REQ].obj_id].replicas) {
    //             for (int cell_idx : cell_idxs) {
    //                     DISKS[disk_id].cells[cell_idx].req_ids.erase(req_id);
    //                 }
    //             }
    //         }
    //     }
    // }
    
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
    // std::cout << "OK" << std::endl;
    // std::cout.flush();
    
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
    }
    
    return 0;
}