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
    soundManager.InitManager();


    povCam = Camera::Create( "pov", 90.0f, float(width) / height, near, far);
    tpsCam = Camera::Create( "tps", 90.0f, float(width) / height, near, far);
    topViewCam = Camera::Create( "camera", fov, float(width) / height, near, far);

    topViewCam->Translate(35, Axis::Z);

    cameraType = CameraType::TPS;
    camera = tpsCam;

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
    prizeMaterial = {std::make_shared<Material>("prizeMaterial", program)}; // empty apple material

    snakeMaterial->AddTexture(0, "textures/box0.bmp", 2);
    prizeMaterial->AddTexture(0, "textures/grass.bmp", 2); // TODO: change apple texture

    snakeMesh = {IglLoader::MeshFromFiles("snake","data/snake2.obj")};
    prizeMesh = {IglLoader::MeshFromFiles("prize" ,"data/ball.obj")}; // TODO: change apple mesh

    auto snakeRoot = NodeModel::Create("snake", snakeMesh, snakeMaterial);
    root->AddChild(snakeRoot);
    snakeRoot->AddChild(povCam);
    snakeRoot->AddChild(tpsCam);
    snakeRoot->Rotate(NINETY_DEGREES_IN_RADIANS, Axis::Y);
    snakeRoot->Translate(-10, Axis::Z);
    povCam->RotateInSystem(povCam->GetRotation(), std::numbers::pi, Axis::Y);
    povCam->RotateInSystem(povCam->GetRotation(), std::numbers::pi / 6.0, Axis::Z);
    povCam->Translate({0, 1, -1});

    tpsCam->RotateInSystem(tpsCam->GetRotation(), std::numbers::pi, Axis::Y);
    tpsCam->RotateInSystem(povCam->GetRotation(), std::numbers::pi / 8.0, Axis::Z);
    tpsCam->Translate({0, 5, -7});

    Snake head(HEAD, snakeRoot, snakeRoot->GetRotation()*Eigen::Vector3f (0,0,1), nullptr, root, 0.0f);
    snakeNodes.push_back(make_shared<Snake>(head));


    //Axis
    Eigen::MatrixXd vertices(6,3);
    vertices << -1,0,0,1,0,0,0,-1,0,0,1,0,0,0,-1,0,0,1;
    Eigen::MatrixXi faces(3,2);
    faces << 0,1,2,3,4,5;
    Eigen::MatrixXd vertexNormals = Eigen::MatrixXd::Ones(6,3);
    Eigen::MatrixXd textureCoords = Eigen::MatrixXd::Ones(6,2);
    std::shared_ptr<Mesh> coordsys = std::make_shared<Mesh>("coordsys",vertices,faces,vertexNormals,textureCoords);

//    int numOfStartChildren = 1;
    int numOfStartChildren = 4;

    for(int i=0; i<numOfStartChildren; i++) {
        AddToTail(snakeNodes.back());
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
        SwitchCamera();

//        PickVisitor visitor;
//        visitor.Init();
//        renderer->RenderViewportAtPos(x, y, &visitor); // pick using fixed colors hack
//        auto modelAndDepth = visitor.PickAtPos(x, renderer->GetWindowHeight() - y);
//        renderer->RenderViewportAtPos(x, y); // draw again to avoid flickering
//        pickedModel = modelAndDepth.first ? std::dynamic_pointer_cast<Model>(modelAndDepth.first->shared_from_this()) : nullptr;
//        pickedModelDepth = modelAndDepth.second;
//        camera->GetRotation().transpose();
//        xAtPress = x;
//        yAtPress = y;
//
//        // if (pickedModel)
//        //     debug("found ", pickedModel->isPickable ? "pickable" : "non-pickable", " model at pos ", x, ", ", y, ": ",
//        //           pickedModel->name, ", depth: ", pickedModelDepth);
//        // else
//        //     debug("found nothing at pos ", x, ", ", y);
//
//        if (pickedModel && !pickedModel->isPickable)
//            pickedModel = nullptr; // for non-pickable models we need only pickedModelDepth for mouse movement calculations later
//
//        if (pickedModel)
//            pickedToutAtPress = pickedModel->GetTout();
//        else
//            cameraToutAtPress = camera->GetTout();
    }
}

void BasicScene::ScrollCallback(Viewport* viewport, int x, int y, int xoffset, int yoffset, bool dragging, int buttonState[])
{
    // note: there's a (small) chance the button state here precedes the mouse press/release event
//    Eigen::Matrix3f system = camera->GetRotation().transpose();
//    if (pickedModel) {
//        pickedModel->TranslateInSystem(system, {0, 0, -float(yoffset)});
//        pickedToutAtPress = pickedModel->GetTout();
//    } else {
//        camera->TranslateInSystem(system, {0, 0, -float(yoffset)});
//        cameraToutAtPress = camera->GetTout();
//    }
}

void BasicScene::CursorPosCallback(Viewport* viewport, int x, int y, bool dragging, int* buttonState)
{
//    if (dragging) {
//        Eigen::Matrix3f system = camera->GetRotation().transpose() * GetRotation();
//        auto moveCoeff = camera->CalcMoveCoeff(pickedModelDepth, viewport->width);
//        auto angleCoeff = camera->CalcAngleCoeff(viewport->width);
//        if (pickedModel) {
//            //pickedModel->SetTout(pickedToutAtPress);
//            if (buttonState[GLFW_MOUSE_BUTTON_RIGHT] != GLFW_RELEASE)
//                pickedModel->TranslateInSystem(system, {-float(xAtPress - x) / moveCoeff, float(yAtPress - y) / moveCoeff, 0});
//            if (buttonState[GLFW_MOUSE_BUTTON_MIDDLE] != GLFW_RELEASE)
//                pickedModel->RotateInSystem(system, float(xAtPress - x) / angleCoeff, Axis::Z);
//            if (buttonState[GLFW_MOUSE_BUTTON_LEFT] != GLFW_RELEASE) {
//                pickedModel->RotateInSystem(system, float(xAtPress - x) / angleCoeff, Axis::Y);
//                pickedModel->RotateInSystem(system, float(yAtPress - y) / angleCoeff, Axis::X);
//            }
//        } else {
//           // camera->SetTout(cameraToutAtPress);
//            if (buttonState[GLFW_MOUSE_BUTTON_RIGHT] != GLFW_RELEASE)
//                root->TranslateInSystem(system, {-float(xAtPress - x) / moveCoeff/10.0f, float( yAtPress - y) / moveCoeff/10.0f, 0});
//            if (buttonState[GLFW_MOUSE_BUTTON_MIDDLE] != GLFW_RELEASE)
//                root->RotateInSystem(system, float(x - xAtPress) / 180.0f, Axis::Z);
//            if (buttonState[GLFW_MOUSE_BUTTON_LEFT] != GLFW_RELEASE) {
//                root->RotateInSystem(system, float(x - xAtPress) / angleCoeff, Axis::Y);
//                root->RotateInSystem(system, float(y - yAtPress) / angleCoeff, Axis::X);
//            }
//        }
//        xAtPress =  x;
//        yAtPress =  y;
//    }
}

void BasicScene::KeyCallback(Viewport* viewport, int x, int y, int key, int scancode, int action, int mods)
{
    Eigen::Matrix3f system = camera->GetRotation().transpose();

    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        switch (key) // NOLINT(hicpp-multiway-paths-covered)
        {
            case GLFW_KEY_1:
                Hit();
                break;
            case GLFW_KEY_TAB:
                SwitchCamera();
                break;
            case GLFW_KEY_ESCAPE:
                glfwSetWindowShouldClose(window, GLFW_TRUE);
                break;
            case GLFW_KEY_UP: case GLFW_KEY_W:
                Turn(UP);
                break;
            case GLFW_KEY_DOWN: case GLFW_KEY_S:
                Turn(DOWN);
                break;
            case GLFW_KEY_LEFT: case GLFW_KEY_A:
                Turn(LEFT);
                break;
            case GLFW_KEY_RIGHT: case GLFW_KEY_D:
                Turn(RIGHT);
                break;
            case GLFW_KEY_SPACE:
                AddToTail(snakeNodes.back());
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
    std::shared_ptr<NodeModel> head = snakeNodes.front()->GetNodeModel();
    for(int i=2; i<snakeNodes.size(); i++) {
        std::shared_ptr<NodeModel> node = snakeNodes[i]->GetNodeModel();
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
}

void BasicScene::PeriodicFunction() {
    mtx.lock();

    int size = snakeNodes[0]->rotationQueue.size();
//    if (size  == MAX_QUEUE_SIZE - 1)
//        snakeNodes[0]->ClearQueue();
    cout<<size<<"\n"<<endl;

    shared_ptr<Snake> snake = snakeNodes[0];
    while (snake != nullptr){
        Rotate(snake);
        snake->MoveForward();
        snake = snake->child;
    }

//    for(auto &node : snakeNodes) {
//        Rotate(node);
//        node->MoveForward();
//    }

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

void BasicScene::Turn(MovementDirection type){
    int axis;
    float angle = SNAKE_TURN_ANGLE_RADIANS;
    switch (type){
        case RIGHT:
            axis = 1; // Y axis
            angle *= -1; // change angle
            snakeNodes[0]->heading += SNAKE_TURN_ANGLE_RADIANS;
            break;
        case LEFT:
            axis = 1; // Y axis
            angle *= 1; // dont change angle
            snakeNodes[0]->heading -= SNAKE_TURN_ANGLE_RADIANS;
            break;
        case UP:
            axis = 0; // X axis
            angle *= -1; // change angle
            break;
        case DOWN:
            axis = 0; // X axis
            angle *= 1; // dont change angle
            break;
    }

    auto posToRot = snakeNodes[0]->GetNodeModel()->GetTranslation();
    auto rotation = make_shared<pair<double, int>>(make_pair(angle, axis));

    auto newTurn = make_shared<pair<Eigen::Vector3f  , shared_ptr<pair<double, int>>>>
            (make_pair( snakeNodes[0]->GetNodeModel()->GetTranslation(), make_shared<pair<double, int>>(make_pair(angle, axis))));

    snakeNodes[0]->AddRotation(newTurn);

}

void BasicScene::Rotate(shared_ptr<Snake> snake) {

    // fix snake rotation queue

    shared_ptr<pair<double, int>> rotation = snake->Rotate();

//    // if shouldnt rotate, safe return
//    if (rotation.second == -1)
//        return;
//
    while(rotation != nullptr)
    {
        float angle = rotation->first;
        Axis turnAxis;
        switch (rotation->second){
            case 0: // if up or down
                turnAxis = Axis::X;
                break;
            case 1: // if right or left
                turnAxis = Axis::Y;
                break;
        }

        // else rotate by angle and turnAxis as in the needed turn
        snake->GetNodeModel()->Rotate(angle, turnAxis);
        auto x = "";
        rotation = snake->Rotate();
    }


}

void BasicScene::AddToTail(shared_ptr<Snake> parent) {

    auto newNode = NodeModel::Create("node", snakeMesh, snakeMaterial);
    shared_ptr<NodeModel> _parent = parent->GetNodeModel();
    double heading = parent->heading;

    root->AddChild(newNode);
    newNode->Rotate(_parent->GetRotation());
    newNode->Translate(_parent->GetTranslation());


    float xTrans = cos(heading);
    float yTrans = sin(heading);

    Vec3 diag = _parent->GetDiag();
//    newNode->Translate(NODE_LENGTH * Eigen::Vector3f(-xTrans, yTrans, 0));

    newNode->Translate( Eigen::Vector3f(-xTrans, yTrans, 0));

    Snake newSnake(TAIL, newNode, newNode->GetRotation() * Eigen::Vector3f (0,0,1), parent, root, (float)heading);
    // add as child of the previous snack (the back of snake list)
    auto add = make_shared<Snake>(newSnake);
    parent->AddChild(add);
    // add new snake to the list;
    snakeNodes.push_back(add);
    auto x = "after ";
}

void BasicScene::ShortenSnake() {
    auto lastNode = snakeNodes.back()->GetNodeModel();
    snakeNodes.back()->RemoveChild();
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





Eigen::Vector3f  BasicScene::RandomSpawnPoint(){

    // roll signs
    float xSign = RollRandomAB(0,1) < 0.5 ? -1 : 1;
    float ySign = RollRandomAB(0.0,1) < 0.5 ? -1 : 1;

    // roll hor
    float x = xSign * RollRandomAB(0, HorizontalBorder);

    // roll ver
    float y = ySign * RollRandomAB(0, VerticalBorder);

    Eigen::Vector3f  spawnPoint(x, y, -10);
    return spawnPoint;
}


void BasicScene::AddPrize(){
    // create the model for the apple
    auto newModel = BallModel::Create("prize", prizeMesh, prizeMaterial);
    root->AddChild(newModel);
//    newModel->Translate(RandomSpawnPoint());
    newModel->Translate(Vec3(0,0,-10));
    newModel->Scale(0.02f, Axis::XYZ);

    newModel->Rotate(NINETY_DEGREES_IN_RADIANS, Axis::Y);

    newModel->Rotate(RollRandomAB(0, (float)(2 * numbers::pi)), Axis::X);
    newModel->Rotate(RollRandomAB(0, (float)(2 * numbers::pi)), Axis::Y);
    newModel->Rotate(RollRandomAB(0, (float)(2 * numbers::pi)), Axis::Z);

    auto velocity = RollRandomAB(PrizeMinVelocity, PrizeMaxVelocity);

    MovingObject n(PRIZE, newModel, newModel->GetRotation() * Eigen::Vector3f (0,0,1), velocity, root);

    movingObjects.push_back(make_shared<MovingObject>(n));

//    Node appleNode()
}

void BasicScene::SwitchCamera() {
    switch (cameraType) {
        case CameraType::TPS:
//            cameraType = CameraType::POV;
//            camera = povCam;
            cameraType = CameraType::TOP_VIEW;
            camera = topViewCam;
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
    povCam->SetTransform(snakeNodes[0]->GetNodeModel()->GetTransform());
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


