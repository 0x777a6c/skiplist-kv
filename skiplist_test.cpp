#include "skiplist.h"
#include "comparator.h"

#include <iostream>
#include <unordered_map>
using namespace std;

// const std::string INSERT = "insert";
// const std::string DELETE = "delete";
// const std::string UPDATE = "update";
// const std::string GET = "get";
// const std::string DUMP = "dump";
// const std::string LOAD = "load";

enum {
    INSERT,
    DELETE,
    UPDATE,
    GET,
    DUMP,
    LOAD,
    QUIT
};

unordered_map<string, int> ops_table = {
    {"insert", INSERT},
    {"delete", DELETE},
    {"update", UPDATE},
    {"get", GET},
    {"dump", DUMP},
    {"load", LOAD},
    {"quit", QUIT}
};


int main() {
    Comparator<int> cmp;
    Skiplist<int, string, Comparator<int>> sl(cmp);

    string op;
    while (true) {
        cin >> op;

        int key;
        string value;
        switch (ops_table[op])
        {
        case INSERT:
            cin >> key >> value;
            sl.insert(key, value);
            break;

        case DELETE:
            cin >> key;
            sl.del(key);
            break;

        case UPDATE:
            cin >> key >> value;
            sl.update(key, value);
            break;

        case GET:
            cin >> key;
            if(sl.get(key, value) == OK) cout << key << ":" << value << endl;
            break;  

        case DUMP:
            sl.dump();
            break;

        case LOAD:
            sl.load();
            break;  

        case QUIT:
            return 0;

        default:
            break;
        }
    }
    
    return 0;
}