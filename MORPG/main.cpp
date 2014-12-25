#include <Windows.h>
#include <process.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <string>
#include <time.h>
#include <assert.h>
#include <conio.h>
#include <vector>
#include <stdlib.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>
using namespace std;

const int DELAY = 50;
const int WAIT_DELAY = 200;
const int WALK_DELAY = 250;
const int ACTION_DELAY = 2100;

// client DC: (0, 23) - (1600, 922) (1600, 983(partial black))
const int SAMPLE_WIDTH = 1600;
const int SAMPLE_HEIGHT = 900;
const int SAMPLE_GRID_WIDTH = 51;
const int SAMPLE_GRID_HEIGHT = 28;
const int SAMPLE_CENTER_X = 785;
const int SAMPLE_CENTER_Y = 420;
const int SAMPLE_RTICON_W = 60;	//right top
const int SAMPLE_RTICON_H = 60;
const int SAMPLE_RTICON_GAP = 4;
const int SAMPLE_RTICON_X0 = 1600;
const int SAMPLE_RTICON_Y = 32;
const int SAMPLE_RTICON_PET_X = 1579;	// checking if pet equipped
const int SAMPLE_RTICON_PET_Y = 68;
const int SAMPLE_RTICON_PET_R = 253;
const int SAMPLE_RTICON_PET_G = 202;
const int SAMPLE_RTICON_PET_B = 0;
const int SAMPLE_INVENTORY_NUMBER_X = 1423;
const int SAMPLE_INVENTORY_NUMBER_Y = 75;
const int SAMPLE_INVENTORY_NUMBER_W = 52;
const int SAMPLE_INVENTORY_NUMBER_H = 20;
const int SAMPLE_POSITION_X = 1087;
const int SAMPLE_POSITION_Y = 3;
const int SAMPLE_POSITION_W = 50;
const int SAMPLE_POSITION_H = 15;
const int SAMPLE_INVENTORY_X = 1326;
const int SAMPLE_INVENTORY_Y = 136-23;
const int SAMPLE_INVENTORY_W = 37;
const int SAMPLE_INVENTORY_H = 37;
const int SAMPLE_INVENTORY_TOP = 120;		// inventory open
const int SAMPLE_INVENTORY_BOTTOM = 317;
const int SAMPLE_HEADER_X = 472;
const int SAMPLE_HEADER_H = 25;
const int SAMPLE_NUMBER_W = 8;
const int SAMPLE_NUMBER_H = 11;
const int SAMPLE_HEAD = 25;
const int SAMPLE_TAIL = 25;

// typedef
typedef bool (*ColorCriteriaFunc)(COLORREF clr);
bool Black(COLORREF clr);

// function prototype
void GetPosition(int& x, int& y);
void GetInventorySpace(int& s1, int& s2);
void Resize(vector<vector<bool> >& vec, int row, int col);
void GetArray(vector<vector<bool> >& vec, HDC dc, int x, int y, int w, int h, ColorCriteriaFunc func);


HWND g_hWnd = 0;
BOOL CALLBACK FindMORPGWindowHandle(HWND hwnd, LPARAM lParam)
{
	char className[256];
	GetClassNameA(hwnd, className, sizeof(className));
	if (strcmp(className, "Chrome_RenderWidgetHostHWND") == 0)
	{
		g_hWnd = hwnd;
		return FALSE;
	}
	return TRUE;
}

int g_nWidth = 0;
int g_nHeight = 0;
void UpdateWindowSize()
{
	RECT rect;
	GetClientRect(g_hWnd, &rect);
	g_nWidth = rect.right - rect.left;
	g_nHeight = rect.bottom - rect.top;
	if (g_nWidth * SAMPLE_HEIGHT  >= g_nHeight * SAMPLE_WIDTH)	// width/height >= SAMPLE_WIDTH/SAMPLE_HEIGHT
	{
		//part of right is black
		g_nWidth = g_nHeight * SAMPLE_WIDTH / SAMPLE_HEIGHT;
	}
	else
	{
		//part of bottom is black
		g_nHeight = g_nWidth * SAMPLE_HEIGHT / SAMPLE_WIDTH;
	}
}

void Key(char ch)
{
	PostMessage(g_hWnd, WM_KEYDOWN, ch, 0);
	Sleep(DELAY);
	PostMessage(g_hWnd, WM_KEYUP, ch, 0);
}

void Click(int x, int y)
{
	SendMessage(g_hWnd, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(x, y));
	Sleep(DELAY);
	SendMessage(g_hWnd, WM_LBUTTONUP, MK_LBUTTON, MAKELPARAM(x, y));
}

void Walk(char ch, int step)
{
	for (int i = 0; i < step; i++)
	{
		Key(ch);
		Sleep(WALK_DELAY);
	}
}

void WalkDown(int step = 1){ Walk('S', step); }
void WalkLeft(int step = 1){ Walk('A', step); }
void WalkRight(int step = 1){ Walk('D', step); }
void WalkUp(int step = 1){ Walk('W', step); }
void WalkTo(int x, int y)
{
	int x_now, y_now;
	int x_last = x, y_last = y;
	int fail = 0;
	do
	{
		GetPosition(x_now, y_now);
		if (x_now == x_last && y_now == y_last)
		{
			fail++;
			if (fail > 3)
			{
				MessageBoxA(0, "Can not find a shortest path to the destination", "Warning", MB_OK|MB_TOPMOST);
				return;
			}
		}
		if (x > x_now)
			WalkRight();
		if (x < x_now)
			WalkLeft();
		if (y > y_now)
			WalkUp();
		if (y < y_now)
			WalkDown();
		x_last = x_now;
		y_last = y_now;
	}
	while (x_now != x || y_now != y);
}
void WalkToRelative(int dx, int dy /* Up: plus; Down: minus */, int additional_steps = 0)
{
	UpdateWindowSize();
	int grid_width = SAMPLE_GRID_WIDTH * g_nWidth / SAMPLE_WIDTH;
	int grid_height = SAMPLE_GRID_HEIGHT * g_nHeight / SAMPLE_HEIGHT;
	int center_x = SAMPLE_CENTER_X * g_nWidth / SAMPLE_WIDTH;
	int center_y = SAMPLE_CENTER_Y * g_nHeight / SAMPLE_HEIGHT;
	Click(center_x + (dx + dy) * grid_width, center_y - (dy - dx) * grid_height);
	Sleep(WALK_DELAY * (abs(dx) + abs(dy) + additional_steps));
}

typedef bool (*CriteriaFunc)(void*);
bool InventoryZero(void*)
{
	int s1, s2;
	GetInventorySpace(s1, s2);
	return s1 == 0;
}
bool InventoryGreater(void* arg)
{
	int num = (int)arg;
	int s1, s2;
	GetInventorySpace(s1, s2);
	return s1 > num;
}
bool InventoryShow(void*)
{
	UpdateWindowSize();
	int y1 = SAMPLE_INVENTORY_TOP * g_nHeight / SAMPLE_HEIGHT;
	int y2 = SAMPLE_INVENTORY_BOTTOM * g_nHeight / SAMPLE_HEIGHT;
	int h = y2 - y1 + 1;
	vector<vector<bool> > right_most_line;
	Resize(right_most_line, h, 1);
	HDC dc = GetDC(g_hWnd);
	GetArray(right_most_line, dc, g_nWidth - 1, y1, 1, h, Black);
	ReleaseDC(g_hWnd, dc);
	int black_count = 0;
	for (int i = 0; i < h; i++)
	{
		if (right_most_line[i][0])
			black_count++;
	}
	return black_count > 0.8 * h;
}
bool InventoryHide(void*){ return !InventoryShow(NULL); }
bool Wait(CriteriaFunc func, void* arg = 0, int timeout = -1)
{
	clock_t start = clock();
	while (true)
	{
		if (func(arg))
			return true;
		if (timeout > 0 && clock() - start > timeout)
			return false;
		Sleep(WAIT_DELAY);
	}
}

void LoadToPet(){ Key('E'); Sleep(16 * DELAY); }
void UnloadFromPet(){ Key('T'); Sleep(16 * DELAY); }
void PutToChest(){ Key('Q'); Wait(InventoryGreater, (void*)30, 1000); }
void GetFromChest(){ Key('G'); Wait(InventoryZero, NULL, 1000); }
void OpenInventory(){ if (!InventoryShow(NULL)){ Key('B'); Wait(InventoryShow, NULL, 1000); } }
void CloseInventory(){ if (InventoryShow(NULL)) { Key('B'); Wait(InventoryHide, NULL, 1000); } }

void Debug(const vector<vector<bool> >& vec)
{
	int h = vec.size();
	int w = vec.front().size();
	int band = 0;
	const int BAND = 79;
	while (band < w)
	{
		cout << "pos: " << band << endl;
		for (int row = 0; row < h; row++)
		{
			for	(int col = band; col < min(band + BAND, w); col++)
			{
				cout << vec[row][col];
			}
			cout << endl;
		}
		band += BAND;
	}
	ofstream output("debug.txt");
	for (int row = 0; row < h; row++)
	{
		for	(int col = 0; col < w; col++)
		{
			output << vec[row][col];
		}
		output << endl;
	}
	output.close();
}

int GetDigit(const vector<vector<bool> >& digit)
{
	static bool init = false;
	static vector<vector<bool> > num[10];
	static vector<vector<bool> > numb[10];
	if (!init)
	{
		for (int i = 0; i <= 9; i++)
		{
			num[i].resize(SAMPLE_NUMBER_H);
			for (int row = 0; row < SAMPLE_NUMBER_H; row++)
				num[i][row].resize(SAMPLE_NUMBER_W);

			ifstream input("digit\\" + to_string(i));
			for (int row = 0; row < SAMPLE_NUMBER_H; row++)
			{
				for (int col = 0; col < SAMPLE_NUMBER_W; col++)
				{
					char ch;
					input >> ch;
					num[i][row][col] = (ch == '1');
				}
			}
			input.close();
			ifstream inputb("digit\\" + to_string(i) + "b");
			if (!inputb.is_open())
				continue;
			numb[i].resize(SAMPLE_NUMBER_H);
			for (int row = 0; row < SAMPLE_NUMBER_H; row++)
				numb[i][row].resize(SAMPLE_NUMBER_W);
			for (int row = 0; row < SAMPLE_NUMBER_H; row++)
			{
				for (int col = 0; col < SAMPLE_NUMBER_W; col++)
				{
					char ch;
					inputb >> ch;
					numb[i][row][col] = (ch == '1');
				}
			}
			inputb.close();
		}
		init = true;
	}
	
	int digit_h = digit.size();
	int digit_w = digit.front().size();
	int min_diff = SAMPLE_NUMBER_W * SAMPLE_NUMBER_H;
	int maxarg_n = -1;
	for (int n = 0; n <= 9; n++)
	{
		// compare num[n] with digit
		int diff = 0;
		int diffb = 0;
		for (int row = 0; row < SAMPLE_NUMBER_H; row++)
		{
			for (int col = 0; col < SAMPLE_NUMBER_W; col++)
			{
				bool src = num[n][row][col];
				bool dest = digit[row * digit_h / SAMPLE_NUMBER_H][col * digit_w / SAMPLE_NUMBER_W];
				if (src != dest)
					diff++;
				if (!numb[n].empty())
				{
					bool srcb = numb[n][row][col];
					if (srcb != dest)
						diffb++;
				}
			}
		}
		if (diff < min_diff)
		{
			min_diff = diff;
			maxarg_n = n;
		}
		if (!numb[n].empty() && diffb < min_diff)
		{
			min_diff = diffb;
			maxarg_n = n;
		}
	}
	return maxarg_n;
}
bool White(COLORREF clr)
{
	int r = GetRValue(clr);
	int g = GetGValue(clr);
	int b = GetBValue(clr);
	return (r > 160 && g > 160 && b > 160);
}
bool BrightWhite(COLORREF clr)
{
	int r = GetRValue(clr);
	int g = GetGValue(clr);
	int b = GetBValue(clr);
	return (r > 192 && g > 192 && b > 192);
}
bool Black(COLORREF clr)
{
	int r = GetRValue(clr);
	int g = GetGValue(clr);
	int b = GetBValue(clr);
	return (r < 64 && g < 64 && b < 64);
}
bool Yellow(COLORREF clr)
{
	int r = GetRValue(clr);
	int g = GetGValue(clr);
	int b = GetBValue(clr);
	return (r > 128 && g > 128 && b < 64);
}
void GetArray(vector<vector<bool> >& test, HDC dc, int x, int y, int w, int h, ColorCriteriaFunc func)
{
	for (int i = 0; i < w; i++)
	{
		for (int j = 0; j <  h; j++)
		{
			COLORREF clr = GetPixel(dc, x + i, y + j);
			if (func(clr))
				test[j][i] = true;
		}
	}
}
bool CheckPetIcon()
{
	//RTICON rightest one (1579, 68-23) = RGB(253, 202, 0)
	//otherwise, RGB(127,127,127)
	UpdateWindowSize();
	HDC dc = GetDC(g_hWnd);
	COLORREF clr = GetPixel(dc, SAMPLE_RTICON_PET_X * g_nWidth / SAMPLE_WIDTH, SAMPLE_RTICON_PET_Y * g_nHeight / SAMPLE_HEIGHT);
	ReleaseDC(g_hWnd, dc);
	int r = GetRValue(clr);
	int g = GetGValue(clr);
	int b = GetBValue(clr);
	return r > 128 && g > 64 && b < 64;
}
void Resize(vector<vector<bool> >& test, int row, int col)
{
	test.resize(row);
	for (int i = 0; i < row; i++)
		test[i].resize(col);
}
int GetLeftMost(const vector<vector<bool> >& test)
{
	int rows = test.size();
	int cols = test.front().size();
	for (int c = 0; c < cols; c++)
	{
		for (int r = 0; r < rows; r++)
		{
			if (test[r][c])
				return c;
		}
	}
	return -1;
}
int GetRightMost(const vector<vector<bool> >& test)
{
	int rows = test.size();
	int cols = test.front().size();
	for (int c = cols - 1; c >= 0; c--)
	{
		for (int r = 0; r < rows; r++)
		{
			if (test[r][c])
				return c;
		}
	}
	return -1;
}
int GetTopMost(const vector<vector<bool> >& test)
{
	int rows = test.size();
	int cols = test.front().size();
	for (int r = 0; r < rows; r++)
	{
		for (int c = 0; c < cols; c++)
		{
			if (test[r][c])
				return r;
		}
	}
	return -1;
}
int GetBottomMost(const vector<vector<bool> >& test)
{
	int rows = test.size();
	int cols = test.front().size();
	for (int r = rows - 1; r >= 0; r--)
	{
		for (int c = 0; c < cols; c++)
		{
			if (test[r][c])
				return r;
		}
	}
	return -1;
}
void Copy(const vector<vector<bool> >& src, int x, int y, vector<vector<bool> >& dst)
{
	assert(src.size() >= y + dst.size());
	assert(src.size() > 0 && dst.size() > 0);
	assert(src.front().size() >= x + dst.front().size());
	int h = dst.size();
	int w = dst.front().size();
	for (int i = 0; i < w; i++)
	{
		for (int j = 0; j < h; j++)
		{
			dst[j][i] = src[y+j][x+i];
		}
	}
}
void Clear(vector<vector<bool> >& test, int x, int y, int w, int h)
{
	assert((int)test.size() >= y + h);
	assert((int)test.size() > 0 && (int)test.front().size() >= x + w);
	for (int i = 0; i < w; i++)
	{
		for (int j = 0; j < h; j++)
		{
			test[y+j][x+i] = 0;
		}
	}
}

void GetNumberPair(const vector<vector<bool> >& vec, int& n1, int& n2)
{
	vector<vector<bool> > copy = vec;
	int num[4] = {0};
	int num_x[4] = {0};
	int num_y1 = GetTopMost(vec);
	int num_y2 = GetBottomMost(vec);
	int num_h = num_y2 - num_y1 + 1;
	int num_w = num_h * SAMPLE_NUMBER_W / SAMPLE_NUMBER_H;
	vector<vector<bool> > digit;
	Resize(digit, num_h, num_w);
	for (int i = 0; i < 4; i++)
	{
		num_x[i] = GetLeftMost(copy);
		if (num_x[i] < 0)
			break;
		Copy(copy, num_x[i], num_y1, digit);
		Debug(digit);
		num[i] = GetDigit(digit);
		if (num[i] == 1)
			num_x[i] -= 2;	// the number 1 bitmap starts at x = 2
		else if (num[i] == 8)
			num_x[i] -= 1;	// the number 8 bitmap starts at x = 1
		Clear(copy, num_x[i], num_y1, num_w, num_h);
		Debug(copy);
	}
	if (num_x[3] > 0)
	{
		n1 = num[0] * 10 + num[1];
		n2 = num[2] * 10 + num[3];
	}
	else if (num_x[2] > 0)
	{
		// 3 digits; 2 package + 1 pet or 1 package + 2 pet
		if (num_x[2] - num_x[1] > num_x[1] - num_x[0])	//2 package + 1 pet
		{
			n1 = num[0] * 10 + num[1];
			n2 = num[2];
		}
		else
		{
			n1 = num[0];
			n2 = num[1] * 10 + num[2];
		}
	}
	else
	{	
		n1 = num[0];
		n2 = num[1];
	}
}

void GetInventorySpace(int& s1, int& s2)
{	
	UpdateWindowSize();
	int inventory_x = SAMPLE_INVENTORY_NUMBER_X * g_nWidth / SAMPLE_WIDTH;
	int inventory_y = SAMPLE_INVENTORY_NUMBER_Y * g_nHeight / SAMPLE_HEIGHT;
	int inventory_w = SAMPLE_INVENTORY_NUMBER_W * g_nWidth / SAMPLE_WIDTH + 1;
	int inventory_h = SAMPLE_INVENTORY_NUMBER_H * g_nHeight / SAMPLE_HEIGHT + 1;
	vector<vector<bool> > inventory;
	Resize(inventory, inventory_h, inventory_w);
	HDC dc = GetDC(g_hWnd);
	GetArray(inventory, dc, inventory_x, inventory_y, inventory_w, inventory_h, Yellow);
	ReleaseDC(g_hWnd, dc);
	GetNumberPair(inventory, s1, s2);
}

int Diff(const vector<vector<bool> >& src, vector<vector<bool> >& dst)
{
	assert(!src.empty() && !dst.empty());
	UpdateWindowSize();
	int w = dst.front().size();
	int h = dst.size();
	int diff = 0;
	for (int i = 0; i < h; i++)
	{
		for (int j = 0; j < w; j++)
		{
			// map dst[i][j] to src
			if (src[i * g_nHeight / SAMPLE_WIDTH][j * g_nWidth / SAMPLE_WIDTH])
				diff++;
		}
	}
	return diff;
}

void Read(const string& filename, vector<vector<bool> >& vec)
{
	ifstream input(filename);
	if (!input.is_open())
	{
		MessageBoxA(0, ("Can not open file " + filename).c_str(), "Error", MB_OK|MB_TOPMOST);
		exit(0);
	}
	int rows = vec.size();
	assert(rows > 0);
	int cols = vec.front().size();
	for (int row = 0; row < rows; row++)
	{
		for (int col = 0; col < cols; col++)
		{
			char ch;
			input >> ch;
			vec[row][col] = (ch == '1');
		}
	}
	input.close();
}


void GetPosition(int& x, int& y)
{
	static bool debug = true;	// note: the position might be affected by Status, Quest, Time, ...etc
	UpdateWindowSize();
	const int SAMPLE_HEADER_X = 472;
	const int SAMPLE_HEADER_H = 25;
	int header_x = SAMPLE_HEADER_X * g_nWidth / SAMPLE_WIDTH;
	int header_w = (SAMPLE_WIDTH - SAMPLE_HEADER_X) * g_nWidth / SAMPLE_WIDTH;
	int header_h = SAMPLE_HEADER_H * g_nHeight / SAMPLE_HEIGHT;
	vector<vector<bool> > header;
	Resize(header, 1, header_w);
	HDC dc = GetDC(g_hWnd);
	GetArray(header, dc, header_x, header_h / 2 - 1, header_w, 1, BrightWhite);
	// trim blank in the front
	int first_white = -1;
	for (int i = 0; i < header_w; i++)
	{
		if (header[0][i])
		{
			first_white = i;
			break;
		}
	}
	// seek first long blacks (between Exp and Location)
	int last_white = first_white;
	for (int i = first_white; i < header_w; i++)
	{
		if (header[0][i])
		{
			int blacks = i - last_white;
			last_white = i;
			if (blacks >= 50)
			{
				break;
			}
		}
	}
	header_x += last_white - 3;
	// seek second long blacks (between Location and Quest)
	int start_white = last_white - 3;
	for (int i = last_white; i < header_w; i++)
	{
		if (header[0][i])
		{
			int blacks = i - last_white;
			if (blacks >= 50)
			{
				break;
			}
			last_white = i;
		}
	}
	header_w = last_white - start_white + 3;
	Resize(header, header_h, header_w);
	GetArray(header, dc, header_x, 0, header_w, header_h, BrightWhite);
	ReleaseDC(g_hWnd, dc);

	int top = GetTopMost(header);
	int bottom = GetBottomMost(header);
	int left = GetLeftMost(header);
	int right = GetRightMost(header);
	vector<vector<bool> > location;
	Resize(location, bottom - top + 1, right - left + 1);
	Copy(header, left, top, location);

	// get rid of parenthesis
	assert(!location.empty());
	int w = location.front().size();
	int h = location.size();
	int last_row = h - 1;
	for (int i = 0; i < w; i++)
	{
		if (location[last_row][i])
		{
			Clear(location, 0, 0, i+1, h);
			break;
		}
	}
	for (int i = w - 1; i >= 0; i--)
	{
		if (location[last_row][i])
		{
			Clear(location, i, 0, w - i, h);
			break;
		}
	}

	// get rid of comma
	int comma_x = -1;
	bottom = GetBottomMost(location);
	for (int i = 0; i < w; i++)
	{
		if (location[bottom][i])
		{
			Clear(location, i, 0, 1, h);
			comma_x = i;
		}
	}

	bottom = GetBottomMost(location);

	GetNumberPair(location, x, y);
}

void SelectInventory(int row, int col)
{
	UpdateWindowSize();
	OpenInventory();
	int x = SAMPLE_INVENTORY_X * g_nWidth / SAMPLE_WIDTH;
	int y = SAMPLE_INVENTORY_Y * g_nHeight / SAMPLE_HEIGHT;
	int w = SAMPLE_INVENTORY_W * g_nWidth / SAMPLE_WIDTH;
	int h = SAMPLE_INVENTORY_H * g_nHeight / SAMPLE_HEIGHT;
	Click(x+col*w, y+row*h);
	Sleep(DELAY);
	CloseInventory();
}

bool CheckAntiBot()
{
	UpdateWindowSize();
	HDC dc = GetDC(g_hWnd);
	COLORREF clr = GetPixel(dc, g_nWidth / 2, g_nHeight / 2 + 120);
	ReleaseDC(g_hWnd, dc);
	return clr == 0x333333;
}

void WaitCooking()
{
	for (int i = 0; i < 38; i++)
	{
		Sleep(ACTION_DELAY);
		if (CheckAntiBot())
		{
			MessageBoxA(0, "Antibot Dialog Detect!", "Warning", MB_OK|MB_TOPMOST);
			Sleep(ACTION_DELAY);
		}
	}
}

void WaitInventoryFull(){}
void CutFir()
{
	// Dorpat
	// from Chest (22, 18) to Fir (24, 27)
	WalkToRelative(2, 9, 6);	// x: +3-3
	WaitInventoryFull();

	LoadToPet();
	WalkUp();	//continue to cut
	WaitInventoryFull();

	// from Fir (24, 26) to Chest (22, 17)
	WalkToRelative(-2, -9, 6);	// x: -3+3
	PutToChest();
	UnloadFromPet();
	PutToChest();
}

void Cook()
{
	// stand at Dorpat (21, 17), campfir at (17, 18), Chest at (22, 18)
	// i.e. campfire at (-4, 1), chest at (4, 1)
	// preparation: 
	//	1. chest item select one raw food (with lots of instances)
	//	2. no open inventory
	//	3. open chest
	//	4. stand at (21, 17) is suggested
	int x, y;
	GetPosition(x, y);
	cout << "Position at (" << x << ", " << y << ")\n";
	if (x != 21 || y != 17)
	{
		MessageBoxA(0, "Suggest initial location at Dorpat (21, 17)", "Warning", MB_OK|MB_TOPMOST);
	}
	int s1, s2;
	GetInventorySpace(s1, s2);
	cout << "Inventory Space is " << s1 << " / " << s2 << "\n";
	if (s1 >= 35)
	{
		// try to access to chest and get raw food out
		WalkTo(21, 17);
	}
	return;
	if (x == 22 && y == 1)
	int s1, s2;
	GetInventorySpace(s1, s2);
	if (s1 >= 30)
	{
		cout << "GetFormChest...";
		GetFromChest();
		cout << "Done\n";
	}
	cout << "SelectInventory...";
	SelectInventory(0, 2);
	cout << "Done\n";
	cout << "WalkTo Campfire...";
	WalkTo(-4, 1);
	cout << "Done\n";
	cout << "Wait cooking...";
	WaitCooking();
	cout << "Done\n";
	cout << "WalkTo Chest...";
	WalkTo(4, 1);
	cout << "Done\n";
	cout << "PutToChest...";
	PutToChest();
	cout << "Done\n";
}

int main()
{
	//find the target HWND
	HWND hWndParent = FindWindowA("Chrome_WidgetWin_0", 0);
	if (hWndParent == 0)
	{
		MessageBoxA(0, "Can not find the specific Windows; please make sure the game window is not mininmized.", "Error", MB_OK|MB_TOPMOST);
		return 0;
	}
	EnumChildWindows(hWndParent, FindMORPGWindowHandle, 0);
	if (g_hWnd == 0)
		return 0;
	Cook();
	system("pause");
	return 0;
}
