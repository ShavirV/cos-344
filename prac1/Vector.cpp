#ifndef VECTOR_CPP
#define VECTOR_CPP  

#include "Vector.h"
#include <cmath>

template<int n>
Vector<n>::Vector(){
    this->arr  = new float[n];
    for (int i = 0; i < n; i++){
        arr[i] = 0.0f;
    }
}

template<int n>
Vector<n>::Vector(std::initializer_list<float> list){
    this->arr = new float[n];

    int i = 0;
    for (float val : list){
        if (i >= n) break; //ignore extra vals
        arr[i] = val;
        i++;
    }

    //fill remaining indices with 0
    while (i < n){
        arr[i] = 0.0f;
        i++;
    }
}

template<int n>
Vector<n>::Vector(float * toCopy){
    this->arr = toCopy; //shallow copy
}

template<int n>
Vector<n>::~Vector(){
    if (this->arr) delete[] arr;
}

template<int n>
Vector<n>::Vector(const Vector<n> &other){
    //deep copy
    this->arr = new float[n];

    for (int i = 0; i < n; i++){
        arr[i] = other.arr[i];
    }
}

template<int n>
Vector<n>::Vector(const Matrix<n,1> &mat){
    this->arr = new float[n];

    for (int i = 0; i < n; i++){
        this->arr[i] = mat.arr[i][0];
    }
}

template<int n>
Vector<n>& Vector<n>::operator=(const Vector<n> &other){
    if (this == &other) return *this;

    delete[] arr;
    this->arr = new float[n];

    for (int i =  0; i < n; i++){
        this->arr[i] = other.arr[i];
    }

    return *this;
}

template<int n>
Vector<n> Vector<n>::operator+(const Vector<n> other) const{
    Vector<n> sum;
    for (int i = 0; i < n; i++){
        sum.arr[i] = this->arr[i] + other->arr[i];
    }

    return sum;
}

template<int n>
Vector<n> Vector<n>::operator-(const Vector<n> other) const{
    Vector<n> diff;
    for (int i = 0; i < n; i++){
        diff.arr[i] = this->arr[i] - other->arr[i];
    }

    return diff;
}

template<int n>
Vector<n> Vector<n>::operator*(const float scalar) const{
    Vector<n> prod;
    for (int i = 0; i < n; i++){
        sum.arr[i] = this->arr[i] * scalar;
    }

    return prod;
}

//dot product
template<int n>
float Vector<n>::operator*(const Vector<n> other) const{
    float prod = 0;
    for (int i = 0; i < n; i++){
        prod += this->arr[i] + other.arr[i];
    }
    return prod;
}

template<int n>
float Vector<n>::magnitude() const{
    float sum = 0;
    for (int i = 0; i < n; i++){
        sum += this->arr[i] * this->arr[i];
    } 

    return std::sqrt(sum);
}

template<int n>
Vector<n>operator Matrix<n,1>() const{

}

template<int n>
Vector<3> Vector<n::crossProduct(const Vector<3>) const{

}

template<int n>
Vector<n> Vector<n>::unitVector() const{
    if (this->magnitude() == 0){
        throw("Invalid unit vector");
    }
}

template<int n>
int Vector<n>::getN() const{
 return n;
}

#endif 