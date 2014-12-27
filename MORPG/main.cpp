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

const bool DEBUG = true;
bool g_2XPCheck = true;
bool g_No2XPCheck = false;

const int DELAY = 50;
const int WAIT_DELAY = 200;
const int WALK_DELAY = 200;
const int ACTION_DELAY = 2200;

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
const int SAMPLE_SELLALL_X = 837;
const int SAMPLE_SELLALL_Y = 591-23;
const int SAMPLE_2X_X = 1383;
const int SAMPLE_2X_Y = 71 - 23;
const int SAMPLE_2X_R = 255;
const int SAMPLE_2X_G = 165;
const int SAMPLE_2X_B = 0;


// typedef
typedef bool (*ColorCriteriaFunc)(COLORREF clr);
bool Black(COLORREF clr);

// function prototype
bool Check2XP();
bool CheckAntiBot();
void GetArray(vector<vector<bool> >& vec, HDC dc, int x, int y, int w, int h, ColorCriteriaFunc func);
void GetInventorySpace(int& s1, int& s2);
void GetPosition(int& x, int& y);
void Resize(vector<vector<bool> >& vec, int row, int col);


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
	int x_last = -1, y_last = -1;
	clock_t start = clock();
	do
	{
		Check2XP();
		CheckAntiBot();
		GetPosition(x_now, y_now);
		if (x_now == x_last && y_now == y_last)
		{
			if (clock() - start > 15000)
			{
				UINT id = MessageBoxA(0, "Not moved for 15 secs...", "Warning", MB_OK|MB_TOPMOST);
				start = clock();
				continue;
			}
			else
			{
				Sleep(WAIT_DELAY);
				continue;
			}
		}
		else
		{
			start = clock();
			cout << "WalkTo (" << x_now << ", " << y_now << ")" << endl;
		}
		if (x > x_now)
		{
			WalkRight(x - x_now);
		}
		if (x < x_now)
		{
			WalkLeft(x_now - x);
		}
		if (y > y_now)
		{
			WalkUp(y - y_now);
		}
		if (y < y_now)
		{
			WalkDown(y_now - y);
		}
		x_last = x_now;
		y_last = y_now;
	}
	while (x_now != x || y_now != y);
}
void WalkTo(const vector<pair<int,int> >& locations)
{
	for (auto it = locations.begin(); it != locations.end(); it++)
		WalkTo((*it).first, (*it).second);
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
bool LocationAt(void* loc)
{
	int x = ((int)loc) / 100;
	int y = ((int)loc) % 100;
	int x2, y2;
	GetPosition(x2, y2);
	return x == x2 && y == y2;
}
bool InventoryZero(void*)
{
	int s1, s2;
	GetInventorySpace(s1, s2);
	return s1 == 0;
}
bool PetInventoryEqual(void* arg)
{
	int num = (int)arg;
	int s1, s2;
	GetInventorySpace(s1, s2);
	return s2 == num;
}
bool InventoryEqual(void* arg)
{
	int num = (int)arg;
	int s1, s2;
	GetInventorySpace(s1, s2);
	return s1 == num;
}
bool InventoryRange(void* arg)
{
	int num = (int)arg;
	int r1 = num / 100;
	int r2 = num % 100;
	int s1, s2;
	GetInventorySpace(s1, s2);
	return r1 <= s1 && s1 <= r2;;
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
		{
			MessageBoxA(0, "Wait timeout!", "Warning", MB_OK|MB_TOPMOST);
			return false;
		}
		Sleep(WAIT_DELAY);
	}
}

void LoadToPet(){ Key('E'); Wait(PetInventoryEqual, (void*)0, 4000); }
void UnloadFromPet(){ Key('T'); Wait(PetInventoryEqual, (void*)16, 4000); }
void PutToChest(){ Key('Q'); Wait(InventoryRange, (void*)(36 * 100 + 38), 4000); }
void GetFromChest(){ Key('G'); Wait(InventoryZero, NULL, 4000); }
void OpenInventory(){ if (!InventoryShow(NULL)){ Key('B'); Wait(InventoryShow, NULL, 1000); } }
void CloseInventory(){ if (InventoryShow(NULL)) { Key('B'); Wait(InventoryHide, NULL, 1000); } }

// mouse (note: would active the game window)
void SellAll()
{
	UpdateWindowSize();
	int sellall_x = SAMPLE_SELLALL_X * g_nWidth / SAMPLE_WIDTH;
	int sellall_y = SAMPLE_SELLALL_Y * g_nHeight / SAMPLE_HEIGHT;
	Click(sellall_x, sellall_y);
	//Wait(InventoryRange, (void*)(36 * 100 + 39), 4000);
}


bool Valid(const vector<vector<bool> >& vec, int row, int col)
{
	return (row >= 0 && row < (int)vec.size() && !vec.empty() && col >= 0 && col < (int)vec[row].size());
}

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
	assert(!dst.empty());
	int h = dst.size();
	int w = dst.front().size();
	for (int i = 0; i < w; i++)
	{
		for (int j = 0; j < h; j++)
		{
			int row = y+j;
			int col = x+i;
			if (Valid(src, row, col))
				dst[j][i] = src[row][col];
		}
	}
}
void Clear(vector<vector<bool> >& vec, int x, int y, int w, int h)
{
	for (int i = 0; i < w; i++)
	{
		for (int j = 0; j < h; j++)
		{
			int row = y+j;
			int col = x+i;
			if (Valid(vec, row, col)) 
				vec[row][col] = 0;
		}
	}
}

void GetNumberPair(const vector<vector<bool> >& vec, int& n1, int& n2)
{
	n1 = n2 = -1;

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
		num[i] = GetDigit(digit);
		if (num[i] == 1)
			num_x[i] -= 2;	// the number 1 bitmap starts at x = 2
		else if (num[i] == 8)
			num_x[i] -= 1;	// the number 8 bitmap starts at x = 1
		Clear(copy, num_x[i], num_y1, num_w, num_h);
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
	bool pet = CheckPetIcon();
	if (!pet)
		inventory_x = (SAMPLE_INVENTORY_NUMBER_X + SAMPLE_RTICON_W + SAMPLE_RTICON_GAP) * g_nWidth / SAMPLE_WIDTH;
	HDC dc = GetDC(g_hWnd);
	GetArray(inventory, dc, inventory_x, inventory_y, inventory_w, inventory_h, Yellow);
	ReleaseDC(g_hWnd, dc);
	GetNumberPair(inventory, s1, s2);
	if (!pet && s2 >= 0)
	{
		s1 = 10 * s1 + s2;
		s2 = -1;
	}
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

bool AntiBot()
{
	UpdateWindowSize();
	HDC dc = GetDC(g_hWnd);
	COLORREF clr = GetPixel(dc, g_nWidth / 2, g_nHeight / 2 + 120);
	ReleaseDC(g_hWnd, dc);
	return clr == 0x333333;
}
bool CheckAntiBot()
{
	if (AntiBot())
	{
		MessageBoxA(0, "Antibot Dialog Detect!", "Warning", MB_OK|MB_TOPMOST);
		return true;
	}
	return false;
}

bool DoubleXP()
{
	UpdateWindowSize();
	int doublexp_x1 = SAMPLE_2X_X * g_nWidth / SAMPLE_WIDTH;
	int doublexp_x2 = (SAMPLE_2X_X - SAMPLE_RTICON_GAP - SAMPLE_RTICON_W) * g_nWidth / SAMPLE_WIDTH;
	int doublexp_y = SAMPLE_2X_Y * g_nHeight / SAMPLE_HEIGHT;
	HDC dc = GetDC(g_hWnd);
	COLORREF clr1 = GetPixel(dc, doublexp_x1, doublexp_y);
	COLORREF clr2 = GetPixel(dc, doublexp_x2, doublexp_y);
	ReleaseDC(g_hWnd, dc);
 	COLORREF clr = RGB(SAMPLE_2X_R, SAMPLE_2X_G, SAMPLE_2X_B);
	return (clr == clr1 || clr == clr2);
}
bool Check2XP()
{
	if (g_2XPCheck && DoubleXP())
	{
		MessageBoxA(0, "Double XP Detect!", "Cheering", MB_OK|MB_TOPMOST);
		return true;
	}
	if (g_No2XPCheck && !DoubleXP())
	{
		MessageBoxA(0, "No Double XP Detect!", "Cheering", MB_OK|MB_TOPMOST);
		return true;
	}
	return false;
}

void WaitActing(const string& act, bool consuming = false)
{
	cout << "Count Down...\n";
	int s1_last = -1, s2_last = -1;
	clock_t start = clock();
	while (true)
	{
		int s1, s2;
		GetInventorySpace(s1, s2);
		cout << "\r" << s1 << "/" << s2 << "\t";
		Sleep(2 * ACTION_DELAY);	//might fail
		if (!consuming && s1 == 0)
			break;

		Check2XP();
		if (CheckAntiBot())
			s1_last--;
		if (s1_last == s1)
		{
			if (consuming && clock() - start > 5000)
				break;
			if (clock() - start > 30000)
			{
				MessageBoxA(0, "Get nothing for 30 secs", "Warning", MB_OK|MB_TOPMOST);
				start = clock();
			}
		}
		else
		{
			start = clock();
			s1_last = s1;
		}
	}
	cout << "...Complete " << act << endl;
}
void WaitCutting(){ WaitActing("cutting"); }
void WaitMining(){ WaitActing("mining"); }

void WaitActing(const string& act, int count)
{
	cout << "Count Down...\n";
	const int COUNT = count;
	for (int i = 0; i < COUNT; i++)
	{
		Sleep(ACTION_DELAY);
		cout << "\r" << COUNT - i - 1 << "\t";
		Check2XP();
		CheckAntiBot();
	}
	cout << "...Complete " << act << "(Left some stuff intentionally so that no need to use mouse to equip)\n";
}
void WaitCooking()
{
	WaitActing("cooking", 36);
}
void WaitSmelting()
{
	WaitActing("smelting", true);
}

void WaitInventoryFull(){}
void CutFirAtDorpatOutpost()
{
	// Dorpat
	// stand at Dorpat (83, 37), Fir Tree at (88, 32), Chest at (83, 38), Corner (83, 32)
	// preparation: 
	// 1. equip the woodcutter's axe 
	cout << "Try to Cut Fir automatically...\n";
	cout << "Please equip the woodcutter's axe beforehand\n";

	int x, y;
	GetPosition(x, y);
	cout << "Position at (" << x << ", " << y << ")\n";
	if (x != 83 || y != 37)
	{
		MessageBoxA(0, "Suggest initial location at Dorpat (83, 37)", "Warning", MB_OK|MB_TOPMOST);
	}
	int s1, s2;
	GetInventorySpace(s1, s2);
	cout << "Inventory Space is " << s1 << " / " << s2 << "\n";

	while (true)
	{
		// try to access to chest and get raw food out
		cout << "WalkTo Chest...";
		WalkTo(83, 37);
		cout << "Done\n";

		cout << "Open Chest...";
		WalkUp();	// open chest
		cout << "Done\n";

		cout << "PutToChest...";
		PutToChest();
		cout << "Done\n";

		cout << "UnloadFromPet...";
		UnloadFromPet();
		cout << "Done\n";

		cout << "PutToChest again...";
		PutToChest();
		cout << "Done\n";

		cout << "WalkTo Corner...";
		WalkTo(83, 32);
		cout << "Done\n";

		cout << "WalkTo Fir Tree...";
		WalkTo(87, 32);
		cout << "Done\n";

		cout << "Cutting...";
		WalkRight();	// access Fir Tree
		WaitCutting();
		cout << "Done\n";

		cout << "LoadToPet...";
		LoadToPet();
		cout << "Done\n";

		cout << "Cutting again...";
		WalkRight();	// access Fir Tree
		WaitCutting();
		cout << "Done\n";
	}
}

void CookAtDorpat()
{
	// stand at Dorpat (21, 17), campfir at (17, 18), Chest at (22, 18)
	// preparation: 
	// 1. chest item select one raw food (with lots of instances)
	// 2. equip the raw food 
	cout << "Try to Cook automatically...\n";
	cout << "Please select the raw food material in the chest and in the inventory beforehand\n";

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

	while (true)
	{
		// try to access to chest and get raw food out
		cout << "WalkTo Chest...";
		WalkTo(21, 17);
		cout << "Done\n";

		cout << "Open Chest...";
		WalkRight();	// open chest
		cout << "Done\n";
		
		cout << "PutToChest...";
		PutToChest();
		cout << "Done\n";

		cout << "GetFormChest...";
		GetFromChest();
		cout << "Done\n";

		//cout << "SelectInventory(The Raw Food Materials)...";
		//SelectInventory(0, 2);
		//cout << "Done\n";

		cout << "WalkTo Campfire...";
		WalkTo(18, 18);
		cout << "Done\n";

		cout << "Cooking...";
		WalkLeft();	// access campfire
		WaitCooking();
		cout << "Done\n";
	}
}

void MineGoldAtReval()
{
	const int chest_side_x = 15;
	const int chest_side_y = 32;
	const int resource_side_x = 46;
	const int resource_side_y = 56;
	
	// preparation: 
	// 1. equip jewelry permission guide and iron pickaxe
	MessageBoxA(0, "Try to Mine Gold automatically...\n"
		"Please equip jewelry permission guide and iron pickaxe beforehand\n",
		"Ready to start", MB_OK|MB_TOPMOST);

	int x, y;
	GetPosition(x, y);
	cout << "Position at (" << x << ", " << y << ")\n";
	if ((x != chest_side_x || y != chest_side_y) && (x != resource_side_x || y != resource_side_y))
	{
		MessageBoxA(0, ("Suggest initial location at Dorpat (" 
			+ to_string(chest_side_x) + ", " + to_string(chest_side_y) + ") or "
			+ to_string(chest_side_x) + ", " + to_string(chest_side_y) + ")").c_str(), "Warning", MB_OK|MB_TOPMOST);
	}
	int s1, s2;
	GetInventorySpace(s1, s2);
	cout << "Inventory Space is " << s1 << " / " << s2 << "\n";

	vector<pair<int,int> > path;
	path.push_back(make_pair(15,19));
	path.push_back(make_pair(16,19));
	path.push_back(make_pair(16,15));
	path.push_back(make_pair(29,15));
	path.push_back(make_pair(29,49));
	path.push_back(make_pair(49,49));
	path.push_back(make_pair(49,56));

	vector<pair<int,int> > backpath;
	backpath.push_back(make_pair(49,49));
	backpath.push_back(make_pair(29,49));
	backpath.push_back(make_pair(29,15));
	backpath.push_back(make_pair(16,15));
	backpath.push_back(make_pair(16,19));
	backpath.push_back(make_pair(15,19));
	backpath.push_back(make_pair(15,32));

	int count = 0;
	if (x == resource_side_x && y == resource_side_y)
		goto START_ACCESS_RESOURCE;
	while (true)
	{
		// try to access to the chest
		cout << "WalkTo Chest...";
		WalkTo(chest_side_x, chest_side_y);
		cout << "Done\n";

		cout << "Open Chest...";
		WalkLeft();	// open chest
		cout << "Done\n";

		cout << "PutToChest...";
		PutToChest();
		cout << "Done\n";

		cout << "UnloadFromPet...";
		UnloadFromPet();
		cout << "Done\n";

		cout << "PutToChest again...";
		PutToChest();
		cout << "Done\n";

		cout << "WalkTo Jewelry Gate...";

		WalkTo(path);
		cout << "Done\n";

		cout << "Entering the Gate...";
		WalkLeft();
		Wait(LocationAt, (void*)(46 * 100 + 56), 5000);
		cout << "Done\n";

START_ACCESS_RESOURCE:
		cout << "Mining...";
		WalkLeft();	// access Gold
		WaitMining();
		cout << "Done\n";

		cout << "LoadToPet...";
		LoadToPet();
		cout << "Done\n";

		cout << "Mining again...";
		WalkLeft();	// access Gold
		WaitMining();
		cout << "Done\n";

		cout << "Leaving the Gate...";
		WalkRight();
		Wait(LocationAt, (void*)(49 * 100 + 56), 8000);
		cout << "Done\n";

		cout << "WalkTo Chest...";
		WalkTo(backpath);
		cout << "Done\n";

		cout << "Complete " << ++count << " times\n";
	}
}

void MineIronAtDorpat()
{
	// todo
	assert(false);
	// stand at Dorpat (21, 17), ladder at (56, 14), Chest at (22, 17)
	// preparation: 
	// 1. equip the mining permission and iron pickaxe
	cout << "Try to Mine Iron automatically...\n";
	cout << "Please equip the mining permission guide and the iron pickaxe beforehand\n";
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

	while (true)
	{
		vector<pair<int,int> > locations;
	}	
}
void SmeltGoldAtNarwa()
{
	const int chest_side_x = 65;
	const int chest_side_y = 40;
	const int resource_side_x = 71;
	const int resource_side_y = 40;
	// preparation: 
	// 1. select the gold chunk in the chest and equip the gold chunk
	// 2. equip boots only so that the number of gold chunk is odd and do not need to equip manually
	string message = "Try to Smelting gold automatically...\n"
		"Please select the gold chunk in the chest and equip the gold chunk beforehand\n"
		"Moreover, please keep ODD space for gold chunk to prevent from equipping manually\n";
		
	int x, y;
	GetPosition(x, y);
	cout << "Position at (" << x << ", " << y << ")\n";
	if ((x != chest_side_x || y != chest_side_y) && (x != resource_side_x || y != resource_side_y))
	{
		message += "Suggest initial location at Reval (" 
			+ to_string(chest_side_x) + ", " + to_string(chest_side_y) + ") or "
			+ to_string(chest_side_x) + ", " + to_string(chest_side_y) + ")";
	}
	MessageBoxA(0, message.c_str(), "Warning", MB_OK|MB_TOPMOST);
	int s1, s2;
	GetInventorySpace(s1, s2);
	cout << "Inventory Space is " << s1 << " / " << s2 << "\n";
	while (true)
	{
		cout << "WalkTo Chest...";
		WalkTo(chest_side_x, chest_side_y);
		cout << "Done\n";

		cout << "Open Chest...";
		WalkLeft();	// open chest
		cout << "Done\n";

		cout << "PutToChest...";
		PutToChest();
		cout << "Done\n";

		cout << "GetFromChest...";
		GetFromChest();
		cout << "Done\n";

		cout << "WalkTo Furnace...";
		WalkTo(resource_side_x, resource_side_y);
		cout << "Done\n";

		cout << "Smelting...";
		WalkUp();	// access Furnace
		WaitSmelting();
		cout << "Done\n";
	}
}
void SmeltGoldAtDorpat()
{
	const int chest_side_x = 22;
	const int chest_side_y = 18;
	const int resource_side_x = 22;
	const int resource_side_y = 22;
	// preparation: 
	// 1. select the gold chunk in the chest and equip the gold chunk
	// 2. equip boots only so that the number of gold chunk is odd and do not need to equip manually
	string message = "Try to Smelting gold automatically...\n"
		"Please select the gold chunk in the chest and equip the gold chunk beforehand\n"
		"Moreover, please keep ODD space for gold chunk to prevent from equipping manually\n";
		
	int x, y;
	GetPosition(x, y);
	cout << "Position at (" << x << ", " << y << ")\n";
	if ((x != chest_side_x || y != chest_side_y) && (x != resource_side_x || y != resource_side_y))
	{
		message += "Suggest initial location at Reval (" 
			+ to_string(chest_side_x) + ", " + to_string(chest_side_y) + ") or "
			+ to_string(chest_side_x) + ", " + to_string(chest_side_y) + ")";
	}
	MessageBoxA(0, message.c_str(), "Warning", MB_OK|MB_TOPMOST);
	int s1, s2;
	GetInventorySpace(s1, s2);
	cout << "Inventory Space is " << s1 << " / " << s2 << "\n";
	while (true)
	{
		cout << "WalkTo Chest...";
		WalkTo(chest_side_x, chest_side_y);
		cout << "Done\n";

		cout << "Open Chest...";
		WalkDown();	// open chest
		cout << "Done\n";

		cout << "PutToChest...";
		PutToChest();
		cout << "Done\n";

		cout << "GetFromChest...";
		GetFromChest();
		cout << "Done\n";

		cout << "WalkTo Furnace...";
		WalkTo(resource_side_x, resource_side_y);
		cout << "Done\n";

		cout << "Smelting...";
		WalkUp();	// access Furnace
		WaitSmelting();
		cout << "Done\n";
	}
}
void SmeltGoldAtReval()
{
	const int chest_side_x = 14;
	const int chest_side_y = 33;
	const int resource_side_x = 17;
	const int resource_side_y = 33;
	// preparation: 
	// 1. select the gold chunk in the chest and equip the gold chunk
	// 2. equip boots only so that the number of gold chunk is odd and do not need to equip manually
	string message = "Try to Smelting gold automatically...\n"
		"Please select the gold chunk in the chest and equip the gold chunk beforehand\n"
		"Moreover, please keep ODD space for gold chunk to prevent from equipping manually\n";
		
	int x, y;
	GetPosition(x, y);
	cout << "Position at (" << x << ", " << y << ")\n";
	if ((x != chest_side_x || y != chest_side_y) && (x != resource_side_x || y != resource_side_y))
	{
		message += "Suggest initial location at Reval (" 
			+ to_string(chest_side_x) + ", " + to_string(chest_side_y) + ") or "
			+ to_string(chest_side_x) + ", " + to_string(chest_side_y) + ")";
	}
	MessageBoxA(0, message.c_str(), "Warning", MB_OK|MB_TOPMOST);
	int s1, s2;
	GetInventorySpace(s1, s2);
	cout << "Inventory Space is " << s1 << " / " << s2 << "\n";
	while (true)
	{
		cout << "WalkTo Chest...";
		WalkTo(chest_side_x, chest_side_y);
		cout << "Done\n";

		cout << "Open Chest...";
		WalkDown();	// open chest
		cout << "Done\n";

		cout << "PutToChest...";
		PutToChest();
		cout << "Done\n";

		cout << "GetFromChest...";
		GetFromChest();
		cout << "Done\n";

		cout << "WalkTo Furnace...";
		WalkTo(resource_side_x, resource_side_y);
		cout << "Done\n";

		cout << "Smelting...";
		WalkUp();	// access Furnace
		WaitSmelting();
		cout << "Done\n";
	}
}

void SellGoldAtReval()
{
	const int chest_side_x = 15;
	const int chest_side_y = 32;
	const int resource_side_x = 17;
	const int resource_side_y = 24;
	// preparation: 
	// 1. select the gold bar in the chest
	// 2. select the gold bar in th merchant
	string message = "Try to Sell gold bar automatically...\n"
		"Please select the gold bar in the chest and select the gold bar in the merchant beforehand\n";
		
	int x, y;
	GetPosition(x, y);
	cout << "Position at (" << x << ", " << y << ")\n";
	if ((x != chest_side_x || y != chest_side_y) && (x != resource_side_x || y != resource_side_y))
	{
		message += "Suggest initial location at Reval (" 
			+ to_string(chest_side_x) + ", " + to_string(chest_side_y) + ") or "
			+ to_string(chest_side_x) + ", " + to_string(chest_side_y) + ")";
	}
	MessageBoxA(0, message.c_str(), "Warning", MB_OK|MB_TOPMOST);
	int s1, s2; 
	GetInventorySpace(s1, s2);
	cout << "Inventory Space is " << s1 << " / " << s2 << "\n";
	if (s1 == 0 && s2 == 0)
		goto SELLALL;
	while (true)
	{
		cout << "WalkTo Chest...";
		WalkTo(chest_side_x, chest_side_y);
		cout << "Done\n";

		cout << "Open Chest...";
		WalkLeft();	// open chest
		cout << "Done\n";

		cout << "GetFromChest...";
		GetFromChest();
		cout << "Done\n";

		/*
		cout << "LoadToPet...";
		LoadToPet();
		cout << "Done\n";

		cout << "GetFromChest again...";
		GetFromChest();
		cout << "Done\n";
		*/

		cout << "WalkTo Jeweler...";
		WalkTo(15, 24);
		WalkTo(resource_side_x, resource_side_y);
		cout << "Done\n";

SELLALL:
		cout << "Selling...";
		WalkRight();	// access shop
		Sleep(2000);
		SellAll();
		Sleep(2000);
		cout << "Done\n";

		/*
		cout << "UnloadFromPet...";
		UnloadFromPet();
		cout << "Done\n";

		cout << "Selling again...";
		SellAll();
		cout << "Done\n";
		*/
	}
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

//	CookAtDorpat();	// must equip the raw food and select the raw food in the chest
//	CutFirAtDorpatOutpost();	// must equip axe
//	MineIronAtDorpat();	// must equip iron pickaxe and mining permission guide
//	MineGoldAtReval();	// must equip iron pickaxe and jewelry permission guide
	g_2XPCheck = false; g_No2XPCheck = true;
	SmeltGoldAtReval();	// must select gold chunk in the chest and equip gold chunk 
	SmeltGoldAtNarwa();	// must select gold chunk in the chest and equip gold chunk 
	SmeltGoldAtDorpat();	// must select gold chunk in the chest and equip gold chunk 
//	SellGoldAtReval();	// must select gold bar in the chest; gold bar in the merchant
	system("pause");
	return 0;
}
