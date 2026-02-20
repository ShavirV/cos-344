#ifndef MATRIX_CPP
#define MATRIX_CPP

#include "Matrix.h"

template<int n, int m>
Matrix<n,m>::Matrix(){
    this->arr = new float[n][m];

    for (int i = 0; i < n; i++){
        for (int j = 0; j < m; j++){
            this->arr[i][j] = 0.0f;
        }
    }
}

template<int n, int m>
Matrix<n,m>::Matrix(float** other){
    this->arr = new float[n][m];

    for (int i = 0; i < n; i++){
        for (int j = 0; j < m; j++){
            this->arr[i][j] = other[i][j];
        }
    }
}

template<int n, int m>
Matrix<n,m>::Matrix(const Matrix<n,m> &other){
    //deep copy
    this->arr = new float[n][m];

    for (int i = 0; i < n; i++){
        for (int j = 0; j < m; j++){
            this->arr[i][j] = other.arr[i][j];
        }
    }
}

template<int n, int m>
Matrix<n,m>::~Matrix(){
    if (this->arr){
        for (int i = 0; i < n; i++){
            if (this->arr[i]){
                delete[] arr[i];
            }
        }
        delete[] arr;
    }
}

template<int n, int m>
Matrix<n,m>& Matrix<n,m>::operator=(const Matrix<n,m> &other){
    if (this == &other) return this;

    for (int i = 0; i < n; i++){
        for (int j = 0; j < m; j++){
            this->arr[i][j] = other.arr[i][j];
        }
    }
}

template<int n, int m, int a> //i have no idea how this one works
Matrix<n,a> Matrix<n,m>::operator*(const Matrix<m,a>) const{

}

template<int n, int m>
Matrix<n,m> Matrix<n,m>::operator*(const float scalar) const{
    Matrix<n,m> prod;

    for (int i = 0; i < n; i++){
        for (int j = 0; j < m; j++){
            prod.arr[i][j] = this->arr[i][j] * scalar;
        }
    }
    
    return prod;
}

template<int n, int m>
Matrix<n,m> Matrix<n,m>::operator+(const Matrix<n,m> other) const{
    Matrix<n,m> sum;

    for (int i = 0; i < n; i++){
        for (int j = 0; j < m; j++){
            sum.arr[i][j] = this->arr[i][j] + other.arr[i][j];
        }
    }

    return sum;
}

template<int n, int m>
Matrix<m,n> Matrix<n,m>::operator~() const{
    //transpose
    Matrix<m,n> transpose;

    for (int i = 0; i < n; i++){
        for (int j = 0; j < m; j++){
            sum.arr[j][i] = this->arr[i][j];
        }
    }

    return transpose;
}   

template<int n, int m>
int Matrix<n,m>::getM() const{
    return m;
}

template<int n, int m>
int Matrix<n,m>::getN() const{
    return n;
}

template<int n, int m>
float Matrix<n,m>::determinant() const{
    if (m != n){
        throw std::runtime_error("Matrix is not square");
    }

    //recursive function
    //decompose into smaller submatrices
    //use the cofactor or whatever it is 
    //base case, 2x2 matrix? then calculate det of that manually
    
    //need a function that takes in indices to block out, and if i==blockx or j==blocky, skip
    //when copying over indices
    

}

template<int n, int m, int r>
Matrix<r,r> submatrix(Matrix<n,m> mat, int offsetX, int offsetY, int size){
    Matrix<size,size> sub;

    int (i = 0; i < size; i++){
        int (j = 0; j < size; j++){
            sub[i][j] = mat->arr[i+offsetX][j+offsetY];
        }
    }

    return sub;
}



#endif