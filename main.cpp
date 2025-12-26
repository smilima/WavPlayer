#include "Application.h"
#include <Windows.h>
#include <ShellScalingApi.h>

#pragma comment(lib, "Shcore.lib")

int WINAPI wWinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine,
    _In_ int nCmdShow
)
{
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
