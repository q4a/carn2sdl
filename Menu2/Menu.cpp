/***************************************************
* AtmosFear 2.1
* Menu.cpp
*
* Main Menu Code
*
*/

#include "Hunt.h"
#include <cmath>

#include <iostream>
#include <iomanip>
#include <sstream>
#include <cstring>
#include <thread>


class MenuSet {
public:
	int32_t		x0, y0;
	uint32_t	Count;
	uint32_t	Offset;
	std::vector<std::pair<std::string, bool>> Item;
	int32_t		Selected;
	int32_t		Hilite;
	SDL_Rect	Rect;
	int32_t		Padding;

	MenuSet() :
		x0(0), y0(0),
		Count(0),
		Offset(0),
		Item(),
		Selected(-1),
		Hilite(-1),
		Rect({ 0,0,0,0 }),
		Padding(0)
	{}

	void AddItem(const std::string& txt);
};


enum MenuSetEnum {
	OPT_GAME = 0,
	OPT_KEYBINDINGS = 1,
	OPT_VIDEO = 2,
	OPT_MAX
};


enum DrawTextAlignEnum {
	// Uses the same values as wingdi.h TA_LEFT and so on.
	// TODO: Write a conversion function for Linux if we port it to Linux
	DTA_LEFT = 0,
	DTA_RIGHT = 2,
	DTA_CENTER = 6,
	DTA_BOTTOM = 8
};


// Private prototypes
void InterfaceSetFont(TTF_Font*);
void DrawRectangle(int, int, int, int, int);
void DrawPicture(int, int, int, int, uint16_t*);
void DrawTextShadow(int x, int y, const std::string& text, uint32_t color, int align);

std::pair<unsigned, unsigned> g_HuntInfo;
Picture g_TrackBar[2];
MenuSet MenuOptions[3];
MenuSet MenuRegistry;
MenuSet MenuHunt[4];

SDL_Surface* drawSurface;
TTF_Font* fontSmall;
TTF_Font* fontMid;
TTF_Font* fontBig;
TTF_Font* gFont;
SDL_Point g_CursorPos;
int g_MouseState;
int g_WaitKey = -1;
uint32_t gCurrColor; // 32 bit ARGB value

// String table
const char g_GitHubURL[] = "https://github.com/carnivores-cpe/Carn2-Menu";
const char st_BoolText[2][4] = { "Off", "On" };
const char st_UnitText[2][10] = { "Metric", "Imperial" };
const char st_HMLText[4][8] = { "Low", "Medium", "High", "Ultra" };
const char st_TextureText[3][5] = { "Low", "High", "Auto" };
const char st_AlphaKeyText[2][14] = { "Color Key", "Alpha Channel" };
const char st_RenText[4][11] = { "Software", "3Dfx Glide", "Direct3D 7", "OpenGL" };
const char st_AudText[5][16] = { "Software", "Direct Sound 3D", "Aureal 3D", "EAX", "OpenAL" };

const char g_RendererFile[4][7] = { "v_soft", "v_3dfx", "v_d3d", "v_gl" };


int MapVKKey(int k);


bool IsPointInRect(SDL_Point& p, SDL_Rect& rc)
{
	return (p.x > rc.x && p.y > rc.y && p.x < rc.x+rc.w && p.y < rc.y+rc.h);
}


void WaitForMouseRelease()
{
	const int mask = SDL_BUTTON(1) | SDL_BUTTON(2) | SDL_BUTTON(3);
	int state;
	do {
		SDL_PumpEvents();
		state = SDL_GetMouseState(NULL, NULL);
	} while ((state & mask) != 0);
}

void AcceptNewKey()
{
	const uint8_t* keystate;

	if ((keystate = SDL_GetKeyboardState(NULL)) != NULL) {

		if (keystate[SDL_SCANCODE_ESCAPE])
		{
			*((uint32_t*)(&g_Options.KeyMap) + g_WaitKey) = 0;
			g_WaitKey = -2;
			return;
		}
		else
		{
			for (int k = 0; k < 255; k++)
			{
				if (keystate[k])
				{
					for (int t = 0; t < 16; t++)
						if (*((uint32_t*)(&g_Options.KeyMap) + t) == k)
							*((uint32_t*)(&g_Options.KeyMap) + t) = 0;

					*((int*)(&g_Options.KeyMap) + g_WaitKey) = k;

#ifdef _DEBUG
					std::cout << "AcceptNewKey() : " << ((int)k) << " MapVK: " << (MapVKKey(k)) << std::endl;
#endif //_DEBUG

					WaitForMouseRelease();
					g_WaitKey = -1;
					return;
				}
			}
		}
	}
}


void ChangeMenuState(int32_t ms)
{
	g_PrevMenuState = g_MenuState;
	g_MenuState = ms;
	LoadGameMenu(g_MenuState);
}


int GetTextW(const std::string& s)
{
	int w = 0;
	TTF_SizeText(gFont, s.c_str(), &w, NULL);
	return w;
}


int GetTextH(const std::string& s)
{
	int h = 0;
	TTF_SizeText(gFont, s.c_str(), NULL, &h);
	return h;
}


/*
Map the virtual-key-code to a scan-code
*/
int MapVKKey(int k)
{
#ifdef _WIN32
	if (k == VK_LBUTTON) return 124;
	if (k == VK_RBUTTON) return 125;
	if (k == VK_MBUTTON) return 126;
	if (k == VK_XBUTTON1) return 128;
	if (k == VK_XBUTTON2) return 129;
	return MapVirtualKey(k, MAPVK_VK_TO_VSC);
#else
	return k;
#endif
}


void MenuSet::AddItem(const std::string& txt)
{
	this->Item.push_back(std::make_pair(txt, false));
	this->Count = this->Item.size();
}


/*
Determine the amount of points that the current license selection
in the hunt screen will cost.
*/
int32_t CalculateDebit()
{
	int32_t debit = 0;

	debit += g_AreaInfo[MenuHunt[0].Selected].m_Price;

	for (unsigned i = 0; i < MenuHunt[1].Item.size(); i++)
	{
		if (MenuHunt[1].Item[i].second)
		{
			debit += g_DinoInfo[g_DinoList[i]].m_Price;
		}
	}

	for (unsigned i = 0; i < MenuHunt[2].Item.size(); i++)
	{
		if (MenuHunt[2].Item[i].second)
		{
			debit += g_WeapInfo[i].m_Price;
		}
	}

	// TODO: Accessories

	return debit;
}


/*
Currently unused
void CALLBACK WaveOutProc(HWAVEOUT hwo, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{

}
*/


/*
Currently unused
void AudioSoftThread()
{
	HWAVEOUT hwo;
	WAVEOUTCAPS woc;
	WAVEFORMATEX wfx;
	unsigned device = WAVE_MAPPER;

	wfx.cbSize = 0;
	wfx.wFormatTag = WAVE_FORMAT_PCM;
	wfx.nChannels = 1;
	wfx.nSamplesPerSec = 22050;
	wfx.nBlockAlign = 2;
	wfx.wBitsPerSample = 16;
	wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;

	std::cout << "Audio: Checking " << waveInGetNumDevs() << " devices for wave out support..." << std::endl;
	for (unsigned i = 0; i < waveInGetNumDevs(); i++)
		if (waveOutOpen(&hwo, i, &wfx, NULL, NULL, CALLBACK_EVENT | WAVE_FORMAT_QUERY) == MMSYSERR_NOERROR)
		{
			device = i;

			std::cout << "WaveOut Device ID: " << device << std::endl;
			std::cout << std::setw(20) << std::setfill('=') << "=" << std::endl;

			waveOutGetDevCaps(device, &woc, sizeof(WAVEOUTCAPS));
			std::cout << "Manufacturer ID: " << woc.wMid << std::endl;
			std::cout << "Product ID: " << woc.wPid << std::endl;
			std::cout << "Driver Version: " << (HIWORD(woc.vDriverVersion)) << "." << (LOWORD(woc.vDriverVersion)) << std::endl;
			std::cout << "Product Name: " << woc.szPname << std::endl;
			std::cout << "Channels: " << woc.wChannels << std::endl;

			std::cout << "Supported formats:" << std::endl;
			if (woc.dwFormats & WAVE_FORMAT_2M08)
				std::cout << "\t22.05 kHz Mono 8-bit" << std::endl;
			if (woc.dwFormats & WAVE_FORMAT_2M16)
				std::cout << "\t22.05 kHz Mono 16-bit" << std::endl;
			if (woc.dwFormats & WAVE_FORMAT_2M08)
				std::cout << "\t22.05 kHz Stereo 8-bit" << std::endl;
			if (woc.dwFormats & WAVE_FORMAT_2M16)
				std::cout << "\t22.05 kHz Stereo 16-bit" << std::endl;

			std::cout << "Supported functionality:" << std::endl;
			if (woc.dwSupport & WAVECAPS_LRVOLUME)
				std::cout << "\tSeparate left and right volume control." << std::endl;
			if (woc.dwSupport & WAVECAPS_PITCH)
				std::cout << "\tPitch control" << std::endl;
			if (woc.dwSupport & WAVECAPS_PLAYBACKRATE)
				std::cout << "\tPlayback rate control" << std::endl;
			if (woc.dwSupport & WAVECAPS_SYNC)
				std::cout << "\tDriver is synchronous" << std::endl;
			if (woc.dwSupport & WAVECAPS_VOLUME)
				std::cout << "\tVolume control" << std::endl;
			if (woc.dwSupport & WAVECAPS_SAMPLEACCURATE)
				std::cout << "\tSample-accurate position" << std::endl;

			break;
		}

	if (waveOutOpen(&hwo, device, &wfx, NULL, (DWORD_PTR)&WaveOutProc, CALLBACK_FUNCTION | WAVE_FORMAT_QUERY) != MMSYSERR_NOERROR)
	{
		std::cout << "Audio: Failed to open WaveOut device!" << std::endl;
		return;
	}

	waveOutClose(hwo);
}
*/


/*
Initialise all the required settings and interface elements
*/
void InitInterface()
{
	std::cout << "Interface: Creating Fonts..." << std::endl;

	if (TTF_Init() < 0) {
		std::stringstream ss;
		ss << "Failed to initialize TTF: " << TTF_GetError() << std::endl;
		throw std::runtime_error(ss.str());
	}

	fontSmall = TTF_OpenFont("LibreBaskerville-Regular.ttf", 10);
	fontMid = TTF_OpenFont("LibreBaskerville-Regular.ttf", 12);
	fontBig = TTF_OpenFont("LibreBaskerville-Regular.ttf", 18);
	if (!fontSmall)
		std::cout << "Interface : Failed to create Small Font" << std::endl;
	if (!fontMid)
		std::cout << "Interface : Failed to create Medium Font" << std::endl;
	if (!fontBig)
		std::cout << "Interface : Failed to create Large Font" << std::endl;

	// Default font is mid size
	gFont = fontMid;

	drawSurface = SDL_CreateRGBSurface(0, 800, 600, 16,
		0x1f << 10,
		0x1f << 5,
		0x1f,
		0x00);
	if (!drawSurface) {
		throw std::runtime_error("Interface: Failed to create draw surface!");
	}

	g_PrevMenuState = -1;
	g_MenuState = 0;
	g_TypingBuffer = "";
	LoadGameMenu(0);

	g_ProfileIndex = 0;
	g_HuntSelectPic = nullptr;

	/************************************************************
	* Register/Profile lists
	*/
	MenuRegistry.x0 = 360;
	MenuRegistry.y0 = 363;
	//MenuEventStart(MENU_REGISTER) contains the AddMenuItem() calls
	MenuRegistry.Rect = { 360, 363, 139, 128 }; // Enough room for 9.14 items with fnt_Small

	/************************************************************
	* Options lists
	*/
	int m = OPT_GAME;
	MenuOptions[m].x0 = 40; // 170 HX, 380 W
	MenuOptions[m].y0 = 75;
	MenuOptions[m].Padding = 20;
	MenuOptions[m].Count = 0;
	MenuOptions[m].AddItem("Agressivity");
	MenuOptions[m].AddItem("Density");
	MenuOptions[m].AddItem("Sensitivity");
	MenuOptions[m].AddItem("View range");
	MenuOptions[m].AddItem("Measurement");
	MenuOptions[m].AddItem("Sound API");
	MenuOptions[m].Rect = { 40, 75, 340, (int)MenuOptions[0].Count * 24 };

	m = OPT_KEYBINDINGS;
	MenuOptions[m].x0 = 422;
	MenuOptions[m].y0 = 75;
	MenuOptions[m].Padding = 20;
	MenuOptions[m].Count = 0;
	MenuOptions[m].AddItem("Forward");
	MenuOptions[m].AddItem("Backward");
	MenuOptions[m].AddItem("Turn Up");
	MenuOptions[m].AddItem("Turn Down");
	MenuOptions[m].AddItem("Turn Left");
	MenuOptions[m].AddItem("Turn Right");
	MenuOptions[m].AddItem("Fire");
	MenuOptions[m].AddItem("Draw Weapon");
	MenuOptions[m].AddItem("Step Left");
	MenuOptions[m].AddItem("Step Right");
	MenuOptions[m].AddItem("Strafe");
	MenuOptions[m].AddItem("Jump");
	MenuOptions[m].AddItem("Run");
	MenuOptions[m].AddItem("Crouch");
	MenuOptions[m].AddItem("Lure Call");
	MenuOptions[m].AddItem("Change Call");
	MenuOptions[m].AddItem("Binoculars");
#ifdef _iceage
	MenuOptions[m].AddItem("Call Resupply");
#endif //_iceage
	MenuOptions[m].AddItem("Invert Mouse");
	MenuOptions[m].AddItem("Mouse sensitivity");
	MenuOptions[m].Rect = { 422, 75, 338, (int)MenuOptions[1].Count * 24 };

	m = OPT_VIDEO;
	MenuOptions[m].x0 = 70;
	MenuOptions[m].y0 = 350;
	MenuOptions[m].Padding = 20;
	MenuOptions[m].Count = 0;
	MenuOptions[m].AddItem("Graphics API");
	MenuOptions[m].AddItem("Resolution");
	MenuOptions[m].AddItem("Shadows");
	MenuOptions[m].AddItem("Fog");
	MenuOptions[m].AddItem("Textures");
	MenuOptions[m].AddItem("Alpha Source");
	MenuOptions[m].AddItem("Brightness");
	MenuOptions[m].Rect = { 40, 350, 340, (int)MenuOptions[2].Count * 24 };

	/************************************************************
	* Hunt lists
	*/
	m = 0; // Areas
	MenuHunt[m].x0 = 10;
	MenuHunt[m].y0 = 382;
	MenuHunt[m].Padding = 4;
	MenuHunt[m].Count = 0;
	MenuHunt[m].Offset = 0;
	// ...
	MenuHunt[m].Rect = { 10, 382, 180, 160 };

	m = 1; // Dinosaurs
	MenuHunt[m].x0 = 10;
	MenuHunt[m].y0 = 382;
	MenuHunt[m].Padding = 4;
	MenuHunt[m].Count = 0;
	MenuHunt[m].Offset = 0;
	// ...
	MenuHunt[m].Rect = { 210, 382, 180, 160 };

	m = 2; // Weapons
	MenuHunt[m].x0 = 210;
	MenuHunt[m].y0 = 382;
	MenuHunt[m].Padding = 4;
	MenuHunt[m].Count = 0;
	MenuHunt[m].Offset = 0;
	// ...
	MenuHunt[m].Rect = { 410, 382, 180, 160 };

	m = 3; // Accessories / Utilities
	MenuHunt[m].x0 = 10;
	MenuHunt[m].y0 = 382;
	MenuHunt[m].Padding = 4;
	MenuHunt[m].Count = 0;
	MenuHunt[m].Offset = 0;
	MenuHunt[m].AddItem("Camouflage");
	MenuHunt[m].AddItem("Cover scent");
	MenuHunt[m].AddItem("Radar");
	MenuHunt[m].AddItem("Double ammo");
#ifdef _iceage
	MenuHunt[m].AddItem("Supply drop");
#endif //_iceage
	MenuHunt[m].Rect = { 610, 382, 180, 160 };

	std::cout << "Interface: Initialisation Ok!" << std::endl;

#ifdef _DEBUG
	/*if (true)
	{
		std::thread audio_thread(AudioSoftThread);
		audio_thread.join();
		}*/
#endif // _DEBUG
}


void ShutdownInterface()
{
	//PlaySound(NULL, NULL, SND_NODEFAULT);

	TTF_CloseFont(fontBig);
	TTF_CloseFont(fontMid);
	TTF_CloseFont(fontSmall);
	TTF_Quit();
	gFont = NULL;

	std::cout << "Interface: Shutdown Ok!" << std::endl;
}

void _Line(int x1, int y1, int x2, int y2)
{
	uint16_t color565 = toHiRGB(gCurrColor);

	int dx = abs(x2 - x1);
	int dy = abs(y2 - y1);

	int step = (dx >= dy) ? dx : dy;
	dx = dx / step;
	dy = dy / step;

	int x = x1;
	int y = y1;

	for (int i = 0; i <= step; i++) {
		*(((uint16_t*)drawSurface->pixels) + y * 800 + x) = color565;
		x += dx;
		y += dy;
	}
}

void DrawProgressBar(int x, int y, float l)
{
	int W = 120; y += 13;

	gCurrColor = 0x000000;
	x += 1; y += 1;
	_Line(x, y - 9, x + W + 1, y - 9);
	_Line(x, y, x + W + 1, y);
	_Line(x, y - 8, x, y);
	_Line(x + W, y - 8, x + W, y);
	_Line(x + W / 2, y - 8, x + W / 2, y);
	_Line(x + W / 4, y - 8, x + W / 4, y);
	_Line(x + W * 3 / 4, y - 8, x + W * 3 / 4, y);

	gCurrColor = 0x009F9F9F;
	x -= 1; y -= 1;
	_Line(x, y - 9, x + W + 1, y - 9);
	_Line(x, y, x + W + 1, y);
	_Line(x, y - 8, x, y);
	_Line(x + W, y - 8, x + W, y);
	_Line(x + W / 2, y - 8, x + W / 2, y);
	_Line(x + W / 4, y - 8, x + W / 4, y);
	_Line(x + W * 3 / 4, y - 8, x + W * 3 / 4, y);

	W -= 2;
	DrawRectangle(x + 2, y - 5, (int)(W * l / 2.f), 4, 0x003FAF3F);
	DrawRectangle(x + 1, y - 6, (int)(W * l / 2.f), 4, 0x003FAF3F);
}


void DrawRectangle(int x, int y, int w, int h, int color)
{
	uint16_t* back_buffer = (uint16_t*)drawSurface->pixels;

	uint16_t color565 = toHiRGB(color);
	for (int i = y; i < y + h -1; i++) {
		for (int j = x; j < x + w -1; j++) {
			back_buffer[j + (i * 800)] = color565;
		}
	}
}


void DrawSliderBar(int x, int y, int w, float v, int slider_rgb = 0xefe4b0)
{
	if (v < 0.0f)
		v = 0.0f;
	if (v > 1.0f)
		v = 1.0f;

	int xs = (int)(x + ((w - 2) * v));

	// Draw track
	gCurrColor = 0x00000000;
	_Line(x, y - 4, x + w + 1, y - 4);
	_Line(x, y - 3, x + w + 1, y - 3);
	_Line(x, y - 2, x + w + 1, y - 2);
	_Line(x, y - 1, x + w + 1, y - 1);
	_Line(x, y + 0, x + w + 1, y + 0);
	_Line(x, y + 1, x + w + 1, y + 1);
	_Line(x, y + 2, x + w + 1, y + 2);
	_Line(x, y + 3, x + w + 1, y + 3);

	DrawRectangle(xs, y - 4, 6, 8, slider_rgb);
}


void DrawPicture(int x, int y, Picture& pic)
{
	if (pic.m_Data == nullptr || pic.m_Width == 0 || pic.m_Height == 0)
		return;

	for (auto i = 0U; i < pic.m_Height; i++) {
		memcpy((uint16_t*)drawSurface->pixels + x + (y + i) * 800U, pic.m_Data + (pic.m_Height - i - 1) * pic.m_Width, pic.m_Width * 2);
	}
}


/*
Draw the menu background and overlay the 'on' layer
*/
void DrawMenuBg(MenuItem& menu)
{
	SDL_Point& p = g_CursorPos;
	uint8_t cursor_id = 0;

	cursor_id = menu.GetID((p.x / 2), (p.y / 2));

	// Render the base background from the on/off states
	for (int yy = 0; yy < 300; yy++) {
		for (int xx = 0; xx < 400; xx++)
		{
			int x = xx * 2;
			int y = yy * 2;
			int on = false;
			uint8_t id2 = menu.GetID((xx), (yy));

			if (cursor_id == id2) on |= true;

			if (g_MenuState == MENU_HUNT && (id2 >= 1 && id2 <= 6)) {
				on |= (int)g_MenuItem.GetIsElementSet(id2);
			}
			else if (g_MenuState == MENU_OPTIONS && (id2 >= 1 && id2 <= 3)) {
				on |= (int)g_MenuItem.GetIsElementSet(id2);
			}

			if (id2 == 0) on = false;

			if (on) {
				*((uint16_t*)drawSurface->pixels + ((x + 0L) + (y + 0L) * 800L)) = menu.m_Image_On[(x + 0) + (600 - (y + 0) - 1) * 800];
				*((uint16_t*)drawSurface->pixels + ((x + 1L) + (y + 0L) * 800L)) = menu.m_Image_On[(x + 1) + (600 - (y + 0) - 1) * 800];
				*((uint16_t*)drawSurface->pixels + ((x + 0L) + (y + 1L) * 800L)) = menu.m_Image_On[(x + 0) + (600 - (y + 1) - 1) * 800];
				*((uint16_t*)drawSurface->pixels + ((x + 1L) + (y + 1L) * 800L)) = menu.m_Image_On[(x + 1) + (600 - (y + 1) - 1) * 800];
			}
			else {
				*((uint16_t*)drawSurface->pixels + ((x + 0L) + (y + 0L) * 800L)) = menu.m_Image[(x + 0) + (600 - (y + 0) - 1) * 800];
				*((uint16_t*)drawSurface->pixels + ((x + 1L) + (y + 0L) * 800L)) = menu.m_Image[(x + 1) + (600 - (y + 0) - 1) * 800];
				*((uint16_t*)drawSurface->pixels + ((x + 0L) + (y + 1L) * 800L)) = menu.m_Image[(x + 0) + (600 - (y + 1) - 1) * 800];
				*((uint16_t*)drawSurface->pixels + ((x + 1L) + (y + 1L) * 800L)) = menu.m_Image[(x + 1) + (600 - (y + 1) - 1) * 800];
			}
		}
	}
}


void DrawTextColor(int x, int y, const std::string& text, uint32_t color, int align = DTA_LEFT)
{
	// Nothing to do, skip out early
	if (text.length() == 0 || !gFont) {
		return;
	}

	if (align == DTA_RIGHT)
	{
		x -= GetTextW(text);
	}

	SDL_Color sdlColor = {
		uint8_t(color >> 16),
		uint8_t(color >> 8), 
		uint8_t(color >> 0),
	};
	SDL_Surface *textSurface;
	if (!(textSurface = TTF_RenderUTF8_Blended(gFont, text.c_str(), sdlColor))) {
		std::cout << "TTF_RenderUTF8_Solid(" << text.c_str() << ") failed:" << TTF_GetError() << std::endl;
	}
	else {
		SDL_Rect r = { x, y, textSurface->w, textSurface->h };
		SDL_BlitSurface(textSurface, NULL, drawSurface, &r);
		SDL_FreeSurface(textSurface);
	}
}


void DrawTextShadow(int x, int y, const std::string& text, uint32_t color, int align = DTA_LEFT)
{
	if (align == DTA_RIGHT)
	{
		x -= GetTextW(text);
	}

	DrawTextColor(x + 1, y + 1, text, 0x00000000);
	DrawTextColor(x, y, text, color);
}


void DrawURLShadow(int x, int y, const std::string& text, uint32_t color, int align = DTA_LEFT)
{
	int W = GetTextW(text);
	int H = GetTextH(text);

	if (align == DTA_RIGHT)
	{
		x -= W;
	}

	SDL_Rect rc = { x, y, W, H };

	if (IsPointInRect(g_CursorPos, rc))
	{
		color = 0x00f40a0a;
	}

	DrawTextColor(x + 1, y + 1, text, 0x000000);
	gCurrColor = 0x00000000;
	_Line(x + 1, y + H + 1, x + W + 1, y + H + 1);

	DrawTextColor(x, y, text, color);
	gCurrColor = color;
	_Line(x, y + H, x + W, y + H);
}


void InterfaceSetFont(TTF_Font* font)
{
	gFont = font;
}


void InterfaceClear(uint16_t Color)
{
	memset(drawSurface->pixels, 0, (800 * 2) * 600);
}

void InterfaceBlt()
{
	SDL_BlitSurface(drawSurface, NULL, screenSurface, NULL);
	SDL_UpdateWindowSurface(window);
}


/*
Menu screen initialization event
NOTE: This only gets called on the first tick that the menu exists, it can be used to
initialize variables and reset various things.
*/
void MenuEventStart(int32_t menu_state)
{
	g_MenuItem.ResetElementSet();

	switch (menu_state) {
	case MENU_REGISTER: {
		MenuRegistry.Offset = 0;
		MenuRegistry.Count = 0;
		MenuRegistry.AddItem("");
		char tname[128];
		for (auto i = 0U; i < 8U; i++) {
			g_Profiles[i].m_Name = "";
			g_Profiles[i].m_RegNumber = i;
			g_Profiles[i].m_Rank = RANK_BEGINNER;
			g_Profiles[i].m_Score = 0U;

			std::stringstream sn;
			sn << "trophy" << std::setfill('0') << std::setw(2) << i << ".sav";

			std::ifstream fs(sn.str());
			if (!fs.is_open()) { continue; }

			fs.read(tname, 128);
			fs.read((char*)&g_Profiles[i].m_RegNumber, 4);
			fs.read((char*)&g_Profiles[i].m_Score, 4);
			fs.read((char*)&g_Profiles[i].m_Rank, 4);

			g_Profiles[i].m_Name = tname;
		}
	} break;
	case MENU_OPTIONS: {
		g_WaitKey = -1;

		for (auto m = 0U; m < OPT_MAX; m++) {
			MenuOptions[m].Hilite = -1;
			MenuOptions[m].Selected = -1;
		}
	} break;
		/****************************************************
		Hunt License Menu */
	case MENU_HUNT:
	{
		g_HuntInfo.first = 0;
		g_HuntInfo.second = 0;

		for (auto i = 1U; i <= 3U; i++)
			g_MenuItem.SetIsElementSet(i, false);

		g_MenuItem.SetIsElementSet(g_TimeOfDay + 1, true);

		g_MenuItem.SetIsElementSet(4, false);
		g_MenuItem.SetIsElementSet(5, g_Options.TranqMode);
		g_MenuItem.SetIsElementSet(6, g_ObserverMode);

		for (auto i = 0U; i < 4U; i++)
		{
			MenuHunt[i].Offset = 0;
		}

		MenuHunt[0].Item.clear();
		for (auto i = 0U; i < g_AreaInfo.size(); i++)
		{
			MenuHunt[0].Item.push_back(std::make_pair(g_AreaInfo[i].m_Name, false));
		}
		MenuHunt[0].Count = g_AreaInfo.size();

		g_DinoList.clear();
		MenuHunt[1].Item.clear();
		for (auto i = 0U; i < g_DinoInfo.size(); i++)
		{
			DinoInfo& dino = g_DinoInfo.at(i);

			if (dino.m_AI >= 10)//&& dino.m_Rank <= g_UserProfile.Rank)
			{
				// Add to a list
				g_DinoList.push_back(i);
				MenuHunt[1].Item.push_back(std::make_pair(dino.m_Name, false));

				std::stringstream spp;

				// Load appropriate text

				spp << "huntdat/menu/txt/dino" << (dino.m_AI - 9);
				if (g_Options.OptSys)
					spp << ".txu";
				else
					spp << ".txm";
				g_DinoInfo[i].m_Description.clear();
				LoadText(g_DinoInfo[i].m_Description, spp.str());
			}
		}
		MenuHunt[1].Count = g_AreaInfo.size();

		MenuHunt[2].Item.clear();
		for (auto i = 0U; i < g_WeapInfo.size(); i++)
		{
			MenuHunt[2].Item.push_back(std::make_pair(g_WeapInfo[i].m_Name, false));
			// Load appropriate text
			std::stringstream spp;
			spp << "huntdat/menu/txt/weapon" << (i + 1) << ".txt";
			g_WeapInfo[i].m_Description.clear();
			LoadText(g_WeapInfo[i].m_Description, spp.str());
		}
		MenuHunt[2].Count = g_WeapInfo.size();

		/* TODO: In future versions of the menu, support new types of accessories/utilities
		In the _RES they can have a string that defines the command line toggle to send to
		the .REN executable, such as '-camo' for camoflauge, or whatever custom accessory
		the modded .REN allows for. */

		// Reset the states
		for (auto m = 0U; m < 4U; m++)
		{
			MenuHunt[m].Selected = -1;
			for (auto i = MenuHunt[m].Item.begin(); i != MenuHunt[m].Item.end(); i++)
			{
				i->second = false;
			}
		}

		if (!g_AreaInfo.empty())
		{
			MenuHunt[0].Selected = 0;
		}

		for (auto i = 0U; i < 3U; i++)
		{
			MenuHunt[i].Item[0].second = true;
		}

		g_ScoreDebit = CalculateDebit();
	} break;
	}
}


void LoadGameMenu(int32_t menu)
{
	std::string	mf_off = "";
	std::string	mf_on = "";
	std::string	mf_map = "";

	switch (menu) {
	case MENU_REGISTER:
	{
		mf_off = "HUNTDAT/MENU/MENUR.TGA";
		mf_on = "HUNTDAT/MENU/MENUR_ON.TGA";
		mf_map = "HUNTDAT/MENU/MR_MAP.RAW";
	} break;
	case MENU_REGISTRY_DELETE:
	{
		mf_off = "HUNTDAT/MENU/MENUD.TGA";
		mf_on = "HUNTDAT/MENU/MENUD_ON.TGA";
		mf_map = "HUNTDAT/MENU/MD_MAP.RAW";
	} break;
	case MENU_REGISTRY_WAIVER:
	{
		mf_off = "HUNTDAT/MENU/MENUL.TGA";
		mf_on = "HUNTDAT/MENU/MENUL_ON.TGA";
		mf_map = "HUNTDAT/MENU/ML_MAP.RAW";
	} break;
	case MENU_MAIN:
	{
		mf_off = "HUNTDAT/MENU/MENUM.TGA";
		mf_on = "HUNTDAT/MENU/MENUM_ON.TGA";
		mf_map = "HUNTDAT/MENU/MAIN_MAP.RAW";
	} break;
	case MENU_HUNT:
	{
		mf_off = "HUNTDAT/MENU/MENU2.TGA";
		mf_on = "HUNTDAT/MENU/MENU2_ON.TGA";
		mf_map = "HUNTDAT/MENU/M2_MAP.RAW";
	} break;
	case MENU_OPTIONS:
	{
		mf_off = "HUNTDAT/MENU/OPT_OFF.TGA";
		mf_on = "HUNTDAT//MENU/OPT_ON.TGA";
		mf_map = "HUNTDAT/MENU/OPT_MAP.RAW";
	} break;
	case MENU_CREDITS:
	{
		mf_off = "HUNTDAT/MENU/CREDITS.TGA";
		mf_on = "";
		mf_map = "";
	} break;
	case MENU_STATISTICS:
	{
		mf_off = "HUNTDAT/MENU/MENUS.TGA";
		mf_on = "";
		mf_map = "";
	} break;
	case MENU_QUIT:
	{
		mf_off = "HUNTDAT/MENU/MENUQ.TGA";
		mf_on = "HUNTDAT/MENU/MENUQ_ON.TGA";
		mf_map = "HUNTDAT/MENU/MQ_MAP.RAW";
	} break;
	default:
	{
		throw std::invalid_argument("Invalid argument: unsigned int menu.\nExpected a member of MenuStateEnum!");
	} break;
	}

	TargaImage tga;

	if (ReadTGAFile(mf_off, tga)) {
		memcpy(g_MenuItem.m_Image, tga.m_Data, (800 * 2) * 600);
	}
	else {
		throw std::runtime_error("Failed to open the file...");
		return;
	}

	if (!mf_on.empty()) {
		if (ReadTGAFile(mf_on, tga)) {
			memcpy(g_MenuItem.m_Image_On, tga.m_Data, (800 * 2) * 600);
		}
		else {
			throw std::runtime_error("Failed to open the file...");
			return;
		}
	}

	if (!mf_map.empty()) {
		std::ifstream fs(mf_map, std::ios::binary);
		if (fs.is_open())
			fs.read((char*)g_MenuItem.m_Image_Map, 400 * 300);
	}
	else {
		memset(g_MenuItem.m_Image_Map, 0, 400 * 300);
	}
}


/*
Draw the profile details that appear at the top of the main menu,
such as Profile Name, Score, Rank.
*/
void DrawMenuProfile()
{
	std::stringstream ss;

	int c = 0x00efe4b0;

	InterfaceSetFont(fontBig);
	DrawTextShadow(90, 9, g_UserProfile.Name, c);

	ss << g_UserProfile.Score;
	DrawTextShadow(592, 9, ss.str().c_str(), c, DTA_RIGHT);

#ifdef _carnivores1
	switch (g_UserProfile.Rank)
	{
	case 0: DrawTextShadow(344, 9, "Novice", c); break;
	case 1: DrawTextShadow(344, 9, "Advanced", c); break;
	case 2: DrawTextShadow(344, 9, "Expert", c); break;
	}
#endif

	InterfaceSetFont(NULL);
}


/*
Draw this profile's last hunt statistics and total hunt statistics, also calls DrawProfileMenu()
to draw the profile name and score/rank as an overlay.
*/
void DrawMenuStatistics()
{
	// X 602 - 792
	SDL_Rect rc = { 602, 70, 190, 230 };
	std::stringstream ss;
	int c = 0x00efe4b0;

	InterfaceSetFont(fontMid);
	int  ttm = (int)g_UserProfile.Total.time;
	int  ltm = (int)g_UserProfile.Last.time;

	DrawTextShadow(rc.x + 4, 78, "Path travelled  ", c);

	if (g_Options.OptSys)
		ss << std::setprecision(4) << (g_UserProfile.Last.path / 0.3f) << " ft.";
	else
		ss << std::setprecision(4) << (g_UserProfile.Last.path) << " m.";

	DrawTextShadow(rc.x + rc.w - 4, 78, ss.str(), c, DTA_RIGHT);
	ss.str(""); ss.clear();

	DrawTextShadow(rc.x + 4, 98, "Time hunted  ", c);
	ss << std::dec << (ltm / 3600) << ":"; // Hours
	ss << std::setfill('0') << std::setw(2) << ((ltm % 3600) / 60) << ":"; // Minutes
	ss << std::setfill('0') << std::setw(2) << (ltm % 60); // Seconds
	DrawTextShadow(rc.x + rc.w - 4, 98, ss.str(), c, DTA_RIGHT);
	ss.str(""); ss.clear();

	DrawTextShadow(rc.x + 4, 118, "Shots made  ", c);
	ss << g_UserProfile.Last.smade;
	DrawTextShadow(rc.x + rc.w - 4, 118, ss.str(), c, DTA_RIGHT);
	ss.str(""); ss.clear();

	int accuracy = 0;
	if (g_UserProfile.Last.success > 0)
		accuracy = ((g_UserProfile.Last.success / g_UserProfile.Last.smade) * 100);

	DrawTextShadow(rc.x + 4, 138, "Accuracy  ", c);
	ss << accuracy << "%";
	DrawTextShadow(rc.x + rc.w - 4, 138, ss.str(), c, DTA_RIGHT);
	ss.str(""); ss.clear();

	/************************** TOTAL STATS **************************/

	DrawTextShadow(rc.x + 4, 208, "Path travelled  ", c);

	if (g_UserProfile.Total.path < 1000)
	{
		if (g_Options.OptSys) ss << std::setprecision(4) << (g_UserProfile.Total.path / 0.3f) << " ft.";
		else                  ss << std::setprecision(4) << (g_UserProfile.Total.path) << " m.";
	}
	else
	{
		if (g_Options.OptSys) ss << std::setprecision(4) << (g_UserProfile.Total.path / 1667.f) << " miles.";
		else                  ss << std::setprecision(4) << (g_UserProfile.Total.path / 1000.f) << " km.";
	}

	DrawTextShadow(rc.x + rc.w - 4, 208, ss.str(), c, DTA_RIGHT);
	ss.str(""); ss.clear();

	DrawTextShadow(rc.x + 4, 228, "Time hunted  ", c);
	ss << std::dec << (ttm / 3600) << ":"; // Hours
	ss << std::setfill('0') << std::setw(2) << ((ttm % 3600) / 60) << ":"; // Minutes
	ss << std::setfill('0') << std::setw(2) << (ttm % 60); // Seconds
	DrawTextShadow(rc.x + rc.w - 4, 228, ss.str(), c, DTA_RIGHT);
	ss.str(""); ss.clear();

	DrawTextShadow(rc.x + 4, 248, "Shots made  ", c);
	ss << g_UserProfile.Total.smade;
	DrawTextShadow(rc.x + rc.w - 4, 248, ss.str(), c, DTA_RIGHT);
	ss.str(""); ss.clear();

	accuracy = 0;
	if (g_UserProfile.Total.success > 0 && g_UserProfile.Total.smade > 0)
		accuracy = static_cast<int>((float)((float)g_UserProfile.Total.success / (float)g_UserProfile.Total.smade) * 100.f);

	DrawTextShadow(rc.x + 4, 268, "Accuracy  ", c);
	ss << accuracy << "%";
	DrawTextShadow(rc.x + rc.w - 4, 268, ss.str(), c, DTA_RIGHT);
	ss.str(""); ss.clear();

	DrawTextShadow(rc.x + 4, 288, "Rank:", c);

	switch (g_UserProfile.Rank)
	{
	case 0: DrawTextShadow(rc.x + rc.w - 4, 288, "Novice", c, DTA_RIGHT); break;
	case 1: DrawTextShadow(rc.x + rc.w - 4, 288, "Advanced", c, DTA_RIGHT); break;
	case 2: DrawTextShadow(rc.x + rc.w - 4, 288, "Expert", c, DTA_RIGHT); break;
	default: DrawTextShadow(rc.x + rc.w - 4, 288, "Eldritch", c, DTA_RIGHT); break; // Easter egg/invalid rank
	}

	DrawMenuProfile();
}


/*
*/
void DrawMenuCredits()
{
	uint32_t color = 0x00efe4b0;
	std::vector<std::string> contributor_list = {
		/* Please do not remove this name */ "Rexhunter99"
	};

	InterfaceSetFont(fontSmall);

	DrawURLShadow(550, 42, g_GitHubURL, 0x7eb2ef);

	DrawTextShadow(550, 60, "Launcher Code:", color);
	
	auto i = 0U;
	for (auto contributor : contributor_list)
	{
		DrawTextShadow(650, 60 + ((i++) * 15), contributor, color);
	}
}


void DrawMenuHunt()
{
	uint32_t c = 0x00efe4b0;
	std::stringstream ss;

	if (g_HuntSelectPic != nullptr)
	{
		DrawPicture(38, 73, *g_HuntSelectPic);
	}

	InterfaceSetFont(fontBig);

	ss << (std::min(9999, std::max(0, g_UserProfile.Score)));
	DrawTextShadow(328 + 8, 38, ss.str(), c, DTA_LEFT);
	ss.str(""); ss.clear();

	ss << (std::min(9999, std::max(-999, (g_UserProfile.Score - g_ScoreDebit))));
	DrawTextShadow(472 - 8, 38, ss.str(), c, DTA_RIGHT);
	ss.str(""); ss.clear();

	InterfaceSetFont(fontMid);


	if (g_HuntInfo.first == 0) // Areas
	{
		for (auto i = 1U; i < g_AreaInfo[g_HuntInfo.second].m_Description.size(); i++)
		{
			DrawTextShadow(424, 96 + ((i-1) * 16), g_AreaInfo[g_HuntInfo.second].m_Description[i], c);
		}
	}
	else if (g_HuntInfo.first == 1) // Dinosaurs
	{
		unsigned d = g_DinoList[g_HuntInfo.second];
		if (!(g_DinoInfo[d].m_Price >= 1000 && g_UserProfile.Score < 1000))
		{
			for (auto i = 0U; i < g_DinoInfo[d].m_Description.size(); i++)
			{
				DrawTextShadow(424, 96 + ((i) * 16), g_DinoInfo[d].m_Description[i], c);
			}

			DrawTextShadow(424, 210 + (0 * 16), "Sight:", c);
			DrawTextShadow(424, 210 + (1 * 16), "Hearing:", c);
			DrawTextShadow(424, 210 + (2 * 16), "Scents:", c);

			DrawProgressBar(424 + 80, 210 + (0 * 16), std::min(1.0f, std::max(0.0f, g_DinoInfo[d].m_LookK)) * 2.f);
			DrawProgressBar(424 + 80, 210 + (1 * 16), std::min(1.0f, std::max(0.0f, g_DinoInfo[d].m_HearK)) * 2.f);
			DrawProgressBar(424 + 80, 210 + (2 * 16), std::min(1.0f, std::max(0.0f, g_DinoInfo[d].m_SmellK)) * 2.f);
		}
	}
	else if (g_HuntInfo.first == 2) // Weapons
	{
		for (auto i = 0U; i < g_WeapInfo[g_HuntInfo.second].m_Description.size(); i++)
		{
			DrawTextShadow(424, 96 + ((i) * 16), g_WeapInfo[g_HuntInfo.second].m_Description[i], c);
		}

		DrawTextShadow(424, 210 + (0 * 16), "Power:", c);
		DrawTextShadow(424, 210 + (1 * 16), "Accuracy:", c);
		DrawTextShadow(424, 210 + (2 * 16), "Volume:", c);

		DrawProgressBar(424 + 80, 210 + (0 * 16), std::min(2.0f, std::max(0.0f, g_WeapInfo[g_HuntInfo.second].m_Power)));
		DrawProgressBar(424 + 80, 210 + (1 * 16), std::min(2.0f, std::max(0.0f, g_WeapInfo[g_HuntInfo.second].m_Prec)));
		DrawProgressBar(424 + 80, 210 + (2 * 16), std::min(2.0f, std::max(0.0f, g_WeapInfo[g_HuntInfo.second].m_Loud)));
	}
	else if (g_HuntInfo.first == 3) // Accessories
	{
		for (auto i = 0U; i < g_UtilInfo[g_HuntInfo.second].m_Description.size(); i++)
		{
			DrawTextShadow(424, 96 + ((i) * 16), g_UtilInfo[g_HuntInfo.second].m_Description[i], c);
		}
	}

	InterfaceSetFont(fontSmall);

	int32_t score = g_UserProfile.Score - g_ScoreDebit;

	unsigned list_max = std::min(g_AreaInfo.size(), (size_t)10);

	for (unsigned ii = MenuHunt[0].Offset; ii < MenuHunt[0].Offset + list_max; ii++) {
		int i = ii - MenuHunt[0].Offset;
		c = 0x70B0B0;

		std::stringstream sc;
		sc << g_AreaInfo[ii].m_Price;

		if (score < g_AreaInfo[ii].m_Price)
			c = 0x707070;

		if (MenuHunt[0].Selected == ii)
		{
			c = 0x00ffff0a;
		}

		DrawTextShadow(MenuHunt[0].Rect.x + 4, MenuHunt[0].Rect.y + (16 * i), g_AreaInfo[ii].m_Name, c);
		DrawTextShadow(MenuHunt[0].Rect.x + MenuHunt[0].Rect.w - 4, MenuHunt[0].Rect.y + (16 * i), sc.str(), c, DTA_RIGHT);
	}

	for (unsigned ii = MenuHunt[1].Offset; ii < MenuHunt[1].Offset + MenuHunt[1].Item.size(); ii++)
	{
		int i = ii - MenuHunt[1].Offset;
		uint32_t c = 0x70B0B0;
		try {
			DinoInfo& di = g_DinoInfo.at(g_DinoList[ii]);
			std::string s = di.m_Name;
			std::stringstream sc;

			sc << di.m_Price;

			// (IceAge)
			// If the dinosaur is worth more than 1000 score and the player has less than, we want to hide what it is
			if (di.m_Price >= 1000 && g_UserProfile.Score < 1000)
			{
				s = "???";
				c = 0x707070;
			}

			if (score < di.m_Price)
			{
				c = 0x707070;
			}

			if (MenuHunt[1].Item[ii].second)
			{
				c = 0x00ffff0a;
			}

			DrawTextShadow(MenuHunt[1].Rect.x + 4, MenuHunt[1].Rect.y + (16 * i), s, c);
			DrawTextShadow(MenuHunt[1].Rect.x + MenuHunt[1].Rect.w - 4, MenuHunt[1].Rect.y + (16 * i), sc.str(), c, DTA_RIGHT);
		}
		catch (std::out_of_range& e) {
			throw std::runtime_error(e.what());
		}
	}

	for (unsigned ii = MenuHunt[2].Offset; ii < MenuHunt[2].Offset + MenuHunt[2].Item.size(); ii++)
	{
		uint32_t c = 0x70B0B0;
		int i = ii - MenuHunt[2].Offset;
		WeapInfo& wi = g_WeapInfo[ii];
		std::stringstream sc;
		sc << wi.m_Price;
		
		if (score < wi.m_Price)
		{
			c = 0x707070;
		}

		if (MenuHunt[2].Item[ii].second)
		{
			c = 0xffff0a;
		}

		DrawTextShadow(MenuHunt[2].Rect.x + 4, MenuHunt[2].Rect.y + (16 * i), g_WeapInfo[ii].m_Name, c);
		DrawTextShadow(MenuHunt[2].Rect.x + MenuHunt[2].Rect.w - 4, MenuHunt[2].Rect.y + (16 * i), sc.str(), c, DTA_RIGHT);
	}

	for (unsigned ii = MenuHunt[3].Offset; ii < MenuHunt[3].Offset + MenuHunt[3].Item.size(); ii++)
	{
		uint32_t c = 0x70B0B0;
		int i = ii - MenuHunt[3].Offset;

		if (MenuHunt[3].Item[ii].second)
		{
			c = 0xffff0a;
		}

		DrawTextShadow(MenuHunt[3].Rect.x + 4, MenuHunt[3].Rect.y + (16 * i), MenuHunt[3].Item[ii].first, c);
	}
}


/*
Draw the save file 'registry' menu
*/
void DrawMenuRegistry()
{
	SDL_Point& p = g_CursorPos;
	std::stringstream ss;
	uint32_t color = 0xefe4b0;

	if (g_MenuState == MENU_REGISTRY_DELETE)
	{
		InterfaceSetFont(fontMid);
		DrawTextShadow(290, 370, "Do you want to delete player", 0x00B08030);
		ss << "\'" << g_Profiles[g_ProfileIndex].m_Name << "\' ?";
		DrawTextShadow(300, 394, ss.str(), 0x00B08030);
		InterfaceSetFont(0);
	}
	else {
		InterfaceSetFont(fontSmall);

		if ((Timer::GetTime() % 800) > 300)
			ss << g_TypingBuffer << "_";
		else
			ss << g_TypingBuffer;

		DrawTextShadow(315, 326, ss.str(), color);

		g_HiliteProfileIndex = g_ProfileIndex;

		// 320, 370
		for (auto i = 0U; i < 7U; i++)
		{
			color = 0x70B0B0; // Base colour

			if (i == g_HiliteProfileIndex)
				color = 0xffaa0a;

			if (p.x >= 308 && p.y >= (368 + (16 * i)) && p.x <= 408 && p.y <= (368 + (16 * i) + 16)) {
				g_HiliteProfileIndex = i; //temporary, move to another function for user input
#ifdef _DEBUG
				color = 0x2aff2a;
#endif
			}

			std::string tname = g_Profiles[i].m_Name;

			if (g_Profiles[i].m_Name.empty()) {
				tname = "...";
			}

			DrawTextShadow(320, 370 + (16 * i), tname, color);
		}

		InterfaceSetFont(0);
	}
}


/*
Keyboard and Mouse handling for menus
NOTE: Could move these to individual functions if a state machine is confusing
*/
void MenuEventInput(int32_t menu)
{
	if (!g_KeyboardState) return;

	uint8_t id = g_MenuItem.GetID(g_CursorPos.x / 2, g_CursorPos.y / 2);

	if (g_MouseState & SDL_BUTTON_LMASK) {
		//PlaySound("huntdat/soundfx/menugo.wav", NULL, SND_NODEFAULT | SND_ASYNC | SND_FILENAME);
		//AddVoice(g_MenuSound_Go.m_Length, g_MenuSound_Go.m_Data);
	}

	if (menu == MENU_CREDITS)
	{
		if (g_KeyboardState[SDL_SCANCODE_RETURN]) { // XXX TODO also check RETURN2?
			g_MouseState |= SDL_BUTTON_LMASK;
		}

		if (g_KeyboardState[SDL_SCANCODE_SPACE]) {
			g_MouseState |= SDL_BUTTON_LMASK;
		}

		if (g_MouseState & SDL_BUTTON_LMASK) {
			WaitForMouseRelease();
			SDL_Rect rc = { 550, 42, 50 + GetTextW(g_GitHubURL), 14 };

			if (IsPointInRect(g_CursorPos, rc))
			{
#ifdef _WIN32				
				ShellExecute(0, 0, TEXT(g_GitHubURL), 0, 0, SW_SHOW);
#endif
			}
			else
			{
				//PlaySound("huntdat/soundfx/menugo.wav", NULL, SND_ASYNC | SND_FILENAME);
				ChangeMenuState(MENU_MAIN);
			}
		}
	}
	else if (menu == MENU_REGISTER)
	{
		if (g_KeyboardState[SDL_SCANCODE_RETURN]) {
			g_MouseState |= SDL_BUTTON_LMASK;
			id = 1;
		}

		if (g_KeyboardState[SDL_SCANCODE_DELETE]) {
			g_MouseState |= SDL_BUTTON_LMASK;
			id = 2;
		}

		if (g_MouseState & SDL_BUTTON_LMASK) {
			if (id == 1) {
				//PlaySound("huntdat/soundfx/menugo.wav", NULL, SND_ASYNC | SND_FILENAME);

				if (g_Profiles[g_ProfileIndex].m_Name.empty()) {
					if (!g_TypingBuffer.empty())
					{
						g_UserProfile.New(g_TypingBuffer);
						g_Options.Default();
						TrophySave(g_UserProfile);
						WaitForMouseRelease();
						ChangeMenuState(MENU_REGISTRY_WAIVER);
					}
					else
					{
#ifdef _WIN32
						MessageBox(NULL, "You need to enter a name!", "Try again!", MB_OK | MB_ICONINFORMATION);
#endif
					}
				}
				else {
					TrophyLoad(g_UserProfile, g_ProfileIndex);
					WaitForMouseRelease();
					ChangeMenuState(MENU_MAIN);
				}
			}
			else if (id == 2) {
				// Delete the selected 'save'
				WaitForMouseRelease();
				//PlaySound("huntdat/soundfx/menugo.wav", NULL, SND_ASYNC | SND_FILENAME);
				ChangeMenuState(MENU_REGISTRY_DELETE);
			}
			else {
				WaitForMouseRelease();
				//PlaySound("huntdat/soundfx/menumov.wav", NULL, SND_ASYNC | SND_FILENAME);
				g_ProfileIndex = g_HiliteProfileIndex;
			}
		}
	}
	/*
	*/
	else if (menu == MENU_REGISTRY_DELETE)
	{
		if (g_MouseState & SDL_BUTTON_LMASK) {
			if (id == 1)
			{
				WaitForMouseRelease();
				TrophyDelete(g_ProfileIndex); // Delete the last clicked profile
				//PlaySound("huntdat/soundfx/menugo.wav", NULL, SND_ASYNC | SND_FILENAME);
				ChangeMenuState(MENU_REGISTER);
			}
			else if (id == 2)
			{
				WaitForMouseRelease();
				//PlaySound("huntdat/soundfx/menugo.wav", NULL, SND_ASYNC | SND_FILENAME);
				ChangeMenuState(MENU_REGISTER);
			}
		}
	}
	/*
	*/
	else if (menu == MENU_REGISTRY_WAIVER)
	{
		if (g_MouseState & SDL_BUTTON_LMASK) {
			if (id == 1)
			{
				WaitForMouseRelease();
				//PlaySound("huntdat/soundfx/menugo.wav", NULL, SND_ASYNC | SND_FILENAME);
				ChangeMenuState(MENU_MAIN);
			}
			else if (id == 2)
			{
				WaitForMouseRelease();
				TrophyDelete(g_ProfileIndex);
				//PlaySound("huntdat/soundfx/menugo.wav", NULL, SND_ASYNC | SND_FILENAME);
				ChangeMenuState(MENU_REGISTER);
			}
		}
	}
	/**************************************
	* Options Menu
	*/
	else if (menu == MENU_OPTIONS) {
		if (g_WaitKey >= 0)
		{
			AcceptNewKey();
		}
		else if (id == 4)
		{
			if (g_MouseState & SDL_BUTTON_LMASK)
			{
				WaitForMouseRelease();
				TrophySave(g_UserProfile); // Save all the settings
				//PlaySound("huntdat/soundfx/menugo.wav", NULL, SND_ASYNC | SND_FILENAME);
				ChangeMenuState(MENU_MAIN);
			}
		}
		else
		{
			for (int m = 0; m < OPT_MAX; m++)
			{
				MenuSet& mo = MenuOptions[m];
				if (IsPointInRect(g_CursorPos, mo.Rect))
				{
					int yd = g_CursorPos.y - mo.y0;
					g_MenuItem.SetIsElementSet(m + 1, true);

					// We use a little division to find out what item is hovered over
					// the original games used for loops to do this... crazy
					if (yd > 0)
					{
						mo.Hilite = yd / 22; // 22 is the height of each item, we should store this in a variable
					}
					else mo.Hilite = -1;

					if (g_MouseState & SDL_BUTTON_LMASK) // Left Click
					{
						if (m == OPT_GAME) {
							MenuSet& menu = MenuOptions[OPT_GAME];
							//int w = (menu.Rect.right - menu.Rect.left) - menu.Padding;
							int x1 = menu.Rect.x + menu.Rect.w - menu.Padding;
							int tbw = (menu.Rect.w / 2) - menu.Padding;
							float v = (float)(g_CursorPos.x - (x1 - tbw)) / (float)tbw;

							mo.Selected = mo.Hilite;

							if (mo.Hilite == 0)
							{
								g_Options.Aggression = (int)(v * 255.f);
							}
							else if (mo.Hilite == 1)
							{
								g_Options.Density = (int)(v * 255.f);
							}
							else if (mo.Hilite == 2)
							{
								g_Options.Sensitivity = (int)(v * 255.f);
							}
							else if (mo.Hilite == 3)
							{
								g_Options.ViewRange = (int)(v * 255.f);
							}
							else if (mo.Hilite == 4) // Metric or Imperial(US)
							{
								WaitForMouseRelease();
								g_Options.OptSys = !g_Options.OptSys;
							}
							else if (mo.Hilite == 5)
							{
								WaitForMouseRelease();
								g_Options.SoundAPI++;
								if (g_Options.SoundAPI == 4)
									g_Options.SoundAPI = 0;
							}
						}
						else if (m == OPT_KEYBINDINGS) { // Left Click
							MenuSet& menu = MenuOptions[OPT_KEYBINDINGS];
							//int w = (menu.Rect.right - menu.Rect.left) - menu.Padding;
							int x1 = menu.Rect.x + menu.Rect.w - menu.Padding;
							int tbw = (menu.Rect.w / 2) - menu.Padding;
							float v = (float)(g_CursorPos.x - (x1 - tbw)) / (float)tbw;

							mo.Selected = mo.Hilite;

							if (static_cast<size_t>(mo.Hilite) < MenuOptions[OPT_KEYBINDINGS].Item.size() - 2)
							{
								WaitForMouseRelease();
								g_WaitKey = mo.Hilite;
							}
							else if (static_cast<int>(mo.Hilite) == MenuOptions[OPT_KEYBINDINGS].Item.size() - 2) // Mouse Y-Axis Inverted
							{
								WaitForMouseRelease();
								g_Options.MouseInvert = !g_Options.MouseInvert;
							}
							else if (static_cast<int>(mo.Hilite) == MenuOptions[OPT_KEYBINDINGS].Item.size() - 1) // Mouse Sensitivty Slider
							{
								g_Options.MouseSensitivity = (int)(v * 255.f);
							}
						}
						else if (m == OPT_VIDEO) { // Left Click
							MenuSet& menu = MenuOptions[OPT_VIDEO];
							//int w = (menu.Rect.right - menu.Rect.left) - menu.Padding;
							int x1 = menu.Rect.x + menu.Rect.w - menu.Padding;
							int tbw = (menu.Rect.w / 2) - menu.Padding;
							float v = (float)(g_CursorPos.x - (x1 - tbw)) / (float)tbw;

							mo.Selected = mo.Hilite;
							if (mo.Hilite == 0)
							{
								WaitForMouseRelease();
								g_Options.RenderAPI++;
								if (g_Options.RenderAPI == 3)
									g_Options.RenderAPI = 0;
							}
							if (mo.Hilite == 1) // Resolution
							{
								WaitForMouseRelease();
								g_Options.Resolution++;
								if (g_Options.Resolution == RES_MAX)
									g_Options.Resolution = 0;
							}
							else if (mo.Hilite == 2) // Shadows
							{
								WaitForMouseRelease();
								g_Options.Shadows = !g_Options.Shadows;
							}
							else if (mo.Hilite == 3) // Fog
							{
								WaitForMouseRelease();
								g_Options.Fog = !g_Options.Fog;
							}
							else if (mo.Hilite == 4) // Textures
							{
								WaitForMouseRelease();
								g_Options.Textures++;
								if (g_Options.Textures == 3)
									g_Options.Textures = 0;
							}
							else if (mo.Hilite == 5) // Colorkey
							{
								WaitForMouseRelease();
								g_Options.AlphaColorKey = !g_Options.AlphaColorKey;
							}
							else if (mo.Hilite == 6) // Brightness
							{
								g_Options.Brightness = (int)(v * 255.f);
							}
						}
					}
				}
				else
				{
					g_MenuItem.SetIsElementSet(m + 1, false);
					mo.Hilite = -1;
				}
			}
		}
	}
	else if (menu == MENU_HUNT) {
		if (g_KeyboardState[SDL_SCANCODE_ESCAPE])
		{
			ChangeMenuState(MENU_QUIT);
		}

		// Mouse hover
		if (IsPointInRect(g_CursorPos, MenuHunt[0].Rect))
		{
			WaitForMouseRelease();
			int32_t scorea = 0;

			if (MenuHunt[0].Selected != -1)
			{
				scorea = g_AreaInfo[MenuHunt[0].Selected].m_Price;
			}

			int32_t score = (g_UserProfile.Score - g_ScoreDebit) + scorea;
			int yd = g_CursorPos.y - MenuHunt[0].Rect.y;

			unsigned index = yd / 16;

			if (index < g_AreaInfo.size())
			{
				if (g_AreaInfo[index].m_Price >= 1000 && g_UserProfile.Score < 1000)
					g_HuntSelectPic = &g_AreaInfo[index].m_ThumbnailHidden;
				else
					g_HuntSelectPic = &g_AreaInfo[index].m_Thumbnail;
				g_HuntInfo.first = 0; // Areas
				g_HuntInfo.second = index;

				if ((g_MouseState & SDL_BUTTON_LMASK) && score >= g_AreaInfo[index].m_Price)
				{
					WaitForMouseRelease();
					//PlaySound("huntdat/soundfx/menumov.wav", NULL, SND_ASYNC | SND_FILENAME);

					// Reset the states
					for (auto i = MenuHunt[0].Item.begin(); i != MenuHunt[0].Item.end(); i++)
					{
						i->second = false;
					}

					MenuHunt[0].Item[index].second = true;
					MenuHunt[0].Selected = index;

					g_ScoreDebit = CalculateDebit();
				}
			}
		}
		else if (IsPointInRect(g_CursorPos, MenuHunt[1].Rect))
		{
			int32_t score = g_UserProfile.Score - g_ScoreDebit;
			int yd = g_CursorPos.y - MenuHunt[1].Rect.y;

			unsigned index = yd / 16;

			if (index < MenuHunt[1].Item.size())
			{
				if (g_DinoInfo[g_DinoList[index]].m_Price >= 1000 && g_UserProfile.Score < 1000)
					g_HuntSelectPic = &g_DinoInfo[g_DinoList[index]].m_ThumbnailHidden;
				else
					g_HuntSelectPic = &g_DinoInfo[g_DinoList[index]].m_Thumbnail;
				g_HuntInfo.first = 1; // Dinos
				g_HuntInfo.second = index;

				if (g_MouseState & SDL_BUTTON_LMASK)
				{
					WaitForMouseRelease();
					//PlaySound("huntdat/soundfx/menumov.wav", NULL, SND_ASYNC | SND_FILENAME);

					if (score >= g_DinoInfo[g_DinoList[index]].m_Price && !MenuHunt[1].Item[index].second)
					{
						MenuHunt[1].Item[index].second = true;
					}
					else
					{
						MenuHunt[1].Item[index].second = false;
					}

					g_ScoreDebit = CalculateDebit();
				}
			}
		}
		else if (IsPointInRect(g_CursorPos, MenuHunt[2].Rect))
		{
			int32_t score = g_UserProfile.Score - g_ScoreDebit;
			int yd = g_CursorPos.y - MenuHunt[2].Rect.y;

			unsigned index = yd / 16;

			if (index < MenuHunt[2].Item.size())
			{
				if (g_WeapInfo[index].m_Price >= 1000 && g_UserProfile.Score < 1000)
					g_HuntSelectPic = &g_WeapInfo[index].m_ThumbnailHidden;
				else
					g_HuntSelectPic = &g_WeapInfo[index].m_Thumbnail;
				g_HuntInfo.first = 2; // Weapons
				g_HuntInfo.second = index;

				if (g_MouseState & SDL_BUTTON_LMASK)
				{
					WaitForMouseRelease();
					//PlaySound("huntdat/soundfx/menumov.wav", NULL, SND_ASYNC | SND_FILENAME);

					if (score >= g_WeapInfo[index].m_Price && !MenuHunt[2].Item[index].second)
					{
						MenuHunt[2].Item[index].second = true;
					}
					else
					{
						MenuHunt[2].Item[index].second = false;
					}

					g_ScoreDebit = CalculateDebit();
				}
			}
		}
		else if (IsPointInRect(g_CursorPos, MenuHunt[3].Rect))
		{
			//int32_t score = g_UserProfile.Score - g_ScoreDebit;
			int yd = g_CursorPos.y - MenuHunt[3].Rect.y;
			unsigned index = yd / 16;

			if (index < MenuHunt[3].Item.size())
			{
				g_HuntSelectPic = &g_UtilInfo[index].m_Thumbnail;
				g_HuntInfo.first = 3; // Accessories
				g_HuntInfo.second = index;

				if (g_MouseState & SDL_BUTTON_LMASK)
				{
					WaitForMouseRelease();
					//PlaySound("huntdat/soundfx/menumov.wav", NULL, SND_ASYNC | SND_FILENAME);

					MenuHunt[3].Item[index].second = !MenuHunt[3].Item[index].second;
				}
			}
		}

		// Left Mouse Click
		if (g_MouseState & SDL_BUTTON_LMASK)
		{
			if (id >= 1 && id <= 6)
			{
				WaitForMouseRelease();
				//PlaySound("huntdat/soundfx/menugo.wav", NULL, SND_ASYNC | SND_FILENAME);
				bool b = g_MenuItem.ToggleIsElementSet(id);

				if (id >= 1 && id <= 3)
				{
					g_TimeOfDay = id - 1;

					for (int i = 1; i <= 3; i++)
					{
						if (i != id)
							g_MenuItem.SetIsElementSet(i, false);
					}
				}

				if (id == 4)
					b = false; // Unused
				if (id == 5)
					g_Options.TranqMode = b;
				if (id == 6)
					g_ObserverMode = b;
			}
			else if (id == 7) // Back
			{
				WaitForMouseRelease();
				//PlaySound("huntdat/soundfx/menugo.wav", NULL, SND_ASYNC | SND_FILENAME);
				ChangeMenuState(MENU_MAIN);
			}
			else if (id == 8) // Hunt/Next
			{
				// Launch the game
				WaitForMouseRelease();
				//PlaySound("huntdat/soundfx/menugo.wav", NULL, SND_ASYNC | SND_FILENAME);
				if (MenuHunt[0].Selected == -1)
				{
					// -- Don't launch
					return;
				}

				int din = 0;
				int wep = 0;

				for (unsigned i = 0; i < g_DinoList.size(); i++)
				{
					if (MenuHunt[1].Item[i].second)
						din |= 1 << i;
				}

				for (unsigned i = 0; i < g_WeapInfo.size(); i++)
				{
					if (MenuHunt[2].Item[i].second)
						wep |= 1 << i;
				}

				std::vector<std::string> params;

				params.push_back(std::string("reg=") + std::to_string(g_UserProfile.RegNumber));
				params.push_back(std::string("prj=huntdat/areas/") + g_AreaInfo[MenuHunt[0].Selected].m_ProjectName);
				params.push_back(std::string("din=") + std::to_string(din));
				params.push_back(std::string("wep=") + std::to_string(wep));
				params.push_back(std::string("dtm=") + std::to_string(g_TimeOfDay));

#ifdef _iceage
				if (MenuHunt[3].Item[4].second)
					params.push_back(g_UtilInfo[4].m_Command);
#endif //_iceage
				if (MenuHunt[3].Item[3].second)
					params.push_back(g_UtilInfo[3].m_Command);
				if (g_Options.RadarMode)
					params.push_back("-radar");
				if (g_Options.TranqMode)
					params.push_back("-tranq");
				if (g_ObserverMode)
					params.push_back("-observ");

#ifdef _DEBUG
				params.push_back("-debug");
#endif //_DEBUG
				std::stringstream renderer("");
				renderer << g_RendererFile[g_Options.RenderAPI] << ".ren";

				if (wep && din)
				{
					TrophySave(g_UserProfile); // Save all the settings
					LaunchProcess(renderer.str(), params);
					TrophyLoad(g_UserProfile, g_UserProfile.RegNumber); // Load the changes
				}
				else
				{
					ShowErrorMessage("You need to select at least:\r\n 1x Creature\r\n 1x Weapon");
				}
			}
		}
	}
	else if (menu == MENU_MAIN)
	{
		if (g_KeyboardState[SDL_SCANCODE_ESCAPE]) {
			ChangeMenuState(MENU_QUIT);
		}
		else if (g_MouseState & SDL_BUTTON_LMASK) {
			WaitForMouseRelease();
			if (id >= 1 && id <= 6) {
				WaitForMouseRelease();
				//PlaySound("huntdat/soundfx/menugo.wav", NULL, SND_ASYNC | SND_FILENAME);
				if (id == 1) { ChangeMenuState(MENU_HUNT); }
				else if (id == 2) { ChangeMenuState(MENU_OPTIONS); }
				else if (id == 3) {
					std::vector<std::string> params;

					params.push_back(std::string("reg=") + std::to_string(g_UserProfile.RegNumber));
					params.push_back("prj=huntdat/areas/trophy");
					params.push_back("dtm=1");
#ifdef _DEBUG
					params.push_back("-debug");
#endif //_DEBUG
					std::stringstream renderer("");
					renderer << g_RendererFile[g_Options.RenderAPI] << ".ren";

					TrophySave(g_UserProfile); // Save the changes
					LaunchProcess(renderer.str(), params);
					TrophyLoad(g_UserProfile, g_UserProfile.RegNumber); // Load the changes
				}
				else if (id == 4) { ChangeMenuState(MENU_CREDITS); }
				else if (id == 5) { ChangeMenuState(MENU_QUIT); }
				else if (id == 6) { ChangeMenuState(MENU_STATISTICS); }
			}
		}
	}
	else if (menu == MENU_STATISTICS)
	{
		if (g_KeyboardState[SDL_SCANCODE_ESCAPE]) {
			ChangeMenuState(MENU_QUIT);
		}

		if (g_MouseState & SDL_BUTTON_LMASK) {
			WaitForMouseRelease();
			//PlaySound("huntdat/soundfx/menugo.wav", NULL, SND_ASYNC | SND_FILENAME);
			ChangeMenuState(MENU_MAIN);
		}
	}
	else if (menu == MENU_QUIT)
	{
		if (g_MouseState & SDL_BUTTON_LMASK) {
			WaitForMouseRelease();
			//if (id == 1 || id == 2) PlaySound("huntdat/soundfx/menugo.wav", NULL, SND_ASYNC | SND_FILENAME);

			if (id == 1)      g_QuitRequested = true;
			else if (id == 2) ChangeMenuState(MENU_MAIN);
		}
	}
}


void DrawMenuOptions()
{
	const int label_c = 0x007696b5; // From Carnivores: Ice Age (JPEG image)
	const int value_c = 0x00abb4a7; // From Carnivores: Ice Age (JPEG image)
	const int off_c = 0xefe4b0;
	const int on_c = 0x1eef1e;
	int c = off_c;

	InterfaceSetFont(fontMid); // XXX!!!!!!  g_FontOptions

	// Game options
	for (auto i = 0U; i < MenuOptions[OPT_GAME].Item.size(); i++) {
		MenuSet& menu = MenuOptions[OPT_GAME];
		int x0 = menu.Rect.x + menu.Padding;// .x0;
		int x1 = menu.Rect.x + menu.Rect.w - menu.Padding;// .x0;
		int y0 = menu.y0 + (22 * i);
		int tbw = (menu.Rect.w / 2) - menu.Padding;

		if (menu.Hilite == i) c = on_c;
		else c = label_c;

		DrawTextShadow(x0, y0, menu.Item[i].first, c);

		if (i == 0) DrawSliderBar(x1 - tbw, y0 + 12, tbw, (float)g_Options.Aggression / 255.0f, label_c);
		if (i == 1) DrawSliderBar(x1 - tbw, y0 + 12, tbw, (float)g_Options.Density / 255.0f, label_c);
		if (i == 2) DrawSliderBar(x1 - tbw, y0 + 12, tbw, (float)g_Options.Sensitivity / 255.0f, label_c);
		if (i == 3) DrawSliderBar(x1 - tbw, y0 + 12, tbw, (float)g_Options.ViewRange / 255.0f, label_c);
		if (i == 4) DrawTextShadow(x1, y0, st_UnitText[g_Options.OptSys], value_c, DTA_RIGHT);
		if (i == 5) DrawTextShadow(x1, y0, st_AudText[g_Options.SoundAPI], value_c, DTA_RIGHT);
	}

	// Control key bindings
	for (auto i = 0U; i < MenuOptions[OPT_KEYBINDINGS].Item.size(); i++) {
		MenuSet& menu = MenuOptions[OPT_KEYBINDINGS];
		int x0 = menu.Rect.x + menu.Padding;// .x0;
		int x1 = menu.Rect.x + menu.Rect.w - menu.Padding;// .x0;
		int y0 = menu.y0 + (22 * i);
		int tbw = (menu.Rect.w / 2) - menu.Padding;

		std::stringstream ss;

		ss << g_KeyNames[MapVKKey(*((int32_t*)&g_Options.KeyMap + i))];

		if (menu.Hilite == i) c = on_c;
		else c = label_c;

		DrawTextShadow(x0, y0, menu.Item[i].first, c);

		if (i < MenuOptions[OPT_KEYBINDINGS].Item.size() - 2)
		{
			if (g_WaitKey == i) DrawTextShadow(x1, y0, "<?>", value_c, DTA_RIGHT);
			else                DrawTextShadow(x1, y0, ss.str(), value_c, DTA_RIGHT);
		}
		else if (i == MenuOptions[OPT_KEYBINDINGS].Item.size() - 2)
			DrawTextShadow(x1, y0, st_BoolText[(int)g_Options.MouseInvert], value_c, DTA_RIGHT);
		else if (i == MenuOptions[OPT_KEYBINDINGS].Item.size() - 1)
			DrawSliderBar(x1 - tbw, y0 + 12, tbw, (float)g_Options.MouseSensitivity / 255.0f, label_c);
	}

	// Video/Graphics options
	for (auto i = 0U; i < MenuOptions[OPT_VIDEO].Item.size(); i++) {
		MenuSet& menu = MenuOptions[OPT_VIDEO];
		int x0 = menu.Rect.x + menu.Padding;// x0;
		int x1 = menu.Rect.x + menu.Rect.w - menu.Padding;// x0;
		int y0 = menu.y0 + (22 * i);
		int tbw = (menu.Rect.w / 2) - menu.Padding;

		if (menu.Hilite == i) c = on_c;
		else c = label_c;

		DrawTextShadow(x0, y0, menu.Item[i].first, c);

		if (i == 0) DrawTextShadow(x1, y0, st_RenText[g_Options.RenderAPI], value_c, DTA_RIGHT);
		else if (i == 1) {
			switch (g_Options.Resolution) {
			case 0: DrawTextShadow(x1, y0, "320 x 240", value_c, DTA_RIGHT); break;
			case 1: DrawTextShadow(x1, y0, "400 x 300", value_c, DTA_RIGHT); break;
			case 2: DrawTextShadow(x1, y0, "512 x 384", value_c, DTA_RIGHT); break;
			case 3: DrawTextShadow(x1, y0, "640 x 480", value_c, DTA_RIGHT); break;
			case 4: DrawTextShadow(x1, y0, "800 x 600", value_c, DTA_RIGHT); break;
			case 5: DrawTextShadow(x1, y0, "1024 x 768", value_c, DTA_RIGHT); break;
			case 6: DrawTextShadow(x1, y0, "1280 x 1024", value_c, DTA_RIGHT); break;
			case 7: DrawTextShadow(x1, y0, "1600 x 1200", value_c, DTA_RIGHT); break;
			}
		}
		else if (i == 2) DrawTextShadow(x1, y0, st_BoolText[g_Options.Shadows], value_c, DTA_RIGHT);
		else if (i == 3) DrawTextShadow(x1, y0, st_BoolText[g_Options.Fog], value_c, DTA_RIGHT);
		else if (i == 4) DrawTextShadow(x1, y0, st_TextureText[g_Options.Textures], value_c, DTA_RIGHT);
		else if (i == 5) DrawTextShadow(x1, y0, st_AlphaKeyText[g_Options.AlphaColorKey], value_c, DTA_RIGHT);
		else if (i == 6) DrawSliderBar(x1 - tbw, y0 + 12, tbw, (float)g_Options.Brightness / 255.0f, label_c);
	}

	InterfaceSetFont(NULL);
}


// TODO: Move these to Hunt.h
#define FPS_TARGET 60L
#define FRAME_TIME_DELTA (1000L / FPS_TARGET) // The time in milliseconds a frame takes to process to achieve FPS_TARGET

int64_t g_PrevFrameTime = 0;
#ifdef _DEBUG
int32_t g_Frames = 0;
int32_t g_FramesPerSecond = 0;
int64_t g_PrevFrameCountTime = 0;
#endif

/*
Perform per-frame/tick update of the menus
*/
void ProcessMenu()
{
	g_MouseState = SDL_GetMouseState(&g_CursorPos.x, &g_CursorPos.y);

	// Get the keyboard state
	if (true /*GetActiveWindow() == hwndMain*/) { // XXX TODO renable after debugging
		g_KeyboardState = SDL_GetKeyboardState(NULL);
	}
	else {
		g_KeyboardState = NULL;
	}

	// Trigger the Start() event of the menu if applicable
	if (g_PrevMenuState != g_MenuState) {
		MenuEventStart(g_MenuState);
		g_PrevMenuState = g_MenuState;
	}

	// Handle input from mouse/keyboard
	MenuEventInput(g_MenuState);

	// Clear the video buffer and then draw the menu background
	InterfaceClear(HIRGB(0, 0, 10));
	DrawMenuBg(g_MenuItem);

	// Draw menus
	switch (g_MenuState)
	{
	case MENU_REGISTRY_DELETE: DrawMenuRegistry(); break;
	case MENU_REGISTER: DrawMenuRegistry(); break;
	case MENU_MAIN: DrawMenuProfile(); break;
	case MENU_STATISTICS: DrawMenuStatistics(); break;
	case MENU_QUIT: DrawMenuProfile(); break;
	case MENU_OPTIONS: DrawMenuOptions(); break;
	case MENU_CREDITS: DrawMenuCredits(); break;
	case MENU_HUNT: DrawMenuHunt(); break;
	}

#ifdef _DEBUG
	// Perform some framerate metric stuff, only for [Debug] builds though
	g_Frames++;

	int64_t t = Timer::GetTime();
	int64_t t_diff = t - g_PrevFrameTime;

	InterfaceSetFont(fontSmall);

	std::stringstream ss;
	ss << "FPS: " << g_FramesPerSecond;
	DrawTextShadow(2, 2, ss.str(), 0xff3c3c);
	ss.str(""); ss.clear();

	ss << "FT:  " << t_diff << "ms";
	DrawTextShadow(2, 2 + 14, ss.str(), 0xff3c3c);
	ss.str(""); ss.clear();

	ss << "XY:  " << g_CursorPos.x << "x" << g_CursorPos.y;
	DrawTextShadow(2, 2 + 28, ss.str(), 0xff3c3c);

	if (t_diff < FRAME_TIME_DELTA) {
		std::this_thread::sleep_for(std::chrono::milliseconds(FRAME_TIME_DELTA - t_diff));
	}

	if ((t - g_PrevFrameCountTime) >= 1000) {
		g_FramesPerSecond = g_Frames;
		g_Frames = 0;
		g_PrevFrameCountTime = t;
	}

	g_PrevFrameTime = Timer::GetTime();
#else // [Release] build frame limiter

	int64_t t = Timer::GetTime();
	int64_t t_diff = t - g_PrevFrameTime;

	if (t_diff < 16) {
		std::this_thread::sleep_for(std::chrono::milliseconds(16 - t_diff));
	}

	g_PrevFrameTime = Timer::GetTime();
#endif

	// Draw the GDI buffer to the window
	InterfaceBlt();
}


void MenuKeyCharEvent(SDL_Keycode code)
{
	if (g_MenuState == MENU_REGISTER) {
		if (code == 8) {
			if (!g_TypingBuffer.empty())
				g_TypingBuffer.pop_back();
		}
		else {
			if (g_TypingBuffer.size() < 19) {
				if (code >= 32 && code <= 128) {
					g_TypingBuffer.push_back(static_cast<char>(code));
				}
			}
		}
	}
}


void MenuMouseScrollEvent(int32_t menu, int32_t scroll)
{
	// TODO: Enable scrolling of text lists
	if (menu == MENU_REGISTER)
	{

	}
	else if (menu == MENU_HUNT)
	{
		for (int i = 0; i < 4; i++)
		{
			if (IsPointInRect(g_CursorPos, MenuHunt[i].Rect) && MenuHunt[i].Item.size() > 10)
			{
				MenuHunt[i].Offset -= scroll;

				if (MenuHunt[i].Offset < 0)
					MenuHunt[i].Offset = 0;
				else if (MenuHunt[i].Offset > MenuHunt[i].Item.size() - 10)
					MenuHunt[i].Offset = MenuHunt[i].Item.size() - 10;
			}
		}
	}
}

