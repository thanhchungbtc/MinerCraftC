#pragma comment (linker, "/subsystem:windows /entry:mainCRTStartup")
#include "SDL.h"
#include "SDL_ttf.h"

#include "SDL_image.h"
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#define FPS  24;
#define CELL_SIZE 24

typedef enum {	
	OPENED,
	CLOSED,
	FLAG,
	BOMB,
	LOST
}CellState;
typedef enum{
	MINER_ICON,
	CELL_NUMBER,
	CLOCK,
	BOMB_COUNTER,
	MAIN_BUTTON,
	LEVEL_BUTTON,
	//BACKGROUND,
	TOTAL_STATE
}TextureSet;
typedef struct {
	int posX, posY;
	int state;
} Button;

typedef enum{
	LEFT, 
	RIGHT
} MouseButton;

typedef struct {
	CellState state;
	int value; // value == -1 means has mine, otherwise contains number of minesaround
	int x, y; // position at board
}Cell;

// board: variable to manage the collection of cells
Cell** board;
int boardRows, boardCols;
int numberOfMines; 
int minesRemain;
int boardPosX, boardPosY;
int spacing = 1; // space between 2 cells
//----------------------------------------------------------
int timerCounter;
TTF_Font* font;

int bRunning; // flag to indicate the game is running
int bGameOver;
int bWin;

int level;
int selectedLevel;
int levelTextWidth, levelTextHeight; // selected level text size, use to calculate position of next button

// window
SDL_Window* pWindow;
SDL_Renderer* pRenderer;
SDL_Texture* pTextures[TOTAL_STATE];
int textureCount;
int windowWidth, windowHeight;
Button mainButton;
Button previousButton;
Button nextButton;
//----------------------------



// variables for events
int mouseButtonStates[2];
int mousePosX, mousePosY;
int bReleased; // flag for left mouse button
int bReleased2; // flag for right mouse button
//----------------------------

void start();
void init();
void update();
void render();
void handleInput();
void clean();

void generateBoard(); // generate a board with random mines's positions
int calculateOneCell(Cell *aCell); // calculate a cell's value

void onMouseMotion(SDL_Event *e);
void onMouseButtonDown(SDL_Event *e);
void onMouseButtonUp(SDL_Event *e);
void onCellClicked();
void onMainButtonClick();
void onLevelButtonClick();

void openCell(Cell* aCell);
Cell* getClickedCell();
void onGameOver();
void clearLevel();

void drawTimerCounter(); // draw timer counter and mines remain(first row on the ui)
void drawImage(int x, int y, int width, int height, SDL_Texture* pTexture, int flip);
void drawCell(Cell* aCell);
void drawBoard();
void drawFrame(int x, int y, int width, int height, int frameID, SDL_Texture* pTexture, int flip);
void drawLevelBar(); // draw last row on the ui (level selection area)

void loadTexture(const char* fileName); // load necessary textures into texture manager pTextures
void freeBoard(); 

int main(int* argc, char** argv)
{
	unsigned int frameStart, frameTime;
	static const int DELAY_TIME = 1000/FPS;
	boardRows = 10;
	boardCols = 10;	
	srand(time(0));
	init();
	generateBoard();
	while (bRunning)
	{
		frameStart = SDL_GetTicks();	
		handleInput();
		update();		
		render();

		// sleep for a time
		frameTime = SDL_GetTicks() - frameStart;
		if (frameTime < DELAY_TIME)
		{
			SDL_Delay(DELAY_TIME - frameTime);
		}		
	}
	clean();	
	return 0;
}

void init()
{
	bRunning = 1; 	
	bReleased = 1;

	if (SDL_Init(SDL_INIT_EVERYTHING) == 0)
	{
		windowWidth = (CELL_SIZE + spacing) * boardCols + 8;
		windowHeight = (CELL_SIZE + spacing) * boardRows + 68;	
		boardPosX = windowWidth - (CELL_SIZE+spacing)*(boardCols) - 4;
		boardPosY = windowHeight - (CELL_SIZE + spacing)*(boardRows) - 34;

		mainButton.posX = (CELL_SIZE + spacing)*(boardCols/2);
		mainButton.posY = (CELL_SIZE + spacing)/2;

		previousButton.posX = 2;
		previousButton.posY = (CELL_SIZE + spacing)*boardRows + boardPosY + 5;

		pWindow = SDL_CreateWindow("Miner", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, windowWidth, windowHeight, 0);
		if (pWindow)
		{			
			SDL_Surface* icon = IMG_Load("images/icon.png");
			SDL_SetWindowIcon(pWindow, icon);
			SDL_FreeSurface(icon);
			pRenderer = SDL_CreateRenderer(pWindow, -1, SDL_RENDERER_ACCELERATED);
			if (pRenderer)
			{
				SDL_SetRenderDrawColor(pRenderer, 49, 98, 98, 255);
			}
			else
			{
				printf("Renderer init failed\n");
				return;
			}
		}
		else
		{
			printf("Window init failed\n");
			return;
		}
	}
	else
	{
		printf("SDL init failed\n");
		return;
	}
	if (TTF_Init() == -1)
	{
		printf("Font load error: %s", TTF_GetError());
	}
	font= TTF_OpenFont("images/arial.ttf", 12);
	TTF_SetFontStyle(font, TTF_STYLE_BOLD);
	textureCount = 0;;
	
	loadTexture("images/minericon.png");
	loadTexture("images/number.png");
	loadTexture("images/clock.png");
	loadTexture("images/bombcounter.png");
	loadTexture("images/mainbutton.png");
	loadTexture("images/levelbutton.png");
	//loadTexture("images/background.png");
}
void handleInput()
{
	SDL_Event e;
	while (SDL_PollEvent(&e))
	{
		switch (e.type)
		{
		case SDL_QUIT:		
			bRunning = 0;			
			break;
		case SDL_MOUSEMOTION:
			onMouseMotion(&e);
			break;
		case SDL_MOUSEBUTTONUP:
			onMouseButtonUp(&e);
			break;
		case SDL_MOUSEBUTTONDOWN:
			onMouseButtonDown(&e);
			break;
		default:
			break;
		}
	}
}
void onMouseMotion(SDL_Event *e)
{
	mousePosX = e->motion.x;
	mousePosY = e->motion.y;
}
void onMouseButtonDown(SDL_Event *e)
{

	if (e->button.button == SDL_BUTTON_LEFT)
	{
		mouseButtonStates[LEFT] = 1;		
	}
	else if (e->button.button == SDL_BUTTON_RIGHT)
	{
		mouseButtonStates[RIGHT] = 1;
	}
}
void onMouseButtonUp(SDL_Event *e)
{
	if (e->button.button == SDL_BUTTON_LEFT)
	{
		mouseButtonStates[LEFT] = 0;
	}
	if (e->button.button == SDL_BUTTON_RIGHT)
	{
		mouseButtonStates[RIGHT] = 0;
	}
}

void update()
{
	static int count = 0;
	// update timer
	if (!bGameOver)
	{
		count++;
		if (count == 24) // 24 FPS
		{
			++timerCounter;
			count = 0;
		}
	}
	
	if (mouseButtonStates[LEFT] && bReleased)
	{
		onMainButtonClick();
		onLevelButtonClick();
		if (!bGameOver)
		{
			onCellClicked();			
			if (bGameOver)
			{
				mainButton.state = 3;
			}									
		}		
		bReleased = 0;
	}
	else if (!mouseButtonStates[LEFT])
	{
		bReleased = 1;
		if (mainButton.state == 1)
		{
			mainButton.state = 0; // set icon to waiting
		}		
	}
	if (mouseButtonStates[RIGHT] && bReleased2)
	{
		if (!bGameOver)
		{
			Cell* clickedCell = getClickedCell();
			if (clickedCell)
			{
				if (clickedCell->state == CLOSED)
				{
					clickedCell->state = FLAG;			
					--minesRemain;
				}
				else if (clickedCell->state == FLAG)
				{
					clickedCell->state = CLOSED;
					++minesRemain;
				}
			}
			if (minesRemain == 0)
			{
				clearLevel(); // check if all bombs are detected
				if (bWin)
				{
					mainButton.state = 2; 
				}
			}
		}		
		bReleased2 = 0;
	}
	else if(!mouseButtonStates[RIGHT])
	{
		bReleased2 = 1;
	}
}
void render()
{		
	SDL_RenderClear(pRenderer);
	//drawImage(0,0, windowWidth, windowHeight, pTextures[BACKGROUND], 0); // back ground

	drawBoard(); // board
	drawTimerCounter(); // draw timer and mines remain
	// draw main button
	drawFrame(mainButton.posX, mainButton.posY, 18, 18, mainButton.state,pTextures[MAIN_BUTTON], 0);
	// draw level bar
	drawLevelBar();

	SDL_RenderPresent(pRenderer);	
}
void clean()
{
	int i;
	printf("Cleanning...\n");
	for (i = 0; i<textureCount; ++i)
	{
		SDL_DestroyTexture(pTextures[i]);
	}
	
	freeBoard();
	TTF_CloseFont(font);
	
	SDL_DestroyWindow(pWindow);
	SDL_DestroyRenderer(pRenderer);
	SDL_Quit();

	printf("Clean finished!\n");
}

int calculateOneCell(Cell *aCell)
{
	int i, j;
	int r, c;
	int mineCount = 0;

	// loop cells around
	for (i = -1; i<2; ++i)
	{
		r = aCell->y + i;
		if (r < 0 || r>= boardRows)
		{
			continue;
		}
		for (j = -1; j<2; ++j)
		{
			// do not process cell itself
			if (i == 0 && j == 0)
			{	
				continue;
			}
			// begin			
			c = aCell->x + j;
			// check bound
			if (c < 0 || c >= boardCols)
			{
				continue;
			}
			else
			{
				if (board[r][c].value == -1)
				{
					++mineCount;
				}
			}
		}
	}
	aCell->value = mineCount;
	return mineCount;
}
void generateBoard()
{
	int i, j;
	int numberOfCells; 
	
	numberOfMines = boardCols*boardRows*2/10; // 20% mines
	minesRemain = numberOfMines;
	numberOfCells = boardRows*boardCols;
	// allocate memory for board
	board = (Cell**)malloc(boardRows*sizeof(Cell*));
	for (i = 0; i<boardRows; ++i)
	{
		board[i] = (Cell*)malloc(boardCols*sizeof(Cell));
	}

	// init the board with mines number
	for (i = 0; i<numberOfMines; ++i)
	{
		board[i/boardCols][i%boardCols].value = -1;
	}
	// shuffle the board	
	for (i = 0; i<numberOfCells; ++i)
	{
		int index = rand()%numberOfCells;
		Cell temp = board[i/boardCols][i%boardCols];
		board[i/boardCols][i%boardCols] = board[index/boardCols][i%boardCols];
		board[index/boardCols][i%boardCols] = temp;
	}

	// calculate remain cells' values	
	for (i = 0; i<boardRows; ++i)
	{
		for (j = 0; j< boardCols; ++j)
		{
			// set cell's state to be closed
			board[i][j].state = CLOSED;
			// set the position
			board[i][j].x = j;
			board[i][j].y = i;

			// calculate one cell
			// do not process cell has mine
			if (board[i][j].value == -1)
			{
				continue;
			}
			calculateOneCell(&board[i][j]);
		}
	}
	
	bGameOver = 0;
}
void drawCell(Cell* aCell)
{
	// cell is opened and has at least 1 mine around, display number of mines around
	if (aCell->state == OPENED && aCell->value > 0)
	{	
		drawFrame(aCell->x*(CELL_SIZE+spacing) + boardPosX, aCell->y*(CELL_SIZE + spacing) + boardPosY,
			CELL_SIZE, CELL_SIZE, aCell->value - 1,pTextures[CELL_NUMBER], 0);
	}
	else // else display appropriate state
	{			 
		drawFrame(aCell->x*(CELL_SIZE+spacing) + boardPosX, aCell->y*(CELL_SIZE + spacing) + boardPosY,
			CELL_SIZE, CELL_SIZE, aCell->state ,pTextures[MINER_ICON], 0);
	}
}
void drawBoard()
{
	int i, j;
	for (i = 0; i<boardRows; ++i)
	{
		for (j = 0; j<boardCols; ++j)
		{
			drawCell(&board[i][j]);
		}
	}
}
void loadTexture(const char* fileName)
{
	SDL_Surface* pTemp;
	SDL_Texture* pTexture;	
	pTemp = IMG_Load(fileName);	
	pTexture = SDL_CreateTextureFromSurface(pRenderer, pTemp);
	SDL_FreeSurface(pTemp);
	
	pTextures[textureCount++] = pTexture;
}
void onCellClicked()
{
	Cell* clickedCell = getClickedCell();
	if (!clickedCell)
	{
		return;
	}
	mainButton.state = 1;
	if (clickedCell->state == OPENED || 
		clickedCell->state == FLAG)
	{
		return;
	}

	if (clickedCell->state == CLOSED)
	{
		if (clickedCell->value == -1) // click on cell has mine
		{
			// Game over;		
			onGameOver();
		}
		else
		{
			openCell(clickedCell);
		}
	}
}

Cell* getClickedCell()
{
	int x, y;
	if (mousePosX - boardPosX < 0 || mousePosY - boardPosY < 0)
	{
		return 0;
	}
	x = (mousePosX - boardPosX)/(CELL_SIZE + spacing);
	y = (mousePosY - boardPosY)/(CELL_SIZE + spacing);

	// check bound
	if (x < 0 || x >= boardCols ||
		y < 0 || y >= boardRows)
	{
		return 0;
	}
	return &board[y][x];	
}
void openCell(Cell* aCell)
{
	int i, j;
	int r, c;

	if (aCell->state == OPENED || aCell->state == FLAG)
	{
		return;
	}
	aCell->state = OPENED;

	// looking for cells around and open it if value = 0

	if (aCell->value == 0)
	{
		for (i = -1; i<2; ++i)
		{
			r = aCell->y + i;
			if (r < 0 || r>= boardRows)
			{
				continue;
			}
			for (j = -1; j<2; ++j)
			{
				// do not process cell itself
				if (i == 0 && j == 0)
				{	
					continue;
				}
				// begin			
				c = aCell->x + j;
				// check bound
				if (c < 0 || c >= boardCols)
				{
					continue;
				}
				else if (board[r][c].state != OPENED && board[r][c].state != FLAG)				
				{
					openCell(&board[r][c]);				
				}
			}
		}
	}
}

void onGameOver()
{
	int i, j;
	bGameOver = 1;
	bWin = 0;
	for (i = 0; i<boardRows; ++i)
	{
		for (j = 0; j<boardCols; ++j)
		{					
			if (board[i][j].value != -1 && board[i][j].state == FLAG)
			{
				board[i][j].state = LOST;										
			}
			else if (board[i][j].value == -1)
			{
				board[i][j].state = BOMB;
			}							
		}
	}	
}

void clearLevel() // won
{
	int i, j;
	for (i = 0; i<boardRows; ++i)
	{
		for (j = 0; j<boardCols; ++j)
		{
			if (board[i][j].state == FLAG && board[i][j].value != -1)
			{
				return; // do nothing 
			}
		}
	}
	// clear level
	bGameOver = 1;
	bWin = 1;	
	// Open remain cells
	for (i = 0; i<boardRows; ++i)
	{
		for (j = 0; j<boardCols; ++j)
		{
			if (board[i][j].state != OPENED)
			{
				board[i][j].state = OPENED;
			}
		}
	}
}

void drawImage(int x, int y, int width, int height, SDL_Texture* pTexture, int flip)
{
	SDL_Rect srcRect, destRect;
	srcRect.x = srcRect.y = 0;
	srcRect.w = destRect.w = width;
	srcRect.h = destRect.h = height;
	destRect.x = x;
	destRect.y = y;	
	SDL_RenderCopyEx(pRenderer, pTexture, &srcRect, &destRect, 0, 0, flip);	
}
void start()
{
	timerCounter = 0;
	bGameOver = 0;
	bWin = 0;
	mainButton.state = 0;	
	generateBoard();	
}

void drawTimerCounter()
{
	static char counterText[10];
	SDL_Color color = {255, 255, 255, 255};	

	SDL_Surface* pTemp;
	SDL_Texture* pTexture;
	itoa(timerCounter, counterText, 10);
		
	pTemp = TTF_RenderText_Solid(font, counterText, color);
	pTexture = SDL_CreateTextureFromSurface(pRenderer, pTemp);
	drawImage((CELL_SIZE + spacing)*(boardCols - 1), (CELL_SIZE + spacing)*1/2 + 2, pTemp->w, pTemp->h, pTexture, 0);
	
	// draw clock icon
	drawImage((CELL_SIZE + spacing)*(boardCols - 2), (CELL_SIZE + spacing)*1/2, 18, 18, pTextures[CLOCK], 0);
		
	SDL_FreeSurface(pTemp);
	SDL_DestroyTexture(pTexture);


	// draw mines remain
	itoa(minesRemain, counterText, 10);

	pTemp = TTF_RenderText_Solid(font, counterText, color);
	pTexture = SDL_CreateTextureFromSurface(pRenderer, pTemp);
	drawImage((CELL_SIZE + spacing)*(1), (CELL_SIZE + spacing)*1/2, pTemp->w, pTemp->h, pTexture, 0);

	drawImage((CELL_SIZE + spacing)*(1) - 20, (CELL_SIZE + spacing)*1/2 + 2, 18,18, pTextures[BOMB_COUNTER], 0);

	SDL_FreeSurface(pTemp);
	SDL_DestroyTexture(pTexture);
}
void drawLevelBar()
{
	static char levels[5][15] = {"Easy", "Medium", "Hard", "Super hard", "Crazy"};
	SDL_Surface* pTemp;
	SDL_Texture* pTexture;
	SDL_Color color = {255, 255, 255, 255};	// white
	pTemp = TTF_RenderText_Solid(font, levels[selectedLevel], color);
	pTexture = SDL_CreateTextureFromSurface(pRenderer, pTemp);
	levelTextWidth = pTemp->w;
	levelTextHeight = pTemp->h;
	
	// update next button position
	nextButton.posX = previousButton.posX + CELL_SIZE + levelTextWidth + 4;
	nextButton.posY = previousButton.posY;
	// draw previous button
	drawFrame(previousButton.posX, previousButton.posY, 18, 18, previousButton.state,pTextures[LEVEL_BUTTON], SDL_FLIP_HORIZONTAL);
	// draw selected level
	drawImage(previousButton.posX + CELL_SIZE, previousButton.posY, levelTextWidth, levelTextHeight, pTexture, 0);
	// draw next button
	drawFrame(nextButton.posX, nextButton.posY, 18, 18, nextButton.state,pTextures[LEVEL_BUTTON], 0);
	SDL_FreeSurface(pTemp);
	SDL_DestroyTexture(pTexture);

	// credit
	pTemp = TTF_RenderText_Solid(font, "Create By BTC", color);
	pTexture = SDL_CreateTextureFromSurface(pRenderer, pTemp);
	drawImage((CELL_SIZE + spacing)*(boardCols) - pTemp->w + boardPosX, previousButton.posY, pTemp->w, pTemp->h, pTexture,0);
	SDL_FreeSurface(pTemp);
	SDL_DestroyTexture(pTexture);
}

void drawFrame(int x, int y, int width, int height, int frameID, SDL_Texture* pTexture, int flip)
{
	SDL_Rect srcRect;
	SDL_Rect destRect;
	srcRect.x = frameID*width;
	srcRect.y = 0;
	destRect.x = x;
	destRect.y = y;
	destRect.w = width;
	destRect.h = height;
	srcRect.w = width;
	srcRect.h = height;

	SDL_RenderCopyEx(pRenderer, pTexture, &srcRect, &destRect, 0, 0, flip);
}

void onMainButtonClick()
{
	if (mousePosX > mainButton.posX && mousePosX < mainButton.posX + 18 &&
		mousePosY > mainButton.posY && mousePosY < mainButton.posY + 18)
	{
		if (selectedLevel != level)
		{
			level = selectedLevel;
			clean();
			// init level data, include boardRows and boardCols
			switch(level)
			{
			case 0: // easy 
				boardRows = 10;
				boardCols = 10;
				break;
			case 1: // medium
				boardCols = 16;
				boardRows = 12;
				break;
			case 2: // hard
				boardCols = 22;
				boardRows = 18;
				break;
			case 3: // super hard
				boardRows = 22;
				boardCols = 30;
				break;
			case 4: // crazy
				boardCols = 70;
				boardRows = 35;
				break;
			}
			init();
		}
		else
		{
			freeBoard();
		}			
		start();
	}
}
void onLevelButtonClick()
{
	// previous button click;
	if (mousePosX > previousButton.posX && mousePosX < previousButton.posX + 18 &&
		mousePosY > previousButton.posY && mousePosY < previousButton.posY + 18)
	{		
		if (selectedLevel == 0)
		{
			selectedLevel = 4;
		}else
			selectedLevel--;
	}

	// next button click;
	if (mousePosX > nextButton.posX && mousePosX < nextButton.posX + 18 &&
		mousePosY > nextButton.posY && mousePosY < nextButton.posY + 18)
	{
		if (selectedLevel == 4)
		{
			selectedLevel = 0;
		}else
			selectedLevel++;
	}
	
}

void freeBoard()
{
	int i;
	for (i = 0; i<boardRows; ++i)
	{
		free(board[i]);
	}
	free(board);

}