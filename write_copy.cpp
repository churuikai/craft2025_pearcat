// #include "disk_obj_req.h"
// #include "constants.h"
// #include <iostream>
// #include <algorithm>
// // #include "debug.h"

// void process_write(Controller& controller) {
//     int n_write;
//     scanf("%d", &n_write);

//     if (n_write == 0) {
//         return;
//     }
    
//     // 处理每个写入请求
//     for (int i = 0; i < n_write; ++i) {
//         int obj_id, obj_size, tag;
//         scanf("%d%d%d", &obj_id, &obj_size, &tag);

//         // 写入文件
//         Object* obj = controller.write(obj_id, obj_size, tag);

//         // 输出写入结果
//         printf("%d\n", obj_id);
//         for (const auto& [disk_id, cells] : obj->replicas) {
//             printf("%d ", disk_id);
//             for (size_t j = 0; j < cells.size(); ++j) {
//                 printf(" %d", cells[j]);    
//             }
//             printf("\n");
//         }
//     }
//     fflush(stdout);
// }

// // 获取磁盘和对应分区
// std::vector<std::pair<int, int>> Controller::_get_disk(int obj_size, int tag) {

//         std::vector<std::pair<int, int>> space; // <disk_id, part_idx>
//         // 每过cycle个周期换一次磁盘
//         int cycle = 1;
//         int tmp_time = TIME / cycle + cycle;
//         // int tmp_time = write_count;
//         // write_count++;

//         // // 优先选择对应分区有空闲空间的磁盘-分区
//         // for (int i = tmp_time; i < N + tmp_time; ++i) {
//         //     int disk_id = (i - 1) % N + 1;
//         //     if (DISKS[disk_id].part_tables[tag * 5 + 1][2]-DISKS[disk_id].tag_free_count[0] >= obj_size) {
//         //         space.push_back({disk_id, tag * 5 + 1});
//         //         if (space.size() == 3-BACK_NUM) {
//         //             break;
//         //         }
//         //     }
//         // }

//         // 优先选择对应分区有空闲空间的磁盘-分区
//         for (int i = tmp_time; i < N + tmp_time; ++i) {
//             int disk_id = (i - 1) % N + 1;
//             if (DISKS[disk_id].part_tables[tag * 5 + obj_size][2] >= obj_size) {
//                 space.push_back({disk_id, tag * 5 + obj_size});
//                 if (space.size() == 3-BACK_NUM) {
//                     break;
//                 }
//             }
//         }
//         // 再选择对应tag任一size分区有空闲空间的磁盘
//         for (int size_ = 1; size_ <= 5; ++size_) {
//             if (space.size() == 3-BACK_NUM) break;
//             if (size_ == obj_size) continue;
//             for (int i = 1 + tmp_time; i <= N + tmp_time; ++i) {
//                 int disk_id = (i - 1) % N + 1;
//                 // 检查磁盘是否已经在space中
//                 if (std::any_of(space.begin(), space.end(), [disk_id](const auto& p) { return p.first == disk_id; })) continue;
//                 if (DISKS[disk_id].part_tables[tag * 5 + size_][2] >= obj_size) {
//                     space.push_back({disk_id, tag * 5 + size_});
//                     if (space.size() == 3-BACK_NUM) {
//                         size_ = 6; break; // 跳出循环
//                     }
//                 }
//             }
//         }
//         // 再选择对应size任一tag分区有空闲空间的磁盘
//         for (int tag_ = 1; tag_ <= M; ++tag_) {
//             if (space.size() == 3-BACK_NUM) break;
//             if (tag_ == tag) continue;
//             for (int i = 1 + tmp_time; i <= N + tmp_time; ++i) {
//                 int disk_id = (i - 1) % N + 1;
//                 // 检查磁盘是否已经在space中
//                 if (std::any_of(space.begin(), space.end(), [disk_id](const auto& p) { return p.first == disk_id; })) continue;
//                 if (DISKS[disk_id].part_tables[tag_ * 5 + obj_size][2] >= obj_size) {
//                     space.push_back({disk_id, tag_ * 5 + obj_size});
//                     if (space.size() == 3-BACK_NUM) {
//                         tag_ = M+1; break; // 跳出循环
//                     }
//                 }
//             }
//         }
        
//         // 再选择任一有空闲空间的磁盘
//         for (int size_ = 1; size_ <= 5; ++size_) {
//             if (space.size() == 3-BACK_NUM) break;
//             for (int tag_ = 1; tag_ <= M; ++tag_) {
//                 for (int i = 1 + tmp_time; i <= N + tmp_time; ++i) {
//                     int disk_id = (i - 1) % N + 1;
//                     if (tag_ == tag || size_ == obj_size) continue;
//                     // 检查磁盘是否已经在space中
//                     if (std::any_of(space.begin(), space.end(), [disk_id](const auto& p) { return p.first == disk_id; })) continue;
//                     if (DISKS[disk_id].part_tables[tag_ * 5 + size_][2] >= obj_size) {
//                         space.push_back({disk_id, tag_ * 5 + size_});
//                         if (space.size() == 3-BACK_NUM) {
//                             size_ = 6; tag_ = M+1; break; // 跳出循环
//                         }
//                     }
//                 }
//             }
//         }

//         assert(space.size() == 3-BACK_NUM && "数据区域磁盘空间不足");
        
//         // 寻找备份区
//         for (int i = 1 + tmp_time; i <= N + tmp_time; ++i) {
//             if (space.size() == 3) break;
//             int disk_id = (i - 1) % N + 1;
//             if (std::any_of(space.begin(), space.end(), [disk_id](const auto& p) { return p.first == disk_id; })) continue;
//             if (DISKS[disk_id].part_tables[0][2] >= obj_size) {
//                 space.push_back({disk_id, 0});
//             }
//         }

//         assert(space.size() == 3 && "备份区磁盘空间不足");

//         return space;
//     }

// // 写入
// Object* Controller::write(int obj_id, int obj_size, int tag) {
//         // 获取对象
//         Object& obj = OBJECTS[obj_id];
//         obj.size = obj_size;
//         obj.tag = tag;
        
//         int unit_id = 1;

//         for (int i = TIME; i < N + TIME; ++i) {
//             int disk_id = (i - 1) % N + 1;
//             for (int block_start = 1; block_start <= DISKS[disk_id].part_tables[0][0]-1; block_start += BLOCK_SIZE) {
//                 if(DISKS[disk_id].cells[block_start]->obj_id != 0 && block_start%BLOCK_SIZE == 1 && DISKS[disk_id].cells[block_start+BLOCK_SIZE]->obj_id != obj_id) {
//                     continue;
//                 }
//                 for(int tmp_start = block_start; tmp_start < block_start+BLOCK_SIZE; ++tmp_start) {
//                     {
//                         if(DISKS[disk_id].cells[tmp_start]->obj_id != 0) {
//                             assert(DISKS[disk_id].cells[tmp_start]->obj_id == obj_id);
//                             continue;
//                         }
//                         DISKS[disk_id].cells[tmp_start]->obj_id = obj_id;
//                         DISKS[disk_id].cells[tmp_start]->unit_id = unit_id++;
//                         DISKS[disk_id].cells[tmp_start]->tag = tag;
//                         if(unit_id > obj_size) {
//                             i = N + TIME; block_start = DISKS[disk_id].part_tables[0][0];
//                             break;
//                         }
//                     }
//                 }
//             }
        
        
        
//         }



//         // 获取磁盘
//         auto space = _get_disk(obj_size, tag);
//         assert(space.size() == 3);

//         if (space.empty()) {
//             return nullptr;
//         }
        
//         // 写入磁盘
//         std::vector<int> units;
//         for (int i = 1; i <= obj_size; ++i) {
//             units.push_back(i);
//         }
        
//         for (size_t i = 0; i < space.size(); ++i) {
//             auto [disk_id, part_idx] = space[i];
//             auto pos = DISKS[disk_id].write(obj_id, units, tag, part_idx);
//             obj.replicas[i].first = disk_id;
//             obj.replicas[i].second.insert(obj.replicas[i].second.end(), pos.begin(), pos.end());
//         }
        
//         return &obj;
//     }


// std::vector<int> Disk::write(int obj_id, const std::vector<int>& units, int tag, int part_idx) {
//         auto& part = part_tables[part_idx];
//         // int pointer = part[0];
//         int pointer = part[3]; // 从上次写入的位置开始遍历
//         std::vector<int> result;
//         for (int unit_id : units) {
//             while (cells[pointer]->obj_id != 0) {
//                 pointer = pointer == part[1] ? part[0] : pointer % size + 1;
//             }
//             // assert(pointer <= part[1] && "分区空间不足");
//             Cell* cell = cells[pointer];
//             cell->obj_id = obj_id;
//             cell->unit_id = unit_id;
//             cell->tag = tag;
//             part[2]--;
//             result.push_back(pointer);
//             pointer = pointer == part[1] ? part[0] : pointer % size + 1;
//         }
//         part[3] = pointer;
//         return result;
//     }