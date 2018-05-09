//CONTROLS: 

#ifdef _WINDOWS
	#include <GL/glew.h>
#endif
#include <SDL_mixer.h>
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_image.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "ShaderProgram.h"
#include "Matrix.h"
#include "FlareMap.h"
#include <iostream>
#include <vector>
#include <random>
#include <functional>
using namespace std;

#ifdef _WINDOWS
	#define RESOURCE_FOLDER ""
#else
	#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

class Vector3 {
public:
	Vector3():x(0), y(0), z(0) {}
	Vector3(float x, float y, float z): x(x), y(y), z(z){}
	float x;
	float y;
	float z;
};

class SheetSprite {
public:
	SheetSprite(){};
	SheetSprite(unsigned int textureID, float u, float v, float width, float height, float size) :
		textureID(textureID), u(u), v(v), width(width), height(height), size(size) {};
	void Draw(ShaderProgram *program){
		glBindTexture(GL_TEXTURE_2D, textureID);
		GLfloat texCoords[] = {
			u, v + height,
			u + width, v,
			u, v,
			u + width, v,
			u, v + height,
			u + width, v + height
		};
		float aspect = width / height;
		float vertices[] = {
			-0.5f * size * aspect, -0.5f * size,
			0.5f * size * aspect, 0.5f * size,
			-0.5f * size * aspect, 0.5f * size,
			0.5f * size * aspect, 0.5f * size,
			-0.5f * size * aspect, -0.5f * size,
			0.5f * size * aspect, -0.5f * size 
		};
		glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
		glEnableVertexAttribArray(program->texCoordAttribute);
		glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertices);
		glEnableVertexAttribArray(program->positionAttribute);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glDisableVertexAttribArray(program->positionAttribute);
		glDisableVertexAttribArray(program->texCoordAttribute);
	};
	float size;
	unsigned int textureID;
	float u;
	float v;
	float width;
	float height;
};

float lerp(float v0, float v1, float t) {
	return (1.0 - t)*v0 + t * v1;
}

class Entity {
public:
	Entity() {};
	void Draw(ShaderProgram *program) {
		Matrix modelMatrix;
		modelMatrix.SetPosition(position.x, position.y, position.z);
		modelMatrix.SetScale(size.x*2, size.y, 1);//modify this based on spritesheet size
		program->SetModelMatrix(modelMatrix);
		sprite.Draw(program);
	};
	void Update(float elapsed) {
		if (!isStatic) {
			velocity.x = lerp(velocity.x, 0.0f, elapsed * 1.5);//x friction
			velocity.y = lerp(velocity.y, 0.0f, elapsed * 1.5);//y friction
			velocity.x += acceleration.x * elapsed;
			velocity.y += acceleration.y * elapsed;
			position.x += velocity.x * elapsed;
			position.y += velocity.y * elapsed;
		}
	};
	Vector3 position;
	Vector3 velocity;
	Vector3 acceleration;
	Vector3 size;
	float rotation;
	SheetSprite sprite;
	bool isStatic;
	bool collidedTop;
	bool collidedBottom;
	bool collidedLeft;
	bool collidedRight;
};

GLuint LoadTexture(const char *filePath) {
	int w, h, comp;
	unsigned char* image = stbi_load(filePath, &w, &h, &comp, STBI_rgb_alpha);
	if (image == NULL) {
		std::cout << "Unable to load image. Make sure the path is correct\n";
		assert(false);
	}
	GLuint retTexture;
	glGenTextures(1, &retTexture);
	glBindTexture(GL_TEXTURE_2D, retTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	stbi_image_free(image);
	return retTexture;
}

void DrawText(ShaderProgram *program, int fontTexture, std::string text, float size, float spacing) {
	float texture_size = 1.0 / 16.0f;
	std::vector<float> vertexData;
	std::vector<float> texCoordData;

	for (int i = 0; i < text.size(); i++) {
		int spriteIndex = (int)text[i];
		float texture_x = (float)(spriteIndex % 16) / 16.0f;
		float texture_y = (float)(spriteIndex / 16) / 16.0f;

		vertexData.insert(vertexData.end(), {
			((size + spacing) * i) + (-0.5f * size), 0.5f * size,
			((size + spacing) * i) + (-0.5f * size), -0.5f * size,
			((size + spacing) * i) + (0.5f * size), 0.5f * size,
			((size + spacing) * i) + (0.5f * size), -0.5f * size,
			((size + spacing) * i) + (0.5f * size), 0.5f * size,
			((size + spacing) * i) + (-0.5f * size), -0.5f * size,
			});
		texCoordData.insert(texCoordData.end(), {
			texture_x, texture_y,
			texture_x, texture_y + texture_size,
			texture_x + texture_size, texture_y,
			texture_x + texture_size, texture_y + texture_size,
			texture_x + texture_size, texture_y,
			texture_x, texture_y + texture_size,
			});
	}
	glBindTexture(GL_TEXTURE_2D, fontTexture);
	// draw this data (use the .data() method of std::vector to get pointer to data)
	// draw this yourself, use text.size() * 6 or vertexData.size()/2 to get number of vertices
	glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoordData.data());
	glEnableVertexAttribArray(program->texCoordAttribute);
	glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertexData.data());
	glEnableVertexAttribArray(program->positionAttribute);
	glDrawArrays(GL_TRIANGLES, 0, text.size()*6);
	glDisableVertexAttribArray(program->positionAttribute);
	glDisableVertexAttribArray(program->texCoordAttribute);
}

bool entitiesCollided(Entity entity1, Entity entity2) {
	if (entity1.position.y - (entity1.size.y / 2) >= entity2.position.y + (entity2.size.y / 2)) {
		return false;
	}
	else if (entity1.position.y + (entity1.size.y / 2) <= entity2.position.y - (entity2.size.y / 2)) {
		return false;
	}
	else if (entity1.position.x - (entity1.size.x / 2) >= entity2.position.x + (entity2.size.x / 2)) {
		return false;
	}
	else if (entity1.position.x + (entity1.size.x / 2) <= entity2.position.x - (entity2.size.x / 2)) {
		return false;
	}
	else { 
		return true; 
	}
}

/*void yCollision(Entity *entity1, Entity *entity2) {
	float penetration = fabs((entity1->position.y - entity2->position.y) - entity1->size.y / 2 - entity2->size.y / 2);
	if (entitiesCollided(*entity1, *entity2)) {
		if (entity1->position.y - (entity1->size.y / 2) <= entity2->position.y + (entity2->size.y / 2) 
			&& entity1->position.y - (entity1->size.y / 2) >= entity2->position.y
			
			) {
			entity1->position.y += penetration + .01;
		}
		float penetration = fabs((entity2->position.y - entity1->position.y) - entity2->size.y / 2 - entity1->size.y / 2);
		if (entity1->position.y + (entity1->size.y / 2) >= entity2->position.y - (entity2->size.y / 2)
			&& entity1->position.y + (entity1->size.y / 2) <= entity2->position.y
			
			) {
			entity1->position.y -= penetration + .01;
		}
	}
}

void xCollision(Entity *entity1, Entity *entity2) {
	float penetration = fabs((entity1->position.x - entity2->position.x) - entity1->size.x / 2 - entity2->size.x / 2);
	if (entitiesCollided(*entity1, *entity2)) {
		if (entity1->position.x - (entity1->size.x / 2) <= entity2->position.x + (entity2->size.x / 2)
			&& entity1->position.x - (entity1->size.x / 2) >= entity2->position.x
			
			) {
			entity1->position.x += penetration + .01;
		}
		float penetration = fabs((entity2->position.x - entity1->position.x) - entity2->size.x / 2 - entity1->size.x / 2);
		if (entity1->position.x + (entity1->size.x / 2) >= entity2->position.x - (entity2->size.x / 2)
			&& entity1->position.x + (entity1->size.x / 2) <= entity2->position.x
			
			) {
			entity1->position.x -= penetration + .01;
		}
	}
}
*/

//left,right,top,bottom are with respect to entity1
void collision(Entity *entity1, Entity *entity2) {
	float xPenetrationLeft = fabs((entity1->position.x - entity2->position.x) - entity1->size.x / 2 - entity2->size.x / 2);
	float xPenetrationRight = fabs((entity2->position.x - entity1->position.x) - entity2->size.x / 2 - entity1->size.x / 2);
	float yPenetrationBottom = fabs((entity1->position.y - entity2->position.y) - entity1->size.y / 2 - entity2->size.y / 2);
	float yPenetrationTop = fabs((entity2->position.y - entity1->position.y) - entity2->size.y / 2 - entity1->size.y / 2);
	if (entitiesCollided(*entity1, *entity2)) {
		if (entity1->position.x - (entity1->size.x / 2) <= entity2->position.x + (entity2->size.x / 2)
			&& entity1->position.x - (entity1->size.x / 2) >= entity2->position.x + (entity2->size.x / 2.2)
			//&& (xPenetrationLeft < yPenetrationBottom || xPenetrationLeft < yPenetrationTop)
			) {
			entity1->position.x += xPenetrationLeft + .01;
			entity1->velocity.x = 0;
		}
		else if (entity1->position.x + (entity1->size.x / 2) >= entity2->position.x - (entity2->size.x / 2)
			&& entity1->position.x + (entity1->size.x / 2) <= entity2->position.x - (entity2->size.x / 2.2)
			//&& (xPenetrationRight < yPenetrationBottom || xPenetrationRight < yPenetrationTop)
			) {
			entity1->position.x -= xPenetrationRight + .01;
			entity1->velocity.x = 0;
		}
		else if (entity1->position.y - (entity1->size.y / 2) <= entity2->position.y + (entity2->size.y / 2)
			&& entity1->position.y - (entity1->size.y / 2) >= entity2->position.y + (entity2->size.y / 2.2)
			//&& (yPenetrationBottom < xPenetrationLeft || yPenetrationBottom < xPenetrationRight)
			) {
			entity1->position.y += yPenetrationBottom + .01;
			entity1->velocity.y = 0;
		}
		else if (entity1->position.y + (entity1->size.y / 2) >= entity2->position.y - (entity2->size.y / 2)
			&& entity1->position.y + (entity1->size.y / 2) <= entity2->position.y - (entity2->size.y / 2.2)
			//&& (yPenetrationTop < xPenetrationLeft || yPenetrationTop < xPenetrationRight)
			) {
			entity1->position.y -= yPenetrationTop + .01;
			entity1->velocity.y = 0;
		}
	}
}

//max is the total amount of bullets in the array
void shootBullet(Entity bullets[], int &index, int max, Entity character) {
	bullets[index].position.x = character.position.x;
	bullets[index].position.y = character.position.y;
	index++;
	if (index > max - 1) {
		index = 0;
	}
}


int main(int argc, char *argv[])
{
	//Setup
	SDL_Window* displayWindow;
	SDL_Init(SDL_INIT_VIDEO);
	displayWindow = SDL_CreateWindow("Final Project", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640*1.9, 360*1.9, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);
	#ifdef _WINDOWS
		glewInit();
	#endif

	Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096);
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glViewport(0, 0, 640*1.9, 360*1.9);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	ShaderProgram program;
	program.Load(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");
	glUseProgram(program.programID);
	Matrix viewMatrix;
	Vector3 viewMatrixPosition = Vector3(0, 0, 0);
	Matrix projectionMatrix;
	projectionMatrix.SetOrthoProjection(-3.55, 3.55, -2.0f, 2.0f, -1.0f, 1.0f);
	float lastFrameTicks = 0.0f;
	// 60 FPS (1.0f/60.0f) (update sixty times a second)
	#define FIXED_TIMESTEP 0.0166666f
	#define MAX_TIMESTEPS 6
	float accumulator = 0.0f;
	SDL_Event event;
	bool done = false;

	enum GameMode { STATE_MAIN_MENU, STATE_GAME_LEVEL_1, STATE_GAME_LEVEL_2, STATE_GAME_LEVEL_3, STATE_GAME_OVER };
	GameMode mode = STATE_MAIN_MENU;
	SDL_Joystick * playerOneController = SDL_JoystickOpen(0);
	SDL_Joystick * playerTwoController = SDL_JoystickOpen(1);

	std::default_random_engine generator;
	std::uniform_int_distribution<int> distribution(0, 500);
	auto bulletRNG = std::bind(distribution, generator);

	//textures, entities, etc.
	Mix_Music *main_menu;
	main_menu = Mix_LoadMUS("menu.mp3");
	Mix_Music *lvl1;
	lvl1 = Mix_LoadMUS("lvl1.mp3");
	Mix_Music *lvl2;
	lvl2 = Mix_LoadMUS("lvl2.mp3");
	Mix_Music *lvl3;
	lvl3 = Mix_LoadMUS("lvl3.mp3");
	Mix_Music *results;
	results = Mix_LoadMUS("results.mp3");
	Mix_Chunk *p1laser;
	p1laser = Mix_LoadWAV("p1laser.wav");
	Mix_Chunk *p2laser;
	p2laser = Mix_LoadWAV("p2laser.wav");
	Mix_Chunk *hurt;
	hurt = Mix_LoadWAV("hurt.wav");
	Mix_Chunk *select;
	select = Mix_LoadWAV("select.wav");
	Mix_Chunk *death;
	death = Mix_LoadWAV("death.wav");
	GLuint sprites = LoadTexture(RESOURCE_FOLDER"sprites.png");
	GLuint text = LoadTexture(RESOURCE_FOLDER"pixel_font.png");
	Matrix textModel;
	GLuint background = LoadTexture(RESOURCE_FOLDER"background.png");
	Matrix backgroundmodel;

	Entity player1;
	player1.isStatic = false;
	player1.sprite = SheetSprite(sprites, 132.0f / 512.0f, 66.0f / 256.0f, 64.0f / 512.0f, 64.0f / 256.0f, 1.0f);
	player1.position = Vector3(-2.5, 0, 0);
	player1.size = Vector3(0.15, 0.15, 0);
	int p1HP = 8;
	int p1Ammo = 20;
	bool p1reloading = false;
	float p1reloadtimer = 0;
	bool p1boostused = false;
	float p1boosttimer = 0;
	int p1_win_count = 0;

	#define MAX_BULLETS 24
	int bullet1Index = 0;
	Entity bullets1[MAX_BULLETS];
	for (int i = 0; i < MAX_BULLETS; i++) {
		bullets1[i].isStatic = false;
		bullets1[i].position = Vector3(-2000.0f, 0, 0);
		bullets1[i].sprite = SheetSprite(sprites, 0.0f / 512.0f, 196.0f / 256.0f, 43.0f / 512.0f, 42.0f / 256.0f, 1.0f);
		bullets1[i].size = Vector3(0.15, 0.15, 0);
	}

	Entity player2;
	player2.isStatic = false;
	player2.sprite = SheetSprite(sprites, 262.0f / 512.0f, 0.0f / 256.0f, 64.0f / 512.0f, 64.0f / 256.0f, 1.0f);
	player2.position = Vector3(2.5, 0, 0);
	player2.size = Vector3(0.15, 0.15, 0);
	int p2HP = 8;
	int p2Ammo = 20;
	bool p2reloading = false;
	float p2reloadtimer = 0;
	bool p2boostused = false;
	float p2boosttimer = 0;
	int p2_win_count = 0;

	int bullet2Index = 0;
	Entity bullets2[MAX_BULLETS];
	for (int i = 0; i < MAX_BULLETS; i++) {
		bullets2[i].isStatic = false;
		bullets2[i].position = Vector3(-2000.0f, 0, 0);
		bullets2[i].sprite = SheetSprite(sprites, 45.0f / 512.0f, 196.0f / 256.0f, 43.0f / 512.0f, 42.0f / 256.0f, 1.0f);
		bullets2[i].size = Vector3(0.15, 0.15, 0);
	}

	Entity floor;
	floor.isStatic = true;
	floor.sprite = SheetSprite(sprites, 198.0f / 512.0f, 132.0f / 256.0f, 64.0f / 512.0f, 64.0f / 256.0f, 1.0f);
	floor.position = Vector3(0, 0, 0);
	floor.size = Vector3(6.5, 3.5, 0);

	Entity lvl_1_Objects[4];
	for (int i = 0; i < 4; i++) {
		lvl_1_Objects[i].isStatic = true;
		lvl_1_Objects[i].sprite = SheetSprite(sprites, 0.0f / 512.0f, 0.0f / 256.0f, 128.0f / 512.0f, 128.0f / 256.0f, 1.0f);
		lvl_1_Objects[i].size = Vector3(.7, .7, 0);
	}
	lvl_1_Objects[0].position = Vector3(1.3, .8, 0);
	lvl_1_Objects[1].position = Vector3(1.3, -.8, 0);
	lvl_1_Objects[2].position = Vector3(-1.3, .8, 0);
	lvl_1_Objects[3].position = Vector3(-1.3, -.8, 0);

	Entity lvl_2_Objects[2];
	for (int i = 0; i < 2; i++) {
		lvl_2_Objects[i].isStatic = true;
		lvl_2_Objects[i].sprite = SheetSprite(sprites, 0.0f / 512.0f, 0.0f / 256.0f, 128.0f / 512.0f, 128.0f / 256.0f, 1.0f);
		lvl_2_Objects[i].size = Vector3(.35, 2, 0);
	}
	lvl_2_Objects[0].position = Vector3(1.3, 0, 0);
	lvl_2_Objects[1].position = Vector3(-1.3, 0, 0);

	Entity lvl_3_Objects[1];
	for (int i = 0; i < 1; i++) {
		lvl_3_Objects[i].isStatic = true;
		lvl_3_Objects[i].sprite = SheetSprite(sprites, 196.0f / 512.0f, 0.0f / 256.0f, 64.0f / 512.0f, 64.0f / 256.0f, 1.2f);
		lvl_3_Objects[i].size = Vector3(1.4, 1.4, 0);
	}
	lvl_3_Objects[0].position = Vector3(0, 0, 0);

	float screenShakeValue = 0;
	float screenShakeTime = 0;

	Mix_PlayMusic(main_menu, -1);
	//
	while (!done) {
		//Input events
		switch (mode) {
		case STATE_MAIN_MENU:
			while (SDL_PollEvent(&event)) {
				if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
					done = true;
				}
				else if (event.type == SDL_KEYDOWN) {
					if (event.key.keysym.scancode == SDL_SCANCODE_RETURN) {
						viewMatrixPosition.x = 8;
						Mix_PlayChannel(-1, select, 0);
						Mix_HaltMusic();
						Mix_PlayMusic(lvl1, -1);
						mode = STATE_GAME_LEVEL_1;
					}
				}
			}
		break;
		case STATE_GAME_LEVEL_1:	
			while (SDL_PollEvent(&event)) {
				if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
					done = true;
				
				}
				else if (event.type == SDL_KEYDOWN && event.key.repeat == 0) {
					//player1 shooting
					if (event.key.keysym.scancode == SDL_SCANCODE_G && !p1reloading && p1Ammo > 0) {
						player1.sprite = SheetSprite(sprites, 130.0f / 512.0f, 0.0f / 256.0f, 64.0f / 512.0f, 64.0f / 256.0f, 1.0f);
						bullets1[bullet1Index].acceleration.y = 9;
						bullets1[bullet1Index].velocity.y = player1.velocity.y;
						bullets1[bullet1Index].velocity.x = player1.velocity.x;
						shootBullet(bullets1, bullet1Index, MAX_BULLETS, player1);
						Mix_PlayChannel(-1, p1laser, 0);
						p1Ammo -= 1;
					}
					else if (event.key.keysym.scancode == SDL_SCANCODE_B && !p1reloading && p1Ammo > 0) {
						player1.sprite = SheetSprite(sprites, 132.0f / 512.0f, 66.0f / 256.0f, 64.0f / 512.0f, 64.0f / 256.0f, 1.0f);
						bullets1[bullet1Index].acceleration.y = -9;
						bullets1[bullet1Index].velocity.y = player1.velocity.y;
						bullets1[bullet1Index].velocity.x = player1.velocity.x;
						shootBullet(bullets1, bullet1Index, MAX_BULLETS, player1);
						Mix_PlayChannel(-1, p1laser, 0);
						p1Ammo -= 1;
					}
					else if (event.key.keysym.scancode == SDL_SCANCODE_N && !p1reloading && p1Ammo > 0) {
						player1.sprite = SheetSprite(sprites, 0.0f / 512.0f, 130.0f / 256.0f, 64.0f / 512.0f, 64.0f / 256.0f, 1.0f);
						bullets1[bullet1Index].acceleration.x = 9;
						bullets1[bullet1Index].velocity.y = player1.velocity.y;
						bullets1[bullet1Index].velocity.x = player1.velocity.x;
						shootBullet(bullets1, bullet1Index, MAX_BULLETS, player1);
						Mix_PlayChannel(-1, p1laser, 0);
						p1Ammo -= 1;
					}
					else if (event.key.keysym.scancode == SDL_SCANCODE_V && !p1reloading && p1Ammo > 0) {
						player1.sprite = SheetSprite(sprites, 264.0f / 512.0f, 66.0f / 256.0f, 64.0f / 512.0f, 64.0f / 256.0f, 1.0f);
						bullets1[bullet1Index].acceleration.x = -9;
						bullets1[bullet1Index].velocity.y = player1.velocity.y;
						bullets1[bullet1Index].velocity.x = player1.velocity.x;
						shootBullet(bullets1, bullet1Index, MAX_BULLETS, player1);
						Mix_PlayChannel(-1, p1laser, 0);
						p1Ammo -= 1;
					}
					//player2 shooting
					if (event.key.keysym.scancode == SDL_SCANCODE_KP_8 && !p2reloading && p2Ammo > 0) {
						player2.sprite = SheetSprite(sprites, 132.0f / 512.0f, 132.0f / 256.0f, 64.0f / 512.0f, 64.0f / 256.0f, 1.0f);
						bullets2[bullet2Index].acceleration.y = 9;
						bullets2[bullet2Index].velocity.y = player2.velocity.y;
						bullets2[bullet2Index].velocity.x = player2.velocity.x;
						shootBullet(bullets2, bullet2Index, MAX_BULLETS, player2);
						Mix_PlayChannel(-1, p2laser, 0);
						p2Ammo -= 1;
					}
					else if (event.key.keysym.scancode == SDL_SCANCODE_KP_5 && !p2reloading && p2Ammo > 0) {
						player2.sprite = SheetSprite(sprites, 262.0f / 512.0f, 0.0f / 256.0f, 64.0f / 512.0f, 64.0f / 256.0f, 1.0f);
						bullets2[bullet2Index].acceleration.y = -9;
						bullets2[bullet2Index].velocity.y = player2.velocity.y;
						bullets2[bullet2Index].velocity.x = player2.velocity.x;
						shootBullet(bullets2, bullet2Index, MAX_BULLETS, player2);
						Mix_PlayChannel(-1, p2laser, 0);
						p2Ammo -= 1;
					}
					else if (event.key.keysym.scancode == SDL_SCANCODE_KP_6 && !p2reloading && p2Ammo > 0) {
						player2.sprite = SheetSprite(sprites, 66.0f / 512.0f, 130.0f / 256.0f, 64.0f / 512.0f, 64.0f / 256.0f, 1.0f);
						bullets2[bullet2Index].acceleration.x = 9;
						bullets2[bullet2Index].velocity.y = player2.velocity.y;
						bullets2[bullet2Index].velocity.x = player2.velocity.x;
						shootBullet(bullets2, bullet2Index, MAX_BULLETS, player2);
						Mix_PlayChannel(-1, p2laser, 0);
						p2Ammo -= 1;
					}
					else if (event.key.keysym.scancode == SDL_SCANCODE_KP_4 && !p2reloading && p2Ammo > 0) {
						player2.sprite = SheetSprite(sprites, 198.0f / 512.0f, 66.0f / 256.0f, 64.0f / 512.0f, 64.0f / 256.0f, 1.0f);
						bullets2[bullet2Index].acceleration.x = -9;
						bullets2[bullet2Index].velocity.y = player2.velocity.y;
						bullets2[bullet2Index].velocity.x = player2.velocity.x;
						shootBullet(bullets2, bullet2Index, MAX_BULLETS, player2);
						Mix_PlayChannel(-1, p2laser, 0);
						p2Ammo -= 1;
					}
				}
			}
		break;
		case STATE_GAME_LEVEL_2:
			while (SDL_PollEvent(&event)) {
				if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
					done = true;

				}
				else if (event.type == SDL_KEYDOWN && event.key.repeat == 0) {
					//player1 shooting
					if (event.key.keysym.scancode == SDL_SCANCODE_G && !p1reloading && p1Ammo > 0) {
						player1.sprite = SheetSprite(sprites, 130.0f / 512.0f, 0.0f / 256.0f, 64.0f / 512.0f, 64.0f / 256.0f, 1.0f);
						bullets1[bullet1Index].acceleration.y = 9;
						bullets1[bullet1Index].velocity.y = player1.velocity.y;
						bullets1[bullet1Index].velocity.x = player1.velocity.x;
						shootBullet(bullets1, bullet1Index, MAX_BULLETS, player1);
						Mix_PlayChannel(-1, p1laser, 0);
						p1Ammo -= 1;
					}
					else if (event.key.keysym.scancode == SDL_SCANCODE_B && !p1reloading && p1Ammo > 0) {
						player1.sprite = SheetSprite(sprites, 132.0f / 512.0f, 66.0f / 256.0f, 64.0f / 512.0f, 64.0f / 256.0f, 1.0f);
						bullets1[bullet1Index].acceleration.y = -9;
						bullets1[bullet1Index].velocity.y = player1.velocity.y;
						bullets1[bullet1Index].velocity.x = player1.velocity.x;
						shootBullet(bullets1, bullet1Index, MAX_BULLETS, player1);
						Mix_PlayChannel(-1, p1laser, 0);
						p1Ammo -= 1;
					}
					else if (event.key.keysym.scancode == SDL_SCANCODE_N && !p1reloading && p1Ammo > 0) {
						player1.sprite = SheetSprite(sprites, 0.0f / 512.0f, 130.0f / 256.0f, 64.0f / 512.0f, 64.0f / 256.0f, 1.0f);
						bullets1[bullet1Index].acceleration.x = 9;
						bullets1[bullet1Index].velocity.y = player1.velocity.y;
						bullets1[bullet1Index].velocity.x = player1.velocity.x;
						shootBullet(bullets1, bullet1Index, MAX_BULLETS, player1);
						Mix_PlayChannel(-1, p1laser, 0);
						p1Ammo -= 1;
					}
					else if (event.key.keysym.scancode == SDL_SCANCODE_V && !p1reloading && p1Ammo > 0) {
						player1.sprite = SheetSprite(sprites, 264.0f / 512.0f, 66.0f / 256.0f, 64.0f / 512.0f, 64.0f / 256.0f, 1.0f);
						bullets1[bullet1Index].acceleration.x = -9;
						bullets1[bullet1Index].velocity.y = player1.velocity.y;
						bullets1[bullet1Index].velocity.x = player1.velocity.x;
						shootBullet(bullets1, bullet1Index, MAX_BULLETS, player1);
						Mix_PlayChannel(-1, p1laser, 0);
						p1Ammo -= 1;
					}
					//player2 shooting
					if (event.key.keysym.scancode == SDL_SCANCODE_KP_8 && !p2reloading && p2Ammo > 0) {
						player2.sprite = SheetSprite(sprites, 132.0f / 512.0f, 132.0f / 256.0f, 64.0f / 512.0f, 64.0f / 256.0f, 1.0f);
						bullets2[bullet2Index].acceleration.y = 9;
						bullets2[bullet2Index].velocity.y = player2.velocity.y;
						bullets2[bullet2Index].velocity.x = player2.velocity.x;
						shootBullet(bullets2, bullet2Index, MAX_BULLETS, player2);
						Mix_PlayChannel(-1, p2laser, 0);
						p2Ammo -= 1;
					}
					else if (event.key.keysym.scancode == SDL_SCANCODE_KP_5 && !p2reloading && p2Ammo > 0) {
						player2.sprite = SheetSprite(sprites, 262.0f / 512.0f, 0.0f / 256.0f, 64.0f / 512.0f, 64.0f / 256.0f, 1.0f);
						bullets2[bullet2Index].acceleration.y = -9;
						bullets2[bullet2Index].velocity.y = player2.velocity.y;
						bullets2[bullet2Index].velocity.x = player2.velocity.x;
						shootBullet(bullets2, bullet2Index, MAX_BULLETS, player2);
						Mix_PlayChannel(-1, p2laser, 0);
						p2Ammo -= 1;
					}
					else if (event.key.keysym.scancode == SDL_SCANCODE_KP_6 && !p2reloading && p2Ammo > 0) {
						player2.sprite = SheetSprite(sprites, 66.0f / 512.0f, 130.0f / 256.0f, 64.0f / 512.0f, 64.0f / 256.0f, 1.0f);
						bullets2[bullet2Index].acceleration.x = 9;
						bullets2[bullet2Index].velocity.y = player2.velocity.y;
						bullets2[bullet2Index].velocity.x = player2.velocity.x;
						shootBullet(bullets2, bullet2Index, MAX_BULLETS, player2);
						Mix_PlayChannel(-1, p2laser, 0);
						p2Ammo -= 1;
					}
					else if (event.key.keysym.scancode == SDL_SCANCODE_KP_4 && !p2reloading && p2Ammo > 0) {
						player2.sprite = SheetSprite(sprites, 198.0f / 512.0f, 66.0f / 256.0f, 64.0f / 512.0f, 64.0f / 256.0f, 1.0f);
						bullets2[bullet2Index].acceleration.x = -9;
						bullets2[bullet2Index].velocity.y = player2.velocity.y;
						bullets2[bullet2Index].velocity.x = player2.velocity.x;
						shootBullet(bullets2, bullet2Index, MAX_BULLETS, player2);
						Mix_PlayChannel(-1, p2laser, 0);
						p2Ammo -= 1;
					}
				}
			}
		break;
		case STATE_GAME_LEVEL_3:
			while (SDL_PollEvent(&event)) {
				if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
					done = true;

				}
				else if (event.type == SDL_KEYDOWN && event.key.repeat == 0) {
					//player1 shooting
					if (event.key.keysym.scancode == SDL_SCANCODE_G && !p1reloading && p1Ammo > 0) {
						player1.sprite = SheetSprite(sprites, 130.0f / 512.0f, 0.0f / 256.0f, 64.0f / 512.0f, 64.0f / 256.0f, 1.0f);
						bullets1[bullet1Index].acceleration.y = 9;
						bullets1[bullet1Index].velocity.y = player1.velocity.y;
						bullets1[bullet1Index].velocity.x = player1.velocity.x;
						shootBullet(bullets1, bullet1Index, MAX_BULLETS, player1);
						Mix_PlayChannel(-1, p1laser, 0);
						p1Ammo -= 1;
					}
					else if (event.key.keysym.scancode == SDL_SCANCODE_B && !p1reloading && p1Ammo > 0) {
						player1.sprite = SheetSprite(sprites, 132.0f / 512.0f, 66.0f / 256.0f, 64.0f / 512.0f, 64.0f / 256.0f, 1.0f);
						bullets1[bullet1Index].acceleration.y = -9;
						bullets1[bullet1Index].velocity.y = player1.velocity.y;
						bullets1[bullet1Index].velocity.x = player1.velocity.x;
						shootBullet(bullets1, bullet1Index, MAX_BULLETS, player1);
						Mix_PlayChannel(-1, p1laser, 0);
						p1Ammo -= 1;
					}
					else if (event.key.keysym.scancode == SDL_SCANCODE_N && !p1reloading && p1Ammo > 0) {
						player1.sprite = SheetSprite(sprites, 0.0f / 512.0f, 130.0f / 256.0f, 64.0f / 512.0f, 64.0f / 256.0f, 1.0f);
						bullets1[bullet1Index].acceleration.x = 9;
						bullets1[bullet1Index].velocity.y = player1.velocity.y;
						bullets1[bullet1Index].velocity.x = player1.velocity.x;
						shootBullet(bullets1, bullet1Index, MAX_BULLETS, player1);
						Mix_PlayChannel(-1, p1laser, 0);
						p1Ammo -= 1;
					}
					else if (event.key.keysym.scancode == SDL_SCANCODE_V && !p1reloading && p1Ammo > 0) {
						player1.sprite = SheetSprite(sprites, 264.0f / 512.0f, 66.0f / 256.0f, 64.0f / 512.0f, 64.0f / 256.0f, 1.0f);
						bullets1[bullet1Index].acceleration.x = -9;
						bullets1[bullet1Index].velocity.y = player1.velocity.y;
						bullets1[bullet1Index].velocity.x = player1.velocity.x;
						shootBullet(bullets1, bullet1Index, MAX_BULLETS, player1);
						Mix_PlayChannel(-1, p1laser, 0);
						p1Ammo -= 1;
					}
					//player2 shooting
					if (event.key.keysym.scancode == SDL_SCANCODE_KP_8 && !p2reloading && p2Ammo > 0) {
						player2.sprite = SheetSprite(sprites, 132.0f / 512.0f, 132.0f / 256.0f, 64.0f / 512.0f, 64.0f / 256.0f, 1.0f);
						bullets2[bullet2Index].acceleration.y = 9;
						bullets2[bullet2Index].velocity.y = player2.velocity.y;
						bullets2[bullet2Index].velocity.x = player2.velocity.x;
						shootBullet(bullets2, bullet2Index, MAX_BULLETS, player2);
						Mix_PlayChannel(-1, p2laser, 0);
						p2Ammo -= 1;
					}
					else if (event.key.keysym.scancode == SDL_SCANCODE_KP_5 && !p2reloading && p2Ammo > 0) {
						player2.sprite = SheetSprite(sprites, 262.0f / 512.0f, 0.0f / 256.0f, 64.0f / 512.0f, 64.0f / 256.0f, 1.0f);
						bullets2[bullet2Index].acceleration.y = -9;
						bullets2[bullet2Index].velocity.y = player2.velocity.y;
						bullets2[bullet2Index].velocity.x = player2.velocity.x;
						shootBullet(bullets2, bullet2Index, MAX_BULLETS, player2);
						Mix_PlayChannel(-1, p2laser, 0);
						p2Ammo -= 1;
					}
					else if (event.key.keysym.scancode == SDL_SCANCODE_KP_6 && !p2reloading && p2Ammo > 0) {
						player2.sprite = SheetSprite(sprites, 66.0f / 512.0f, 130.0f / 256.0f, 64.0f / 512.0f, 64.0f / 256.0f, 1.0f);
						bullets2[bullet2Index].acceleration.x = 9;
						bullets2[bullet2Index].velocity.y = player2.velocity.y;
						bullets2[bullet2Index].velocity.x = player2.velocity.x;
						shootBullet(bullets2, bullet2Index, MAX_BULLETS, player2);
						Mix_PlayChannel(-1, p2laser, 0);
						p2Ammo -= 1;
					}
					else if (event.key.keysym.scancode == SDL_SCANCODE_KP_4 && !p2reloading && p2Ammo > 0) {
						player2.sprite = SheetSprite(sprites, 198.0f / 512.0f, 66.0f / 256.0f, 64.0f / 512.0f, 64.0f / 256.0f, 1.0f);
						bullets2[bullet2Index].acceleration.x = -9;
						bullets2[bullet2Index].velocity.y = player2.velocity.y;
						bullets2[bullet2Index].velocity.x = player2.velocity.x;
						shootBullet(bullets2, bullet2Index, MAX_BULLETS, player2);
						Mix_PlayChannel(-1, p2laser, 0);
						p2Ammo -= 1;
					}
				}
			}
		break;
		case STATE_GAME_OVER:
			while (SDL_PollEvent(&event)) {
				if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
					done = true;
				}
				else if (event.type == SDL_KEYDOWN) {
					if (event.key.keysym.scancode == SDL_SCANCODE_RETURN) {
						mode = STATE_MAIN_MENU;
						Mix_PlayChannel(-1, select, 0);
						Mix_HaltMusic();
						Mix_PlayMusic(main_menu, -1);
					}
				}
			}
		break;
		}

		//Update
		const Uint8 *keys = SDL_GetKeyboardState(NULL);
		float ticks = (float)SDL_GetTicks() / 1000.0f;
		float elapsed = ticks - lastFrameTicks;
		lastFrameTicks = ticks;
		elapsed += accumulator;
		if(keys[SDL_SCANCODE_ESCAPE]){
			done = true;
		}
		
		if (elapsed < FIXED_TIMESTEP) {
			accumulator = elapsed;
			continue;
			}
		while (elapsed >= FIXED_TIMESTEP) {
			float angle = 0;
			angle += FIXED_TIMESTEP;

			screenShakeValue += FIXED_TIMESTEP;
			if (screenShakeTime > 0) {
				screenShakeTime -= FIXED_TIMESTEP;
			}

			//player 1 movement/reloading
			if (keys[SDL_SCANCODE_D]) {
				player1.acceleration.x = 2;
			}
			else if (keys[SDL_SCANCODE_A]) {
				player1.acceleration.x = -2;
			}
			else {
				player1.acceleration.x = 0;
			}
			if (keys[SDL_SCANCODE_W]) {
				player1.acceleration.y = 2;
			}
			else if (keys[SDL_SCANCODE_S]) {
				player1.acceleration.y = -2;
			}
			else {
				player1.acceleration.y = 0;
			}
			if (p1boostused) {
				p1boosttimer -= FIXED_TIMESTEP;
			}
			if (p1boosttimer <= 0 && p1boostused) {
				p1boostused = false;
			}
			if ((p1Ammo == 0 || keys[SDL_SCANCODE_R])&& !p1reloading) {
				p1reloading = true;
				p1reloadtimer = 1.5;
			}
			if (p1reloading) {
				p1reloadtimer -= FIXED_TIMESTEP;
			}
			if (p1reloadtimer <= 0 && p1reloading) {
				for (int i = 0; i < MAX_BULLETS; i++) {
					bullets1[i].acceleration.x = 0;
					bullets1[i].acceleration.y = 0;
				}
				p1reloading = false;
				p1Ammo = 20;
			}

			//player 2 movement/reloading
			if (keys[SDL_SCANCODE_RIGHT]) {
				player2.acceleration.x = 2;
			}
			else if (keys[SDL_SCANCODE_LEFT]) {
				player2.acceleration.x = -2;
			}
			else {
				player2.acceleration.x = 0;
			}
			if (keys[SDL_SCANCODE_UP]) {
				player2.acceleration.y = 2;
			}
			else if (keys[SDL_SCANCODE_DOWN]) {
				player2.acceleration.y = -2;
			}
			else {
				player2.acceleration.y = 0;
			}
			if (p2boostused) {
				p2boosttimer -= FIXED_TIMESTEP;
			}
			if (p2boosttimer <= 0 && p2boostused) {
				p2boostused = false;
			}
			if ((p2Ammo == 0 || keys[SDL_SCANCODE_KP_0]) && !p2reloading) {
				p2reloading = true;
				p2reloadtimer = 1.5;
			}
			if (p2reloading) {
				p2reloadtimer -= FIXED_TIMESTEP;
			}
			if (p2reloadtimer <= 0 && p2reloading) {
				for (int i = 0; i < MAX_BULLETS; i++) {
					bullets2[i].acceleration.x = 0;
					bullets2[i].acceleration.y = 0;
				}
				p2reloading = false;
				p2Ammo = 20;
			}

			switch (mode) {
			case STATE_MAIN_MENU:
				p1HP = 8;
				p1Ammo = 20;
				p1reloading = false;
				p1reloadtimer = 0;
				p1boostused = false;
				p1boosttimer = 0;
				p2HP = 8;
				p2Ammo = 20;
				p2reloading = false;
				p2reloadtimer = 0;
				p2boostused = false;
				p2boosttimer = 0;
				player2.position = Vector3(2.5, 0, 0);
				player1.position = Vector3(-2.5, 0, 0);
				player2.velocity = Vector3(0, 0, 0);
				player1.velocity = Vector3(0, 0, 0);
				for (int i = 0; i < MAX_BULLETS; i++) {
					bullets1[i].position = Vector3(-2000, 0, 0);
					bullets1[i].velocity = Vector3(0, 0, 0);
					bullets1[i].acceleration = Vector3(0, 0, 0);
					bullets2[i].position = Vector3(-2000, 0, 0);
					bullets2[i].velocity = Vector3(0, 0, 0);
					bullets2[i].acceleration = Vector3(0, 0, 0);
				}
				p1_win_count = 0;
				p2_win_count = 0;
			break;
			case STATE_GAME_LEVEL_1:
				if (viewMatrixPosition.x <= 0) {
					//player1 dash
					if (keys[SDL_SCANCODE_LSHIFT] && !p1boostused) {
						player1.acceleration.x *= 100;
						player1.acceleration.y *= 100;
						p1boostused = true;
						p1boosttimer = 4;
					}
					player1.Update(FIXED_TIMESTEP);
					for (int i = 0; i < 4; i++) {
						collision(&player1, &lvl_1_Objects[i]);
					}
					//player 1 bullet collisions
					for (int i = 0; i < MAX_BULLETS; i++) {
						bullets1[i].Update(FIXED_TIMESTEP);
						for (int j = 0; j < 4; j++) {
							if (entitiesCollided(bullets1[i], lvl_1_Objects[j])) {
								bullets1[i].position.x = -200;
								bullets1[i].position.y = -200;
							}
							if (entitiesCollided(bullets1[i], player2)) {
								Mix_PlayChannel(-1, hurt, 0);
								screenShakeTime = 1;
								p2HP -= 1;
								bullets1[i].position.x = -200;
								bullets1[i].position.y = -200;
							}
						}
					}
					//player2 dash
					if (keys[SDL_SCANCODE_KP_ENTER] && !p2boostused) {
						player2.acceleration.x *= 100;
						player2.acceleration.y *= 100;
						p2boostused = true;
						p2boosttimer = 4;
					}
					player2.Update(FIXED_TIMESTEP);
					for (int i = 0; i < 4; i++) {
						collision(&player2, &lvl_1_Objects[i]);
					}
					//player 2 bullet collisions
					for (int i = 0; i < MAX_BULLETS; i++) {
						bullets2[i].Update(FIXED_TIMESTEP);
						for (int j = 0; j < 4; j++) {
							if (entitiesCollided(bullets2[i], lvl_1_Objects[j])) {
								bullets2[i].position.x = -200;
								bullets2[i].position.y = -200;
							}
							if (entitiesCollided(bullets2[i], player1)) {
								Mix_PlayChannel(-1, hurt, 0);
								p1HP -= 1;
								screenShakeTime = 1;
								bullets2[i].position.x = -200;
								bullets2[i].position.y = -200;
							}
						}
					}
					if (entitiesCollided(player1, player2)) {
						player1.velocity.x *= -1;
						player1.velocity.y *= -1;
						player2.velocity.x *= -1;
						player2.velocity.y *= -1;
					}
					collision(&player1, &player2);
					//win conditions
					if (!entitiesCollided(player1, floor)) {
						player1.position.z = -10;
						p2_win_count = 1;
					}
					if (!entitiesCollided(player2, floor)) {
						player2.position.z = -10;
						p1_win_count = 1;
					}
					if (p1HP == 0) {
						p2_win_count = 1;
					}
					if (p2HP == 0) {
						p1_win_count = 1;
					}
					if (p1_win_count == 1 || p2_win_count == 1) {
						Mix_PlayChannel(-1, death, 0);
						SDL_Delay(1000);
						Mix_HaltMusic();
						Mix_PlayMusic(lvl2, -1);
						viewMatrixPosition.x = 10;
						mode = STATE_GAME_LEVEL_2;
						p1HP = 8;
						p1Ammo = 20;
						p1reloading = false;
						p1reloadtimer = 0;
						p1boostused = false;
						p1boosttimer = 0;
						p2HP = 8;
						p2Ammo = 20;
						p2reloading = false;
						p2reloadtimer = 0;
						p2boostused = false;
						p2boosttimer = 0;
						player2.position = Vector3(2.5, 0, 0);
						player1.position = Vector3(-2.5, 0, 0);
						player2.velocity = Vector3(0, 0, 0);
						player1.velocity = Vector3(0, 0, 0);
						for (int i = 0; i < MAX_BULLETS; i++) {
							bullets1[i].position = Vector3(-2000, 0, 0);
							bullets1[i].velocity = Vector3(0, 0, 0);
							bullets1[i].acceleration = Vector3(0, 0, 0);
							bullets2[i].position = Vector3(-2000, 0, 0);
							bullets2[i].velocity = Vector3(0, 0, 0);
							bullets2[i].acceleration = Vector3(0, 0, 0);
						}
					}
				}
				else {
					viewMatrixPosition.x -= FIXED_TIMESTEP*3;
				}
			break;
			case STATE_GAME_LEVEL_2:
				if (viewMatrixPosition.x <= 0) {
					//player1 dash
					if (keys[SDL_SCANCODE_LSHIFT] && !p1boostused) {
						player1.acceleration.x *= 100;
						player1.acceleration.y *= 100;
						p1boostused = true;
						p1boosttimer = 4;
					}
					player1.Update(FIXED_TIMESTEP);
					for (int i = 0; i < 2; i++) {
						collision(&player1, &lvl_2_Objects[i]);
					}
					//player 1 bullet collisions
					for (int i = 0; i < MAX_BULLETS; i++) {
						bullets1[i].Update(FIXED_TIMESTEP);
						for (int j = 0; j < 4; j++) {
							if (entitiesCollided(bullets1[i], lvl_2_Objects[j])) {
								bullets1[i].position.x = -200;
								bullets1[i].position.y = -200;
							}
							if (entitiesCollided(bullets1[i], player2)) {
								Mix_PlayChannel(-1, hurt, 0);
								screenShakeTime = 1;
								p2HP -= 1;
								bullets1[i].position.x = -200;
								bullets1[i].position.y = -200;
							}
						}
					}
					//player2 dash
					if (keys[SDL_SCANCODE_KP_ENTER] && !p2boostused) {
						player2.acceleration.x *= 100;
						player2.acceleration.y *= 100;
						p2boostused = true;
						p2boosttimer = 4;
					}
					player2.Update(FIXED_TIMESTEP);
					for (int i = 0; i < 4; i++) {
						collision(&player2, &lvl_2_Objects[i]);
					}
					//player 2 bullet collisions
					for (int i = 0; i < MAX_BULLETS; i++) {
						bullets2[i].Update(FIXED_TIMESTEP);
						for (int j = 0; j < 4; j++) {
							if (entitiesCollided(bullets2[i], lvl_2_Objects[j])) {
								bullets2[i].position.x = -200;
								bullets2[i].position.y = -200;
							}
							if (entitiesCollided(bullets2[i], player1)) {
								Mix_PlayChannel(-1, hurt, 0);
								screenShakeTime = 1;
								p1HP -= 1;
								bullets2[i].position.x = -200;
								bullets2[i].position.y = -200;
							}
						}
					}
					if (entitiesCollided(player1, player2)) {
						player1.velocity.x *= -1;
						player1.velocity.y *= -1;
						player2.velocity.x *= -1;
						player2.velocity.y *= -1;
					}
					collision(&player1, &player2);
					//win conditions
					if (p1_win_count == 0) {
						if (!entitiesCollided(player2, floor)) {
							player2.position.z = -10;
							p1_win_count = 1;
						}
						if (p2HP == 0) {
							p1_win_count = 1;
						}
					}
					else if (p1_win_count == 1) {
						if (!entitiesCollided(player2, floor)) {
							player2.position.z = -10;
							p1_win_count = 2;
						}
						if (p2HP == 0) {
							p1_win_count = 2;
						}
					}
					if (p2_win_count == 0) {
						if (!entitiesCollided(player1, floor)) {
							player1.position.z = -10;
							p2_win_count = 1;
						}
						if (p1HP == 0) {
							p2_win_count = 1;
						}
					}
					else if (p2_win_count == 1) {
						if (!entitiesCollided(player1, floor)) {
							player1.position.z = -10;
							p2_win_count = 2;
						}
						if (p1HP == 0) {
							p2_win_count = 2;
						}
					}
					if (p1_win_count == 1 && p2_win_count == 1) {
						Mix_PlayChannel(-1, death, 0);
						Mix_HaltMusic();
						Mix_PlayMusic(lvl3, -1);
						SDL_Delay(1000);
						viewMatrixPosition.x = 10;
						mode = STATE_GAME_LEVEL_3;
						p1HP = 8;
						p1Ammo = 20;
						p1reloading = false;
						p1reloadtimer = 0;
						p1boostused = false;
						p1boosttimer = 0;
						p2HP = 8;
						p2Ammo = 20;
						p2reloading = false;
						p2reloadtimer = 0;
						p2boostused = false;
						p2boosttimer = 0;
						player2.position = Vector3(2.5, 0, 0);
						player1.position = Vector3(-2.5, 0, 0);
						player2.velocity = Vector3(0, 0, 0);
						player1.velocity = Vector3(0, 0, 0);
						for (int i = 0; i < MAX_BULLETS; i++) {
							bullets1[i].position = Vector3(-2000, 0, 0);
							bullets1[i].velocity = Vector3(0, 0, 0);
							bullets1[i].acceleration = Vector3(0, 0, 0);
							bullets2[i].position = Vector3(-2000, 0, 0);
							bullets2[i].velocity = Vector3(0, 0, 0);
							bullets2[i].acceleration = Vector3(0, 0, 0);
						}
					}
					if (p1_win_count == 2 || p2_win_count == 2) {
						Mix_PlayChannel(-1, death, 0);
						Mix_HaltMusic();
						Mix_PlayMusic(results, -1);
						SDL_Delay(1000);
						//viewMatrixPosition.x = 8;
						mode = STATE_GAME_OVER;
						p1HP = 8;
						p1Ammo = 20;
						p1reloading = false;
						p1reloadtimer = 0;
						p1boostused = false;
						p1boosttimer = 0;
						p2HP = 8;
						p2Ammo = 20;
						p2reloading = false;
						p2reloadtimer = 0;
						p2boostused = false;
						p2boosttimer = 0;
						player2.position = Vector3(2.5, 0, 0);
						player1.position = Vector3(-2.5, 0, 0);
						player2.velocity = Vector3(0, 0, 0);
						player1.velocity = Vector3(0, 0, 0);
						for (int i = 0; i < MAX_BULLETS; i++) {
							bullets1[i].position = Vector3(-2000, 0, 0);
							bullets1[i].velocity = Vector3(0, 0, 0);
							bullets1[i].acceleration = Vector3(0, 0, 0);
							bullets2[i].position = Vector3(-2000, 0, 0);
							bullets2[i].velocity = Vector3(0, 0, 0);
							bullets2[i].acceleration = Vector3(0, 0, 0);
						}
					}
				}
				else {
					viewMatrixPosition.x -= FIXED_TIMESTEP*3;
				}
			break;
			case STATE_GAME_LEVEL_3:
				if (viewMatrixPosition.x <= 0) {
					//player1 dash
					if (keys[SDL_SCANCODE_LSHIFT] && !p1boostused) {
						player1.acceleration.x *= 100;
						player1.acceleration.y *= 100;
						p1boostused = true;
						p1boosttimer = 4;
					}
					player1.Update(FIXED_TIMESTEP);
					for (int i = 0; i < 4; i++) {
						if (entitiesCollided(player1, lvl_3_Objects[i])) {
							p2_win_count = 2;
						};
					}
					//player 1 bullet collisions
					for (int i = 0; i < MAX_BULLETS; i++) {
						bullets1[i].Update(FIXED_TIMESTEP);
						for (int j = 0; j < 4; j++) {
							if (entitiesCollided(bullets1[i], player2)) {
								Mix_PlayChannel(-1, hurt, 0);
								screenShakeTime = 1;
								p2HP -= 1;
								bullets1[i].position.x = -200;
								bullets1[i].position.y = -200;
							}
						}
					}
					//player2 dash
					if (keys[SDL_SCANCODE_KP_ENTER] && !p2boostused) {
						player2.acceleration.x *= 100;
						player2.acceleration.y *= 100;
						p2boostused = true;
						p2boosttimer = 4;
					}
					player2.Update(FIXED_TIMESTEP);
					for (int i = 0; i < 4; i++) {
						if (entitiesCollided(player2, lvl_3_Objects[i])) {
							p1_win_count = 2;
						};
					}
					//player 2 bullet collisions
					for (int i = 0; i < MAX_BULLETS; i++) {
						bullets2[i].Update(FIXED_TIMESTEP);
						for (int j = 0; j < 4; j++) {
							if (entitiesCollided(bullets2[i], player1)) {
								Mix_PlayChannel(-1, hurt, 0);
								screenShakeTime = 1;
								p1HP -= 1;
								bullets2[i].position.x = -200;
								bullets2[i].position.y = -200;
							}
						}
					}
					if (entitiesCollided(player1, player2)) {
						player1.velocity.x *= -1;
						player1.velocity.y *= -1;
						player2.velocity.x *= -1;
						player2.velocity.y *= -1;
					}
					collision(&player1, &player2);
					//win conditions
					if (!entitiesCollided(player1, floor)) {
						player1.position.z = -10;
						p2_win_count = 2;
					}
					if (!entitiesCollided(player2, floor)) {
						player2.position.z = -10;
						p1_win_count = 2;
					}
					if (p1HP == 0) {
						p2_win_count = 2;
					}
					if (p2HP == 0) {
						p1_win_count = 2;
					}
					if (p1_win_count == 2 || p2_win_count == 2) {
						Mix_PlayChannel(-1, death, 0);
						Mix_HaltMusic();
						Mix_PlayMusic(results, -1);
						SDL_Delay(1000);
						//viewMatrixPosition.x = 8;
						mode = STATE_GAME_OVER;
						p1HP = 8;
						p1Ammo = 20;
						p1reloading = false;
						p1reloadtimer = 0;
						p1boostused = false;
						p1boosttimer = 0;
						p2HP = 8;
						p2Ammo = 20;
						p2reloading = false;
						p2reloadtimer = 0;
						p2boostused = false;
						p2boosttimer = 0;
						player2.position = Vector3(2.5, 0, 0);
						player1.position = Vector3(-2.5, 0, 0);
						player2.velocity = Vector3(0, 0, 0);
						player1.velocity = Vector3(0, 0, 0);
						for (int i = 0; i < MAX_BULLETS; i++) {
							bullets1[i].position = Vector3(-2000, 0, 0);
							bullets1[i].velocity = Vector3(0, 0, 0);
							bullets1[i].acceleration = Vector3(0, 0, 0);
							bullets2[i].position = Vector3(-2000, 0, 0);
							bullets2[i].velocity = Vector3(0, 0, 0);
							bullets2[i].acceleration = Vector3(0, 0, 0);
						}
					}
				}
				else {
					viewMatrixPosition.x -= FIXED_TIMESTEP*3;
				}
			break;
			case STATE_GAME_OVER:
				if (viewMatrixPosition.x > 0) {
					viewMatrixPosition.x -= FIXED_TIMESTEP;
				}
				p1HP = 8;
				p1Ammo = 20;
				p1reloading = false;
				p1reloadtimer = 0;
				p1boostused = false;
				p1boosttimer = 0;
				p2HP = 8;
				p2Ammo = 20;
				p2reloading = false;
				p2reloadtimer = 0;
				p2boostused = false;
				p2boosttimer = 0;
				player2.position = Vector3(2.5, 0, 0);
				player1.position = Vector3(-2.5, 0, 0);
				player2.velocity = Vector3(0, 0, 0);
				player1.velocity = Vector3(0, 0, 0);
				for (int i = 0; i < MAX_BULLETS; i++) {
					bullets1[i].position = Vector3(-2000, 0, 0);
					bullets1[i].velocity = Vector3(0, 0, 0);
					bullets1[i].acceleration = Vector3(0, 0, 0);
					bullets2[i].position = Vector3(-2000, 0, 0);
					bullets2[i].velocity = Vector3(0, 0, 0);
					bullets2[i].acceleration = Vector3(0, 0, 0);
				}
			break;
			}
			elapsed -= FIXED_TIMESTEP;
		}
		accumulator = elapsed;
		
		//Rendering
		glClear(GL_COLOR_BUFFER_BIT);
		program.SetProjectionMatrix(projectionMatrix);
		program.SetViewMatrix(viewMatrix);

		if (screenShakeTime > 0) {
			viewMatrix.SetPosition(viewMatrixPosition.x, sin(screenShakeValue * 50)*.01, 0);
		}
		else {
			viewMatrix.SetPosition(viewMatrixPosition.x, 0, 0);
		}
		program.SetModelMatrix(backgroundmodel);
		backgroundmodel.SetScale(35, 4, 1);
		float vertices[] = { -0.5, -0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5, 0.5, 0.5, -0.5, 0.5 };
		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
		glEnableVertexAttribArray(program.positionAttribute);
		float texCoords[] = { 0.0, 1.0, 6.0, 1.0, 6.0, 0.0, 0.0, 1.0, 6.0, 0.0, 0.0, 0.0 };
		glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
		glEnableVertexAttribArray(program.texCoordAttribute);
		glBindTexture(GL_TEXTURE_2D, background);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glDisableVertexAttribArray(program.positionAttribute);
		glDisableVertexAttribArray(program.texCoordAttribute);
		
		switch (mode) {
		case STATE_MAIN_MENU:
			program.SetModelMatrix(textModel);
			textModel.SetPosition(-3.29, .8, 0);
			DrawText(&program, text, "Press Enter to Start", .07, 0);
			program.SetModelMatrix(textModel);
			textModel.SetPosition(-0.65, -0.5, 0);
			DrawText(&program, text, "CS3113 FINAL PROJECT", .35, 0);
		break;
		case STATE_GAME_LEVEL_1:
			if (viewMatrixPosition.x <= 7.9) {
				floor.Draw(&program);
				for (int i = 0; i < 4; i++) {
					lvl_1_Objects[i].Draw(&program);
				}
				for (int i = 0; i < MAX_BULLETS; i++) {
					bullets1[i].Draw(&program);
				}
				for (int i = 0; i < MAX_BULLETS; i++) {
					bullets2[i].Draw(&program);
				}
				player1.Draw(&program);
				player2.Draw(&program);
				program.SetModelMatrix(textModel);
				textModel.SetPosition(-3.4, 1.9, 0);
				DrawText(&program, text, "P2:", .07, 0);
				program.SetModelMatrix(textModel);
				textModel.SetPosition(-3.1, 1.9, 0);
				DrawText(&program, text, "P1:", .07, 0);
				program.SetModelMatrix(textModel);
				textModel.SetPosition(-2.9, 1.9, 0);
				DrawText(&program, text, "HP:", .07, 0);
				program.SetModelMatrix(textModel);
				textModel.SetPosition(-2.75, 1.9, 0);
				DrawText(&program, text, std::to_string(p1HP), .07, 0);
				program.SetModelMatrix(textModel);
				textModel.SetPosition(-2.4, 1.9, 0);
				if (!p1reloading) {
					DrawText(&program, text, "AMMO:", .07, 0);
				}
				else {
					DrawText(&program, text, "RELOADN", .07, 0);
				}
				program.SetModelMatrix(textModel);
				textModel.SetPosition(-2.2, 1.9, 0);
				if (!p1reloading) {
					DrawText(&program, text, std::to_string(p1Ammo), .07, 0);
				}
				program.SetModelMatrix(textModel);
				textModel.SetPosition(3.3, 1.9, 0);
				if (!p1boostused) {
					DrawText(&program, text, "DASH", .07, 0);
				}
				program.SetModelMatrix(textModel);
				textModel.SetPosition(3.1, 1.9, 0);
				if (!p2boostused) {
					DrawText(&program, text, "DASH", .07, 0);
				}
				program.SetModelMatrix(textModel);
				textModel.SetPosition(2.75, 1.9, 0);
				if (!p2reloading) {
					DrawText(&program, text, std::to_string(p2Ammo), .07, 0);
				}
				program.SetModelMatrix(textModel);
				textModel.SetPosition(2.6, 1.9, 0);
				if (!p2reloading) {
					DrawText(&program, text, "AMMO:", .07, 0);
				}
				else {
					DrawText(&program, text, "RELOADN", .07, 0);
				}
				program.SetModelMatrix(textModel);
				textModel.SetPosition(2.4, 1.9, 0);
				DrawText(&program, text, std::to_string(p2HP), .07, 0);
				program.SetModelMatrix(textModel);
				textModel.SetPosition(2.1, 1.9, 0);
				DrawText(&program, text, "HP:", .07, 0);
			}
		break;
		case STATE_GAME_LEVEL_2:
			if (viewMatrixPosition.x <= 7.9) {
				floor.Draw(&program);
				for (int i = 0; i < 2; i++) {
					lvl_2_Objects[i].Draw(&program);
				}
				for (int i = 0; i < MAX_BULLETS; i++) {
					bullets1[i].Draw(&program);
				}
				for (int i = 0; i < MAX_BULLETS; i++) {
					bullets2[i].Draw(&program);
				}
				player1.Draw(&program);
				player2.Draw(&program);
				program.SetModelMatrix(textModel);
				textModel.SetPosition(-3.4, 1.9, 0);
				DrawText(&program, text, "P2:", .07, 0);
				program.SetModelMatrix(textModel);
				textModel.SetPosition(-3.1, 1.9, 0);
				DrawText(&program, text, "P1:", .07, 0);
				program.SetModelMatrix(textModel);
				textModel.SetPosition(-2.9, 1.9, 0);
				DrawText(&program, text, "HP:", .07, 0);
				program.SetModelMatrix(textModel);
				textModel.SetPosition(-2.75, 1.9, 0);
				DrawText(&program, text, std::to_string(p1HP), .07, 0);
				program.SetModelMatrix(textModel);
				textModel.SetPosition(-2.4, 1.9, 0);
				if (!p1reloading) {
					DrawText(&program, text, "AMMO:", .07, 0);
				}
				else {
					DrawText(&program, text, "RELOADN", .07, 0);
				}
				program.SetModelMatrix(textModel);
				textModel.SetPosition(-2.2, 1.9, 0);
				if (!p1reloading) {
					DrawText(&program, text, std::to_string(p1Ammo), .07, 0);
				}
				program.SetModelMatrix(textModel);
				textModel.SetPosition(3.3, 1.9, 0);
				if (!p1boostused) {
					DrawText(&program, text, "DASH", .07, 0);
				}
				program.SetModelMatrix(textModel);
				textModel.SetPosition(3.1, 1.9, 0);
				if (!p2boostused) {
					DrawText(&program, text, "DASH", .07, 0);
				}
				program.SetModelMatrix(textModel);
				textModel.SetPosition(2.75, 1.9, 0);
				if (!p2reloading) {
					DrawText(&program, text, std::to_string(p2Ammo), .07, 0);
				}
				program.SetModelMatrix(textModel);
				textModel.SetPosition(2.6, 1.9, 0);
				if (!p2reloading) {
					DrawText(&program, text, "AMMO:", .07, 0);
				}
				else {
					DrawText(&program, text, "RELOADN", .07, 0);
				}
				program.SetModelMatrix(textModel);
				textModel.SetPosition(2.4, 1.9, 0);
				DrawText(&program, text, std::to_string(p2HP), .07, 0);
				program.SetModelMatrix(textModel);
				textModel.SetPosition(2.1, 1.9, 0);
				DrawText(&program, text, "HP:", .07, 0);
			}
		break;
		case STATE_GAME_LEVEL_3:
			if (viewMatrixPosition.x <= 7.9) {
				floor.Draw(&program);
				for (int i = 0; i < 1; i++) {
					lvl_3_Objects[i].Draw(&program);
				}
				for (int i = 0; i < MAX_BULLETS; i++) {
					bullets1[i].Draw(&program);
				}
				for (int i = 0; i < MAX_BULLETS; i++) {
					bullets2[i].Draw(&program);
				}
				player1.Draw(&program);
				player2.Draw(&program);
				program.SetModelMatrix(textModel);
				textModel.SetPosition(-3.4, 1.9, 0);
				DrawText(&program, text, "P2:", .07, 0);
				program.SetModelMatrix(textModel);
				textModel.SetPosition(-3.1, 1.9, 0);
				DrawText(&program, text, "P1:", .07, 0);
				program.SetModelMatrix(textModel);
				textModel.SetPosition(-2.9, 1.9, 0);
				DrawText(&program, text, "HP:", .07, 0);
				program.SetModelMatrix(textModel);
				textModel.SetPosition(-2.75, 1.9, 0);
				DrawText(&program, text, std::to_string(p1HP), .07, 0);
				program.SetModelMatrix(textModel);
				textModel.SetPosition(-2.4, 1.9, 0);
				if (!p1reloading) {
					DrawText(&program, text, "AMMO:", .07, 0);
				}
				else {
					DrawText(&program, text, "RELOADN", .07, 0);
				}
				program.SetModelMatrix(textModel);
				textModel.SetPosition(-2.2, 1.9, 0);
				if (!p1reloading) {
					DrawText(&program, text, std::to_string(p1Ammo), .07, 0);
				}
				program.SetModelMatrix(textModel);
				textModel.SetPosition(3.3, 1.9, 0);
				if (!p1boostused) {
					DrawText(&program, text, "DASH", .07, 0);
				}
				program.SetModelMatrix(textModel);
				textModel.SetPosition(3.1, 1.9, 0);
				if (!p2boostused) {
					DrawText(&program, text, "DASH", .07, 0);
				}
				program.SetModelMatrix(textModel);
				textModel.SetPosition(2.75, 1.9, 0);
				if (!p2reloading) {
					DrawText(&program, text, std::to_string(p2Ammo), .07, 0);
				}
				program.SetModelMatrix(textModel);
				textModel.SetPosition(2.6, 1.9, 0);
				if (!p2reloading) {
					DrawText(&program, text, "AMMO:", .07, 0);
				}
				else {
					DrawText(&program, text, "RELOADN", .07, 0);
				}
				program.SetModelMatrix(textModel);
				textModel.SetPosition(2.4, 1.9, 0);
				DrawText(&program, text, std::to_string(p2HP), .07, 0);
				program.SetModelMatrix(textModel);
				textModel.SetPosition(2.1, 1.9, 0);
				DrawText(&program, text, "HP:", .07, 0);
			}
		break;
		case STATE_GAME_OVER:
			program.SetModelMatrix(textModel);
			textModel.SetPosition(-1, 0, 0);
			DrawText(&program, text, "Press Enter to Return", .07, 0);
			program.SetModelMatrix(textModel);
			textModel.SetPosition(-0.65, -0.5, 0);
			if (p1_win_count == 2) {
				DrawText(&program, text, "PLAYER 1 WINS!!!", .15, 0);
			}
			else if (p2_win_count == 2) {
				DrawText(&program, text, "PLAYER 2 WINS!!!", .15, 0);
			}
		break;
		}
		SDL_GL_SwapWindow(displayWindow);
	}
	
	SDL_JoystickClose(playerOneController);
	SDL_JoystickClose(playerTwoController);
	SDL_Quit();
	return 0;
}
