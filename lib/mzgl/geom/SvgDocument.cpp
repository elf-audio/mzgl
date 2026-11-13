//
//  SvgDocument.cpp
//  mzgl
//

#include "SvgDocument.h"

#include "DisableAllWarnings.h"
#include "Graphics.h"
#include "Geometry.h"
#include "colors.h"
#include "log.h"
#include "util.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <unordered_map>

DISABLE_WARNINGS
#include "tesselator.h"
#include "pu_gixml.hpp"
RESTORE_WARNINGS

#ifdef __ANDROID__
#	include "androidUtil.h"
#endif

using glm::vec2;
using glm::vec4;

///////////////////////////////////////////////////////////////////////////////
// PARSING

namespace {
	constexpr float kPi	   = 3.14159265358979323846f;
	constexpr float kKappa = 0.5522847498f; // cubic control distance for a quarter circle

	bool isSeparator(char c) {
		return c == ' ' || c == ',' || c == '\t' || c == '\n' || c == '\r';
	}

	void skipSeparators(const char *&p, const char *end) {
		while (p < end && isSeparator(*p))
			++p;
	}

	// Locale independent, allocation free. Accepts everything the SVG number
	// grammar does, including the run-together forms path data likes
	// ("1.5.5" is 1.5 then .5, "1-2" is 1 then -2).
	bool parseNumber(const char *&p, const char *end, float &out) {
		skipSeparators(p, end);
		const char *s = p;
		if (s >= end) return false;

		bool negative = false;
		if (*s == '-' || *s == '+') {
			negative = *s == '-';
			++s;
		}
		double value = 0;
		bool digits	 = false;
		while (s < end && *s >= '0' && *s <= '9') {
			value  = value * 10.0 + (*s - '0');
			digits = true;
			++s;
		}
		if (s < end && *s == '.') {
			++s;
			double place = 0.1;
			while (s < end && *s >= '0' && *s <= '9') {
				value += (*s - '0') * place;
				place *= 0.1;
				digits = true;
				++s;
			}
		}
		if (!digits) return false;

		if (s < end && (*s == 'e' || *s == 'E')) {
			const char *e	 = s + 1;
			bool expNegative = false;
			if (e < end && (*e == '-' || *e == '+')) {
				expNegative = *e == '-';
				++e;
			}
			if (e < end && *e >= '0' && *e <= '9') { // otherwise it's a unit like "em"
				int exponent = 0;
				while (e < end && *e >= '0' && *e <= '9') {
					exponent = std::min(exponent * 10 + (*e - '0'), 400);
					++e;
				}
				value *= std::pow(10.0, expNegative ? -exponent : exponent);
				s = e;
			}
		}
		out = static_cast<float>(negative ? -value : value);
		p	= s;
		return true;
	}

	// arc flags are single characters and may be run together: "a1 1 0 0110 10"
	bool parseFlag(const char *&p, const char *end, bool &out) {
		skipSeparators(p, end);
		if (p >= end || (*p != '0' && *p != '1')) return false;
		out = *p == '1';
		++p;
		return true;
	}

	float numberOr(const char *s, float fallback) {
		if (s == nullptr) return fallback;
		const char *p = s;
		float v		  = fallback;
		parseNumber(p, s + std::strlen(s), v);
		return v;
	}

	std::string_view trim(std::string_view s) {
		while (!s.empty() && (isSeparator(s.front())))
			s.remove_prefix(1);
		while (!s.empty() && (isSeparator(s.back())))
			s.remove_suffix(1);
		return s;
	}

	struct Style {
		bool hasFill = true; // the svg default is a black fill
		glm::vec3 fill {0, 0, 0};
		float fillAlpha	  = 1; // alpha carried by the colour itself (#rrggbbaa, rgba())
		float fillOpacity = 1;
		bool evenOdd	  = false;

		bool hasStroke = false;
		glm::vec3 stroke {0, 0, 0};
		float strokeAlpha	= 1;
		float strokeOpacity = 1;
		float strokeWidth	= 1;
		float miterLimit	= 4;
		SvgDocument::Cap cap   = SvgDocument::Cap::Butt;
		SvgDocument::Join join = SvgDocument::Join::Miter;

		// not inherited in svg, but there are no offscreen layers here, so group
		// opacity is approximated by multiplying it into everything below
		float opacity = 1;
		bool visible  = true;
	};

	void parsePaint(std::string_view value, bool &enabled, glm::vec3 &colour, float &alpha) {
		value = trim(value);
		if (value.empty() || value == "inherit") return;
		if (value == "none" || value == "transparent") {
			enabled = false;
			return;
		}
		if (value == "currentColor") {
			enabled = true;
			colour	= {0, 0, 0};
			alpha	= 1;
			return;
		}
		const std::string s(value);
		std::optional<vec4> c;
		if (isHexColor(s)) c = svgHexColor(s);
		else if (isRGBColor(s)) c = rgbColor(s);
		else if (isRGBAColor(s)) c = rgbaColor(s);
		else c = namedColor(s);

		// gradients/patterns (url(#...)) and anything unrecognised: not drawn
		if (!c) {
			enabled = false;
			return;
		}
		enabled = true;
		colour	= {c->r, c->g, c->b};
		alpha	= c->a;
	}

	void applyProperty(Style &st, std::string_view name, std::string_view value) {
		value = trim(value);
		auto number = [&](float fallback) {
			const char *p = value.data();
			float v		  = fallback;
			parseNumber(p, value.data() + value.size(), v);
			return v;
		};
		if (name == "fill") {
			parsePaint(value, st.hasFill, st.fill, st.fillAlpha);
		} else if (name == "stroke") {
			parsePaint(value, st.hasStroke, st.stroke, st.strokeAlpha);
		} else if (name == "fill-opacity") {
			st.fillOpacity = number(1);
		} else if (name == "stroke-opacity") {
			st.strokeOpacity = number(1);
		} else if (name == "opacity") {
			st.opacity *= number(1);
		} else if (name == "stroke-width") {
			st.strokeWidth = number(1);
		} else if (name == "stroke-miterlimit") {
			st.miterLimit = number(4);
		} else if (name == "fill-rule") {
			st.evenOdd = value == "evenodd";
		} else if (name == "stroke-linecap") {
			st.cap = value == "round"	 ? SvgDocument::Cap::Round
					 : value == "square" ? SvgDocument::Cap::Square
										 : SvgDocument::Cap::Butt;
		} else if (name == "stroke-linejoin") {
			st.join = value == "round"	 ? SvgDocument::Join::Round
					  : value == "bevel" ? SvgDocument::Join::Bevel
										 : SvgDocument::Join::Miter;
		} else if (name == "display") {
			if (value == "none") st.visible = false;
		} else if (name == "visibility") {
			st.visible = value != "hidden" && value != "collapse";
		}
	}

	void applyNodeStyle(Style &st, const pu_gi::xml_node &n) {
		const char *inlineStyle = nullptr;
		for (auto a = n.first_attribute(); a; a = a.next_attribute()) {
			const char *name = a.name();
			// cheap reject: every presentation attribute we handle starts with one of these
			switch (name[0]) {
				case 'f':
				case 'o':
				case 'd':
				case 'v': applyProperty(st, name, a.value()); break;
				case 's':
					if (std::strcmp(name, "style") == 0) inlineStyle = a.value();
					else applyProperty(st, name, a.value());
					break;
				default: break;
			}
		}
		// css declarations win over presentation attributes
		if (inlineStyle != nullptr) {
			std::string_view s(inlineStyle);
			while (!s.empty()) {
				const auto semi		  = s.find(';');
				std::string_view decl = s.substr(0, semi);
				const auto colon	  = decl.find(':');
				if (colon != std::string_view::npos) {
					applyProperty(st, trim(decl.substr(0, colon)), decl.substr(colon + 1));
				}
				if (semi == std::string_view::npos) break;
				s.remove_prefix(semi + 1);
			}
		}
	}

	SvgDocument::Affine parseTransform(const char *str) {
		SvgDocument::Affine m;
		if (str == nullptr) return m;
		const char *p	= str;
		const char *end = str + std::strlen(str);
		while (p < end) {
			skipSeparators(p, end);
			const char *nameStart = p;
			while (p < end && std::isalpha(static_cast<unsigned char>(*p)))
				++p;
			std::string_view name(nameStart, p - nameStart);
			skipSeparators(p, end);
			if (p >= end || *p != '(') break;
			++p;
			float v[6];
			int count = 0;
			while (count < 6 && parseNumber(p, end, v[count]))
				++count;
			while (p < end && *p != ')')
				++p;
			if (p < end) ++p;

			SvgDocument::Affine t;
			if (name == "translate" && count >= 1) {
				t.e = v[0];
				t.f = count >= 2 ? v[1] : 0;
			} else if (name == "scale" && count >= 1) {
				t.a = v[0];
				t.d = count >= 2 ? v[1] : v[0];
			} else if (name == "rotate" && count >= 1) {
				const float theta = v[0] * kPi / 180.f;
				const float cs = std::cos(theta), sn = std::sin(theta);
				t.a = cs;
				t.b = sn;
				t.c = -sn;
				t.d = cs;
				if (count >= 3) {
					t.e = v[1] - cs * v[1] + sn * v[2];
					t.f = v[2] - sn * v[1] - cs * v[2];
				}
			} else if (name == "matrix" && count == 6) {
				t = {v[0], v[1], v[2], v[3], v[4], v[5]};
			} else if (name == "skewX" && count >= 1) {
				t.c = std::tan(v[0] * kPi / 180.f);
			} else if (name == "skewY" && count >= 1) {
				t.b = std::tan(v[0] * kPi / 180.f);
			}
			m = m * t;
		}
		return m;
	}
} // namespace

struct SvgParser {
	SvgDocument &doc;
	std::unordered_map<std::string_view, pu_gi::xml_node> ids;
	bool idsBuilt = false;

	explicit SvgParser(SvgDocument &d)
		: doc(d) {}

	// ---- path building -----------------------------------------------------
	void moveTo(vec2 p) {
		doc.cmds.push_back(SvgDocument::MoveTo);
		doc.coords.insert(doc.coords.end(), {p.x, p.y});
	}
	void lineTo(vec2 p) {
		doc.cmds.push_back(SvgDocument::LineTo);
		doc.coords.insert(doc.coords.end(), {p.x, p.y});
	}
	void cubicTo(vec2 c1, vec2 c2, vec2 p) {
		doc.cmds.push_back(SvgDocument::CubicTo);
		doc.coords.insert(doc.coords.end(), {c1.x, c1.y, c2.x, c2.y, p.x, p.y});
	}
	void close() { doc.cmds.push_back(SvgDocument::Close); }

	// svg endpoint arc -> cubics (spec appendix B.2.4)
	void arcTo(vec2 p0, float rx, float ry, float xAxisRotationDeg, bool largeArc, bool sweep, vec2 p1) {
		rx = std::abs(rx);
		ry = std::abs(ry);
		if (rx < 1e-6f || ry < 1e-6f) {
			lineTo(p1);
			return;
		}
		if (p0 == p1) return;

		const float phi	   = xAxisRotationDeg * kPi / 180.f;
		const float cosPhi = std::cos(phi), sinPhi = std::sin(phi);
		const vec2 half = (p0 - p1) * 0.5f;
		const vec2 p(cosPhi * half.x + sinPhi * half.y, -sinPhi * half.x + cosPhi * half.y);

		const float lambda = (p.x * p.x) / (rx * rx) + (p.y * p.y) / (ry * ry);
		if (lambda > 1) {
			const float s = std::sqrt(lambda);
			rx *= s;
			ry *= s;
		}
		const float num = rx * rx * ry * ry - rx * rx * p.y * p.y - ry * ry * p.x * p.x;
		const float den = rx * rx * p.y * p.y + ry * ry * p.x * p.x;
		float k			= den > 0 ? std::sqrt(std::max(0.f, num / den)) : 0.f;
		if (largeArc == sweep) k = -k;
		const vec2 cp(k * rx * p.y / ry, -k * ry * p.x / rx);
		const vec2 mid = (p0 + p1) * 0.5f;
		const vec2 centre(cosPhi * cp.x - sinPhi * cp.y + mid.x, sinPhi * cp.x + cosPhi * cp.y + mid.y);

		const float theta1 = std::atan2((p.y - cp.y) / ry, (p.x - cp.x) / rx);
		const float theta2 = std::atan2((-p.y - cp.y) / ry, (-p.x - cp.x) / rx);
		float delta		   = theta2 - theta1;
		if (sweep && delta < 0) delta += 2 * kPi;
		else if (!sweep && delta > 0) delta -= 2 * kPi;

		const int segments = std::max(1, static_cast<int>(std::ceil(std::abs(delta) / (kPi * 0.5f) - 1e-4f)));
		const float step   = delta / segments;
		const float t	   = 4.f / 3.f * std::tan(step * 0.25f);

		auto pointAt = [&](float theta) {
			const float x = rx * std::cos(theta), y = ry * std::sin(theta);
			return vec2(cosPhi * x - sinPhi * y + centre.x, sinPhi * x + cosPhi * y + centre.y);
		};
		auto tangentAt = [&](float theta) {
			const float x = -rx * std::sin(theta), y = ry * std::cos(theta);
			return vec2(cosPhi * x - sinPhi * y, sinPhi * x + cosPhi * y);
		};

		float theta = theta1;
		vec2 from	= p0;
		for (int i = 0; i < segments; i++) {
			const float next = theta + step;
			const vec2 to	 = i == segments - 1 ? p1 : pointAt(next);
			cubicTo(from + tangentAt(theta) * t, to - tangentAt(next) * t, to);
			theta = next;
			from  = to;
		}
	}

	void parsePathData(const char *d) {
		const char *p	= d;
		const char *end = d + std::strlen(d);

		vec2 cur(0, 0), start(0, 0), lastCtrl(0, 0);
		char cmd		= 0;
		char prevCmd	= 0; // lower case, for smooth curve reflection
		bool needMoveTo = true;

		auto begin = [&]() {
			if (needMoveTo) {
				moveTo(cur);
				start	   = cur;
				needMoveTo = false;
			}
		};
		auto num = [&](float &v) { return parseNumber(p, end, v); };

		while (true) {
			skipSeparators(p, end);
			if (p >= end) break;

			if (std::isalpha(static_cast<unsigned char>(*p))) {
				cmd = *p++;
			} else if (cmd == 0 || cmd == 'z' || cmd == 'Z') {
				break; // data without a command
			} else if (cmd == 'M') {
				cmd = 'L'; // extra coordinate pairs after a moveto are linetos
			} else if (cmd == 'm') {
				cmd = 'l';
			}

			const bool relative = cmd >= 'a' && cmd <= 'z';
			const char lower	= relative ? cmd : static_cast<char>(cmd - 'A' + 'a');
			const vec2 origin	= relative ? cur : vec2(0, 0);
			float a[7];
			bool ok = true;

			switch (lower) {
				case 'm':
					if ((ok = num(a[0]) && num(a[1]))) {
						cur = origin + vec2(a[0], a[1]);
						moveTo(cur);
						start	   = cur;
						needMoveTo = false;
					}
					break;
				case 'l':
					if ((ok = num(a[0]) && num(a[1]))) {
						begin();
						cur = origin + vec2(a[0], a[1]);
						lineTo(cur);
					}
					break;
				case 'h':
					if ((ok = num(a[0]))) {
						begin();
						cur.x = origin.x + a[0];
						lineTo(cur);
					}
					break;
				case 'v':
					if ((ok = num(a[0]))) {
						begin();
						cur.y = origin.y + a[0];
						lineTo(cur);
					}
					break;
				case 'c':
					if ((ok = num(a[0]) && num(a[1]) && num(a[2]) && num(a[3]) && num(a[4]) && num(a[5]))) {
						begin();
						const vec2 c1 = origin + vec2(a[0], a[1]);
						lastCtrl	  = origin + vec2(a[2], a[3]);
						cur			  = origin + vec2(a[4], a[5]);
						cubicTo(c1, lastCtrl, cur);
					}
					break;
				case 's':
					if ((ok = num(a[0]) && num(a[1]) && num(a[2]) && num(a[3]))) {
						begin();
						const vec2 c1 = (prevCmd == 'c' || prevCmd == 's') ? cur * 2.f - lastCtrl : cur;
						lastCtrl	  = origin + vec2(a[0], a[1]);
						cur			  = origin + vec2(a[2], a[3]);
						cubicTo(c1, lastCtrl, cur);
					}
					break;
				case 'q':
				case 't': {
					vec2 q;
					if (lower == 'q') {
						if (!(ok = num(a[0]) && num(a[1]) && num(a[2]) && num(a[3]))) break;
						q = origin + vec2(a[0], a[1]);
					} else {
						if (!(ok = num(a[2]) && num(a[3]))) break;
						q = (prevCmd == 'q' || prevCmd == 't') ? cur * 2.f - lastCtrl : cur;
					}
					begin();
					const vec2 to = origin + vec2(a[2], a[3]);
					cubicTo(cur + (q - cur) * (2.f / 3.f), to + (q - to) * (2.f / 3.f), to);
					lastCtrl = q;
					cur		 = to;
					break;
				}
				case 'a': {
					bool largeArc = false, sweep = false;
					if ((ok = num(a[0]) && num(a[1]) && num(a[2]) && parseFlag(p, end, largeArc)
							  && parseFlag(p, end, sweep) && num(a[3]) && num(a[4]))) {
						begin();
						const vec2 to = origin + vec2(a[3], a[4]);
						arcTo(cur, a[0], a[1], a[2], largeArc, sweep, to);
						cur = to;
					}
					break;
				}
				case 'z':
					if (!needMoveTo) {
						close();
						cur		   = start;
						needMoveTo = true;
					}
					break;
				default: ok = false; break;
			}
			if (!ok) break; // malformed: keep what we have, like browsers do
			prevCmd = lower;
		}
	}

	// ---- elements ----------------------------------------------------------
	void rect(const pu_gi::xml_node &n) {
		const float x = numberOr(n.attribute("x").value(), 0);
		const float y = numberOr(n.attribute("y").value(), 0);
		const float w = numberOr(n.attribute("width").value(), 0);
		const float h = numberOr(n.attribute("height").value(), 0);
		if (w <= 0 || h <= 0) return;
		const auto rxAttr = n.attribute("rx");
		const auto ryAttr = n.attribute("ry");
		float rx		  = rxAttr ? numberOr(rxAttr.value(), 0) : -1;
		float ry		  = ryAttr ? numberOr(ryAttr.value(), 0) : -1;
		if (rx < 0 && ry < 0) rx = ry = 0;
		else if (rx < 0) rx = ry;
		else if (ry < 0) ry = rx;
		rx = std::min(rx, w * 0.5f);
		ry = std::min(ry, h * 0.5f);

		if (rx <= 0 || ry <= 0) {
			moveTo({x, y});
			lineTo({x + w, y});
			lineTo({x + w, y + h});
			lineTo({x, y + h});
			close();
			return;
		}
		const float kx = rx * (1 - kKappa), ky = ry * (1 - kKappa);
		moveTo({x + rx, y});
		lineTo({x + w - rx, y});
		cubicTo({x + w - kx, y}, {x + w, y + ky}, {x + w, y + ry});
		lineTo({x + w, y + h - ry});
		cubicTo({x + w, y + h - ky}, {x + w - kx, y + h}, {x + w - rx, y + h});
		lineTo({x + rx, y + h});
		cubicTo({x + kx, y + h}, {x, y + h - ky}, {x, y + h - ry});
		lineTo({x, y + ry});
		cubicTo({x, y + ky}, {x + kx, y}, {x + rx, y});
		close();
	}

	void ellipse(vec2 c, float rx, float ry) {
		if (rx <= 0 || ry <= 0) return;
		const float kx = rx * kKappa, ky = ry * kKappa;
		moveTo({c.x + rx, c.y});
		cubicTo({c.x + rx, c.y + ky}, {c.x + kx, c.y + ry}, {c.x, c.y + ry});
		cubicTo({c.x - kx, c.y + ry}, {c.x - rx, c.y + ky}, {c.x - rx, c.y});
		cubicTo({c.x - rx, c.y - ky}, {c.x - kx, c.y - ry}, {c.x, c.y - ry});
		cubicTo({c.x + kx, c.y - ry}, {c.x + rx, c.y - ky}, {c.x + rx, c.y});
		close();
	}

	void poly(const pu_gi::xml_node &n, bool closed) {
		const char *s	= n.attribute("points").value();
		const char *p	= s;
		const char *end = s + std::strlen(s);
		float x, y;
		bool first = true;
		while (parseNumber(p, end, x) && parseNumber(p, end, y)) {
			if (first) moveTo({x, y});
			else lineTo({x, y});
			first = false;
		}
		if (closed && !first) close();
	}

	void emitShape(const Style &st, const SvgDocument::Affine &xf, uint32_t firstCmd, uint32_t firstCoord) {
		const auto numCmds = static_cast<uint32_t>(doc.cmds.size()) - firstCmd;
		SvgDocument::Shape s;
		s.firstCmd	 = firstCmd;
		s.numCmds	 = numCmds;
		s.firstCoord = firstCoord;
		s.transform	 = xf;
		s.fill		 = vec4(st.fill, st.fillAlpha * st.fillOpacity * st.opacity);
		s.stroke	 = vec4(st.stroke, st.strokeAlpha * st.strokeOpacity * st.opacity);
		s.hasFill	 = st.hasFill && s.fill.a > 0;
		s.hasStroke	 = st.hasStroke && s.stroke.a > 0 && st.strokeWidth > 0;
		s.evenOdd	 = st.evenOdd;
		s.strokeWidth = st.strokeWidth;
		s.miterLimit  = st.miterLimit;
		s.cap		  = st.cap;
		s.join		  = st.join;

		if (numCmds == 0 || !st.visible || (!s.hasFill && !s.hasStroke)) {
			doc.cmds.resize(firstCmd);
			doc.coords.resize(firstCoord);
			return;
		}
		doc.shapes.push_back(s);
	}

	void buildIds(const pu_gi::xml_node &n) {
		for (auto c = n.first_child(); c; c = c.next_sibling()) {
			if (c.type() != pu_gi::node_element) continue;
			const char *id = c.attribute("id").value();
			if (id[0] != 0) ids.emplace(id, c);
			buildIds(c);
		}
	}

	void element(const pu_gi::xml_node &n, Style st, SvgDocument::Affine xf, int useDepth) {
		const char *tag = n.name();
		const auto is	= [&](const char *name) { return std::strcmp(tag, name) == 0; };

		// things that hold definitions or have no geometry
		if (is("defs") || is("title") || is("desc") || is("metadata") || is("style") || is("clipPath")
			|| is("mask") || is("filter") || is("symbol") || is("linearGradient") || is("radialGradient")
			|| is("pattern") || is("marker") || is("text") || is("image") || is("script")) {
			return;
		}

		applyNodeStyle(st, n);
		if (!st.visible) return;
		if (const auto t = n.attribute("transform")) xf = xf * parseTransform(t.value());

		if (is("g") || is("svg") || is("a") || is("switch")) {
			for (auto c = n.first_child(); c; c = c.next_sibling()) {
				if (c.type() == pu_gi::node_element) element(c, st, xf, useDepth);
			}
			return;
		}

		if (is("use")) {
			auto href = n.attribute("href");
			if (!href) href = n.attribute("xlink:href");
			const char *ref = href.value();
			if (ref[0] != '#' || useDepth > 8) return;
			if (!idsBuilt) {
				buildIds(n.root());
				idsBuilt = true;
			}
			const auto it = ids.find(ref + 1);
			if (it == ids.end()) return;
			SvgDocument::Affine offset;
			offset.e = numberOr(n.attribute("x").value(), 0);
			offset.f = numberOr(n.attribute("y").value(), 0);
			element(it->second, st, xf * offset, useDepth + 1);
			return;
		}

		const auto firstCmd	  = static_cast<uint32_t>(doc.cmds.size());
		const auto firstCoord = static_cast<uint32_t>(doc.coords.size());

		if (is("path")) {
			parsePathData(n.attribute("d").value());
		} else if (is("rect")) {
			rect(n);
		} else if (is("circle")) {
			const float r = numberOr(n.attribute("r").value(), 0);
			ellipse({numberOr(n.attribute("cx").value(), 0), numberOr(n.attribute("cy").value(), 0)}, r, r);
		} else if (is("ellipse")) {
			ellipse({numberOr(n.attribute("cx").value(), 0), numberOr(n.attribute("cy").value(), 0)},
					numberOr(n.attribute("rx").value(), 0),
					numberOr(n.attribute("ry").value(), 0));
		} else if (is("line")) {
			moveTo({numberOr(n.attribute("x1").value(), 0), numberOr(n.attribute("y1").value(), 0)});
			lineTo({numberOr(n.attribute("x2").value(), 0), numberOr(n.attribute("y2").value(), 0)});
		} else if (is("polygon")) {
			poly(n, true);
		} else if (is("polyline")) {
			poly(n, false);
		} else {
			return;
		}
		emitShape(st, xf, firstCmd, firstCoord);
	}
};

bool SvgDocument::parse(std::string_view svg) {
	cmds.clear();
	coords.clear();
	shapes.clear();
	width = height = 0;

	pu_gi::xml_document xml;
	const auto status = xml.load_buffer(svg.data(), svg.size());
	if (status.status != pu_gi::status_ok) {
		Log::e() << "ERROR: couldn't parse SVG: " << status.description() << " at character " << status.offset;
		return false;
	}
	const pu_gi::xml_node root = xml.document_element();

	float viewBox[4] = {0, 0, 0, 0};
	{
		const char *vb	= root.attribute("viewBox").value();
		const char *p	= vb;
		const char *end = vb + std::strlen(vb);
		int count		= 0;
		while (count < 4 && parseNumber(p, end, viewBox[count]))
			++count;
		if (count != 4 || viewBox[2] <= 0 || viewBox[3] <= 0) {
			viewBox[0] = viewBox[1] = 0;
			viewBox[2]				= numberOr(root.attribute("width").value(), 0);
			viewBox[3]				= numberOr(root.attribute("height").value(), 0);
		}
	}
	width  = viewBox[2];
	height = viewBox[3];

	// roughly one coordinate per 4 characters of markup
	coords.reserve(svg.size() / 4);
	cmds.reserve(svg.size() / 12);

	Affine xf;
	xf.e = -viewBox[0];
	xf.f = -viewBox[1];
	SvgParser parser(*this);
	parser.element(root, Style {}, xf, 0);

	cmds.shrink_to_fit();
	coords.shrink_to_fit();
	shapes.shrink_to_fit();
	return true;
}

bool SvgDocument::load(const std::string &path) {
	std::string data;
#ifdef __ANDROID__
	data	= loadAndroidAssetAsString(path);
	bool ok = !data.empty();
#else
	bool ok = readStringFromFile(path, data);
#endif
	if (!ok) {
		Log::e() << "ERROR: could not read svg file at " << path;
		return false;
	}
	return parse(data);
}

glm::mat3 SvgDocument::centredWithScale(float sx, float sy) const {
	return {sx, 0, 0, 0, sy, 0, -width * 0.5f * sx, -height * 0.5f * sy, 1};
}

size_t SvgDocument::memoryUsage() const {
	return cmds.capacity() * sizeof(uint8_t) + coords.capacity() * sizeof(float)
		   + shapes.capacity() * sizeof(Shape);
}

///////////////////////////////////////////////////////////////////////////////
// TESSELLATION

namespace {
	struct Contour {
		uint32_t start;
		uint32_t count;
		bool closed;
	};

	struct Ring {
		const vec2 *data = nullptr;
		size_t count	 = 0;
	};

	void *tessAlloc(void *, unsigned int size) {
		return std::malloc(size);
	}
	void *tessRealloc(void *, void *ptr, unsigned int size) {
		return std::realloc(ptr, size);
	}
	void tessFree(void *, void *ptr) {
		std::free(ptr);
	}

	float cross(vec2 a, vec2 b) {
		return a.x * b.y - a.y * b.x;
	}

	// a simple polygon that only ever turns one way and goes round once
	bool isConvex(const vec2 *p, size_t n) {
		if (n < 3) return false;
		int turn		   = 0;
		int xFlips		   = 0;
		int lastXDirection = 0;
		for (size_t i = 0; i < n; i++) {
			const vec2 a = p[i], b = p[(i + 1) % n], c = p[(i + 2) % n];
			const float cr = cross(b - a, c - b);
			if (cr != 0) {
				const int s = cr > 0 ? 1 : -1;
				if (turn == 0) turn = s;
				else if (s != turn) return false;
			}
			const float dx = b.x - a.x;
			if (dx != 0) {
				const int dir = dx > 0 ? 1 : -1;
				if (lastXDirection != 0 && dir != lastXDirection) ++xFlips;
				lastXDirection = dir;
			}
		}
		// a convex outline changes x direction twice (the wrap-around may hide one)
		return turn != 0 && xFlips <= 2;
	}

	struct Tessellator {
		Geometry &out;
		const SvgDocument::Options &options;
		SvgDocument::Stats *stats;

		// scratch, reused for every shape
		std::vector<vec2> points;
		std::vector<Contour> contours;
		std::vector<vec2> strokePoints;

		TESStesselator *tess = nullptr;
		std::vector<Ring> rings;

		// Curves are flattened to the full tolerance; the simplify pass afterwards
		// only gets a sliver, enough to drop points that are collinear for all
		// practical purposes (straight "curves", redundant linetos). Giving it a
		// real share was measured and loses: uniformly stepped curves have nothing
		// to give back, so the tighter flattening just costs vertices.
		float flattenTolerance;
		float simplifyTolerance;

		Tessellator(Geometry &g, const SvgDocument::Options &o, SvgDocument::Stats *s)
			: out(g)
			, options(o)
			, stats(s)
			, flattenTolerance(std::max(o.tolerance, 1e-4f))
			, simplifyTolerance(std::max(o.tolerance, 1e-4f) * 0.1f) {}

		~Tessellator() {
			if (tess != nullptr) tessDeleteTess(tess);
		}

		uint32_t vert(vec2 p, const vec4 &colour) {
			out.verts.push_back(p);
			if (options.colors) out.cols.push_back(colour);
			return static_cast<uint32_t>(out.verts.size() - 1);
		}
		void tri(uint32_t a, uint32_t b, uint32_t c) { out.indices.insert(out.indices.end(), {a, b, c}); }

		// ---- flattening ----------------------------------------------------
		void addPoint(vec2 p) {
			auto &c = contours.back();
			if (c.count > 0) {
				const vec2 d = p - points.back();
				if (d.x * d.x + d.y * d.y < 1e-8f) return;
			}
			points.push_back(p);
			++c.count;
		}

		void flatten(const SvgDocument::Shape &shape,
					 const SvgDocument::Affine &m,
					 const uint8_t *cmds,
					 const float *coords) {
			points.clear();
			contours.clear();
			const float *c = coords + shape.firstCoord;
			vec2 cur(0, 0);
			for (uint32_t i = 0; i < shape.numCmds; i++) {
				switch (cmds[shape.firstCmd + i]) {
					case SvgDocument::MoveTo:
						simplifyLastContour();
						contours.push_back({static_cast<uint32_t>(points.size()), 0, false});
						cur = m.apply(c[0], c[1]);
						addPoint(cur);
						c += 2;
						break;
					case SvgDocument::LineTo:
						cur = m.apply(c[0], c[1]);
						addPoint(cur);
						c += 2;
						break;
					case SvgDocument::CubicTo: {
						const vec2 p0 = cur;
						const vec2 p1 = m.apply(c[0], c[1]);
						const vec2 p2 = m.apply(c[2], c[3]);
						const vec2 p3 = m.apply(c[4], c[5]);
						c += 6;
						// Wang's bound: the number of uniform steps that keeps the
						// polyline within tolerance of the curve. A straight "curve"
						// (as Figma exports plenty of) comes out as one segment.
						const vec2 dd0 = p0 - 2.f * p1 + p2;
						const vec2 dd1 = p1 - 2.f * p2 + p3;
						const float dd = std::sqrt(std::max(glm::dot(dd0, dd0), glm::dot(dd1, dd1)));
						const int steps =
							std::clamp(static_cast<int>(std::ceil(std::sqrt(0.75f * dd / flattenTolerance))), 1, 512);
						const float dt = 1.f / static_cast<float>(steps);
						for (int s = 1; s < steps; s++) {
							const float t = dt * static_cast<float>(s);
							const float u = 1.f - t;
							addPoint(u * u * u * p0 + 3.f * u * u * t * p1 + 3.f * u * t * t * p2 + t * t * t * p3);
						}
						addPoint(p3);
						cur = p3;
						break;
					}
					case SvgDocument::Close:
						if (!contours.empty()) contours.back().closed = true;
						break;
					default: break;
				}
			}
			simplifyLastContour();
			if (stats != nullptr) stats->outlinePoints += points.size();
		}

		static float distanceToSegmentSquared(vec2 p, vec2 a, vec2 b) {
			const vec2 ab	   = b - a;
			const float lenSq = glm::dot(ab, ab);
			const float t	   = lenSq > 0 ? std::clamp(glm::dot(p - a, ab) / lenSq, 0.f, 1.f) : 0.f;
			const vec2 d	   = p - (a + ab * t);
			return glm::dot(d, d);
		}

		// Drops points that sit within simplifyTolerance of the line through their
		// kept neighbours ("opening window").
		void simplifyLastContour() {
			if (contours.empty()) return;
			auto &c = contours.back();
			if (c.count < 3) return;
			vec2 *p				 = points.data() + c.start;
			const float tolSq	 = simplifyTolerance * simplifyTolerance;
			const uint32_t n	 = c.count;
			uint32_t kept		 = 1;
			uint32_t anchor		 = 0;
			for (uint32_t i = 2; i < n; i++) {
				bool within = i - anchor <= 32; // bounds the work on long straight runs
				for (uint32_t j = anchor + 1; within && j < i; j++) {
					within = distanceToSegmentSquared(p[j], p[anchor], p[i]) <= tolSq;
				}
				if (!within) {
					anchor	  = i - 1;
					p[kept++] = p[anchor];
				}
			}
			p[kept++] = p[n - 1];
			c.count	  = kept;
			points.resize(c.start + kept);
		}

		// ---- fill ----------------------------------------------------------
		void fill(const SvgDocument::Shape &shape) {
			// filling closes every subpath; drop the repeated first point if there is one
			rings.clear();
			for (const auto &c: contours) {
				size_t n	  = c.count;
				const vec2 *p = points.data() + c.start;
				if (n > 1 && p[0] == p[n - 1]) --n;
				if (n >= 3) rings.push_back({p, n});
			}
			if (rings.empty()) return;

			if (rings.size() == 1 && isConvex(rings[0].data, rings[0].count)) {
				const auto &r	 = rings[0];
				const auto first = static_cast<uint32_t>(out.verts.size());
				for (size_t i = 0; i < r.count; i++)
					vert(r.data[i], shape.fill);
				for (uint32_t i = 1; i + 1 < r.count; i++)
					tri(first, first + i, first + i + 1);
				if (stats != nullptr) ++stats->convexFills;
				return;
			}

			if (stats != nullptr) ++stats->triangulated;
			triangulate(shape);
		}

		// libtess2: a sweep line that resolves the fill rule itself, so subpaths can
		// nest, overlap or cross themselves
		void triangulate(const SvgDocument::Shape &shape) {
			if (tess == nullptr) {
				TESSalloc alloc {};
				alloc.memalloc	 = tessAlloc;
				alloc.memrealloc = tessRealloc;
				alloc.memfree	 = tessFree;
				// icons are tens to hundreds of points - the defaults (512) allocate
				// far more than that up front
				alloc.meshEdgeBucketSize   = 128;
				alloc.meshVertexBucketSize = 128;
				alloc.meshFaceBucketSize   = 64;
				alloc.dictNodeBucketSize   = 128;
				alloc.regionBucketSize	   = 64;
				alloc.extraVertices		   = 64;
				tess					   = tessNewTess(&alloc);
				if (tess == nullptr) return;
			}
			for (const auto &r: rings) {
				tessAddContour(tess, 2, r.data, sizeof(vec2), static_cast<int>(r.count));
			}
			const TESSreal normal[3] = {0, 0, 1};
			const int rule			 = shape.evenOdd ? TESS_WINDING_ODD : TESS_WINDING_NONZERO;
			if (!tessTesselate(tess, rule, TESS_POLYGONS, 3, 2, normal)) {
				// a failed tessellation leaves the mesh half built
				tessDeleteTess(tess);
				tess = nullptr;
				return;
			}
			const int numVerts		 = tessGetVertexCount(tess);
			const int numTris		 = tessGetElementCount(tess);
			const TESSreal *verts	 = tessGetVertices(tess);
			const TESSindex *indices = tessGetElements(tess);
			const auto first		 = static_cast<uint32_t>(out.verts.size());
			for (int i = 0; i < numVerts; i++)
				vert({verts[i * 2], verts[i * 2 + 1]}, shape.fill);
			for (int i = 0; i < numTris * 3; i += 3) {
				if (indices[i] == TESS_UNDEF || indices[i + 1] == TESS_UNDEF || indices[i + 2] == TESS_UNDEF)
					continue;
				tri(first + indices[i], first + indices[i + 1], first + indices[i + 2]);
			}
		}

		// ---- stroke --------------------------------------------------------
		static vec2 rotate(vec2 v, float angle) {
			const float c = std::cos(angle), s = std::sin(angle);
			return {v.x * c - v.y * s, v.x * s + v.y * c};
		}

		// number of segments to draw an arc of the given angle within tolerance
		[[nodiscard]] int arcSteps(float radius, float angle) const {
			const float maxStep =
				radius > options.tolerance ? 2.f * std::acos(1.f - options.tolerance / radius) : kPi * 0.5f;
			return std::clamp(static_cast<int>(std::ceil(std::abs(angle) / maxStep)), 1, 256);
		}

		// fan round `centre`, sweeping `from` (relative to centre) through `angle`
		// to reach vertex `last`, pivoting on `pivot`
		void arcFan(vec2 centre, vec2 from, float angle, uint32_t pivot, uint32_t first, uint32_t last, const vec4 &col) {
			const int steps = arcSteps(glm::length(from), angle);
			uint32_t prev	= first;
			for (int i = 1; i < steps; i++) {
				const uint32_t v = vert(centre + rotate(from, angle * static_cast<float>(i) / static_cast<float>(steps)), col);
				tri(pivot, prev, v);
				prev = v;
			}
			tri(pivot, prev, last);
		}

		struct Pair {
			uint32_t l, r; // the +normal side and the -normal side
		};

		// Builds the join at p between an incoming and an outgoing segment.
		// `end` closes the incoming segment's quad, `begin` opens the outgoing one.
		void join(vec2 p,
				  vec2 dIn,
				  float lenIn,
				  vec2 dOut,
				  float lenOut,
				  float hw,
				  const SvgDocument::Shape &shape,
				  Pair &end,
				  Pair &begin) {
			const vec4 &col = shape.stroke;
			const vec2 nIn(-dIn.y * hw, dIn.x * hw);
			const vec2 nOut(-dOut.y * hw, dOut.x * hw);
			const float cr	= cross(dIn, dOut);
			const float dot = glm::dot(dIn, dOut);

			if (std::abs(cr) < 1e-4f && dot > 0) { // straight on
				end = begin = {vert(p + nIn, col), vert(p - nIn, col)};
				return;
			}

			// the turn is towards +normal when cr > 0, so the outside of the corner is -normal
			const float side   = cr < 0 ? 1.f : -1.f;
			const bool hasMiter = dot > -0.9999f; // otherwise it doubles back on itself
			vec2 miter(0, 0);
			float miterRatio = 1e9f;
			if (hasMiter) {
				miter	   = (nIn + nOut) / (1.f + dot);
				miterRatio = glm::length(miter) / hw;
			}

			// inside of the corner: one shared point where the two edges meet, unless
			// the segments are too short for the edges to meet before they run out
			uint32_t innerIn, innerOut, pivot;
			const float shortest = std::min(lenIn, lenOut);
			if (hasMiter && glm::dot(miter, miter) <= shortest * shortest + hw * hw) {
				innerIn = innerOut = pivot = vert(p - side * miter, col);
			} else {
				innerIn	 = vert(p - side * nIn, col);
				innerOut = vert(p - side * nOut, col);
				pivot	 = vert(p, col);
				tri(pivot, innerIn, innerOut);
			}

			uint32_t outerIn, outerOut;
			if (shape.join == SvgDocument::Join::Miter && hasMiter && miterRatio <= shape.miterLimit) {
				outerIn = outerOut = vert(p + side * miter, col);
			} else {
				outerIn	 = vert(p + side * nIn, col);
				outerOut = vert(p + side * nOut, col);
				if (shape.join == SvgDocument::Join::Round) {
					arcFan(p, side * nIn, std::atan2(cr, dot), pivot, outerIn, outerOut, col);
				} else {
					tri(pivot, outerIn, outerOut);
				}
			}

			if (side > 0) {
				end	  = {outerIn, innerIn};
				begin = {outerOut, innerOut};
			} else {
				end	  = {innerIn, outerIn};
				begin = {innerOut, outerOut};
			}
		}

		void quad(Pair a, Pair b) {
			tri(a.l, a.r, b.l);
			tri(a.r, b.r, b.l);
		}

		void strokeContour(const Contour &contour, const SvgDocument::Shape &shape, float hw) {
			const vec4 &col = shape.stroke;
			const vec2 *p	= points.data() + contour.start;
			size_t n		= contour.count;
			bool closed		= contour.closed;
			if (closed && n > 1 && p[0] == p[n - 1]) --n;
			if (n < 2) return;
			if (n < 3) closed = false;

			const size_t numSegments = closed ? n : n - 1;
			auto direction			 = [&](size_t i, float &len) {
				  const vec2 d = p[(i + 1) % n] - p[i];
				  len		   = glm::length(d);
				  return d / len;
			};

			float lenPrev = 0, lenNext = 0;
			Pair current {}, firstEnd {};
			vec2 dPrev;

			if (closed) {
				const vec2 dLast = direction(n - 1, lenPrev);
				dPrev			 = direction(0, lenNext);
				join(p[0], dLast, lenPrev, dPrev, lenNext, hw, shape, firstEnd, current);
			} else {
				dPrev = direction(0, lenNext);
				const vec2 normal(-dPrev.y * hw, dPrev.x * hw);
				const vec2 at = shape.cap == SvgDocument::Cap::Square ? p[0] - dPrev * hw : p[0];
				current		  = {vert(at + normal, col), vert(at - normal, col)};
				if (shape.cap == SvgDocument::Cap::Round) {
					arcFan(p[0], normal, kPi, current.l, current.l, current.r, col);
				}
			}

			for (size_t i = 1; i < numSegments; i++) {
				lenPrev = lenNext;
				const vec2 dNext = direction(i, lenNext);
				Pair end {}, begin {};
				join(p[i], dPrev, lenPrev, dNext, lenNext, hw, shape, end, begin);
				quad(current, end);
				current = begin;
				dPrev	= dNext;
			}

			if (closed) {
				quad(current, firstEnd);
			} else {
				const vec2 normal(-dPrev.y * hw, dPrev.x * hw);
				const vec2 at = shape.cap == SvgDocument::Cap::Square ? p[n - 1] + dPrev * hw : p[n - 1];
				const Pair end {vert(at + normal, col), vert(at - normal, col)};
				quad(current, end);
				if (shape.cap == SvgDocument::Cap::Round) {
					arcFan(p[n - 1], -normal, kPi, end.r, end.r, end.l, col);
				}
			}
		}

		void stroke(const SvgDocument::Shape &shape, float scale) {
			const float hw = shape.strokeWidth * scale * 0.5f;
			if (hw <= 0) return;
			for (const auto &c: contours)
				strokeContour(c, shape, hw);
		}
	};
} // namespace

void SvgDocument::tessellate(const glm::mat3 &transform,
							 Geometry &geom,
							 const Options &options,
							 Stats *stats) const {
	const Affine xf {transform[0][0], transform[0][1], transform[1][0], transform[1][1], transform[2][0], transform[2][1]};

	// a guess that's right for line art and a decent head start for curves
	geom.verts.reserve(geom.verts.size() + coords.size() / 2);
	if (options.colors) geom.cols.reserve(geom.cols.size() + coords.size() / 2);
	geom.indices.reserve(geom.indices.size() + coords.size() * 3 / 2);

	Tessellator t(geom, options, stats);
	if (stats != nullptr) stats->shapes += shapes.size();

	for (const auto &shape: shapes) {
		const Affine m = xf * shape.transform;
		t.flatten(shape, m, cmds.data(), coords.data());
		if (shape.hasFill) t.fill(shape);
		if (shape.hasStroke) t.stroke(shape, std::sqrt(std::abs(m.a * m.d - m.b * m.c)));
	}
}
