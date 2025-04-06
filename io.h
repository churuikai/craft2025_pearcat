#pragma once
#include "controller.h"

inline void process_timestamp(Controller &controller, int timestamp)
{
    scanf("%*s%*d");
    // 更新各个磁盘的tokens
    for (int i = 1; i <= N; ++i) {
        controller.DISKS[i].tokens1 = G;
        controller.DISKS[i].tokens2 = G;
    }
    // 同步时间
    controller.timestamp = timestamp;

    printf("TIMESTAMP %d\n", timestamp);
    fflush(stdout);

};
void process_delete(Controller &controller);
void process_write(Controller &controller);
void process_read(Controller &controller);
void process_busy(Controller &controller);
void process_gc(Controller &controller);