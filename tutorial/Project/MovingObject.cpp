//
// Created by User on 2/23/2023.
//
#include "MovingObject.h"

using namespace cg3d;
using namespace std;
using namespace Eigen;


float sign(float orig){return orig >= 0 ? 1.0f : -1.0;}
float angleBetweenVecs(Vector3f a, Vector3f b){
    float x1 = a.x();
    float y1 = a.y();
    float z1 = a.z();
    float x2 = b.x();
    float y2 = b.y();
    float z2 = b.z();
    float dot = x1*x2 + y1*y2 + z1*z2;
    float lenSq1 = x1*x1 + y1*y1 + z1*z1;
    float lenSq2 = x2*x2 + y2*y2 + z2*z2;
    return acos(dot/sqrt(lenSq1 * lenSq2));
}

void MovingObject::MoveForward() {
    float deltaFromBorder = 0;
    auto model = GetModel();

    if (IsPrize())
    {
        float x = model->GetTranslation().x();
        float y = model->GetTranslation().y();
        bool touchingHorizonal = sign(x) * x + deltaFromBorder >= VerticalBorder;
        bool touchingVertical = sign(y) * y + deltaFromBorder >= HorizontalBorder;

        if (touchingVertical || touchingHorizonal){
            root->RemoveChild(model);
        }
        model->Translate((velocity/5) * direction);
    }
}

Vector3f NormalBetweenTwoVectors(Vector3f a, Vector3f b){
    Vector3f crossProd (a.x() * b.x(), a.y() * b.y(), a.z() * b.z());
    return (1/crossProd.norm()) * crossProd;
}

bool SamePos(Vec3 a, Vec3 b){
    Vec3 diff = b - a;
    float delta = 0.05;
    float abs = std::sqrt(diff.x() * diff.x() + diff.y() * diff.y() + diff.z() * diff.z());
    return abs < delta;
//    for (int i = 0; i < 3; i++){
//        float abs = diff.array().abs();
//        auto diffSign = sign(diff[i]);
//        auto diffPositive = diff[i] < delta;
//        auto diffNegative = -1 * diff[i] < delta;
//        bool isDiffIOk = sign(diff[i]) > 0 ? diffPositive : diffNegative;
//        if (!isDiffIOk)
//            return false;
////        cout<<delta<<"\n"<<endl;
//    }
    return true;
//    sign(diff.z()) > 1 ? diff.z() < delta : -1 * diff.z() > delta;

}

void Snake::MoveForward() {
    if (rotationsQueue.size() > MAX_QUEUE_SIZE)
            return;
    auto rot = GetNodeModel()->GetRotation();

    Vector3f front = Vector3f (0,0,1);
    Vector3f back = -1.0 * front;
    Vector3f zero = Vector3f (0,0,0);

    Vector3f p0(GetNodeModel()->GetTranslation());
    Vector3f p1(zero);
    Vector3f p2(rot * front);
    Vec3 parentPos;

    if (IsTail()){// && SamePos(parentPos = parent->GetNodeModel()->GetTranslation(), p0)){
//        Vec3 currentPos = p0;

//        p2 = parent->GetNodeModel()->GetRotation() * front;   // !!!!! can follow tail


//        Vec3 parentPos = p2;
//
//
//        Vec3 projectionParOnFront = rot * Vec3(0,0,parentPos.z());
//        projectionParOnFront += currentPos;
//
//        Vector3f adj = (projectionParOnFront - currentPos);
//        Vector3f hypo = (parentPos - currentPos);
//
//        float arccos = adj.norm() / hypo.norm();
//        float tetha = acos(arccos);
//        Vector3f normal = NormalBetweenTwoVectors(adj, hypo);
//
//            snakeModel->Rotate(tetha, normal);
    }


    // Translate to p2
    snakeModel->Translate((velocity / 30.0f) * p2);
    invisibleBrother->Translate((velocity / 30.0f) * p2);
    // or
    // Do bezier
}

void Snake::AddRotation(shared_ptr<RotationCommand> command) {
    if (rotationsQueue.size() > MAX_QUEUE_SIZE) return;
    rotationsQueue.push(std::make_shared<FutureRotation>(snakeModel->GetTranslation(), *command));
}

/**
 *
 * @return the angle and axis of the rotation if a rotation should happen, pair {0, -1} otherwise.
 */


shared_ptr<RotationCommand> Snake::Rotate() {

    // make sure rotation queue isnt empty
    if (rotationsQueue.empty())
        return nullptr;

   auto nextCommand = rotationsQueue.front();
   if(IsTail() && (nextCommand->rotationCommand.destination.isApprox(snakeModel->GetTranslation()) || algebra::distance(nextCommand->recievedAt, snakeModel->GetTranslation()) < DISTANCE_FOR_MIMICING_ROTATIONS)) {
       return nullptr;
   }


    // pop from the queue
    rotationsQueue.pop();
    auto commandPtr = std::make_shared<RotationCommand>(nextCommand->rotationCommand);

    // check for child snake
    if (child != nullptr){
//        cout << "the needed position to rotate: " << poppedPair.first << "\n" << endl;
//        cout << "my position: " << snakeModel->GetTranslation() << "\n" << endl;
        child->AddRotation(std::make_shared<RotationCommand>(commandPtr->axis, commandPtr->angle, snakeModel->GetTranslation()));
    }

    return commandPtr;
}

void Snake::ClearQueue() {
//    rotationsQueue.c
//    queue<shared_ptr<pair<Vector3f, shared_ptr<pair<double, int>>>>> empty;
//    swap(rotationQueue, empty);
//
//    if (child != nullptr)
//        child->ClearQueue();
}

bool Snake::InRotation() {
    return !rotationsQueue.empty();
}

Snake::Snake(SnakeType _type, shared_ptr<NodeModel> _model, Vector3f _direction, shared_ptr<Snake> _parent,
             shared_ptr<Movable> root, float _h):
        MovingObject(SNAKE, nullptr, _direction, 1.5, root), type(_type), parent(_parent), heading(_h), snakeModel(_model){
  invisibleBrother = NodeModel::Create("invisible model", _model->GetMesh(0), _model->material);
  invisibleBrother->SetTransform(_model->GetTransform());

}
