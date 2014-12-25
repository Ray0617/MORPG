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
using namespace std;

const int SAMPLE_INVENTORY_X = 1443;
const int SAMPLE_INVENTORY_Y = 78;
const int SAMPLE_INVENTORY_W = 8;
const int SAMPLE_INVENTORY_H = 11;

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

int main()
{
	//find the target HWND
	HWND hWndParent = FindWindowA("Chrome_WidgetWin_0", 0);
	EnumChildWindows(hWndParent, FindMORPGWindowHandle, 0);
	if (g_hWnd == 0)
		return 0;

	COLORREF clr[SAMPLE_INVENTORY_W][SAMPLE_INVENTORY_H] = {0};
	bool test[SAMPLE_INVENTORY_W][SAMPLE_INVENTORY_H] = {0};
	HDC dc = GetDC(g_hWnd);
	for (int x = SAMPLE_INVENTORY_X; x < SAMPLE_INVENTORY_X + SAMPLE_INVENTORY_W; x++)
	{
		for (int y = SAMPLE_INVENTORY_Y; y < SAMPLE_INVENTORY_Y + SAMPLE_INVENTORY_H; y++)
		{
			int i = x - SAMPLE_INVENTORY_X;
			int j = y - SAMPLE_INVENTORY_Y;
			clr[i][j] = GetPixel(dc, x, y);
			int r = GetRValue(clr[i][j]);
			int g = GetGValue(clr[i][j]);
			int b = GetBValue(clr[i][j]);
			if (r > 128 && g > 128 && b < 128)
				test[i][j] = true;
		}
	}
	ReleaseDC(g_hWnd, dc);
	ofstream output("digit\\0");
	for (int j = 0; j < SAMPLE_INVENTORY_H; j++)
	{
		for (int i = 0; i < SAMPLE_INVENTORY_W; i++)
		{
			cout << test[i][j];
			output << test[i][j];
		}
		cout << endl;
		output << endl;
	}
	output.close();
	system("pause");
	return 0;
}
