//  Created by Marek Bereza on 17/02/2023.

#include "glUtil.h"
#include "log.h"
#include "mzOpenGL.h"

int primitiveTypeToGLMode(Vbo::PrimitiveType mode) {
	switch(mode) {
		case Vbo::PrimitiveType::Triangles:		return GL_TRIANGLES;
		case Vbo::PrimitiveType::TriangleStrip:	return GL_TRIANGLE_STRIP;
		case Vbo::PrimitiveType::TriangleFan: 	return GL_TRIANGLE_FAN;
		case Vbo::PrimitiveType::LineLoop:		return GL_LINE_LOOP;
		case Vbo::PrimitiveType::LineStrip:		return GL_LINE_STRIP;
		case Vbo::PrimitiveType::Lines:			return GL_LINES;
			
		default: {
			Log::e() << "ERROR!! invalid primitive type " << (int)mode;
			return GL_TRIANGLES;
		}
	}
}
