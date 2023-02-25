//
// Created by nadavh on 2/24/23.
//

#include "Skeleton.h"

Skeleton::Skeleton(const std::vector<Joint> &joints): joints(joints) {
    dmat.resize(joints.size(), 4*4);
    for (int i = 0; i < joints.size(); i++) {
//        dmat.row(joints[i].id) = Eigen::Affine3d::Identity().matrix();
    }
}
