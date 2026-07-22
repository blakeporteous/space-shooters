#include <SDL.h>
#include <stdio.h>
#include <vector>
#include <algorithm>
#include <cmath>
#include <iostream>
#undef main

const int SCREEN_WIDTH = 320;
const int SCREEN_HEIGHT = 480;

SDL_Window* gWindow = NULL;
SDL_Surface* gScreenSurface = NULL;
SDL_Surface* ImgGreyAndRedEnemy = NULL;
SDL_Surface* ImgGreenEnemy = NULL;
SDL_Surface* ImgOrangeEnemy = NULL;
SDL_Surface* ImgGreyShip = NULL;
SDL_Surface* ImgPurpleEnemy = NULL;
SDL_Surface* ImgStartScreen = NULL;
SDL_Surface* ImgLoseScreen = NULL;
SDL_Surface* ExplosionScreen1 = NULL;
SDL_Surface* ExplosionScreen2 = NULL;
SDL_Surface* ExplosionScreen3 = NULL;
SDL_Surface* ExplosionScreen4 = NULL;

struct Bullet {
	int x, y;
	int w = 3;
	int h = 10;
	float speed = 4;
};

enum MovementType { CIRCLE, ZIGZAG, LEFT_DOWN, MIDDLE_JIGGLE };

struct Enemy {
	float x, y;
	int w, h;
	float angleOffset;
	float t;
	MovementType movement;
	SDL_Surface* sprite;
};

struct Explosion {
	float x, y;
	int frame;
	float timer;
};

std::vector<Bullet> bullets;
std::vector<Bullet> enemyBullets;
std::vector<Enemy> enemies;
std::vector<Explosion> explosions;

SDL_Surface* explosionFrames[4];
const float explosionFrameTime = 0.05f;

float circleAngle = 0.0f;
float circleRadius = 100.0f;
float circleCenterX = SCREEN_WIDTH / 2.0f;
float circleCenterY = -200.0f;

SDL_Rect playerRect;
Uint32 lastShotTime = 0;
const Uint32 shootCooldown = 150;
Uint32 lastEnemyShotTime = 0;
const Uint32 enemyShootCooldown = 1700;

bool init() {
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		printf("SDL could not initialize! SDL Error: %s\n", SDL_GetError());
		return false;
	}
	gWindow = SDL_CreateWindow("SDL2 Alien Invasion", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
	if (!gWindow) {
		printf("Window could not be created! SDL Error: %s\n", SDL_GetError());
		return false;
	}
	gScreenSurface = SDL_GetWindowSurface(gWindow);
	return true;
}

SDL_Surface* loadBMP(const char* path) {
	SDL_Surface* surf = SDL_LoadBMP(path);
	if (!surf) {
		printf("Unable to load image %s! SDL Error: %s\n", path, SDL_GetError());
		return NULL;
	}
	Uint32 colorkey = SDL_MapRGB(surf->format, 0, 255, 0);
	SDL_SetColorKey(surf, SDL_TRUE, colorkey);
	return surf;
}

bool loadMedia() {
	ImgGreyAndRedEnemy = loadBMP("images/GreyAndRedEnemy.bmp");
	ImgGreenEnemy = loadBMP("images/GreenEnemy.bmp");
	ImgOrangeEnemy = loadBMP("images/OrangeEnemy.bmp");
	ImgGreyShip = loadBMP("images/GreySpaceship.bmp");
	ImgPurpleEnemy = loadBMP("images/PurpleEnemy.bmp");
	ImgStartScreen = SDL_LoadBMP("images/StartScreen.bmp");
	ImgLoseScreen = SDL_LoadBMP("images/LoseScreen.bmp");

	ExplosionScreen1 = loadBMP("images/Explosion1.bmp");
	ExplosionScreen2 = loadBMP("images/Explosion2.bmp");
	ExplosionScreen3 = loadBMP("images/Explosion3.bmp");
	ExplosionScreen4 = loadBMP("images/Explosion4.bmp");

	explosionFrames[0] = ExplosionScreen1;
	explosionFrames[1] = ExplosionScreen2;
	explosionFrames[2] = ExplosionScreen3;
	explosionFrames[3] = ExplosionScreen4;

	return (ImgGreyAndRedEnemy && ImgGreenEnemy && ImgOrangeEnemy && ImgGreyShip && ImgPurpleEnemy &&
		ImgStartScreen && ImgLoseScreen && ExplosionScreen1 && ExplosionScreen2 && ExplosionScreen3 && ExplosionScreen4);
}

void close() {
	SDL_FreeSurface(ImgGreyAndRedEnemy);
	SDL_FreeSurface(ImgGreenEnemy);
	SDL_FreeSurface(ImgOrangeEnemy);
	SDL_FreeSurface(ImgGreyShip);
	SDL_FreeSurface(ImgPurpleEnemy);
	SDL_FreeSurface(ImgStartScreen);
	SDL_FreeSurface(ImgLoseScreen);
	SDL_FreeSurface(ExplosionScreen1);
	SDL_FreeSurface(ExplosionScreen2);
	SDL_FreeSurface(ExplosionScreen3);
	SDL_FreeSurface(ExplosionScreen4);
	SDL_DestroyWindow(gWindow);
	SDL_Quit();
}

// --- SPAWN & UPDATE ENEMIES ---
void spawnEnemies() {
	enemies.clear();
	const float verticalGap = 50.0f;
	const float startOffsets[] = { 0.0f, -600.0f, -1300.0f, -1700.0f };
	for (int type = 0; type < 4; ++type) {
		if (type == LEFT_DOWN) {
			const int leftCount = 4, middleCount = 5, rightCount = 4, columnGap = 40;
			for (int i = 0; i < leftCount; ++i) { Enemy e; e.x = 10.0f; e.y = startOffsets[type] - i * verticalGap; e.w = ImgGreyShip->w; e.h = ImgGreyShip->h; e.angleOffset = 0; e.t = 0.0f; e.movement = LEFT_DOWN; e.sprite = ImgGreyShip; enemies.push_back(e); }
			for (int i = 0; i < middleCount; ++i) { Enemy e; e.x = 10.0f + columnGap; e.y = startOffsets[type] - i * verticalGap + 20; e.w = ImgGreyShip->w; e.h = ImgGreyShip->h; e.angleOffset = 0; e.t = 0.0f; e.movement = LEFT_DOWN; e.sprite = ImgGreyShip; enemies.push_back(e); }
			for (int i = 0; i < rightCount; ++i) { Enemy e; e.x = 10.0f + 2 * columnGap; e.y = startOffsets[type] - i * verticalGap; e.w = ImgGreyShip->w; e.h = ImgGreyShip->h; e.angleOffset = 0; e.t = 0.0f; e.movement = LEFT_DOWN; e.sprite = ImgGreyShip; enemies.push_back(e); }
		}
		else if (type == MIDDLE_JIGGLE) {
			const int leftCount = 5, rightCount = 5, columnGap = 60;
			for (int i = 0; i < leftCount; ++i) { Enemy e; e.x = (SCREEN_WIDTH / 2.0f) - columnGap / 2.0f; e.y = startOffsets[type] - i * verticalGap; e.w = 30; e.h = 20; e.angleOffset = 0; e.t = i * 0.5f; e.movement = MIDDLE_JIGGLE; e.sprite = ImgPurpleEnemy; enemies.push_back(e); }
			for (int i = 0; i < rightCount; ++i) { Enemy e; e.x = (SCREEN_WIDTH / 2.0f) + columnGap / 2.0f; e.y = startOffsets[type] - i * verticalGap; e.w = 30; e.h = 20; e.angleOffset = 0; e.t = i * 0.5f; e.movement = MIDDLE_JIGGLE; e.sprite = ImgPurpleEnemy; enemies.push_back(e); }
		}
		else if (type == ZIGZAG) {
			const float gap = 100.0f;
			for (int i = 0; i < 5; ++i) { Enemy e; e.x = SCREEN_WIDTH / 2.0f - 100.0f; e.y = startOffsets[type] - i * gap; e.w = 30; e.h = 20; e.angleOffset = i * (2 * M_PI / 10.0f); e.t = i * 0.5f; e.movement = ZIGZAG; e.sprite = ImgOrangeEnemy; enemies.push_back(e); }
		}
		else {
			const int enemiesPerType = 10; const float gap = 100.0f;
			for (int i = 0; i < enemiesPerType; ++i) { Enemy e; e.x = circleCenterX; e.y = startOffsets[type] - i * gap; e.w = 30; e.h = 20; e.angleOffset = i * (2 * M_PI / 10.0f); e.t = i * 0.5f; e.movement = CIRCLE; e.sprite = ImgGreenEnemy; enemies.push_back(e); }
		}
	}
}

void updateEnemies(float dt) {
	circleAngle += 0.01f;
	circleCenterY += 0.6f;
	for (auto& enemy : enemies) {
		enemy.t += dt;
		switch (enemy.movement) {
		case CIRCLE: enemy.x = circleCenterX + cos(circleAngle + enemy.angleOffset) * circleRadius * 1.1f; enemy.y = circleCenterY + sin(circleAngle + enemy.angleOffset) * circleRadius; break;
		case ZIGZAG: enemy.y += 0.8f; enemy.x = (SCREEN_WIDTH / 2.0f) + 4.0f * sin(enemy.t * 2.0f) * 30.0f; break;
		case LEFT_DOWN: enemy.y += 0.9f; break;
		case MIDDLE_JIGGLE: enemy.y += 0.9f; enemy.x += sin(enemy.t * 3.0f) * 0.5f; break;
		}
	}
}

// --- MAIN LOOP ---
int main(int argc, char* args[]) {
	if (!init() || !loadMedia()) return 1;

	playerRect = { (SCREEN_WIDTH - ImgGreyAndRedEnemy->w) / 2, SCREEN_HEIGHT - ImgGreyAndRedEnemy->h - 10, ImgGreyAndRedEnemy->w, ImgGreyAndRedEnemy->h };
	spawnEnemies();

	bool quit = false, running = false, spacePressed = false;
	SDL_Event e;
	Uint32 lastTicks = SDL_GetTicks();

	// START SCREEN
	while (!quit && !running) {
		while (SDL_PollEvent(&e)) {
			if (e.type == SDL_QUIT) quit = true;
			if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_SPACE) running = true;
		}
		SDL_Rect destRect = { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT };
		SDL_BlitScaled(ImgStartScreen, NULL, gScreenSurface, &destRect);
		SDL_UpdateWindowSurface(gWindow);
		SDL_Delay(16);
	}

	// GAME LOOP
	while (!quit) {
		Uint32 currentTicks = SDL_GetTicks();
		float dt = (currentTicks - lastTicks) / 1000.0f;
		lastTicks = currentTicks;

		// EVENTS
		while (SDL_PollEvent(&e)) {
			if (e.type == SDL_QUIT) quit = true;
			if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_SPACE) {
				Uint32 now = SDL_GetTicks();
				if (!spacePressed && now - lastShotTime >= shootCooldown) {
					bullets.push_back({ playerRect.x + playerRect.w / 2 - 2, playerRect.y });
					lastShotTime = now;
					spacePressed = true;
				}
			}
			if (e.type == SDL_KEYUP && e.key.keysym.sym == SDLK_SPACE) spacePressed = false;
		}

		// PLAYER MOVEMENT
		const Uint8* keys = SDL_GetKeyboardState(NULL);
		if (keys[SDL_SCANCODE_LEFT]) playerRect.x -= 3;
		if (keys[SDL_SCANCODE_RIGHT]) playerRect.x += 3;
		if (playerRect.x < 0) playerRect.x = 0;
		if (playerRect.x + playerRect.w > SCREEN_WIDTH) playerRect.x = SCREEN_WIDTH - playerRect.w;

		// UPDATE BULLETS
		for (auto& bullet : bullets) bullet.y -= bullet.speed;
		bullets.erase(std::remove_if(bullets.begin(), bullets.end(), [](const Bullet& b) { return b.y + b.h < 0; }), bullets.end());

		// ENEMY SHOOTING
		Uint32 now = SDL_GetTicks();
		if (now - lastEnemyShotTime >= enemyShootCooldown) {
			for (auto& enemy : enemies) {
				if (enemy.y >= 0 && enemy.y <= SCREEN_HEIGHT) {
					enemyBullets.push_back({ (int)enemy.x + enemy.w / 2 - 2, (int)enemy.y + enemy.h, 3, 10, 2.0f });
				}
			}
			lastEnemyShotTime = now;
		}
		for (auto& bullet : enemyBullets) bullet.y += bullet.speed;
		enemyBullets.erase(std::remove_if(enemyBullets.begin(), enemyBullets.end(), [](const Bullet& b) { return b.y > SCREEN_HEIGHT; }), enemyBullets.end());

		updateEnemies(dt);

		// COLLISIONS
		for (auto bulletIt = bullets.begin(); bulletIt != bullets.end();) {
			bool removedBullet = false;
			for (auto enemyIt = enemies.begin(); enemyIt != enemies.end();) {
				if (bulletIt->x < enemyIt->x + enemyIt->w && bulletIt->x + bulletIt->w > enemyIt->x &&
					bulletIt->y < enemyIt->y + enemyIt->h && bulletIt->y + bulletIt->h > enemyIt->y) {
					explosions.push_back({ enemyIt->x, enemyIt->y, 0, 0.0f });
					enemyIt = enemies.erase(enemyIt);
					bulletIt = bullets.erase(bulletIt);
					removedBullet = true;
					break;
				}
				else ++enemyIt;
			}
			if (!removedBullet) ++bulletIt;
		}

		// UPDATE EXPLOSIONS
		for (auto it = explosions.begin(); it != explosions.end();) {
			it->timer += dt;
			if (it->timer >= explosionFrameTime) {
				it->timer = 0.0f;
				it->frame++;
				if (it->frame >= 4) {
					it = explosions.erase(it);
					continue;
				}
			}
			++it;
		}

		// CHECK PLAYER DEATH
		bool playerDead = false;
		for (auto& bullet : enemyBullets) {
			if (bullet.x < playerRect.x + playerRect.w && bullet.x + bullet.w > playerRect.x &&
				bullet.y < playerRect.y + playerRect.h && bullet.y + bullet.h > playerRect.y) {
				playerDead = true;
				break;
			}
		}

		if (playerDead) {
			bool showingLoseScreen = true;
			SDL_Rect destRect = { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT };
			while (showingLoseScreen) {
				while (SDL_PollEvent(&e)) {
					if (e.type == SDL_QUIT) { showingLoseScreen = false; quit = true; }
					if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_SPACE) { showingLoseScreen = false; quit = true; }
				}
				SDL_BlitScaled(ImgLoseScreen, NULL, gScreenSurface, &destRect);
				SDL_UpdateWindowSurface(gWindow);
				SDL_Delay(16);
			}
			break;
		}

		// RENDER
		SDL_FillRect(gScreenSurface, NULL, SDL_MapRGB(gScreenSurface->format, 0, 0, 0));
		SDL_BlitSurface(ImgGreyAndRedEnemy, NULL, gScreenSurface, &playerRect);

		SDL_Rect enemyRect;
		for (const auto& enemy : enemies) {
			enemyRect = { (int)enemy.x, (int)enemy.y, enemy.w, enemy.h };
			SDL_BlitSurface(enemy.sprite, NULL, gScreenSurface, &enemyRect);
		}

		SDL_Rect bulletRect;
		for (const auto& bullet : bullets) { bulletRect = { bullet.x, bullet.y, bullet.w, bullet.h }; SDL_FillRect(gScreenSurface, &bulletRect, SDL_MapRGB(gScreenSurface->format, 255, 0, 0)); }
		for (const auto& bullet : enemyBullets) { bulletRect = { bullet.x, bullet.y, bullet.w, bullet.h }; SDL_FillRect(gScreenSurface, &bulletRect, SDL_MapRGB(gScreenSurface->format, 255, 255, 0)); }

		SDL_Rect explosionRect;
		for (const auto& exp : explosions) {
			explosionRect = { (int)exp.x, (int)exp.y, 30, 30 };
			SDL_BlitSurface(explosionFrames[exp.frame], NULL, gScreenSurface, &explosionRect);
		}

		SDL_UpdateWindowSurface(gWindow);
		SDL_Delay(10);
	}

	close();
	return 0;
}
