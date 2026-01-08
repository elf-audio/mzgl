
#include "App.h"
#include <cmath>

class MyApp : public App {
public:
	MyApp(Graphics &g)
		: App(g) {}

	void draw() override {
		g.clear(0, 0, 0);

		///////////////////////////////////////////////////////////////////////
		// DRAW A CIRCLE

		// set current colour to white - setColor takes 1 or 3 arguments
		// always between 0 and 1, if you use 1 argument its a grayscale
		// value, if you use 3 its rgb. (it can also take 4 args, which
		// means the 4th is alpha)
		g.setColor(0.25);

		vec2 centre {g.width / 4, g.height * 0.25};
		float radius = g.height / 8;

		g.drawCircle(centre, radius);

		// all shapes drawn after this will be just outlines (e.g. no fill)
		// the default is filled
		g.noFill();

		// set colour to green
		g.setColor(0, 1, 0);
		g.drawCircle(centre, radius);

		///////////////////////////////////////////////////////////////////////
		// DRAW A RECTANGLE

		// Rectf is an important class, which defines a rectangle by
		// x, y, width, height. We make one over to the right
		Rectf rect;
		rect.setFromCentre({g.width * 0.75f, g.height * 0.25f}, radius, radius);

		// all shapes drawn after this call will be filled (rather than outlined)
		g.fill();

		// make a value that sinusoidally goes between 0 and 1
		float redAmount = 0.5 + 0.5 * std::sin(g.currFrameTime);
		g.setColor(redAmount, 0, 1 - redAmount);
		g.drawRect(rect);

		///////////////////////////////////////////////////////////////////////
		// DRAW A TRIANGLE

		// this draws 12 turquoise triangles on top of eachother
		// and rotates them

		// set colour to turquoise with alpha of 12.5%
		g.setColor(0.1, 1, 1, 0.125);

		{
			ScopedTransform tr(g);
			// move to the bottom left corner of the screen
			g.translate(g.width * 0.25, g.height * 0.75);

			// rotate by the amount of time that has elapsed
			// (angle is in radians)
			g.rotateZ(g.currFrameTime * 2.f);

			// draw a triangle around the coordinates {0,0}
			float h = g.height * 0.125;
			for (int i = 0; i < 12; i++) {
				// rotate a bit more
				g.rotateZ(0.25);

				g.drawTriangle({0.f, -h}, {-h, h * 0.7f}, {h, h * 0.7f});
			}
		}

		///////////////////////////////////////////////////////////////////////
		// DRAW A polygon

		// yellow
		g.setColor(1, 1, 0);

		// prepare some vertices
		std::vector<vec2> verts;
		// the centre of the triangle fan
		verts.emplace_back(0.f, 0.f);

		// go around the circle varying the radius
		for (float angle = 0.f; angle <= M_PI * 2.02; angle += M_PI / 64.f) {
			float radius = 100 + std::sin(angle * 8.f + g.currFrameTime * 4) * 30 * sin(g.currFrameTime * 2.f);
			float x		 = radius * cos(angle);
			float y		 = radius * sin(angle);
			verts.emplace_back(x, y);
		}

		g.pushMatrix();
		// move to the bottom right corner of the screen
		g.translate(g.width * 0.75, g.height * 0.75);

		// draw the verts as a triangle fan
		g.drawVerts(verts, Vbo::PrimitiveType::TriangleFan);
		g.popMatrix();
	}
};

std::shared_ptr<App> instantiateApp(Graphics &g) {
	g.width	 = 800;
	g.height = 800;
	return std::make_shared<MyApp>(g);
}
