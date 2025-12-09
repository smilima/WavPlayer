#pragma once
#include <Windows.h>
#include <d2d1.h>
#include <dwrite.h>
#include <string>
#include <memory>
#include <numeric>

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "winmm.lib")

// Forward declarations
class MainWindow;

class Application {
public:
    static Application& getInstance();
    
    bool initialize(HINSTANCE hInstance);
    int run();
    void shutdown();

    ID2D1Factory* getD2DFactory() const { return m_d2dFactory; }
    IDWriteFactory* getDWriteFactory() const { return m_dwriteFactory; }
    HINSTANCE getHInstance() const { return m_hInstance; }

private:
    Application() = default;
    ~Application();
    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    HINSTANCE m_hInstance = nullptr;
    ID2D1Factory* m_d2dFactory = nullptr;
    IDWriteFactory* m_dwriteFactory = nullptr;
    std::unique_ptr<MainWindow> m_mainWindow;
};
