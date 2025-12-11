#include "Application.h"
#include "MainWindow.h"

Application& Application::getInstance() {
    static Application instance;
    return instance;
}

Application::~Application() {
    shutdown();
}

bool Application::initialize(HINSTANCE hInstance) {
    m_hInstance = hInstance;

    // Initialize COM (needed for some Windows APIs)
    HRESULT hrCoInit = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(hrCoInit)) {
        MessageBox(nullptr, L"Failed to initialize COM", L"Error", MB_OK);
        return false;
    }

    // Initialize Direct2D factory
    HRESULT hr = D2D1CreateFactory(
        D2D1_FACTORY_TYPE_SINGLE_THREADED,
        &m_d2dFactory
    );
    if (FAILED(hr)) {
        MessageBox(nullptr, L"Failed to create Direct2D factory", L"Error", MB_OK);
        CoUninitialize();
        return false;
    }

    // Initialize DirectWrite factory for text rendering
    hr = DWriteCreateFactory(
        DWRITE_FACTORY_TYPE_SHARED,
        __uuidof(IDWriteFactory),
        reinterpret_cast<IUnknown**>(&m_dwriteFactory)
    );
    if (FAILED(hr)) {
        MessageBox(nullptr, L"Failed to create DirectWrite factory", L"Error", MB_OK);
        CoUninitialize();
        return false;
    }

    // Create the main window
    m_mainWindow = std::make_unique<MainWindow>();
    if (!m_mainWindow->create(L"Simple DAW", 1400, 800)) {
        MessageBox(nullptr, L"Failed to create main window", L"Error", MB_OK);
        CoUninitialize();
        return false;
    }

    return true;
}

int Application::run() {
    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return static_cast<int>(msg.wParam);
}

void Application::shutdown() {
    m_mainWindow.reset();

    if (m_dwriteFactory) {
        m_dwriteFactory->Release();
        m_dwriteFactory = nullptr;
    }
    if (m_d2dFactory) {
        m_d2dFactory->Release();
        m_d2dFactory = nullptr;
    }

    CoUninitialize();
}
