#include "disk_obj_req.h"

void process_gc(Controller &controller)
{
    // 空读取 “GARBAGE COLLECTION”
    scanf("%*s%*s");
    printf("GARBAGE COLLECTION\n");
    for(int i = 0; i < N; i++) {
        printf("0\n");
    }
    fflush(stdout);
}

