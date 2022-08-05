#ifndef COMPARATOR_H

#include <string>

template <typename T>
class Comparator
{
public:
    int operator()(const T& a, const T& b) {
        if(a < b) return 1;
        else if(a > b) return -1;
        else return 0;
    }
};

#endif