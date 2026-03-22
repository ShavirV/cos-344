#ifndef SQUARE_CPP
#define SQUARE_CPP

#include "Square.h"

template <int n>
Square<n>::Square(const Vector<n>& center, float height, float width){
    //copy center and apply calculations to move in 2d
    tl = center;
    tl[0] -= width/2;
    tl[1] += height/2;

    tr = center; 
    tr[0] += width/2;
    tr[1] += height/2;

    bl = center; 
    bl[0] -= width/2;
    bl[1] -= height/2;
    
    br = center; 
    br[0] += width/2;
    br[1] -= height/2;
}

template <int n>
Square<n>::Square(const Vector<n>& tl, const Vector<n>& tr, const Vector<n>& br, const Vector<n>& bl){
    this->tl = tl;
    this->tr = tr;
    this->br = br;
    this->bl = bl;
}

template <int n>
Square<n>::Square(const Square<n>&other){
    this->tl = other.tl;
    this->tr = other.tr;
    this->br = other.br;
    this->bl = other.bl;
}

template <int n>
Square<n>& Square<n>::operator*=(const Matrix<n,n>&transform){
    //perform matrix multipliation on each of the vectors

    //convert to <n,1> matrix, multiply, then convert back to vector
    tl = transform * Matrix<n,1>(tl);
    tr = transform * Matrix<n,1>(tr);
    bl = transform * Matrix<n,1>(bl);
    br = transform * Matrix<n,1>(br);

    return *this;
}

template <int n>
Square<n>* Square<n>::operator*(const Matrix<n,n>&transform) const{
    Square<n>* temp = new Square(*this);
    *temp *= transform;
    return temp;
}

template <int n>
float* Square<n>::getPoints() const{
    float* points = new float[4 * n];

    int index = 0;

    for (int i = 0; i < n; i++){
        points[index++] = tl[i];
    }

    for (int i = 0; i < n; i++){
        points[index++] = tr[i];
    }

    for (int i = 0; i < n; i++){
        points[index++] = br[i];
    }

    for (int i = 0; i < n; i++){
        points[index++] = bl[i];
    }

    return points;
}

template <int n>
int Square<n>::getNumPoints() const{
    return n*4;
}


#endif