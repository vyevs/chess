#include <Windows.h>

#include <stdio.h>
#include <stdbool.h>

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "gdi32.lib")

void show_message_box_and_exit(const char *msg) {
	char buf[128];
	DWORD last_error_code = GetLastError();
	sprintf(buf, "%s error, error code %d\n", msg, last_error_code);
	MessageBoxA(NULL, buf, "Fatal Error", 0);

	// @CleanUp
	// @CleanUp// @CleanUp// @CleanUp// @CleanUp// @CleanUp// @CleanUp// @CleanUp// @CleanUp// @CleanUp// @CleanUp// @CleanUp// @CleanUp// @CleanUp// @CleanUp// @CleanUp
// @CleanUp// @CleanUp// @CleanUp// @CleanUp// @CleanUp// @CleanUp// @CleanUp// @CleanUp// @CleanUp// @CleanUp// @CleanUp
// @CleanUp// @CleanUp// @CleanUp// @CleanUp// @CleanUp// @CleanUp// @CleanUp// @CleanUp// @CleanUp
// @CleanUp// @CleanUp// @CleanUp// @CleanUp// @CleanUp// @CleanUp// @CleanUp// @CleanUp// @CleanUp// @CleanUp// @CleanUp
	exit(1);
}



static int client_height;
static int client_width;

HBITMAP chessboard;

LRESULT WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
		case WM_CREATE: {
			chessboard = LoadImageA(NULL, "C:\\Users\\vlad\\c\\chess\\assets\\chessboard.bmp", IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION | LR_DEFAULTSIZE | LR_LOADFROMFILE);
			if (chessboard == NULL) {
				show_message_box_and_exit("LoadImageA");
			}
			return 0;
		};
		break;
		
		case WM_SIZE: {
			// the new client size is in the lParam for WM_SIZE messages
			client_width = LOWORD(lParam);
			client_height = HIWORD(lParam);
		};
		break;

		case WM_PAINT: {
			PAINTSTRUCT ps;
			HDC dc = BeginPaint(hwnd, &ps);
	
			RECT rect;
			

	/*
			if (DrawText(dc, buf, -1, &rect, DT_CENTER | DT_SINGLELINE | DT_VCENTER) == 0) {
				show_message_box_and_exit("DrawText");
			}*/

			EndPaint(hwnd, &ps);
			return 0;
		};
		break;
		
		/*
		case WM_ERASEBKGND: {
			return 0;
		};
		break;
		*/
		/*
		By default, WM_CLOSE passed to DefWindowProc causes window to be destroyed. When the window is being destroyed WM_DESTROY message is sent
		case WM_CLOSE: {
			running = false;
		};
		break;
		

		
		/*
		case WM_QUIT: {
			return 0;
		};
		break;
		*/
	

		case WM_DESTROY: {
			// PostQuitMessage causes a WM_QUIT to be produced, which we should use to exit
			PostQuitMessage(0);
			return 0;
		};
		break;
	}
	
	return DefWindowProcA(hwnd, uMsg, wParam, lParam);
}



int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
	
	WNDCLASSEXA window_class_params = {0};
	window_class_params.cbSize = sizeof(WNDCLASSEXA);
	window_class_params.style = CS_OWNDC | CS_CLASSDC | CS_HREDRAW | CS_VREDRAW;
	window_class_params.lpfnWndProc = WindowProc;
	window_class_params.hInstance = hInstance;
	window_class_params.lpszClassName = "chess_window_class";
	window_class_params.hbrBackground = GetStockObject(BLACK_BRUSH); // calls to BeginPaint will color the client area white before returning

	ATOM window_class = RegisterClassExA(&window_class_params);
	if (window_class == 0) {
		show_message_box_and_exit("RegisterClassExA");
	}
	
	HWND the_window = CreateWindowExA(
					0,                    // dwExStyle
					"chess_window_class", // lpClassName
					"chess",      		  // lpWindowName 
					WS_OVERLAPPEDWINDOW | WS_SYSMENU | WS_VISIBLE,           // dwStyle
					CW_USEDEFAULT,   	  // x
					CW_USEDEFAULT,        // y
                    1024,         		  // nWidth
                    768,          		  // nHeight
                    NULL,         		  // hWndParent   
				    NULL,         		  // hMenu
					hInstance,    		  // hInstance
					NULL          		  // lpParam
				);

	if (the_window == NULL) {
		show_message_box_and_exit("CreateWindowExA");
	}
	
	MSG the_msg;
	BOOL get_message_result;
	
	// loop until GetMessageA returns 0, which indicates that we got a WM_QUIT window message
	while ((get_message_result = GetMessageA(&the_msg, the_window, 0, 0)) != 0) {
		
		if (get_message_result == -1) {
			show_message_box_and_exit("GetMessage");
		}

		TranslateMessage(&the_msg);
		DispatchMessage(&the_msg);
	}

	return the_msg.wParam;
}