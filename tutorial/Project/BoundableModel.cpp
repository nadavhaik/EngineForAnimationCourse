//
// Created by nadavh on 2/23/23.
//

#include "BoundableModel.h"

#include <utility>
#include "functionals.h"

Box BoundableModel::GetBoundingBox() {
    std::vector<Eigen::Vector3f> fixedVertices = GetFixedVertices();

    Eigen::Vector3f minCorner(std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity());
    Eigen::Vector3f maxCorner(-std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity());

    for(Eigen::Vector3f vertex : fixedVertices) {
        minCorner.x() = std::min(minCorner.x(), vertex.x());
        minCorner.y() = std::min(minCorner.y(), vertex.y());
        minCorner.z() = std::min(minCorner.z(), vertex.z());

        maxCorner.x() = std::max(maxCorner.x(), vertex.x());
        maxCorner.y() = std::max(maxCorner.y(), vertex.y());
        maxCorner.z() = std::max(maxCorner.z(), vertex.z());
    }


    Eigen::AlignedBox<float, 3> box(minCorner, maxCorner);

    return box;
}

Vec3 BoundableModel::FixVertex(const Eigen::Vector3f &vertex) {
    Eigen::Vector4f verAffine(vertex.x(), vertex.y(), vertex.z(), 1);
    Eigen::Vector4f fixedAffine = GetTransform() * verAffine;
    return {fixedAffine.x(), fixedAffine.y(), fixedAffine.z()};
}


std::vector<Vec3> BoundableModel::FixVertices(const std::vector<Eigen::Vector3f>& vertices) {
    return functionals::map<Vec3, Vec3>(vertices,
                                        [this](const Vec3 &vec){return FixVertex(vec);});
}


std::vector<Vec3> BoundableModel::GetFixedVertices() {
    std::vector<Vec3> fixedVertices;
    for(auto meshList : GetMeshList()) {
        for (auto meshData: meshList->data) {
            Eigen::MatrixXd vertices = meshData.vertices;
            for(int i=0; i<vertices.rows(); i++) {
                Eigen::Vector4d vertex4d(vertices(i,0), vertices(i, 1), vertices(i, 2), 1);
                Eigen::Vector4d fixed4d = GetTransform().cast<double>() * vertex4d;
                Eigen::Vector3d fixed(fixed4d.x(), fixed4d.y(), fixed4d.z());
                fixedVertices.emplace_back(fixed.cast<float>());
            }
        }
    }

    return fixedVertices;
}

void printVec(Vec3 vec) {
    std::cout << "(" << vec.x() << "," << vec.y() << "," << vec.z() << ")" << std::endl;
}

Vec3 BoundableModel::GetScalingVec() {
    float Sx = GetTransform().block<3, 1>(0, 0).norm();
    float Sy = GetTransform().block<3, 1>(0, 1).norm();
    float Sz = GetTransform().block<3, 1>(0, 2).norm();

    return {Sx, Sy, Sz};
}

Vec3 BoundableModel::ApplySelfTransform(const Vec3 &vec) {
    Eigen::Affine3f transform;
    transform.matrix() = GetAggregatedTransform();
    return transform * vec;
}

Vec3 BoundableModel::GetCenter() {
    return GetTranslation();
}


Box NodeModel::GetBoundingBox() {
    Vec3 scaling = GetScalingVec();

    Vec3 center = GetCenter();


    Vec3 diag = {NODE_WIDTH * scaling.x(), NODE_HEIGHT * scaling.y(), NODE_LENGTH * scaling.z()};

    Vec3 p1 = center - 0.5 * diag;
    Vec3 p2 = center + 0.5 * diag;

    Vec3 minNode = p1;
    Vec3 maxNode = p2;
//
//    minNode.x() = std::min(p1.x(), p2.x());
//    minNode.y() = std::min(p1.y(), p2.y());
//    minNode.z() = std::min(p1.z(), p2.z());
//
//    maxNode.x() = std::max(p1.x(), p2.x());
//    maxNode.y() = std::max(p1.y(), p2.y());
//    maxNode.z() = std::max(p1.z(), p2.z());


    auto radius = 0.5 * NODE_LENGTH * scaling.z();
    Vec3 radiusVec = {radius, radius, radius};

    Vec3 minBall = center-radiusVec;
    Vec3 maxBall = center+radiusVec;

    bool debug = false;
    if(debug) {
        std::cout << "Center:" << std::endl;
        printVec(GetTranslation());

        std::cout << "Ball:" << std::endl;
        std::cout << "min:" << std::endl;
        printVec(minBall);
        std::cout << "max:" << std::endl;
        printVec(maxBall);

        std::cout << "Node:" << std::endl;
        std::cout << "min:" << std::endl;
        printVec(minNode);
        std::cout << "max:" << std::endl;
        printVec(maxNode);

        std::cout << std::endl;
        std::cout << std::endl;

    }


    return Box(minNode, maxNode);


}

Box BallModel::GetBoundingBox() {
    auto radius = GetScaledRadius();
    Vec3 radiusVec = {radius, radius, radius};
    Vec3 center = GetTranslation();

    return Box(center - radiusVec, center + radiusVec);
}

float BallModel::GetScaledRadius() {
    Vec3 scaling = GetScalingVec();


    assert(scaling.x() == scaling.y());
    assert(scaling.y() == scaling.z());

    return BALL_RADIUS * scaling.x();
}