#include<stdio.h>
#include<easyx.h>
#include<time.h>
#include<stdlib.h>
#include<math.h>
#include<conio.h>
#include<Windows.h>

#pragma comment(lib,"winmm.lib")

#define SCREEN_WIDTH 400
#define SCREEN_HEIGHT 800

#define PLANE_SIZE 50

#define ENEMY_NUM 8
#define ENEMY_SPEED 1.0
#define BULLET_NUM 10

typedef struct pos {
	int x;
	int y;
} POS;

typedef struct plane {
	POS planePos;
	POS planeBullets[BULLET_NUM];
	int bulletLen;
	int bulletSpeed;
} PLANE;

PLANE myPlane;
PLANE enemyPlanes[ENEMY_NUM];
int enemyPlaneLen;
static time_t startTtime, endTime;
IMAGE img[3];
int score = 0;


int main() {
	loadimage(&img[0], "img/background.jpg", SCREEN_WIDTH, SCREEN_HEIGHT);
	loadimage(&img[1], "img/enemyPlane1.jpg", PLANE_SIZE, PLANE_SIZE);
	loadimage(&img[2], "img/planeNormal_2.jpg", PLANE_SIZE, PLANE_SIZE);

	initGame();
	getchar();
	return 0;
}

void initGame() {
	initgraph(SCREEN_WIDTH, SCREEN_HEIGHT);
	score = 0;
	srand((unsigned)time(NULL));	//随机数种子
	myPlane.bulletLen = 0;
	myPlane.bulletSpeed = 3;
	myPlane.planePos = { SCREEN_WIDTH / 2 - PLANE_SIZE / 2,SCREEN_HEIGHT / 2 - PLANE_SIZE };
	enemyPlaneLen = 0;
	startTtime = time(NULL);

}

