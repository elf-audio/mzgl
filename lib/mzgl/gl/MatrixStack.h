//
//  MatrixStack.h
//  MZGL
//
//  Created by Marek Bereza on 16/01/2018.
//  Copyright © 2018 Marek Bereza. All rights reserved.
//

#pragma once
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class MatrixStack {
public:
	std::vector<glm::mat4> stack;
	glm::mat4 curr; // identity is default
	MatrixStack() { curr = glm::mat4(1.0); }
	const glm::mat4 &getMatrix() const { return curr; }

	void pushMatrix() { stack.push_back(curr); }

	void popMatrix() {
		curr = stack.back();
		stack.pop_back();
	}
	size_t size() { return stack.size(); }

	void loadIdentity() { curr = glm::mat4(); }

	void translate(float x, float y, float z = 0) { curr = glm::translate(curr, glm::vec3(x, y, z)); }

	void scale(float x, float y, float z = 1) { curr = glm::scale(curr, glm::vec3(x, y, z)); }

	void rotate(float angle, glm::vec3 axis) { curr = glm::rotate(curr, angle, axis); }
};
