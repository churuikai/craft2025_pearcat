#pragma once
#include "controller.h"
#include "data_analysis.h"

inline std::vector<int> INPUT;
inline int INPUT_POINTER = 0;
inline int REAL_TIME_TIMESTAMP = 1;

inline void process_incremental_info(Controller &controller, int timestamp)
{
    int n_incremental;
    scanf("%d", &n_incremental);
    for (int i = 0; i < n_incremental; ++i)
    {
        int obj_id, tag;
        scanf("%d%d", &obj_id, &tag);
    }
}


inline void process_timestamp(Controller &controller, int timestamp)
{
    scanf("%*s%*d");
    REAL_TIME_TIMESTAMP = timestamp;

    // 更新各个磁盘的tokens
    for (int i = 1; i <= N; ++i)
    {
        // controller.DISKS[i].tokens1 = G + get_token(timestamp);
        // controller.DISKS[i].tokens1 = G + get_token(timestamp);
        controller.DISKS[i].tokens1 = G;
        controller.DISKS[i].tokens2 = G;
        controller.DISKS[i].K = k;
    }
    // 同步时间
    controller.timestamp = (timestamp-1) % (T + EXTRA_TIME) + 1;
    printf("TIMESTAMP %d\n", (timestamp-1) % (T + EXTRA_TIME) + 1);
    fflush(stdout);
};

inline void process_delete(Controller &controller)
{
    int n_delete;
    if(REAL_TIME_TIMESTAMP <= T + EXTRA_TIME) 
    {
        scanf("%d", &n_delete);
        INPUT.push_back(n_delete);
    } 
    else
    {
        n_delete = INPUT[INPUT_POINTER++];
    }

    if (n_delete == 0)
    {
        printf("0\n");
        fflush(stdout);
        return;
    }

    // 读取要删除的文件ID
    std::vector<int> delete_ids(n_delete);
    for (int i = 0; i < n_delete; ++i)
    {
        if(REAL_TIME_TIMESTAMP <= T + EXTRA_TIME) 
        {
            scanf("%d", &delete_ids[i]);
            INPUT.push_back(delete_ids[i]);
        }
        else
        {
            delete_ids[i] = INPUT[INPUT_POINTER++];
        }
    }

    // 统计需要取消的请求
    std::vector<int> aborted_requests;
    for (int obj_id : delete_ids)
    {
        auto reqs = controller.delete_obj(obj_id);
        aborted_requests.insert(aborted_requests.end(), reqs.begin(), reqs.end());
    }
    // 输出结果
    printf("%d\n", (int)aborted_requests.size());
    for (int req_id : aborted_requests)
    {
        printf("%d\n", req_id);
    }
    fflush(stdout);
}

inline void process_write(Controller &controller)
{
    int n_write;
    if(REAL_TIME_TIMESTAMP <= T + EXTRA_TIME) 
    {
        scanf("%d", &n_write);
        INPUT.push_back(n_write);
    } 
    else
    {
        n_write = INPUT[INPUT_POINTER++];
    }
    if (n_write == 0)
        return;
    // 处理每个写入请求
    for (int i = 0; i < n_write; ++i)
    {
        int obj_id, obj_size, tag;
        if(REAL_TIME_TIMESTAMP <= T + EXTRA_TIME) 
        {
            scanf("%d%d%d", &obj_id, &obj_size, &tag);
            INPUT.push_back(obj_id);
            INPUT.push_back(obj_size);
            INPUT.push_back(tag);
        } 
        else
        {
            obj_id = INPUT[INPUT_POINTER++];
            obj_size = INPUT[INPUT_POINTER++];
            tag = INPUT[INPUT_POINTER++];
        }
        Object *obj = controller.write(obj_id, obj_size, tag);
        printf("%d\n", obj_id);
        for (const auto &[disk_id, cell_idxs] : obj->replicas)
        {
            printf("%d ", disk_id);
            for (size_t j = 0; j < cell_idxs.size(); ++j)
                printf(" %d", cell_idxs[j]);
            printf("\n");
        }
    }
    fflush(stdout);
}

inline void process_read(Controller &controller)
{
    int n_read;
    if(REAL_TIME_TIMESTAMP <= T + EXTRA_TIME) 
    {
        scanf("%d", &n_read);
        INPUT.push_back(n_read);
    } 
    else
    {
        n_read = INPUT[INPUT_POINTER++];
    }
    // 添加请求
    std::vector<std::pair<int, int>> reqs;
    for (int i = 0; i < n_read; ++i)
    {
        int req_id, obj_id;
        if(REAL_TIME_TIMESTAMP <= T + EXTRA_TIME) 
        {
            scanf("%d %d", &req_id, &obj_id);
            INPUT.push_back(req_id);
            INPUT.push_back(obj_id);
        } 
        else
        {
            req_id = INPUT[INPUT_POINTER++];
            obj_id = INPUT[INPUT_POINTER++];
        }
        reqs.push_back({req_id, obj_id});
    }

    // 前置过滤
    // controller.pre_filter_req(reqs);

    // 添加请求
    for(auto [req_id, obj_id] : reqs)
    {
        controller.add_req(req_id, obj_id);
    }

    // 处理所有请求
    auto [disk_operations, completed_requests] = controller.read();

    // 后置请求过滤
    controller.post_filter_req();
    
    // 输出磁头操作
    for (const auto &op : disk_operations)
    {
        printf("%s\n", op.c_str());
    }

    // 输出完成的请求
    printf("%d\n", (int)completed_requests.size());
    for (int req_id : completed_requests)
    {
        printf("%d\n", req_id);
    }
    fflush(stdout);
}


inline void process_busy(Controller &controller)
{
    // 输出主动过滤的超载请求
    int n_over_load = controller.over_load_reqs.size();
    // 输出被动过滤的繁忙请求
    int n_busy = controller.busy_reqs.size();

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

    controller.over_load_reqs.clear();
    controller.busy_reqs.clear();

    controller.busy_count += n_busy;
    controller.over_load_count += n_over_load;
}

inline void process_gc(Controller &controller)
{
    if(REAL_TIME_TIMESTAMP <= T + EXTRA_TIME) 
        (void)scanf("%*s%*s");
    printf("GARBAGE COLLECTION\n");

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