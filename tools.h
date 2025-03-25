#include <cstdint>
#include <iterator>

// 只能存储1-7的int的集合
class Int3Set {
private:
    uint8_t bits;
    static constexpr uint8_t first_bit_lookup[64] = {
        6, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0
    };
    static constexpr uint8_t bit_count_lookup[64] = {
        0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4,
        1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
        1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
        2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6
    };
    
public:
    // 正向迭代器类
    class iterator {
    private:
        uint8_t remaining_bits;
        uint8_t current_pos;
        
    public:
        using difference_type = std::ptrdiff_t;
        using value_type = uint8_t;
        using pointer = const uint8_t*;
        using reference = const uint8_t&;
        using iterator_category = std::forward_iterator_tag;
        
        // 构造一个位于当前集合开始的迭代器
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
        
        // 构造结束迭代器
        iterator() : remaining_bits(0), current_pos(6) {}
        
        value_type operator*() const { return current_pos; }
        
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
        
        iterator operator++(int) {
            iterator tmp = *this;
            ++(*this);
            return tmp;
        }
        
        bool operator==(const iterator& other) const {
            return current_pos == other.current_pos;
        }
        
        bool operator!=(const iterator& other) const {
            return current_pos != other.current_pos;
        }
    };
    
    // 构造函数
    Int3Set() : bits(0) {}
    
    // 清空集合
    void clear() {
        bits = 0;
    }
    
    // 集合大小
    size_t size() const {
        return bit_count_lookup[bits & 0x3F];
    }
    
    // 是否为空
    bool empty() const {
        return bits == 0;
    }
    
    // 插入元素
    bool insert(uint8_t value) {
        uint8_t mask = 1U << value;
        if (bits & mask) return false;
        
        bits |= mask;
        return true;
    }
    
    // 移除元素
    bool erase(uint8_t value) {
        uint8_t mask = 1U << value;
        if (!(bits & mask)) return false;
        
        bits &= ~mask;
        return true;
    }
    
    // 查找元素
    bool contains(uint8_t value) const {
        return (bits & (1U << value)) != 0;
    }
    
    // 迭代器支持
    iterator begin() const {
        return iterator(bits);
    }
    
    iterator end() const {
        return iterator();
    }
    
    // 集合的并集操作
    void unite(const Int3Set& other) {
        bits |= other.bits;
    }
    
    // 集合的交集操作
    void intersect(const Int3Set& other) {
        bits &= other.bits;
    }
};