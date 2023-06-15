#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <SDL_mixer.h>
#include "menu.h"
#include <iostream>
#include <string>
#include <ctime>
#include <cstdlib>
#include <sstream>
#include <vector>

using namespace std;

#define WIDTH 620
#define HEIGHT 720
#define FONT_SIZE 32
#define BALL_SPEED 8
#define SPEED 9
#define SIZE 16
#define COL 13
#define ROW 8
#define PI 3.14159265358979323846

//khoang cach giua 2 brick
int SPACING = 16;

//but ve cua game
SDL_Renderer* renderer;

//con tro cua so 
SDL_Window* window;

//cai thu duoc ve ra man hinh
SDL_Surface* screen;

//font chu su dung trong game
TTF_Font* font;

//mau sac
SDL_Color color, brick_col;

bool running;
int frameCount, timerFPS, lastFrame, fps = 60;

SDL_Rect paddle, ball, txtFrame, brick;
float velY, velX;
int livesCount = 3, score = 0;

bool bricks[ROW * COL];

vector<Mix_Music*> music;
int volume;

//Am thanh
void setVolume(int v)
{
	volume = (MIX_MAX_VOLUME * v) / 100;
}

void initMixer()
{
	Mix_Init(MIX_INIT_MP3);
	SDL_Init(SDL_INIT_AUDIO);
	setVolume(80);
}

int loadMusic(const char* filename)
{
	Mix_Music* m = NULL;
	m = Mix_LoadMUS(filename);
	music.push_back(m);
	return music.size() - 1;
}

void playMusic(int m)
{
	if (Mix_PlayingMusic() == 0) {
		Mix_Volume(1, volume);
		Mix_PlayMusic(music[m], -1);
	}
}

//Reset trang thai cho pad va ball khi bat dau luot moi
void resetPadnBall(SDL_Rect& paddle, SDL_Rect& ball)
{
	// Dat vi tri cua pad vao giua man hinh
	paddle.x = (WIDTH / 2) - (paddle.w / 2);

	//Hoanh do ball o giua man hinh, tung do ball o giua cua pad
	ball.x = WIDTH / 2 - (SIZE / 2);
	ball.y = paddle.y - (paddle.h * 4);

	// Van toc ban dau cua ball chi la van toc theo truc y, van toc theo truc x = 0
	velY = BALL_SPEED;
	velX = 0;
}

// Reset lai ban brick
void resetBricks() {
	// Gan tat ca brick = 1 = brick van con song
	for (int i = 0; i < COL * ROW; i++) 
	{
		bricks[i] = 1;
	}
	resetPadnBall(paddle, ball);
}

//Set vi tri cua cac brick
//Moi brick la 1 phan tu cua mang brick, cach deu nhau 1 khoang /SPACING, moi brick co hoanh do x, tung do y, chieu rong w, chieu day h
//i la so thu tu brick (mang brick la bool 1 chieu nen so phan tu la row*col)
void setBrick(int i) {
	//SPACING = rand() % 32;
	//i = rand();
	brick.x = (((i % COL) + 1) * SPACING) + ((i % COL) * brick.w) - (SPACING / 2);
	brick.y = brick.h * 3 + (((i % ROW) + 1) * SPACING) + ((i % ROW) * brick.h) - (SPACING / 2);
}

//Chuyen so thanh xau
string toStr(double num)
{
	stringstream ss;
	ss << num;
	string text;
	ss >> text;
	return text;
}

// Viet 1 xau ra man hinh tai vi tri co toa do (x,y)
void write(string text, int x, int y) 
{
	//Surface la mot "bo nho tam" de luu cac pixel cua chu can viet ra man hinh
	SDL_Surface* surface;
	//Texture la "buc anh" duoc "tai tao" tu bo pixel trong surface
	SDL_Texture* texture;

	//Chu can ghi ra ( o day la so mang, so diem ), chuyen thanh dang con tro char*
	const char* t = text.c_str();

	//	Lay cac pixel cua chu dat vao "bo nho tam" surface
	// Can truyen vao font (font chu can in), t (con tro char* chi den vi tri chu), color (mau nen cho khung chu)
	surface = TTF_RenderText_Solid(font, t, color);
	texture = SDL_CreateTextureFromSurface(renderer, surface);

	//Can chinh vi tri de ghi chu (o giua man hinh)
	//txtFrame la cau truc chu nhat SDL_Rect co nhiem vu lam "khung anh" de ta in chu ra
	txtFrame.w = surface->w;
	txtFrame.h = surface->h;
	txtFrame.x = x - txtFrame.w;
	txtFrame.y = y - txtFrame.h;

	//Giai phong con tro surface xong nhiem vu
	SDL_FreeSurface(surface);
	
	//Chep "buc anh" trong texture vao con tro chinh renderer de viet ra man hinh
	SDL_RenderCopy(renderer, texture, NULL, &txtFrame);

	//Texture xong nhiem vu, xoa di
	SDL_DestroyTexture(texture);
}


//Check xem ball va cham voi brick o mat nao (LR-trai hay phai)
string checkCollisionSideLR(SDL_Rect &ball, SDL_Rect &brick)
{
	/*	- - - - - - - - - - - - - > \x
		|				
		|			_________
		|		o	|________|	o
		|				
		|				
		v  \y
	*/
	if (ball.x >= brick.x) return "right";
	else return "left";
}

//Check xem ball va cham voi brick o mat nao (UD-tren hay duoi)
string checkCollisionSideUD(SDL_Rect& ball, SDL_Rect& brick)
{
	/*	- - - - - - - - - - - - - > \x
		|				o
		|			_________
		|			|________|	
		|				o
		|
		v  \y
	*/
	if (ball.y >= brick.y) return "down";
	else return "up";
}

//Tinh goc nay 
double calcBounceAngle(SDL_Rect &ball, SDL_Rect &paddle)
{
	/*						velY
							 |------------ velX
						 \   |  /
						  \  | /
						   \ |/
							\/	\ bounce
					________rel__\_____
							norm
	*/
	//rel la diem roi khi ball cham vao pad
	double rel = (paddle.x + (paddle.w / 2)) - (ball.x + (SIZE / 2));
	//norm la ti le khoang cach so voi trung tam pad
	double norm = rel / (paddle.w / 2);
	//goc nay bang norm * 5pi/12
	return norm * (5 * PI / 12);
}

//check xem ball cham bien tren man hinh ko
bool collidewUpperBound(SDL_Rect& ball)
{
	// Tung do ball <=0 khi ball cham vao mat tren cua so
	return (ball.y <= 0);
}

//check xem ball cham bien trai man hinh ko
bool collidewLeftBound(SDL_Rect& ball)
{
	//Hoanh do cua ball <= 0 (ball cham bien trai man hinh)
	return (ball.x <= 0);
}

//Check xem ball cham bien phai man hinh ko
bool collidewRightBound(SDL_Rect& ball)
{
	return (ball.x + SIZE >= WIDTH);
	//hoanh do > chieu rong cua so (ball cham bien phai man hinh)

}

//Check xem ball roi xuong day man hinh (die) ko
bool dropped(SDL_Rect& ball)
{
	// Tung do ball >= chieu cao toi da cua so (ball roi xuong day cua so) 
	return (ball.y + SIZE >= HEIGHT);
}

//Check xem mat het mang chua
bool die(int& livesCount)
{
	return (livesCount <= 0);
}

//	Ball dang chuyen dong (van toc phan thanh van toc theo phuong truc x và phuong truc y)
//	Can truyen vao doi tuong ball, van toc cua ball theo 2 phuong
//	Khi ball di chuyen, hoanh do va tung do se thay doi 1 gia tri bang van toc theo chieu tuong ung
void move(SDL_Rect& ball, int velX, int velY)
{
	ball.x += velX;
	ball.y += velY;
}

void update()
{
	//Di chuyen ball
	move(ball, velX, velY);

	// Neu phat hien va cham ball va pad (ball va pad giao nhau
	if (SDL_HasIntersection(&ball, &paddle)) {
		// goc nay bounce
		double bounce = calcBounceAngle(ball, paddle);

		// tinh van toc theo 2 chieu qua goc nay
		velY = -BALL_SPEED * cos(bounce);
		velX = BALL_SPEED * -sin(bounce);

	}

	if (collidewUpperBound(ball) == true)
	{
		//Neu phat hien cham bien tren, cho van toc theo chieu y dao nguoc de ball nay xuong
		velY = -velY;
	}

	//Neu ball roi xuong day man hinh ==> mat mang (liveCount tru di 1)
	// Reset lai vi tri pad o trung tam
	// Reset van toc cua ball theo 2 chieu va vi tri cua ball
	if (dropped(ball) == true)
	{
		resetPadnBall(paddle, ball);
		livesCount--;
	}

	// Neu phat hien cham bien trai hoac phai 
	// Van toc theo phuong x doi chieu
	if (collidewLeftBound(ball) || collidewRightBound(ball))
	{
		velX = -velX;
	}

	// "Khoa", khong cho pad di chuyen qua muc ve ben trai
	if (paddle.x < 0)
	{
		paddle.x = 0;
	}

	// "khoa", khong cho pad dich chuyen qua muc ve ben phai
	if (paddle.x + paddle.w > WIDTH)
	{
		paddle.x = WIDTH - paddle.w;
	}

	// Mat het mang thi reset lai brick va choi lai tu dau
	if (die(livesCount) == true)
	{
		score = 0;
		resetBricks();
		livesCount = 3;
	}

	bool reset = 1;

	//check qua tung brick mot de xem ball co cham vao cai nao ko
	for (int i = 0; i < COL * ROW; i++) {
		setBrick(i);
		
		//Neu co cham vao brick
		if (SDL_HasIntersection(&ball, &brick) && bricks[i]) {
			
			//cho brick die
			bricks[i] = 0;

			//an 1 brick duoc them diem
			score++;

			//SPACING = rand() % 64; //Bo comment de lam chaotic mode
			
			//bat dau check vi tri va cham de xac dinh chieu nay cho ball
			string sideLR = checkCollisionSideLR(ball, brick);
			string sideUD = checkCollisionSideUD(ball, brick);
			
			//cham theo chieu x (trai-phai) thi nay lai theo chieu x
			if (sideLR == "right" || sideLR == "left")
			{
				velX = -velX;
			}
			
			//cham theo chieu y (tren-duoi) thi nay lai theo chieu y
			if (sideUD == "up" || sideUD == "down")
			{
				velY = -velY;
			}
			
		}

		//danh dau co brick bi an
		if (bricks[i] == true)
		{
			reset = 0;
		}
	}

	//tat ca brick deu bi an het thi reset lai choi tiep (check het ma reset van bang 1 tuc la da an het)
	if (reset)
	{
		resetBricks();
	}
}

void input() {
	//	su kien e de "bat" cac su kien (chang han nguoi dung nhan phim nao do, nguoi dung di chuyen chuot, ...)
	SDL_Event e;

	//	keystates la con tro hang kieu Uint8* (1 kieu so nguyen) tro toi 1 mang chua trang thai "nhan hay chua nhan" cua cac phim
	//	Neu keystate[ phim nao do ] = 1 nghia la phim duoc nhan, = 0 nghia la chua duoc nhan
	const Uint8* keystates = SDL_GetKeyboardState(NULL);

	//	Kiem tra su kien lien tuc
	while (SDL_PollEvent(&e))
	{
		//	Neu phat hien co su kien thoat (o day la khi nguoi dung nhan phim ESC) thi cho dung chuong trinh
		if (e.type == SDL_QUIT) running = false;
	}
	
	//	Check trang thai phim
	// keystate[ cua phim ESC ] == 1 => ESC duoc nhan => dung chuong trinh (running = false);
	if (keystates[SDL_SCANCODE_ESCAPE]) running = false;

	//	keystate[ phim <- ] == 1 => phim <- duoc nhan => pad di chuyen ve ben trai
	// => Hoanh do x cua pad thay doi 1 gia tri bang toc do cua pad (hang so SPEED)
	//	tuong tu cho phim ->
	if (keystates[SDL_SCANCODE_LEFT]) paddle.x -= SPEED;
	if (keystates[SDL_SCANCODE_RIGHT]) paddle.x += SPEED;
}

//	Gioi han FPS de hinh anh ko chay qua nhanh
//	Cac tham so can truyen gom 
//	-	const int fps	: fps cua game, co dinh la 60
//	-	int framecount	: so luong khung hinh da render
//	-	int lastFrame	: so thu tu cua khung hinh cuoi cung duoc render (tinh tu luc bat dau khoi tao thu vien SDL)
//	-	int timerFPS	: thoi gian chenh lech tu luc khoi tao thu vien SDL toi khung hinh cuoi cung
//	Neu thoi gian chenh lech nho hon 1000/fps thi nghi 1 khoang nho de cho cho timerFPS lon hon gia tri 1000/fps (dung ham SDL_Delay)
void limitFPS(const int fps, int frameCount, int lastFrame, int timerFPS)
{
	frameCount++;
	timerFPS = SDL_GetTicks() - lastFrame;
	if (timerFPS < (1000 / fps))
	{
		SDL_Delay((1000 / fps) - timerFPS);
	}
}

void drawBrick(SDL_Rect& brick, int seed)
{
	setBrick(seed);
	SDL_RenderFillRect(renderer, &brick);
}

void render() {
	//Dat lai mau va clear but ve renderer sau moi luot render de ko de lai vet tren man hinh
	SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 255);
	SDL_RenderClear(renderer);
	
	//Gioi han fps
	limitFPS(fps, frameCount, lastFrame, timerFPS);

	// Dat mau cho but ve, o day la con tro cua game /renderer
	
	//SDL_SetRenderDrawColor(SDL_Renderer * renderer, int red, int green, int blue, int alpha);
	SDL_SetRenderDrawColor(renderer, color.r , color.g, color.b, 255);

	//	renderer chinh la con tro but ve cua game;
	//	but ve gom 3 tham so mau chinh la mau do (color.r), mau luc (color.g), mau lam (color.b)
	//	255 la gia tri mac dinh cho tham so alpha
	
	//Lap day hinh chu nhat bang mau da chon, HCN o day la pad va ball
	SDL_RenderFillRect(renderer, &paddle);
	SDL_RenderFillRect(renderer, &ball);

	// Viet so mang con lai len man hinh cho can 
	//	FONT_SIZE la hang so chi kich co cua font chu, gia tri la 32
	write("Left:" + toStr(livesCount), WIDTH -  + FONT_SIZE / 2, FONT_SIZE * 1.5);

	// Viet so diem len man hinh
	write("Score:" + toStr(score), 275, 50);
	
	brick_col.b = rand() % 256; //Bo cmt de lam brick nhap nhay :))
	
	//Duyet qua cac brick
	//Brick ve tu tren xuong, tu trai qua, so thu tu i cua brick cung vay
	for (int i = 0; i < COL * ROW; i++) {

		// Chinh lai mau cho but ve, mac dinh la mau do
		SDL_SetRenderDrawColor(renderer, brick_col.r, brick_col.g, brick_col.b, 255);
		
		// Dat cac hang le la mau luc (do hang dau la hang 1 ma i bat dau tu 0 nen de i % 2 == 0
		// Vi brick duyet tu tren xuong, tu trai qua nen se ra duoc cac hang xen ke mau (i chan mau xanh, i le mau do)
		if (i % 2 == 0)SDL_SetRenderDrawColor(renderer, brick_col.g, brick_col.r, brick_col.b, 255);

		//Brick con song thi ve ra
		if (bricks[i]) 
		{
			drawBrick(brick, i);
		}
	}

	// Chinh thuc render man hinh (cap nhat 1 frame)
	SDL_RenderPresent(renderer);
}

//	Ham dat kich thuoc cho pad (can truyen vao dia chi cua pad)
//	Pad la mot struct SDL_Rect (cau truc dang chu nhat) gom cac thuoc tinh chieu rong w, chieu cao h, hoanh do x, tung do y
void setPadSize(SDL_Rect& paddle)
{
	//	set chieu day h chieu dai w va tung do y cho pad. Cac thong so nay co dinh trong suot tro choi 
	//	Hoanh do x cua pad thay doi lien tuc qua input (pad chuyen dong)
	paddle.h = 15;
	paddle.w = HEIGHT / 4;
	paddle.y = HEIGHT - paddle.h - 32;
}

//	Ham dat kich thuoc cho ball (can truyen dia chi cua ball)
//	Ball cung la cau truc chu nhat giong pad nhung co w = h de tao thanh hinh vuong
void setBallSize(SDL_Rect& ball)
{
	// set kich thuoc cho ball, SIZE la hang so kich thuoc ball
	ball.w = ball.h = SIZE;
}

void createSDLWindow()
{
	// Khoi tao 1 cua so  va font ttf
	
	//Dat co phong khi tao cua so that bai
	if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
	{
		cout << "Failed at SDL_Init()" << endl;
	}
	if (SDL_CreateWindowAndRenderer(WIDTH, HEIGHT, 0, &window, &renderer) < 0)
	{
		cout << "Failed at SDL_CreateWindowAndRenderer())" << endl;
	}
	
	//Dat ten cho cua so
	SDL_SetWindowTitle(window, "Brick Breaker");

	//Khoi tao cac chuc nang lien quan den chu cai
	TTF_Init();
	font = TTF_OpenFont("font.ttf", FONT_SIZE);

}

void displayingMenu(EngineMenu engineMenu)
{
	SDL_Event event;
	SDL_PollEvent(&event);
	if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE) {
		running = false;
	}

	//clear the render
	SDL_RenderClear(renderer);

	//render the splash screen
	engineMenu.displaySplashScreen();

	//render the new texture
	SDL_RenderPresent(renderer);
}


int main(int argc, char* argv[])
{
	createSDLWindow();

	EngineMenu engineMenu(renderer, window);

	//init the splash screen
	engineMenu.initSplashScreen("Press SPACE to start", "font.ttf", "null.bmp");

	//1 = van dang chay, 0 = dung chuong trinh
	running = 1;

	//set mau cho pad va ball
	color.r = color.g = color.b = 255;

	//set mau cho brick
	brick_col.r = 255; brick_col.g = brick_col.b = 0;

	setPadSize(paddle);

	setBallSize(ball);


	//	set chieu day h va chieu dai w cho brick
	brick.w = (WIDTH - (SPACING * COL)) / COL;
	brick.h = 22;

	screen = SDL_GetWindowSurface(window);

	//khoi tao 1 ban brick moi
	resetBricks();



	SDL_Event e;
	/*
	while (SDL_PollEvent(&e) != 0)
		if (e.type == SDL_KEYDOWN)
		{
			if (e.key.keysym.sym == SDLK_SPACE)
			{
				break;
			}
		}
		else
	*/
	
	initMixer();
	int song = loadMusic("song.mp3");
	
	//hien thi menu va cho nhan phim space de bat dau
	do
	{
		playMusic(song);
		displayingMenu(engineMenu);
		SDL_Delay(10);
		SDL_PollEvent(&e);
	} while (e.key.keysym.sym != SDLK_SPACE);

	//main game lôp
	while (running) 
	{		
		lastFrame = SDL_GetTicks();
		//if (lastFrame >= (lastTime + 1000)) {
		//	lastTime = lastFrame;
		//	fps = frameCount;
		//	frameCount = 0;
		//}

		//song = loadMusic("song.mp3");
		//playMusic(song);

		update();
		input();
		render();
	}

	engineMenu.quitSplashScreen();

	//Xoa cua so va giai phong bo nho cac con tro khi xong
	TTF_CloseFont(font);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
	
	for (int s = 0; s < music.size(); s++) {
		Mix_FreeMusic(music[s]);
		music[s] = NULL;
	}
	Mix_Quit();

	return 0;
}