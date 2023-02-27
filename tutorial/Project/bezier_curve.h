//
// Created by nadavh on 2/27/23.
//
#include <functional>
#include <Eigen/Core>
#include <memory>
#include "types_macros.h"

#ifndef ENGINEREWORK_BEZIER_CURBE_H
#define ENGINEREWORK_BEZIER_CURBE_H

static std::function<Vec3(float)> CubicBezier(const Vec3& p0, const Vec3& p1, const Vec3& p2) {
    return [p0, p1, p2](float t) {
        return (1.0f-t) * (1.0f-t) * p0 + 2.0f * (1.0f - t) * t * p1 + t * t * p2;
    };
}

#endif //ENGINEREWORK_BEZIER_CURBE_H
