/*
  Michael Eggers
  December 2019

  This program demonstrates how to import a model (OBJ) via Assimp and render it through OpenGL.
  The purpose is to help understand the logical flow of OpenGL, from initialization, setting up
  buffers, shaders, upload vertex data onto the GPU, update shaders and, finally, drawing.

  --------------------------------------------------------------------------------------------------------
  TODOs:
   This program lacks time measurement, so the model animation will run slower/faster
   across different hardware-configurations.

   Resizing the window won't update glViewport and recalculate the frustum given to glm::perspective.
  --------------------------------------------------------------------------------------------------------
  
  Windows only. In order to port it to Linux you just have to replace the read_file function.
  You can use std::filesystem for that or the standard c library.
  For Mac, several more steps have to be done throughout the application code and shader-code.
  
  Questions or mistakes in the code, please report to: eggers@hm.edu! Thank you :)
*/

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <assimp/cimport.h>        // Plain-C interface
#include <assimp/scene.h>          // Output data structure
#include <assimp/postprocess.h>    // Post processing flags

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define WINDOW_WIDTH	1920
#define WINDOW_HEIGHT   1080

struct Vertex
{
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 uv;
};

struct Model
{
	Vertex * vertices;
	uint32_t vertex_count;
	glm::vec3 position;
	glm::mat4 model_matrix;
};

/* Loads textfile from disk. */
char * read_file(char const * file)
{
	char* result = 0;
	HANDLE fileHandle;
	fileHandle = CreateFile(file,
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);
	if (fileHandle == INVALID_HANDLE_VALUE)
		printf("unable to open file!\n");

	DWORD filesize = GetFileSize(fileHandle, NULL);

	char *buffer = (char*)VirtualAlloc(
		NULL,
		filesize * sizeof(char),
		MEM_RESERVE | MEM_COMMIT,
		PAGE_READWRITE
	);

	_OVERLAPPED ov = { 0 };
	DWORD numBytesRead = 0;
	DWORD error;
	if (ReadFile(fileHandle, buffer, filesize, &numBytesRead, NULL) == 0)
	{
		error = GetLastError();
		char errorMsgBuf[256];
		FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			errorMsgBuf, (sizeof(errorMsgBuf) / sizeof(char)), NULL);
		printf("%s\n", errorMsgBuf);
	}
	else
	{
		//buffer[filesize] = '\0';
		result = buffer;
	}
	CloseHandle(fileHandle);

	return result;
}

int main(void)
{
	GLFWwindow* window;

	/* Initialize the library */
	if (!glfwInit())
		return -1;
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	/* Create a windowed mode window and its OpenGL context */
	window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Phong Illumination", NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		return -1;
	}

	/* Make the window's context current */
	glfwMakeContextCurrent(window);

	/* GLAD takes the responsibility of loading OpenGL extensions from us. 
	   If we don't do this we are stuck with OpenGL 1.1 (No shaders, fixed function pipeline). */
	gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

	// Init Model throug Assimp
	Model model = {};
	aiScene const * scene = aiImportFile("assets\\obj\\stormtrooper\\stormtrooper.obj", aiProcess_Triangulate);
	uint32_t vertex_count = 0;
	for (int i = 0; i < scene->mNumMeshes; ++i) {
		vertex_count += scene->mMeshes[i]->mNumVertices;
	}
	model.vertex_count = vertex_count;
	model.vertices = (Vertex*)malloc(vertex_count * sizeof(Vertex));
	memset(model.vertices, 0, vertex_count*sizeof(Vertex));
	Vertex * current_vertex = model.vertices;
	for (int i = 0; i < scene->mNumMeshes; ++i) {
		for (int j = 0; j < scene->mMeshes[i]->mNumVertices; ++j) {
			aiVector3D p = scene->mMeshes[i]->mVertices[j];
			aiVector3D n = scene->mMeshes[i]->mNormals[j];
			//aiVector3D uv = scene->mMeshes[i]->mTextureCoords[j];
			current_vertex->position = { p.x, p.y, p.z };
			current_vertex->normal   = { n.x, n.y, n.z };
			current_vertex++;
		}
	}
	/* The Stormtrooper's origin is set at its feet. So we move it down along the y-axis
	   to center it in our screen. Alternatively, moving the camera up would have the same result.
	*/
	model.position = glm::vec3(0, -30, 0);
	
	// Initialize View and Projection Matrices
	glm::mat4 view_matrix = glm::lookAt(glm::vec3(0, 0, 100), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
	glm::mat4 projection_matrix = glm::perspective(glm::radians(45.f), (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT, 0.1f, 200.f);

	// OpenGL global setup
	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);
	glFrontFace(GL_CW);
	/* We want fragments closer to the camera obscure fragments farther behind.*/
	glEnable(GL_DEPTH_TEST);
	/* We use the fragments z value for testing. If the fragment about to be drawn has a smaller (GL_LESS)
	   z-value than the fragment drawn before at this x,y position, we overdraw, else we discard this fragment.
	   Note that most implementations actually discard fragments at the rasterizer stage, so fragments that
	   would be overdrawn anyway don't even make it to the fragment-shading stage. */
	glDepthFunc(GL_LESS);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);

	// Setup OpenGL Buffers. The vao, vbo variables are just handles (an ID) the OpenGL implementation uses
	// internally. The data behind those handles is completely opaque to the us, the programmer, because OpenGL
	// does not want the user to mess with its memory. For example, when we call glBindBuffer with a vbo-handel and
	// glBufferData. The driver will allocate memory for us on the GPU and takes care of things like alignment (Vertex
	// data has to be aligned at - for example - 256 byte steps. The exact alignment value depends on the GPU-hardware).
	// OpenGL makes use of this concept extensively. Note that this is very different from Vulkan, a more modern Graphics API,
	// released in 2016 to the public. The user takes care of most of the memory management (things like alignment mentioned
	// above). This makes the Vulkan driver much smaller and - if the user knows what she is doing - can lay ouy
	// the program's memory organization specifically to her needs, potentially resulting in higher throughput.
	// That being said, OpenGL had been around for quite some time and the drivers are extremely well optimized.
	GLuint vao = 0;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	GLuint vbo = 0;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, model.vertex_count * sizeof(Vertex), model.vertices, GL_STATIC_DRAW);
	// Position (the index 0 matches the layout location in the vertex shader)
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);
	glEnableVertexAttribArray(0);
	// Normal (the index 1 matches the layout location the vertex shader)
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)sizeof(glm::vec3));
	glEnableVertexAttribArray(1);

	// Load shaders' sources and compile/link them
	char * vertex_shader_code   = read_file("vert.glsl");
	char * fragment_shader_code = read_file("frag.glsl");
	GLuint vs = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vs, 1, &vertex_shader_code, 0);
	glCompileShader(vs);
	GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fs, 1, &fragment_shader_code, 0);
	glCompileShader(fs);
	GLuint shader_program = glCreateProgram();
	glAttachShader(shader_program, vs);
	glAttachShader(shader_program, fs);
	glLinkProgram(shader_program);
	glUseProgram(shader_program);

	// Get uniform locations.
	// These are the 'uniform' inputs to the vertex shader.
	// NOTE: calling glGetUniformLocation has quite some impact on performace, so you should
	// call it once at startup and save the returned location handle to use it later.
	int model_mat_location = glGetUniformLocation(shader_program, "model");
	int view_mat_location = glGetUniformLocation(shader_program, "view");
	int projection_mat_location = glGetUniformLocation(shader_program, "projection");
	/* Upload data into the shader. */
	glUniformMatrix4fv(model_mat_location, 1, GL_FALSE, (GLfloat*)&(model.model_matrix));
	glUniformMatrix4fv(view_mat_location, 1, GL_FALSE, (GLfloat*)&(view_matrix));
	glUniformMatrix4fv(projection_mat_location, 1, GL_FALSE, (GLfloat*)&(projection_matrix));


	/* Render Loop */
	while (!glfwWindowShouldClose(window))
	{
		/* Clear backbuffer color and depth-buffer */
		glClearColor(.2f, 0.2f, 0.2f, 0.f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glUseProgram(shader_program);
		
		/* Update Shader Inputs and upload to shader. */
		static float rot = 0.f;
		rot += .7f;
		glm::mat4 translate = glm::mat4(1.f);
		translate = glm::translate(translate, model.position);
		glm::mat4 rotate = glm::mat4(1.f);
		rotate = glm::rotate(rotate, glm::radians(rot), glm::vec3(0, 1, 0));
		model.model_matrix = translate*rotate;
		glUniformMatrix4fv(model_mat_location, 1, GL_FALSE, (GLfloat*)&(model.model_matrix));

		/* Draw the model */
		glBindVertexArray(vao);
		glDrawArrays(GL_TRIANGLES, 0, model.vertex_count);

		/* This gives the driver the information, that drawing is done and it can now present
		   the image, we just have drawn to, to the screen. When the swap actually happens depends on
		   the GPU/Operating System. When V-Sync is active this call will block until a certain amount
		   of time has passed. */
		glfwSwapBuffers(window);

		/* Poll for and process events */
		glfwPollEvents();
	}

	/* NOTE: We do not free the memory previously allocated (malloc, VirtualAlloc) explicitly here. 
	         The Operating System will free the resources for us. It is ok to do this, because
			 this is a rather small program. More serious applications need serious memory management, of course! */

	glfwTerminate();
	return 0;
}

