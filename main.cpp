#include "Application.h"
#include <Windows.h>
#include <ShellScalingApi.h>

#pragma comment(lib, "Shcore.lib")

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, 
                    LPWSTR lpCmdLine, int nCmdShow) {
    // Enable per-monitor DPI awareness (Windows 8.1+)
    // Falls back gracefully on older systems
    SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);

    Application& app = Application::getInstance();
    
    if (!app.initialize(hInstance)) {
        return -1;
    }

    int result = app.run();
    
    app.shutdown();
    
    return result;
}
