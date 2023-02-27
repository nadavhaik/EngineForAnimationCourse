//
// Created by nadavh on 2/23/23.
//

#ifndef ENGINEREWORK_BOUNDABLEMODEL_H
#define ENGINEREWORK_BOUNDABLEMODEL_H


#include "Model.h"

#include <utility>
#include "types_macros.h"

#define NODE_LENGTH 1.5f
#define NODE_WIDTH 0.75f
#define NODE_HEIGHT 0.75f
#define BALL_RADIUS 38.0f

class BoundableModel : public cg3d::Model {
public:
    virtual Box GetBoundingBox();
    Vec3 GetCenter();

protected:
    explicit BoundableModel(std::string name, std::shared_ptr<cg3d::Mesh> mesh, std::shared_ptr<cg3d::Material> material)
        : cg3d::Movable(name), Model{name, std::move(mesh), std::move(material)} {};
    Vec3 FixVertex(const Vec3 &vertex);
    std::vector<Vec3> FixVertices(const std::vector<Vec3>& vertices);
    std::vector<Vec3> GetFixedVertices();
    Vec3 GetScalingVec();
    Vec3 ApplySelfTransform(const Vec3 &vec);

};
#define BoundablePtr std::shared_ptr<BoundableModel>

class ConstantBoundable : public BoundableModel {
public:
    Box GetBoundingBox() override;
    template<typename... Args>
    static std::shared_ptr<ConstantBoundable> Create(Args&&... args) { return std::make_shared<ConstantBoundable>(ConstantBoundable{std::forward<Args>(args)...}); };
    void CalculateBB();

private:
    explicit ConstantBoundable(std::string name, std::shared_ptr<cg3d::Mesh> mesh, std::shared_ptr<cg3d::Material> material);
    Box boundingBox;


};


class NodeModel : public BoundableModel {
public:
    Box GetBoundingBox() override;
    Vec3 GetDiag();
    template<typename... Args>
    static std::shared_ptr<NodeModel> Create(Args&&... args) { return std::make_shared<NodeModel>(NodeModel{std::forward<Args>(args)...}); };

private:
    explicit NodeModel(std::string name, std::shared_ptr<cg3d::Mesh> mesh, std::shared_ptr<cg3d::Material> material) :
        cg3d::Movable(name), BoundableModel{name, std::move(mesh), std::move(material)} {}

};
#define NodePtr std::shared_ptr<NodeModel>

class BallModel : public BoundableModel {
public:
    Box GetBoundingBox() override;
    template<typename... Args>
    static std::shared_ptr<BallModel> Create(Args&&... args) { return std::make_shared<BallModel>(BallModel{std::forward<Args>(args)...}); };


private:
    template<typename... Args>
    explicit BallModel(std::string name, std::shared_ptr<cg3d::Mesh> mesh,
                       std::shared_ptr<cg3d::Material> material, float radius=BALL_RADIUS) :
        cg3d::Movable(name), BoundableModel{name, std::move(mesh), std::move(material)}, radius(radius) {}
    float GetScaledRadius();
    float radius;
};
#define BallPtr std::shared_ptr<BallModel>





#endif //ENGINEREWORK_BOUNDABLEMODEL_H
