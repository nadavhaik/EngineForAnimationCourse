//
// Created by nadavh on 2/23/23.
//

#ifndef ENGINEREWORK_FUNCTIONALS_H
#define ENGINEREWORK_FUNCTIONALS_H

#include <vector>
#include <functional>


namespace functionals {
    template<typename T1, typename T2>
    std::vector<T2> map(std::vector<T1> list, const std::function<T2(T1)> &mapper) {
        std::vector<T2> mapped;
        std::transform(list.begin(), list.end(), std::back_inserter(mapped), mapper);
        return mapped;
    }

    template<typename T>
    std::vector<T> filter(std::vector<T> list, const std::function<bool(T)> &pred) {
        std::vector<T> filtered;
        std::copy_if(list.begin(), list.end(), std::back_inserter(filtered), pred);
        return filtered;
    }
}

#endif //ENGINEREWORK_FUNCTIONALS_H
