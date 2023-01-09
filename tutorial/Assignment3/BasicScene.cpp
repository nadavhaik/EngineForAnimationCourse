#include "BasicScene.h"
#include <Eigen/src/Core/Matrix.h>
#include <edges.h>
#include <memory>
#include <per_face_normals.h>
#include <read_triangle_mesh.h>
#include <utility>
#include <vector>
#include "GLFW/glfw3.h"
#include "Mesh.h"
#include "PickVisitor.h"
#include "Renderer.h"
#include "ObjLoader.h"
#include "IglMeshLoader.h"

#include "igl/per_vertex_normals.h"
#include "igl/per_face_normals.h"
#include "igl/unproject_onto_mesh.h"
#include "igl/edge_flaps.h"
#include "igl/loop.h"
#include "igl/upsample.h"
#include "igl/AABB.h"
#include "igl/parallel_for.h"
#include "igl/shortest_edge_and_midpoint.h"
#include "igl/circulation.h"
#include "igl/edge_midpoints.h"
#include "igl/collapse_edge.h"
#include "igl/edge_collapse_is_valid.h"
#include "igl/write_triangle_mesh.h"

// #include "AutoMorphingModel.h"

using namespace cg3d;

#define RADIANS_IN_DEGREE 0.0174533f

#define DRAW_AXIS false
#define SPHERE_SCALE_FACTOR 0.8f
#define CYLS_SCALE_FACTOR 0.5f
#define SINGLE_CYLINDER_SIZE 1.6f
#define MIN_DISTANCE_FOR_MOVEMENT 0.05f
#define DEGREES_PER_FRAME 0.5f
#define RADIANS_PER_FRAME (RADIANS_IN_DEGREE * DEGREES_PER_FRAME)
#define NUMBER_OF_CYLINDERS 5


void BasicScene::Init(float fov, int width, int height, float near, float far)
{
    camera = Camera::Create( "camera", fov, float(width) / height, near, far);
    
    AddChild(root = Movable::Create("root")); // a common (invisible) parent object for all the shapes
    auto daylight{std::make_shared<Material>("daylight", "shaders/cubemapShader")}; 
    daylight->AddTexture(0, "textures/cubemaps/Daylight Box_", 3);
    auto background{Model::Create("background", Mesh::Cube(), daylight)};
    AddChild(background);
    background->Scale(120, Axis::XYZ);
    background->SetPickable(false);
    background->SetStatic();

 
    auto program = std::make_shared<Program>("shaders/phongShader");
    auto program1 = std::make_shared<Program>("shaders/pickingShader");
    
    auto material{ std::make_shared<Material>("material", program)}; // empty material
    auto material1{ std::make_shared<Material>("material", program1)}; // empty material
//    SetNamedObject(cube, Model::Create, Mesh::Cube(), material, shared_from_this());
 
    material->AddTexture(0, "textures/box0.bmp", 2);
    auto sphereMesh{IglLoader::MeshFromFiles("sphere_igl", "data/sphere.obj")};
    auto cylMesh{IglLoader::MeshFromFiles("cyl_igl","data/xcylinder.obj")};
    auto cubeMesh{IglLoader::MeshFromFiles("cube_igl","data/cube_old.obj")};
    sphere1 = Model::Create( "sphere",sphereMesh, material);    
//    cube = Model::Create( "cube", cubeMesh, material);

    //Axis
    Eigen::MatrixXd vertices(6,3);
    vertices << -1,0,0,1,0,0,0,-1,0,0,1,0,0,0,-1,0,0,1;
    Eigen::MatrixXi faces(3,2);
    faces << 0,1,2,3,4,5;
    Eigen::MatrixXd vertexNormals = Eigen::MatrixXd::Ones(6,3);
    Eigen::MatrixXd textureCoords = Eigen::MatrixXd::Ones(6,2);
    std::shared_ptr<Mesh> coordsys = std::make_shared<Mesh>("coordsys",vertices,faces,vertexNormals,textureCoords);


    cyls.push_back( Model::Create("cyl",cylMesh, material));
    cyls[0]->SetCenter(Eigen::Vector3f(-SINGLE_CYLINDER_SIZE / 2.0f, 0, 0));
    cyls[0]->Scale(CYLS_SCALE_FACTOR, Axis::YZ);
    root->AddChild(cyls[0]);
    if(DRAW_AXIS) {
        axis.push_back(Model::Create("axis", coordsys, material1));
        axis[0]->mode = 1;
        axis[0]->Scale(4, Axis::XYZ);
        axis[0]->lineWidth = 5;
        axis[0]->Translate(SINGLE_CYLINDER_SIZE  / 2.0f, Axis::X);
        axis[0]->SetCenter(Eigen::Vector3f(-SINGLE_CYLINDER_SIZE / 2.0f, 0, 0));
        cyls[0]->AddChild(axis[0]);
    }


   
    for(int i = 1;i < NUMBER_OF_CYLINDERS; i++)
    { 
        cyls.push_back( Model::Create("cyl", cylMesh, material));
        cyls[i]->Translate(SINGLE_CYLINDER_SIZE, Axis::X);
        cyls[i]->SetCenter(Eigen::Vector3f(-SINGLE_CYLINDER_SIZE / 2.0f, 0, 0));
        cyls[i]->Scale(CYLS_SCALE_FACTOR, Axis::YZ);
        cyls[i-1]->AddChild(cyls[i]);

        if(DRAW_AXIS) {
            axis.push_back(Model::Create("axis", coordsys, material1));
            axis[i]->mode = 1;
            axis[i]->lineWidth = 5;
            axis[i]->Scale(4, Axis::XYZ);
            axis[i]->Translate(SINGLE_CYLINDER_SIZE / 2.0f, Axis::X);
            axis[i]->SetCenter(Eigen::Vector3f(-SINGLE_CYLINDER_SIZE / 2.0f, 0, 0));
            cyls[i]->AddChild(axis[i]);
        }
    }
    cyls[0]->Translate({SINGLE_CYLINDER_SIZE / 2.0f, 0, 0});
    auto morphFunc = [](Model* model, cg3d::Visitor* visitor) {
      return model->meshIndex;//(model->GetMeshList())[0]->data.size()-1;
    };
//    autoCube = AutoMorphingModel::Create(*cube, morphFunc);

  
    sphere1->showWireframe = true;
    sphere1->Scale(SPHERE_SCALE_FACTOR, Axis::XYZ);
//    autoCube->Translate({-6,0,0});
//    autoCube->Scale(1.5f);
    state = STATIC;
//    sphere1->Translate({-2,0,0});


//    autoCube->showWireframe = true;
    camera->Translate(22, Axis::Z);
    root->AddChild(sphere1);
//    root->AddChild(cyl);
//    root->AddChild(autoCube);
    // points = Eigen::MatrixXd::Ones(1,3);
    // edges = Eigen::MatrixXd::Ones(1,3);
    // colors = Eigen::MatrixXd::Ones(1,3);
    
    // cyl->AddOverlay({points,edges,colors},true);
//    cube->mode =1   ;
//    auto mesh = cube->GetMeshList();

    //autoCube->AddOverlay(points,edges,colors);
    // mesh[0]->data.push_back({V,F,V,E});
//    int num_collapsed;

  // Function to reset original mesh and data structures
//    V = mesh[0]->data[0].vertices;
//    F = mesh[0]->data[0].faces;
//   // igl::read_triangle_mesh("data/cube.off",V,F);
//    igl::edge_flaps(F,E,EMAP,EF,EI);
//    std::cout<< "vertices: \n" << V <<std::endl;
//    std::cout<< "faces: \n" << F <<std::endl;
//
//    std::cout<< "edges: \n" << E.transpose() <<std::endl;
//    std::cout<< "edges to faces: \n" << EF.transpose() <<std::endl;
//    std::cout<< "faces to edges: \n "<< EMAP.transpose()<<std::endl;
//    std::cout<< "edges indices: \n" << EI.transpose() <<std::endl;


    //RotateConstantly(cyls[0], Eigen::Vector3f(1, 1 ,1), 0.8);
    nextArmToMove = cyls.size()-1;

}

void BasicScene::Update(const Program& program, const Eigen::Matrix4f& proj, const Eigen::Matrix4f& view, const Eigen::Matrix4f& model)
{
    Scene::Update(program, proj, view, model);
    program.SetUniform4f("lightColor", 0.8f, 0.3f, 0.0f, 0.5f);
    program.SetUniform4f("Kai", 1.0f, 0.3f, 0.6f, 1.0f);
    program.SetUniform4f("Kdi", 0.5f, 0.5f, 0.0f, 1.0f);
    program.SetUniform1f("specular_exponent", 5.0f);
    program.SetUniform4f("light_position", 0.0, 15.0f, 0.0, 1.0f);
//    cyl->Rotate(0.001f, Axis::Y);
//    cube->Rotate(0.1f, Axis::XYZ);
    if(state != ANIMATING) return;
    if(movements.empty()) {
        CalculateNextSteps();
    } else {
        AnimateNextStep();
    }
}

void BasicScene::CalculateNextSteps() {
    Eigen::Vector3f E = GetCylinderPos(cyls.size()-1);
    Eigen::Vector3f D = GetDestination();
    float distanceFromStart = D.norm();
    float distanceFromEnd = (D - E).norm();
    if(distanceFromStart > SINGLE_CYLINDER_SIZE * cyls.size()) {
        std::cout << "Too far! distance is: " << distanceFromStart << std::endl;
        StopAnimating();
        return;
    }
    if(distanceFromEnd < MIN_DISTANCE_FOR_MOVEMENT){
        std::cout << "Done!" << std::endl;
        StopAnimating();
        return;
    }

    Eigen::Vector3f R;
    if(nextArmToMove == 0) {
        R = Eigen::Vector3f::Zero();
    } else {
        R = GetCylinderPos(nextArmToMove - 1);
    }
//    std::cout << "nextArm: " << nextArmToMove << ", R: " << VectorToString(R) << std::endl;
    Eigen::Vector3f RD = D - R;
    Eigen::Vector3f RE = E - R;

    Eigen::Vector3f normal = RE.cross(RD).normalized();


//    std::cout << "normal: " << VectorToString(normal) << std::endl;

    float rotationAngle = acos(RE.dot(RD) / (RE.norm() * RD.norm()));

    RotateConstantly(cyls[nextArmToMove], normal, rotationAngle);
    nextArmToMove = nextArmToMove == 0 ? cyls.size() - 1 : nextArmToMove - 1;
}


void BasicScene::MouseCallback(Viewport* viewport, int x, int y, int button, int action, int mods, int buttonState[])
{
    // note: there's a (small) chance the button state here precedes the mouse press/release event

    if (action == GLFW_PRESS) { // default mouse button press behavior
        PickVisitor visitor;
        visitor.Init();
        renderer->RenderViewportAtPos(x, y, &visitor); // pick using fixed colors hack
        auto modelAndDepth = visitor.PickAtPos(x, renderer->GetWindowHeight() - y);
        renderer->RenderViewportAtPos(x, y); // draw again to avoid flickering
        pickedModel = modelAndDepth.first ? std::dynamic_pointer_cast<Model>(modelAndDepth.first->shared_from_this()) : nullptr;
        pickedModelDepth = modelAndDepth.second;
        camera->GetRotation().transpose();
        xAtPress = x;
        yAtPress = y;

        // if (pickedModel)
        //     debug("found ", pickedModel->isPickable ? "pickable" : "non-pickable", " model at pos ", x, ", ", y, ": ",
        //           pickedModel->name, ", depth: ", pickedModelDepth);
        // else
        //     debug("found nothing at pos ", x, ", ", y);

        if (pickedModel && !pickedModel->isPickable)
            pickedModel = nullptr; // for non-pickable models we need only pickedModelDepth for mouse movement calculations later

        if (pickedModel)
            pickedToutAtPress = pickedModel->GetTout();
        else
            cameraToutAtPress = camera->GetTout();
    }
}

void BasicScene::ScrollCallback(Viewport* viewport, int x, int y, int xoffset, int yoffset, bool dragging, int buttonState[])
{
    // note: there's a (small) chance the button state here precedes the mouse press/release event
    Eigen::Matrix3f system = camera->GetRotation().transpose();
    if (pickedModel) {
        if(std::find(cyls.begin(), cyls.end(), pickedModel) != cyls.end()) {
            return;
        }
        pickedModel->TranslateInSystem(system, {0, 0, -float(yoffset)});
        pickedToutAtPress = pickedModel->GetTout();
    } else {
        camera->TranslateInSystem(system, {0, 0, -float(yoffset)});
        cameraToutAtPress = camera->GetTout();
    }
}

void BasicScene::CursorPosCallback(Viewport* viewport, int x, int y, bool dragging, int* buttonState)
{
    if (dragging) {
        Eigen::Matrix3f system = camera->GetRotation().transpose() * GetRotation();
        auto moveCoeff = camera->CalcMoveCoeff(pickedModelDepth, viewport->width);
        auto angleCoeff = camera->CalcAngleCoeff(viewport->width);
        if (pickedModel) {
            //pickedModel->SetTout(pickedToutAtPress);
            if (buttonState[GLFW_MOUSE_BUTTON_RIGHT] != GLFW_RELEASE) {
                if(std::find(cyls.begin(), cyls.end(), pickedModel) != cyls.end()) {
                    return;
                }
                pickedModel->TranslateInSystem(system,
                                               {-float(xAtPress - x) / moveCoeff, float(yAtPress - y) / moveCoeff, 0});
            }
            if (buttonState[GLFW_MOUSE_BUTTON_MIDDLE] != GLFW_RELEASE)
                pickedModel->RotateInSystem(system, float(xAtPress - x) / angleCoeff, Axis::Z);
            if (buttonState[GLFW_MOUSE_BUTTON_LEFT] != GLFW_RELEASE) {
                pickedModel->RotateInSystem(system, float(xAtPress - x) / angleCoeff, Axis::Y);
                pickedModel->RotateInSystem(system, float(yAtPress - y) / angleCoeff, Axis::X);
            }
        } else {
           // camera->SetTout(cameraToutAtPress);
            if (buttonState[GLFW_MOUSE_BUTTON_RIGHT] != GLFW_RELEASE)
                root->TranslateInSystem(system, {-float(xAtPress - x) / moveCoeff/10.0f, float( yAtPress - y) / moveCoeff/10.0f, 0});
            if (buttonState[GLFW_MOUSE_BUTTON_MIDDLE] != GLFW_RELEASE)
                root->RotateInSystem(system, float(x - xAtPress) / 180.0f, Axis::Z);
            if (buttonState[GLFW_MOUSE_BUTTON_LEFT] != GLFW_RELEASE) {
                root->RotateInSystem(system, float(x - xAtPress) / angleCoeff, Axis::Y);
                root->RotateInSystem(system, float(y - yAtPress) / angleCoeff, Axis::X);
            }
        }
        xAtPress =  x;
        yAtPress =  y;
    }
}

void BasicScene::KeyCallback(Viewport* viewport, int x, int y, int key, int scancode, int action, int mods)
{
    Eigen::Matrix3f system = camera->GetRotation().transpose();

    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        switch (key) // NOLINT(hicpp-multiway-paths-covered)
        {
            case GLFW_KEY_SPACE:
                ChangeAnimationState();
                break;
            case GLFW_KEY_ESCAPE:
                glfwSetWindowShouldClose(window, GLFW_TRUE);
                break;
            case GLFW_KEY_UP:
                cyls[pickedIndex]->RotateInSystem(system, 0.1f, Axis::X);
                break;
            case GLFW_KEY_DOWN:
                cyls[pickedIndex]->RotateInSystem(system, -0.1f, Axis::X);
                break;
            case GLFW_KEY_LEFT:
                cyls[pickedIndex]->RotateInSystem(system, 0.1f, Axis::Y);
                break;
            case GLFW_KEY_RIGHT:
                cyls[pickedIndex]->RotateInSystem(system, -0.1f, Axis::Y);
                break;
            case GLFW_KEY_D:
                std::cout << "Destination: " << VectorToString(GetDestination()) << std::endl;
                break;
            case GLFW_KEY_P:
                PrintRotationMatrices(cyls[pickedIndex]);
                break;
            case GLFW_KEY_T:
                for(int i=0; i<cyls.size(); i++){
                    std::cout<<"Cylinder no. "<< i << ": " << VectorToString(GetCylinderPos(i)) << std::endl;
                }
                break;
            case GLFW_KEY_1:
                if(pickedIndex > 0)
                  pickedIndex--;
                break;
            case GLFW_KEY_2:
                if(pickedIndex < cyls.size()-1)
                    pickedIndex++;
                break;
            case GLFW_KEY_3:
                if( tipIndex >= 0)
                {
                  if(tipIndex == cyls.size())
                    tipIndex--;
                  sphere1->Translate(GetSpherePos());
                  tipIndex--;
                }
                break;
            case GLFW_KEY_4:
                if(tipIndex < cyls.size())
                {
                    if(tipIndex < 0)
                      tipIndex++;
                    sphere1->Translate(GetSpherePos());
                    tipIndex++;
                }
                break;

            case GLFW_KEY_5:
                for(int i=0; i<cyls.size(); i++) {
                    auto pos = GetCylinderPos(i);
                    std::cout << pos[0] << "," << pos[1] << "," << std::endl;
                }
                break;

        }
    }
}

Eigen::Vector3f BasicScene::GetSpherePos()
{
      Eigen::Vector3f l = Eigen::Vector3f(SINGLE_CYLINDER_SIZE, 0, 0);
      Eigen::Vector3f res;
      res = cyls[tipIndex % cyls.size()]->GetRotation()*l;
      return res;
}

Eigen::Vector3f BasicScene::GetCylinderPos(int cylIndex) {
    Eigen::Vector3f l = Eigen::Vector3f(SINGLE_CYLINDER_SIZE, 0, 0);
    Eigen::Vector3f result = Eigen::Vector3f::Zero();
    for(int i=0; i<=cylIndex; i++) {
        result += cyls[i]->GetRotation() * l;
    }
    return result;
}


void BasicScene::RotateConstantly(std::shared_ptr<cg3d::Model> model, Eigen::Vector3f axis, float angle) {

    if(angle < 0) {
        angle += 2 * std::numbers::pi;
    }

    for(float movement = 0.0f; movement < angle; movement += RADIANS_PER_FRAME) {
        movements.push({model, axis, RADIANS_PER_FRAME});
    }
    float remainder = (RADIANS_PER_FRAME * angle) - ((int) (RADIANS_PER_FRAME * angle));
    if(remainder > 0) {
        movements.push({model, axis, remainder});
    }
}


void BasicScene::AnimateNextStep() {
    if(movements.empty()) {
        return;
    }
    MovementCommand nextCommand = movements.front();
    if(movements.empty()) {
        return;
    }
    movements.pop();
    nextCommand.model->Rotate(nextCommand.angle, nextCommand.axis);
}

void BasicScene::StartAnimating() { state = ANIMATING; }
void BasicScene::StopAnimating() { state = STATIC; }

void BasicScene::ChangeAnimationState() {
    if(state == STATIC) StartAnimating();
    else StopAnimating();
}

void BasicScene::printMatrix(const Eigen::MatrixXf &mat) {
    for(int i=0; i<mat.cols(); i++) {
        for(int j=0; j<mat.rows(); j++) {
            std::cout << mat(j, i) << " ";
        }
        std::cout << std::endl;
    }
}

std::tuple<Eigen::Matrix3f, Eigen::Matrix3f, Eigen::Matrix3f>
        BasicScene::GetRotationMatrices(std::shared_ptr<cg3d::Model> model) {
    Eigen::Vector3f eulerAngles = cyls[pickedIndex]->GetRotation().eulerAngles(2, 0, 2);
    Eigen::Matrix3d A1;
    A1 <<   cos(eulerAngles[0]), -sin(eulerAngles[0]), 0,
            sin(eulerAngles[0]),  cos(eulerAngles[0]), 0,
            0                      ,  0                    , 1;

    Eigen::Matrix3d A2;
    A2 << 1         , 0                     ,  0                     ,
            0       , cos(eulerAngles[1]), -sin(eulerAngles[1]),
            0       , sin(eulerAngles[1]),  cos(eulerAngles[1]);

    Eigen::Matrix3d A3;
    A3 <<   cos(eulerAngles[2])    , -sin(eulerAngles[2]) , 0,
            sin(eulerAngles[2])    , cos(eulerAngles[2])  , 0,
            0                         , 0                       , 1;

    return std::make_tuple(A1.cast<float>(), A2.cast<float>(), A3.cast<float>());

}

void BasicScene::PrintRotationMatrices(std::shared_ptr<cg3d::Model> model) {
    std::tuple<Eigen::Matrix3f, Eigen::Matrix3f, Eigen::Matrix3f> rotationMatrices = GetRotationMatrices(model);

    Eigen::Matrix3f A1 = std::get<0>(rotationMatrices);
    Eigen::Matrix3f A2 = std::get<1>(rotationMatrices);
    Eigen::Matrix3f A3 = std::get<2>(rotationMatrices);

    std::cout << "A1:" << std::endl;
    printMatrix(A1);

    std::cout << "A2:" << std::endl;
    printMatrix(A2);

    std::cout << "A3:" << std::endl;
    printMatrix(A3);
}

Eigen::Vector3f BasicScene::GetDestination() {
    return sphere1->GetRotation()*sphere1->GetTranslation();
}

std::string BasicScene::VectorToString(Eigen::Vector3f vec) {
    return "(" + std::to_string(vec.x()) + ", " + std::to_string(vec.y()) + ", " +
        std::to_string(vec.z()) + ")";
}


