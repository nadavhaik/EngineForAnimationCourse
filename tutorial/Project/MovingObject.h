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
#include <format>


#define HorizontalBorder 1.0f
#define VerticalBorder 1.0f



using namespace cg3d;
using namespace std;
using namespace Eigen;


class MovingObject {

public:
    MovingObject(ObjectType _type, shared_ptr<Model> _model, Vector3f _direction, float _velocity, shared_ptr<Movable> _root):
                    type(_type), model(_model), direction(_direction), velocity(_velocity), root(_root){};

    bool IsPrize(){return type == PRIZE;};
    bool IsSnake(){return type == SNAKE;};

    void MoveForward();

    shared_ptr<Model> GetModel(){return model;};
    float GetVelocity(){return velocity;};

private:
    ObjectType type;
    shared_ptr<Model> model;
    Vector3f direction;
    float velocity;
    std::shared_ptr<Movable> root;
};

class Snake: MovingObject{
public:
    Snake(SnakeType _type, shared_ptr<Model> _model, Vector3f _direction, shared_ptr<Snake> _parent, shared_ptr<Movable> root):
        MovingObject(SNAKE, _model, _direction, 1, root), type(_type), parent(_parent){};

    bool IsHead(){return type == HEAD;};
    bool IsTail(){return type == TAIL;};

private:
    SnakeType type;
    shared_ptr<Snake> parent;
};


#endif //ENGINEREWORK_MOVINGOBJECT_H
