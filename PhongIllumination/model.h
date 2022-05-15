#ifndef _MODEL_H_
#define _MODEL_H_

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct Vertex
{
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 uv;
};

struct Model
{
	Vertex*		vertices;
	uint32_t	vertex_count;
	glm::vec3	position;
	glm::mat4	model_matrix;
};


Model	loadModel(char const* path);
void	unloadModel(Model model);

#endif