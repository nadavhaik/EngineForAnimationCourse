#pragma once
#include "AutoMorphingModel.h"
#include "Scene.h"
#include "PeriodicExecutor.h"
#include <memory>
#include <utility>

#define PERIODIC_INTERVAL_MILLIS 20
#define NINETY_DEGREES_IN_RADIANS 1.57079633f
#define SNAKE_TURN_ANGLE_RADIANS 0.1f
#define NODE_HEIGHT 1.5f

enum MovementDirection {RIGHT, LEFT, UP, DOWN};
enum MovementType {STRAIGHT, TURN};

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
    void TurnRight();
    void TurnLeft();
    void AddToTail();
    static Eigen::AlignedBox<float, 3> BoxOfModel(std::shared_ptr<cg3d::Model> model);
    static bool ModelsCollide(std::shared_ptr<cg3d::Model> m1, std::shared_ptr<cg3d::Model> m2);
private:
    std::shared_ptr<Movable> root;
    std::vector<std::shared_ptr<cg3d::Model>> snakeNodes;
    int pickedIndex = 0;
    int tipIndex = 0;
    Eigen::VectorXi EMAP;
    Eigen::MatrixXi F,E,EF,EI;
    Eigen::VectorXi EQ;
    // If an edge were collapsed, we'd collapse it to these points:
    Eigen::MatrixXd V, C, N, T, points,edges,colors;
    std::shared_ptr<PeriodicExecutor> executor;
    std::shared_ptr<cg3d::Mesh> snakeMesh;
    std::shared_ptr<cg3d::Material> snakeMaterial;
    std::vector<float> headings;
    float headHeading = NINETY_DEGREES_IN_RADIANS;
};
