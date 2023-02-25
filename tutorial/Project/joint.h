//
// Created by nadavh on 2/24/23.
//

#ifndef ENGINEREWORK_JOINT_H
#define ENGINEREWORK_JOINT_H

#include <string>
#include <Eigen/Geometry>

struct Joint {
    std::string name;
    int id;
    int parent_id;
    Eigen::Affine3d transform;
    Eigen::Affine3d bind_pose;

    Joint() : name(""), parent_id(-1), transform(Eigen::Affine3d::Identity()), bind_pose(Eigen::Affine3d::Identity()) {}
};

#endif //ENGINEREWORK_JOINT_H
