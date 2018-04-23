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
	Vector3() {};
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

class Entity {
public:
	Entity() {};
	void Draw() {};
	void Update(float elapsed) {};
	Vector3 position;
	Vector3 velocity;
	Vector3 size;
	float rotation;
	SheetSprite sprite;
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
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
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

void shootBullet(Entity bullets[], int &index, int max, Entity character) {
	bullets[index].position.x = character.position.x;
	bullets[index].position.y = character.position.y;
	index++;
	if (index > max - 1) {
		index = 0;
	}
}

//Space Invader Specific funcs
void enemyMovementArray(Entity enemy[], float elapsed) {
	float offset = 5.5f;
	for (int i = 0; i < 11; i++) {
		enemy[i].position.y += enemy[i].velocity.y*elapsed;
		if (enemy[i].position.x >= (3.9 - offset)) {
			enemy[i].velocity.x *= -1;
		}
		if (enemy[i].velocity.x < 0) {
			if (enemy[i].position.x <= (2.1 - offset)) {
				enemy[i].velocity.x *= -1;
			}
		}
		enemy[i].position.x += enemy[i].velocity.x*elapsed;
		offset -= 0.5f;
	}
}

int main(int argc, char *argv[])
{
	//Setup
	SDL_Window* displayWindow;
	SDL_Init(SDL_INIT_VIDEO);
	displayWindow = SDL_CreateWindow("Space Invaders", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640*1.5, 360*1.5, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);
	#ifdef _WINDOWS
		glewInit();
	#endif

	Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glViewport(0, 0, 640*1.5, 360*1.5);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	ShaderProgram program;
	program.Load(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");
	glUseProgram(program.programID);
	Matrix viewMatrix;
	Matrix projectionMatrix;
	projectionMatrix.SetOrthoProjection(-3.55, 3.55, -2.0f, 2.0f, -1.0f, 1.0f);
	float lastFrameTicks = 0.0f;
	// 60 FPS (1.0f/60.0f) (update sixty times a second)
	#define FIXED_TIMESTEP 0.0166666f
	#define MAX_TIMESTEPS 6
	float accumulator = 0.0f;
	SDL_Event event;
	bool done = false;

	enum GameMode { STATE_MAIN_MENU, STATE_GAME_LEVEL, STATE_GAME_OVER };
	GameMode mode = STATE_MAIN_MENU;

	//Space Invader Objects, etc.
	Mix_Music *music;
	music = Mix_LoadMUS("BoxCat_Games_Epic_Song.mp3");
	Mix_Chunk *select;
	select = Mix_LoadWAV("select.wav");
	Mix_Chunk *playerlaser;
	playerlaser = Mix_LoadWAV("playerlaser.wav");
	Mix_Chunk *playerdestroyed;
	playerdestroyed = Mix_LoadWAV("playerdestroyed.wav");
	Mix_Chunk *enemylaser;
	enemylaser = Mix_LoadWAV("enemylaser.wav");
	Mix_Chunk *enemydestroyed;
	enemydestroyed = Mix_LoadWAV("enemydestroyed.wav");
	GLuint spritesheet = LoadTexture(RESOURCE_FOLDER"sheet.png");
	GLuint background = LoadTexture(RESOURCE_FOLDER"blue.png");
	GLuint fontTexture = LoadTexture(RESOURCE_FOLDER"font1.png");
	Matrix textModel;
	Matrix backgroundmodel;

	Matrix playerModel;
	Entity player;
	player.position = Vector3(0, -1.75, 0);
	player.velocity = Vector3(3, 0, 0);
	player.sprite = SheetSprite(spritesheet, 211.0f/1024.0f, 941.0f/1024.0f, 99.0f/1024.0f, 75.0f/1024.0f, 0.3);
	player.size = Vector3(0.3, 0.3, 0);

    #define MAX_BULLETS 30
	int bulletIndex = 0;
	Entity bullets[MAX_BULLETS];
	for (int i = 0; i < MAX_BULLETS; i++) {
		bullets[i].position = Vector3(-2000.0f, 0, 0);
		bullets[i].velocity = Vector3(0, 4, 0);
		bullets[i].sprite = SheetSprite(spritesheet, 856.0f/1024.0f, 421.0f/1024.0f, 9.0f/1024.0f, 54.0f/1024.0f, 0.3);
		bullets[i].size = Vector3(0.15, 0.15, 0);
	}
	Matrix bulletModels[MAX_BULLETS];

	int enemyBulletIndex = 0;
	Entity enemyBullets[MAX_BULLETS];
	for (int i = 0; i < MAX_BULLETS; i++) {
		enemyBullets[i].position = Vector3(-2000.0f, 0, 0);
		enemyBullets[i].velocity = Vector3(0, -2, 0);
		enemyBullets[i].sprite = SheetSprite(spritesheet, 858.0f / 1024.0f, 230.0f / 1024.0f, 9.0f / 1024.0f, 54.0f / 1024.0f, 0.3);
		enemyBullets[i].size = Vector3(0.15, 0.15, 0);
	}
	Matrix enemyBulletModels[MAX_BULLETS];
	
	float enemy_x_pos = -3.3f;
	Matrix enemyModelsBlack[11];
	Entity enemiesBlack[11];
	for (int i = 0; i < 11; i++) {
		enemiesBlack[i].position = Vector3(enemy_x_pos, 1.75, 0);
		enemiesBlack[i].velocity = Vector3(.5, -0.03f, 0);
		enemiesBlack[i].sprite = SheetSprite(spritesheet, 423.0f / 1024.0f, 728.0f / 1024.0f, 93.0f / 1024.0f, 84.0f / 1024.0f, 0.3);
		enemiesBlack[i].size = Vector3(0.3, 0.3, 0);
		enemy_x_pos += 0.5f;
	}

	enemy_x_pos = -3.3f;
	Matrix enemyModelsBlue[11];
	Entity enemiesBlue[11];
	for (int i = 0; i < 11; i++) {
		enemiesBlue[i].position = Vector3(enemy_x_pos, 1.25, 0);
		enemiesBlue[i].velocity = Vector3(.5, -0.03f, 0);
		enemiesBlue[i].sprite = SheetSprite(spritesheet, 143.0f / 1024.0f, 293.0f / 1024.0f, 104.0f / 1024.0f, 84.0f / 1024.0f, 0.3); 
		enemiesBlue[i].size = Vector3(0.3, 0.3, 0);
		enemy_x_pos += 0.5f;
	}
	
	enemy_x_pos = -3.3f;
	Matrix enemyModelsGreen[11];
	Entity enemiesGreen[11];
	for (int i = 0; i < 11; i++) {
		enemiesGreen[i].position = Vector3(enemy_x_pos, 0.75, 0);
		enemiesGreen[i].velocity = Vector3(.5, -0.03f, 0);
		enemiesGreen[i].sprite = SheetSprite(spritesheet, 224.0f / 1024.0f, 496.0f / 1024.0f, 103.0f / 1024.0f, 84.0f / 1024.0f, 0.3);
		enemiesGreen[i].size = Vector3(0.3, 0.3, 0);
		enemy_x_pos += 0.5f;
	}
	
	enemy_x_pos = -3.3f;
	Matrix enemyModelsRed[11];
	Entity enemiesRed[11];
	for (int i = 0; i < 11; i++) {
		enemiesRed[i].position = Vector3(enemy_x_pos, 0.25, 0);
		enemiesRed[i].velocity = Vector3(.5, -0.03f, 0);
		enemiesRed[i].sprite = SheetSprite(spritesheet, 520.0f / 1024.0f, 577.0f / 1024.0f, 82.0f / 1024.0f, 84.0f / 1024.0f, 0.3);
		enemiesRed[i].size = Vector3(0.3, 0.3, 0);
		enemy_x_pos += 0.5f;
	}
	
	std::default_random_engine generator;
	std::uniform_int_distribution<int> distribution(0, 500);
	auto bulletRNG = std::bind(distribution, generator);

	int score = 0;
	int stage = 1;
	int enemiesDefeated = 0;
	Mix_PlayMusic(music, -1);
	Mix_VolumeChunk(playerlaser, 60);
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
						mode = STATE_GAME_LEVEL;
						Mix_PlayChannel(-1, select, 0);
					}
				}
			}
		break;
		case STATE_GAME_LEVEL:	
			while (SDL_PollEvent(&event)) {
				if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
					done = true;
				}
				else if (event.type == SDL_KEYDOWN && event.key.repeat == 0) {
					if (event.key.keysym.scancode == SDL_SCANCODE_SPACE) {
						shootBullet(bullets, bulletIndex, MAX_BULLETS, player);
						Mix_PlayChannel(-1, playerlaser, 0);
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
		
		if (elapsed < FIXED_TIMESTEP) {
			accumulator = elapsed;
			continue;
			}
		while (elapsed >= FIXED_TIMESTEP) {
			float angle = 0;
			angle += FIXED_TIMESTEP;

			switch (mode) {
			case STATE_MAIN_MENU:
				score = 0;
				stage = 1;
				enemiesDefeated = 0;
			break;
			case STATE_GAME_LEVEL:
				if (keys[SDL_SCANCODE_A]) {
					if (player.position.x >= -3.55 + player.size.x / 2) { player.position.x -= player.velocity.x*FIXED_TIMESTEP; }
				}
				else if (keys[SDL_SCANCODE_D]) {
					if (player.position.x <= 3.55 - player.size.x / 2) { player.position.x += player.velocity.x*FIXED_TIMESTEP; }
				}

				for (int i = 0; i < MAX_BULLETS; i++) {
					bullets[i].position.y += bullets[i].velocity.y*FIXED_TIMESTEP;
				}

				for (int i = 0; i < MAX_BULLETS; i++) {
					enemyBullets[i].position.y += enemyBullets[i].velocity.y*FIXED_TIMESTEP;
				}

				enemyMovementArray(enemiesBlack, FIXED_TIMESTEP);
				enemyMovementArray(enemiesBlue, FIXED_TIMESTEP);
				enemyMovementArray(enemiesGreen, FIXED_TIMESTEP);
				enemyMovementArray(enemiesRed, FIXED_TIMESTEP);

				for (int i = 0; i < MAX_BULLETS; i++) {
					for (int j = 0; j < 11; j++) {
						if (entitiesCollided(bullets[i], enemiesBlack[j])) {
							enemiesBlack[j].position.y = -200.0f;
							bullets[i].position.x = -200.0f;
							enemiesDefeated += 1;
							score += 100;
							Mix_PlayChannel(-1, enemydestroyed, 0);
						}
						if (entitiesCollided(bullets[i], enemiesBlue[j])) {
							enemiesBlue[j].position.y = -200.0f;
							bullets[i].position.x = -200.0f;
							enemiesDefeated += 1;
							score += 100;
							Mix_PlayChannel(-1, enemydestroyed, 0);
						}
						if (entitiesCollided(bullets[i], enemiesGreen[j])) {
							enemiesGreen[j].position.y = -200.0f;
							bullets[i].position.x = -200.0f;
							enemiesDefeated += 1;
							score += 100;
							Mix_PlayChannel(-1, enemydestroyed, 0);
						}
						if (entitiesCollided(bullets[i], enemiesRed[j])) {
							enemiesRed[j].position.y = -200.0f;
							bullets[i].position.x = -200.0f;
							enemiesDefeated += 1;
							score += 100;
							Mix_PlayChannel(-1, enemydestroyed, 0);
						}
					}
				}

				if (enemiesDefeated == 44) {
					enemiesDefeated = 0;
					stage += 1;
					for (int i = 0; i < 11; i++) {
						enemiesBlack[i].position.y = 1.75f;
						enemiesBlack[i].velocity.y *= 1.5;
						enemiesBlue[i].position.y = 1.25f;
						enemiesBlue[i].velocity.y *= 1.5;
						enemiesGreen[i].position.y = 0.75f;
						enemiesGreen[i].velocity.y *= 1.5;
						enemiesRed[i].position.y = 0.25f;
						enemiesRed[i].velocity.y *= 1.5;
					}
				}
				for (int i = 0; i < 11; i++) {
					if ((enemiesBlack[i].position.y <= -1.75 && enemiesBlack[i].position.y >= -2.0) ||
						(enemiesBlue[i].position.y <= -1.75 && enemiesBlue [i].position.y >= -2.0) ||
						(enemiesGreen[i].position.y <= -1.75 && enemiesGreen[i].position.y >= -2.0) ||
						(enemiesRed[i].position.y <= -1.75 && enemiesRed[i].position.y >= -2.0)) {
						mode = STATE_GAME_OVER;
					}
				}
				for (int i = 0; i < 11; i++) {
					if (bulletRNG() == 95) {
						shootBullet(enemyBullets, enemyBulletIndex, MAX_BULLETS, enemiesBlack[i]);
					}
					if(bulletRNG() == 90){
						shootBullet(enemyBullets, enemyBulletIndex, MAX_BULLETS, enemiesBlue[i]);
					}
					if (bulletRNG() == 85) {
						shootBullet(enemyBullets, enemyBulletIndex, MAX_BULLETS, enemiesGreen[i]);
					}
					if (bulletRNG() == 80) {
						shootBullet(enemyBullets, enemyBulletIndex, MAX_BULLETS, enemiesRed[i]);
					}
				}
				for (int i = 0; i < MAX_BULLETS; i++) {
					if (entitiesCollided(enemyBullets[i], player)) {
						mode = STATE_GAME_OVER;
						Mix_PlayChannel(-1, playerdestroyed, 0);
					}
				}
				
			break;
			case STATE_GAME_OVER:
				for (int i = 0; i < 11; i++) {
					enemiesBlack[i].position.y = 1.75f;
					enemiesBlack[i].velocity.y = -0.03;
					enemiesBlue[i].position.y = 1.25f;
					enemiesBlue[i].velocity.y = -0.03;
					enemiesGreen[i].position.y = 0.75f;
					enemiesGreen[i].velocity.y = -0.03;
					enemiesRed[i].position.y = 0.25f;
					enemiesRed[i].velocity.y = -0.03;
				}
				for (int i = 0; i < MAX_BULLETS; i++) {
					enemyBullets[i].position.x = -200.0f;
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

		program.SetModelMatrix(backgroundmodel);
		backgroundmodel.SetScale(7.5, 6, 1);
		float vertices[] = { -0.5, -0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5, 0.5, 0.5, -0.5, 0.5 };
		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
		glEnableVertexAttribArray(program.positionAttribute);
		float texCoords[] = { 0.0, 3.0, 3.0, 3.0, 3.0, 0.0, 0.0, 3.0, 3.0, 0.0, 0.0, 0.0 };
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
			textModel.SetPosition(-3.2, 0.25, 0);
			DrawText(&program, fontTexture, "'A' and 'D' to Move, 'SPACE' to Shoot", .15, 0);
			program.SetModelMatrix(textModel);
			textModel.SetPosition(-2.025, -0.4, 0);
			DrawText(&program, fontTexture, "SPACE INVADERS", .5, 0);
			program.SetModelMatrix(textModel);
			textModel.SetPosition(-2.8, -0.75f, 0);
			DrawText(&program, fontTexture, "Press Return to Start", .2, 0);
		break;
		case STATE_GAME_LEVEL:
			for (int i = 0; i < MAX_BULLETS; i++) {
				program.SetModelMatrix(bulletModels[i]);
				bulletModels[i].SetPosition(bullets[i].position.x, bullets[i].position.y, bullets[i].position.z);
				bullets[i].sprite.Draw(&program);
			}

			for (int i = 0; i < MAX_BULLETS; i++) {
				program.SetModelMatrix(enemyBulletModels[i]);
				enemyBulletModels[i].SetPosition(enemyBullets[i].position.x, enemyBullets[i].position.y, enemyBullets[i].position.z);
				enemyBullets[i].sprite.Draw(&program);
			}

			program.SetModelMatrix(playerModel);
			playerModel.SetPosition(player.position.x, player.position.y, player.position.z);
			player.sprite.Draw(&program);

			for (int i = 0; i < 11; i++) {
				program.SetModelMatrix(enemyModelsBlack[i]);
				enemyModelsBlack[i].SetPosition(enemiesBlack[i].position.x, enemiesBlack[i].position.y, enemiesBlack[i].position.z);
				enemiesBlack[i].sprite.Draw(&program);
				program.SetModelMatrix(enemyModelsBlue[i]);
				enemyModelsBlue[i].SetPosition(enemiesBlue[i].position.x, enemiesBlue[i].position.y, enemiesBlue[i].position.z);
				enemiesBlue[i].sprite.Draw(&program);
				program.SetModelMatrix(enemyModelsGreen[i]);
				enemyModelsGreen[i].SetPosition(enemiesGreen[i].position.x, enemiesGreen[i].position.y, enemiesGreen[i].position.z);
				enemiesGreen[i].sprite.Draw(&program);
				program.SetModelMatrix(enemyModelsRed[i]);
				enemyModelsRed[i].SetPosition(enemiesRed[i].position.x, enemiesRed[i].position.y, enemiesRed[i].position.z);
				enemiesRed[i].sprite.Draw(&program);
			}
			program.SetModelMatrix(textModel);
			textModel.SetPosition(-3.5, 1.85, 0);
			DrawText(&program, fontTexture, std::to_string(stage), .15, 0);
			program.SetModelMatrix(textModel);
			textModel.SetPosition(2.85, 1.85, 0);
			DrawText(&program, fontTexture, "Score", .15, 0);
			program.SetModelMatrix(textModel);
			textModel.SetPosition(-3.5, 1.75, 0);
			DrawText(&program, fontTexture, "Stage", .15, 0);
			program.SetModelMatrix(textModel);
			textModel.SetPosition(2.85, 1.75, 0);
			DrawText(&program, fontTexture, std::to_string(score), .15, 0);
		break;
		case STATE_GAME_OVER:
			program.SetModelMatrix(textModel);
			textModel.SetPosition(-2.025, 0.25, 0);
			DrawText(&program, fontTexture, "Score:", .15, 0);
			program.SetModelMatrix(textModel);
			textModel.SetPosition(-2.025, -0.4, 0);
			DrawText(&program, fontTexture, "GAME OVER", .5, 0);
			program.SetModelMatrix(textModel);
			textModel.SetPosition(-0.35, -1.0f, 0);
			DrawText(&program, fontTexture, "Press Return to Retry", .2, 0);
			program.SetModelMatrix(textModel);
			textModel.SetPosition(-0.35, -0.75f, 0);
			DrawText(&program, fontTexture, std::to_string(score), .15, 0);
		break;
		}
		SDL_GL_SwapWindow(displayWindow);
	}

	SDL_Quit();
	return 0;
}
