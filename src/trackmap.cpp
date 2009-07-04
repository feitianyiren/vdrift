#include "trackmap.h"

#ifdef __APPLE__
#include <SDL_gfx/SDL_gfxPrimitives.h>
#else
#include <SDL/SDL_gfxPrimitives.h> //part of the SDL_gfx package
#endif


using std::endl;
using std::string;
using std::ostream;
using std::list;
using std::pair;

TRACKMAP::TRACKMAP() : scale(1.0), MAP_WIDTH(256),MAP_HEIGHT(256),surface(NULL),mapnode(NULL),mapdraw(NULL)
{
}

TRACKMAP::~TRACKMAP()
{
	Unload();
}

void TRACKMAP::Unload()
{
	assert((!mapnode && !mapdraw) || (mapnode && mapdraw));
	if (mapnode)
	{
		for (list <CARDOT>::iterator i = dotlist.begin(); i != dotlist.end(); i++)
			mapnode->Delete(i->GetDrawable());
		dotlist.clear();
		
		mapnode->Delete(mapdraw);
		mapnode->GetParent()->Delete(mapnode);
		mapnode = NULL;
		mapdraw = NULL;
	}
	if (surface)
	{
		SDL_FreeSurface(surface);
		surface = NULL;
	}
	if (track_map.Loaded()) track_map.Unload();
	if (cardot0.Loaded()) cardot0.Unload();
	if (cardot1.Loaded()) cardot1.Unload();
	if (cardot0_focused.Loaded()) cardot0_focused.Unload();
	if (cardot1_focused.Loaded()) cardot1_focused.Unload();
}

void TRACKMAP::CalcPosition(int w, int h)
{
	screen[0] = (float)w;
	screen[1] = (float)h;
	position[0] = 1.0 - MAP_WIDTH / screen[0];
	position[1] = 0.12;
	size[0] = MAP_WIDTH / screen[0];
	size[1] = MAP_HEIGHT / screen[1];
	dot_size[0] = cardot0.GetW() / 2.0 / screen[0]; 
	dot_size[1] = cardot0.GetH() / 2.0 / screen[1]; 
}

/*void TRACKMAP::DrawCarDot(int car_index, Vamos_Geometry::Three_Vector position, bool focused)
{
	VERTEX car_pos;
	car_pos.x = position[0];
	car_pos.y = position[2];
	car_pos.z = -position[1];

	float x = position_x + ((car_pos.x - map_w_min) * scale + 1) / screen_w;
	float y = position_y + ((car_pos.z - map_h_min) * scale + 1) / screen_h;

	if (car_index == 0) // player car
	{
		if (focused)
			game.utility.Draw2D(x - dot_w_half, y - dot_h_half, x + dot_w_half, y + dot_h_half, &cardot0_focused);
		else
			game.utility.Draw2D(x - dot_w_half, y - dot_h_half, x + dot_w_half, y + dot_h_half, &cardot0);
	}
	else // opponent car
	{
		if (focused)
			game.utility.Draw2D(x - dot_w_half, y - dot_h_half, x + dot_w_half, y + dot_h_half, &cardot1_focused);
		else
			game.utility.Draw2D(x - dot_w_half, y - dot_h_half, x + dot_w_half, y + dot_h_half, &cardot1);
	}
}

void TRACKMAP::DrawMap() { game.utility.Draw2D(position_x, position_y, position_x + width, position_y + height, &track_map); }
*/
bool TRACKMAP::BuildMap(SCENENODE * parentnode, const std::list <ROADSTRIP> & roads, int w, int h, const std::string & texturepath, const std::string & texsize, std::ostream & error_output)
{
	Unload();
	
	int outsizex = MAP_WIDTH;
	int outsizey = MAP_HEIGHT;
	
	Uint32 rmask, gmask, bmask, amask;

	// SDL interprets each pixel as a 32-bit number, so our masks must depend
	// on the endianness (byte order) of the machine
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
	rmask = 0xff000000;
	gmask = 0x00ff0000;
	bmask = 0x0000ff00;
	amask = 0x000000ff;
#else
	rmask = 0x000000ff;
	gmask = 0x0000ff00;
	bmask = 0x00ff0000;
	amask = 0xff000000;
#endif
	
	surface = SDL_CreateRGBSurface(SDL_SWSURFACE, outsizex, outsizey, 32, rmask, gmask, bmask, amask);
	
	//find the map width and height
	mapsize.Set(0,0);
	map_w_min = FLT_MAX;
	map_w_max = FLT_MIN;
	map_h_min = FLT_MAX;
	map_h_max = FLT_MIN;

	for (list <ROADSTRIP>::const_iterator road = roads.begin(); road != roads.end(); road++)
	{
		for (list <ROADPATCH>::const_iterator curp = road->GetPatchList().begin();
		     curp != road->GetPatchList().end(); curp++)
		{
			for (int i = 0; i < 4; i++)
			{
				for (int j = 0; j < 4; j++)
				{
					MATHVECTOR <float, 3> p = curp->GetPatch()[i+j*4];
					if (p[0] < map_w_min)
					{
						map_w_min = p[0];
					}
					if (p[0] > map_w_max)
					{
						map_w_max = p[0];
					}
					if (p[2] < map_h_min)
					{
						map_h_min = p[2];
					}
					if (p[2] > map_h_max)
					{
						map_h_max = p[2];
					}
				}
			}
		}
	}

	mapsize[0] = map_w_max - map_w_min;
	mapsize[1] = map_h_max - map_h_min;

	//determine the scaling factor
	//we will leave a 1 pixel border
	float scale_w = (outsizex-2) / mapsize[0];
	float scale_h = (outsizey-2) / mapsize[1];
	scale = (scale_w < scale_h)?scale_w:scale_h;

	boxRGBA(surface, 0, 0, outsizex-1, outsizey-1, 0, 0, 0, 0);

	for (list <ROADSTRIP>::const_iterator road = roads.begin(); road != roads.end(); road++)
	{
		for (list <ROADPATCH>::const_iterator curp = road->GetPatchList().begin();
		     curp != road->GetPatchList().end(); curp++)
		{
			Sint16 x[4], y[4];

			const BEZIER & b(curp->GetPatch());
			MATHVECTOR <float, 3> back_l = b.GetBL();
			MATHVECTOR <float, 3> back_r = b.GetBR();
			MATHVECTOR <float, 3> front_l = b.GetFL();
			MATHVECTOR <float, 3> front_r = b.GetFR();

			x[0] = int((back_l[0] - map_w_min) * scale) + 1;
			y[0] = int((back_l[2] - map_h_min) * scale) + 1;
			x[1] = int((front_l[0] - map_w_min) * scale) + 1;
			y[1] = int((front_l[2] - map_h_min) * scale) + 1;
			x[2] = int((front_r[0] - map_w_min) * scale) + 1;
			y[2] = int((front_r[2] - map_h_min) * scale) + 1;
			x[3] = int((back_r[0] - map_w_min) * scale) + 1;
			y[3] = int((back_r[2] - map_h_min) * scale) + 1;
			filledPolygonRGBA(surface, x, y, 4, 255, 255, 255, 255);
			aapolygonRGBA(surface, x, y, 4, 255, 255, 255, 255);
		}
	}

	//draw the starting line
	/*BEZIER* startpatch = track->GetLapSequence(0);
	if (startpatch)
	{
		VERTEX startline_l = startpatch->points[3][0];
		VERTEX startline_r = startpatch->points[3][3];
		int x1 = int((startline_l.x - map_w_min) * scale) + 1;
		int y1 = int((startline_l.z - map_h_min) * scale) + 1;
		int x2 = int((startline_r.x - map_w_min) * scale) + 1;
		int y2 = int((startline_r.z - map_h_min) * scale) + 1;
		aalineRGBA(surface, x1, y1, x2, y2, 255, 0, 0, 255);
		filledCircleRGBA(surface, x1, y1, 2, 255, 0, 0, 255);
		filledCircleRGBA(surface, x2, y2, 2, 255, 0, 0, 255);
	}*/

	//draw a black border around the track
	Uint32* rawpixels = (Uint32*)surface->pixels;
	Uint32 rgbmask = rmask | gmask | bmask;

	for (int x = 0; x < outsizex; x++)
	{
		for (int y = 0; y < outsizey; y++)
		{
			//if this pixel is black
			if (rawpixels[outsizex * y + x] == 0)
			{
				//if the pixel above this one is non-black
				if ((y > 0) && ((rawpixels[outsizex * (y-1) + x] & rgbmask) > 0))
				{
					//set this pixel to non-transparent
					rawpixels[outsizex * y + x] |= amask;
				}
				//if the pixel left of this one is non-black
				if ((x > 0) && ((rawpixels[outsizex * y + x - 1] & rgbmask) > 0))
				{
					//set this pixel to non-transparent
					rawpixels[outsizex * y + x] |= amask;
				}
				//if the pixel right of this one is non-black
				if ((x < (outsizex - 1)) && ((rawpixels[outsizex * y + x + 1] & rgbmask) > 0))
				{
					//set this pixel to non-transparent
					rawpixels[outsizex * y + x] |= amask;
				}
				//if the pixel below this one is non-black
				if ((y < (outsizey - 1)) && ((rawpixels[outsizex * (y+1) + x] & rgbmask) > 0))
				{
					//set this pixel to non-transparent
					rawpixels[outsizex * y + x] |= amask;
				}
			}
		}
	}
	
	TEXTUREINFO texinfo;
	texinfo.SetSurface(surface);
	texinfo.SetRepeat(false, false);
	if (!track_map.Load(texinfo, error_output, texsize)) return false;
	
	if (!cardot0.Load(TEXTUREINFO(texturepath+"/cardot0.png"), error_output, texsize)) return false;
	if (!cardot1.Load(TEXTUREINFO(texturepath+"/cardot1.png"), error_output, texsize)) return false;
	if (!cardot0_focused.Load(TEXTUREINFO(texturepath+"/cardot0_focused.png"), error_output, texsize)) return false;
	if (!cardot1_focused.Load(TEXTUREINFO(texturepath+"/cardot1_focused.png"), error_output, texsize)) return false;
	
	CalcPosition(w, h);
	
	assert(parentnode);
	mapnode = &parentnode->AddNode();
	assert(mapnode);
	mapdraw = &mapnode->AddDrawable();
	assert(mapdraw);
	mapdraw->SetDiffuseMap(&track_map);
	mapdraw->SetVertArray(&mapverts);
	mapdraw->SetLit(false);
	mapdraw->Set2D(true);
	mapdraw->SetCull(false, false);
	mapdraw->SetColor(1,1,1,0.7);
	mapdraw->SetDrawOrder(0);
	mapverts.SetToBillboard(position[0], position[1], position[0]+size[0], position[1]+size[1]);
	
	return true;
}

void TRACKMAP::Update(bool mapvisible, const std::list <std::pair<MATHVECTOR <float, 3>, bool> > & carpositions)
{
	//only update car positions when the map is visible, so we get a slight speedup if the map is hidden
	if (mapvisible)
	{
		std::list <std::pair<MATHVECTOR <float, 3>, bool> >::const_iterator car = carpositions.begin();
		std::list <CARDOT>::iterator dot = dotlist.begin();
		int count = 0;
		while (car != carpositions.end())
		{
			//determine which texture to use
			TEXTURE_GL * tex = &cardot0_focused;
			if (!car->second)
				tex = &cardot1;
			
			//find the coordinates of the dot
			MATHVECTOR <float, 2> dotpos = position;
			dotpos[0] += ((car->first[1] - map_w_min)*scale + 1) / screen[0];
			dotpos[1] += ((car->first[0] - map_h_min)*scale + 1) / screen[1];
			MATHVECTOR <float, 2> corner1 = dotpos - dot_size;
			MATHVECTOR <float, 2> corner2 = dotpos + dot_size;
			
			if (dot == dotlist.end())
			{
				//need to insert a new dot
				dotlist.push_back(CARDOT(mapnode, *tex, corner1, corner2));
				dot = dotlist.end();
				
				//std::cout << count << ". inserting new dot: " << corner1 << " || " << corner2 << endl;
			}
			else
			{
				//update existing dot
				dot->Retexture(*tex);
				dot->Reposition(corner1, corner2);
				
				//std::cout << count << ". reusing existing dot: " << corner1 << " || " << corner2 << endl;
				
				dot++;
			}
			
			car++;
			count++;
		}
		for (list <CARDOT>::iterator i = dot; i != dotlist.end(); i++)
			mapnode->Delete(i->GetDrawable());
		dotlist.erase(dot,dotlist.end());
	}
	
	if (mapdraw)
		mapdraw->SetDrawEnable(mapvisible);
	for (list <CARDOT>::iterator i = dotlist.begin(); i != dotlist.end(); i++)
		i->SetVisible(mapvisible);
	
	/*for (list <CARDOT>::iterator i = dotlist.begin(); i != dotlist.end(); i++)
		i->DebugPrint(std::cout);*/
}
