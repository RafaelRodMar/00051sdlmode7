#include<SDL.h>
#include<sdl_ttf.h>
#include <iostream>
#include <algorithm>
#include <vector>
#include <list>
#include <cmath>
#include <fstream>
#include <sstream>
#include <time.h>
#include "game.h"
#include "json.hpp"
#include <chrono>
#include <random>

class Rnd {
public:
	std::mt19937 rng;

	Rnd()
	{
		std::mt19937 prng(std::chrono::steady_clock::now().time_since_epoch().count());
		rng = prng;
	}

	int getRndInt(int min, int max)
	{
		std::uniform_int_distribution<int> distribution(min, max);
		return distribution(rng);
	}

	double getRndDouble(double min, double max)
	{
		std::uniform_real_distribution<double> distribution(min, max);
		return distribution(rng);
	}
} rnd;

//la clase juego:
Game* Game::s_pInstance = 0;

//Takes a camera object, a floating - point value v, and performs perspective projection 
//calculations on a line defined by points a and b based on the camera's near and far planes.
//The resulting projected line is then returned.
Line Game::getProjected(Camera & c, float v)
{
	Line ret;
	float in = 1.f / c.znear;
	float ifar = 1.f / c.zfar;
	float iz = in + v * (ifar - in);
	ret.a.m_x = c.near.a.m_x * in + v * (c.far.a.m_x * ifar - c.near.a.m_x * in);
	ret.a.m_y = c.near.a.m_y * in + v * (c.far.a.m_y * ifar - c.near.a.m_y * in);

	ret.b.m_x = c.near.b.m_x * in + v * (c.far.b.m_x * ifar - c.near.b.m_x * in);
	ret.b.m_y = c.near.b.m_y * in + v * (c.far.b.m_y * ifar - c.near.b.m_y * in);

	ret.a.m_x /= iz;
	ret.b.m_x /= iz;
	ret.a.m_y /= iz;
	ret.b.m_y /= iz;
	return ret;
}

//Takes a floating - point value u and a line defined by points a and b,
//and calculates the texture coordinates at the interpolated position along 
//the line using linear interpolation.The resulting texture coordinates are 
//wrapped or clamped within the range of the texture dimensions and returned
//as a Vector2D object.
Vector2D Game::getTexCoords(float u, Line & line)
{
	Vector2D inter;
	inter.m_x = line.a.m_x + u * (line.b.m_x - line.a.m_x);
	inter.m_y = line.a.m_y + u * (line.b.m_y - line.a.m_y);

	Vector2D ret;
	ret.m_x = inter.m_x;
	ret.m_y = -inter.m_y;

	ret.m_x = (textureW + ((int)ret.m_x % textureW)) % textureW;
	ret.m_y = (textureH + ((int)ret.m_y % textureH)) % textureH;

	return ret;
}

Vector2D Game::getTexCoords2(float u, Line & line)
{
	Vector2D inter;
	inter.m_x = line.a.m_x + u * (line.b.m_x - line.a.m_x);
	inter.m_y = line.a.m_y + u * (line.b.m_y - line.a.m_y);

	Vector2D ret;
	ret.m_x = inter.m_x;
	ret.m_y = -inter.m_y;

	ret.m_x = (skyW + ((int)ret.m_x % skyW)) % skyW;
	ret.m_y = (skyH + ((int)ret.m_y % skyH)) % skyH;

	return ret;
}

Game::Game()
{
	m_pRenderer = NULL;
	m_pWindow = NULL;
}

Game::~Game()
{

}

SDL_Window* g_pWindow = 0;
SDL_Renderer* g_pRenderer = 0;

bool Game::init(const char* title, int xpos, int ypos, int width,
	int height, bool fullscreen)
{
	// almacenar el alto y ancho del juego.
	m_gameWidth = width;
	m_gameHeight = height;

	// attempt to initialize SDL
	if (SDL_Init(SDL_INIT_EVERYTHING) == 0)
	{
		int flags = 0;
		if (fullscreen)
		{
			flags = SDL_WINDOW_FULLSCREEN;
		}

		std::cout << "SDL init success\n";
		// init the window
		m_pWindow = SDL_CreateWindow(title, xpos, ypos,
			width, height, flags);
		if (m_pWindow != 0) // window init success
		{
			std::cout << "window creation success\n";
			m_pRenderer = SDL_CreateRenderer(m_pWindow, -1, 0);
			if (m_pRenderer != 0) // renderer init success
			{
				std::cout << "renderer creation success\n";
				SDL_SetRenderDrawColor(m_pRenderer,
					255, 255, 255, 255);
			}
			else
			{
				std::cout << "renderer init fail\n";
				return false; // renderer init fail
			}
		}
		else
		{
			std::cout << "window init fail\n";
			return false; // window init fail
		}
	}
	else
	{
		std::cout << "SDL init fail\n";
		return false; // SDL init fail
	}
	if (TTF_Init() == 0)
	{
		std::cout << "sdl font initialization success\n";
	}
	else
	{
		std::cout << "sdl font init fail\n";
		return false;
	}

	std::cout << "init success\n";
	m_bRunning = true; // everything inited successfully, start the main loop

	//Joysticks
	InputHandler::Instance()->initialiseJoysticks();

	//load images, sounds, music and fonts
	AssetsManager::Instance()->loadAssetsJson(); //ahora con formato json
	Mix_Volume(-1, 16); //adjust sound/music volume for all channels

	state = GAME;

	sur = SDL_LoadBMP("assets//img//background.bmp");
	tex = SDL_ConvertSurfaceFormat(sur, SDL_PIXELFORMAT_RGBA32, 0);
	SDL_FreeSurface(sur);
	scr = SDL_CreateTexture(m_pRenderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, W, H);

	surSky = SDL_LoadBMP("assets//img//sky.bmp");
	texSky = SDL_ConvertSurfaceFormat(surSky, SDL_PIXELFORMAT_RGBA32, 0);
	SDL_FreeSurface(surSky);
	scrSky = SDL_CreateTexture(m_pRenderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, W, H);

	//camera
	c.pos.m_x = 150;
	c.pos.m_y = 150;
	c.h = 16;
	c.dir = 0;

	textureW = tex->w;
	textureH = tex->h;
	skyW = texSky->w;
	skyH = texSky->h;

	return true;
}

void Game::render()
{
	SDL_SetRenderDrawColor(Game::Instance()->getRenderer(), 100, 100, 255, 0);
	SDL_RenderClear(m_pRenderer); // clear the renderer to the draw color

	if (state == MENU)
	{
	}

	if (state == GAME)
	{
		/*The overall process involve projecting a line in the 3D scene 
		using the camera, and then for each pixel in the desired buffer,
		determining the corresponding texture coordinate and retrieving 
		the pixel value from the texture data.The retrieved pixel values 
		are stored in the buffer for further processing or display.*/
		int* tdata = (int*)tex->pixels;
		for (int y = 0; y < H; y++) {
			float v = 1.f - ((float)y / (float)H);
			Line inter = getProjected(c, v);
			for (int x = 0; x < W; x++) {
				float u = (float)x / (float)W;
				Vector2D coord = getTexCoords(u, inter);
				buff[y][x] = tdata[(int)(coord.m_y * (tex->pitch / 4) + coord.m_x)];
			}
		}
		SDL_UpdateTexture(scr, NULL, buff, W * 4);
		SDL_Rect r;
		r.x = 0;
		r.y = H * pxs;
		r.w = W * pxs;
		r.h = H * pxs;
		SDL_RenderCopy(m_pRenderer, scr, NULL, &r);

		///////sky
		int* tdata2 = (int*)texSky->pixels;
		for (int y = 0; y < H; y++) {
			float v = 1.f - ((float)y / (float)H);
			Line inter = getProjected(c, v);
			for (int x = 0; x < W; x++) {
				float u = (float)x / (float)W;
				Vector2D coord = getTexCoords2(u, inter);
				//cout << H << "-" << y << endl;
				buffSky[H-y-1][x] = tdata2[(int)(coord.m_y * (texSky->pitch / 4) + coord.m_x)];
			}
		}
		SDL_UpdateTexture(scrSky, NULL, buffSky, W * 4);
		r.x = 0;
		r.y = 0;
		r.w = W * pxs;
		r.h = H * pxs;
		SDL_RenderCopy(m_pRenderer, scrSky, NULL, &r);

		//AssetsManager::Instance()->draw("bg", 0, 0, m_gameWidth, 400, m_pRenderer);
	}

	if (state == END_GAME)
	{
	}

	SDL_RenderPresent(m_pRenderer); // draw to the screen
}

void Game::quit()
{
	m_bRunning = false;
}

void Game::clean()
{
	SDL_FreeSurface(tex);
	SDL_FreeSurface(texSky);
	SDL_DestroyTexture(scr);
	SDL_DestroyTexture(scrSky);
	std::cout << "cleaning game\n";
	InputHandler::Instance()->clean();
	AssetsManager::Instance()->clearFonts();
	TTF_Quit();
	SDL_DestroyWindow(m_pWindow);
	SDL_DestroyRenderer(m_pRenderer);
	Game::Instance()->m_bRunning = false;
	SDL_Quit();
}

void Game::handleEvents()
{
	InputHandler::Instance()->update();

	//HandleKeys
	if (state == MENU)
	{
	}

	if (state == GAME)
	{
		bool recalculate = false;
		if (InputHandler::Instance()->isKeyDown(SDL_SCANCODE_W)) c.h += 0.5f;
		if (InputHandler::Instance()->isKeyDown(SDL_SCANCODE_S)) c.h -= 0.5f;
		if (InputHandler::Instance()->isKeyDown(SDL_SCANCODE_LEFT)) c.dir += 2.f;
		if (InputHandler::Instance()->isKeyDown(SDL_SCANCODE_RIGHT)) c.dir -= 2.f;
		if (InputHandler::Instance()->isKeyDown(SDL_SCANCODE_UP))
		{
			c.pos.m_y += cosf(c.dir * DTR) * 3;
			c.pos.m_x -= sinf(c.dir * DTR) * 3;
		}
		if (InputHandler::Instance()->isKeyDown(SDL_SCANCODE_DOWN))
		{
			c.pos.m_y -= cosf(c.dir * DTR) * 3;
			c.pos.m_x += sinf(c.dir * DTR) * 3;
		}

		c.update();
	}

	if (state == END_GAME)
	{
	}
}

bool Game::isCollide(Entity *a, Entity *b)
{
	return (b->m_position.m_x - a->m_position.m_x)*(b->m_position.m_x - a->m_position.m_x) +
		(b->m_position.m_y - a->m_position.m_y)*(b->m_position.m_y - a->m_position.m_y) <
		(a->m_radius + b->m_radius)*(a->m_radius + b->m_radius);
}

bool Game::isCollideRect(Entity *a, Entity * b) {
	if (a->m_position.m_x < b->m_position.m_x + b->m_width &&
		a->m_position.m_x + a->m_width > b->m_position.m_x &&
		a->m_position.m_y < b->m_position.m_y + b->m_height &&
		a->m_height + a->m_position.m_y > b->m_position.m_y) {
		return true;
	}
	return false;
}

void Game::update()
{
	/*if (state == GAME)
	{
		for (auto i = entities.begin(); i != entities.end(); i++)
		{
			Entity *e = *i;

			e->update();
		}
	}*/

}

const int FPS = 60;
const int DELAY_TIME = 1000.0f / FPS;

int main(int argc, char* args[])
{
	srand(time(NULL));

	Uint32 frameStart, frameTime;

	std::cout << "game init attempt...\n";
	if (Game::Instance()->init("Pseudo 3D planes (Mode 7)", 100, 100, W * pxs, 2 * H * pxs,
		false))
	{
		std::cout << "game init success!\n";
		while (Game::Instance()->running())
		{
			frameStart = SDL_GetTicks(); //tiempo inicial

			Game::Instance()->handleEvents();
			Game::Instance()->update();
			Game::Instance()->render();

			frameTime = SDL_GetTicks() - frameStart; //tiempo final - tiempo inicial

			if (frameTime < DELAY_TIME)
			{
				//con tiempo fijo el retraso es 1000 / 60 = 16,66
				//procesar handleEvents, update y render tarda 1, y hay que esperar 15
				//cout << "frameTime : " << frameTime << "  delay : " << (int)(DELAY_TIME - frameTime) << endl;
				SDL_Delay((int)(DELAY_TIME - frameTime)); //esperamos hasta completar los 60 fps
			}
		}
	}
	else
	{
		std::cout << "game init failure - " << SDL_GetError() << "\n";
		return -1;
	}
	std::cout << "game closing...\n";
	Game::Instance()->clean();
	return 0;
}