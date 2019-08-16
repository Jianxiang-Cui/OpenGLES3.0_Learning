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
	GLuint sceneProgramObject;
	GLuint shadowMapProgramObject;

	// Uniform locations
	GLint  sceneMvpLoc;
	GLint  shadowMapMvpLoc;
	GLint  sceneMvpLightLoc;
	GLint  shadowMapMvpLightLoc;

	// Sampler location
	GLint shadowMapSamplerLoc;

	// shadow map Texture handle
	GLuint shadowMapTextureId;
	GLuint shadowMapBufferId;
	GLuint shadowMapTextureWidth;
	GLuint shadowMapTextureHeight;

	// Rotation angle
	GLfloat angle;

	// Sampler locations
	GLint ailianMapLoc;

	// Texture handle
	GLuint ailianMapTexId;
	GLuint floorMapTexId;

	ESMatrix  mvpLightMatrix;
	float lightPosition[3];
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
	GLfloat** vtArr; //存放纹理坐标的二维数组
	GLfloat** vnArr; //存放法线的二维数组
	int** fvArr; //存放面顶点的二维数组
	int** ftArr; //存放纹理坐标的二维数组
	int** fnArr; //存放面法线的二维数组

	GLfloat* vertices;
	GLuint* indices;

	GLfloat* updatedVertices;
	GLfloat* texCoords;
	GLuint* texIndices;

	GLfloat* updatedTexCoords;
	GLfloat* updatedAgainVertices;

	GLfloat* midbot;
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


void GetDataNum(const char* filename, int* v_num, int* vt_num, int* vn_num, int* f_num)
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

	ObjData.vtArr = (GLfloat * *)malloc(sizeof(GLfloat*) * ObjData.vt_num);
	for (int i = 0; i < ObjData.vt_num; i++)
	{
		ObjData.vtArr[i] = (GLfloat*)malloc(sizeof(GLfloat) * 2); // ignore the third value of vt
	}
	ObjData.texCoords = (GLfloat*)malloc(sizeof(GLfloat) * 2 * ObjData.vt_num);

	ObjData.vnArr = (GLfloat * *)malloc(sizeof(GLfloat*) * ObjData.vn_num);
	for (int i = 0; i < ObjData.vn_num; i++)
	{
		ObjData.vnArr[i] = (GLfloat*)malloc(sizeof(GLfloat) * 3);
	}

	ObjData.fvArr = (int**)malloc(sizeof(int*) * ObjData.f_num);
	ObjData.ftArr = (int**)malloc(sizeof(int*) * ObjData.f_num);
	ObjData.fnArr = (int**)malloc(sizeof(int*) * ObjData.f_num);
	for (int i = 0; i < ObjData.f_num; i++)
	{
		// for the "f" part of ailian.obj, the max num of vertex an f contain is 4
		ObjData.fvArr[i] = (int*)malloc(sizeof(int) * 4);
		ObjData.ftArr[i] = (int*)malloc(sizeof(int) * 4);
		ObjData.fnArr[i] = (int*)malloc(sizeof(int) * 4);
	}

	file = fopen(filename, "r");
	if (file == NULL)
	{
		printf("file cannot open \n");
	}

	// store data to arrays
	int vnRow = 0, vtRow = 0, vRow = 0, fRow = 0;
	int vtIndex = 0;
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
				//printf("vn:\n");
				while (token != NULL && strcmp(token, "\n") != 0) // the last token would be "\n"
				{
					if (count > 0)
					{
						// filter the first token
						ObjData.vnArr[vnRow][count - 1] = atof(token);
						//printf("%f\n", ObjData.vnArr[vnRow][count - 1]);
					}
					//printf("\n");
					token = strtok_s(NULL, " ", &next_token);
					count++;
				}
				//printf("\n");
				vnRow++;
			}
			else if (line[1] == 't')		// vt
			{
				// sometimes vt has three value, we ignore the third in this case
				// [ 0.01992, 0.01992,
				//   0.998,   0.998,
				//   ...              ]
				while (token != NULL && strcmp(token, "\n") != 0) // the last token would be "\n"
				{
					if (count > 0 && count < 3)
					{
						// filter the first token and ignore the third value of vt
						ObjData.texCoords[vtIndex] = atof(token);
						ObjData.vtArr[vtRow][count - 1] = atof(token);
						vtIndex++;
						//printf("%f\n", ObjData.vnArr[vnRow][count - 1]);
					}
					//printf("\n");
					token = strtok_s(NULL, " ", &next_token);
					count++;
				}
				vtRow++;
			}
			else							// v
			{
				//printf("v:\n");
				while (token != NULL && strcmp(token, "\n") != 0) // the last token would be "\n"
				{
					if (count > 0)
					{
						ObjData.vArr[vRow][count - 1] = atof(token);
						//printf("%f\n", ObjData.vArr[vRow][count - 1]);
					}
					token = strtok_s(NULL, " ", &next_token);
					count++;
					//printf("\n");
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
							//printf("%d\n", ObjData.fvArr[fRow][count - 1]);
						}
						else if (face_count == 1) // ftArr
						{
							ObjData.ftArr[fRow][count - 1] = atoi(face_token);
						}
						else if (face_count == 2) // fnArr
						{

						}

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
		//printf("count: %d\n\n\n", count);
	}

	fclose(file);

	return 0;
}

// [[0.5, 0.5, 0.5], [-0.5, 0.5, 0.5]...] --> [0.5, 0.5, 0.5, -0.5, 0.5, 0.5...]
void TransVArr(const GLfloat** source, GLfloat* ret)
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
void TransFArr(const int** source, GLuint* ret)
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

void UpdateVertices(GLfloat* ret)
{
	for (int i = 0; i < 3 * ObjData.updated_face_num; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			ret[ObjData.texIndices[i] * 3 + j] = ObjData.vertices[ObjData.indices[i] * 3 + j];
		}
	}
}

// rearrange vetices again cuz the pair, updatedVertices -- texCoords -- texIndices
// causes part of vetices lost
void UpdatedAgainVertices()
{
	for (int i = 0; i < ObjData.updated_face_num * 3; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			ObjData.updatedAgainVertices[i * 3 + j] = ObjData.vArr[ObjData.indices[i]][j];
		}
		for (int j = 0; j < 2; j++)
		{
			ObjData.updatedTexCoords[i * 2 + j] = ObjData.vtArr[ObjData.texIndices[i]][j];
		}
	}
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

InitMVP(ESContext* esContext)
{
	UserData* userData = esContext->userData;
	ESMatrix perspective;
	ESMatrix ortho;
	ESMatrix modelview;
	ESMatrix model;
	ESMatrix view;
	

	// Generate an orthographic projection matrix for the shadow map rendering
	esMatrixLoadIdentity(&ortho);
	esOrtho(&ortho, -10, 10, -10, 10, -30, 30);

	// GROUND
   // Generate a model view matrix to rotate/translate the ground
	esMatrixLoadIdentity(&model);

	// Center the ground
	esTranslate(&model, 0.0f, 0.0f, -2.0f);

	// create view matrix transformation from the light position
	esMatrixLookAt(&view,
		userData->lightPosition[0], userData->lightPosition[1], userData->lightPosition[2],
		0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f);
	esMatrixMultiply(&modelview, &model, &view);

	esMatrixMultiply(&userData->mvpLightMatrix, &modelview, &ortho);

	return TRUE;
}

int InitShadowMap(ESContext* esContext)
{
	UserData* userData = esContext->userData;
	GLenum none = GL_NONE;
	GLint defaultFramebuffer = 0;

	// use 1K by 1K texture for shadow map
	userData->shadowMapTextureWidth = userData->shadowMapTextureHeight = 1024;

	glGenTextures(1, &userData->shadowMapTextureId);
	glBindTexture(GL_TEXTURE_2D, userData->shadowMapTextureId);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	// Setup hardware comparison
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24,
		userData->shadowMapTextureWidth, userData->shadowMapTextureHeight,
		0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, NULL);

	glBindTexture(GL_TEXTURE_2D, 0);

	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &defaultFramebuffer);

	// setup fbo
	glGenFramebuffers(1, &userData->shadowMapBufferId);
	glBindFramebuffer(GL_FRAMEBUFFER, userData->shadowMapBufferId);

	glDrawBuffers(1, &none);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, userData->shadowMapTextureId, 0);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, userData->shadowMapTextureId);

	if (GL_FRAMEBUFFER_COMPLETE != glCheckFramebufferStatus(GL_FRAMEBUFFER))
	{
		return FALSE;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebuffer);

	return TRUE;
}

///
// Initialize the shader and program object
//
int Init(ESContext* esContext)
{
	UserData* userData = esContext->userData;

	// init ObjData

	const char* filename = "bear.obj";
	GetDataNum(filename, ObjData.v_num, ObjData.vt_num, ObjData.vn_num, ObjData.f_num);
	ObjData.vertices = malloc(sizeof(GLfloat) * 3 * ObjData.v_num);

	ObjData.face_counts = malloc(sizeof(int) * ObjData.f_num);

	LoadDataFromFile(filename);
	ObjData.updated_face_num = updatedFaceNum(ObjData.face_counts, ObjData.f_num);

	ObjData.indices = malloc(sizeof(GLuint) * 3 * ObjData.updated_face_num);

	TransVArr(ObjData.vArr, ObjData.vertices);
	TransFArr(ObjData.fvArr, ObjData.indices);


	ObjData.texIndices = malloc(sizeof(GLuint) * 3 * ObjData.updated_face_num);
	ObjData.updatedVertices = malloc(sizeof(GLfloat) * 3 * ObjData.vt_num);
	TransFArr(ObjData.ftArr, ObjData.texIndices);
	UpdateVertices(ObjData.updatedVertices);


	ObjData.updatedAgainVertices = malloc(sizeof(GLfloat) * 3 * 3 * ObjData.updated_face_num); // three float num makes a vertex, three vertices make a face
	ObjData.updatedTexCoords = malloc(sizeof(int) * 2 * 3 * ObjData.updated_face_num); // two float num makes a texcoord, three texcoords make a face

	UpdatedAgainVertices();

	ObjData.midbot = malloc(sizeof(GLfloat) * 3);
	GetMidBotPosition(ObjData.vArr, ObjData.v_num, ObjData.midbot);

	// print out data for testing
	/*for (int i = 0; i < 3 * ObjData.updated_face_num; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			printf("%f\t", ObjData.updatedAgainVertices[i * 3 + j]);
		}
		printf("\n");
	}*/



	// init CameraData
	CameraData.aspect = (GLfloat)esContext->width / (GLfloat)esContext->height;
	CameraData.fov = 90.0f;
	CameraData.up = malloc(sizeof(float) * 3);
	CameraData.up[0] = 0.0f;
	CameraData.up[1] = 1.0f;
	CameraData.up[2] = 0.0f;
	CameraData.eye = malloc(sizeof(float) * 3);
	CameraData.eye[0] = 0.0f;
	CameraData.eye[1] = 2.0f;
	CameraData.eye[2] = 5.0f;
	CameraData.target = malloc(sizeof(float) * 3);
	CameraData.target[0] = 0.0f;
	CameraData.target[1] = 0.0f;
	CameraData.target[2] = 0.0f;
	CameraData.lookAt = malloc(sizeof(float) * 3);

	// init light data
	userData->lightPosition[0] = 7.0f;
	userData->lightPosition[1] = 6.0f;
	userData->lightPosition[2] = 3.0f;
	
	const char vShadowMapShaderStr[] =
		"#version 300 es                                  \n"
		"uniform mat4 mvpMatrix;						  \n"
		"uniform mat4 u_mvpLightMatrix;                   \n"
		"layout(location = 0) in vec4 a_position;         \n"
		"out vec4 v_color;                                \n"
		"void main()                                      \n"
		"{                                                \n"
		"   gl_Position = u_mvpLightMatrix * a_position;  \n"
		"}                                                \n";

	const char fShadowMapShaderStr[] =
		"#version 300 es                                  \n"
		"precision lowp float;                            \n"
		"void main()                                      \n"
		"{                                                \n"
		"}                                                \n";
	


	char vSceneShaderStr[] =
		"#version 300 es                          \n"
		"layout(location = 0) in vec4 vPosition;  \n"
		"layout(location = 1) in vec2 a_texCoord; \n"
		"out vec2 v_texCoord;					  \n"
		"out vec4 v_shadowCoord;                  \n"
		"uniform mat4 mvpMatrix;				  \n"
		"uniform mat4 u_mvpLightMatrix;           \n"
		"void main()                              \n"
		"{                                        \n"
		"   gl_Position = mvpMatrix * vPosition;  \n"
		"   v_texCoord = a_texCoord;			  \n"
		"   v_shadowCoord = u_mvpLightMatrix * vPosition; \n"
		"                                                  \n"
		"   // transform from [-1,1] to [0,1];             \n"
		"   v_shadowCoord = v_shadowCoord * 0.5 + 0.5;     \n"
		"}                                        \n";

	char fSceneShaderStr[] =
		"#version 300 es                              \n"
		"precision lowp float;                     \n"
		"uniform lowp sampler2DShadow s_shadowMap;    \n"
		"uniform sampler2D s_ailianMap;				  \n"
		"in vec2 v_texCoord;						  \n"
		"in vec4 v_shadowCoord;                       \n"
		"layout(location = 0) out vec4 fragColor;     \n"
		"                                             \n"
		"float lookup ( float x, float y )                              \n"
		"{                                                              \n"
		"   float pixelSize = 0.002; // 1/500                           \n"
		"   vec4 offset = vec4 ( x * pixelSize * v_shadowCoord.w,       \n"
		"                        y * pixelSize * v_shadowCoord.w,       \n"
		"                        -0.005 * v_shadowCoord.w, 0.0 );       \n"
		"   return textureProj ( s_shadowMap, v_shadowCoord + offset ); \n"
		"}                                                              \n"
		"                                                               \n"
		"void main()                                  \n"
		"{                                            \n"
		"   // 3x3 kernel with 4 taps per sample, effectively 6x6 PCF   \n"
		"   float sum = 0.0;                                            \n"
		"   float x, y;                                                 \n"
		"   for ( x = -2.0; x <= 2.0; x += 2.0 )                        \n"
		"      for ( y = -2.0; y <= 2.0; y += 2.0 )                     \n"
		"         sum += lookup ( x, y );                               \n"
		"                                                               \n"
		"   // divide sum by 9.0                                        \n"
		"   sum = sum * 0.11;                                           \n"
		"   fragColor = texture(s_ailianMap, v_texCoord) * sum;  \n"
		"}                                            \n";

	GLuint vertexShader;
	GLuint fragmentShader;
	GLuint sceneProgramObject;
	GLint linked;


	// handle ShadowMap Program
	// Load the shaders and get a linked program object
	userData->shadowMapProgramObject = esLoadProgram(vShadowMapShaderStr, fShadowMapShaderStr);
	userData->shadowMapMvpLoc = glGetUniformLocation(userData->shadowMapProgramObject, "mvpMatrix");
	userData->shadowMapMvpLightLoc = glGetUniformLocation(userData->shadowMapProgramObject, "u_mvpLightMatrix");
	
	// Get the sampler location
	userData->shadowMapSamplerLoc = glGetUniformLocation(userData->shadowMapProgramObject, "s_shadowMap");

	// Load the vertex/fragment shaders
	vertexShader = LoadShader(GL_VERTEX_SHADER, vSceneShaderStr);
	fragmentShader = LoadShader(GL_FRAGMENT_SHADER, fSceneShaderStr);

	// Create the program object
	sceneProgramObject = glCreateProgram();

	if (sceneProgramObject == 0)
	{
		return 0;
	}

	glAttachShader(sceneProgramObject, vertexShader);
	glAttachShader(sceneProgramObject, fragmentShader);

	// Link the program
	glLinkProgram(sceneProgramObject);

	// Check the link status
	glGetProgramiv(sceneProgramObject, GL_LINK_STATUS, &linked);

	if (!linked)
	{
		GLint infoLen = 0;

		glGetProgramiv(sceneProgramObject, GL_INFO_LOG_LENGTH, &infoLen);

		if (infoLen > 1)
		{
			char* infoLog = malloc(sizeof(char) * infoLen);

			glGetProgramInfoLog(sceneProgramObject, infoLen, NULL, infoLog);
			esLogMessage("Error linking program:\n%s\n", infoLog);

			free(infoLog);
		}

		glDeleteProgram(sceneProgramObject);
		return FALSE;
	}

	

	// Store the program object
	userData->sceneProgramObject = sceneProgramObject;

	userData->sceneMvpLoc = glGetUniformLocation(userData->sceneProgramObject, "mvpMatrix");
	userData->sceneMvpLightLoc = glGetUniformLocation(userData->sceneProgramObject, "u_mvpLightMatrix");

	userData->angle = 0.3f;

	userData->ailianMapLoc = glGetUniformLocation(userData->sceneProgramObject, "s_ailianMap");

	userData->ailianMapTexId = LoadTexture(esContext->platformData, "bear.tga");

	if (userData->ailianMapTexId == 0)
	{
		return FALSE;
	}

	// create depth texture
	if (!InitShadowMap(esContext))
	{
		return FALSE;
	}

	// init OpenGL status
	glEnable(GL_DEPTH_TEST);
	// disable culling
	glDisable(GL_CULL_FACE);

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

// based on the origin, draw a square plat floor with given side length 
void DrawFloor(ESContext* esContext, const GLfloat* origin, const GLfloat length)
{
	UserData* userData = esContext->userData;

	GLfloat originX = origin[0];
	GLfloat originY = origin[1];
	GLfloat originZ = origin[2];
	GLfloat floorVertices[] =
	{
		originX + length,	originY,	originZ - length,
		originX - length,	originY,	originZ - length,
		originX - length,	originY,	originZ + length,

		originX + length,	originY,	originZ - length,
		originX - length,	originY,	originZ + length,
		originX + length,	originY,	originZ + length
	};
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, floorVertices);

	userData->floorMapTexId = CreateSimpleTexture2D();

	if (userData->floorMapTexId == 0)
	{
		return FALSE;
	}

	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, floorVertices);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, userData->floorMapTexId);

	glUniform1i(userData->ailianMapLoc, 0);

	glDrawArrays(GL_TRIANGLES, 0, 6);
}

void DrawScene(ESContext* esContext, GLint mvpLoc, GLint mvpLightLoc)
{
	UserData* userData = esContext->userData;

	// draw model
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, ObjData.updatedAgainVertices);
	glEnableVertexAttribArray(0);

	glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, (GLfloat*)& CameraData.mvpMatrix.m[0][0]);
	glUniformMatrix4fv(mvpLightLoc, 1, GL_FALSE, (GLfloat*)& userData->mvpLightMatrix.m[0][0]);

	//// stick texture on ailian
	//glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, ObjData.updatedTexCoords);
	//glEnableVertexAttribArray(1);

	//glActiveTexture(GL_TEXTURE0);
	//glBindTexture(GL_TEXTURE_2D, userData->ailianMapTexId);

	//glUniform1i(userData->ailianMapLoc, 0);


	////glDrawElements(GL_TRIANGLES, 3 * ObjData.updated_face_num, GL_UNSIGNED_INT, ObjData.texIndices);

	//glDrawArrays(GL_TRIANGLES, 0, 3 * ObjData.updated_face_num);
	//glBindTexture(GL_TEXTURE_2D, 0); // clear GL_TEXTURE_2D status

	//// draw floor
	//DrawFloor(esContext, ObjData.midbot, 4.0f);
}

///
// Draw a triangle using the shader pair created in Init()
//
void Draw(ESContext* esContext)
{
	UserData* userData = esContext->userData;
	GLint defaultFramebuffer = 0;

	InitMVP(esContext);

	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &defaultFramebuffer);

	// FIRST PASS: Render the scene from light position to generate the shadow map texture
	glBindFramebuffer(GL_FRAMEBUFFER, userData->shadowMapBufferId);

	// Set the viewport
	glViewport(0, 0, userData->shadowMapTextureWidth, userData->shadowMapTextureHeight);

	// clear depth buffer
	glClear(GL_DEPTH_BUFFER_BIT);

	// disable color rendering, only write to depth buffer
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

	// reduce shadow rendering artifact
	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(5.0f, 100.0f);

	glUseProgram(userData->shadowMapProgramObject);
	DrawScene(esContext, userData->shadowMapMvpLoc, userData->shadowMapMvpLightLoc);

	glDisable(GL_POLYGON_OFFSET_FILL);






	// SECOND PASS: Render the scene from eye location using the shadow map texture created in the first pass
	glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebuffer);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

	// Set the viewport
	glViewport(0, 0, esContext->width, esContext->height);

	// Clear the color buffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Use the program object
	glUseProgram(userData->sceneProgramObject);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, userData->shadowMapTextureId);
	glUniform1i(userData->shadowMapSamplerLoc, 1);

	// Load the vertex data

	// corresponding relation
	// updatedVertices -- texCoords -- texIndices
	// updatedAgainVertices -- updatedTexCoords -- glDrawArrays


	//// draw model
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, ObjData.updatedAgainVertices);
	glEnableVertexAttribArray(0);

	glUniformMatrix4fv(userData->sceneMvpLoc, 1, GL_FALSE, (GLfloat*)& CameraData.mvpMatrix.m[0][0]);
	glUniformMatrix4fv(userData->sceneMvpLightLoc, 1, GL_FALSE, (GLfloat*)& userData->mvpLightMatrix.m[0][0]);

	// stick texture on ailian
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, ObjData.updatedTexCoords);
	glEnableVertexAttribArray(1);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, userData->ailianMapTexId);

	glUniform1i(userData->ailianMapLoc, 0);
	

	//glDrawElements(GL_TRIANGLES, 3 * ObjData.updated_face_num, GL_UNSIGNED_INT, ObjData.texIndices);

	glDrawArrays(GL_TRIANGLES, 0, 3 * ObjData.updated_face_num);
	glBindTexture(GL_TEXTURE_2D, 0); // clear GL_TEXTURE_2D status

	

	// draw floor
	DrawFloor(esContext, ObjData.midbot, 4.0f);
}

void Shutdown(ESContext* esContext)
{
	UserData* userData = esContext->userData;

	glDeleteProgram(userData->sceneProgramObject);
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