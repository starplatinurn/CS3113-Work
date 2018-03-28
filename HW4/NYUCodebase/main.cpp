//CONTROLS: 'A' & 'D' to move, 'SPACE' to jump

#ifdef _WINDOWS
	#include <GL/glew.h>
#endif
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
		modelMatrix.SetScale(size.x, size.y, 1);
		program->SetModelMatrix(modelMatrix);
		sprite.Draw(program);
	};
	void Update(float elapsed) {
		if (!isStatic) {
			velocity.x = lerp(velocity.x, 0.0f, elapsed * 1.5);//x friction
			//velocity.y = lerp(velocity.y, 0.0f, elapsed * 1.5);//y friction
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
	displayWindow = SDL_CreateWindow("Platformer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640*1.5, 360*1.5, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);
	#ifdef _WINDOWS
		glewInit();
	#endif

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glViewport(0, 0, 640*1.5, 360*1.5);
	glClearColor(0.9f, 0.7f, 0.5f, 1.0f);
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
	GameMode mode = STATE_GAME_LEVEL;
	
	std::default_random_engine generator;
	std::uniform_int_distribution<int> distribution(0, 500);
	auto bulletRNG = std::bind(distribution, generator);

	//Platformer textures, entities, etc.
	GLuint p1 = LoadTexture(RESOURCE_FOLDER"p1_spritesheet.png");
	GLuint tiles = LoadTexture(RESOURCE_FOLDER"tiles_spritesheet.png"); 
	GLuint items = LoadTexture(RESOURCE_FOLDER"items_spritesheet.png");
	GLuint background = LoadTexture(RESOURCE_FOLDER"background.png");
	Matrix backgroundmodel;

	Entity player;
	player.isStatic = false;
	player.sprite = SheetSprite(p1, 67.0f/512.0f, 196.0f/512.0f, 66.0f/512.0f, 92.0f/512.0f, 0.8f);
	player.position = Vector3(0, 0, 0);
	player.size = Vector3(0.3, 0.3, 0);

	Entity floor;
	floor.isStatic = true;
	floor.sprite = SheetSprite(tiles, 720.0f / 1024.0f, 864.0f / 1024.0f, 70.0f / 1024.0f, 70.0f / 1024.0f, 1);
	floor.position = Vector3(0, -2, 0);
	floor.size = Vector3(20, 1, 0);

	Entity wall;
	wall.isStatic = true;
	wall.sprite = SheetSprite(tiles, 720.0f /1024.0f, 864.0f /1024.0f, 70.0f /1024.0f, 70.0f /1024.0f, 1.12);
	wall.position = Vector3(-10, 0, 0);
	wall.size = Vector3(1, 20, 0);

	Entity wall2;
	wall2.isStatic = true;
	wall2.sprite = SheetSprite(tiles, 720.0f / 1024.0f, 864.0f / 1024.0f, 70.0f / 1024.0f, 70.0f / 1024.0f, 1.12);
	wall2.position = Vector3(10, 0, 0);
	wall2.size = Vector3(1, 20, 0);

	float coin_pos = -3.3f;
	Entity coins[11];
	for (int i = 0; i < 11; i++) {
		coins[i].isStatic = false;
		coins[i].position = Vector3(coin_pos, -0.5, 0);
		coins[i].sprite = SheetSprite(items, 288.0f / 576.0f, 360.0f / 576.0f, 70.0f / 576.0f, 70.0f / 576.0f, 1);
		coins[i].size = Vector3(0.3, 0.3, 0);
		coin_pos += 1.0f;
	}
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
					if (event.key.keysym.scancode == SDL_SCANCODE_SPACE && entitiesCollided(player,floor)) {
						player.velocity.y = 2;//jump
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
		
			break;
			case STATE_GAME_LEVEL:
				if (keys[SDL_SCANCODE_D]) {
					player.acceleration.x = 3;
				}
				else if (keys[SDL_SCANCODE_A]) {
					player.acceleration.x = -3;
				}
				else {
					player.acceleration.x = 0;
				}
				
				player.velocity.y += -2 * FIXED_TIMESTEP;//gravity
				
				//collision checks
				if (entitiesCollided(player, floor) && player.velocity.y <= 0) {
					player.velocity.y = 0;
				}
				if (entitiesCollided(player, wall)) {
					player.velocity.x = 0;
					player.acceleration.x = 0;
					if (keys[SDL_SCANCODE_D]) {
						player.acceleration.x = 3;
					}
				}
				if (entitiesCollided(player, wall2) ) {
					player.velocity.x = 0;
					player.acceleration.x = 0;
					if (keys[SDL_SCANCODE_A]) {
						player.acceleration.x = -3;
					}
				}
				for (int i = 0; i < 11; i++) {
					if (entitiesCollided(player, coins[i])) {
						coins[i].position.y = -2000.0f;
					}
				}
				player.Update(FIXED_TIMESTEP);
				
			break;
			case STATE_GAME_OVER:
				
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
		backgroundmodel.SetScale(27, 12, 1);
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
			
		break;
		case STATE_GAME_LEVEL:
			player.Draw(&program);
			viewMatrix.SetPosition(-(player.position.x), 0, 0);
			floor.Draw(&program);
			wall.Draw(&program); 
			wall2.Draw(&program);
			for (int i = 0; i < 11; i++) {
				coins[i].Draw(&program);
			}
		break;
		case STATE_GAME_OVER:
			
		break;
		}
		SDL_GL_SwapWindow(displayWindow);
	}

	SDL_Quit();
	return 0;
}
