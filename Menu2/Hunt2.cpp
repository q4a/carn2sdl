/***************************************************
* AtmosFear 2.1
* Hunt2.cpp
*
* Engine Related Processes
*
*/

#define _MAIN_
#include "Hunt.h"

#include "resource.h" // For IDI_ICON1

#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <thread>
#include <chrono>


std::streambuf* g_COutBuf = nullptr;
std::ofstream g_LogFile;
std::chrono::steady_clock::duration Timer::m_TimeStart;


void CreateLog()
{
#if !defined(_DEBUG)
	g_LogFile.open("menu.log", std::ios::trunc);

	if (g_LogFile.is_open()) {
		g_COutBuf = std::cout.rdbuf(g_LogFile.rdbuf());
	}
#endif //_DEBUG

#ifdef _iceage
	std::cout << "Carnivores: Ice Age - Menu\n";
#else // _iceage
	std::cout << "Carnivores 2 - Menu\n";
#endif // !_iceage

	std::cout << " Version: " << VERSION_MAJOR << "." << VERSION_MINOR;
	if (VERSION_REVISION)
		std::cout << "." << VERSION_REVISION;
	std::cout << " ";

#ifdef _DEBUG
	std::cout << "DEBUG ";
#endif // _DEBUG
#ifdef _W64
	std::cout << "64-Bit ";
#else // !_W64
	std::cout << "32-Bit ";
#endif // _W64
	std::cout << __DATE__ << std::endl;
	PrintLogSeparater();
}


void CloseLogs()
{
#ifdef _DEBUG
	std::cout.rdbuf(g_COutBuf);
	g_LogFile.close();
#endif //_DEBUG
}


void PrintLogSeparater()
{
	std::cout << std::setfill('=') << std::setw(40) << "=" << std::endl;
}

int LaunchProcess(const std::string& exe_name, std::vector<std::string> args)
{
	uint32_t exitCode = 0;

	SDL_MinimizeWindow(window);

#ifdef _WIN32
	PROCESS_INFORMATION processInformation = { 0 };
	STARTUPINFO startupInfo = { 0 };
	startupInfo.cb = sizeof(startupInfo);

	// Build single string out of args
	std::stringstream cmd_line;
	for (auto iter = args.begin(); iter != args.end(); iter++) {
		cmd_line << " " << (*iter).c_str();
	}

	// Create the process
	BOOL result = CreateProcess(exe_name.c_str(), const_cast<char*>(cmd_line.str().c_str()),
		NULL, NULL, FALSE,
		NORMAL_PRIORITY_CLASS,
		NULL, NULL, &startupInfo, &processInformation);

	if (!result)
	{
		DWORD error = GetLastError();
		std::cout << "CreateProcess(" << exe_name << ", " << cmd_line.str() << ") failed with error code: " << error << std::endl;
	}

	// Successfully created the process.  Wait for it to finish.
	WaitForSingleObject(processInformation.hProcess, INFINITE);

	// Get the exit code.
	result = GetExitCodeProcess(processInformation.hProcess, (DWORD*)&exitCode);

	if (result)
	{
		std::cout << "! Warning !\nThe process exited with error code: " << static_cast<int>(result) << "\n";
		std::cout << " Check the associated `render.log` for more details, or debug the binary `" << exe_name << "`" << std::endl;
	}

	// Close the handles.
	CloseHandle(processInformation.hProcess);
	CloseHandle(processInformation.hThread);
#else
	// Do the *NIX thing (fork/exec)
#endif

	SDL_RestoreWindow(window);

	// Reset to the main menu like the original game does
	ChangeMenuState(MENU_MAIN);

	return exitCode;
}

std::string errordialog_str = "";


void ShowErrorMessage(const std::string& error_text)
{
	errordialog_str = error_text;
	std::cout << error_text << std::endl;
}

bool CreateMainWindow()
{
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		std::stringstream ss;
		ss << "Failed to initialize SDL: " << SDL_GetError() << std::endl;
		throw std::runtime_error(ss.str());
	}

#if 0
#ifdef _iceage
	const char* title = "Carnivores: Ice Age - Menu";
#else // !_iceage
	const char* title = "Carnivores 2 - Menu";
#endif
#else
	// Partialfix: P.Rex requested a method to change the title but this is a compromise
	const char *title = "Carnivores - Menu";
#endif

	window = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, SDL_WINDOW_SHOWN);
	if (window == NULL)
	{
		printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
	}

	screenSurface = SDL_GetWindowSurface(window);

// XXX
//	wc.hIcon = LoadIcon(wc.hInstance, MAKEINTRESOURCE(IDI_ICON1));
//	wc.hIconSm = LoadIcon(wc.hInstance, MAKEINTRESOURCE(IDI_ICON1));
//	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
//	wc.lpszClassName = "CarnivoresMenu2";

	std::cout << "Main Window Creation: Ok!" << std::endl;

	return true;
}


int main(int argc, char* argv[]) {
	Timer::Init();

	try {
		// Create Log Files
		CreateLog();

		CreateMainWindow();
		InitInterface();
		//InitAudioSystem(hwndMain, NULL, 0);

		LoadResourcesScript();
		LoadResources();

		// -- Message Loop
		std::cout << "Entering Messages Loop." << std::endl;

		SDL_Event e;
		g_QuitRequested = false;
		bool shown = true;

		while (!g_QuitRequested)
		{
			//Handle SDL event queue
			while (SDL_PollEvent(&e) != 0)
			{
				switch (e.type)
				{
				case SDL_QUIT:
					// user requested quit
					g_QuitRequested = true;
					break;
				case SDL_WINDOWEVENT:
					switch (e.window.event) {
					case SDL_WINDOWEVENT_MINIMIZED:
						shown = false;
						break;
					case SDL_WINDOWEVENT_RESTORED:
						shown = true;
						break;
					}
					break;
				case SDL_KEYDOWN:
					MenuKeyCharEvent(e.key.keysym.sym);
					break;
				case SDL_MOUSEWHEEL:
					{
						std::cout << "Mouse wheel: " << e.wheel.x << "," << e.wheel.y << " dir: " << e.wheel.direction;
						MenuMouseScrollEvent(g_MenuState, e.wheel.y);
					}
					break;
				default:
					break;
				}
			}

			if (!shown) {
				// Sleep when the window is not the active one (10 ticks/frames per second)
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
			} else {
				ProcessMenu();
			}
		}

		std::cout << "Exited Message Loop." << std::endl;
	}
	catch (std::runtime_error& e) {
		std::stringstream ss;
		ss << "\r\n! FATAL EXCEPTION: Runtime Error !\r\n";
		ss << "An std::runtime_error exception has occurred, here are some details:\r\n" << e.what() << "\r\n";
		std::cout << ss.str() << std::endl;
		
		//MessageBox(HWND_DESKTOP, "A fatal exception has occured and Menu2 must close.\r\nPlease refer to the `menu.log`", "Runtime Exception", MB_OK | MB_ICONERROR);
		
		std::string s = ss.str();
		ss.str(""); ss.clear();
		ss << "A fatal runtime exception has occured and Menu2 must close.\r\nPlease refer to the `menu.log`\r\n" << s;

		ShowErrorMessage(ss.str());
	}
	catch (std::exception& e) {
		std::stringstream ss;
		ss << "\r\n! UNCAUGHT EXCEPTION: C++ Standard Exception !\r\n";
		ss << "An std::exception has occurred but was not handled, here are some details:\r\n" << e.what() << "\r\n";
		std::cout << ss.str() << std::endl;

		//MessageBox(HWND_DESKTOP, "A fatal exception has occured and Menu2 must close.\r\nPlease refer to the `menu.log`", "Runtime Exception", MB_OK | MB_ICONERROR);

		std::string s = ss.str();
		ss.str(""); ss.clear();
		ss << "An uncaught C++ exception has occured and Menu2 must close.\r\nPlease refer to the `menu.log`\r\n" << s;

		ShowErrorMessage(ss.str());
	}

	ReleaseResources();
	//Audio_Shutdown();
	ShutdownInterface();

	SDL_DestroyWindow(window);
	SDL_Quit();

	CloseLogs();

	return 0;
}
