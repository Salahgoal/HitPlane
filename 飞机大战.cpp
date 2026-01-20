#include<stdio.h>
// EasyX图形库核心头文件（窗口创建、绘图、贴图等）
#include<easyx.h>
// 时间相关头文件（srand随机数种子需要time(NULL)）
#include<time.h>
// 标准库头文件（rand随机数生成）
#include<stdlib.h>
// 控制台输入输出头文件（_kbhit/_getch检测按键）
#include<conio.h>
// Windows API头文件（GetAsyncKeyState/PlaySound/GetTickCount等）
#include<Windows.h>
//aaa
// 链接多媒体库（PlaySound播放音效必须）
#pragma comment(lib,"winmm.lib")
// 链接透明贴图库（TransparentBlt实现图片透明必须）
#pragma comment(lib,"msimg32.lib")

// 游戏窗口宽度（常量定义，方便修改）
#define SCREEN_WIDTH 400
// 游戏窗口高度
#define SCREEN_HEIGHT 800
// 飞机尺寸（宽/高，敌机和玩家飞机通用）
#define PLANE_SIZE 50
// 最大敌机数量（避免数组越界）
#define ENEMY_NUM 8
// 敌机下落速度（浮点型，可精细调整）
#define ENEMY_SPEED 1.0
// 最大子弹数量（避免子弹数组越界）
#define BULLET_NUM 10

// 坐标结构体：封装x/y轴坐标，代码更整洁
typedef struct pos {
	int x;  // x轴坐标（水平）
	int y;  // y轴坐标（垂直）
} POS;

// 飞机结构体：玩家飞机/敌机通用
typedef struct plane {
	POS planePos;               // 飞机自身的坐标
	POS planeBullets[BULLET_NUM];// 子弹数组（存储每颗子弹的坐标）
	int bulletLen;              // 当前已发射的子弹数量
	int bulletSpeed;            // 子弹飞行速度（向上）
} PLANE;

// 全局变量：方便函数间共享数据
PLANE myPlane;                 // 玩家飞机对象
PLANE enemyPlanes[ENEMY_NUM];  // 敌机数组（最多存8架敌机）
int enemyPlaneLen;             // 当前屏幕上的敌机数量
static DWORD lastEnemyTime = 0;// 上次生成敌机的时间（毫秒级，避免敌机瞬间生成）
static DWORD lastShootTime = 0;// 上次发射子弹的时间（毫秒级，实现发射冷却）
IMAGE img[3];	               // 图片数组：0=背景图 1=敌机图 2=玩家飞机图
int score = 0;                 // 游戏分数（当前阶段暂未用到，保留）
bool gameover = false;

// 函数声明
void initGame();               // 游戏初始化（窗口、参数）
void drawGame();               // 绘制游戏画面（背景、飞机、子弹、分数）
void updateGame();             // 更新游戏逻辑（移动、发射、敌机生成）
void putimageAlpha(int x, int y, IMAGE* img); // 透明贴图函数
void initEnemyPlane();         // 生成敌机函数
bool isCircleCrash(POS c1, POS c2, int r1, int r2);// 圆形碰撞检测函数

int main() {
	// 1. 初始化游戏（创建窗口、重置参数）
	initGame();

	// 2. 加载游戏图片
	// 加载背景图：路径img/background.jpg，尺寸适配窗口
	loadimage(&img[0], _T("img/background.jpg"), SCREEN_WIDTH, SCREEN_HEIGHT);
	// 加载敌机图：路径img/enemyPlane2.png，尺寸PLANE_SIZE×PLANE_SIZE
	loadimage(&img[1], _T("img/enemyPlane2.png"), PLANE_SIZE, PLANE_SIZE);
	// 加载玩家飞机图：路径img/planeNormal_2.png，尺寸PLANE_SIZE×PLANE_SIZE
	loadimage(&img[2], _T("img/planeNormal_2.png"), PLANE_SIZE, PLANE_SIZE);

	// 3. 游戏主循环（无限循环，直到按ESC退出或飞机碰撞）
	while (!gameover) {
		drawGame();   // 绘制当前帧画面
		updateGame(); // 更新当前帧逻辑
		Sleep(10);

		// 检测ESC键（ASCII码27），按下则退出游戏
		if (_kbhit()) {          // 检查是否有按键按下（非阻塞）
			char key = _getch(); // 获取按下的按键
			if (key == 27) {     // 判断是否是ESC键
				break;           // 跳出循环，结束游戏
}
		}
	}

	// 4. 关闭图形窗口（释放资源，规范写法）
	closegraph();
	return 0; // 程序正常退出
}

// 游戏初始化函数：重置所有参数，创建游戏窗口
void initGame() {
	// 创建图形窗口：宽度SCREEN_WIDTH，高度SCREEN_HEIGHT
	initgraph(SCREEN_WIDTH, SCREEN_HEIGHT);
	// 设置窗口背景色为黑色（备用，实际用背景图覆盖）
	setbkcolor(BLACK);
	// 清空画布（应用背景色）
	cleardevice();
	// 分数重置为0
	score = 0;
	// 设置随机数种子（基于系统时间，让敌机生成位置随机）
	srand((unsigned)time(NULL));
	// 玩家子弹数量初始化为0（无子弹）
	myPlane.bulletLen = 0;
	// 玩家子弹速度设为3（每帧向上移动3像素）
	myPlane.bulletSpeed = 3;
	// 玩家飞机初始位置：窗口水平居中，垂直在底部100像素处
	myPlane.planePos = { SCREEN_WIDTH / 2, SCREEN_HEIGHT - 100 };
	// 敌机数量初始化为0（屏幕上无敌机）
	enemyPlaneLen = 0;
}

// 透明贴图函数：解决图片白色背景问题，让飞机图片更自然
// x/y：贴图的左上角坐标；img：要贴的图片对象
void putimageAlpha(int x, int y, IMAGE* img) {
	// 获取图片的宽度和高度
	int w = img->getwidth();
	int h = img->getheight();
	// TransparentBlt：透明贴图API
	// 参数说明：
	// GetImageHDC(NULL)：目标画布（游戏窗口）的设备上下文
	// x/y：目标位置；w/h：目标尺寸
	// GetImageHDC(img)：源图片的设备上下文
	// 0,0,w,h：源图片的裁剪区域（完整图片）
	// RGB(255,255,255)：透明色（白色）
	TransparentBlt(GetImageHDC(NULL), x, y, w, h, GetImageHDC(img), 0, 0, w, h, RGB(255, 255, 255));
}

// 绘制游戏画面函数：批量绘制所有元素，避免画面闪烁
void drawGame() {
	// 开始批量绘制（所有绘制操作先缓存，最后一次性显示）
	BeginBatchDraw();

	// 清空画布（避免上一帧画面残留）
	cleardevice();

	// 1. 绘制背景图（最底层，覆盖整个窗口）
	putimage(0, 0, &img[0]);

	// 2. 绘制玩家飞机（透明贴图，居中显示）
	// x偏移-PLANE_SIZE/2：让飞机中心对齐坐标点（而非左上角）
	putimageAlpha(myPlane.planePos.x - PLANE_SIZE / 2, myPlane.planePos.y - PLANE_SIZE / 2, &img[2]);

	// 3. 绘制所有敌机（遍历敌机数组）
	for (int i = 0; i < enemyPlaneLen; i++) {
		// 敌机同样居中显示，透明贴图
		putimageAlpha(enemyPlanes[i].planePos.x - PLANE_SIZE / 2, enemyPlanes[i].planePos.y - PLANE_SIZE / 2, &img[1]);
	}

	// 4. 绘制所有子弹（黄色圆形，半径5）
	setfillcolor(YELLOW); // 设置子弹填充色为黄色
	for (int i = 0; i < myPlane.bulletLen; i++) {
		// solidcircle：绘制实心圆，参数：x/y坐标，半径
		solidcircle(myPlane.planeBullets[i].x, myPlane.planeBullets[i].y, 5);
	}

	// 5. 绘制分数（屏幕顶部居中显示）
	settextcolor(WHITE);        // 设置文字颜色为白色
	setbkmode(TRANSPARENT);     // 设置文字背景透明（不遮挡背景）
	settextstyle(20, 0, _T("宋体")); // 设置文字大小20，宋体
	TCHAR scoreText[50];        // 存储分数文本的数组
	// 格式化分数文本（把数字score转为字符串）
	_stprintf_s(scoreText, _T("分数：%d"), score);
	// 定义文字显示区域：顶部0~50像素高度，整个窗口宽度
	RECT rect = { 0, 0, SCREEN_WIDTH, 50 };
	// 绘制文字：居中、垂直居中、单行显示
	drawtext(scoreText, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

	// 结束批量绘制（一次性显示所有缓存的绘制内容，防闪烁）
	EndBatchDraw();
}

// 更新游戏逻辑函数：处理移动、发射、敌机生成
void updateGame() {
	// 1. 调用敌机生成函数（每帧执行，检查是否该生成新敌机）
	initEnemyPlane();

	// 2. 玩家飞机移动逻辑（W/A/S/D键，带边界检测，避免飞出屏幕）
	// W键：向上移动（检测是否超过屏幕顶部）
	if (GetAsyncKeyState('W') & 0x8000) {
		if (myPlane.planePos.y > PLANE_SIZE / 2)
			myPlane.planePos.y -= 4; // 每帧向上移动4像素
	}
	// S键：向下移动（检测是否超过屏幕底部）
	if (GetAsyncKeyState('S') & 0x8000) {
		if (myPlane.planePos.y < SCREEN_HEIGHT - PLANE_SIZE / 2)
			myPlane.planePos.y += 4; // 每帧向下移动4像素
	}
	// A键：向左移动（检测是否超过屏幕左侧）
	if (GetAsyncKeyState('A') & 0x8000) {
		if (myPlane.planePos.x > PLANE_SIZE / 2)
			myPlane.planePos.x -= 4; // 每帧向左移动4像素
	}
	// D键：向右移动（检测是否超过屏幕右侧）
	if (GetAsyncKeyState('D') & 0x8000) {
		if (myPlane.planePos.x < SCREEN_WIDTH - PLANE_SIZE / 2)
			myPlane.planePos.x += 4; // 每帧向右移动4像素
	}

	// 3. 子弹发射逻辑
	// VK_SPACE是空格的虚拟键码，&0x8000检测是否按下
	if (GetAsyncKeyState(VK_SPACE) & 0x8000) {
		if (myPlane.bulletLen < BULLET_NUM && GetTickCount() - lastShootTime > 200) {
			PlaySound(_T("img/man.wav"), NULL, SND_FILENAME | SND_ASYNC | SND_NOWAIT);
			myPlane.planeBullets[myPlane.bulletLen] = myPlane.planePos;
			myPlane.bulletLen++;
			lastShootTime = GetTickCount();
		}
	}

	// 4. 敌机下落逻辑（遍历所有敌机，每帧向下移动）
	for (int i = 0; i < enemyPlaneLen; i++) {
		// 使用定义的ENEMY_SPEED常量（1.0），而非硬编码
		enemyPlanes[i].planePos.y += ENEMY_SPEED;
	}

	// 5. 子弹移动逻辑（遍历所有子弹，每帧向上移动）
	for (int i = 0; i < myPlane.bulletLen; i++) {
		// 子弹y坐标减少（向上飞），速度为myPlane.bulletSpeed=3
		myPlane.planeBullets[i].y -= myPlane.bulletSpeed;
	}

	// 6. 子弹边界清理(倒序遍历)
	for (int j = myPlane.bulletLen - 1; j >= 0;j--) {
		if (myPlane.planeBullets[j].y < 0) {
			//数组前移删除
			for (int k = j; k < myPlane.bulletLen - 1; k++) {
				myPlane.planeBullets[k] = myPlane.planeBullets[k + 1];
			}
			myPlane.bulletLen--;
		}

	}

	// 7. 敌机边界清理
	for (int i = enemyPlaneLen - 1; i >= 0; i--) {
		if (enemyPlanes[i].planePos.y > SCREEN_HEIGHT + PLANE_SIZE) {
			// 数组前移删除
			for (int j = i; j < enemyPlaneLen - 1; j++) {
				enemyPlanes[j] = enemyPlanes[j + 1];
			}
			enemyPlaneLen--;
		}
	}
	
	//8.检查子弹与敌机的碰撞
	//倒序遍历子弹
	for (int j = myPlane.bulletLen - 1; j >= 0;j--) {
		//倒序遍历敌机
		for (int i = enemyPlaneLen - 1; i >= 0; i--) {
			if (isCircleCrash(myPlane.planeBullets[j], enemyPlanes[i].planePos, 5, PLANE_SIZE / 2)) {
				//子弹击中敌机加分并删除子弹
				score += 10;
				//销毁当前子弹（数组前移）
				for (int k = j; k < myPlane.bulletLen - 1; k++) {
					myPlane.planeBullets[k] = myPlane.planeBullets[k + 1];
				}
				myPlane.bulletLen--;
				//销毁当前敌机（数组前移）
				for (int k = i; k < enemyPlaneLen - 1; k++) {
					enemyPlanes[k] = enemyPlanes[k + 1];
				}
				enemyPlaneLen--;

				break; //跳出敌机循环，检查下一颗子弹
			}
		}
	}

	//9.检测我机与敌机的碰撞
	for (int i = 0; i < enemyPlaneLen; i++) {
		if (isCircleCrash(myPlane.planePos, enemyPlanes[i].planePos, PLANE_SIZE / 2, PLANE_SIZE / 2)) {
			//定义缓冲区存格式化后的提示文本
			TCHAR msg[100];
			//把socre的值格式化到msg中
			_stprintf_s(msg, _T("Manba Out! 你和牢大合砍:%d分"), score);
			PlaySound(_T("img/Manbaout.wav"), NULL, SND_FILENAME | SND_ASYNC | SND_NOWAIT);
			//弹出消息框显示最终得分
			MessageBox(NULL, msg, _T("Manba Out!"), MB_OK);
			gameover = true;
			break;
		}
	}
}

// 生成敌机函数：每隔1秒生成1架敌机，最多8架
void initEnemyPlane() {
	// GetTickCount()：获取系统启动到现在的毫秒数
	// 条件：距离上次生成敌机超过1000毫秒（1秒）→ 避免敌机生成过快
	if (GetTickCount() - lastEnemyTime > 1000) {
		// 检查敌机数量是否未超过最大值（ENEMY_NUM=8）
		if (enemyPlaneLen < ENEMY_NUM) {
			// 随机生成敌机X坐标：保证敌机完全在屏幕内，不贴边
			// rand() % (SCREEN_WIDTH - PLANE_SIZE)：0~350的随机数
			// + PLANE_SIZE/2：偏移25，最终X范围25~375
			int x = rand() % (SCREEN_WIDTH - PLANE_SIZE) + PLANE_SIZE / 2;
			// 敌机Y坐标：-PLANE_SIZE（-50）→ 从屏幕上方外生成，慢慢下落
			int y = -PLANE_SIZE;

			// 给新敌机赋值坐标
			enemyPlanes[enemyPlaneLen].planePos.x = x;
			enemyPlanes[enemyPlaneLen].planePos.y = y;

			// 敌机数量+1（下次生成用下一个数组索引）
			enemyPlaneLen++;

			// 更新上次生成敌机时间（重置计时）
			lastEnemyTime = GetTickCount();
		}
	}
}
// 检测圆形碰撞函数：判断玩家飞机与敌机是否碰撞
// c1/c2：两个圆心坐标；r1/r2：两个圆的半径
//参数:子弹(我机)圆心坐标,敌机圆心坐标,子弹半径,敌机半径
bool isCircleCrash(POS c1,POS c2,int r1,int r2) {
	//计算两个圆心的x、y差值
	int dx = c1.x - c2.x;
	int dy = c1.y - c2.y;
	//计算距离的平方和半径的平方
	int distanceSq = dx * dx + dy * dy;
	int radiusSumSq = (r1 + r2) * (r1 + r2);
	//判断是否碰撞:距离平方小于等于半径平方和则碰撞
	return distanceSq <= radiusSumSq;
}