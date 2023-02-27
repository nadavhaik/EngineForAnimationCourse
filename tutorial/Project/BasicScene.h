#pragma once

#include "PeriodicExecutor.h"
#include "types_macros.h"
#include <memory>
#include <utility>
#include "BoundableModel.h"
#include "MovingObject.h"
#include "SoundManager.h"
#include <numbers>
#include "functionals.h"
#include "mutex"
#include "SceneWithImGui.h"
#include "imgui.h"
#include "SkinnedSnakeModel.h"

#define PrizeMaxVelocity 8.0f
#define PrizeMinVelocity 2.0f


#define UPDATE_INTERVAL_MILLIS 20
#define COLLUSION_DETECTION_INTERVAL_MILLIS 500

#define NINETY_DEGREES_IN_RADIANS 1.57079633f
#define SNAKE_TURN_ANGLE_RADIANS 0.1f

#define MOVEMENT_DISTANCE 0.10f


#include "algebra.h"


enum MovementDirection {RIGHT, LEFT, UP, DOWN};
enum MovementType {STRAIGHT, TURN};
enum CameraType {POV, TPS, TOP_VIEW};

using namespace Eigen;

struct SnakeNode {
    std::shared_ptr<NodeModel> model;
    float heading{};
};

enum MenuType {MAIN, GAME, PAUSE, DEATH, WIN};
struct MenuCords{
    MenuCords(float x, float y, float udb, float sb): x_pos(x), y_pos(y), up_down_bor(udb), side_bor(sb){}
    float x_pos;
    float y_pos;
    float up_down_bor;
    float side_bor;
};

class BasicScene : public cg3d::SceneWithImGui
{
public:
    explicit BasicScene(std::string name, cg3d::Display* display) : SceneWithImGui(std::move(name), display) {};
    void BuildImGui() override;
    ~BasicScene() override;
    void Init(float fov, int width, int height, float near, float far);
    void Update(const cg3d::Program& program, const Eigen::Matrix4f& proj, const Eigen::Matrix4f& view, const Eigen::Matrix4f& model) override;
    void MouseCallback(cg3d::Viewport* viewport, int x, int y, int button, int action, int mods, int buttonState[]) override;
    void ScrollCallback(cg3d::Viewport* viewport, int x, int y, int xoffset, int yoffset, bool dragging, int buttonState[]) override;
    void CursorPosCallback(cg3d::Viewport* viewport, int x, int y, bool dragging, int* buttonState)  override;
    void KeyCallback(cg3d::Viewport* viewport, int x, int y, int key, int scancode, int action, int mods) override;
    void PeriodicFunction();
    void SwitchCamera();
    void AddToTail(shared_ptr<Snake> parent);
    void ShortenSnake();
    static bool ModelsCollide(BoundablePtr m1, BoundablePtr m2);
    void DetectCollisions();
    void RegisterPeriodic(int interval, const std::function<void(void)>& func);
    void AddViewportCallback(cg3d::Viewport* _viewport) override;
    void ViewportSizeCallback(cg3d::Viewport* _viewport) override;
    void Rotate(shared_ptr<Snake> snake);

    Eigen::Vector3f RandomSpawnPoint();
    Axis RandomAxis();
    void AddPrize();
    Vec3 RandomPointInBox();
    void AddPrizeLinear();
    void AddPrizeBezier();

    float RollRandomAB(float min, float max){return min + static_cast <float> (rand()) /( static_cast <float> (RAND_MAX/(max - min)));}
    std::shared_ptr<Movable> root;
    void RemoveMoving(shared_ptr<MovingObject> moving);
    void FollowHeadWithCamera();
    bool InBox(const BoundablePtr &model);

    SoundManager soundManager;



private:

    void Hit(std::shared_ptr<MovingObject> object){
        RemoveMoving(object);
        soundManager.PlayHitSoundEffect();
        ShortenSnake();
        // TODO
        //        ShortenSnake();
    }
    void EatPrize(std::shared_ptr<MovingObject> object){
        RemoveMoving(object);
        soundManager.PlayPrizeSoundEffect();
        score++;
        // TODO
    }
    void Win(){
        soundManager.PlayWinSoundEffect();
        // TODO
    }
    void Die(){
        soundManager.PlayLoseSoundEffect();
        // TODO
    }
    void ButtonPress(){
        soundManager.PlayButtonSoundEffect();
    }
    void Mute(){soundManager.Mute();};

    void Turn(MovementDirection type);

    std::vector<shared_ptr<Snake>> snakeNodes;
    std::vector<std::shared_ptr<MovingObject>> movingObjects;
    CameraType cameraType = TOP_VIEW;
    std::shared_ptr<SkinnedSnakeModel> snakeSkin;
    std::shared_ptr<Camera> topViewCam;
    std::shared_ptr<Camera> povCam;
    std::shared_ptr<Camera> tpsCam;
    std::shared_ptr<ConstantBoundable> backgroundBox;

    int pickedIndex = 0;
    int tipIndex = 0;


    Eigen::VectorXi EMAP;
    Eigen::MatrixXi F,E,EF,EI;
    Eigen::VectorXi EQ;
    // If an edge were collapsed, we'd collapse it to these points:
    Eigen::MatrixXd V, C, N, T, points,edges,colors;
    std::vector<PeriodicExecutor> executors;

    std::shared_ptr<cg3d::Mesh> prizeMesh;
    std::shared_ptr<cg3d::Material> prizeMaterial;
    std::shared_ptr<cg3d::Mesh> bombMesh;
    std::shared_ptr<cg3d::Material> bombMaterial;
    std::shared_ptr<cg3d::Mesh> nodeMesh;
    std::shared_ptr<cg3d::Mesh> snakeMesh;
    std::shared_ptr<cg3d::Material> snakeMaterial;
    std::shared_ptr<cg3d::Mesh> spawnerMesh;
    std::shared_ptr<cg3d::Material> spawnerMaterial;
    float headHeading = NINETY_DEGREES_IN_RADIANS;
    std::mutex mtx;
    cg3d::Viewport* viewport = nullptr;
    int lastQueueSize;


    MenuType DrawMainMenu();
    MenuType DrawGameMenu();
    MenuType DrawPauseMenu();
    MenuType DrawDeathMenu();
    MenuType DrawWinMenu();

    void DrawPlayerStats();

    MenuType menuType = MAIN;

    shared_ptr<MenuCords> menuCords;
    shared_ptr<MenuCords> gameCords;

    shared_ptr<MenuCords> GetCords(){
        switch (menuType) {
            case GAME:
                return gameCords;
            case MAIN:
            case PAUSE:
            case DEATH:
            case WIN:
            default:
                return menuCords;
        };
    }

    void ResetSnake();

    void Reset(){
        ResetSnake();

        ClearMovingObjectList();

        score = 0;
    };

    int score = 0;

    Vec3 snakeStartPo = {-10, 0, -10};

    void ClearMovingObjectList(){
        for (auto &object: movingObjects){
            root->RemoveChild(object->GetModel());
        }

        movingObjects.clear();

//        while (movingObjects.back()->GetModel()->GetTranslation().z() <= 999)
//            movingObjects.pop_back();
//        for (int i = 0; i < movingObjects.size(); i++){
//            auto object = movingObjects.at(i)->GetModel();
//            if (object->GetTranslation().z() <= 999)
//            {
//                std::remove(movingObjects.begin(), movingObjects.end(), object);
//                root->RemoveChild(object);
//            }
//        }
    }
};
