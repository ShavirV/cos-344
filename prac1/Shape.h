#ifndef SHAPE_H
#define SHAPE_H

#include "Matrix.h"

#include <iostream>

template <int n>
class Shape{
    public:
        virtual Shape& operator*=(const Matrix<n,n>&) =0;
        virtual Shape* operator*(const Matrix<n,n>&) const =0;
        virtual float* getPoints() const =0;
        virtual int getNumPoints() const =0;
        virtual void print() const =0;
};

#endif /*SHAPE_H*/