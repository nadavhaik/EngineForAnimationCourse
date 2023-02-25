#pragma once
#include "PeriodicExecutor.h"
#include "types_macros.h"
#include <memory>
#include <utility>
#include "BoundableModel.h"
#include "MovingObject.h"
#include <numbers>
#include "functionals.h"
#include "mutex"

#define PrizeMaxVelocity 0.8f
#define PrizeMinVelocity 0.2f


#define UPDATE_INTERVAL_MILLIS 20
#define COLLUSION_DETECTION_INTERVAL_MILLIS 500

#define NINETY_DEGREES_IN_RADIANS 1.57079633f
#define SNAKE_TURN_ANGLE_RADIANS 0.1f


#define MOVEMENT_DISTANCE 0.03f

enum MovementDirection {RIGHT, LEFT, UP, DOWN};
enum MovementType {STRAIGHT, TURN};

enum CameraType {POV, TPS, TOP_VIEW};

using namespace Eigen;

struct SnakeNode {
    std::shared_ptr<NodeModel> model;
    float heading{};
};

class BasicScene : public cg3d::Scene
{
public:
    explicit BasicScene(std::string name, cg3d::Display* display) : Scene(std::move(name), display) {};
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

    Vector3f RandomSpawnPoint();
    void AddPrize();

    float RollRandomAB(float min, float max){return min + static_cast <float> (rand()) /( static_cast <float> (RAND_MAX/(max - min)));}
    std::shared_ptr<Movable> root;
    void RemoveMoving(shared_ptr<MovingObject> moving);
    void FollowHeadWithCamera();

private:
    void Turn(MovementDirection type);

    std::vector<shared_ptr<Snake>> snakeNodes;
    vector<shared_ptr<MovingObject>> movingObjects;
    CameraType cameraType = TOP_VIEW;

    std::shared_ptr<Camera> topViewCam;
    std::shared_ptr<Camera> povCam;
    std::shared_ptr<Camera> tpsCam;

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
    std::shared_ptr<cg3d::Mesh> snakeMesh;
    std::shared_ptr<cg3d::Material> snakeMaterial;
    float headHeading = NINETY_DEGREES_IN_RADIANS;
    std::mutex mtx;
    cg3d::Viewport* viewport = nullptr;
    int lastQueueSize;
};
