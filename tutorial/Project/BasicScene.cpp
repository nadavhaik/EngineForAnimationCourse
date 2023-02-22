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
    
     snakeMaterial = {std::make_shared<Material>("snakeMaterial", program)}; // empty snakeMaterial
//    SetNamedObject(cube, Model::Create, Mesh::Cube(), snakeMaterial, shared_from_this());
 
    snakeMaterial->AddTexture(0, "textures/box0.bmp", 2);

    snakeMesh = {IglLoader::MeshFromFiles("snake","data/snake2.obj")};
    auto snakeRoot = Model::Create("snake", snakeMesh, snakeMaterial);
    snakeRoot->Rotate(1.5708, Axis::Y);
    snakeRoot->Translate(-10, Axis::Z);
    root->AddChild(snakeRoot);

    snakeNodes.push_back(snakeRoot);
    headings.push_back(0);
    
    //Axis
    Eigen::MatrixXd vertices(6,3);
    vertices << -1,0,0,1,0,0,0,-1,0,0,1,0,0,0,-1,0,0,1;
    Eigen::MatrixXi faces(3,2);
    faces << 0,1,2,3,4,5;
    Eigen::MatrixXd vertexNormals = Eigen::MatrixXd::Ones(6,3);
    Eigen::MatrixXd textureCoords = Eigen::MatrixXd::Ones(6,2);
    std::shared_ptr<Mesh> coordsys = std::make_shared<Mesh>("coordsys",vertices,faces,vertexNormals,textureCoords);

    executor = std::make_shared<PeriodicExecutor>(PERIODIC_INTERVAL_MILLIS, [this]() {PeriodicFunction();});
    executor->Start();
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

//    snakeNodes[0]->Translate(0.01f, Axis::Y);
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
            if (buttonState[GLFW_MOUSE_BUTTON_RIGHT] != GLFW_RELEASE)
                pickedModel->TranslateInSystem(system, {-float(xAtPress - x) / moveCoeff, float(yAtPress - y) / moveCoeff, 0});
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
            case GLFW_KEY_ESCAPE:
                glfwSetWindowShouldClose(window, GLFW_TRUE);
                break;
            case GLFW_KEY_UP:
                break;
            case GLFW_KEY_DOWN:
                break;
            case GLFW_KEY_LEFT:
                TurnLeft();
                break;
            case GLFW_KEY_RIGHT:
                TurnRight();
                break;
            case GLFW_KEY_W:
                camera->TranslateInSystem(system, {0, 0.1f, 0});
                break;
            case GLFW_KEY_S:
                camera->TranslateInSystem(system, {0, -0.1f, 0});
                break;
            case GLFW_KEY_A:
                camera->TranslateInSystem(system, {-0.1f, 0, 0});
                break;
            case GLFW_KEY_D:
                camera->TranslateInSystem(system, {0.1f, 0, 0});
                break;
            case GLFW_KEY_B:
                camera->TranslateInSystem(system, {0, 0, 0.1f});
                break;
            case GLFW_KEY_F:
                camera->TranslateInSystem(system, {0, 0, -0.1f});
                break;
            case GLFW_KEY_1:
                if( pickedIndex > 0)
                  pickedIndex--;
                break;
            case GLFW_KEY_2:

                break;
            case GLFW_KEY_3:
                if( tipIndex >= 0)
                {

                  tipIndex--;
                }
                break;
            case GLFW_KEY_4:

                break;
            case GLFW_KEY_SPACE:
                AddToTail();
                break;
        }
    }
}

#define MOVEMENT_DISTANCE 0.01f

void BasicScene::PeriodicFunction() {
    for(auto &node : snakeNodes) {
        Eigen::Vector3f translation = MOVEMENT_DISTANCE * node->GetRotation() * Eigen::Vector3f(0, 0, 1);
        node->Translate(translation);
    }

    std::shared_ptr<Model> head = snakeNodes.front();
    for(int i=1; i<snakeNodes.size(); i++) {
        std::shared_ptr<Model> node = snakeNodes[i];
        if(ModelsCollide(head, node)) {
            std::cout << "Collusion!!!" << std::endl;
        }
    }

}

BasicScene::~BasicScene() {
    if(executor != nullptr) {
        executor->Stop();
    }
}

void BasicScene::TurnRight() {
//    headHeading -= NINETY_DEGREES_IN_RADIANS;
    snakeNodes[0]->Rotate(SNAKE_TURN_ANGLE_RADIANS, Axis::X);
    headings[0] += SNAKE_TURN_ANGLE_RADIANS;
}

void BasicScene::TurnLeft() {
//    headHeading += NINETY_DEGREES_IN_RADIANS;
    snakeNodes[0]->Rotate(-SNAKE_TURN_ANGLE_RADIANS, Axis::X);
    headings[0] -= SNAKE_TURN_ANGLE_RADIANS;
}

void BasicScene::AddToTail() {

    auto newNode = Model::Create("node", snakeMesh, snakeMaterial);

    std::shared_ptr<Model> parent = snakeNodes.back();
    root->AddChild(newNode);
    newNode->Rotate(parent->GetRotation());
    newNode->Translate(parent->GetTranslation());
    double heading = headings.back();

    float xTrans = cos(heading);
    float yTrans = sin(heading);

    newNode->Translate( NODE_HEIGHT * Eigen::Vector3f(-xTrans, yTrans, 0));

    snakeNodes.push_back(newNode);
    headings.push_back(heading);
}

Eigen::AlignedBox<float, 3> BasicScene::BoxOfModel(std::shared_ptr<cg3d::Model> model) {
    std::vector<Eigen::Vector3f> fixedVertices;
    for(auto meshList : model->GetMeshList()) {
        for (auto meshData: meshList->data) {
            Eigen::MatrixXd vertices = meshData.vertices;
            for(int i=0; i<vertices.rows(); i++) {
                Eigen::Vector4d vertex4d(vertices(i,0), vertices(i, 1), vertices(i, 2), 1);
                Eigen::Vector4d fixed4d = model->GetTransform().cast<double>() * vertex4d;
                Eigen::Vector3d fixed(fixed4d.x(), fixed4d.y(), fixed4d.z());
                fixedVertices.emplace_back(fixed.cast<float>());
            }

        }
    }

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

#define AlignedBox3f Eigen::AlignedBox<float, 3>
bool BasicScene::ModelsCollide(std::shared_ptr<cg3d::Model> m1, std::shared_ptr<cg3d::Model> m2) {
    AlignedBox3f b1 = BoxOfModel(m1);
    AlignedBox3f b2 = BoxOfModel(m2);

    return b1.intersects(b2);
}

