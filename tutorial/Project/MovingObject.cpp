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
    }


    model->Translate((velocity/10) * direction);

}