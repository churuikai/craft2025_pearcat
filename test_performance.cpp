// #include <iostream>
// #include <chrono>
// #include <vector>
// #include <unordered_set>
// #include <memory>

// // 简化版Cell结构，模拟原始代码中的Cell
// struct Cell {
//     std::unordered_set<int> req_ids;
//     int obj_id;
//     int unit_id;
//     int tag;
//     int part_idx;
    
//     Cell() : obj_id(0), unit_id(0), tag(0), part_idx(0) {}
    
//     void free() {
//         req_ids.clear();
//         obj_id = 0;
//         unit_id = 0;
//         tag = 0;
//     }
    
//     int process() {
//         // 模拟一些处理逻辑
//         int sum = obj_id + unit_id + tag + part_idx;
//         for (auto id : req_ids) {
//             sum += id;
//         }
//         return sum;
//     }
// };

// // 测试对象数组性能
// void test_object_array(int size, int iterations) {
//     std::cout << "测试对象数组性能..." << std::endl;
    
//     // 创建对象数组
//     auto start_creation = std::chrono::high_resolution_clock::now();
//     Cell* objects = new Cell[size];
//     auto end_creation = std::chrono::high_resolution_clock::now();
    
//     // 初始化数组
//     auto start_init = std::chrono::high_resolution_clock::now();
//     for (int i = 0; i < size; i++) {
//         objects[i].obj_id = i % 100;
//         objects[i].unit_id = i % 50;
//         objects[i].tag = i % 10;
//         objects[i].part_idx = i % 5;
//         objects[i].req_ids.insert(i);
//         objects[i].req_ids.insert(i+1);
//     }
//     auto end_init = std::chrono::high_resolution_clock::now();
    
//     // 遍历并访问
//     auto start_traverse = std::chrono::high_resolution_clock::now();
//     int sum = 0;
//     for (int iter = 0; iter < iterations; iter++) {
//         for (int i = 0; i < size; i++) {
//             sum += objects[i].process();
//         }
//     }
//     auto end_traverse = std::chrono::high_resolution_clock::now();
    
//     // 销毁对象
//     auto start_destroy = std::chrono::high_resolution_clock::now();
//     delete[] objects;
//     auto end_destroy = std::chrono::high_resolution_clock::now();
    
//     // 计算耗时
//     auto creation_time = std::chrono::duration_cast<std::chrono::microseconds>(end_creation - start_creation).count();
//     auto init_time = std::chrono::duration_cast<std::chrono::microseconds>(end_init - start_init).count();
//     auto traverse_time = std::chrono::duration_cast<std::chrono::microseconds>(end_traverse - start_traverse).count();
//     auto destroy_time = std::chrono::duration_cast<std::chrono::microseconds>(end_destroy - start_destroy).count();
    
//     std::cout << "对象数组结果: " << sum << std::endl;
//     std::cout << "创建时间: " << creation_time << " 微秒" << std::endl;
//     std::cout << "初始化时间: " << init_time << " 微秒" << std::endl;
//     std::cout << "遍历时间: " << traverse_time << " 微秒" << std::endl;
//     std::cout << "销毁时间: " << destroy_time << " 微秒" << std::endl;
//     std::cout << "总时间: " << creation_time + init_time + traverse_time + destroy_time << " 微秒" << std::endl;
//     std::cout << std::endl;
// }

// // 测试指针数组性能
// void test_pointer_array(int size, int iterations) {
//     std::cout << "测试指针数组性能..." << std::endl;
    
//     // 创建指针数组
//     auto start_creation = std::chrono::high_resolution_clock::now();
//     Cell** pointers = new Cell*[size];
//     for (int i = 0; i < size; i++) {
//         pointers[i] = new Cell();
//     }
//     auto end_creation = std::chrono::high_resolution_clock::now();
    
//     // 初始化数组
//     auto start_init = std::chrono::high_resolution_clock::now();
//     for (int i = 0; i < size; i++) {
//         pointers[i]->obj_id = i % 100;
//         pointers[i]->unit_id = i % 50;
//         pointers[i]->tag = i % 10;
//         pointers[i]->part_idx = i % 5;
//         pointers[i]->req_ids.insert(i);
//         pointers[i]->req_ids.insert(i+1);
//     }
//     auto end_init = std::chrono::high_resolution_clock::now();
    
//     // 遍历并访问
//     auto start_traverse = std::chrono::high_resolution_clock::now();
//     int sum = 0;
//     for (int iter = 0; iter < iterations; iter++) {
//         for (int i = 0; i < size; i++) {
//             sum += pointers[i]->process();
//         }
//     }
//     auto end_traverse = std::chrono::high_resolution_clock::now();
    
//     // 销毁对象
//     auto start_destroy = std::chrono::high_resolution_clock::now();
//     for (int i = 0; i < size; i++) {
//         delete pointers[i];
//     }
//     delete[] pointers;
//     auto end_destroy = std::chrono::high_resolution_clock::now();
    
//     // 计算耗时
//     auto creation_time = std::chrono::duration_cast<std::chrono::microseconds>(end_creation - start_creation).count();
//     auto init_time = std::chrono::duration_cast<std::chrono::microseconds>(end_init - start_init).count();
//     auto traverse_time = std::chrono::duration_cast<std::chrono::microseconds>(end_traverse - start_traverse).count();
//     auto destroy_time = std::chrono::duration_cast<std::chrono::microseconds>(end_destroy - start_destroy).count();
    
//     std::cout << "指针数组结果: " << sum << std::endl;
//     std::cout << "创建时间: " << creation_time << " 微秒" << std::endl;
//     std::cout << "初始化时间: " << init_time << " 微秒" << std::endl;
//     std::cout << "遍历时间: " << traverse_time << " 微秒" << std::endl;
//     std::cout << "销毁时间: " << destroy_time << " 微秒" << std::endl;
//     std::cout << "总时间: " << creation_time + init_time + traverse_time + destroy_time << " 微秒" << std::endl;
//     std::cout << std::endl;
// }

// int main() {
//     // 数组大小和迭代次数
//     const int SIZE = 5000;
//     const int ITERATIONS = 1000;
    
//     std::cout << "开始性能测试，数组大小: " << SIZE << "，迭代次数: " << ITERATIONS << std::endl << std::endl;
    
//     // 测试对象数组
//     test_object_array(SIZE, ITERATIONS);
    
//     // 测试指针数组
//     test_pointer_array(SIZE, ITERATIONS);
    
//     return 0;
// } 