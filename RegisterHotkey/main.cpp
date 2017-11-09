#include <stdio.h>
#include <thread>
#include <cstdio>
#include <iostream>
#include <map>
#include <string>
#include <stack>
#include <queue>
#include <mutex>
#include <condition_variable>

#include <windows.h>

#define WM_HOTKEY_REGISTER WM_USER + 0x0001
#define WM_HOTKEY_UNREGISTER WM_USER + 0x0002

struct Hotkey {
	std::string name;
	UINT modifiers;
	UINT vk;
	int id;
};

std::queue<int> pressedhotkeys;
std::map<int, Hotkey> hotkeys;

bool thread_ready;
std::mutex thread_ready_mutex;
std::condition_variable thread_ready_cv;

uint16_t currentHotkeyId = 1;
std::queue<uint16_t> freeHotkeyIds;


int threadmain() {
	MSG message;
	BOOL breet;
	PeekMessage(&message, (HWND)-1, 0, 0, PM_NOREMOVE);
	if (!thread_ready) {
		std::unique_lock<std::mutex> thread_ready_mutex;
		thread_ready = true;
		thread_ready_cv.notify_all();
	}
	while ((breet = GetMessage(&message, (HWND)-1, 0, 0)) != 0) {
		//TranslateMessage(&message);

		switch (message.message) {
			case WM_HOTKEY:
				{
				SHORT state = GetAsyncKeyState(message.lParam >> 16);
				std::cout << state << std::endl;
				break;
				}
			case WM_HOTKEY_REGISTER:
				{
					Hotkey* hk = reinterpret_cast<Hotkey*>(message.lParam);

					if (freeHotkeyIds.size() > 0) {
						hk->id = freeHotkeyIds.front();
						freeHotkeyIds.pop();
					} else {
						hk->id = currentHotkeyId++;
					}

					BOOL rhk_ret;
					if ((rhk_ret = RegisterHotKey(0, hk->id, hk->modifiers, hk->vk)) != 0) {
						hotkeys.insert(std::make_pair((hk->modifiers) | (hk->vk << 8), *hk));
					} else {
						DWORD error = GetLastError();
						std::cout << "error while register hotkey" << error << '\n';
					}
					break;
				}
			case WM_HOTKEY_UNREGISTER:
				{
					Hotkey* hk = reinterpret_cast<Hotkey*>(message.lParam);

					UnregisterHotKey(0, hk->id);
					freeHotkeyIds.push(hk->id);

					break;
				}
			default:
				std::cout << "Unknown message." << std::endl;
				break;
		}

		if (!WaitMessage())
			break;
	}
	return 0;
}
std::thread thatthread;

int main() {
	std::unique_lock<std::mutex> ulock(thread_ready_mutex);
	thatthread = std::thread(threadmain);
	PostThreadMessage(GetThreadId(thatthread.native_handle()),
		WM_APP, 0, 0);
	do {
		thread_ready_cv.wait(ulock, [] {return thread_ready; });
		std::cout << "mainloop woken" << '\n';
	} while (!thread_ready);

	Hotkey myhotkey;
	myhotkey.modifiers = MOD_ALT;
	myhotkey.vk = 0x41;
	myhotkey.name = "Alt+A";

	PostThreadMessage(GetThreadId(thatthread.native_handle()),
		WM_HOTKEY_REGISTER, 0, reinterpret_cast<LPARAM>(&myhotkey));

	thatthread.join();
	return 0;
}