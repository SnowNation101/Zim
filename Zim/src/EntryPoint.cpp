#include "framework.h"
#include "EntryPoint.h"

#define max(a, b) ((a) > (b) ? (a) : (b))
#define min(a, b) ((a) < (b) ? (a) : (b))

#define TAB_SIZE 4
#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst; // current instance
TCHAR szTitle[MAX_LOADSTRING]; // The title bar text
TCHAR szWindowClass[MAX_LOADSTRING]; // the main window class name

HWND hDlg;

FILE* stream;
String file_buffer;
TCHAR file_data[10000000];
INT file_lines = 1;
TCHAR numbers_buffer[10000000] = {'1', '\n', '\0',};

HANDLE g_hOutput;

// Declarations of functions
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK DlgAbout(HWND, UINT, WPARAM, LPARAM);

VOID add_menus(HWND);
VOID open_file(HWND);
VOID save_file(HWND);
VOID save_file_as(HWND);
VOID make_number_string();
VOID draw_caret(HWND, INT, INT, INT);

VOID OnPaint(HWND, SCROLLINFO&, INT&, INT&, INT, INT, INT, INT, INT, INT);
VOID OnCommand(HWND, WPARAM, int&, int&, int&, int&);
INT CountLines(String);
size_t get_pos_in_string(INT line, INT column);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, INT nCmdShow)
{
	//AllocConsole();
	g_hOutput = GetStdHandle(STD_OUTPUT_HANDLE);
	// Initialize global strings
	LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadStringW(hInstance, IDC_ZIM, szWindowClass, MAX_LOADSTRING);

	// Register window class
	WNDCLASS wc{};
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = CreateSolidBrush(RGB(48, 56, 65));
	wc.lpszMenuName = (TCHAR*)IDR_MENU1;
	wc.lpszClassName = szWindowClass;

	if (!RegisterClass(&wc))
	{
		MessageBox(NULL, TEXT("This program requires Windows NT!"), szWindowClass, MB_ICONERROR);
		return FALSE;
	}

	hInst = hInstance; // Store instance handle in our global variable

	HWND hwnd = CreateWindowW(
		szWindowClass, // window class name
		szTitle, // window caption
		WS_OVERLAPPEDWINDOW, // window style
		CW_USEDEFAULT, // initial x position
		CW_USEDEFAULT, // initial y position
		CW_USEDEFAULT, // initial x size
		CW_USEDEFAULT, // initial y size
		NULL, // parent window handle
		NULL, // window menu handle
		hInstance, // program instance handle
		NULL); // creation parameters

	if (!hwnd)
	{
		return FALSE;
	}

	ShowWindow(hwnd, nCmdShow);
	UpdateWindow(hwnd);

	MSG msg{};
	while (GetMessage(&msg, nullptr, 0, 0))
	{		
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static DWORD dwCharSet = DEFAULT_CHARSET;
	static int cxCaps; // width of a capital letter
	static int cxChar, cyChar; // width ant height of a character
	static int cxClient, cyClient; // width and height of client window
	static int line = 1, column = 1; // Line and column of current position
	static int cxBuffer, cyBuffer;
	static int xCaret = 0, yCaret = 0;
	int iVertPos, iHorzPos;
	static TCHAR* pBuffer = NULL;
	HDC hdc;
	int x, y;
	PAINTSTRUCT ps;
	SCROLLINFO si;
	TEXTMETRIC tm;

	switch (uMsg)
	{
	case WM_INPUTLANGCHANGE:
		dwCharSet = wParam;
	case WM_COMMAND:
		OnCommand(hWnd, wParam, xCaret, yCaret, line, column);
		break;
	case WM_CREATE:
		hdc = GetDC(hWnd);
		SelectObject(hdc, CreateFont(27, 0, 0, 0, 900, 0, 0, 0, DEFAULT_CHARSET, 0, 0, 0, 0, L"Consolas"));
		GetTextMetrics(hdc, &tm);
		cxChar = tm.tmAveCharWidth;
		cxCaps = (tm.tmPitchAndFamily & 1 ? 3 : 2) * cxChar / 2;
		cyChar = tm.tmHeight + tm.tmExternalLeading;
		DeleteObject(SelectObject(hdc, GetStockObject(SYSTEM_FONT)));
		ReleaseDC(hWnd, hdc);
		draw_caret(hWnd, 85 + xCaret * cxChar, yCaret * cyChar, cyChar);
		break;
	case WM_SIZE:
		{
			// obtain window size in pixels
			if (uMsg == WM_SIZE)
			{
				cxClient = LOWORD(lParam);
				cyClient = HIWORD(lParam);
			}

			// Set vertical scroll bar range and page size
			si.cbSize = sizeof(si);
			si.fMask = SIF_RANGE | SIF_PAGE;
			si.nMin = 0;
			si.nMax = 200000;
			si.nPage = cyClient;
			SetScrollInfo(hWnd, SB_VERT, &si, TRUE);

			// Set horizontal scroll bar range and page size
			si.cbSize = sizeof(si);
			si.fMask = SIF_RANGE | SIF_PAGE;
			si.nMin = 0;
			si.nMax = 250; // TODO: make ...
			si.nPage = cxClient / cxChar;
			SetScrollInfo(hWnd, SB_HORZ, &si, TRUE);

			if (hWnd == GetFocus()) draw_caret(hWnd, 85 + xCaret * cxChar, yCaret * cyChar, cyChar);
			InvalidateRect(hWnd, nullptr, TRUE);
			return 0;
		}

	case WM_VSCROLL:
		{
			// Get all the vertical scroll bar information
			si.cbSize = sizeof(si);
			si.fMask = SIF_ALL;
			GetScrollInfo(hWnd, SB_VERT, &si);
			// Save the position for comparison later on
			iVertPos = si.nPos;
			switch (LOWORD(wParam))
			{
			case SB_TOP:
				si.nPos = si.nMin;
				break;
			case SB_BOTTOM:
				si.nPos = si.nMax;
				break;
			case SB_LINEUP:
				si.nPos -= 20;
				break;
			case SB_LINEDOWN:
				si.nPos += 20;
				break;
			case SB_PAGEUP:
				si.nPos -= si.nPage;
				break;
			case SB_PAGEDOWN:
				si.nPos += si.nPage;
				break;
			case SB_THUMBTRACK:
				si.nPos = si.nTrackPos;
				break;
			default:
				break;
			}
			// Set the position and then retrieve it. Due to adjustments
			// by Windows it may not be the same as the value set.
			si.fMask = SIF_POS;
			SetScrollInfo(hWnd, SB_VERT, &si, TRUE);
			GetScrollInfo(hWnd, SB_VERT, &si);
			// If the position has changed, scroll the window and update it
			if (si.nPos != iVertPos)
			{
				RECT scroll_rect;
				GetClientRect(hWnd, &scroll_rect);
				scroll_rect.bottom -= 20;

				ScrollWindowEx(hWnd, 0, iVertPos - si.nPos, &scroll_rect, &scroll_rect, nullptr, nullptr,
							   SW_INVALIDATE | SW_ERASE);
				UpdateWindow(hWnd);
			}
			return 0;
		}

	case WM_HSCROLL:
		{
			// Get all the vertical scroll bar information
			si.cbSize = sizeof(si);
			si.fMask = SIF_ALL;
			// Save the position for comparison later on
			GetScrollInfo(hWnd, SB_HORZ, &si);
			iHorzPos = si.nPos;
			switch (LOWORD(wParam))
			{
			case SB_LINELEFT:
				si.nPos -= 1;
				break;
			case SB_LINERIGHT:
				si.nPos += 1;
				break;
			case SB_PAGELEFT:
				si.nPos -= si.nPage;
				break;
			case SB_PAGERIGHT:
				si.nPos += si.nPage;
				break;
			case SB_THUMBPOSITION:
				si.nPos = si.nTrackPos;
				break;
			default:
				break;
			}
			// Set the position and then retrieve it. Due to adjustments
			// by Windows it may not be the same as the value set.
			si.fMask = SIF_POS;
			SetScrollInfo(hWnd, SB_HORZ, &si, TRUE);
			GetScrollInfo(hWnd, SB_HORZ, &si);

			// If the position has changed, scroll the window
			if (si.nPos != iHorzPos)
			{
				RECT scroll_rect;
				GetClientRect(hWnd, &scroll_rect);

				ScrollWindowEx(hWnd, iHorzPos - si.nPos, 0, nullptr, nullptr, nullptr, nullptr,
							   SW_INVALIDATE | SW_ERASE);
				UpdateWindow(hWnd);
			}
			return 0;
		}

	case WM_MOUSEWHEEL:
		{
			short zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
			WORD wScrollNotify = 0xFFFF;

			if (zDelta < 0) wScrollNotify = SB_LINEDOWN;
			else if (zDelta > 0) wScrollNotify = SB_LINEUP;

			SendMessage(hWnd, WM_VSCROLL, MAKELONG(wScrollNotify, 0), 0L);
			break;
		}

	case WM_SETFOCUS:
		{
			RECT rect;
			GetClientRect(hWnd, &rect);
			InvalidateRect(hWnd, &rect, TRUE);
			UpdateWindow(hWnd);
			return 0;
		}

	case WM_KEYDOWN:
		{
			switch (wParam)
			{
			case VK_LEFT:
				{
					xCaret = max(xCaret - 1, 0);
					column = max(column - 1, 1);
					break;
				}
			case VK_RIGHT:
				{
					size_t pos = get_pos_in_string(line, column);
					if (file_buffer[pos] != '\n' && file_buffer[pos] != '\0')
					{
						column++;
						xCaret++;
					}
					break;
				}
			case VK_UP:
				{
					yCaret = max(yCaret - 1, 0);
					line = yCaret + 1;
					xCaret = 0;
					size_t pos = get_pos_in_string(line, 1);
					int tmp = 1;
					while (file_buffer[pos] != '\n' && tmp != column)
					{
						pos++;
						xCaret++;
						tmp++;
					}
					column = tmp;
					break;
				}
			case VK_DOWN:
				{
					yCaret = min(yCaret + 1, file_lines - 1);
					line = yCaret + 1;
					xCaret = 0;
					size_t pos = get_pos_in_string(line, 1);
					int tmp = 1;
					while (file_buffer[pos] != '\n' && file_buffer[pos] != '\0' && tmp != column)
					{
						pos++;
						xCaret++;
						tmp++;
					}
					column = tmp;
					break;
				}
			default:
				break;
			}
			RECT rect;
			GetClientRect(hWnd, &rect);
			InvalidateRect(hWnd, &rect, TRUE);
			UpdateWindow(hWnd);
			return 0;
		}

	case WM_CHAR:
		{
			switch (wParam)
			{
			case '\b':
				{
					// backspace
					size_t pos = get_pos_in_string(line, column);
					if (pos != 0)
					{
						if (file_buffer[pos - 1] != '\n')
						{
							file_buffer.erase(pos - 1, 1);
							xCaret = max(xCaret - 1, 0);
							column = max(column - 1, 0);
						}
						else
						{
							yCaret--;
							line--;
							xCaret = 0;
							column = 1;
							pos = get_pos_in_string(line, column);
							if (file_buffer[pos] != '\n')
							{
								for (size_t i = pos; file_buffer[i + 1] != '\n'; i++)
								{
									xCaret++;
									column++;
								}
								pos = get_pos_in_string(line, column);
								file_buffer.erase(pos + 1, 1);
								xCaret++;
								column++;
							}
							else
							{
								file_buffer.erase(pos, 1);
							}
							file_lines--;
						}
					}
					break;
				}

			case '\t':
				// Tab
				for (int i = 0; i < TAB_SIZE; i++)
				{
					SendMessage(hWnd, WM_CHAR, ' ', 1);
				}
				break;
			case '\n':
				{
					// Line feed
					break;
				}
			case '\r':
				{
					// Carriage return
					SIZE_T pos = get_pos_in_string(line, column);
					file_buffer.insert(pos, '\n');
					xCaret = 0;
					column = 1;
					yCaret++;
					line++;
					file_lines++;
					break;
				}
			default:
				// character codes
				SIZE_T pos = get_pos_in_string(line, column);
				file_buffer.insert(pos, (TCHAR)wParam);
				xCaret++;
				column++;
				break;
			}
			RECT rect;
			GetClientRect(hWnd, &rect);
			InvalidateRect(hWnd, &rect, TRUE);
			UpdateWindow(hWnd);
			return 0;
		}

	case WM_CLOSE:
		DestroyWindow(hWnd);
		break;

	case WM_DESTROY:
		if (stream != nullptr) fclose(stream);
		PostQuitMessage(0);
		break;

	case WM_PAINT:
		OnPaint(hWnd, si, iHorzPos, iVertPos, xCaret, yCaret, cxChar, cyChar, line, column);
		break;

	case WM_LBUTTONDOWN:
		break;

	default:
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
	return 0;
}

VOID find_text(HWND hWnd)
{
	FINDREPLACE fr;       // common dialog box structure
	TCHAR szFindWhat[80];  // buffer receiving string
	szFindWhat[0] = '\0';

	// Initialize FINDREPLACE
	ZeroMemory(&fr, sizeof(fr));
	fr.lStructSize = sizeof(fr);
	fr.hwndOwner = hWnd;
	fr.lpstrFindWhat = szFindWhat;
	fr.wFindWhatLen = 80;
	fr.Flags = 0;

	hDlg = FindText(&fr);
}

VOID open_file(HWND hWnd)
{
	OPENFILENAME ofn;

	ZeroMemory(&ofn, sizeof(OPENFILENAME));

	// Make file_name a string
	TCHAR file_name[128];
	file_name[0] = '\0';

	// Initialize open file dialog
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = hWnd;
	ofn.lpstrFile = file_name;
	ofn.nMaxFile = 128;
	ofn.lpstrFilter = TEXT(
		"All Files (*.*)\0*.*\0C (*.c;*.h)\0*.c;*.h\0C++ (*.cpp;*.cc;*.cp;*.cxx;*.c++;*.C;*.h;*.hh;*.hpp;*.hxx;*.h++;*.inl;*.ipp)\0*.cpp;*.cc;*.cp;*.cxx;*.c++;*.C;*.h;*.hh;*.hpp;*.hxx;*.h++;*.inl;*.ipp\0\0");
	ofn.nFilterIndex = 1;

	GetOpenFileName(&ofn);

	if (stream != nullptr) fclose(stream);

	_tfopen_s(&stream, ofn.lpstrFile, TEXT("a+, ccs=UTF-8"));

	if (stream != nullptr)
	{
		file_buffer = TEXT("");

		TCHAR str[1000];
		while (fgetws(str, 1000, stream) != nullptr)
		{
			file_buffer = file_buffer + str;
		}

		file_lines = CountLines(file_buffer);

		make_number_string();

		RECT rect;
		GetClientRect(hWnd, &rect);
		InvalidateRect(hWnd, &rect, TRUE);
		UpdateWindow(hWnd);
	}
}

VOID save_file()
{
	fputws(file_buffer.c_str(), stream);
}

VOID save_file_as(HWND hWnd)
{
	OPENFILENAME ofn;

	ZeroMemory(&ofn, sizeof(OPENFILENAME));

	// Make file_name a string
	TCHAR file_name[128];
	file_name[0] = '\0';

	// Initialize open file dialog
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = hWnd;
	ofn.lpstrFile = file_name;
	ofn.nMaxFile = 128;
	ofn.lpstrFilter = TEXT(
		"All Files (*.*)\0*.*\0C (*.c;*.h)\0*.c;*.h\0C++ (*.cpp;*.cc;*.cp;*.cxx;*.c++;*.C;*.h;*.hh;*.hpp;*.hxx;*.h++;*.inl;*.ipp)\0*.cpp;*.cc;*.cp;*.cxx;*.c++;*.C;*.h;*.hh;*.hpp;*.hxx;*.h++;*.inl;*.ipp\0\0");
	ofn.nFilterIndex = 1;

	GetSaveFileName(&ofn);

	if (stream != nullptr) fclose(stream);
	_tfopen_s(&stream, ofn.lpstrFile, TEXT("w+, ccs=UTF-8"));
	save_file();
}

VOID make_number_string()
{
	numbers_buffer[0] = '\0';
	for (int i = 1; i <= file_lines; i++)
	{
		TCHAR temp[100];
		temp[0] = '\0';
		_stprintf_s(temp, 99, TEXT("%d\n"), i);
		_tcscat_s(numbers_buffer, _tcslen(numbers_buffer) + _tcslen(temp) + 1, temp);
	}
}

VOID OnPaint(HWND hWnd, SCROLLINFO& si, 
	INT& xPos,  INT& yPos,
	INT xCaret, INT yCaret, 
	INT cWidth, INT cHeight,
	INT line,   INT column)

{
	HDC hdc;
	PAINTSTRUCT ps;

	hdc = BeginPaint(hWnd, &ps);

	si.cbSize = sizeof(si);
	si.fMask = SIF_POS;
	// Get vertical scroll bar position
	GetScrollInfo(hWnd, SB_VERT, &si);
	yPos = si.nPos;
	// Get horizontal scroll bar position
	GetScrollInfo(hWnd, SB_HORZ, &si);
	xPos = si.nPos;


	RECT text_rect;
	GetClientRect(hWnd, &text_rect);
	text_rect.left += 85;

	text_rect.top -= yPos;
	text_rect.left -= xPos;
	text_rect.bottom -= yPos;
	text_rect.right -= xPos;

	RECT number_back_rect;
	GetClientRect(hWnd, &number_back_rect);
	number_back_rect.right = 80;
	number_back_rect.left -= xPos;
	number_back_rect.right -= xPos;
	FillRect(hdc, &number_back_rect, CreateSolidBrush(RGB(76, 88, 99)));

	// Draw the text
	HFONT textFont = CreateFont(27, 0, 0, 0, 900, 0, 0, 0, DEFAULT_CHARSET, 0, 0, 0, 0, L"Consolas");
	HGDIOBJ nOldFont = SelectObject(hdc, textFont);
	SetTextColor(hdc, RGB(216, 222, 233));
	SetBkMode(hdc, TRANSPARENT);
	DrawText(hdc, file_buffer.c_str(), (INT)file_buffer.size(), &text_rect, DT_NOCLIP);

	// Draw the line numbers
	RECT number_rect;
	GetClientRect(hWnd, &number_rect);
	number_rect.right = number_rect.left + 60;
	number_rect.top -= yPos;
	number_rect.left -= xPos;
	number_rect.bottom -= yPos;
	number_rect.right -= xPos;
	make_number_string();
	SetTextColor(hdc, RGB(191, 197, 208));
	DrawText(hdc, numbers_buffer, _tcslen(numbers_buffer), &number_rect, DT_NOCLIP | DT_RIGHT);

	// Draw the caret
	draw_caret(hWnd, 85 + xCaret * cWidth - xPos, yCaret * cHeight - yPos, cHeight);

	RECT status_rect;
	GetClientRect(hWnd, &status_rect);
	status_rect.top = status_rect.bottom - 20;
	FillRect(hdc, &status_rect, CreateSolidBrush(RGB(199, 203, 209)));

	// Draw the status
	TCHAR status[100];
	swprintf_s(status, 99, TEXT("line: %d, Column: %d"), line, column);
	HFONT statusFont = CreateFont(0, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, 0, 0, 0, 0,
								  L"Microsoft YaHei UI Light");
	SelectObject(hdc, statusFont);
	SetTextColor(hdc, RGB(118, 121, 124));
	status_rect.left += 20;
	DrawText(hdc, status, _tcslen(status), &status_rect, DT_VCENTER | DT_SINGLELINE);


	SelectObject(hdc, nOldFont);
	DeleteObject(textFont);
	DeleteObject(statusFont);
	EndPaint(hWnd, &ps);
}

void OnCommand(HWND hWnd, WPARAM wParam, int& xCaret, int& yCaret, int& line, int& column)
{
	switch (wParam)
	{
	case ID_FILE_NEWFILE:
		break;
	case ID_FILE_OPENFILE:
		xCaret = 0;
		yCaret = 0;
		line = 1;
		column = 1;
		open_file(hWnd);
		break;
	case ID_FILE_OPENFOLDER:
		break;
	case ID_FILE_OPENRECENT:
		break;
	case ID_FILE_SAVEAS:
		save_file_as(hWnd);
		break;
	case ID_FILE_EXIT:
		DestroyWindow(hWnd);
		break;
	case ID_FIND_FIND:
		find_text(hWnd);
		break;
	case ID_HELP_DOCUMENTATION:
		break;
	case ID_HELP_REPORTABUG:
		break;
	case ID_HELP_QQ:
		break;
	case ID_HELP_CHECKFORUPDATES:
		break;
	case ID_HELP_CHANGELOG:
		break;
	case ID_HELP_ABOUTZIM:
		DialogBox(hInst, (TCHAR*)IDD_DIALOG1, hWnd, DlgAbout);
		break;
	}
}

INT CountLines(String str)
{
	int count = 1;
	for (int i = 0; i < str.size(); i++)
	{
		if (str[i] == '\n') count++;
	}

	return count;
}

VOID draw_caret(HWND hWnd, INT xPos, INT yPos, INT cHeight)
{
	HDC hdc = GetDC(hWnd);

	HPEN hPen = CreatePen(PS_SOLID, 2, RGB(247, 174, 88));
	HGDIOBJ nOldPen = SelectObject(hdc, hPen);

	MoveToEx(hdc, xPos, yPos - 2, nullptr);
	LineTo(hdc, xPos, yPos + cHeight + 2);
	ReleaseDC(hWnd, hdc);

	SelectObject(hdc, nOldPen);
	DeleteObject(hPen);
}

size_t get_pos_in_string(INT line, INT column)
{
	int pos = 0;
	for (int i = 0; i < line - 1;)
	{
		if (file_buffer[pos] == '\n') i++;
		pos++;
	}

	return pos + column - 1;
}

INT_PTR CALLBACK DlgAbout(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
	{
		HWND hwndOwner;
		RECT rc, rcDlg, rcOwner;

		// Get the owner window and dialog box rectangles. 

		if ((hwndOwner = GetParent(hDlg)) == NULL)
		{
			hwndOwner = GetDesktopWindow();
		}

		GetWindowRect(hwndOwner, &rcOwner);
		GetWindowRect(hDlg, &rcDlg);
		CopyRect(&rc, &rcOwner);

		// Offset the owner and dialog box rectangles so that right and bottom 
		// values represent the width and height, and then offset the owner again 
		// to discard space taken up by the dialog box. 

		OffsetRect(&rcDlg, -rcDlg.left, -rcDlg.top);
		OffsetRect(&rc, -rc.left, -rc.top);
		OffsetRect(&rc, -rcDlg.right, -rcDlg.bottom);

		// The new position is the sum of half the remaining space and the owner's 
		// original position. 

		SetWindowPos(hDlg,
			HWND_TOP,
			rcOwner.left + (rc.right / 2),
			rcOwner.top + (rc.bottom / 2),
			0, 0, // Ignores size arguments. 
			SWP_NOSIZE);

		return TRUE;
	}
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hDlg, &ps);

		WCHAR name[] = L"Zim";
		WCHAR license[] = L"Single User License";
		WCHAR copyright[] = L"Copyright (c) 2022-2022 SnowNation Pty Ltd";
		WCHAR build[] = L"Stable Channel, Build 1314";

		RECT rect;
		GetClientRect(hDlg, &rect);


		HFONT nameFont = CreateFont(40, 0, 0, 0, 900, 0, 0, 0, DEFAULT_CHARSET, 0, 0, 0, 0, L"Goudy Stout");
		HGDIOBJ nOldFont = SelectObject(hdc, nameFont);
		SetTextColor(hdc, RGB(0, 0, 0));
		SetBkMode(hdc, TRANSPARENT);
		RECT rect0 = rect;
		rect0.top += 35;
		DrawText(hdc, name, _tcslen(name), &rect0, DT_CENTER);


		HFONT hFont = CreateFont(15, 0, 0, 0, 500, 0, 0, 0, DEFAULT_CHARSET, 0, 0, 0, 0, L"Cascadia Mono");
		SelectObject(hdc, hFont);
		SetTextColor(hdc, RGB(100, 100, 100));
		SetBkMode(hdc, TRANSPARENT);

		RECT rect1 = rect;
		rect1.top += 75;
		DrawText(hdc, license, _tcslen(license), &rect1, DT_CENTER);
		RECT rect2 = rect;
		rect2.top += 125;
		DrawText(hdc, copyright, _tcslen(copyright), &rect2, DT_CENTER);
		RECT rect3 = rect;
		rect3.top += 145;
		DrawText(hdc, build, _tcslen(build), &rect3, DT_CENTER);
		SelectObject(hdc, nOldFont);
		DeleteObject(hFont);

		EndPaint(hDlg, &ps);
	}
	break;
	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
};