#include "BasicScene.h"
#include <Eigen/src/Core/Matrix.h>
#include <edges.h>
#include <memory>
#include <per_face_normals.h>
#include <read_triangle_mesh.h>
#include <utility>
#include <vector>
#include "GLFW/glfw3.h"
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
#include "types_macros.h"
#include "BoundableModel.h"

// #include "AutoMorphingModel.h"

using namespace cg3d;



void BasicScene::Init(float fov, int width, int height, float near, float far)
{
    povCam = Camera::Create( "pov", 90.0f, float(width) / height, near, far);
    tpsCam = Camera::Create( "tps", 90.0f, float(width) / height, near, far);
    topViewCam = Camera::Create( "camera", 80.0f, float(width) / height, near, far);

    topViewCam->Translate(10, Axis::Z);
    camera = topViewCam;

    AddChild(root = Movable::Create("root")); // a common (invisible) parent object for all the shapes
    auto boxMat{std::make_shared<Material>("boxMat", "shaders/cubemapShader")};
    boxMat->AddTexture(0, "textures/cubemaps/box0_", 3);
    backgroundBox = ConstantBoundable::Create("backgroundBox", Mesh::Cube(), boxMat);
    AddChild(backgroundBox);
    backgroundBox->Scale(40, Axis::XYZ);
    backgroundBox->SetPickable(false);
    backgroundBox->SetStatic();
    backgroundBox->CalculateBB();
 
    auto program = std::make_shared<Program>("shaders/phongShader");
    auto program1 = std::make_shared<Program>("shaders/pickingShader");
    
     snakeMaterial = {std::make_shared<Material>("snakeMaterial", program)}; // empty snakeMaterial
     prizeMaterial = {std::make_shared<Material>("prizeMaterial", program)}; // empty apple material
//    SetNamedObject(cube, Model::Create, Mesh::Cube(), snakeMaterial, shared_from_this());

    snakeMaterial->AddTexture(0, "textures/box0.bmp", 2);
    prizeMaterial->AddTexture(0, "textures/grass.bmp", 2); // TODO: change apple texture

    nodeMesh = {IglLoader::MeshFromFiles("snake", "data/snake2.obj")};
    snakeMesh = {IglLoader::MeshFromFiles("snake", "data/snake2.obj")};

    prizeMesh = {IglLoader::MeshFromFiles("prize" ,"data/ball.obj")}; // TODO: change apple mesh

    auto snakeRoot = NodeModel::Create("snake", nodeMesh, snakeMaterial);
    root->AddChild(snakeRoot);
    snakeRoot->AddChild(povCam);
    snakeRoot->AddChild(tpsCam);
    snakeRoot->Rotate(NINETY_DEGREES_IN_RADIANS, Axis::Y);
    snakeRoot->Translate(-10, Axis::Z);
    povCam->RotateInSystem(povCam->GetRotation(), std::numbers::pi, Axis::Y);
    tpsCam->RotateInSystem(tpsCam->GetRotation(), std::numbers::pi, Axis::Y);
    tpsCam->RotateInSystem(povCam->GetRotation(), std::numbers::pi / 6.0, Axis::Z);
    tpsCam->Translate({0, 1, -1});
    povCam->Translate({0, 0, 1});


    snakeNodes.push_back({snakeRoot, 0.0f});

    
    //Axis
    Eigen::MatrixXd vertices(6,3);
    vertices << -1,0,0,1,0,0,0,-1,0,0,1,0,0,0,-1,0,0,1;
    Eigen::MatrixXi faces(3,2);
    faces << 0,1,2,3,4,5;
    Eigen::MatrixXd vertexNormals = Eigen::MatrixXd::Ones(6,3);
    Eigen::MatrixXd textureCoords = Eigen::MatrixXd::Ones(6,2);
    std::shared_ptr<Mesh> coordsys = std::make_shared<Mesh>("coordsys",vertices,faces,vertexNormals,textureCoords);
    for(int i=0; i<16; i++) {
        AddToTail();
    }


    RegisterPeriodic(UPDATE_INTERVAL_MILLIS, [this]() {PeriodicFunction();});
}

void BasicScene::Update(const Program& program, const Eigen::Matrix4f& proj, const Eigen::Matrix4f& view, const Eigen::Matrix4f& model)
{
    mtx.lock();
    Scene::Update(program, proj, view, model);
    program.SetUniform4f("lightColor", 0.8f, 0.3f, 0.0f, 0.5f);
    program.SetUniform4f("Kai", 1.0f, 0.3f, 0.6f, 1.0f);
    program.SetUniform4f("Kdi", 0.5f, 0.5f, 0.0f, 1.0f);
    program.SetUniform1f("specular_exponent", 5.0f);
    program.SetUniform4f("light_position", 0.0, 15.0f, 0.0, 1.0f);
    mtx.unlock();
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
            case GLFW_KEY_TAB:
                SwitchCamera();
                break;
            case GLFW_KEY_ESCAPE:
                glfwSetWindowShouldClose(window, GLFW_TRUE);
                break;
            case GLFW_KEY_UP:
                TurnUp();
                break;
            case GLFW_KEY_DOWN:
                TurnDown();
                break;
            case GLFW_KEY_LEFT:
                TurnLeft();
                break;
            case GLFW_KEY_RIGHT:
                TurnRight();
                break;
            case GLFW_KEY_W:
                TurnUp();
                break;
            case GLFW_KEY_S:
                TurnDown();
                break;
            case GLFW_KEY_A:
                TurnLeft();
                break;
            case GLFW_KEY_D:
                TurnRight();
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


            case GLFW_KEY_Z:
                AddPrize();
                break;

        }
    }
}

#define MOVEMENT_DISTANCE 0.05f


void BasicScene::RemoveMoving(shared_ptr<MovingObject> moving) {
    std::erase_if(movingObjects,
                  [moving](std::shared_ptr<MovingObject> movingObj) {return movingObj == moving;});
    root->RemoveChild(moving->GetModel());
}


void BasicScene::DetectCollisions() {
    std::shared_ptr<NodeModel> head = snakeNodes.front().model;
    for(int i=2; i<snakeNodes.size(); i++) {
        std::shared_ptr<NodeModel> node = snakeNodes[i].model;
        if(ModelsCollide(head, node)) {
            std::cout << "Collusion With Tail!!!" << std::endl;
        }
    }

    for(const auto &prize : functionals::filter<MovingPtr>(movingObjects,
                                                    [](const MovingPtr &obj){return obj->IsPrize();})) {
        if(ModelsCollide(head, prize->GetModel())) {
            RemoveMoving(prize);
            ShortenSnake();
        }
    }

    if(!InBox(head)) {
        std::cout << "Head is not in box!" << std::endl;
    }
}

void BasicScene::PeriodicFunction() {
    mtx.lock();

    for(auto &node : snakeNodes) {
        Eigen::Vector3f translation = MOVEMENT_DISTANCE * node.model->GetRotation() * Eigen::Vector3f(0, 0, 1);
        node.model->Translate(translation);
    }

//    FollowHeadWithCamera();


    for (auto &node : movingObjects){
        node->MoveForward();
    }
    DetectCollisions();

    mtx.unlock();

}

BasicScene::~BasicScene() {
    for(auto &executor : executors) {
        executor.Stop();
    }
}

void BasicScene::TurnUp() {
//    if(cameraType == TOP_VIEW) {
//        snakeNodes[0].model->Rotate(SNAKE_TURN_ANGLE_RADIANS, Axis::Y);
//    } else {
        snakeNodes[0].model->Rotate(-SNAKE_TURN_ANGLE_RADIANS, Axis::X);
//    }
//    FollowHeadWithCamera();

}

void BasicScene::TurnDown() {
//    if(cameraType == TOP_VIEW) {
//        snakeNodes[0].model->Rotate(-SNAKE_TURN_ANGLE_RADIANS, Axis::Y);
//    } else {
        snakeNodes[0].model->Rotate(SNAKE_TURN_ANGLE_RADIANS, Axis::X);
//    }
//    FollowHeadWithCamera();

}

void BasicScene::TurnRight() {
//    headHeading -= NINETY_DEGREES_IN_RADIANS;
//    if(cameraType == TOP_VIEW) {
//        snakeNodes[0].model->Rotate(SNAKE_TURN_ANGLE_RADIANS, Axis::X);
//    } else {
        snakeNodes[0].model->Rotate(-SNAKE_TURN_ANGLE_RADIANS, Axis::Y);
//    }
//    FollowHeadWithCamera();
    snakeNodes[0].heading += SNAKE_TURN_ANGLE_RADIANS;
}

void BasicScene::TurnLeft() {
//    headHeading += NINETY_DEGREES_IN_RADIANS;
//    if(cameraType == TOP_VIEW) {
//        snakeNodes[0].model->Rotate(-SNAKE_TURN_ANGLE_RADIANS, Axis::X);
//    } else {
        snakeNodes[0].model->Rotate(SNAKE_TURN_ANGLE_RADIANS, Axis::Y);
//    }
//    FollowHeadWithCamera();

    snakeNodes[0].heading -= SNAKE_TURN_ANGLE_RADIANS;
}

void BasicScene::AddToTail() {

    auto newNode = NodeModel::Create("node", nodeMesh, snakeMaterial);
    std::shared_ptr<NodeModel> parent = snakeNodes.back().model;
    double heading = snakeNodes.back().heading;

    root->AddChild(newNode);
    newNode->Rotate(parent->GetRotation());
    newNode->Translate(parent->GetTranslation());


    float xTrans = cos(heading);
    float yTrans = sin(heading);

    Vec3 diag = parent->GetDiag();
//    newNode->Translate(NODE_LENGTH * Eigen::Vector3f(-xTrans, yTrans, 0));
    newNode->Translate( Eigen::Vector3f(-xTrans, yTrans, 0));

    snakeNodes.push_back({newNode, (float)heading});
}

void BasicScene::ShortenSnake() {
    auto lastNode = snakeNodes.back().model;
    snakeNodes.pop_back();
    root->RemoveChild(lastNode);
}


bool BasicScene::ModelsCollide(BoundablePtr m1, BoundablePtr m2) {
    Box b1 = m1->GetBoundingBox();
    Box b2 = m2->GetBoundingBox();

    return b1.intersects(b2);
}

void BasicScene::RegisterPeriodic(int interval, const std::function<void(void)>& func) {
    executors.emplace_back(interval, func);
    executors.back().Start();
}





Vector3f BasicScene::RandomSpawnPoint(){

    // roll signs
    float xSign = RollRandomAB(0,1) < 0.5 ? -1 : 1;
    float ySign = RollRandomAB(0.0,1) < 0.5 ? -1 : 1;

    // roll hor
    float x = xSign * RollRandomAB(0, HorizontalBorder);

    // roll ver
    float y = ySign * RollRandomAB(0, VerticalBorder);

    Vector3f spawnPoint(x, y, -10);
    return spawnPoint;
}


void BasicScene::AddPrize(){
    // create the model for the apple
    auto newModel = BallModel::Create("prize", prizeMesh, prizeMaterial);
    root->AddChild(newModel);
    newModel->Translate(RandomSpawnPoint());
    newModel->Scale(0.02f, Axis::XYZ);

    newModel->Rotate(NINETY_DEGREES_IN_RADIANS, Axis::Y);

    newModel->Rotate(RollRandomAB(0, (float)(2 * numbers::pi)), Axis::X);

    auto velocity = RollRandomAB(PrizeMinVelocity, PrizeMaxVelocity);

    MovingObject n(PRIZE, newModel, newModel->GetRotation() * Vector3f(0,0,1), velocity, root);

    movingObjects.push_back(make_shared<MovingObject>(n));

//    Node appleNode()
}

void BasicScene::SwitchCamera() {
    switch (cameraType) {
        case CameraType::TPS:
            cameraType = CameraType::POV;
            camera = povCam;
            break;
        case CameraType::POV:
            cameraType = CameraType::TOP_VIEW;
            camera = topViewCam;
            break;
        case CameraType::TOP_VIEW:
            cameraType = CameraType::TPS;
            camera = tpsCam;
            break;
    }
    viewport->camera = camera;
}

void BasicScene::FollowHeadWithCamera() {
    povCam->SetTransform(snakeNodes[0].model->GetTransform());
    povCam->RotateInSystem(povCam->GetRotation(), std::numbers::pi, Axis::Y);
    povCam->Translate({1, 1, 1});
}

void BasicScene::ViewportSizeCallback(cg3d::Viewport *_viewport) {
    topViewCam->SetProjection(float(_viewport->width) / float(_viewport->height));
    povCam->SetProjection(float(_viewport->width) / float(_viewport->height));
}

void BasicScene::AddViewportCallback(cg3d::Viewport *_viewport) {
    Scene::ViewportSizeCallback(_viewport);
    viewport = _viewport;

    Scene::AddViewportCallback(viewport);
}

bool BasicScene::InBox(const std::shared_ptr<BoundableModel>& model) {
    return backgroundBox->GetBoundingBox().contains(model->GetBoundingBox());
}
