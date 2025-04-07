#include "disk_obj_req.h"
#include "controller.h"
#include "debug.h"
#include "data_analysis.h"
#include <climits>
#include <iomanip>
#include <sstream>

void process_gc(Controller &controller)
{
    (void)scanf("%*s%*s");
    

    printf("GARBAGE COLLECTION\n");
    for(int i = 0; i < N; i++) {
        printf("0\n");
    }
    fflush(stdout);
}

