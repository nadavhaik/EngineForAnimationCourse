//
// Created by nadavh on 2/25/23.
//
#include "Model.h"

#include <utility>
#include "BoundableModel.h"
#ifndef ENGINEREWORK_SKINNEDMODEL_H
#define ENGINEREWORK_SKINNEDMODEL_H

using namespace cg3d;

class SkinnedSnakeModel : public Model {
public:
    template<typename... Args>
    static std::shared_ptr<SkinnedSnakeModel> Create(Args&&... args) { return std::make_shared<SkinnedSnakeModel>(SkinnedSnakeModel{std::forward<Args>(args)...}); };
    void Skin();

    inline std::shared_ptr<Mesh> GetMesh(int index = 0) const override;
    inline std::vector<std::shared_ptr<Mesh>> GetMeshList() const override;
    void UpdateDataAndDrawMeshes(const Program& program, bool _showFaces, bool bindTextures) override;
    void MoveSnake(Eigen::Vector3d trans);

private:
    explicit SkinnedSnakeModel(std::string name, std::shared_ptr<cg3d::Mesh> mesh,
                               std::shared_ptr<cg3d::Material> material, std::vector<std::shared_ptr<Model>> joints);
    std::vector<std::shared_ptr<Model>> joints;
    std::vector<std::shared_ptr<Mesh>> modifiedMeshList;

    // vars needed for skinning:
    Eigen::MatrixXd CT;
    Eigen::MatrixXi BET;
    Eigen::MatrixXd W, Cp, U, M;
    Eigen::MatrixXi  BE;
    std::vector<Eigen::Quaterniond> vQs;
    std::vector<Eigen::Vector3d> vCs;
    std::vector<Eigen::Vector3d> vTs;
    Eigen::Quaterniond quat;



};


#endif //ENGINEREWORK_SKINNEDMODEL_H
