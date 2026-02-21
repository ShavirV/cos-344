#include "Matrix.h"
#include "Vector.h"
#include "Shape.h"
//#include "Triangle.h"
//#include "Square.h"

#include <iostream>
#include <cassert>
#include <cmath>
#include <sstream>

#define EPSILON 1e-5
#define EPS 1e-4f

bool floatEqual(float a, float b)
{
    return std::fabs(a - b) < EPSILON;
}

/* ===============================
   Constructor Tests
   =============================== */

void testVectorDefaultConstructor()
{
    Vector<3> v;
    for (int i = 0; i < 3; i++)
        assert(floatEqual(v[i], 0.0f));

    std::cout << "Default constructor\n";
}

void testVectorInitializerListConstructor()
{
    Vector<3> v{1.0f, 2.0f, 3.0f};
    assert(floatEqual(v[0], 1.0f));
    assert(floatEqual(v[1], 2.0f));
    assert(floatEqual(v[2], 3.0f));

    Vector<4> v2{1.0f, 2.0f};
    assert(floatEqual(v2[2], 0.0f));
    assert(floatEqual(v2[3], 0.0f));

    std::cout << "Initializer list constructor\n";
}

void testVectorPointerConstructor()
{
    float* arr = new float[3]{4.0f, 5.0f, 6.0f};
    Vector<3> v(arr);

    assert(floatEqual(v[0], 4.0f));
    assert(floatEqual(v[1], 5.0f));
    assert(floatEqual(v[2], 6.0f));

    // WARNING: destructor will delete arr
    std::cout << "Pointer constructor (shallow copy)\n";
}

void testVectorCopyConstructor()
{
    Vector<3> v1{1,2,3};
    Vector<3> v2(v1);

    v2[0] = 99;

    assert(floatEqual(v1[0], 1.0f));
    assert(floatEqual(v2[0], 99.0f));

    std::cout << "Copy constructor (deep copy)\n";
}

/* ===============================
   Assignment Operator
   =============================== */

void testVectorAssignmentOperator()
{
    Vector<3> v1{1,2,3};
    Vector<3> v2;

    v2 = v1;
    v2[1] = 42;

    assert(floatEqual(v1[1], 2.0f));
    assert(floatEqual(v2[1], 42.0f));

    std::cout << "Assignment operator\n";
}

/* ===============================
   Arithmetic Operators
   =============================== */

void testVectorAddition()
{
    Vector<3> a{1,2,3};
    Vector<3> b{4,5,6};

    Vector<3> c = a + b;

    assert(floatEqual(c[0], 5));
    assert(floatEqual(c[1], 7));
    assert(floatEqual(c[2], 9));

    std::cout << "Vector addition\n";
}

void testVectorSubtraction()
{
    Vector<3> a{5,7,9};
    Vector<3> b{1,2,3};

    Vector<3> c = a - b;

    assert(floatEqual(c[0], 4));
    assert(floatEqual(c[1], 5));
    assert(floatEqual(c[2], 6));

    std::cout << "Vector subtraction\n";
}

void testVectorScalarMultiplication()
{
    Vector<3> v{1,2,3};
    Vector<3> r = v * 2.0f;

    assert(floatEqual(r[0], 2));
    assert(floatEqual(r[1], 4));
    assert(floatEqual(r[2], 6));

    std::cout << "Scalar multiplication\n";
}

/* ===============================
   Dot & Cross Product
   =============================== */

void testVectorDotProduct()
{
    Vector<3> a{1,2,3};
    Vector<3> b{4,5,6};

    float dot = a * b;

    // Expected: 1*4 + 2*5 + 3*6 = 32
    assert(floatEqual(dot, 32));

    std::cout << "Dot product\n";
}

void testVectorCrossProduct()
{
    Vector<3> a{1,0,0};
    Vector<3> b{0,1,0};

    Vector<3> c = a.crossProduct(b);

    assert(floatEqual(c[0], 0));
    assert(floatEqual(c[1], 0));
    assert(floatEqual(c[2], 1));

    std::cout << "Cross product\n";
}

/* ===============================
   Magnitude & Unit Vector
   =============================== */

void testVectorMagnitude()
{
    Vector<3> v{3,4,0};
    assert(floatEqual(v.magnitude(), 5.0f));

    std::cout << "Magnitude\n";
}

void testVectorUnitVector()
{
    Vector<3> v{3,0,0};
    Vector<3> u = v.unitVector();

    assert(floatEqual(u[0], 1.0f));
    assert(floatEqual(u[1], 0.0f));
    assert(floatEqual(u[2], 0.0f));

    std::cout << "Unit vector\n";
}

void testVectorZeroUnitVectorThrows()
{
    try {
        Vector<3> v{0,0,0};
        v.unitVector();
        assert(false); // should not reach here
    } catch (...) {
        std::cout << "Zero vector unitVector throws\n";
    }
}

/* ===============================
   Index Operator
   =============================== */

void testVectorIndexBounds()
{
    Vector<3> v{1,2,3};

    try {
        v[5];
        assert(false);
    } catch (...) {
        std::cout << "Index out of bounds throws\n";
    }
}

/* ===============================
   getN()
   =============================== */

void testVectorGetN()
{
    Vector<7> v;
    assert(v.getN() == 7);

    std::cout << "getN()\n";
}

/*
 * Matrix testing
 */
// Helper function to compare floats
bool nearlyEqual(float a, float b, float eps = EPS) {
    return std::fabs(a - b) < eps;
}

void testMatrixConstructorAndIndexing() {
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

void testMatrixCopyConstructor() {
    Matrix<2,2> A;
    A[0][0] = 1; A[0][1] = 2;
    A[1][0] = 3; A[1][1] = 4;

    Matrix<2,2> B(A);

    B[0][0] = 100;

    assert(A[0][0] == 1); // deep copy check
    std::cout << "Copy Constructor OK\n";
}

void testMatrixAssignmentOperator() {
    Matrix<2,2> A;
    Matrix<2,2> B;

    A[0][0] = 5;
    B = A;

    assert(B[0][0] == 5);

    B = B; // self-assignment
    assert(B[0][0] == 5);

    std::cout << "Assignment Operator OK\n";
}

void testMatrixAddition() {
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

void testMatrixScalarMultiplication() {
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

void testMatrixTranspose() {
    Matrix<2,3> A;

    A[0][1] = 5;
    A[1][2] = 9;

    Matrix<3,2> T = ~A;

    assert(T[1][0] == 5);
    assert(T[2][1] == 9);

    std::cout << "Transpose OK\n";
}

void testMatrixDeterminant2x2() {
    Matrix<2,2> A;

    A[0][0] = 4; A[0][1] = 6;
    A[1][0] = 3; A[1][1] = 8;

    float det = A.determinant();
    assert(nearlyEqual(det, 14)); // 4*8 - 6*3

    std::cout << "Determinant 2x2 OK\n";
}

void testMatrixDeterminant3x3() {
    Matrix<3,3> A;

    A[0][0] = 6; A[0][1] = 1; A[0][2] = 1;
    A[1][0] = 4; A[1][1] = -2; A[1][2] = 5;
    A[2][0] = 2; A[2][1] = 8; A[2][2] = 7;

    float det = A.determinant();
    assert(nearlyEqual(det, -306));

    std::cout << "Determinant 3x3 OK\n";
}

void testMatrixDeterminant4x4() {
    Matrix<4,4> A;

    A[0][0] = 6; A[0][1] = 1; A[0][2] = 1; A[0][3] = 3;
    A[1][0] = 4; A[1][1] = -2; A[1][2] = 5; A[1][3] = 5;
    A[2][0] = 2; A[2][1] = 8; A[2][2] = 7; A[2][3] = 0;
    A[3][0] = 2; A[3][1] = 2; A[3][2] = 6; A[3][3] = 0;

    float det = A.determinant();

    std::cout << "Determinant 4x4 " << det << "\n";
}

void testMatrixDeterminantRowSwapCase() {
    Matrix<2,2> A;

    A[0][0] = 0; A[0][1] = 1;
    A[1][0] = 1; A[1][1] = 0;

    float det = A.determinant();
    assert(nearlyEqual(det, -1)); // requires row swap

    std::cout << "Determinant Row Swap OK\n";
}

void testMatrixBounds() {
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

    testMatrixConstructorAndIndexing();
    testMatrixCopyConstructor();
    testMatrixAssignmentOperator();
    testMatrixAddition();
    testMatrixScalarMultiplication();
    testMatrixMultiplication();
    testMatrixTranspose();
    testMatrixDeterminant2x2();
    testMatrixDeterminant3x3();
    testMatrixDeterminant4x4();
    testMatrixDeterminantRowSwapCase();
    testMatrixBounds();

    std::cout << "\nALL MATRIX TESTS PASSED SUCCESSFULLY\n";

    testVectorDefaultConstructor();
    testVectorInitializerListConstructor();
    testVectorPointerConstructor();
    testVectorCopyConstructor();
    testVectorAssignmentOperator();
    testVectorAddition();
    testVectorSubtraction();
    testVectorScalarMultiplication();
    testVectorDotProduct();
    testVectorCrossProduct();
    testVectorMagnitude();
    testVectorUnitVector();
    testVectorZeroUnitVectorThrows();
    testVectorIndexBounds();
    testVectorGetN();

    std::cout << "\nALL VECTOR TESTS PASSED!\n";

    return 0;
}
