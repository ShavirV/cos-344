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


#endif