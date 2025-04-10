#include "constants.h"
#include "disk_obj_req.h"
#include "controller.h"
#include <limits>
#include "debug.h"
#include "data_analysis.h"


void Controller::disk_init() {
    info("=============================================================");
    info("备份区数量: "+ std::to_string(BACK_NUM)+ ", 是否反向: "+ std::to_string(IS_INTERVAL_REVERSE)+ ", 是否按大小分配: "+ std::to_string(IS_PART_BY_SIZE));

    // tag_order = {14, 13, 15, 9, 11, 2, 5, 6, 7, 10, 4, 8, 1, 12, 16, 3};
    // tag_order = {14, 3, 13, 16, 15, 12, 9, 1, 11, 8, 2, 4, 5,10, 6, 7};
    // tag_order = {9, 1, 13, 16, 15, 12,  11, 8, 5,10, 6, 7,  2, 4,3,14};
    // tag_order = {14, 7, 13, 10, 15, 4, 9, 8, 11, 1, 2, 12, 5,16, 6, 3};
    // tag_order = {13, 14, 1, 5, 9, 10, 4, 3, 6, 15, 7, 8, 2, 12, 16, 11}; // official
    // tag_order = {13, 6, 10, 15, 1,7, 5,8, 9,2, 14,12, 4,16, 3,11}; // official 最高
    // tag_order = {13, 6, 14, 15, 1,7, 5,8, 9,2, 10,12, 4,16, 3,11}; // official
    // tag_order = {13, 11, 14, 16, 1, 12, 5, 2, 9, 8, 10, 7, 4, 15, 3, 6};
    // std::vector<int> tag_order = {1, 4, 13, 7, 12, 9, 3, 16, 6, 14, 10, 8, 11, 2, 5, 15};
    // {1, 6, 4, 14, 13, 10, 7, 8, 12, 11, 9, 2, 3, 5, 16, 15},
    // {1, 15, 4, 5, 13, 2, 7, 11, 12, 8, 9, 10, 3, 14, 16, 6},
    // TAG_ORDERS = {
    //     {16, 6, 14, 13, 12, 3, 8, 10, 9, 4, 7, 11, 5, 2, 15, 1},
    //     {16, 6, 14, 13, 12, 3, 8, 10, 9, 4, 7, 11, 5, 2, 15, 1},
    //     {16, 6, 14, 13, 12, 3, 8, 10, 9, 4, 7, 11, 5, 2, 15, 1},
    //     {16, 6, 14, 13, 12, 3, 8, 10, 9, 4, 7, 11, 5, 2, 15, 1},
    //     {16, 6, 14, 13, 12, 3, 8, 10, 9, 4, 7, 11, 5, 2, 15, 1},
    //     {16, 6, 14, 13, 12, 3, 8, 10, 9, 4, 7, 11, 5, 2, 15, 1},
    //     {16, 6, 14, 13, 12, 3, 8, 10, 9, 4, 7, 11, 5, 2, 15, 1},
    //     {16, 6, 14, 13, 12, 3, 8, 10, 9, 4, 7, 11, 5, 2, 15, 1},
    //     {16, 6, 14, 13, 12, 3, 8, 10, 9, 4, 7, 11, 5, 2, 15, 1},
    //     {16, 6, 14, 13, 12, 3, 8, 10, 9, 4, 7, 11, 5, 2, 15, 1},
        // {1, 4, 6, 15, 2, 10, 13, 8, 14, 3, 12, 9, 16, 7, 11, 5},
        // {1, 4, 6, 15, 2, 10, 13, 8, 14, 3, 12, 9, 16, 7, 11, 5},
        // {1, 4, 6, 15, 2, 10, 13, 8, 14, 3, 12, 9, 16, 7, 11, 5},
        // {1, 4, 6, 15, 2, 10, 13, 8, 14, 3, 12, 9, 16, 7, 11, 5},
        // {1, 4, 6, 15, 2, 10, 13, 8, 14, 3, 12, 9, 16, 7, 11, 5},
        // {1, 4, 6, 15, 2, 10, 13, 8, 14, 3, 12, 9, 16, 7, 11, 5},
        // {1, 4, 6, 15, 2, 10, 13, 8, 14, 3, 12, 9, 16, 7, 11, 5},
        // {1, 4, 6, 15, 2, 10, 13, 8, 14, 3, 12, 9, 16, 7, 11, 5},
        // {1, 4, 6, 15, 2, 10, 13, 8, 14, 3, 12, 9, 16, 7, 11, 5},
        // {1, 4, 6, 15, 2, 10, 13, 8, 14, 3, 12, 9, 16, 7, 11, 5},

    // };

    info("实际磁盘标签顺序==============================================");
    for(int disk_id=0; disk_id<N; ++disk_id)
    {
        info(TAG_ORDERS[disk_id]);
    }

    info("标签占用空间比例==============================================");
    std::string tag_size_rate_info = "";
    for(int i=1; i<M+1; ++i) {
        tag_size_rate_info += "tag"+std::to_string(i)+": "+std::to_string(TAG_SIZE_RATE[i])+"; ";
    }
    info(tag_size_rate_info);


    // 初始化各个磁盘
    for (int i = 1; i <= N; ++i) {
        DISKS[i].id = i;
        DISKS[i].back = BACK_NUM;
        DISKS[i].controller = this;
        DISKS[i].init(V, TAG_ORDERS[i-1], TAG_SIZE_RATE, TAG_SIZE_DB);
    }

    // 输出每个磁盘的分区表信息
    info("磁盘分区表信息==============================================");
    for (int i = 1; i <= N; ++i) {
        std::string disk_info = "disk " + std::to_string(i) + ": ";
        // 输出备份区
        for (auto& part : DISKS[i].get_parts(0, 0)) {
            disk_info += "back-part (" + std::to_string(part.start) + "-" + std::to_string(part.end) + 
                         ", " + std::to_string(part.free_cells) + "); ";
        }
        // 输出数据区
        for (int tag_idx = 0; tag_idx < TAG_ORDERS[i-1].size(); ++tag_idx) {
            int tag_id = TAG_ORDERS[i-1][tag_idx];
            if (IS_PART_BY_SIZE) {
                // 如果按大小分区，输出所有size的分区
                for (int size = 1; size <= 5; ++size) {
                    for (auto& part : DISKS[i].get_parts(tag_id, size)) {
                        disk_info += "tag-" + std::to_string(tag_id) + " size-" + std::to_string(size) + "(" + 
                                     std::to_string(part.start) + "-" + std::to_string(part.end) + 
                                     ", " + std::to_string(part.free_cells) + "); ";
                    }
                }
            } else {
                // 如果不按大小分区，只输出size为1的分区
                for (auto& part : DISKS[i].get_parts(tag_id, 1)) {
                    disk_info += "tag-" + std::to_string(tag_id) + "(" + 
                                 std::to_string(part.start) + "-" + std::to_string(part.end) + 
                                 ", " + std::to_string(part.free_cells) + "); ";
                }
            }
        }
        info(disk_info);
    }
} 

// Disk::init的实现
void Disk::init(int size, const std::vector<int>& tag_order, const std::vector<double>& tag_size_rate, const std::vector<std::vector<double>>& tag_size_db) {
    // 预分配所有可能的req_pos空间
    req_pos.reserve(300000);
    this->size = size;
    
    // 分配cells内存
    cells = new Cell*[size+1];
    for (int i = 1; i <= size; i++) {
        cells[i] = new Cell();
    }

    // 针对磁盘的具体标签来调整合适的空间，针对非一个磁盘16种标签的策略
    auto this_tag_size_rate = tag_size_rate;
    // float all_rate = 0;
    // for(int tag: tag_order) {
    //     all_rate += this_tag_size_rate[tag];
    // }

    // for(int tag: tag_order) {
    //     this_tag_size_rate[tag] /= all_rate;
    // }

    // float data_rate = all_rate*(1.0*M/tag_order.size());
    float data_rate = 1;
    
    // 磁盘分区
    // tag 0 : 备份区，占比 90%*back/3*size
    // tag 1.1 1.2 1.3 1.4 1.5 ~M.1 M.2 M.3 M.4 M.5 : 数据区，占比 size-90%*back/3*size
    // 冗余区 tag 17.1 17.2 17.3 17.4 17.5 : 数据区空间不够时动态扩充的部分
    int back_size = this->back == 0 ? 0 : static_cast<int>(0.305 * this->back * size) + 1;
    int data_size = size - back_size;
    //调整
    data_size = static_cast<int>(data_size * data_rate);
    back_size = this->back == 0 ? 0 : size - data_size;
    
    part_tables.resize((M + 2) * 5 + 1);

    // 备份区初始化 {start, end, size, pointer}
    get_parts(0, 0).push_back(Part(data_size + 1, size, back_size, data_size+1, 0, 0));

    // 数据区压缩系数

    // 数据1区初始化 {start, end, size, pointer}
    data_size1 = data_size/2;
    int pointer_temp = 1;
    for (int tag_id : tag_order) 
    {
        assert(not IS_PART_BY_SIZE);
        int tag_id_end;
        if(tag_id == tag_order[0] or tag_id == tag_order[tag_order.size()-1]) 
            tag_id_end = pointer_temp + static_cast<int>(DATA_COMPRESSION*data_size1 * this_tag_size_rate[tag_id]*1.5) - 1;
        else
            tag_id_end = pointer_temp + static_cast<int>(DATA_COMPRESSION*data_size1 * this_tag_size_rate[tag_id]) - 1;
        // 由大到小分配
        if (IS_PART_BY_SIZE) 
        {
            // 分配 size为2-5的区
            for (int i = 5; i > 1; --i) 
            {
                // 计算每个分区的 size = date_size*该tag比例*该tag对应大小i的比例
                if(tag_size_db[tag_id][i - 1]==0) continue;
                int size_temp = static_cast<int>(data_size1 * this_tag_size_rate[tag_id] * tag_size_db[tag_id][i - 1]);
                size_temp = size_temp - size_temp % i; // 取整对齐粒度
                auto& this_tables = get_parts(tag_id, i);
                this_tables.push_back(Part(pointer_temp, pointer_temp + size_temp - 1, size_temp, pointer_temp, tag_id, i));
                pointer_temp = this_tables.back().end + 1;
            }
        }
        // 分配 size为1的区，如果不按大小分配，则所有size都分配到size=1区
        get_parts(tag_id, 1).push_back(Part(pointer_temp, tag_id_end, tag_id_end - pointer_temp + 1, pointer_temp, tag_id, 1));
        pointer_temp = tag_id_end + 1;
    }

    // 冗余区
    assert(data_size1 - pointer_temp + 1 >= 0);
    auto& dynamic_tables1 = get_parts(17, 1);
    dynamic_tables1.push_back(Part(pointer_temp, data_size1, data_size1 - pointer_temp + 1, pointer_temp, 17, 1));
    pointer_temp = dynamic_tables1.back().end + 1;

    // 调整边界
    // auto& this_tables1 = get_parts(tag_order.back(), 1);
    // this_tables1.back().end = data_size1;
    // this_tables1.back().free_cells = data_size1 - this_tables1.back().start + 1;

    // 数据2区初始化
    data_size2 = data_size - data_size1;
    pointer_temp = 1+data_size1;
    for (int tag_id : tag_order) 
    {
        int tag_id_end = pointer_temp + static_cast<int>(DATA_COMPRESSION*data_size2 * this_tag_size_rate[tag_id]) - 1;
        // 由大到小分配
        if (IS_PART_BY_SIZE) 
        {
            // 分配 size为2-5的区
            for (int i = 5; i > 1; --i) 
            {
                // 计算每个分区的 size = date_size*该tag比例*该tag对应大小i的比例
                if(tag_size_db[tag_id][i - 1]==0) continue;
                int size_temp = static_cast<int>(data_size2 * this_tag_size_rate[tag_id] * tag_size_db[tag_id][i - 1]);
                size_temp = size_temp - size_temp % i; // 取整对齐粒度
                auto& this_tables = get_parts(tag_id, i);
                this_tables.push_back(Part(pointer_temp, pointer_temp + size_temp - 1, size_temp, pointer_temp, tag_id, i));
                pointer_temp = this_tables.back().end + 1;
            }
        }
        // 分配 size为1的区，如果不按大小分配，则所有size都分配到size=1区
        get_parts(tag_id, 1).push_back(Part(pointer_temp, tag_id_end, tag_id_end - pointer_temp + 1, pointer_temp, tag_id, 1));
        pointer_temp = tag_id_end + 1;
    }

    // 冗余区
    assert(data_size - pointer_temp + 1 >= 0);
    auto& dynamic_tables2 = get_parts(17, 1);
    dynamic_tables2.push_back(Part(pointer_temp, data_size, data_size - pointer_temp + 1, pointer_temp, 17, 1));
    pointer_temp = dynamic_tables2.back().end + 1;

    // 调整边界
    // auto& this_tables2 = get_parts(tag_order.back(), 1);
    // this_tables2.back().end = data_size;
    // this_tables2.back().free_cells = data_size - this_tables2.back().start + 1;

    // 备份区初始化单元
    for (auto& part : get_parts(0,0)) {
        for (int cell_id = part.start; cell_id <= part.end; ++cell_id) {
            cells[cell_id]->part = &part;
        }
    }
    // 数据区初始化单元
    for (int tag: tag_order) {
        for (int size = 1; size <= 5; ++size) {
            for (auto& part : get_parts(tag, size)) {
                for (int cell_id = part.start; cell_id <= part.end; ++cell_id) {
                    cells[cell_id]->part = &part;
                }
            }
        }
    }
    // 冗余区初始化单元
    for (auto& part : get_parts(17, 1)) {
        for (int cell_id = part.start; cell_id <= part.end; ++cell_id) {
            cells[cell_id]->part = &part;
        }
    }

    // 标签间接反向
    if(IS_INTERVAL_REVERSE) {
        // part_tables[0].start, part_tables[0].end = part_tables[0].end, part_tables[0].start;
        // 备份区反向
        auto& back_tables = get_parts(0, 0);
        std::swap(back_tables[0].start, back_tables[0].end);   

        // 数据区间歇反向, 从第一个开始
        for(int i = 0; i<tag_order.size(); i+=2) {
            for(int j=1; j<=5; ++j) {
                for(auto& part : get_parts(tag_order[i], j)) {
                    // part.start, part.end = part.end, part.start;
                    std::swap(part.start, part.end);
                }
            }
        }
        // 记录标签反向的对象
        tag_reverse[tag_order[0]] = tag_order[tag_order.size()-1];
        tag_reverse[tag_order[tag_order.size()-1]] = tag_order[0];
        for(int i = 2; i < tag_order.size(); i+=2) {
            assert(tag_order[i-1] != 0);
            tag_reverse[tag_order[i-1]] = tag_order[i];
            tag_reverse[tag_order[i]] = tag_order[i-1];
        }

        // 冗余区反向
        for(auto& part : get_parts(17, 1)) {
            std::swap(part.start, part.end);
        }
    }
    
    // 初始化所有分区的空闲块链表（除备份区外）
    for (int tag : tag_order) {
        for (int size = 1; size <= 5; ++size) {
            for (auto& part : get_parts(tag, size)) {
                if (part.free_cells > 0) {
                    part.init_free_list();
                }
            }
        }
    }
    
    // 初始化冗余区的空闲块链表
    for(int size=1; size<=5; ++size) {
        for (auto& part : get_parts(17, size)) {
            if (part.free_cells > 0) {
                part.init_free_list();
            }
        }
    }
}

// 添加Disk析构函数实现
Disk::~Disk() {
    // 清理所有分区的空闲块链表
    for (auto& tag_parts : part_tables) {
        for (auto& part : tag_parts) {
            part._clear_free_list();
        }
    }
    
    // 释放cells内存
    if (cells) {
        for (int i = 1; i <= size; i++) {
            delete cells[i];
        }
        delete[] cells;
        cells = nullptr;
    }
}