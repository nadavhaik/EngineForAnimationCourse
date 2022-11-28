//
// Created by ditto on 26/11/2022.
//

#include <per_vertex_normals.h>
#include "Connector.h"
#include "Model.h"


Connector::Connector(std::shared_ptr<cg3d::Model> model): originalModel(model) {
    V = model->GetMesh(0)->data[0].vertices;
    F = model->GetMesh(0)->data[0].faces;
    igl::edge_flaps(F,E,EMAP,EF,EI);
    C.resize(E.rows(),V.cols());
    Eigen::VectorXd costs(E.rows());
    // https://stackoverflow.com/questions/2852140/priority-queue-clear-method
    // Q.clear();
    Q = {};
    EQ = Eigen::VectorXi::Zero(E.rows());
    {
        Eigen::VectorXd costs(E.rows());
        igl::parallel_for(E.rows(),[&](const int e)
        {
            double cost = e;
            Eigen::RowVectorXd p(1,3);
            igl::shortest_edge_and_midpoint(e,V,F,E,EMAP,EF,EI,cost,p);
            C.row(e) = p;
            costs(e) = cost;
        },10000);
        for(int e = 0;e<E.rows();e++)
        {
            Q.emplace(costs(e),e,0);
        }
    }

    num_collapsed = 0;
}

bool Connector::simplifyMesh(igl::opengl::glfw::Viewer *viewer, int numberOfFacesToDelete, std::shared_ptr<cg3d::Mesh> mesh) {


    // If animating then collapse 10% of edges
    if(!Q.empty())
    {
        bool something_collapsed = false;
        // collapse edge
        const int max_iter = std::ceil(numberOfFacesToDelete);
        for(int j = 0;j<max_iter;j++)
        {
            if(!igl::collapse_edge(igl::shortest_edge_and_midpoint,V,F,E,EMAP,EF,EI,Q,EQ,C))
            {
                break;
            }
            something_collapsed = true;
            num_collapsed++;
        }

        if(something_collapsed)
        {
            viewer->data().clear();
            Eigen::MatrixXd vertexNormals;
            igl::per_vertex_normals(V,F,vertexNormals);
            Eigen::MatrixXd textureCoords = Eigen::MatrixXd::Zero(V.rows(),2);
            mesh->data.push_back({V, F, vertexNormals, textureCoords});
             ++originalModel->meshIndex;
             //viewer->draw();
            return true;
        } else {
            return false;
        }
    }
    return false;
}

bool Connector::simplify(igl::opengl::glfw::Viewer *viewer, int numberOfFacesToDelete) {
    return simplifyMesh(viewer, numberOfFacesToDelete, originalModel->GetMeshList()[0]);
}

bool Connector::simplifyTenPercent(igl::opengl::glfw::Viewer *viewer) {
    int facesToDelete =  std::ceil(Q.size()*0.01);
    return simplify(viewer,facesToDelete);
}
