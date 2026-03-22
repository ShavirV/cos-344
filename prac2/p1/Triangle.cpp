#ifndef TRIANGLE_CPP
#define TRIANGLE_CPP

#include "Triangle.h"

template<int n>
Triangle<n>::Triangle(const Vector<n>& p1, const Vector<n>& p2, const Vector<n>& p3){
    this->p1 = p1;
    this->p2 = p2;
    this->p3 = p3;
}

template<int n>
Triangle<n>::Triangle(const Triangle<n>&other){
    this->p1 = other.p1;
    this->p2 = other.p2;
    this->p3 = other.p3;
}

template<int n>
Triangle<n>& Triangle<n>::operator*=(const Matrix<n,n>&transform){
    p1 = transform * Matrix<n,1>(p1);
    p2 = transform * Matrix<n,1>(p2);
    p3 = transform * Matrix<n,1>(p3);
    
    return *this;
}

template<int n>
Triangle<n>* Triangle<n>::operator*(const Matrix<n,n>&transform) const{
    Triangle<n>* temp = new Triangle(*this);
    *temp *= transform;
    return temp;
}

template<int n>
float* Triangle<n>::getPoints() const{

    float* points = new float[3 * n];

    int index = 0;

    for (int i = 0; i < n; i++){
        points[index++] = p1[i];
    }

    for (int i = 0; i < n; i++){
        points[index++] = p2[i];
    }

    for (int i = 0; i < n; i++){
        points[index++] = p3[i];
    }

    return points;
}

template<int n>
int Triangle<n>::getNumPoints() const{
    return 3*n;
}

template<int n>
virtual void midpoint(float* coordinates) override{
    for (int i = 0; i < n; i++)
        coordinates[i] = vertices[0][i];
}

//selection already in shape.h


//move translated the polygon in 2D, building a homogenous translation matrix
// [1 0 dx]
// [0 1 dy]
// [0 0  1]
// then *= this matrix
// w=up, s=down, a=left, d=right
virtual void move(char dir) override{
    const float STEP = 0.05f; ///adjust as needed
    float dx = 0.0f, dy = 0.0f;

    //decide how to move
    switch (dir) {
        case 'w': dy =  STEP; break;
        case 's': dy = -STEP; break;
        case 'a': dx = -STEP; break;
        case 'd': dx =  STEP; break;
        default: return; //not our problem
    }

    //build the transformation matrix
        // [1 0 dx]
        // [0 1 dy]
        // [0 0  1]
    Matrix<n,n> T = Matrix::identity();
    T[0][2] = dx;
    T[1][2] = dy;

    *this *= T;
}

//scale the polygon uniformly AROUND ITS CENTRE
virtual void scale(char dir) override {
    const float FACTOR_UP = 1.15f;
    const float FACTOR_DOWN = 1.0f / FACTOR_UP;
    float s = (dir == "+") ? FACTOR_UP : FACTOR_DOWN;

    //get the centre and transform
    float cx = vertices[0][0];
    float cy = vertices[0][1];

    //T(-centre): translate centre to origin
    Matrix<n,n> Tneg;
    Tneg[0][2] = -cx;
    Tneg[1][2] = -cy;

    //S(s): uniform scale
    Matrix<n,n> S;
    S[0][0] = s;
    S[1][1] = s;

    //T(+centre): translate back
    Matrix<n,n> Tpos;
    Tpos[0][2] = cx;
    Tpos[1][2] = cy;

    //Combined: Tpos * S * Tneg
    //Apply right-to-left: first Tneg, then S, then Tpos
    *this *= Tneg;
    *this *= S;
    *this *= Tpos;

    //this should happen instantaneoulsy when rendered, thus the translation should not be visible
}

template<int n>
virtual void rotate(char dir) override {
    const float ANGLE_STEP = 0.1f; //in radians, about 6 degrees
    float angle = (dir == 'l') ANGLE_STEP : -ANGLE_STEP;

    float cx = vertices[0][0];
    float cy = vertices[0][1];
    float cosA = std::cos(angle);
    float sinA = std::sin(angle);

    //same as scaling pretty much, just apply Cosa and Sina instead

    //T(-centre)
    Matrix<n,n> Tneg;
    Tneg[0][2] = -cx;
    Tneg[1][2] = -cy;

    //R(angle)
    Matrix<n,n> R;
    R[0][0] =  cosA;  R[0][1] = -sinA;
    R[1][0] =  sinA;  R[1][1] =  cosA;

    //T(+centre)
    Matrix<n,n> Tpos;
    Tpos[0][2] = cx;
    Tpos[1][2] = cy;

    *this *= Tneg;
    *this *= R;
    *this *= Tpos;
}


#endif