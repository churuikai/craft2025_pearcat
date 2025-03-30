#include "constants.h"
#include "disk_obj_req.h"
// #include "debug.h"



void process_delete(Controller& controller) {
    int n_delete;
    // std::cin >> n_delete;
    scanf("%d", &n_delete);
    
    if (n_delete == 0) {
        // std::cout << "0" << std::endl;
        printf("0\n");
        // std::cout.flush();
        fflush(stdout);
        return;
    }
    
    // 读取要删除的文件ID
    std::vector<int> delete_ids(n_delete);
    for (int i = 0; i < n_delete; ++i) {
        // std::cin >> delete_ids[i];
        scanf("%d", &delete_ids[i]);
    }
    
    // 统计需要取消的请求
    std::vector<int> aborted_requests;
    for (int obj_id : delete_ids) {
        auto reqs = controller.delete_obj(obj_id);
        aborted_requests.insert(aborted_requests.end(), reqs.begin(), reqs.end());

    }
    
    // 输出结果
    // std::cout << aborted_requests.size() << std::endl;
    printf("%d\n", aborted_requests.size());
    for (int req_id : aborted_requests) {
        // std::cout << req_id << std::endl;
        printf("%d\n", req_id);
    }
    // std::cout.flush();
    fflush(stdout);
}

    // 删除
std::vector <int> Controller::delete_obj(int obj_id) {
        // 释放磁盘
        for (auto& [disk_id, units] : OBJECTS[obj_id].replicas) {
            for (int cell_id : units) {
                // debug(DISKS[disk_id].part_tables[DISKS[disk_id].cells[cell_id].part_idx][2]);
                DISKS[disk_id].free_cell(cell_id);
                // std::string ss = "delete obj id " + std::to_string(obj_id) + " disk id " + std::to_string(disk_id) + " cell id " + std::to_string(cell_id) + " objsize " + std::to_string(OBJECTS[obj_id].size);
                // debug(ss);
                // debug(DISKS[disk_id].part_tables[DISKS[disk_id].cells[cell_id].part_idx][2]);
            }
        }
        // 释放请求
        std::vector<int> aborted_requests(OBJECTS[obj_id].req_ids.begin(), OBJECTS[obj_id].req_ids.end());
        for (int req_id : aborted_requests) {
            activate_reqs.erase(req_id);
        }
        OBJECTS[obj_id].req_ids.clear();
        return aborted_requests;
}

void Disk::free_cell(int cell_id) {
        // 清除req_pos
        req_cells_num -= cells[cell_id]->req_ids.size();
        for (int req_id : cells[cell_id]->req_ids) {
            if (req_pos.find(req_id) != req_pos.end()) {
                req_pos.erase(req_id);
            }
        }
        part_tables[cells[cell_id]->part_idx][2]++;
        cells[cell_id]->free();
    }