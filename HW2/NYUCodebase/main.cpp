//--PONG--
//CONTROLS: PLAYER 2 (BLUE) MOVES WITH 'W' AND 'S' KEYS. PLAYER 1 (RED) MOVES WITH 'UP ARROW' AND 'DOWN ARROW' KEYS.  


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
#include <iostream>
using namespace std;

#ifdef _WINDOWS
	#define RESOURCE_FOLDER ""
#else
	#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

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

SDL_Window* displayWindow;

int main(int argc, char *argv[])
{
	SDL_Init(SDL_INIT_VIDEO);
	displayWindow = SDL_CreateWindow("Pong", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640*1.5, 360*1.5, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);
	#ifdef _WINDOWS
		glewInit();
	#endif

	float lastFrameTicks = 0.0f;

	//Paddle, ball positions
	float pong1_y_pos = 0.0f;
	float pong2_y_pos = 0.0f;
	float ball_y_pos = 0.0f;
	float ball_x_pos = 0.0f;
	bool xpositive = true;
	bool ypositive = true;
	//

	glViewport(0, 0, 640 * 1.5, 360 * 1.5);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	ShaderProgram program;
	program.Load(RESOURCE_FOLDER"vertex.glsl", RESOURCE_FOLDER"fragment.glsl");
	
	//Paddle, ball matrices
	Matrix pong1;
	Matrix pong2;
	Matrix ball;
	//

	Matrix viewMatrix;
	Matrix projectionMatrix;
	
	projectionMatrix.SetOrthoProjection(-3.55f, 3.55f, -2.0f, 2.0f, -1.0f, 1.0f);

	glUseProgram(program.programID);

	SDL_Event event;
	bool done = false;
	while (!done) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
				done = true;
			}
		}
		glClear(GL_COLOR_BUFFER_BIT);

		float ticks = (float)SDL_GetTicks() / 1000.0f;
		float elapsed = ticks - lastFrameTicks;
		lastFrameTicks = ticks;

		//Ball movement
		if(ball_x_pos >= 3.1875f && ball_x_pos <= 3.2f && ball_y_pos <= (pong1_y_pos+0.375) && ball_y_pos >= (pong1_y_pos-0.375)) {
			xpositive = false;
		}
		else if (ball_x_pos >= 3.55) { std::cout << "Player 2 Wins!\n"; done = true; }
		
		if(ball_x_pos <= -3.1875f && ball_x_pos >= -3.2f && ball_y_pos <= (pong2_y_pos + 0.375) && ball_y_pos >= (pong2_y_pos - 0.375)) {
			xpositive = true;
		}
		else if (ball_x_pos <= -3.55) { std::cout << "Player 1 Wins!\n"; done = true; }
		
		if (ball_y_pos >= 2.0f) {
			ypositive = false;
		}
		if (ball_y_pos <= -2.0f) {
			ypositive = true;
		}
		
		if (xpositive) {
			ball_x_pos += elapsed*1.5;
		}
		else { ball_x_pos -= elapsed*1.5; }

		if (ypositive) {
			ball_y_pos += elapsed*1.5;
		}
		else { ball_y_pos -= elapsed*1.5; }
		//
		
		//Paddle Movement
		const Uint8 *keys = SDL_GetKeyboardState(NULL);
		if (keys[SDL_SCANCODE_UP]) {
			if (pong1_y_pos <= 1.625) { pong1_y_pos += .005; }
		}
		else if (keys[SDL_SCANCODE_DOWN]) {
			if (pong1_y_pos >= -1.625) { pong1_y_pos -= .005; }
		}
		if (keys[SDL_SCANCODE_W]) {
			if (pong2_y_pos <= 1.625) { pong2_y_pos += .005; }
		}
		else if (keys[SDL_SCANCODE_S]) {
			if (pong2_y_pos >= -1.625) { pong2_y_pos -= .005; }
		}
		//

		//Drawing
		program.SetProjectionMatrix(projectionMatrix);
		program.SetViewMatrix(viewMatrix);

		float vertices[] = { -0.5, -0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5, 0.5, 0.5, -0.5, 0.5 };

		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
		glEnableVertexAttribArray(program.positionAttribute);

		program.SetModelMatrix(pong1);
		program.SetColor(0.7f, 0.2f, 0.2f, 1.0f);
		pong1.SetPosition(3.25, pong1_y_pos, 0);
		pong1.SetScale(.125, .75, 1);

		glDrawArrays(GL_TRIANGLES, 0, 6);

		program.SetModelMatrix(pong2);
		program.SetColor(0.3f, 0.3f, 0.7f, 1.0f);
		pong2.SetPosition(-3.25, pong2_y_pos, 0);
		pong2.SetScale(.125, .75, 1);

		glDrawArrays(GL_TRIANGLES, 0, 6);

		program.SetModelMatrix(ball);
		program.SetColor(1.0f, 1.0f, 1.0f, 1.0f);
		ball.SetPosition(ball_x_pos, ball_y_pos, 0);
		ball.SetScale(.125, .125, 1);

		glDrawArrays(GL_TRIANGLES, 0, 6);

		glDisableVertexAttribArray(program.positionAttribute);

		SDL_GL_SwapWindow(displayWindow);
	}

	SDL_Quit();
	return 0;
}
