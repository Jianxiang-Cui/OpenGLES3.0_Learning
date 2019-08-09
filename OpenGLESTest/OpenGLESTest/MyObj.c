// The MIT License (MIT)
//
// Copyright (c) 2013 Dan Ginsburg, Budirijanto Purnomo
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

//
// Book:      OpenGL(R) ES 3.0 Programming Guide, 2nd Edition
// Authors:   Dan Ginsburg, Budirijanto Purnomo, Dave Shreiner, Aaftab Munshi
// ISBN-10:   0-321-93388-5
// ISBN-13:   978-0-321-93388-1
// Publisher: Addison-Wesley Professional
// URLs:      http://www.opengles-book.com
//            http://my.safaribooksonline.com/book/animation-and-3d/9780133440133
//
// Hello_Triangle.c
//
//    This is a simple example that draws a single triangle with
//    a minimal vertex/fragment shader.  The purpose of this
//    example is to demonstrate the basic concepts of
//    OpenGL ES 3.0 rendering.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "esUtil.h"
#include "myUtil.h"

#define PI 3.14159265



typedef struct
{
	// Handle to a program object
	GLuint programObject;

	// Uniform locations
	GLint  mvpLoc;

	// Rotation angle
	GLfloat angle;

} UserData;

struct
{
	int v_num;
	int vt_num;
	int vn_num;
	int f_num;
	int* face_counts; // [3,3,3,3,3,4,4,4,4,3...]
	int updated_face_num;

	GLfloat** vArr; //存放点的二维数组
	GLfloat** vnArr; //存放法线的二维数组
	int** fvArr; //存放面顶点的二维数组
	int** fnArr; //存放面法线的二维数组

	GLfloat* vertices; 
	GLuint* indices;
} ObjData;

struct 
{
	GLfloat* eye;
	GLfloat* target;
	GLfloat* lookAt;
	GLfloat* up;
	GLfloat  fov;
	GLfloat  aspect;
	ESMatrix  mvpMatrix;
} CameraData;



///
// Create a shader object, load the shader source, and
// compile the shader.
//
GLuint LoadShader(GLenum type, const char* shaderSrc)
{
	GLuint shader;
	GLint compiled;

	// Create the shader object
	shader = glCreateShader(type);

	if (shader == 0)
	{
		return 0;
	}

	// Load the shader source
	glShaderSource(shader, 1, &shaderSrc, NULL);

	// Compile the shader
	glCompileShader(shader);

	// Check the compile status
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);

	if (!compiled)
	{
		GLint infoLen = 0;

		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);

		if (infoLen > 1)
		{
			char* infoLog = malloc(sizeof(char) * infoLen);

			glGetShaderInfoLog(shader, infoLen, NULL, infoLog);
			esLogMessage("Error compiling shader:\n%s\n", infoLog);

			free(infoLog);
		}

		glDeleteShader(shader);
		return 0;
	}

	return shader;

}

void getDataNum(const char* filename, int* v_num, int* vt_num, int* vn_num, int* f_num)
{
	char line[1024];
	FILE* file;

	file = fopen(filename, "r");
	if (file == NULL)
	{
		printf("file cannot open \n");
	}
	while (!feof(file))
	{
		for (int i = 0; i < 1024; i++)
			line[i] = 0;
		fgets(line, 1024, file);

		// process file, consider the format is correct
		// (not including empty line and so on)
		if (line[0] == 'v')
		{
			if (line[1] == 'n')				// vn
			{
				ObjData.vn_num++;
			}
			else if (line[1] == 't')		// vt
			{
				ObjData.vt_num++;
			}
			else							// v
			{
				ObjData.v_num++;
			}
		}
		else if (line[0] == 'f')
		{
			ObjData.f_num++;
		}
	}
	fclose(file);
}

int LoadDataFromFile(const char* filename)
{
	char line[1024];
	FILE* file;

	// allocate memory to all arrays
	ObjData.vArr = (GLfloat * *)malloc(sizeof(GLfloat*) * ObjData.v_num);
	for (int i = 0; i < ObjData.v_num; i++)
	{
		ObjData.vArr[i] = (GLfloat*)malloc(sizeof(GLfloat) * 3);
	}

	ObjData.vnArr = (GLfloat * *)malloc(sizeof(GLfloat*) * ObjData.vn_num);
	for (int i = 0; i < ObjData.vn_num; i++)
	{
		ObjData.vnArr[i] = (GLfloat*)malloc(sizeof(GLfloat) * 3);
	}

	ObjData.fvArr = (int**)malloc(sizeof(int*) * ObjData.f_num);
	ObjData.fnArr = (int**)malloc(sizeof(int*) * ObjData.f_num);
	for (int i = 0; i < ObjData.f_num; i++)
	{
		// for the "f" part of ailian.obj, the max num of vertex an f contain is 4
		ObjData.fvArr[i] = (int*)malloc(sizeof(int) * 4);
		ObjData.fnArr[i] = (int*)malloc(sizeof(int) * 4);
	}

	file = fopen(filename, "r");
	if (file == NULL)
	{
		printf("file cannot open \n");
	}

	// store data to arrays
	int vnRow = 0, vRow = 0, fRow = 0;
	while (!feof(file))
	{
		fgets(line, 1024, file);

		char* token = NULL;
		char* next_token = NULL;
		token = strtok_s(line, " ", &next_token);
		int count = 0;

		

		if (line[0] == 'v')
		{
			if (line[1] == 'n')				// vn
			{
				printf("vn:\n");
				while (token != NULL && strcmp(token, "\n") != 0) // the last token would be "\n"
				{
					if (count > 0)
					{
						// filter the first token
						ObjData.vnArr[vnRow][count - 1] = atof(token);
						printf("%f\n", ObjData.vnArr[vnRow][count - 1]);
					}
					printf("\n");
					token = strtok_s(NULL, " ", &next_token);
					count++;
				}
				printf("\n");
				vnRow++;
			}
			else if (line[1] == 't')		// vt
			{

			}
			else							// v
			{
				printf("v:\n");
				while (token != NULL && strcmp(token, "\n") != 0) // the last token would be "\n"
				{
					if (count > 0)
					{
						ObjData.vArr[vRow][count - 1] = atof(token);
						printf("%f\n", ObjData.vArr[vRow][count - 1]);
					}
					token = strtok_s(NULL, " ", &next_token);
					count++;
					printf("\n");
				}
				vRow++;
			}
		}
		else if (line[0] == 'f') // fv, fn
		{
			// ["3/13/5", "4/14/6", "5/15/7" or "1/3" or "160//12"]
			
			while (token != NULL && strcmp(token, "\n") != 0) // the last token would be "\n"
			{
				int face_count = 0; // 0--"3" 1--"13" 2--"5"
				if (count > 0)
				{
					char* face_token = NULL;
					char* face_next_token = NULL;
					face_token = strtok_s(token, "/", &face_next_token);

					while (face_token != NULL)
					{
						if (face_count == 0) // fvArr
						{
							ObjData.fvArr[fRow][count - 1] = atoi(face_token);
							//printf("%s\n", face_token);
							printf("%d\n", ObjData.fvArr[fRow][count - 1]);
						} 
						else if (face_count == 1) // ftArr
						{

						}
						else if (face_count == 2) // fnArr
						{

						}

						//printf("%s\n", face_token);
						face_token = strtok_s(NULL, "/", &face_next_token);
						face_count++;
					}
				}
				token = strtok_s(NULL, " ", &next_token);
				count++;
				printf("\n");
			}

			ObjData.face_counts[fRow] = count - 1;
			fRow++;
		}
		printf("count: %d\n\n\n", count);
	}
	
	fclose(file);

	return 0;
}

// [[0.5, 0.5, 0.5], [-0.5, 0.5, 0.5]...] --> [0.5, 0.5, 0.5, -0.5, 0.5, 0.5...]
void transVArr(const GLfloat** source, GLfloat* ret)
{
	for (int i = 0; i < ObjData.v_num; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			ret[i * 3 + j] = source[i][j];
		}
	}
}

// [[7,1,5,N],     [7,1,5,
//  [5,1,3,7], -->  5,1,3, 5,3,7,
//  [4,8,6,N]]      4,8,6]
void transFvArr(const int** source, GLuint* ret)
{
	int row = 0;
	int capacity = ObjData.face_counts[row];
	int ret_index = 0;

	// ret_index < 3 * ObjData.updated_face_num
	for (; row < ObjData.f_num; row++, capacity += ObjData.face_counts[row])
	{
		if (ObjData.face_counts[row] == 3) // [7,1,5,N]
		{
			for (int i = 0; i < 3; i++, ret_index++)
			{
				ret[ret_index] = source[row][i] - 1;
			}
		}
		else if (ObjData.face_counts[row] == 4) // [5,1,3,7] --> [5,1,3, 5,3,7]
		{
			for (int i = 0; i < 3; i++, ret_index++) // [5,1,3]
			{
				ret[ret_index] = source[row][i] - 1;
			}
			// [5,3,7]
			ret[ret_index] = source[row][0] - 1;
			ret_index++;
			ret[ret_index] = source[row][2] - 1;
			ret_index++;
			ret[ret_index] = source[row][3] - 1;
			ret_index++;
		}
	}
}

///
// Initialize the shader and program object
//
int Init(ESContext* esContext)
{
	// init ObjData
	const char* filename = "stone.obj";
	getDataNum(filename, ObjData.v_num, ObjData.vt_num, ObjData.vn_num, ObjData.f_num);
	ObjData.vertices = malloc(sizeof(GLfloat) * 3 * ObjData.v_num);
	
	ObjData.face_counts = malloc(sizeof(int) * ObjData.f_num);

	LoadDataFromFile(filename);
	ObjData.updated_face_num = updatedFaceNum(ObjData.face_counts, ObjData.f_num);
	ObjData.indices = malloc(sizeof(GLuint) * 3 * ObjData.updated_face_num);

	transVArr(ObjData.vArr, ObjData.vertices);
	transFvArr(ObjData.fvArr, ObjData.indices);

	

	// print out data for testing
	//printf("v_num: %d\nvt_num: %d\nvu_num: %d\nf_num: %d\n", ObjData.v_num, ObjData.vt_num, ObjData.vn_num, ObjData.f_num);
	/*for (int i = 0; i < ObjData.updated_face_num; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			printf("%d\t", ObjData.indices[i*3+j]);
		}
		printf("\n");
	}*/
	/*for (int i = 0; i < ObjData.f_num; i++)
	{
		printf("%d\n", ObjData.face_counts[i]);
	}*/
	//printf("%d", ObjData.f_num);
	




	// init CameraData
	CameraData.aspect = (GLfloat)esContext->width / (GLfloat)esContext->height;
	CameraData.fov = 90.0f;
	CameraData.up = malloc(sizeof(float) * 3);
	CameraData.up[0] = 0.0f;
	CameraData.up[1] = 1.0f;
	CameraData.up[2] = 0.0f;
	CameraData.eye = malloc(sizeof(float) * 3);
	CameraData.eye[0] = 0.0f;
	CameraData.eye[1] = 0.0f;
	CameraData.eye[2] = 20.0f;
	CameraData.target = malloc(sizeof(float) * 3);
	CameraData.target[0] = 0.0f;
	CameraData.target[1] = 0.0f;
	CameraData.target[2] = 0.0f;
	CameraData.lookAt = malloc(sizeof(float) * 3);


	UserData* userData = esContext->userData;
	char vShaderStr[] =
		"#version 300 es                          \n"
		"layout(location = 0) in vec4 vPosition;  \n"
		"uniform mat4 mvpMatrix;				  \n"
		"void main()                              \n"
		"{                                        \n"
		"   gl_Position = mvpMatrix * vPosition;  \n"
		"}                                        \n";

	char fShaderStr[] =
		"#version 300 es                              \n"
		"precision mediump float;                     \n"
		"out vec4 fragColor;                          \n"
		"void main()                                  \n"
		"{                                            \n"
		"   fragColor = vec4 ( 1.0, 0.0, 0.0, 1.0 );  \n"
		"}                                            \n";

	GLuint vertexShader;
	GLuint fragmentShader;
	GLuint programObject;
	GLint linked;

	// Load the vertex/fragment shaders
	vertexShader = LoadShader(GL_VERTEX_SHADER, vShaderStr);
	fragmentShader = LoadShader(GL_FRAGMENT_SHADER, fShaderStr);

	// Create the program object
	programObject = glCreateProgram();

	if (programObject == 0)
	{
		return 0;
	}

	glAttachShader(programObject, vertexShader);
	glAttachShader(programObject, fragmentShader);

	// Link the program
	glLinkProgram(programObject);

	// Check the link status
	glGetProgramiv(programObject, GL_LINK_STATUS, &linked);

	if (!linked)
	{
		GLint infoLen = 0;

		glGetProgramiv(programObject, GL_INFO_LOG_LENGTH, &infoLen);

		if (infoLen > 1)
		{
			char* infoLog = malloc(sizeof(char) * infoLen);

			glGetProgramInfoLog(programObject, infoLen, NULL, infoLog);
			esLogMessage("Error linking program:\n%s\n", infoLog);

			free(infoLog);
		}

		glDeleteProgram(programObject);
		return FALSE;
	}

	// init OpenGL status
	glEnable(GL_DEPTH_TEST);
	/*glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glFrontFace(GL_CW);*/

	// Store the program object
	userData->programObject = programObject;
	
	userData->mvpLoc = glGetUniformLocation(userData->programObject, "mvpMatrix");

	userData->angle = 0.3f;


	glClearColor(1.0f, 1.0f, 1.0f, 0.0f);
	return TRUE;
}


void Update(ESContext* esContext, float deltaTime)
{
	UserData* userData = esContext->userData;
	ESMatrix perspective;
	ESMatrix modelview;
	ESMatrix eyeview;


	esMatrixLoadIdentity(&perspective);
	esMatrixLoadIdentity(&modelview);

	esPerspective(&perspective, CameraData.fov, CameraData.aspect, 1.0f, 250.0f);

	Rotate(CameraData.eye, userData->angle);
	Redirection(CameraData.lookAt, CameraData.target, CameraData.eye);

	esMatrixLookAt(&eyeview,
		CameraData.eye[0], CameraData.eye[1], CameraData.eye[2],
		CameraData.lookAt[0], CameraData.lookAt[1], CameraData.lookAt[2],
		CameraData.up[0], CameraData.up[1], CameraData.up[2]);

	esMatrixMultiply(&perspective, &eyeview, &perspective);

	//esTranslate(&modelview, 0.0, 0.0, -5.0);

	esMatrixMultiply(&CameraData.mvpMatrix, &modelview, &perspective);
}



///
// Draw a triangle using the shader pair created in Init()
//
void Draw(ESContext* esContext)
{
	UserData* userData = esContext->userData;
	GLfloat floorVertices[] =
	{
		-4.0f,	0.0f,	-0.4f,
		-4.0f,	0.0f,	4.0f,
		4.0f,	0.0f,	4.0f,

		4.0f,	0.0f,	4.0f,
		-4.0f,	0.0f,	4.0f,
		4.0f,	0.0f,	4.0f
	};

	// Set the viewport
	glViewport(0, 0, esContext->width, esContext->height);

	// Clear the color buffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Use the program object
	glUseProgram(userData->programObject);

	// Load the vertex data
	// draw ailian
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, ObjData.vertices);
	//glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, floorVertices);
	glEnableVertexAttribArray(0);

	glUniformMatrix4fv(userData->mvpLoc, 1, GL_FALSE, (GLfloat*)& CameraData.mvpMatrix.m[0][0]);

	glDrawElements(GL_TRIANGLES, 3*ObjData.updated_face_num, GL_UNSIGNED_INT, ObjData.indices);

	// draw floor
	//glDrawArrays(GL_TRIANGLES, 0, 3);
}

void Shutdown(ESContext* esContext)
{
	UserData* userData = esContext->userData;

	glDeleteProgram(userData->programObject);
}

int esMain(ESContext* esContext)
{
	esContext->userData = malloc(sizeof(UserData));

	esCreateWindow(esContext, "ailian", 960, 720, ES_WINDOW_RGB | ES_WINDOW_DEPTH);

	if (!Init(esContext))
	{
		return GL_FALSE;
	}

	esRegisterShutdownFunc(esContext, Shutdown);
	esRegisterUpdateFunc(esContext, Update);
	esRegisterDrawFunc(esContext, Draw);

	return GL_TRUE;
}




// back up

//void VecMatMultiply3(GLfloat* vec3, const ESMatrix3 mat3)
//{
//	GLfloat vecNew[3];
//	vecNew[0] = vec3[0] * mat3.m[0][0] + vec3[1] * mat3.m[1][0] + vec3[2] * mat3.m[2][0];
//	vecNew[1] = vec3[0] * mat3.m[0][1] + vec3[1] * mat3.m[1][1] + vec3[2] * mat3.m[2][1];
//	vecNew[2] = vec3[0] * mat3.m[0][2] + vec3[1] * mat3.m[1][2] + vec3[2] * mat3.m[2][2];
//	vec3[0] = vecNew[0];
//	vec3[1] = vecNew[1];
//	vec3[2] = vecNew[2];
//}

//void Rotate(GLfloat* eye, GLfloat angle)
//{
//	// rotate along y axis mat
//	ESMatrix3 mat3;
//	mat3.m[0][0] = cos(angle * PI / 180.0f);
//	mat3.m[0][1] = 0.0f;
//	mat3.m[0][2] = -sin(angle * PI / 180.0f);
//	mat3.m[1][0] = 0.0f;
//	mat3.m[1][1] = 1.0f;
//	mat3.m[1][2] = 0.0f;
//	mat3.m[2][0] = sin(angle * PI / 180.0f);
//	mat3.m[2][1] = 0.0f;
//	mat3.m[2][2] = cos(angle * PI / 180.0f);
//
//	VecMatMultiply3(eye, mat3);
//
//}

//void Redirection(GLfloat* lookAt, const GLfloat* target, const GLfloat* eye)
//{
//	lookAt[0] = target[0] - eye[0];
//	lookAt[1] = target[1] - eye[1];
//	lookAt[2] = target[2] - eye[2];
//}

//int updatedFaceNum(const int* face_counts, const int f_num)
//{
//	int updatedFaceNum = f_num;
//	for (int i = 0; i < f_num; i++)
//	{
//		if (face_counts[i] == 4)
//			updatedFaceNum++;
//	}
//	return updatedFaceNum;
//}