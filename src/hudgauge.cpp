#include "hudgauge.h"
#include "text_draw.h"

#include <sstream>

HUDGAUGE::HUDGAUGE() :
	centerx(0),
	centery(0),
	scalex(1),
	scaley(1),
	startangle(0),
	endangle(0),
	scale(1)
{
	// ctor
}

void HUDGAUGE::Set(
	SCENENODE & parent,
	const FONT & font,
	float hwratio,
	float centerx,
	float centery,
	float radius,
	float startangle,
	float endangle,
	float startvalue,
	float endvalue,
	float valuedelta)
{
	// calculate number of segments (max 9)
	float segments = (endvalue - startvalue) / valuedelta;
	float factor = ceil(segments / 9.0f);
	segments = ceil(segments / factor);
	valuedelta = valuedelta * factor;
	endvalue = startvalue + segments * valuedelta;

	this->centerx = centerx;
	this->centery = centery;
	this->scalex = radius * hwratio;
	this->scaley = radius;
	this->startangle = startangle;
	this->endangle = endangle;
	this->scale = (endangle - startangle) / (endvalue - startvalue);

	// reset
	if (dialnum_draw.valid()) parent.GetDrawlist().text.erase(dialnum_draw);
	if (pointer_draw.valid()) parent.GetDrawlist().twodim.erase(pointer_draw);
	if (dial_draw.valid()) 	parent.GetDrawlist().twodim.erase(dial_draw);
	pointer_rotated.Clear();
	pointer.Clear();
	dial_label.Clear();
	dial_marks.Clear();

	// dial marks
	{
		// big marker
		float pb[] = {-0.02, 1, 0, 0.02, 1, 0, 0.02, 0.92, 0, -0.02, 0.92, 0};
		float t[] = {0, 0, 1, 0, 1, 1, 0, 1};
		int f[] = {0, 2, 1, 0, 3, 2};
		VERTEXARRAY bm;
		bm.SetVertices(pb, 12);
		bm.SetTexCoordSets(1);
		bm.SetTexCoords(0, t, 8);
		bm.SetFaces(f, 6);

		// small marker
		float ps[] = {-0.01, 1, 0, 0.01, 1, 0, 0.01, 0.95, 0, -0.01, 0.95, 0};
		VERTEXARRAY sm;
		sm.SetVertices(ps, 12);
		sm.SetTexCoordSets(1);
		sm.SetTexCoords(0, t, 8);
		sm.SetFaces(f, 6);

		float delta = (endangle - startangle) / (3.0 * segments);
		float angle = startangle;
		for (int i = 0; i <= 3 * segments; ++i)
		{
			VERTEXARRAY temp = (i % 3) ? sm : bm;
			temp.Rotate(angle, 0, 0, -1);
			dial_marks = dial_marks + temp;
			angle = angle + delta;
		}
		dial_marks.Scale(radius * hwratio, radius, 1);
		dial_marks.Translate(centerx, centery, 0.0);

		dial_draw = parent.GetDrawlist().twodim.insert(DRAWABLE());
		DRAWABLE & drawref = parent.GetDrawlist().twodim.get(dial_draw);
		drawref.SetVertArray(&dial_marks);
		drawref.SetCull(false, false);
		//drawref.SetColor(1, 1, 1, 0.5);
		//drawref.SetDrawOrder(4);
	}

	// dial label
	{
		VERTEXARRAY temp;
		float w = 0.25 * radius * hwratio;
		float h = 0.25 * radius;
		float angle = startangle;
		float angle_delta = (endangle - startangle) / segments;
		float value = startvalue;
		float value_delta = (endvalue - startvalue) / segments;
		for (int i = 0; i <= segments; ++i)
		{
			std::stringstream sstr;
			std::string text;
			sstr << value;
			sstr >> text;
			float x = centerx + 0.75 * sin(angle) * radius * hwratio;
			float y = centery + 0.75 * cos(angle) * radius;
			float xn = TEXT_DRAW::RenderText(font, text, x, y, w, h, temp);
			temp.Translate((x - xn) * 0.5, 0, 0);
			dial_label = dial_label + temp;
			angle += angle_delta;
			value += value_delta;
		}

		dialnum_draw = parent.GetDrawlist().text.insert(DRAWABLE());
		DRAWABLE & drawref = parent.GetDrawlist().text.get(dialnum_draw);
		drawref.SetDiffuseMap(font.GetFontTexture());
		drawref.SetVertArray(&dial_label);
		drawref.SetCull(false, false);
		//drawref.SetColor(1, 1, 1, 0.5);
		//drawref.SetDrawOrder(4);
	}

	// pointer
	{
		float p[] = {-0.015, 0.92, 0, 0.015, 0.92, 0, 0.025, -0.1, 0, -0.025, -0.1, 0};
		float t[] = {0, 0, 1, 0, 1, 1, 0, 1};
		int f[] = {0, 2, 1, 0, 3, 2};
		pointer.SetVertices(p, 12);
		pointer.SetTexCoordSets(1);
		pointer.SetTexCoords(0, t, 8);
		pointer.SetFaces(f, 6);

		pointer_draw = parent.GetDrawlist().twodim.insert(DRAWABLE());
		DRAWABLE & drawref = parent.GetDrawlist().twodim.get(pointer_draw);
		drawref.SetVertArray(&pointer_rotated);
		drawref.SetCull(false, false);
		//drawref.SetColor(1, 1, 1, 0.5);
		//drawref.SetDrawOrder(4);
	}
}

void HUDGAUGE::Revise(
	SCENENODE & parent,
	const FONT & font,
	float startvalue,
	float endvalue,
	float valuedelta)
{
	Set(parent, font, scalex / scaley, centerx, centery, scaley,
		startangle, endangle, startvalue, endvalue, valuedelta);
}

void HUDGAUGE::Update(SCENENODE & parent, float value)
{
	float angle = value * scale + startangle;
	pointer_rotated = pointer;
	pointer_rotated.Rotate(angle, 0, 0, -1);
	pointer_rotated.Scale(scalex, scaley, 1);
	pointer_rotated.Translate(centerx, centery, 0);
}