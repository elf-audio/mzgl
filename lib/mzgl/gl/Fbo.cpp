//
//  Fbo.cpp
//  MZGL
//
//  Created by Marek Bereza on 27/02/2018.
//  Copyright © 2018 Marek Bereza. All rights reserved.
//

#include "Fbo.h"
#include "glm/gtc/matrix_transform.hpp"
#include "Graphics.h"
#include "util.h"
#include <iostream>
#include "error.h"
#include "log.h"

using namespace std;

// with help from http://www.songho.ca/opengl/gl_fbo.html

#include "Graphics.h"

bool checkFramebufferStatus(GLuint fbo)
{
	// check FBO status
	glBindFramebuffer(GL_FRAMEBUFFER, fbo); // bind
	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	switch(status)
	{
		case GL_FRAMEBUFFER_COMPLETE:
			std::cout << "Framebuffer complete." << std::endl;
			return true;
			
		case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
			std::cout << "[ERROR] Framebuffer incomplete: Attachment is NOT complete." << std::endl;
			return false;
			
		case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
			std::cout << "[ERROR] Framebuffer incomplete: No image is attached to FBO." << std::endl;
			return false;
			/*
			 case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS:
			 std::cout << "[ERROR] Framebuffer incomplete: Attached images have different dimensions." << std::endl;
			 return false;
			 
			 case GL_FRAMEBUFFER_INCOMPLETE_FORMATS:
			 std::cout << "[ERROR] Framebuffer incomplete: Color attached images have different internal formats." << std::endl;
			 return false;
			 */
//		case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
//			std::cout << "[ERROR] Framebuffer incomplete: Draw buffer." << std::endl;
//			return false;
//			
//		case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
//			std::cout << "[ERROR] Framebuffer incomplete: Read buffer." << std::endl;
//			return false;
//			
		case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
			std::cout << "[ERROR] Framebuffer incomplete: Multisample." << std::endl;
			return false;
			
		case GL_FRAMEBUFFER_UNSUPPORTED:
			std::cout << "[ERROR] Framebuffer incomplete: Unsupported by FBO implementation." << std::endl;
			return false;
			
		default:
			std::cout << "[ERROR] Framebuffer incomplete: Unknown error." << std::endl;
			return false;
	}
	//glBindFramebuffer(GL_FRAMEBUFFER, 0);   // unbind
}



void Fbo::setup(int w, int h, GLenum type, bool hasDepth, int numSamples) {
	deallocate();
	this->width = w;
	this->height = h;
	this->numSamples = numSamples;
	tex = Texture::create();
	tex->allocate(w,h, type);
	
	
	glGenFramebuffers(1, &fboId);
	glBindFramebuffer(GL_FRAMEBUFFER, fboId);
	

	
	if(hasDepth) {
		glGenRenderbuffers(1, &rboDepthId);
		glBindRenderbuffer(GL_RENDERBUFFER, rboDepthId);
		if(numSamples > 1) {
			glRenderbufferStorageMultisample(GL_RENDERBUFFER, numSamples, GL_DEPTH_COMPONENT, width, height);
		} else {
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
		}
		glBindRenderbuffer(GL_RENDERBUFFER, 0);
	}
	
	
	if(hasDepth) {
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepthId);
	}
	
	GLenum DrawBuffers[1] = {GL_COLOR_ATTACHMENT0};
	glDrawBuffers(1, DrawBuffers); // "1" is the size of DrawBuffers
	GetError();
	if(numSamples > 1) {
		// create color rende buffer for MSAA
		glGenRenderbuffers(1, &rboMsaaColorId);
		glBindRenderbuffer(GL_RENDERBUFFER, rboMsaaColorId);
		GetError();
		GLuint internalFormat = GL_RGBA8;
		if(type==GL_RGB) {
			internalFormat = GL_RGB8;
		} else if(type==GL_RGBA) {
			internalFormat = GL_RGBA8;
		} else {
			assert(0);
		}
		glRenderbufferStorageMultisample(GL_RENDERBUFFER, numSamples, internalFormat, width, height);
		GetError();
		glBindRenderbuffer(GL_RENDERBUFFER, 0);
		GetError();
		glFramebufferRenderbuffer(GL_FRAMEBUFFER,       // 1. fbo target: GL_FRAMEBUFFER
								  GL_COLOR_ATTACHMENT0, // 2. color attachment point
								  GL_RENDERBUFFER,      // 3. rbo target: GL_RENDERBUFFER
								  rboMsaaColorId);          // 4. rbo ID
		GetError();
		// create the downsampled version
		glGenFramebuffers(1, &downsampleFboId);
		glBindFramebuffer(GL_FRAMEBUFFER, downsampleFboId);
		GetError();
//		if(hasDepth) {
//			glGenRenderbuffers(1, &downsampleDepthRboId);
//			glBindRenderbuffer(GL_RENDERBUFFER, downsampleDepthRboId);
//			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
//			glBindRenderbuffer(GL_RENDERBUFFER, 0);
//
//			// attach a rbo to FBO depth attachement point
//			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, downsampleDepthRboId);
//		}
		
		// attach a texture to FBO color attachement point
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex->getId(), 0);
		
	} else {
	
	
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex->getId(), 0);
	}
	
	
	GetError();
	
	
	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if(status != GL_FRAMEBUFFER_COMPLETE) {
		Log::e() << "Error: Incomplete FrameBuffer";
		checkFramebufferStatus(downsampleFboId);
	} else {
		//printf("HAPPY FBO\n");
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	GetError();
}

void Fbo::begin(Graphics &g) {
	// set the rendering destination to FBO
	glBindFramebuffer(GL_FRAMEBUFFER, fboId);
	g.setupView(false, width, height);
	
	glViewport(0, 0, width, height);
	

}
void Fbo::readPixels(Graphics &g, vector<uint8_t> &data) {

	if(data.size()!=4 * width * height) {
		Log::e() << "FBO size mismatch";
	}
	glBindFramebuffer(GL_FRAMEBUFFER, fboId);
	
	glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data.data());

	glBindFramebuffer(GL_FRAMEBUFFER, g.getDefaultFrameBufferId()); // unbind
}

void Fbo::end(Graphics &g) {
	
	
	GetError();
	if(numSamples>1) {
		
		//glDrawBuffer(GL_NONE)
		//glReadBuffer(GL_NONE)
		glBindFramebuffer(GL_READ_FRAMEBUFFER, fboId);
		//CheckFramebufferStatus( );

		GetError();
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, downsampleFboId);
		//CheckFramebufferStatus( );

		GetError();
		glBlitFramebuffer(0, 0, width, height,  // src rect
						  0, 0, width, height,  // dst rect
						  GL_COLOR_BUFFER_BIT, // buffer mask
						  GL_NEAREST);//GL_LINEAR);                           // scale filter
		//CheckFramebufferStatus( );

		GetError();
	}
	
	// https://stackoverflow.com/questions/10761902/ios-glkit-and-back-to-default-framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, g.getDefaultFrameBufferId()); // unbind
	GetError();
	g.setupView();
	
	GetError();
	glViewport(0, 0, g.width, g.height);
}

void Fbo::bind() {
	tex->bind();
}
void Fbo::unbind() {
	tex->unbind();
}

Fbo::~Fbo() {
	deallocate();
}

void Fbo::deallocate() {
	// TODO: needs to get rid of everything (inc msaa buffers)
	if(fboId!=0) glDeleteFramebuffers(1, &fboId);
	if(rboDepthId!=0) glDeleteRenderbuffers(1, &rboDepthId);
	if(tex) tex->deallocate();
	width = 0;
	height = 0;
}


