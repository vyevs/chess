#include <stdio.h>

#include <Windows.h>

#pragma comment(lib, "user32.lib")



void show_message_box_and_exit(const char *msg) {
	char buf[128];
	DWORD last_error_code = GetLastError();
	sprintf(buf, "%s error, error code %d\n", msg, last_error_code);
	MessageBoxA(NULL, buf, "Fatal Error", 0);
	exit(1);
}

LRESULT WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
		case WM_PAINT: {
			return 0;
		};
		break;
		
		case WM_ERASEBKGND: {
			return 0;
		};
		break;

		default: {
			return DefWindowProcA(hwnd, uMsg, wParam, lParam);
		}
	}
}

int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
	
	WNDCLASSEXA window_class_params;
	memset(&window_class_params, 0, sizeof(window_class_params));
	window_class_params.cbSize = sizeof(WNDCLASSEXA);
	window_class_params.style = CS_OWNDC | CS_CLASSDC;
	window_class_params.lpfnWndProc = WindowProc;
	window_class_params.hInstance = hInstance;
	window_class_params.lpszMenuName = "menu name";
	window_class_params.lpszClassName = "chess_window_class";

	ATOM window_class = RegisterClassExA(&window_class_params);
	if (window_class == 0) {
		show_message_box_and_exit("RegisterClassExA");
	}
	
	HWND the_window = CreateWindowExA(
					0,                    // dwExStyle
					"chess_window_class", // lpClassName
					"chess",      		  // lpWindowName 
					WS_VISIBLE,           // dwStyle
					0,            		  // x
					0,            		  // y
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
	
	while (1) {
		MSG the_msg;
		BOOL get_message_result = GetMessageA(&the_msg, the_window, 0, 0);
	
		if (get_message_result == -1) {
			show_message_box_and_exit("GetMessage");
		}

		TranslateMessage(&the_msg);
		DispatchMessage(&the_msg);
	}


	return 0;
}