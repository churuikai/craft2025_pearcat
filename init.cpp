#include "constants.h"
#include "disk_obj_req.h"
#include <limits>
#include "debug.h"
void init_input() {
    // 读取基本参数
    scanf("%d%d%d%d%d", &T, &M, &N, &V, &G);

    G_float = G;

    // 初始化频率数据
    FRE.resize(MAX_TAG_NUM + 1, std::vector<std::vector<int>>((MAX_SLICING_NUM + 1) / FRE_PER_SLICING + 1, std::vector<int>(3, 0)));
    
    // 读取删除频率数据
    for (int tag_id = 1; tag_id <= M; ++tag_id) {
        for (int i = 1; i <= (T - 1) / FRE_PER_SLICING + 1; ++i) {
            scanf("%d", &FRE[tag_id][i][0]);
        }
    }
    // 读取写入频率数据
    for (int tag_id = 1; tag_id <= M; ++tag_id) {
        for (int i = 1; i <= (T - 1) / FRE_PER_SLICING + 1; ++i) {
            scanf("%d", &FRE[tag_id][i][1]);
        }
    }   
    // 读取读取频率数据
    for (int tag_id = 1; tag_id <= M; ++tag_id) {
        for (int i = 1; i <= (T - 1) / FRE_PER_SLICING + 1; ++i) {
            scanf("%d", &FRE[tag_id][i][2]);
        }
    }
    
    // 读取次数最大的标签
    int max_read_tag = 0;
    int max_read_count = 0;
    for (int tag_id = 1; tag_id <= M; ++tag_id) {
        int count = 0;
        for (int i = 0; i < M + 1; ++i) {
            count += FRE[tag_id][i][0];
        }
        if (count > max_read_count) {
            max_read_count = count;
            max_read_tag = tag_id;
        }
    }
    // debug(max_read_tag);
    std::vector<int> tag_order = {max_read_tag};
    
    // 按读频率曲线最相似排列
    while (tag_order.size() < static_cast<size_t>(M)) {
        double min_diff_sum = std::numeric_limits<double>::infinity();
        int min_diff_tag = 0;
        std::vector<double> fre_tmp_old;
        
        for (int i = 0; i < T / FRE_PER_SLICING + 1; ++i) {
            fre_tmp_old.push_back(FRE[tag_order.back()][i][0]);
        }
        
        double max_val = *std::max_element(fre_tmp_old.begin(), fre_tmp_old.end());
        if (max_val > 0) {
            for (double& val : fre_tmp_old) {
                val /= max_val;
            }
        }
        
        std::vector<double> fre_tmp_new;
        
        for (int tag_id = 1; tag_id <= M; ++tag_id) {
            if (std::find(tag_order.begin(), tag_order.end(), tag_id) != tag_order.end()) {
                continue;
            }
            
            std::vector<double> fre_tmp;
            for (int i = 0; i < T / FRE_PER_SLICING + 1; ++i) {
                fre_tmp.push_back(FRE[tag_id][i][0]);
            }
            
            double max_val = *std::max_element(fre_tmp.begin(), fre_tmp.end());
            if (max_val > 0) {
                for (double& val : fre_tmp) {
                    val /= max_val;
                }
            }
            
            // 差异总和
            double diff_sum = 0;
            for (size_t i = 0; i < fre_tmp.size(); ++i) {
                diff_sum += std::abs(fre_tmp[i] - fre_tmp_old[i]);
            }
            
            if (diff_sum < min_diff_sum) {
                min_diff_sum = diff_sum;
                min_diff_tag = tag_id;
                fre_tmp_new = fre_tmp;
            }
        }
        
        fre_tmp_old = fre_tmp_new;
        tag_order.push_back(min_diff_tag);
    }
 
    debug(tag_order);

    // tag_order = {14, 13, 15, 9, 11, 2, 5, 6, 7, 10, 4, 8, 1, 12, 16, 3};
    // tag_order = {14, 3, 13, 16, 15, 12, 9, 1, 11, 8, 2, 4, 5,10, 6, 7};
    // tag_order = {9, 1, 13, 16, 15, 12,  11, 8, 5,10, 6, 7,  2, 4,3,14};
    // tag_order = {14, 7, 13, 10, 15, 4, 9, 8, 11, 1, 2, 12, 5,16, 6, 3};
    // tag_order = {13, 14, 1, 5, 9, 10, 4, 3, 6, 15, 7, 8, 2, 12, 16, 11}; // official
    tag_order = {13, 6, 10, 15, 1,7, 5,8, 9,2, 14,12, 4,16, 3,11}; // official
    // tag_order = {13, 6, 14, 15, 1,7, 5,8, 9,2, 10,12, 4,16, 3,11}; // official
    // tag_order = {13, 11, 14, 16, 1, 12, 5, 2, 9, 8, 10, 7, 4, 15, 3, 6};
    // 初始化各个磁盘
    for (int i = 1; i <= N; ++i) {
        DISKS[i].id = i;
        DISKS[i].back = BACK_NUM;
        DISKS[i].init(V, tag_order, TAG_SIZE_RATE, TAG_SIZE_DB);
    }
} 

// Disk::init的实现
void Disk::init(int size, const std::vector<int>& tag_order, const std::vector<double>& tag_size_rate, const std::vector<std::vector<double>>& tag_size_db) {
    // 预分配所有可能的req_pos空间
    // req_pos.reserve(300000); // 估计请求数量
    this->size = size;
    
    // 分配cells内存
    cells = new Cell*[size+1];
    for (int i = 1; i <= size; i++) {
        cells[i] = new Cell();
    }
    
    // 磁盘分区
    // tag 0 : 备份区，占比 90%*back/3*size
    // tag 1.1 1.2 1.3 1.4 1.5 ~M.1 M.2 M.3 M.4 M.5 : 数据区，占比 size-90%*back/3*size
    int back_size = this->back == 0 ? 0 : static_cast<int>(0.305 * this->back * size) + 1;
    int data_size = size - back_size;
    
    part_tables.resize((tag_order.size() + 1) * 5 + 1);

    // 备份区初始化 {start, end, size, pointer}
    get_parts(0, 0).push_back(Part(data_size + 1, size, back_size, data_size+1, 0, 0));



    // 数据区初始化 {start, end, size, pointer}
    int pointer_temp = 1;
    for (int tag_id : tag_order) 
    {
        int tag_id_end = pointer_temp + static_cast<int>(0.84*data_size * tag_size_rate[tag_id]) - 1;
        // 由大到小分配
        if (IS_PART_BY_SIZE) 
        {
            // 分配 size为2-5的区
            for (int i = 5; i > 1; --i) 
            {
                // 计算每个分区的 size = date_size*该tag比例*该tag对应大小i的比例
                int size_temp = static_cast<int>(data_size * tag_size_rate[tag_id] * tag_size_db[tag_id][i - 1]);
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

    
    // 调整边界
    auto& this_tables = get_parts(tag_order.back(), 1);
    if(this_tables.size() == 0) assert(false);
    this_tables.back().end = data_size;
    this_tables.back().free_cells = data_size - this_tables.back().start + 1;
    
    // 初始化单元
    // for (size_t i = 0; i < part_tables.size(); ++i) {
    //     if (part_tables[i].free_cells == 0) continue;
    //     for (int cell_id = part_tables[i].start; cell_id <= part_tables[i].end; ++cell_id) {
    //         cells[cell_id]->part_idx = i;
    //     }
    // }
    // 备份区初始化单元
    for (auto& part : get_parts(0,0)) {
        for (int cell_id = part.start; cell_id <= part.end; ++cell_id) {
            cells[cell_id]->part = &part;
        }
    }
    // 数据区初始化单元
    for (int tag = 1; tag <= tag_order.size(); ++tag) {
        for (int size = 1; size <= 5; ++size) {
            for (auto& part : get_parts(tag, size)) {
                for (int cell_id = part.start; cell_id <= part.end; ++cell_id) {
                    cells[cell_id]->part = &part;
                }
            }
        }
    }

    // 标签间接反向
    if(IS_INTERVAL_REVERSE) {
        // part_tables[0].start, part_tables[0].end = part_tables[0].end, part_tables[0].start;
        // 备份区反向
        auto& back_tables = get_parts(0, 0);
        back_tables[0].start, back_tables[0].end = back_tables[0].end, back_tables[0].start;

        // 数据区间歇反向
        for(int i = 1; i<=tag_order.size(); i+=2) {
            for(int j=1; j<=5; ++j) {
                for(auto& part : get_parts(tag_order[i], j)) {
                    part.start, part.end = part.end, part.start;
                }
            }
        }
        // 记录标签反向的对象
        for(int i = 1; i <= tag_order.size(); i+=2) {
            assert(tag_order[i-1] != 0);
            tag_reverse[tag_order[i-1]] = tag_order[i];
            tag_reverse[tag_order[i]] = tag_order[i-1];
        }
    }
    
    // debug格式化输出
    // if (DEBUG) {
    //     std::string output = "disk: " + std::to_string(id) + " part_tables: [";
    //     for (size_t i = 0; i < part_tables.size(); ++i) {
    //         if (i > 0) output += ", ";
    //     int tag_id = (i - 1) / 5;
    //     int part_type = (i - 1) % 5 + 1;
    //     output += "(" + std::to_string(i) + ", " + std::to_string(tag_id) + "." + std::to_string(part_type) + ", [";
    //     output += std::to_string(part_tables[i][0]) + ", " + std::to_string(part_tables[i][1]) + ", " + std::to_string(part_tables[i][2]);
    //         output += "])";
    //     }
    //     output += "]";
    //     debug(output);
    // }
}

// 添加Disk析构函数实现
Disk::~Disk() {
    if (cells) {
        for (int i = 0; i < MAX_DISK_SIZE; i++) {
            delete cells[i];
        }
        delete[] cells;
        cells = nullptr;
    }
}