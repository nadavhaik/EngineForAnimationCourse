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

    return Box(p1, p2);


}

Vec3 NodeModel::GetDiag() {
    Vec3 scaling = GetScalingVec();


    return ApplySelfTransform( GetCenter()- Eigen::Vector3f {NODE_WIDTH * scaling.x(), NODE_HEIGHT * scaling.y(), NODE_LENGTH * scaling.z()});
}

Box BallModel::GetBoundingBox() {
    auto radius = GetScaledRadius();
    Vec3 radiusVec = {radius, radius, radius};
    Vec3 center = GetTranslation();



    return Box(center - radiusVec, center + radiusVec);
}

float BallModel::GetScaledRadius() {
    Vec3 scaling = GetScalingVec();


    assert(scaling.x() - scaling.y() < 0.0001);
    assert(scaling.y() - scaling.z() < 0.0001);

    return BALL_RADIUS * scaling.x();
}


ConstantBoundable::ConstantBoundable(std::string name, std::shared_ptr<cg3d::Mesh> mesh,
                                     std::shared_ptr<cg3d::Material> material)  : cg3d::Movable(name),
                                     BoundableModel{name, std::move(mesh), std::move(material)}
                                     {}

Box ConstantBoundable::GetBoundingBox() {
    return boundingBox;
}

void ConstantBoundable::CalculateBB() {

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


    boundingBox = {minCorner, maxCorner};
}
