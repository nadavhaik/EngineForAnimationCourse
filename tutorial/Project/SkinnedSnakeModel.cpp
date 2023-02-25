//
// Created by nadavh on 2/25/23.
//

#include "SkinnedSnakeModel.h"

#include <utility>
#include <dqs.cpp>
#include <deform_skeleton.h>
#include <per_vertex_normals.h>
#include <igl/triangle/triangulate.h>

SkinnedSnakeModel::SkinnedSnakeModel(std::string name, std::shared_ptr<cg3d::Mesh> mesh,
                                     std::shared_ptr<cg3d::Material> material, std::vector<std::shared_ptr<Model>> joints) :
        cg3d::Movable(name), Model(name, mesh, std::move(material)), joints(joints) {

    if(mesh->data.size() > 1) {
        throw std::runtime_error("More than 1 mesh isn't supported here!!!");
    }
    modifiedMeshList = {mesh};

    vCs.resize(joints.size() + 1);
    vTs.resize(joints.size() + 1);
    vQs.resize(joints.size() + 1, Eigen::Quaterniond::Identity());

    Eigen::Vector3d minV = mesh->data[0].vertices.colwise().minCoeff();
    Eigen::Vector3d maxV = mesh->data[0].vertices.colwise().maxCoeff();

    BE.resize(joints.size(), 2);
    Cp.resize(joints.size() + 1, 3);
    CT.resize(joints.size() * 2, 3);

    for(int i=0; i<joints.size(); i++) {
        BE(i, 0) = i;
        BE(i, 1) = i+1;
    }

    Eigen::Vector3d pos(0, 0, -10);
    Eigen::MatrixXd points(joints.size()+1, 3);

    for (int i = 0; i < joints.size() + 1; i++) {
        vCs[i] = vTs[i] = points.row(i) = pos;
        pos += Eigen::Vector3d(NODE_LENGTH, 0, 0);
    }

    for (int i = 0; i < joints.size() + 1; i++) {
        Cp.row(i) = vCs[i];
        if(i == 0) continue;
        CT.row(i * 2 - 1) = vCs[i];
    }

    // calculating W:
    int n = mesh->data[0].vertices.rows();
    W.resize(n, joints.size()+1);
    W.setZero();
    for (int i = 0; i < n; i++) {
        double currentZ = mesh->data[0].vertices.row(i).z();
        for(int j=0; j<joints.size()+1; j++) {
            if (currentZ >= minV.z() + NODE_LENGTH * j && currentZ <=  minV.z() + NODE_LENGTH * (j + 1)) {
                auto weight = 10 * abs(currentZ - (minV.z() + NODE_LENGTH * (j+1)));
                W(i, j) = weight;
                W(i, j+1) = 1 - weight;
                break;
            }
        }
    }

}

float RollRandomAB(float min, float max){
    return min + static_cast <float> (rand()) /( static_cast <float> (RAND_MAX/(max - min)));
}

void SkinnedSnakeModel::Skin() {
    int dim = Cp.cols();
    Eigen::MatrixXd T(BE.rows() * (dim + 1), dim);

    auto originalMesh = Model::GetMesh();
    auto modifiedMesh = modifiedMeshList[0];
    for (int i = 0; i < joints.size(); i++) {
       vTs[i] = joints[i]->GetTranslation().cast<double>();
//       vTs[i] = {RollRandomAB(-100, 100), RollRandomAB(-100, 100), RollRandomAB(-100, 100)};
    }
//    vTs[joints.size()] = {0, 0, 0};

//    for (int i = 0; i < joints.size() + 1; i++) {
//
//        vTs[i] = joints[i]->GetTranslation().cast<double>();
//    }

//    for (int i = 0; i < joints.size() + 1; i++) {
//        vTs[i] = joints[i]->GetTranslation().cast<double>();
//    }

    for (int e = 0; e < BE.rows(); e++) {
        Eigen::Affine3d a = Eigen::Affine3d::Identity();
        a.translate(vTs[e]);
        a.rotate(vQs[e]);
        T.block(e * (dim + 1), 0, dim + 1, dim) =
                a.matrix().transpose().block(0, 0, dim + 1, dim);
    }

    igl::dqs(modifiedMesh->data[0].vertices, W, vQs, vTs, U);
    igl::deform_skeleton(Cp, BE, T, CT, BET);


    Eigen::MatrixXd textureCoords = Eigen::MatrixXd::Zero(U.rows(),2);
//    Eigen::MatrixXd Vout;
//    Eigen::MatrixXi Fout;
    Eigen::MatrixXi Eout;
//    igl::triangle::triangulate(U, BET, {}, "p", Vout, Fout);

    std::vector<cg3d::MeshData> newMeshDataList = {{U, originalMesh->data[0].faces, originalMesh->data[0].vertexNormals, textureCoords}};
    modifiedMeshList = {std::make_shared<Mesh>("modified mesh", newMeshDataList)};
//    SetMeshList(modifiedMeshList);
}

std::shared_ptr<Mesh> SkinnedSnakeModel::GetMesh(int index) const {
    return modifiedMeshList[index];
}

std::vector<std::shared_ptr<Mesh>> SkinnedSnakeModel::GetMeshList() const {
    return modifiedMeshList;
}

void SkinnedSnakeModel::UpdateDataAndDrawMeshes(const Program &program, bool _showFaces, bool bindTextures) {
    for (auto& viewerDataList: viewerDataListPerMesh) {
        auto &viewerData = viewerDataList[std::min(meshIndex, int(viewerDataList.size() - 1))];
        UpdateDataAndBindMesh(viewerData, program);
        if (bindTextures) material->BindTextures();
        if (mode == 0)
            viewerData.meshgl.draw_mesh(_showFaces);
        else
            viewerData.meshgl.draw_lines();
    }
}
