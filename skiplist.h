#ifndef SKIPLIST_H
#define SKIPLIST_H

#include <cassert>
#include <random>
#include <iostream>
#include <string>
#include <fstream>
#include <vector>


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

    void insert(const Key& key, const Value& value) {
        int height = get_random_height();
        Node* prev[max_height] = {nullptr};

        Node* x = find_greater_or_equal(key, prev);
        if(x != nullptr && Equal(x->key, key)) {
            std::cerr << "The key existed, please use update." << std::endl;
            return;
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
    }

    void del(const Key& key) {
        Node* prev[max_height] = {nullptr};
        Node* x = find_greater_or_equal(key, prev);
        if(x == nullptr || !Equal(x->key, key)) {
            std::cerr << "The key doesn't exist, delete failed." << std::endl;
            return;
        }

        for(int i = 0; i < max_height; ++i) {
            if(prev[i] != nullptr) {
                prev[i]->set_next(i, x->get_next(i));
            }
        }
        delete x;
    }

    void update(const Key& key, const Value& value) {
        Node* x = find_greater_or_equal(key, nullptr);
        if(x == nullptr || !Equal(x->key, key)) {
            std::cerr << "The key doesn't exist, please use insert." << std::endl;
            return;
        }

        x->set_value(value);
    }

    const Value get(const Key& key) {
        Node* x = find_greater_or_equal(key, nullptr);
        if(x == nullptr || !Equal(x->key, key)) {
            std::cerr << "The key doesn't exist, get failed." << std::endl;
            return Value();
        }

        return x->get_value();
    }

    void dump() {
        file_writer.open(dump_file_path, std::ios::app);
        if(!file_writer.is_open()) {
            std::cerr << "cannot open the file." << std::endl;
            return;
        }

        Node* cur = head->get_next(0);
        while(cur != nullptr){
            file_writer << cur->key << dump_delimiter << cur->get_value() << "\n";
            cur = cur->get_next(1);
        }

        file_writer.close();
    }

    void load() {
        file_reader.open(dump_file_path, std::ios::in);
        if(!file_reader.is_open()) {
            std::cerr << "cannot open the file." << std::endl;
            return;
        }

        std::string line, key, value;
        while(std::getline(file_reader, line)) {
            get_key_and_value_from_line(line, key, value);
            insert(key, value);
        }
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

    bool is_valid_line(const std::string& line) {
        if(line.empty()) return false;
        if(line.find(dump_delimiter) == std::string::npos) return false;
        return true;
    }

    void get_key_and_value_from_line (const std::string& line, std::string& key, std::string& value) {
        if(!is_valid_line(line)) {
            key = std::string();
            value = std::string();
            return;
        }

        key = line.substr(0, line.find(dump_delimiter));
        value = line.substr(line.find(dump_delimiter)+1);
    }
};

#endif
