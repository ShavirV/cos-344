#include "Matrix.h"
#include "Vector.h"
#include "Shape.h"
//#include "Triangle.h"
//#include "Square.h"

#include <iostream>
#include <cassert>
#include <cmath>
#include <sstream>

#define EPS 1e-4f

// Helper function to compare floats
bool nearlyEqual(float a, float b, float eps = EPS) {
    return std::fabs(a - b) < eps;
}

void testConstructorAndIndexing() {
    Matrix<2,3> A;

    A[0][0] = 1;
    A[0][1] = 2;
    A[0][2] = 3;
    A[1][0] = 4;
    A[1][1] = 5;
    A[1][2] = 6;

    assert(A[1][2] == 6);
    std::cout << "Constructor + Indexing OK\n";
}

void testCopyConstructor() {
    Matrix<2,2> A;
    A[0][0] = 1; A[0][1] = 2;
    A[1][0] = 3; A[1][1] = 4;

    Matrix<2,2> B(A);

    B[0][0] = 100;

    assert(A[0][0] == 1); // deep copy check
    std::cout << "Copy Constructor OK\n";
}

void testAssignmentOperator() {
    Matrix<2,2> A;
    Matrix<2,2> B;

    A[0][0] = 5;
    B = A;

    assert(B[0][0] == 5);

    B = B; // self-assignment
    assert(B[0][0] == 5);

    std::cout << "Assignment Operator OK\n";
}

void testAddition() {
    Matrix<2,2> A;
    Matrix<2,2> B;

    A[0][0] = 1; A[0][1] = 2;
    A[1][0] = 3; A[1][1] = 4;

    B[0][0] = 5; B[0][1] = 6;
    B[1][0] = 7; B[1][1] = 8;

    Matrix<2,2> C = A + B;

    assert(C[0][0] == 6);
    assert(C[1][1] == 12);

    std::cout << "Addition OK\n";
}

void testScalarMultiplication() {
    Matrix<2,2> A;
    A[0][0] = 2;
    A[1][1] = 3;

    Matrix<2,2> B = A * 2.0f;

    assert(B[0][0] == 4);
    assert(B[1][1] == 6);

    std::cout << "Scalar Multiplication OK\n";
}

void testMatrixMultiplication() {
    Matrix<2,3> A;
    Matrix<3,2> B;

    // A
    A[0][0] = 1; A[0][1] = 2; A[0][2] = 3;
    A[1][0] = 4; A[1][1] = 5; A[1][2] = 6;

    // B
    B[0][0] = 7;  B[0][1] = 8;
    B[1][0] = 9;  B[1][1] = 10;
    B[2][0] = 11; B[2][1] = 12;

    Matrix<2,2> C = A * B;

    assert(C[0][0] == 58);
    assert(C[0][1] == 64);
    assert(C[1][0] == 139);
    assert(C[1][1] == 154);

    std::cout << "Matrix Multiplication OK\n";
}

void testTranspose() {
    Matrix<2,3> A;

    A[0][1] = 5;
    A[1][2] = 9;

    Matrix<3,2> T = ~A;

    assert(T[1][0] == 5);
    assert(T[2][1] == 9);

    std::cout << "Transpose OK\n";
}

void testDeterminant2x2() {
    Matrix<2,2> A;

    A[0][0] = 4; A[0][1] = 6;
    A[1][0] = 3; A[1][1] = 8;

    float det = A.determinant();
    assert(nearlyEqual(det, 14)); // 4*8 - 6*3

    std::cout << "Determinant 2x2 OK\n";
}

void testDeterminant3x3() {
    Matrix<3,3> A;

    A[0][0] = 6; A[0][1] = 1; A[0][2] = 1;
    A[1][0] = 4; A[1][1] = -2; A[1][2] = 5;
    A[2][0] = 2; A[2][1] = 8; A[2][2] = 7;

    float det = A.determinant();
    assert(nearlyEqual(det, -306));

    std::cout << "Determinant 3x3 OK\n";
}

void testDeterminant4x4() {
    Matrix<4,4> A;

    A[0][0] = 6; A[0][1] = 1; A[0][2] = 1; A[0][3] = 3;
    A[1][0] = 4; A[1][1] = -2; A[1][2] = 5; A[1][3] = 5;
    A[2][0] = 2; A[2][1] = 8; A[2][2] = 7; A[2][3] = 0;
    A[3][0] = 2; A[3][1] = 2; A[3][2] = 6; A[3][3] = 0;

    float det = A.determinant();

    std::cout << "Determinant 4x4 " << det << "\n";
}

void testDeterminantRowSwapCase() {
    Matrix<2,2> A;

    A[0][0] = 0; A[0][1] = 1;
    A[1][0] = 1; A[1][1] = 0;

    float det = A.determinant();
    assert(nearlyEqual(det, -1)); // requires row swap

    std::cout << "Determinant Row Swap OK\n";
}

void testBounds() {
    Matrix<2,2> A;

    bool caught = false;
    try {
        A[3][0] = 5;
    } catch (...) {
        caught = true;
    }

    assert(caught);
    std::cout << "Bounds Checking OK\n";
}

int main() {

    testConstructorAndIndexing();
    testCopyConstructor();
    testAssignmentOperator();
    testAddition();
    testScalarMultiplication();
    testMatrixMultiplication();
    testTranspose();
    testDeterminant2x2();
    testDeterminant3x3();
    testDeterminant4x4();
    testDeterminantRowSwapCase();
    testBounds();

    std::cout << "\nALL TESTS PASSED SUCCESSFULLY\n";

    return 0;
}
