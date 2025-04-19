/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 *  ██╗███╗   ██╗██╗████████╗
 *  ██║████╗  ██║██║╚══██╔══╝
 *  ██║██╔██╗ ██║██║   ██║   
 *  ██║██║╚██╗██║██║   ██║   
 *  ██║██║ ╚████║██║   ██║   
 *  ╚═╝╚═╝  ╚═══╝╚═╝   ╚═╝   
 * 
 * ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 * 【模块功能】
 * ┌─────────────────┬───────────────────────────────────────────────────────────┐
 * │ 系统初始化       │ 实现系统启动时的初始化配置                                  │
 * ├─────────────────┼───────────────────────────────────────────────────────────┤
 * │ 磁盘分区         │ 管理磁盘空间分配和分区布局                                  │
 * ├─────────────────┼───────────────────────────────────────────────────────────┤
 * │ 资源配置         │ 初始化系统资源和运行参数                                    │
 * └─────────────────┴───────────────────────────────────────────────────────────┘
 * ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/

#include "constants.h"
#include "ctrl_disk_obj_req.h"
#include <limits>
#include "debug.h"
#include "data_analysis.h"
#include <random>

/*╔══════════════════════════════ 系统初始化函数 ═══════════════════════════════╗*/
/**
 * @brief     初始化控制器下的所有磁盘
 * @details   执行以下初始化步骤:
 * ┌──────────────────────────────────────────────────────────────────────┐
 * │ 1. 输出实际磁盘标签顺序                                               │
 * │ 2. 输出各标签占用空间比例                                             │
 * │ 3. 初始化每个磁盘的基本参数                                           │
 * │ 4. 输出每个磁盘的分区表信息                                           │
 * └──────────────────────────────────────────────────────────────────────┘
 */
void Controller::disk_init() 
{
    // ◆ 输出实际磁盘标签顺序
    info("实际磁盘标签顺序==============================================");
    for(int disk_id=0; disk_id<N; ++disk_id)
    {
        info(TAG_ORDERS[disk_id]);
    }

    // ◆ 输出标签占用空间比例
    info("标签占用空间比例==============================================");
    std::string tag_size_rate_info = "";
    for(int i=1; i<M+1; ++i) 
    {
        tag_size_rate_info += "tag"+std::to_string(i)+": "+std::to_string(TAG_SIZE_RATE[i])+"; ";
    }
    info(tag_size_rate_info);

    // ◆ 初始化各个磁盘
    for (int i = 1; i <= N; ++i) 
    {
        DISKS[i].id = i;
        DISKS[i].controller = this;
        DISKS[i].init(V, TAG_ORDERS[i-1], TAG_SIZE_RATE, TAG_SIZE_DB);
    }

    // ◆ 输出每个磁盘的分区表信息
    info("磁盘分区表信息==============================================");
    for (int i = 1; i <= N; ++i) 
    {
        std::string disk_info = "disk " + std::to_string(i) + ": ";
        
        // ● 输出备份区信息
        for (auto& part : DISKS[i].get_parts(0)) 
        {
            disk_info += "back-part (" + std::to_string(part.start) + "-" + std::to_string(part.end) + 
                        ", " + std::to_string(part.free_cells) + "); ";
        }
        
        // ● 输出数据区信息
        for (int tag_idx = 0; tag_idx < TAG_ORDERS[i-1].size(); ++tag_idx) 
        {
            int tag_id = TAG_ORDERS[i-1][tag_idx];
            for (auto &part : DISKS[i].get_parts(tag_id))
            {
                disk_info += "tag-" + std::to_string(tag_id) + "(" +
                            std::to_string(part.start) + "-" + std::to_string(part.end) +
                            ", " + std::to_string(part.free_cells) + "); ";
            }
        }
        info(disk_info);
    }
} 
/*╚═════════════════════════════════════════════════════════════════════════╝*/

/*╔══════════════════════════════ 磁盘初始化函数 ═══════════════════════════════╗*/
/**
 * @brief     初始化单个磁盘的分区和资源
 * @param     size 磁盘大小
 * @param     tag_order 标签顺序
 * @param     tag_size_rate 标签大小比例
 * @param     tag_size_db 标签大小数据库
 * @details   执行以下初始化步骤:
 * ┌──────────────────────────────────────────────────────────────────────┐
 * │ 1. 初始化基本参数和随机数生成器                                        │
 * │ 2. 分配磁盘空间和分区表                                               │
 * │ 3. 初始化备份区(tag 0)                                               │
 * │ 4. 初始化数据区(tag 1-M)                                             │
 * │ 5. 初始化冗余区(tag 17)                                              │
 * │ 6. 配置分区反向和空闲块链表                                           │
 * └──────────────────────────────────────────────────────────────────────┘
 */
void Disk::init(int size, const std::vector<int>& tag_order, 
                const std::vector<double>& tag_size_rate, 
                const std::vector<std::vector<double>>& tag_size_db) 
{
    // ◆ 初始化随机数生成器
    double range = 0.00001;
    std::random_device rd;
    std::mt19937 gen(66);
    std::uniform_real_distribution<> dis(1-range, 1+range);

    this->size = size;
    
    // ◆ 分配资源
    cells.resize(size+1);
    part_tables.resize(M + 2);
 
    // ◆ 计算分区大小
    // ● 备份区(tag 0): 占比 90%*back/3*size
    // ● 数据区(tag 1-M): 占比 size-90%*back/3*size
    // ● 冗余区(tag 17): 数据区空间不够时动态扩充
    int back_size = static_cast<int>(0.305 * 2 * size) + 1;
    int data_size = size - back_size;

    // ◆ 初始化备份区
    get_parts(0).push_back(Part(data_size + 1, size, back_size, data_size+1, 0, 0));

    // ◆ 初始化数据1区
    data_size1 = data_size/2;
    int pointer_temp = 1;
    for (int tag_id : tag_order) 
    {
        // ● 计算分区大小(带随机扰动)
        double random_rate = dis(gen);
        int tag_id_end;
        if((tag_id == tag_order[0] or tag_id == tag_order[tag_order.size()-1]) and IS_EXTEND) 
        {
            tag_id_end = pointer_temp + static_cast<int>(random_rate* DATA_COMPRESSION*data_size1 * tag_size_rate[tag_id]*1.6) - 1;
        }
        else
        {
            tag_id_end = pointer_temp + static_cast<int>(random_rate*DATA_COMPRESSION*data_size1 * tag_size_rate[tag_id]) - 1;
        }

        // ● 创建分区
        get_parts(tag_id).push_back(Part(pointer_temp, tag_id_end, tag_id_end - pointer_temp + 1, pointer_temp, tag_id, 1));
        pointer_temp = tag_id_end + 1;
    }

    // ◆ 初始化数据1区冗余区
    assert(data_size1 - pointer_temp + 1 >= 0);
    auto& dynamic_tables1 = get_parts(17);
    dynamic_tables1.push_back(Part(pointer_temp, data_size1, data_size1 - pointer_temp + 1, pointer_temp, 17, 1));
    pointer_temp = dynamic_tables1.back().end + 1;

    // ◆ 初始化数据2区
    data_size2 = data_size - data_size1;
    pointer_temp = 1+data_size1;
    for (int tag_id : tag_order) 
    {
        // ● 计算分区大小(带随机扰动)
        double random_rate = dis(gen);
        int tag_id_end;
        if((tag_id == tag_order[0] or tag_id == tag_order[tag_order.size()-1]) and IS_EXTEND) 
        {
            tag_id_end = pointer_temp + static_cast<int>(random_rate*DATA_COMPRESSION*data_size2 * tag_size_rate[tag_id]*1.6) - 1;
        }
        else
        {
            tag_id_end = pointer_temp + static_cast<int>(random_rate*DATA_COMPRESSION*data_size2 * tag_size_rate[tag_id]) - 1;
        }

        // ● 创建分区
        get_parts(tag_id).push_back(Part(pointer_temp, tag_id_end, tag_id_end - pointer_temp + 1, pointer_temp, tag_id, 1));
        pointer_temp = tag_id_end + 1;
    }

    // ◆ 初始化数据2区冗余区
    assert(data_size - pointer_temp + 1 >= 0);
    auto& dynamic_tables2 = get_parts(17);
    dynamic_tables2.push_back(Part(pointer_temp, data_size, data_size - pointer_temp + 1, pointer_temp, 17, 1));
    pointer_temp = dynamic_tables2.back().end + 1;

    // ◆ 初始化各分区单元格
    // ● 初始化备份区单元格
    for (auto& part : get_parts(0)) 
    {
        for (int cell_id = part.start; cell_id <= part.end; ++cell_id) 
        {
            cells[cell_id].part = &part;
        }
    }
    
    // ● 初始化数据区单元格
    for (int tag: tag_order) 
    {
        for (auto& part : get_parts(tag)) 
        {
            for (int cell_id = part.start; cell_id <= part.end; ++cell_id) 
            {
                cells[cell_id].part = &part;
            }
        }
    }
    
    // ● 初始化冗余区单元格
    for (auto& part : get_parts(17)) 
    {
        for (int cell_id = part.start; cell_id <= part.end; ++cell_id) 
        {
            cells[cell_id].part = &part;
        }
    }

    // ◆ 配置分区反向
    if(IS_INTERVAL_REVERSE) 
    {
        // ● 备份区反向
        auto& back_tables = get_parts(0);
        std::swap(back_tables[0].start, back_tables[0].end);

        // ● 数据区间歇反向
        for (int i = 0; i < tag_order.size(); i += 2)
        {
            for (auto &part : get_parts(tag_order[i]))
            {
                std::swap(part.start, part.end);
            }
        }
        
        // ● 记录标签反向关系
        if(IS_EXTEND)
        {
            tag_reverse[tag_order[0]] = tag_order[tag_order.size()-1];
            tag_reverse[tag_order[tag_order.size()-1]] = tag_order[0];
            for(int i = 2; i < tag_order.size(); i+=2) 
            {
                assert(tag_order[i-1] != 0);
                tag_reverse[tag_order[i-1]] = tag_order[i];
                tag_reverse[tag_order[i]] = tag_order[i-1];
            }
        }
        else
        {
            for(int i = 1; i < tag_order.size(); i+=2) 
            {
                assert(tag_order[i-1] != 0);
                tag_reverse[tag_order[i-1]] = tag_order[i];
                tag_reverse[tag_order[i]] = tag_order[i-1];
            }
        }
        
        // ● 冗余区反向
        for(auto& part : get_parts(17)) 
        {
            std::swap(part.start, part.end);
        }
    }
    
    // ◆ 初始化空闲块链表
    // ● 初始化数据区空闲块链表
    for (int tag : tag_order) 
    {
        for (auto& part : get_parts(tag)) 
        {
            if (part.free_cells > 0) 
            {
                part.init_free_list();
            }
        }
    }
    
    // ● 初始化冗余区空闲块链表
    for (auto& part : get_parts(17)) 
    {
        if (part.free_cells > 0) 
        {
            part.init_free_list();
        }
    }
}
/*╚═════════════════════════════════════════════════════════════════════════╝*/

/*╔══════════════════════════════ 资源清理函数 ═══════════════════════════════╗*/
/**
 * @brief     磁盘析构函数，清理所有分区的空闲块链表
 */
Disk::~Disk() 
{
    for (auto& tag_parts : part_tables) 
    {
        for (auto& part : tag_parts) 
        {
            part._clear_free_list();
        }
    }
}
/*╚═════════════════════════════════════════════════════════════════════════╝*/