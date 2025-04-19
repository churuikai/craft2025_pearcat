/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 *  ████████╗ ██████╗  ██████╗ ██╗     ███████╗   ██╗  ██╗
 *  ╚══██╔══╝██╔═══██╗██╔═══██╗██║     ██╔════╝   ██║  ██║
 *     ██║   ██║   ██║██║   ██║██║     ███████╗   ███████║
 *     ██║   ██║   ██║██║   ██║██║     ╚════██║   ██╔══██║
 *     ██║   ╚██████╔╝╚██████╔╝███████╗███████║   ██║  ██║
 *     ╚═╝    ╚═════╝  ╚═════╝ ╚══════╝╚══════╝   ╚═╝  ╚═╝
 * 
 * ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 * 【模块功能】
 * ┌─────────────────┬───────────────────────────────────────────────────────────┐
 * │ 数据结构优化     │ 提供高效的紧凑型数据结构实现                                 │
 * ├─────────────────┼───────────────────────────────────────────────────────────┤
 * │ 位操作工具       │ 实现基于位运算的快速数据处理                                 │
 * ├─────────────────┼───────────────────────────────────────────────────────────┤
 * │ 迭代器支持       │ 提供标准兼容的容器遍历接口                                  │
 * └─────────────────┴───────────────────────────────────────────────────────────┘
 * ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/

#pragma once
#include <cstdint>
#include <cstddef>
#include <iterator>

/*╔══════════════════════════════ Int3Set类定义 ═══════════════════════════════╗*/
/**
 * @brief     高效存储小整数的集合类
 * @details   针对1-5范围对象unit的优化实现:
 * ┌──────────────────────────────────────────────────────────────────────┐
 * │ ● 使用位图方式存储1-7范围的整数                                      │
 * │ ● 采用查找表加速位操作                                               │
 * │ ● 支持标准集合操作和迭代器接口                                       │
 * └──────────────────────────────────────────────────────────────────────┘
 */
class Int3Set {
private:
    uint8_t bits;  // 位图存储，每位表示一个数字是否存在
    
    /**
     * @brief     快速定位第一个置位的查找表
     * @details   优化位置查找性能:
     * ┌──────────────────────────────────────────────────────────────────────┐
     * │ ● 表大小：64，覆盖所有可能的6位组合                                     │
     * │ ● 返回值：6表示无置位，0-5表示最低位的1的位置                           │
     * └──────────────────────────────────────────────────────────────────────┘
     */
    static constexpr uint8_t first_bit_lookup[64] = {
        6, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0
    };
    
    /**
     * @brief     快速计算置位数量的查找表
     * @details   优化计数性能:
     * ┌──────────────────────────────────────────────────────────────────────┐
     * │ ● 表大小：64，覆盖所有可能的6位组合                                     │
     * │ ● 返回值：0-6，表示二进制中1的个数                                      │
     * └──────────────────────────────────────────────────────────────────────┘
     */
    static constexpr uint8_t bit_count_lookup[64] = {
        0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4,
        1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
        1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
        2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6
    };
    
public:
    /*╔════════════════════════════ 迭代器实现 ═════════════════════════════╗*/
    /**
     * @brief     集合的正向迭代器
     * @details   标准迭代器接口实现:
     * ┌──────────────────────────────────────────────────────────────────────┐
     * │ ● 提供前向迭代器功能                                                   │
     * │ ● 支持标准算法库操作                                                   │
     * │ ● 实现高效的位遍历                                                     │
     * └──────────────────────────────────────────────────────────────────────┘
     */
    class iterator {
    private:
        uint8_t remaining_bits;  // 剩余待处理的位
        uint8_t current_pos;     // 当前位置
        
    public:
        // 迭代器特性类型定义
        using difference_type = std::ptrdiff_t;
        using value_type = uint8_t;
        using pointer = const uint8_t*;
        using reference = const uint8_t&;
        using iterator_category = std::forward_iterator_tag;
        
        /**
         * @brief     构造位于集合开始的迭代器
         * @param     bits 位图数据
         * @details   初始化步骤:
         * ┌──────────────────────────────────────────────────────────────────────┐
         * │ ● 保存位图数据到remaining_bits                                        │
         * │ ● 定位第一个有效位置                                                  │
         * │ ● 更新剩余位图数据                                                    │
         * └──────────────────────────────────────────────────────────────────────┘
         */
        explicit iterator(uint8_t bits) : remaining_bits(bits), current_pos(0) {
            if (remaining_bits) {
                current_pos = first_bit_lookup[remaining_bits & 0x3F];
                if (current_pos < 6) {
                    remaining_bits &= ~(1U << current_pos);
                }
            } else {
                current_pos = 6; // 结束位置
            }
        }
        
        /**
         * @brief 构造结束迭代器
         */
        iterator() : remaining_bits(0), current_pos(6) {}
        
        /**
         * @brief 解引用操作符
         * @return 当前元素值
         */
        value_type operator*() const { return current_pos; }
        
        /**
         * @brief 前置递增操作符
         * @return 递增后的迭代器引用
         */
        iterator& operator++() {
            if (remaining_bits) {
                current_pos = first_bit_lookup[remaining_bits & 0x3F];
                if (current_pos < 6) {
                    remaining_bits &= ~(1U << current_pos);
                }
            } else {
                current_pos = 6; // 结束位置
            }
            return *this;
        }
        
        /**
         * @brief 后置递增操作符
         * @return 递增前的迭代器副本
         */
        iterator operator++(int) {
            iterator tmp = *this;
            ++(*this);
            return tmp;
        }
        
        /**
         * @brief 相等比较操作符
         * @param other 要比较的迭代器
         * @return 是否相等
         */
        bool operator==(const iterator& other) const {
            return current_pos == other.current_pos;
        }
        
        /**
         * @brief 不等比较操作符
         * @param other 要比较的迭代器
         * @return 是否不等
         */
        bool operator!=(const iterator& other) const {
            return current_pos != other.current_pos;
        }
    };
    
    /*╔════════════════════════════ 集合操作方法 ════════════════════════════╗*/
    /**
     * @brief     集合操作接口实现
     * @details   支持的操作:
     * ┌──────────────────────────────────────────────────────────────────────┐
     * │ ● 基本操作：插入、删除、查找                                           │
     * │ ● 容器操作：清空、大小查询                                             │
     * │ ● 集合操作：并集、交集                                                 │
     * └──────────────────────────────────────────────────────────────────────┘
     */
    
    /**
     * @brief     默认构造函数
     * @details   初始化空集合
     */
    Int3Set() : bits(0) {}
    
    /**
     * @brief     清空集合
     * @details   重置所有位为0
     */
    void clear() {
        bits = 0;
    }
    
    /**
     * @brief     获取集合大小
     * @return    元素数量
     * @details   使用查找表快速计算置位数
     */
    size_t size() const {
        return bit_count_lookup[bits & 0x3F];
    }
    
    /**
     * @brief     检查集合是否为空
     * @return    true表示空集合，false表示非空
     */
    bool empty() const {
        return bits == 0;
    }
    
    /**
     * @brief     插入元素
     * @param     value 要插入的值[0-5]
     * @return    true表示插入成功，false表示元素已存在
     */
    bool insert(uint8_t value) {
        uint8_t mask = 1U << value;
        if (bits & mask) return false;
        
        bits |= mask;
        return true;
    }
    
    /**
     * @brief     移除元素
     * @param     value 要移除的值[0-5]
     * @return    true表示移除成功，false表示元素不存在
     */
    bool erase(uint8_t value) {
        uint8_t mask = 1U << value;
        if (!(bits & mask)) return false;
        
        bits &= ~mask;
        return true;
    }
    
    /**
     * @brief     检查是否包含元素
     * @param     value 要检查的值[0-5]
     * @return    true表示存在，false表示不存在
     */
    bool contains(uint8_t value) const {
        return (bits & (1U << value)) != 0;
    }
    
    /**
     * @brief     获取起始迭代器
     * @return    指向集合第一个元素的迭代器
     */
    iterator begin() const {
        return iterator(bits);
    }
    
    /**
     * @brief     获取结束迭代器
     * @return    指向集合末尾的迭代器
     */
    iterator end() const {
        return iterator();
    }
    
    /**
     * @brief     执行集合并集操作
     * @param     other 另一个集合
     * @details   使用位或运算实现并集
     */
    void unite(const Int3Set& other) {
        bits |= other.bits;
    }
    
    /**
     * @brief     执行集合交集操作
     * @param     other 另一个集合
     * @details   使用位与运算实现交集
     */
    void intersect(const Int3Set& other) {
        bits &= other.bits;
    }
};
/*╚═════════════════════════════════════════════════════════════════════════╝*/