//
// Created by Marek Bereza on 16/05/2024.
//

#pragma once

// clang-format off
std::string nothingVertSrc = STRINGIFY(
	uniform mat4 mvp;

	in vec4 Position;
	uniform lowp vec4 color;
	out lowp vec4 colorV;

	void main(void) {
	   colorV	   = color;
	   gl_Position = mvp * Position;
	}
);

std::string nothingFragSrc = STRINGIFY(
	in lowp vec4 colorV;
	out vec4 fragColor;

	void main(void) {
		fragColor = colorV;
	}
);


std::string colorVertSrc = STRINGIFY(
	uniform mat4 mvp; uniform lowp vec4 color;

	in vec4 Position;
	in lowp vec4 Color;

	out lowp vec4 colorV;

	void main(void) {
		colorV	  = Color * color;
		gl_Position = mvp * Position;
	}
);


std::string colorFragSrc = STRINGIFY(

	in lowp vec4 colorV;
	out vec4 fragColor;

	void main(void) {
		fragColor = colorV;
	}

);

std::string colorTextureVertSrc = STRINGIFY(

	uniform mat4 mvp;

	in vec4 Position;
	in lowp vec2 TexCoord;
	in lowp vec4 Color;

	out lowp vec4 colorV;
	out lowp vec2 texCoordV;
	void main(void) {
		colorV		= Color;
		texCoordV	= TexCoord;
		gl_Position = mvp * Position;
	}

);
std::string colorTextureFragSrc = STRINGIFY(

	in lowp vec4 colorV;
	in lowp vec2 texCoordV;
	out vec4 fragColor;
	uniform sampler2D myTextureSampler;

	void main(void) {
		fragColor = texture(myTextureSampler, texCoordV) * colorV;
	}

);

std::string fontVertSrc = STRINGIFY(

	uniform mat4 mvp;

	in vec4 Position;
	in vec2 TexCoord;

	uniform lowp vec4 color;

	out vec2 texCoordV;

	void main() {
		texCoordV	 = TexCoord;
		gl_Position = mvp * Position;
	}
);

std::string fontFragSrc = STRINGIFY(

	uniform lowp vec4 color;
	in vec2 texCoordV;
	out vec4 fragColor;
	uniform sampler2D myTextureSampler;

	void main() {
		fragColor = color;
		fragColor.a *= texture(myTextureSampler, texCoordV).a;
	}
);

std::string texVertSrc = STRINGIFY(

	uniform mat4 mvp;

	in vec4 Position;
	in lowp vec2 TexCoord;
	uniform lowp vec4 color;

	out lowp vec4 colorV;
	out lowp vec2 texCoordV;
	void main(void) {
		colorV		= color;
		texCoordV	= TexCoord;
		gl_Position = mvp * Position;
	}
);

std::string texFragSrc = STRINGIFY(

	in lowp vec4 colorV;
	in lowp vec2 texCoordV;
	out vec4 fragColor;
	uniform sampler2D myTextureSampler;

	void main(void) {
		fragColor = texture(myTextureSampler, texCoordV) * colorV;
	}
);

// clang-format on