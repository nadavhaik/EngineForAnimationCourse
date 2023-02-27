//
// Created by nadavh on 2/26/23.
//
//#pragma once
#include <Eigen/Core>
#include "types_macros.h"

#ifndef ENGINEREWORK_ALGEBRIANFUNCTIONS_H
#define ENGINEREWORK_ALGEBRIANFUNCTIONS_H

namespace algebra
{
    static float abs(const Vec3 &vec) {
        return std::sqrt(vec.x() * vec.x() + vec.y() * vec.y() + vec.z() * vec.z());
    }
    static float distance(const Vec3 &v1, const Vec3 &v2) {
        return abs(v2 - v1);
    }
}


#endif //ENGINEREWORK_ALGEBRIANFUNCTIONS_H
