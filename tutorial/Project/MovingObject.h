//
// Created by User on 2/23/2023.
//

#ifndef ENGINEREWORK_MOVINGOBJECT_H
enum ObjectType{SNAKE, PRIZE};
enum SnakeType{HEAD, TAIL};
#define ENGINEREWORK_MOVINGOBJECT_H

#include "Scene.h"
#include "Eigen/Core"
#include "Mesh.h"
#include "AutoMorphingModel.h"
#include "BoundableModel.h"
#include <queue>
#include "algebra.h"


#define HorizontalBorder 9.0f
#define VerticalBorder 9.0f
#define MAX_QUEUE_SIZE 10000
#define DISTANCE_FOR_MIMICING_ROTATIONS 1.0f


using namespace cg3d;
using namespace std;
using namespace Eigen;

typedef Movable::Axis Axis;

struct RotationCommand {
    Axis axis;
    double angle;
    Vec3 destination;
    RotationCommand(Axis axis, double angle, Vec3 destination): axis(axis), angle(angle), destination(destination) {};
};

struct FutureRotation {
    Vec3 recievedAt;
    RotationCommand rotationCommand;
    FutureRotation(Vec3 recievedAt, RotationCommand rotationCommand):
        recievedAt(recievedAt), rotationCommand(rotationCommand) {};
};

struct Rotation {
    Axis axis;
    float angle;
    Rotation(Axis axis, float angle): axis(axis), angle(angle) {};
};

struct ContinuousRotation {
    Axis axis;
    float totalAngle;
    float rotated;
    ContinuousRotation(Axis axis, float angle): axis(axis), totalAngle(angle), rotated(0) {};
};

struct BezierInformation {
    std::function<Vec3(float)> curve;
    std::function<float(void)> getT;
    ContinuousRotation rotationInfo;
    BezierInformation(std::function<Vec3(float)> curve,
                      std::function<float(void)> getT, Rotation rotation):
        curve(std::move(curve)), getT(std::move(getT)), rotationInfo(rotation.axis, rotation.angle) {};
};


class MovingObject {

public:
    MovingObject(ObjectType _type, BoundablePtr _model, Vector3f _direction, float _velocity, shared_ptr<Movable> _root):
                    type(_type), model(_model), direction(_direction), velocity(_velocity), root(_root){};

    // collision


    bool IsPrize(){return type == PRIZE;};
    bool IsSnake(){return type == SNAKE;};

    void MoveForward();

    virtual BoundablePtr GetModel(){return model;};
    float GetVelocity(){return velocity;};
    Vector3f direction;
    float velocity;

private:
    BoundablePtr model;
    ObjectType type;
    std::shared_ptr<Movable> root;
};

#define MovingPtr std::shared_ptr<MovingObject>

class Snake: MovingObject {
public:
    Snake(SnakeType _type, shared_ptr<NodeModel> _model, Vector3f _direction, shared_ptr<Snake> _parent, shared_ptr<Movable> root, float _h);

    bool IsHead(){return type == HEAD;};
    bool IsTail(){return type == TAIL;};
    SnakeType type;
    shared_ptr<Snake> parent;
    shared_ptr<Snake> child;
    void AddChild(shared_ptr<Snake> _child){child = _child;};
    void RemoveChild(){child = nullptr;};
    float heading;
    shared_ptr<NodeModel> GetNodeModel(){return snakeModel;};
    void MoveForward();
    void AddRotation(shared_ptr<RotationCommand> command);
    void AddBezier(shared_ptr<RotationCommand> command);
    void ClearQueue();
    shared_ptr<RotationCommand> Rotate();
    queue<shared_ptr<FutureRotation>> rotationsQueue;
    bool InRotation();
    shared_ptr<NodeModel> invisibleBrother;
    std::queue<BezierInformation> beziers;

private:
    shared_ptr<NodeModel> snakeModel;
};


#endif //ENGINEREWORK_MOVINGOBJECT_H
