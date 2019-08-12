#pragma once

#ifndef MYUTIL_H
#define MYUTIL_H

#include <stdlib.h>
#include <GLES3/gl3.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "esUtil.h"

#define PI 3.14159265


//             [m1, m2, m3]
// [x, y, z] * [m4, m5, m6]
//             [m7, m8, m9]
void VecMatMultiply3(GLfloat* vec3, const ESMatrix3 mat3);

// rotate along y axis
void Rotate(GLfloat* eye, GLfloat angle);

// any time the position of eye changes, the direction needs to be changed also
void Redirection(GLfloat* lookAt, const GLfloat* target, const GLfloat* eye);

// 2d array should be used for OpenGL instead of OpenGLES, thus we need to transform 2d to 1d array for ES
// [[0.5, 0.5, 0.5], [-0.5, 0.5, 0.5]...] --> [0.5, 0.5, 0.5, -0.5, 0.5, 0.5...]
// void transVArr(const GLfloat** source, const int rowCount, GLfloat* ret);

// in a obj file, if a face part is like this:
// f 5/11/23 1/10/22 3/12/24 7/9/21
// which is more than three vertex. In this case, it represents two faces instead of one.
// this method is for getting a correct number of face.
int updatedFaceNum(const int* face_counts, const int f_num);

// assign value to ret (a,b,c) based on vArr, representing the very middle and bottom position of 
// the model in order to draw a floor
void GetMidBotPosition(const GLfloat** vArr, const int length, GLfloat* ret);

///
// Load texture from disk
// Used for model texture
//
GLuint LoadTexture(void* ioContext, char* fileName);

///
// Create a simple 2x2 texture image with four different colors
// Used for drawing floor
//
GLuint CreateSimpleTexture2D();

#endif