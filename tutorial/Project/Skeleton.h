//
// Created by nadavh on 2/24/23.
//

#ifndef ENGINEREWORK_SKELETON_H
#define ENGINEREWORK_SKELETON_H


#include <vector>
#include <Eigen/Core>
#include <Eigen/Geometry>
#include "joint.h"


class Skeleton {
public:
    std::vector<Joint> joints;
    Eigen::MatrixXd dmat;
    Skeleton(const std::vector<Joint>& joints);
    void SetJointTransform(int joint_id, const Eigen::Affine3d& transform);
    Eigen::Affine3d GetJointTransform(int joint_id) const;

};


#endif //ENGINEREWORK_SKELETON_H
