#include "myUtil.h"

#define PI 3.14159265

//             [m1, m2, m3]
// [x, y, z] * [m4, m5, m6]
//             [m7, m8, m9]
// the result value will be stored in vec3
void VecMatMultiply3(GLfloat* vec3, const ESMatrix3 mat3)
{
	GLfloat vecNew[3];
	vecNew[0] = vec3[0] * mat3.m[0][0] + vec3[1] * mat3.m[1][0] + vec3[2] * mat3.m[2][0];
	vecNew[1] = vec3[0] * mat3.m[0][1] + vec3[1] * mat3.m[1][1] + vec3[2] * mat3.m[2][1];
	vecNew[2] = vec3[0] * mat3.m[0][2] + vec3[1] * mat3.m[1][2] + vec3[2] * mat3.m[2][2];
	vec3[0] = vecNew[0];
	vec3[1] = vecNew[1];
	vec3[2] = vecNew[2];
}

// rotate along y axis, and change the position of eye ([x, y, z])
void Rotate(GLfloat* eye, GLfloat angle)
{
	// rotate along y axis mat
	ESMatrix3 mat3;
	mat3.m[0][0] = cos(angle * PI / 180.0f);
	mat3.m[0][1] = 0.0f;
	mat3.m[0][2] = -sin(angle * PI / 180.0f);
	mat3.m[1][0] = 0.0f;
	mat3.m[1][1] = 1.0f;
	mat3.m[1][2] = 0.0f;
	mat3.m[2][0] = sin(angle * PI / 180.0f);
	mat3.m[2][1] = 0.0f;
	mat3.m[2][2] = cos(angle * PI / 180.0f);

	VecMatMultiply3(eye, mat3);
}

// any time the position of eye changes, the direction (lookAt) needs to be changed also
void Redirection(GLfloat* lookAt, const GLfloat* target, const GLfloat* eye)
{
	lookAt[0] = target[0] - eye[0];
	lookAt[1] = target[1] - eye[1];
	lookAt[2] = target[2] - eye[2];
}

// 2d array should be used for OpenGL instead of OpenGLES, thus we need to transform 2d to 1d array for ES
// [[0.5, 0.5, 0.5], [-0.5, 0.5, 0.5]...] --> [0.5, 0.5, 0.5, -0.5, 0.5, 0.5...]
//void transVArr(const GLfloat** source, const int rowCount, GLfloat* ret)
//{
//
//}

// in a obj file, if a face part is like this:
// f 5/11/23 1/10/22 3/12/24 7/9/21
// which is more than three vertex. In this case, it represents two faces instead of one.
// this method is for getting a correct number of face.
int updatedFaceNum(const int* face_counts, const int f_num)
{
	int updatedFaceNum = f_num;
	for (int i = 0; i < f_num; i++)
	{
		if (face_counts[i] == 4)
			updatedFaceNum++;
	}
	return updatedFaceNum;
}