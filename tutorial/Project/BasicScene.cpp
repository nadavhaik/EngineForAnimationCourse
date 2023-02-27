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
#include "SkinnedSnakeModel.h"


// #include "AutoMorphingModel.h"

using namespace cg3d;

#define FOG_ENABLED 1


void BasicScene::Init(float fov, int width, int height, float near, float far)
{


    // init gui
    gameCords = make_shared<MenuCords>(1671,68 ,120 ,124);
    menuCords = make_shared<MenuCords>(650,200 ,550 ,550);

    // init sound
    soundManager.InitManager();

    // cameras
    povCam = Camera::Create( "pov", 90.0f, float(width) / height, near, far);
    tpsCam = Camera::Create( "tps", 90.0f, float(width) / height, near, far);
    topViewCam = Camera::Create( "camera", 80.0f, float(width) / height, near, far);
    topViewCam->Translate(15, Axis::Z);

    // first camera
    cameraType = CameraType::TPS;
    camera = tpsCam;

    // create root
    AddChild(root = Movable::Create("root")); // a common (invisible) parent object for all the shapes

    // create daylight material
    auto daylight{std::make_shared<Material>("daylight", "shaders/cubemapShader")};
    daylight->AddTexture(0, "textures/cubemaps/Daylight Box_", 3);
    // create background cube for the daylight
    auto background{Model::Create("background", Mesh::Cube(), daylight)};
    AddChild(background);
    background->Scale(120, Axis::XYZ);
    background->SetPickable(false);
    background->SetStatic();

    // create background
    auto boxMat{std::make_shared<Material>("boxMat", "shaders/cubemapShader")};
    boxMat->AddTexture(0, "textures/cubemaps/box0_", 3);
    backgroundBox = ConstantBoundable::Create("backgroundBox", Mesh::Cube(), boxMat);
    AddChild(backgroundBox);
    backgroundBox->Scale(40, Axis::XYZ);
    backgroundBox->SetPickable(false);
    backgroundBox->SetStatic();
    backgroundBox->CalculateBB();


    auto snakeShader = std::make_shared<Program>("shaders/phongShader");
    auto prizeShader = std::make_shared<Program>("shaders/PrizeShader");
    auto bombShader = std::make_shared<Program>("shaders/BombShader");
    auto spawnerShader = std::make_shared<Program>("shaders/SpawnerShader");

 

    snakeMaterial = {std::make_shared<Material>("snakeMaterial", snakeShader)}; // empty snakeMaterial
    prizeMaterial = {std::make_shared<Material>("prizeMaterial", prizeShader)}; // empty apple material
    bombMaterial = {std::make_shared<Material>("bombMaterial", bombShader)}; // empty apple material
    spawnerMaterial = {std::make_shared<Material>("spawnerMaterial", spawnerShader)};

    // TODO: change textures
    snakeMaterial->AddTexture(0, "textures/box0.bmp", 2);
    prizeMaterial->AddTexture(0, "textures/grass.bmp", 2);
    bombMaterial->AddTexture(0, "textures/grass.bmp", 2);
    spawnerMaterial->AddTexture(0, "textures/grass.bmp", 2);

    // TODO: change meshes
    snakeMesh = {IglLoader::MeshFromFiles("snake","data/snake2.obj")};
    prizeMesh = {IglLoader::MeshFromFiles("prize" ,"data/ball.obj")};
    bombMesh = {IglLoader::MeshFromFiles("bomb" ,"data/ball.obj")};
    spawnerMesh = {IglLoader::MeshFromFiles("spawner" ,"data/cube_old.obj")};

    // create "spawner" at 0,0,0
    auto spawnerRoot = Model::Create("spawner", spawnerMesh, spawnerMaterial);
    background->Scale(1.5, Axis::XYZ);
    root->AddChild(spawnerRoot);
    spawnerRoot->Translate({0, 0, -10});

 

    auto snakeRoot = NodeModel::Create("snake", snakeMesh, snakeMaterial);
    root->AddChild(snakeRoot);
    snakeRoot->AddChild(povCam);
    snakeRoot->AddChild(tpsCam);
    snakeRoot->Rotate(NINETY_DEGREES_IN_RADIANS, Axis::Y);
    snakeRoot->Translate(snakeStartPo);
//    snakeRoot->Translate(-10, Axis::Z);
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
    faces << 0,1,2,3,4,50;
    Eigen::MatrixXd vertexNormals = Eigen::MatrixXd::Ones(6,3);
    Eigen::MatrixXd textureCoords = Eigen::MatrixXd::Ones(6,2);
    std::shared_ptr<Mesh> coordsys = std::make_shared<Mesh>("coordsys",vertices,faces,vertexNormals,textureCoords);

//    int numOfStartChildren = 1;
    int numOfStartChildren = 15;
    RecreateSnake(numOfStartChildren);

//    (std::string name, std::shared_ptr<cg3d::Mesh> mesh,
//            std::shared_ptr<cg3d::Material> material, std::vector<std::shared_ptr<Model>> joints);
    auto allNodes = functionals::map<shared_ptr<Snake>, ModelPtr>(snakeNodes, [](shared_ptr<Snake> node) {return node->GetNodeModel();});

    snakeSkin = SkinnedSnakeModel::Create("Snake skin", snakeMesh, snakeMaterial, allNodes);

    snakeSkin->Skin();
    snakeSkin->Scale(3.5, Axis::Z);
    snakeSkin->Rotate(NINETY_DEGREES_IN_RADIANS, Axis::Y);
    snakeSkin->Translate(-10, Axis::Z);
    snakeSkin->Translate(-NODE_LENGTH * 3, Axis::X);
//    snakeRoot->AddChild(snakeSkin);
//    root->AddChild(snakeSkin);


    startingHealth = snakeNodes.size();


    RegisterPeriodic(UPDATE_INTERVAL_MILLIS, [this]() {PeriodicFunction();});

}

void BasicScene::ResetSnake(){
    int lastHealth = snakeNodes.size() - 1;
    // delete old model
    auto oldModel = snakeNodes[0]->GetNodeModel();
    root->RemoveChild(oldModel);
    oldModel->RemoveChild(povCam);
    oldModel->RemoveChild(tpsCam);
    // delete all snakes afterwards
    for(int i = 0; i < snakeNodes.size(); i++)
        root->RemoveChild(snakeNodes[i]->GetNodeModel());
    snakeNodes.clear();

    // create new model
    auto program = std::make_shared<Program>("shaders/phongShader");
    auto program1 = std::make_shared<Program>("shaders/pickingShader");
    auto snakeRoot = NodeModel::Create("snake", snakeMesh, snakeMaterial);
    root->AddChild(snakeRoot);
    snakeRoot->AddChild(povCam);
    snakeRoot->AddChild(tpsCam);
    snakeRoot->Rotate(NINETY_DEGREES_IN_RADIANS, Axis::Y);
    snakeRoot->Translate(snakeStartPo);

    Snake head(HEAD, snakeRoot, snakeRoot->GetRotation()*Eigen::Vector3f (0,0,1), nullptr, root, 0.0f);
    snakeNodes.push_back(make_shared<Snake>(head));

    RecreateSnake(lastHealth);
}
void BasicScene::RecreateSnake(int numOfStartChildren){

    for(int i=0; i<numOfStartChildren; i++) {
        AddToTail(snakeNodes.back());
    }

}

void BasicScene::Update(const Program& program, const Eigen::Matrix4f& proj, const Eigen::Matrix4f& view, const Eigen::Matrix4f& model)
{
    /**
     *   if (node->IsBomb())
        {
            programCopy.SetUniform4f("color", 0.8f, 0.3f, 0.0f, 0.5f);
        }
     */

    mtx.lock();
    Scene::Update(program, proj, view, model);
    program.SetUniform4f("lightColor", 0.80078125f, 0.51953125f, 0.24609375f, 1.0f);
    program.SetUniform4f("Kai", 1.0f, 0.3f, 0.6f, 1.0f);
    program.SetUniform4f("Kdi", 0.5f, 0.5f, 0.0f, 1.0f);
    program.SetUniform1f("specular_exponent", 5.0f);
    Vec3 cameraPos = camera->GetTranslation();
    program.SetUniform3f("cameraPos", cameraPos.x(), cameraPos.y(), cameraPos.z());
    program.SetUniform1i("fog_enabled", FOG_ENABLED);
    program.SetUniform4f("light_position", 0.0, 15.0f, 0.0, 1.0f);

    // if its a MovingObject, find its moving object


    mtx.unlock();
//    cyl->Rotate(0.001f, Axis::Y);
//    snakeNodes[0]->Translate(0.01f, Axis::Y);
}


void BasicScene::MouseCallback(Viewport* viewport, int x, int y, int button, int action, int mods, int buttonState[])
{
    // note: there's a (small) chance the button state here precedes the mouse press/release event
    auto currentMenuCords = GetCords();
    auto xDis = currentMenuCords->x_pos + currentMenuCords->side_bor;
    auto yDis = currentMenuCords->y_pos + currentMenuCords->up_down_bor;
    if (action == GLFW_PRESS) { // default mouse button press behavior
        if (x <= xDis && y<= yDis)
            return;

        SwitchCamera();
    }
}

void BasicScene::ScrollCallback(Viewport* viewport, int x, int y, int xoffset, int yoffset, bool dragging, int buttonState[])
{
}

void BasicScene::CursorPosCallback(Viewport* viewport, int x, int y, bool dragging, int* buttonState)
{
}

void BasicScene::KeyCallback(Viewport* viewport, int x, int y, int key, int scancode, int action, int mods)
{
    Eigen::Matrix3f system = camera->GetRotation().transpose();

    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        if (key == GLFW_KEY_K)
        {
            Die();
            return;
        }
        if (key == GLFW_KEY_L)
        {
            Win();
            return;
        }
        // key checks even in paused game
        switch (key) // NOLINT(hicpp-multiway-paths-covered)
        {
            // swtich cam with TAB anywhere in the game
            case GLFW_KEY_TAB:
                SwitchCamera();
                break;
            // exit the game with ESCAPE
            case GLFW_KEY_ESCAPE:
                switch(menuType){
                    case GAME:
                        menuType = PAUSE;
                        break;
                    case PAUSE:
                        menuType = GAME;
                        break;
                    default:
                        break;
                }
                break;
        }
        // checks if and only if game isnt paused
        if (menuType != GAME)
            return;

        switch (key) // NOLINT(hicpp-multiway-paths-covered)
        {
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

            case GLFW_KEY_1:
//                if( pickedIndex > 0)
//                  pickedIndex--;
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
                AddToTail(snakeNodes.back());
                break;

            case GLFW_KEY_B:
                AddPrizeBezier();
                break;
            case GLFW_KEY_Z:
                AddPrizeLinear();
                break;

        }
    }
}



void BasicScene::RemoveMoving(shared_ptr<MovingObject> moving) {

    std::vector<std::shared_ptr<MovingObject>> newMovings;
    std::copy_if(movingObjects.begin(), movingObjects.end(), std::back_inserter(newMovings),
                 [moving](std::shared_ptr<MovingObject> other) {return moving != other;});

    movingObjects = newMovings;
    moving->GetModel()->Translate({100, 100, 100});
    moving->velocity = 0;
//    root->RemoveChild(moving->GetModel());

}


void BasicScene::DetectCollisions() {
    std::shared_ptr<NodeModel> head = snakeNodes.front()->GetNodeModel();
    for(int i=2; i<snakeNodes.size(); i++) {
        std::shared_ptr<NodeModel> node = snakeNodes[i]->GetNodeModel();
        if(ModelsCollide(head, node)) {
            Hit();
        }
    }

    for(const auto &object : functionals::filter<MovingPtr>(movingObjects,
                                                    [](const MovingPtr &obj){return !obj->IsSnake();})) {
        if(ModelsCollide(head, object->GetModel())) {
            if (object->IsPrize()){
                EatPrize(object);
            }
            else if (object->IsBomb()){
                Hit(object);
            }
        }
    }

    if(!InBox(head)) {
        std::cerr << "Head is not in box!" << std::endl;
        exit(-1);
    }
}

void BasicScene::PeriodicFunction() {
    if (menuType != GAME)
        return;
    if (score >= ScoreToPass()){
        Win();
        return;
    }
    mtx.lock();

    int size = snakeNodes[0]->rotationsQueue.size();
//    if (size  == MAX_QUEUE_SIZE - 1)
//        snakeNodes[0]->ClearQueue();
//    cout<<size<<"\n"<<endl;

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
        if (node == nullptr) continue;
        node->MoveForward();
    }
    DetectCollisions();
    snakeSkin->Skin();

    mtx.unlock();


}

BasicScene::~BasicScene() {
    for(auto &executor : executors) {
        executor.Stop();
    }
}

void BasicScene::Turn(MovementDirection type){
    Axis axis;
    float angle = SNAKE_TURN_ANGLE_RADIANS;
    switch (type){
        case RIGHT:
            axis = Axis::Y; // Y axis
            angle *= -1; // change angle
            snakeNodes[0]->heading += SNAKE_TURN_ANGLE_RADIANS;
            break;
        case LEFT:
            axis = Axis::Y; // Y axis
            angle *= 1; // dont change angle
            snakeNodes[0]->heading -= SNAKE_TURN_ANGLE_RADIANS;
            break;
        case UP:
            axis = Axis::X; // X axis
            angle *= -1; // change angle
            break;
        case DOWN:
            axis = Axis::X; // X axis
            angle *= 1; // dont change angle
            break;
    }

    auto posToRot = snakeNodes[0]->GetNodeModel()->GetTranslation();
    auto rotation = std::make_shared<RotationCommand>(axis, angle, Vec3::Zero());
    snakeNodes[0]->AddRotation(rotation);

}

bool AlmostEqual(Mat3x3 m1, Mat3x3 m2) {
    float maxDelta = 100 * std::numeric_limits<float>::min();
    for(int i=0; i<3; i++) {
        for(int j=0; j<3; j++) {
            if(std::abs(m1(i, j) - m2(i, j)) > maxDelta) {
                return false;
            }
        }
    }
    return true;
}

void BasicScene::Rotate(shared_ptr<Snake> snake) {


    auto rotation = snake->Rotate();

//    // if shouldnt rotate, safe return
//    if (rotation.second == -1)
//        return;
//
    while(rotation != nullptr)
    {
        float angle = rotation->angle;
        Axis turnAxis = rotation->axis;

        // else rotate by angle and turnAxis as in the needed turn
        snake->GetNodeModel()->Rotate(angle, turnAxis);
        snake->invisibleBrother->Rotate(angle, turnAxis);

        float distance = algebra::distance(snake->GetNodeModel()->GetTranslation(), snake->invisibleBrother->GetTranslation());


        auto x = "";
        auto prevRot = rotation;
        rotation = snake->Rotate();

//         fine tuning for tails:
        if(rotation == nullptr && snake->IsTail() && !snake->parent->InRotation() &&
                AlmostEqual(snake->GetNodeModel()->GetRotation(), snake->parent->GetNodeModel()->GetRotation())) {
//            continue;
//                snake->GetNodeModel()->GetRotation().isApprox(snake->parent->GetNodeModel()->GetRotation())) {

            Vec3 delta = snake->parent->GetNodeModel()->GetTranslation() - snake->GetNodeModel()->GetTranslation() - snake->parent->GetNodeModel()->GetRotation() * Eigen::Vector3f(0, 0, 1);
            Vec3 deltaInSystem = snake->parent->GetNodeModel()->GetRotation().inverse() * delta;

            Vec3 fixedDelta = snake->parent->GetNodeModel()->GetRotation() * Vec3(deltaInSystem.x(), deltaInSystem.y(), 0);

            std::cout << "delta: (" << delta.x() << "," << delta.y() << ","
                      << delta.z() << ")" << std::endl;
            std::cout << "delta in system: (" << deltaInSystem.x() << "," << deltaInSystem.y() << ","
                << deltaInSystem.z() << ")" << std::endl;

            float deviation = std::abs(1.0f - algebra::abs({deltaInSystem.x(), deltaInSystem.y(), 0}));
            if(deviation > 0.01) return;

            snake->GetNodeModel()->Translate(fixedDelta);
//            snake->GetNodeModel()->TranslateInSystem(snake->GetNodeModel()->GetRotation(), {deltaInSystem.x(), deltaInSystem.y(), deltaInSystem.z()});
//            snake->GetNodeModel()->Translate({delta.x(), delta.y(), delta.z()});

//            Vec3 parentPos = snake->parent->GetNodeModel()->GetTranslation();
//            Vec3 deltaFromParent = parentPos - snake->GetNodeModel()->GetTranslation();

//            float distanceFromParent = algebra::distance(parentPos, deltaFromParent);
//            snake->GetNodeModel()->TranslateInSystem(snake->GetNodeModel()->GetRotation(),
//                                                     {0, 0, 1.0f - distanceFromParent});
//            snake->GetNodeModel()->SetCenter(snake->GetNodeModel()->GetRotation() * ((1.0f - distanceFromParent) * deltaFromParent));
//            snake->GetNodeModel()->Translate({0, 0, 1.0f - distanceFromParent});
        }
    }


}

void BasicScene::AddToTail(shared_ptr<Snake> parent) {

    auto newNode = NodeModel::Create("node", snakeMesh, snakeMaterial);
    shared_ptr<NodeModel> _parent = parent->GetNodeModel();
    double heading = parent->heading;

    root->AddChild(newNode);
    newNode->Rotate(_parent->GetRotation());
    newNode->Translate(_parent->GetTranslation() - _parent->GetRotation() * Vec3(0, 0, 1));

    Vec3 diag = _parent->GetDiag();

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


void BasicScene::AddPrizeLinear(){
    // create the model for the apple
    auto newModel = BallModel::Create("prize", prizeMesh, prizeMaterial);
    auto axis = RandomAxis();
    float angle = RollRandomAB(0, 2 * std::numbers::pi);

    newModel->Rotate(angle, axis);

    auto velocity = RollRandomAB(PrizeMinVelocity, PrizeMaxVelocity);

    MovingObject n(PRIZE, newModel, newModel->GetRotation() * Eigen::Vector3f (0,0,1), velocity, root);

    root->AddChild(newModel);

    movingObjects.push_back(make_shared<MovingObject>(n));

//    newModel->Translate(RandomSpawnPoint());
    newModel->Translate(Vec3(0,0,-10));
    newModel->Scale(0.02f, Axis::XYZ);

    newModel->Rotate(NINETY_DEGREES_IN_RADIANS, Axis::Y);

    newModel->Rotate(RollRandomAB(0, (float)(2 * numbers::pi)), Axis::X);
    newModel->Rotate(RollRandomAB(0, (float)(2 * numbers::pi)), Axis::Y);
    newModel->Rotate(RollRandomAB(0, (float)(2 * numbers::pi)), Axis::Z);



//    Node appleNode()
}

Vec3 BasicScene::RandomPointInBox() {
    Vec3 min = backgroundBox->GetBoundingBox().min();
    Vec3 max = backgroundBox->GetBoundingBox().max();

    return {RollRandomAB(min.x(), max.x()),
            RollRandomAB(min.y(), max.y()),
            RollRandomAB(min.z(), max.z())};
}

void BasicScene::AddPrizeBezier() {
    auto newModel = BallModel::Create("prize", bombMesh, bombMaterial);
    root->AddChild(newModel);
    newModel->Translate(Vec3(0,0,-10));
    newModel->Scale(0.02f, Axis::XYZ);
    Vec3 p0 = newModel->GetTranslation();
    Vec3 p1 = RandomPointInBox();
    Vec3 p2 = RandomPointInBox();

    movingObjects.push_back(make_shared<BezierMoving>(BOMB, newModel, root, CubicBezier(p0, p1, p2)));
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





void BasicScene::BuildImGui()
{
    bool drawPlayerStats = true;
    switch (menuType) {
        case MAIN:
            menuType = DrawMainMenu();
            drawPlayerStats = false;
            break;
        case BETWEEN:
            menuType = DrawBetweenMenu();
            break;
        case GAME:
            menuType = DrawGameMenu();
            break;
        case PAUSE:
            menuType = DrawPauseMenu();
            break;
        case DEATH:
            menuType = DrawDeathMenu();
            break;
        case WIN:
            menuType = DrawWinMenu();
            break;
        default:
            menuType = DrawMainMenu();
            break;

    };
    if (drawPlayerStats)
        DrawPlayerStats();


    //          SIDE DEBUG MENU FOR MENUS
    ImGui::Begin("Debug Menus", nullptr, ImGuiWindowFlags_None );
    ImGui::SetWindowPos(ImVec2(0,0), ImGuiCond_Always);
    ImGui::SetWindowSize(ImVec2(0,0), ImGuiCond_Always);

    if (ImGui::Button ("Main Menu")){menuType = MAIN;}
    if (ImGui::Button ("Game Menu")){menuType = GAME;}
    if (ImGui::Button ("Pause Menu")){menuType = PAUSE;}
    if (ImGui::Button ("Death Menu")){menuType = DEATH;}
    if (ImGui::Button ("Win Menu")){menuType = WIN;}
    ImGui::End();
}

#define NOT_YET_IMPLEMENTED cout<<"'Moshe Not Yet Implemented!!\n"<<endl
#define LABEL_SIZE 2.4
#define BUTTON_SIZE 1.7

/**
 * drawing the game menu. returns the needed next menu type (game mode)
 */
MenuType BasicScene::DrawMainMenu() {
    MenuType ans = MAIN;
    shared_ptr<MenuCords> cords = GetCords();
    int flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;
    bool* pOpen = nullptr;

    // CHANGE
    ImGui::Begin("Welcome! First time here?", pOpen, flags);
    ImGui::SetWindowFontScale(LABEL_SIZE);
    ImGui::Text("\n\n\t\t\tSnake 3D\n\tMade by Mattan and Nadav");
    ImGui::Text("\n\n\n\n");
    ImGui::SetWindowFontScale(BUTTON_SIZE);
    ImGui::SetWindowPos(ImVec2(cords->x_pos, cords->y_pos), ImGuiCond_Always);
    ImGui::SetWindowSize(ImVec2(cords->side_bor, cords->up_down_bor), ImGuiCond_Always);


    if (ImGui::Button ("\t\t\t\tPlay the Game\t\t\t\t\t")){ans = BETWEEN; soundManager.PlayButtonSoundEffect();}
    ImGui::Text("\n\n");
    if (ImGui::Button ("\t\t\t\t  Mute Sound\t\t\t\t\t  ")){Mute(); soundManager.PlayButtonSoundEffect();}
    ImGui::Text("\n\n");
    if (ImGui::Button ("\t\t\t\t\tQuit\t\t\t\t\t\t")){soundManager.PlayButtonSoundEffect(); glfwSetWindowShouldClose(window, GLFW_TRUE);}
    ImGui::End();
    return ans;
}

/**
 *
 * @return the next game mode
 */
MenuType BasicScene::DrawBetweenMenu() {
    MenuType ans =  BETWEEN;
    shared_ptr<MenuCords> cords = GetCords();
    int flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;
    bool* pOpen = nullptr;
    string levelMenu = "Level " + std::to_string(currentLevel) + " " + (currentLevel >= 4 ? "holy shit!" : "");
    ImGui::Begin(levelMenu.c_str(), pOpen, flags);
    ImGui::SetWindowFontScale(LABEL_SIZE);
    ImGui::Text(("\n \t\t\tLevel " + std::to_string(currentLevel)).c_str());
    ImGui::Text(("\n\n\tYou need to get " + std::to_string(ScoreToPass()) + " prizes.").c_str());
    ImGui::Text("\n\t\t\tGood luck!");
    ImGui::Text("\n\n");
    ImGui::SetWindowFontScale(BUTTON_SIZE);
    ImGui::SetWindowPos(ImVec2(cords->x_pos, cords->y_pos), ImGuiCond_Always);
    ImGui::SetWindowSize(ImVec2(cords->side_bor, cords->up_down_bor), ImGuiCond_Always);

    if (ImGui::Button ("\t\t\t\t   Start\t\t\t\t\t\t")){ans = GAME; soundManager.PlayButtonSoundEffect();}
    ImGui::Text("\n\n");
    if (ImGui::Button ("\t\t\t\t  Main Menu\t\t\t\t\t\t")){ans = MAIN; soundManager.PlayButtonSoundEffect(); RestartGame();}
    ImGui::End();
    return ans;
}
/**
 *
 * @return
 */
MenuType BasicScene::DrawGameMenu() {
    MenuType ans = GAME;
    shared_ptr<MenuCords> cords = GetCords();

    int flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;
    bool* pOpen = nullptr;

    ImGui::Begin("Menu", pOpen, flags);
    ImGui::SetWindowPos(ImVec2(cords->x_pos, cords->y_pos), ImGuiCond_Always);
    ImGui::SetWindowSize(ImVec2(cords->side_bor, cords->up_down_bor), ImGuiCond_Always);

    ImGui::SetWindowFontScale(0.5);
    ImGui::Text("");
    ImGui::SetWindowFontScale(6.5);
    if (ImGui::Button ("||")){ans = PAUSE; soundManager.PlayButtonSoundEffect();}
    ImGui::End();

    return ans;
}
/**
 *
 * @return the next game mode
 */
MenuType BasicScene::DrawPauseMenu() {
    MenuType ans =  PAUSE;
    shared_ptr<MenuCords> cords = GetCords();
    int flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;
    bool* pOpen = nullptr;

    ImGui::Begin("Pause Menu", pOpen, flags);
    ImGui::SetWindowFontScale(LABEL_SIZE );
    ImGui::Text(("\n\n\t\"Pee Break\" Menu! Level " + std::to_string(currentLevel)).c_str());
    ImGui::Text(("  " + std::to_string(score) + " out of " + std::to_string(ScoreToPass()) + " prizes collected.").c_str());
    ImGui::Text(("  " + std::to_string(ScoreToPass() - score) + " needed for the next level.\n").c_str());
    ImGui::Text("\n\n");
    ImGui::SetWindowFontScale(BUTTON_SIZE);
    ImGui::SetWindowPos(ImVec2(cords->x_pos, cords->y_pos), ImGuiCond_Always);
    ImGui::SetWindowSize(ImVec2(cords->side_bor, cords->up_down_bor), ImGuiCond_Always);

    if (ImGui::Button ("\t\t\t\t   Resume\t\t\t\t\t\t")){ans = GAME; soundManager.PlayButtonSoundEffect();}
    ImGui::Text("\n\n");
    if (ImGui::Button ("\t\t\t\t Mute Sound\t\t\t\t\t  ")){Mute(); soundManager.PlayButtonSoundEffect();}
    ImGui::Text("\n\n");
    if (ImGui::Button ("\t\t\t\t  Main Menu\t\t\t\t\t\t")){ans = MAIN; soundManager.PlayButtonSoundEffect(); RestartGame();}
    ImGui::End();
    return ans;
}

MenuType BasicScene::DrawDeathMenu() {
    MenuType ans = DEATH;
    shared_ptr<MenuCords> cords = GetCords();
    int flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;
    bool* pOpen = nullptr;

    // CHANGE
    ImGui::Begin(/* TODO : if score needed to pass is 1/5 out of needed show so close, else show you were trying*/"Oh no! You were so close!", pOpen, flags);

    ImGui::SetWindowFontScale(LABEL_SIZE);
    ImGui::Text("\n\n\n  \t\tNooo you died!");
    ImGui::Text(("  \t You got " + std::to_string(score) + " out of " + std::to_string(ScoreToPass())).c_str());
    ImGui::Text(("  \t\tin level " + std::to_string(currentLevel)).c_str());
    ImGui::Text("\n\n\n");
    ImGui::SetWindowFontScale(BUTTON_SIZE);
    ImGui::SetWindowPos(ImVec2(cords->x_pos, cords->y_pos), ImGuiCond_Always);
    ImGui::SetWindowSize(ImVec2(cords->side_bor, cords->up_down_bor), ImGuiCond_Always);

    if (ImGui::Button ("\t\t\t\t  Restart Game\t\t\t\t\t\t")){ans = BETWEEN; soundManager.PlayButtonSoundEffect(); RestartGame();}
    ImGui::Text("\n\n");
    if (ImGui::Button ("\t\t\t\t  Main Menu\t\t\t\t\t\t")){ans = MAIN; soundManager.PlayButtonSoundEffect(); RestartGame();}
    ImGui::Text("\n\n");
    if (ImGui::Button ("\t\t\t\t\tQuit\t\t\t\t\t\t")){soundManager.PlayButtonSoundEffect(); glfwSetWindowShouldClose(window, GLFW_TRUE);}

    ImGui::End();
    return ans;
}

MenuType BasicScene::DrawWinMenu() {
    MenuType ans = WIN;
    shared_ptr<MenuCords> cords = GetCords();
    int flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;
    bool* pOpen = nullptr;

    string text;
    // CHANGE
    ImGui::Begin("Congrats! You Won!", pOpen, flags);

    ImGui::SetWindowFontScale(LABEL_SIZE);
    ImGui::Text("\n\n\n   \tYou did amazing!");
    ImGui::Text(("  \t  You beat level " + std::to_string(currentLevel)).c_str());
    ImGui::Text(("  \t  by eating " + std::to_string(ScoreToPass()) + " prizes!").c_str());
    ImGui::Text("\n\n\n");
    ImGui::SetWindowFontScale(BUTTON_SIZE);
    ImGui::SetWindowPos(ImVec2(cords->x_pos, cords->y_pos), ImGuiCond_Always);
    ImGui::SetWindowSize(ImVec2(cords->side_bor, cords->up_down_bor), ImGuiCond_Always);

    if (ImGui::Button ("\t\t\t\tNext Level\t\t\t\t\t\t")){ans = BETWEEN; soundManager.PlayButtonSoundEffect(); NextLevel();}
    ImGui::Text("\n\n");
    if (ImGui::Button ("\t\t\t\t Main Menu\t\t\t\t\t\t")){ans = MAIN; soundManager.PlayButtonSoundEffect(); RestartGame();}
    ImGui::Text("\n\n");
    if (ImGui::Button ("\t\t\t\t\tQuit\t\t\t\t\t\t")){soundManager.PlayButtonSoundEffect(); glfwSetWindowShouldClose(window, GLFW_TRUE);}

    ImGui::End();
    return ans;
}

void BasicScene::DrawPlayerStats() {
    int flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;
    bool* pOpen = nullptr;

    ImGui::Begin("Player Stats", pOpen, flags);
    ImGui::SetWindowPos(ImVec2(721, 68), ImGuiCond_Always);
    ImGui::SetWindowSize(ImVec2(474, 50), ImGuiCond_Always);

    ImGui::SetWindowFontScale(2);
    string healthText = "Health : " + std::to_string(snakeNodes.size() - 1);
    string scoreText = "Score : " + std::to_string(score) + " / " + std::to_string(ScoreToPass());
    string text = healthText + "\t|\t" + scoreText;
    ImGui::Text(text.c_str());

    ImGui::End();
}

/**
*
 * relocate and move menus
 *  shared_ptr<MenuCords> cords = GetCords();
    int flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;
    bool* pOpen = nullptr;

    // CHANGE
    ImGui::Begin("Win Menu", pOpen, flags);
    ImGui::SetWindowPos(ImVec2(cords->x_pos, cords->y_pos), ImGuiCond_Always);
    ImGui::SetWindowSize(ImVec2(cords->side_bor, cords->up_down_bor), ImGuiCond_Always);

      if (ImGui::Button ("<-")) {cords->x_pos -= 50;}
    if (ImGui::Button ("->")) {cords->x_pos+= 50;}
    if (ImGui::Button ("/|\\")) {cords->y_pos+= 50;}
    if (ImGui::Button ("\\|/")) {cords->y_pos-= 50;}
    if (ImGui::Button ("<--->")) {cords->side_bor+= 50;}
    if (ImGui::Button (">-<")) {cords->side_bor-= 50;}
    if (ImGui::Button ("-\n\n-")) {cords->up_down_bor+= 50;}
    if (ImGui::Button ("-\n-")) {cords->up_down_bor-= 50;}
    // CHANGE
    if (ImGui::Button ("Print Cords.")) {
        cout << "winCords = make_shared<MenuCords>(" << cords->x_pos << "," << cords->y_pos << " ," << cords->up_down_bor << " ," << cords->side_bor << ");\n" << endl;
    }
    if (ImGui::Button ("Main Menu")){ans = MAIN;}
    if (ImGui::Button ("Game Menu")){ans = GAME;}
    if (ImGui::Button ("Pause Menu")){ans = PAUSE;}
    if (ImGui::Button ("Death Menu")){ans = DEATH;}
    if (ImGui::Button ("Win Menu")){ans = WIN;}
    ImGui::End();
*/


bool BasicScene::InBox(const std::shared_ptr<BoundableModel>& model) {
    return backgroundBox->GetBoundingBox().contains(model->GetBoundingBox());
}

Axis BasicScene::RandomAxis() {
    float t = RollRandomAB(0, 3);
    if(0 <= t && t < 1) {
        return Axis::X;
    } else if(1 <= t && t < 2) {
        return Axis::Y;
    }

    return Axis::Z;
}

