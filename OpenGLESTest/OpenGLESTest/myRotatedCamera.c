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
#include "esUtil.h"
#include <math.h>


#define NUM_FACES 6
#define PI 3.14159265

typedef struct
{
	// Handle to a program object
	GLuint programObject;

	// Uniform locations
	GLint  mvpLoc;

	// Vertex daata
	GLfloat* vertices;
	GLuint* indices;
	int       numIndices;

	// Rotation angle
	GLfloat   angle;

	// MVP matrix
	ESMatrix  mvpMatrix;

	// Cube vertices
	GLfloat* vVertices;
	GLfloat* vColors;

} UserData;

struct
{
	GLfloat* eye;
	GLfloat* target;
	GLfloat* lookat;
	GLfloat* up;
	GLfloat  fov;
	GLfloat  aspect;
	ESMatrix viewMat;
	ESMatrix projMat;
} CameraData;

GLfloat* subVector(GLfloat* v1, GLfloat* v2)
{
	GLfloat* vec3 = (GLfloat*)malloc(sizeof(GLfloat) * 3);
	vec3[0] = v1[0] - v2[0];
	vec3[1] = v1[1] - v2[1];
	vec3[2] = v1[2] - v2[2];
	return vec3;
}

GLfloat* addVector(GLfloat* v1, GLfloat* v2)
{
	GLfloat* vec3 = (GLfloat*)malloc(sizeof(GLfloat) * 3);
	vec3[0] = v1[0] + v2[0];
	vec3[1] = v1[1] + v2[1];
	vec3[2] = v1[2] + v2[2];
	return vec3;
}

GLfloat* normalizeVector(const GLfloat* v)
{
	GLfloat* vec3 = (GLfloat*)malloc(sizeof(GLfloat) * 3);
	float len = sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
	if (len > 0.00001)
	{
		vec3[0] = v[0] / len;
		vec3[1] = v[1] / len;
		vec3[2] = v[2] / len;
	}
	else
	{
		vec3[0] = 0.0f;
		vec3[1] = 0.0f;
		vec3[2] = 0.0f;
	}
	return vec3;
}

GLfloat* crossMutiVector(GLfloat* v1, GLfloat* v2)
{
	GLfloat* vec3 = (GLfloat*)malloc(sizeof(GLfloat) * 3);
	vec3[0] = v1[1] * v2[2] - v1[2] * v2[1];
	vec3[1] = v1[2] * v2[0] - v1[0] * v2[2];
	vec3[2] = v1[0] * v2[1] - v1[1] * v2[0];
	return vec3;
}

GLfloat dotMultiVector(const GLfloat* v1, const GLfloat* v2)
{
	GLfloat res = 0;
	for (int i = 0; i < 3; i++)
	{
		res += v1[i] * v2[i];
	}	
	return res;
}

ESMatrix transposeMatrix(ESMatrix mat)
{
	ESMatrix res;
	for (int i = 0; i < 4; i++) 
	{
		for (int j = 0; j < 4; j++) 
		{
			res.m[i][j] = mat.m[j][i];
		}
	}
	return res;
}

ESMatrix multiMatrix44(ESMatrix m1, ESMatrix m2)
{
	ESMatrix mat1 = transposeMatrix(m1);
	ESMatrix mat2 = transposeMatrix(m2);
	ESMatrix res;
	for (int i = 0; i < 4; i++) 
	{
		GLfloat* row = malloc(sizeof(GLfloat) * 4);
		row[0] = mat1.m[i][0];
		row[1] = mat1.m[i][1];
		row[2] = mat1.m[i][2];
		row[3] = mat1.m[i][3];
		//[mat1[i * 4], mat1[i * 4 + 1], mat1[i * 4 + 2], mat1[i * 4 + 3]];
		for (int j = 0; j < 4; j++) 
		{
			GLfloat* col = malloc(sizeof(GLfloat) * 4); 
			col[0] = mat2.m[0][j];
			col[1] = mat2.m[1][j];
			col[2] = mat2.m[2][j];
			col[3] = mat2.m[3][j];
			//[mat2[j], mat2[j + 4], mat2[j + 8], mat2[j + 12]];
			res.m[i][j] = dotMultiVector(row, col);
		}
	}
	return transposeMatrix(res);
}

ESMatrix setCamera(GLfloat* eye, GLfloat* target, GLfloat* up)
{
	GLfloat* N = (GLfloat*)malloc(sizeof(GLfloat) * 3);
	N = normalizeVector(subVector(target, eye));

	GLfloat* U = (GLfloat*)malloc(sizeof(GLfloat) * 3);
	U = normalizeVector(crossMutiVector(N, up));

	GLfloat* V = (GLfloat*)malloc(sizeof(GLfloat) * 3);
	V = crossMutiVector(U, N);

	ESMatrix R;
	R.m[0][0] = U[0]; R.m[0][1] = V[0]; R.m[0][2] = -N[0]; R.m[0][3] = 0.0f;
	R.m[1][0] = U[1]; R.m[1][1] = V[1]; R.m[1][2] = -N[1]; R.m[1][3] = 0.0f;
	R.m[2][0] = U[2]; R.m[2][1] = V[2]; R.m[2][2] = -N[2]; R.m[2][3] = 0.0f;
	R.m[3][0] = 0.0f; R.m[3][1] = 0.0f; R.m[3][2] = 0.0f; R.m[3][3] = 1.0f;

	ESMatrix T;
	esMatrixLoadIdentity(&T);
	esTranslate(&T, -CameraData.eye[0], -CameraData.eye[1], -CameraData.eye[2]);
	return multiMatrix44(R, T);
}


void VecMatMultiply(GLfloat* vec3, const ESMatrix3 mat3)
{
	GLfloat vecNew[3];
	vecNew[0] = mat3.m[0][0] * vec3[0] + mat3.m[0][1] * vec3[1] + mat3.m[0][2] * vec3[2];
	vecNew[1] = mat3.m[1][0] * vec3[0] + mat3.m[1][1] * vec3[1] + mat3.m[1][2] * vec3[2];
	vecNew[2] = mat3.m[2][0] * vec3[0] + mat3.m[2][1] * vec3[1] + mat3.m[2][2] * vec3[2];
	vec3[0] = vecNew[0];
	vec3[1] = vecNew[1];
	vec3[2] = vecNew[2];
}

void Rotate(GLfloat* eye, GLfloat* target, GLfloat* up, GLfloat angle)
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

	VecMatMultiply(eye, mat3);
}

void Redirection(GLfloat* target, const GLfloat* eye)
{
	target[0] = 0.0f - eye[0];
	target[1] = 0.0f - eye[1];
	target[2] = 0.0f - eye[2];
}

void VecMatMultiply4(GLfloat* vec4, ESMatrix mat4)
{
	vec4[0] = mat4.m[0][0] * vec4[0] + mat4.m[0][1] * vec4[1] + mat4.m[0][2] * vec4[2] + mat4.m[0][3] * vec4[3];
	vec4[1] = mat4.m[1][0] * vec4[0] + mat4.m[1][1] * vec4[1] + mat4.m[1][2] * vec4[2] + mat4.m[1][3] * vec4[3];
	vec4[2] = mat4.m[2][0] * vec4[0] + mat4.m[2][1] * vec4[1] + mat4.m[2][2] * vec4[2] + mat4.m[2][3] * vec4[3];
	vec4[3] = mat4.m[3][0] * vec4[0] + mat4.m[3][1] * vec4[1] + mat4.m[3][2] * vec4[2] + mat4.m[3][3] * vec4[3];
}






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

///
// Initialize the shader and program object
//
int Init(ESContext* esContext)
{
	
	UserData* userData = esContext->userData;
	char vShaderStr[] =
		"#version 300 es                          \n"
		"layout(location = 0) in vec4 vPosition;  \n"
		"layout(location = 1) in vec4 outColor;	  \n"
		"uniform mat4 mvpMatrix;				  \n"
		"out vec4 vColor;						  \n"
		"void main()                              \n"
		"{                                        \n"
		"   gl_Position = mvpMatrix*vPosition;    \n"
		"	vColor = outColor;					  \n"
		"}                                        \n";

	char fShaderStr[] =
		"#version 300 es                              \n"
		"precision mediump float;                     \n"
		"in vec4 vColor;							  \n"
		"out vec4 fragColor;                          \n"
		"void main()                                  \n"
		"{                                            \n"
		"   fragColor = vColor;  \n"
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

	// Store the program object
	userData->programObject = programObject;


	// my work
	userData->mvpLoc = glGetUniformLocation(userData->programObject, "mvpMatrix");
	userData->numIndices = 36;
	userData->angle = 0.0f;

	GLfloat vVertices[] = { 0.5f,	0.5f,	0.5f,
							-0.5f,	0.5f,	0.5f,
							-0.5f,	-0.5f,	0.5f,
							0.5f,	-0.5f,	0.5f,
							0.5f,	-0.5f,	-0.5f,
							0.5f,	0.5f,	-0.5f,
							-0.5f,	0.5f,	-0.5f,
							-0.5f,	-0.5f,	-0.5f
	};

	userData->vVertices = malloc(sizeof(float) * 24);
	for (size_t i = 0; i < 24; i++)
	{
		userData->vVertices[i] = vVertices[i];
	}
	
	ESMatrix perspective;
	ESMatrix modelview;
	float    aspect;




	CameraData.aspect = (GLfloat)esContext->width / (GLfloat)esContext->height;
	CameraData.fov = 60.0f;
	CameraData.up = malloc(sizeof(float) * 3);
	CameraData.up[0] = 0.0f;
	CameraData.up[1] = 1.0f;
	CameraData.up[2] = 0.0f;
	CameraData.eye = malloc(sizeof(float) * 3);
	CameraData.eye[0] = 0.0f;
	CameraData.eye[1] = 0.0f;
	CameraData.eye[2] = 1.0f;
	CameraData.target = malloc(sizeof(float) * 3);
	CameraData.target[0] = 0.0f;
	CameraData.target[1] = 0.0f;
	CameraData.target[2] = -5.0f;


	glClearColor(1.0f, 1.0f, 1.0f, 0.0f);
	return TRUE;
}

void Update(ESContext* esContext, float deltaTime)
{
	UserData* userData = esContext->userData;
	ESMatrix perspective;
	ESMatrix modelview;
	ESMatrix eyeview;
	ESMatrix cameraMat;

	userData->angle = 3.0f;

	// Generate a perspective matrix with a 60 degree FOV
	esMatrixLoadIdentity(&perspective);
	esPerspective(&perspective, CameraData.fov, CameraData.aspect, 1.0f, 200.0f);

	Rotate(CameraData.eye, CameraData.target, CameraData.up, userData->angle);
	Redirection(CameraData.target, CameraData.eye);


	//printf("%f\n", sizeof(CameraData.eye)); 
	
	
	for (int i = 0; i < 3; i++)
	{
		printf("%f\t", CameraData.eye[i]);
	}
	printf("// eye position\n---------------------------\n");

	for (int i = 0; i < 3; i++)
	{
		printf("%f\t", CameraData.target[i]);
	}
	printf("// direction vector\n---------------------------\n\n\n");

	esMatrixLookAt(&eyeview,
		CameraData.eye[0], CameraData.eye[1], CameraData.eye[2],
		CameraData.target[0], CameraData.target[1], CameraData.target[2],
		CameraData.up[0], CameraData.up[1], CameraData.up[2]);

	esMatrixMultiply(&perspective, &eyeview, &perspective);

	// Generate a model view matrix to rotate/translate the cube
	esMatrixLoadIdentity(&modelview);

	// Translate away from the viewer
	esTranslate(&modelview, 0.0, 0.0, -5.0);

	// Rotate the cube
	//esRotate(&modelview, userData->angle, 1.0, 1.0, 0.0);

	// Compute the final MVP by multiplying the
	// modevleiw and perspective matrices together
	esMatrixMultiply(&userData->mvpMatrix, &modelview, &perspective);
}

///
// Draw a triangle using the shader pair created in Init()
//
void Draw(ESContext* esContext)
{
	UserData* userData = esContext->userData;
	GLfloat vVertices[] = { 
	  -0.5f, -0.5f, -0.5f,
	  -0.5f, -0.5f,  0.5f,
	  0.5f, -0.5f,  0.5f,
	  0.5f, -0.5f, -0.5f,
	  -0.5f,  0.5f, -0.5f,
	  -0.5f,  0.5f,  0.5f,
	  0.5f,  0.5f,  0.5f,
	  0.5f,  0.5f, -0.5f,
	  -0.5f, -0.5f, -0.5f,
	  -0.5f,  0.5f, -0.5f,
	  0.5f,  0.5f, -0.5f,
	  0.5f, -0.5f, -0.5f,
	  -0.5f, -0.5f, 0.5f,
	  -0.5f,  0.5f, 0.5f,
	  0.5f,  0.5f, 0.5f,
	  0.5f, -0.5f, 0.5f,
	  -0.5f, -0.5f, -0.5f,
	  -0.5f, -0.5f,  0.5f,
	  -0.5f,  0.5f,  0.5f,
	  -0.5f,  0.5f, -0.5f,
	  0.5f, -0.5f, -0.5f,
	  0.5f, -0.5f,  0.5f,
	  0.5f,  0.5f,  0.5f,
	  0.5f,  0.5f, -0.5f,
	};

	GLfloat	vColors[] = {	1.0f,	0.0f,	0.0f, 1.0f,
							1.0f,	0.0f,	0.0f, 1.0f,
							1.0f,	0.0f,	0.0f, 1.0f,
							1.0f,	0.0f,	0.0f, 1.0f,
												  
							0.0f,	1.0f,	0.0f, 1.0f,
							0.0f,	1.0f,	0.0f, 1.0f,
							0.0f,	1.0f,	0.0f, 1.0f,
							0.0f,	1.0f,	0.0f, 1.0f,
												  
							0.0f,	0.0f,	1.0f, 1.0f,
							0.0f,	0.0f,	1.0f, 1.0f,
							0.0f,	0.0f,	1.0f, 1.0f,
							0.0f,	0.0f,	1.0f, 1.0f,
												  
							0.0f,	1.0f,	1.0f, 1.0f,
							0.0f,	1.0f,	1.0f, 1.0f,
							0.0f,	1.0f,	1.0f, 1.0f,
							0.0f,	1.0f,	1.0f, 1.0f,
												  
							1.0f,	1.0f,	0.0f, 1.0f,
							1.0f,	1.0f,	0.0f, 1.0f,
							1.0f,	1.0f,	0.0f, 1.0f,
							1.0f,	1.0f,	0.0f, 1.0f,
												  
							1.0f,	0.0f,	1.0f, 1.0f,
							1.0f,	0.0f,	1.0f, 1.0f,
							1.0f,	0.0f,	1.0f, 1.0f,
							1.0f,	0.0f,	1.0f, 1.0f,
	};
	/*GLfloat vVertices[24] = { 0.0f,	0.5f,	0.705f,
							-0.705f,	0.5f,	0.0f,
							-0.705f,	-0.5f,	0.0f,
							0.0f,	-0.5f,	0.705f,
							0.705f,	-0.5f,	-0.0f,
							0.705f,	0.5f,	-0.0f,
							0.0f,	0.5f,	-0.705f,
							0.0f,	-0.5f,	-0.705f
	};*/

	/*GLubyte triangle_indices[] = {
		0, 1, 1, 2, 2, 3, 3, 0,
		0, 3, 3, 4, 4, 5, 5, 0,
		0, 5, 5, 6, 6, 1, 1, 0,
		7, 6, 6, 1, 1, 2, 2, 7,
		7, 4, 4, 5, 5, 6, 6, 7,
		7, 2, 2, 3, 3, 4, 4, 7
	};*/
	GLuint triangle_indices[] = {
		 0, 2, 1,
		 0, 3, 2,
		 4, 5, 6,
		 4, 6, 7,
		 8, 9, 10,
		 8, 10, 11,
		 12, 15, 14,
		 12, 14, 13,
		 16, 17, 18,
		 16, 18, 19,
		 20, 23, 22,
		 20, 22, 21
	};


	// Set the viewport
	glViewport(0, 0, esContext->width, esContext->height);

	// Clear the color buffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	// Use the program object
	glUseProgram(userData->programObject);
	

	// Load the vertex data
	glEnable(GL_DEPTH_TEST);
	//glEnable(GL_BLEND);
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, vVertices);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, vColors);
	glEnableVertexAttribArray(1);

	glUniformMatrix4fv(userData->mvpLoc, 1, GL_FALSE, (GLfloat*)& userData->mvpMatrix.m[0][0]);

	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, triangle_indices);
	//glDrawElements(GL_LINES, 48, GL_UNSIGNED_BYTE, triangle_indices);
}

void Shutdown(ESContext* esContext)
{
	UserData* userData = esContext->userData;

	glDeleteProgram(userData->programObject);
}

int esMain(ESContext* esContext)
{
	esContext->userData = malloc(sizeof(UserData));

	esCreateWindow(esContext, "Hello Triangle", 960, 640, ES_WINDOW_RGB | ES_WINDOW_ALPHA | ES_WINDOW_DEPTH);

	if (!Init(esContext))
	{
		return GL_FALSE;
	}

	esRegisterShutdownFunc(esContext, Shutdown);
	esRegisterUpdateFunc(esContext, Update);
	esRegisterDrawFunc(esContext, Draw);

	return GL_TRUE;
}
