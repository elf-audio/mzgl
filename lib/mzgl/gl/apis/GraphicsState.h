#pragma once

class GraphicsState {
public:
    GraphicsState(int &width, int &height) : width(width), height(height) {}
	float strokeWeight = 1;
    int &width;
    int &height;
	bool blendingEnabled = false;
	glm::vec4 color;
	//glm::mat4 mvp;
	MatrixStack modelMatrixStack;
	glm::mat4 projectionMatrix;
	glm::mat4 viewMatrix;
	// this is cached version of the above multiplied
	glm::mat4 viewProjectionMatrix;
	friend class ScopedAlphaBlend;
	friend struct ScopedTranslate;

	uint32_t immediateVertexArray  = 0;
	uint32_t immediateVertexBuffer = 0;
	uint32_t immediateColorBuffer  = 0;
	uint32_t immediateIndexBuffer  = 0;

	Rectf scissor;

	// could probs be a shared ptr/ShaderRef
	Shader *currShader = nullptr;
	std::vector<Shader *> shaders;

    // these are the default shaders
    ShaderRef nothingShader;
    ShaderRef colorShader;
    ShaderRef colorTextureShader;
    ShaderRef texShader;
    ShaderRef fontShader;

};