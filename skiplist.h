// - 实现基本的插入、删除、查找功能；可以dump和load，支持服务快速重启
// - 实现一个基于skiplist的简易版内存kv服务，从磁盘中逐行读取数据，
//   对skiplist内进行增删改；同时接收请求进行多线程并发读操作，假定所
//   有读操作执行时间绝不会超过2s(加分项：该场景可以不使用任何锁实现)
//   需保证写时读的一致性，即在进行写时，读线程不能访问写的那块内存


#ifndef SKIPLIST_H
#define SKIPLIST_H

#include <cassert>
#include <random>
#include <iostream>
#include <string>
#include <fstream>
#include <vector>

enum Status{
    FAIL = -1,
    OK = 1
};

// requirements：
// 1. Key 对 operator<, operator==, operator> 有相应的重载
// 分别意为严格小于, 相等, 严格大于
// 2. 对于 Key/Value(以下使用T代指 Key/Value), 分别存在以下的两个的友元函数：
// ifstream& operator>>(ifstream& fin, T& t);
// ofstream& operator<<(ofstream& fout, T& t);
// 分别意为从 fin 写入与向 fout 写出
template <typename Key, typename Value, class Comparator>
class Skiplist
{
private:
    struct Node{
        const Key key;

        Node(const Key& _key, const Value& _value, const int& _height) : key(_key), value(_value), height(_height) {}

        void set_next(int n, Node* x) {
            assert(n >= 0);
            next[n] = x;
        }

        Node* get_next(int n) {
            assert(n >= 0);
            return next[n];
        }

        void set_value(const Value& _value) {
            value = _value;
        }

        const Value get_value() {
            return value;
        }

    private:
        Value value;
        Node* next[1];
        int height;
    };

public:
    Skiplist(const Comparator& _cmp) : cmp(_cmp){
        head = new_node(Key(), Value(), max_height);
        for(int i = 0; i < max_height; ++i) {
            head->set_next(i, nullptr);
        }
    }
    Skiplist(const Comparator& _cmp, const std::string& dump_path) : cmp(_cmp), dump_file_path(dump_path) {
        head = new_node(Key(), Value(), max_height);
        for(int i = 0; i < max_height; ++i) {
            head->set_next(i, nullptr);
        }
    }
    ~Skiplist() {
        Node* cur = head->get_next(1);
        while(cur) {
            Node* prev = cur;
            cur = cur->get_next(1);
            delete prev;
        }
        delete head;
    }

    Status insert(const Key& key, const Value& value) {
        int height = get_random_height();
        Node* prev[max_height] = {nullptr};

        Node* x = find_greater_or_equal(key, prev);
        if(x != nullptr && Equal(x->key, key)) {
            std::cerr << "The key existed, please use update." << std::endl;
            return FAIL;
        }

        if(height > cur_height){
            for(int i = cur_height; i < height; ++i) {
                prev[i] = head;
            }
            cur_height = height;
        }

        Node* insert_node = new_node(key, value, height);
        for(int i = 0; i < height; ++i) {
            insert_node->set_next(i, prev[i]->get_next(i));
            prev[i]->set_next(i, insert_node);
        }

        return OK;
    }

    Status del(const Key& key) {
        Node* prev[max_height] = {nullptr};
        Node* x = find_greater_or_equal(key, prev);
        if(x == nullptr || !Equal(x->key, key)) {
            std::cerr << "The key doesn't exist, delete failed." << std::endl;
            return FAIL;
        }

        for(int i = 0; i < max_height; ++i) {
            if(prev[i] != nullptr) {
                prev[i]->set_next(i, x->get_next(i));
            }
        }
        delete x;
        return OK;
    }

    Status update(const Key& key, const Value& value) {
        Node* x = find_greater_or_equal(key, nullptr);
        if(x == nullptr || !Equal(x->key, key)) {
            std::cerr << "The key doesn't exist, please use insert." << std::endl;
            return FAIL;
        }

        x->set_value(value);
        return OK;
    }

    Status get(const Key& key, Value& value) {
        Node* x = find_greater_or_equal(key, nullptr);
        if(x == nullptr || !Equal(x->key, key)) {
            std::cerr << "The key doesn't exist, get failed." << std::endl;
            return FAIL;
        }

        value = x->get_value();
        return OK;
    }

    Status dump() {
        file_writer.open(dump_file_path, std::ios::out);
        if(!file_writer.is_open()) {
            std::cerr << "cannot open the file." << std::endl;
            return FAIL;
        }

        Node* cur = head->get_next(0);
        while(cur != nullptr){
            file_writer << cur->key << " " << dump_delimiter << " " << cur->get_value() << "\n";
            cur = cur->get_next(0);
        }

        file_writer.close();

        return OK;
    }

    Status load() {
        file_reader.open(dump_file_path, std::ios::in);
        if(!file_reader.is_open()) {
            std::cerr << "cannot open the file." << std::endl;
            return FAIL;
        }

        Key key;
        Value value;
        std::string placeholder;
        while(file_reader >> key >> placeholder >> value) {
            if(insert(key, value) == FAIL) update(key, value);
        }

        file_reader.close();

        return OK;
    }

private:
    enum {max_height = 12};
    int cur_height = 1;
    Node* head;
    Comparator cmp;
    std::string dump_file_path = "./store/dump_file";
    std::string dump_delimiter = ":";
    std::ifstream file_reader;
    std::ofstream file_writer;

    Node* new_node(const Key& key, const Value& value, int height) {
        void* mem = operator new(sizeof(Node) + sizeof(Node*) * (height - 1));
        Node* p = new (mem) Node(key, value, height);
        return p;
    }

    //  使用cmp比较key与node->key，cmp(node->key, key)返回1则key应该插入node之后
    bool key_is_after_node(const Key& key, Node* node) {
        return (node != nullptr) && (cmp(node->key, key) > 0);
    }

    Node* find_greater_or_equal(const Key& key, Node** prev) {
        int level = cur_height - 1;
        Node* cur = head;
        Node* next = cur->get_next(level);

        while(true) {
            if(key_is_after_node(key, next)) {
                cur = next;
            }else {
                if(prev != nullptr) prev[level] = cur;
                if(level == 0) break;
                level--;
            }
            next = cur->get_next(level);
        }

        return next;
    }

    int get_random_height() {
        std::default_random_engine engine(time(0));
        std::bernoulli_distribution distribution(0.25);

        int height = 1;
        while(height <= max_height && distribution(engine)) {
            height++;
        }

        return height;
    }

    bool Equal(const Key& key_1, const Key& key_2) {
        if(cmp(key_1, key_2) == 0) return true;
        return false;
    }
};

#endif
