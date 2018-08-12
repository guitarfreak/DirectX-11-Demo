#pragma once

#include "input.h"

bool mouseInClientArea(HWND windowHandle) {
	POINT point;    
	GetCursorPos(&point);
	ScreenToClient(windowHandle, &point);

	Vec2i mp = vec2i(point.x, point.y);

	RECT cr; 
	GetClientRect(windowHandle, &cr);
	bool result = (mp.x >= cr.left && mp.x < cr.right && 
				   mp.y >= cr.top  && mp.y < cr.bottom);

	return result;
}

void updateCursorIcon(WindowSettings* ws) {
	if(!ws->customCursor) {
		SetCursor(LoadCursor(0, IDC_ARROW));
	}
	ws->customCursor = false;
}

void setCursorIcon(WindowSettings* ws, LPCSTR type) {
	SetCursor(LoadCursor(0, type));
	ws->customCursor = true;
}

void showCursor(bool show) {
	if(show) {
		while(ShowCursor(true) < 0) {};
	} else {
		while(ShowCursor(false) >= 0) {};
	}
}

void captureMouse(HWND windowHandle, bool t) {
	if(t) {
		int w,h;
		Vec2i wPos;
		getWindowProperties(windowHandle, &w, &h, 0, 0, &wPos.x, &wPos.y);
		SetCursorPos(wPos.x + w/2, wPos.y + h/2);

		while(ShowCursor(false) >= 0);
	} else {
		while(ShowCursor(true) < 0);
	}
}

//

Vec2 getMousePos(HWND windowHandle, bool yInverted = true) {
	POINT point;    
	GetCursorPos(&point);
	ScreenToClient(windowHandle, &point);
	Vec2 mousePos = vec2(0,0);
	mousePos.x = point.x;
	mousePos.y = point.y;
	if(yInverted) mousePos.y = -mousePos.y;

	return mousePos;
}

Vec2 getMousePosS(bool yInverted = true) {
	POINT point;    
	GetCursorPos(&point);
	Vec2 mousePos = vec2(0,0);
	mousePos.x = point.x;
	mousePos.y = point.y;
	if(yInverted) mousePos.y = -mousePos.y;

	return mousePos;
}

//

int vkToKeycode(int vk) {

	switch(vk) {
		case VK_CONTROL: return KEYCODE_CTRL;
		case VK_RCONTROL: return KEYCODE_CTRL_RIGHT;
		case VK_SHIFT: return KEYCODE_SHIFT;
		case VK_RSHIFT: return KEYCODE_SHIFT_RIGHT;
		case VK_MENU: return KEYCODE_ALT;
		case VK_CAPITAL: return KEYCODE_CAPS;
		case VK_TAB: return KEYCODE_TAB;
		case VK_SPACE: return KEYCODE_SPACE;
		case VK_RETURN: return KEYCODE_RETURN;
		case VK_ESCAPE: return KEYCODE_ESCAPE;
		case VK_BACK: return KEYCODE_BACKSPACE;
		case VK_DELETE: return KEYCODE_DEL;
		case VK_HOME: return KEYCODE_HOME;
		case VK_END: return KEYCODE_END;
		case VK_PRIOR: return KEYCODE_PAGEUP;
		case VK_NEXT: return KEYCODE_PAGEDOWN;
		case VK_UP: return KEYCODE_UP;
		case VK_DOWN: return KEYCODE_DOWN;
		case VK_LEFT: return KEYCODE_LEFT;
		case VK_RIGHT: return KEYCODE_RIGHT;

		default: {
				 if(vk >= WIN_KEY_NUMERIC_START && vk <= WIN_KEY_NUMERIC_END) return KEYCODE_0 + vk - WIN_KEY_NUMERIC_START;
			else if(vk >= WIN_KEY_LETTERS_START && vk <= WIN_KEY_LETTERS_END) return KEYCODE_A + vk - WIN_KEY_LETTERS_START;
			else if(vk >= WIN_KEY_F_START 		&& vk <= WIN_KEY_F_END) 	  return KEYCODE_F1 + vk - WIN_KEY_F_START;
		}
	}

	return -1;
}

void initInput(Input* input) {
    *input = {};

    input->firstFrame = true;
}

void inputPrepare(Input* input) {input->anyKey = false;
    input->mouseWheel = 0;
    for(int i = 0; i < arrayCount(input->mouseButtonPressed); i++) input->mouseButtonPressed[i] = 0;
    for(int i = 0; i < arrayCount(input->mouseButtonReleased); i++) input->mouseButtonReleased[i] = 0;
    for(int i = 0; i < arrayCount(input->keysPressed); i++) input->keysPressed[i] = 0;
    input->inputCharacterCount = 0;
    input->mouseDelta = vec2(0,0);

    input->doubleClick = false;

    input->closeWindow = false;
	input->maximizeWindow = false;
	input->minimizeWindow = false;

	input->altEnter = false;
}

void CALLBACK updateInput(SystemData* sd) {
	for(;;) {

		Input* input = sd->input;
		HWND windowHandle = sd->windowHandle;

		SetTimer(windowHandle, 1, 1, 0);

	    bool mouseInClient = mouseInClientArea(windowHandle);

	    MSG message;
	    while(PeekMessage(&message, windowHandle, 0, 0, PM_REMOVE)) {
	        switch(message.message) {
		        case WM_LBUTTONDBLCLK: {
		        	input->doubleClick = true;
					input->doubleClickPos = getMousePos(windowHandle, true);
		        } break;

	            case WM_KEYDOWN:
	            case WM_KEYUP: {
	                uint vk = uint(message.wParam);

	                bool keyDown = (message.message == WM_KEYDOWN);
	                int keycode = vkToKeycode(vk);
	                input->keysDown[keycode] = keyDown;
	                input->keysPressed[keycode] = keyDown;
	                // input->mShift = ((GetKeyState(VK_SHIFT) & 0x80) != 0);
	                // input->mCtrl = ((GetKeyState(VK_CONTROL) & 0x80) != 0);
	                bool alt = ((GetKeyState(VK_MENU) & 0x80) != 0);
	                if(keyDown && keycode == KEYCODE_RETURN && alt) {
	                	input->altEnter = true;
	                }

	                if(keyDown) {
	                	input->anyKey = true;
	                }

	                TranslateMessage(&message); 
	                DispatchMessage(&message); 
	            } break;

	            case WM_SYSKEYDOWN:
	            case WM_SYSKEYUP: {
	                uint vk = uint(message.wParam);
	            	bool keyDown = (message.message == WM_SYSKEYDOWN);

	            	if(keyDown) {
		            	if(vk == VK_RETURN) {
		            		input->altEnter = true;
		            	}
	            	}

	            	TranslateMessage(&message); 
	            	DispatchMessage(&message); 
	            };

	            case WM_CHAR: {
	                // input->inputCharacters[input->inputCharacterCount] = (char)uint(message.wParam);
	            	uint charIndex = uint(message.wParam);
	            	if(charIndex < ' ' || charIndex > '~') break;
	            	char c = (char)charIndex;
	                input->inputCharacters[input->inputCharacterCount] = c;
	                input->inputCharacterCount++;
	            } break;

	            case WM_INPUT: {
	            	RAWINPUT inputBuffer;
	            	UINT rawInputSize = sizeof(inputBuffer);
	            	GetRawInputData((HRAWINPUT)(message.lParam), RID_INPUT, &inputBuffer, &rawInputSize, sizeof(RAWINPUTHEADER));
	            	RAWINPUT* raw = (RAWINPUT*)(&inputBuffer);
	            	
	            	if (raw->header.dwType == RIM_TYPEMOUSE && raw->data.mouse.usFlags == MOUSE_MOVE_RELATIVE) {

	            	    input->mouseDelta += vec2(raw->data.mouse.lLastX, raw->data.mouse.lLastY);

	            	    USHORT buttonFlags = raw->data.mouse.usButtonFlags;

	            	    if(mouseInClient) {
							if(buttonFlags & RI_MOUSE_LEFT_BUTTON_DOWN) {
								// SetCapture(windowHandle);
								input->mouseButtonPressed[0] = true; 
								input->mouseButtonDown[0] = true; 
							} else if(buttonFlags & RI_MOUSE_RIGHT_BUTTON_DOWN) {
								// SetCapture(windowHandle);
								input->mouseButtonPressed[1] = true; 
								input->mouseButtonDown[1] = true; 
							} else if(buttonFlags & RI_MOUSE_MIDDLE_BUTTON_DOWN) {
								// SetCapture(windowHandle);
								input->mouseButtonPressed[2] = true; 
								input->mouseButtonDown[2] = true; 
							} else if(buttonFlags & RI_MOUSE_WHEEL) {
								input->mouseWheel += ((SHORT)raw->data.mouse.usButtonData) / WHEEL_DELTA;
							}
	            	    }

	            	    if(buttonFlags & RI_MOUSE_LEFT_BUTTON_UP) {
	            	    	// ReleaseCapture();
	            	    	input->mouseButtonDown[0] = false; 
	            	    	input->mouseButtonReleased[0] = true; 
	            	    } else if(buttonFlags & RI_MOUSE_RIGHT_BUTTON_UP) {
	            	    	// ReleaseCapture();
	            	    	input->mouseButtonDown[1] = false; 
	            	    	input->mouseButtonReleased[1] = true; 
	            	    } else if(buttonFlags & RI_MOUSE_MIDDLE_BUTTON_UP) {
	            	    	// ReleaseCapture();
	            	    	input->mouseButtonDown[2] = false; 
	            	    	input->mouseButtonReleased[2] = true; 
	            	    }

	            	} break;

	            	TranslateMessage(&message);
	            	DispatchMessage(&message);
	            } break;

	            case WM_DESTROY: 
	            case WM_CLOSE: 
	            case WM_QUIT: 
	            	input->closeWindow = true;
	            	break;

	            default: {
	                TranslateMessage(&message); 
	                DispatchMessage(&message); 
	            } break;
	        }
	    }

	    if(!sd->windowIsFocused) {
	    	for(int i = 0; i < arrayCount(input->mouseButtonPressed); i++) input->mouseButtonPressed[i] = 0;
	    	for(int i = 0; i < arrayCount(input->mouseButtonReleased); i++) input->mouseButtonReleased[i] = 0;
	    	input->mouseWheel = 0;
	    }
	    sd->setFocus = false;

	    // In case we clear because of focus.
	    bool closeWindowTemp = input->closeWindow;

	    if(sd->killedFocus) {
	    	for(int i = 0; i < KEYCODE_COUNT; i++) {
	    		input->keysDown[i] = false;
	    	}
	    	*input = {};

	    	for(int i = 0; i < arrayCount(input->mouseButtonReleased); i++) {
		    	input->mouseButtonReleased[i] = true;
	    	}

	    	sd->killedFocus = false;
	    }

	    if(input->altEnter) {
	    	input->keysPressed[KEYCODE_RETURN] = false;
	    }

	    input->closeWindow = closeWindowTemp;

	    input->mousePos = getMousePos(windowHandle, false);
	    input->mousePosNegative = getMousePos(windowHandle, true);

	    input->mousePosScreen = getMousePosS(false);
	    input->mousePosNegativeScreen = getMousePosS(true);

	    input->lastMousePos = input->mousePos;

	    input->firstFrame = false;

	    SwitchToFiber(sd->mainFiber);
	}
}