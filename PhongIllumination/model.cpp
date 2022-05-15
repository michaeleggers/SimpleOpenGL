#include <assert.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <assimp/cimport.h>        // Plain-C interface
#include <assimp/scene.h>          // Output data structure
#include <assimp/postprocess.h>    // Post processing flags

#include "model.h"

Model loadModel(char const * path) 
{
	Model model = {};

	aiScene const* scene = aiImportFile(path, aiProcess_Triangulate);
	uint32_t vertex_count = 0;
	for (int i = 0; i < scene->mNumMeshes; ++i) {
		vertex_count += scene->mMeshes[i]->mNumVertices;
	}
	model.vertex_count = vertex_count;
	model.vertices = (Vertex*)malloc(vertex_count * sizeof(Vertex));
	assert(model.vertices != NULL);	
	memset(model.vertices, 0, vertex_count * sizeof(Vertex));
	Vertex* current_vertex = model.vertices;

	for (int i = 0; i < scene->mNumMeshes; ++i) {
		for (int j = 0; j < scene->mMeshes[i]->mNumVertices; ++j) {
			aiVector3D p = scene->mMeshes[i]->mVertices[j];
			aiVector3D n = scene->mMeshes[i]->mNormals[j];
			//aiVector3D uv = scene->mMeshes[i]->mTextureCoords[j];
			current_vertex->position = { p.x, p.y, p.z };
			current_vertex->normal = { n.x, n.y, n.z };
			current_vertex++;
		}
	}

	return model;
}

void unloadModel(Model model)
{
	assert(model.vertices != NULL);
	free(model.vertices);
}