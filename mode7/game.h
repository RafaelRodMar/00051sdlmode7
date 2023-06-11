#pragma once

#include <vector>
#include <list>
#include <SDL.h>
#include <SDL_image.h>
#include "Vector2D.h"
#include "AssetsManager.h"
#include "InputHandler.h"
#include "Entity.h"

const float PI = 3.1415916;
const float DTR = PI / 180.f;
const int W = 400;
const int H = 200;
const int pxs = 2;

////Game States
enum GAMESTATES {SPLASH, MENU, LOAD_LEVEL, GAME, END_GAME};
enum SUBSTATES {LINES, TILES, TERRAIN, TERRAINRECT};

struct Line {
	Vector2D a, b;
	float getDist() {
		float x = a.m_x - b.m_x;
		float y = a.m_y - b.m_y;
		return sqrt(x*x + y * y);
	}
};

struct Camera {
public:
	Vector2D pos;
	float dir;
	float znear, zfar;
	Line near, far;
	float h;
	
	Vector2D transformPoint(Vector2D& p) {
		Vector2D ret;
		ret.m_x = pos.m_x + p.m_x * cosf(dir * DTR) - p.m_y * sinf(dir * DTR);
		ret.m_y = pos.m_y + p.m_y * cosf(dir * DTR) + p.m_x * sinf(dir * DTR);
		return ret;
	}

	void update() {
		znear = h;
		near.a.m_x = -znear;
		near.a.m_y = znear;
		near.a = transformPoint(near.a);

		near.b.m_x = znear;
		near.b.m_y = znear;
		near.b = transformPoint(near.b);

		zfar = h * 20;

		far.a.m_x = -zfar;
		far.a.m_y = zfar;
		far.a = transformPoint(far.a);

		far.b.m_x = zfar;
		far.b.m_y = zfar;
		far.b = transformPoint(far.b);
	}
};

class Game {
public:
	static Game* Instance()
	{
		if (s_pInstance == 0)
		{
			s_pInstance = new Game();
			return s_pInstance;
		}
		return s_pInstance;
	}
	
	SDL_Renderer* getRenderer() const { return m_pRenderer; }
	
	~Game();

	bool init(const char* title, int xpos, int ypos, int width, int height, bool fullscreen);
	void render();
	void update();
	void handleEvents();
	void clean();
	void quit();

	bool running() { return m_bRunning; }

	int getGameWidth() const { return m_gameWidth; }
	int getGameHeight() const { return m_gameHeight; }
	Line getProjected(Camera& c, float v);
	Vector2D getTexCoords(float u, Line& line);
	Vector2D getTexCoords2(float u, Line& line);

private:
	Game();
	static Game* s_pInstance;
	SDL_Window* m_pWindow;
	SDL_Renderer* m_pRenderer;

	player *p;
	int state = -1;
	int subState = -1;
	int cellSize = 20;

	Camera c;
	//terrain
	int buff[H][W]; //size of the window
	int textureW, textureH;
	SDL_Surface* sur;
	SDL_Surface* tex;
	SDL_Texture* scr;

	//sky
	int buffSky[H][W];
	int skyW, skyH;
	SDL_Surface* surSky;
	SDL_Surface* texSky;
	SDL_Texture* scrSky;

	std::list<Entity*> entities;
	bool isCollide(Entity *a, Entity *b);
	bool isCollideRect(Entity *a, Entity *b);
	//std::vector<GameObject*> m_gameObjects;

	bool m_bRunning;
	int m_gameWidth;
	int m_gameHeight;
};