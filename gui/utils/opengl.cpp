#include "opengl.h"
#include <QOpenGLContext>


bool checkDesktopGl()
{
    auto checkOpenGL = []()
    {
        //qtbase\src\plugins\platforms\windows\qwindowsopengltester.cpp
        //we try this code before it crashes
#ifdef _WIN32
        typedef HGLRC(WINAPI* CreateContextType)(HDC);
        typedef BOOL(WINAPI* DeleteContextType)(HGLRC);
        typedef BOOL(WINAPI* MakeCurrentType)(HDC, HGLRC);
        typedef PROC(WINAPI* WglGetProcAddressType)(LPCSTR);

        HMODULE lib = nullptr;
        HWND wnd = nullptr;
        HDC dc = nullptr;
        HGLRC context = nullptr;
        LPCTSTR className = L"opengltest";

        CreateContextType CreateContext = nullptr;
        DeleteContextType DeleteContext = nullptr;
        MakeCurrentType MakeCurrent = nullptr;
        WglGetProcAddressType WGL_GetProcAddress = nullptr;

        bool result = false;

        lib = LoadLibraryA("opengl32.dll");
        if (lib)
        {
            CreateContext = reinterpret_cast<CreateContextType>(reinterpret_cast<QFunctionPointer>(::GetProcAddress(lib, "wglCreateContext")));
            if (!CreateContext)
                goto cleanup;
            DeleteContext = reinterpret_cast<DeleteContextType>(reinterpret_cast<QFunctionPointer>(::GetProcAddress(lib, "wglDeleteContext")));
            if (!DeleteContext)
                goto cleanup;
            MakeCurrent = reinterpret_cast<MakeCurrentType>(reinterpret_cast<QFunctionPointer>(::GetProcAddress(lib, "wglMakeCurrent")));
            if (!MakeCurrent)
                goto cleanup;
            WGL_GetProcAddress = reinterpret_cast<WglGetProcAddressType>(reinterpret_cast<QFunctionPointer>(::GetProcAddress(lib, "wglGetProcAddress")));
            if (!WGL_GetProcAddress)
                goto cleanup;

            WNDCLASS wclass;
            wclass.cbClsExtra = 0;
            wclass.cbWndExtra = 0;
            wclass.hInstance = static_cast<HINSTANCE>(GetModuleHandle(nullptr));
            wclass.hIcon = nullptr;
            wclass.hCursor = nullptr;
            wclass.hbrBackground = HBRUSH(COLOR_BACKGROUND);
            wclass.lpszMenuName = nullptr;
            wclass.lpfnWndProc = DefWindowProc;
            wclass.lpszClassName = className;
            wclass.style = CS_OWNDC;
            if (!RegisterClass(&wclass))
                goto cleanup;
            wnd = CreateWindow(className, L"openglproxytest", WS_OVERLAPPED, 0, 0, 640, 480, nullptr, nullptr, wclass.hInstance, nullptr);
            if (!wnd)
                goto cleanup;
            dc = GetDC(wnd);
            if (!dc)
                goto cleanup;

            PIXELFORMATDESCRIPTOR pfd;
            memset(&pfd, 0, sizeof(PIXELFORMATDESCRIPTOR));
            pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
            pfd.nVersion = 1;
            pfd.dwFlags = PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_GENERIC_FORMAT;
            pfd.iPixelType = PFD_TYPE_RGBA;
            int pixelFormat = ChoosePixelFormat(dc, &pfd);
            if (!pixelFormat)
                goto cleanup;
            if (!SetPixelFormat(dc, pixelFormat, &pfd))
                goto cleanup;
            context = CreateContext(dc);
            if (!context)
                goto cleanup;
            if (!MakeCurrent(dc, context))
                goto cleanup;

            typedef const GLubyte* (APIENTRY* GetString_t)(GLenum name);
            auto GetString = reinterpret_cast<GetString_t>(reinterpret_cast<QFunctionPointer>(::GetProcAddress(lib, "glGetString")));
            if (GetString)
            {
                if (const char* versionStr = reinterpret_cast<const char*>(GetString(GL_VERSION))) {
                    const QByteArray version(versionStr);
                    const int majorDot = version.indexOf('.');
                    if (majorDot != -1)
                    {
                        int minorDot = version.indexOf('.', majorDot + 1);
                        if (minorDot == -1)
                            minorDot = version.size();

                        const int major = version.mid(0, majorDot).toInt();
                        const int minor = version.mid(majorDot + 1, minorDot - majorDot - 1).toInt();
                        if (major == 1)
                            result = false;
                    }
                }
            }
            else
            {
                result = false;
            }

            if (WGL_GetProcAddress("glCreateShader"))
                result = true;
        }

    cleanup:
        if (MakeCurrent)
            MakeCurrent(nullptr, nullptr);
        if (context)
            DeleteContext(context);
        if (dc && wnd)
            ReleaseDC(wnd, dc);
        if (wnd)
        {
            DestroyWindow(wnd);
            UnregisterClass(className, GetModuleHandle(nullptr));
        }

        return result;

#else
        return false;
#endif //_WIN32
    };

    static bool openGLSupported = checkOpenGL();
    return openGLSupported;
}

bool isOpenGLSupported()
{
    auto checkOpenGL = []()
    {
        QOpenGLContext context;
        return context.create();
    };
    static bool openGLSupported = checkOpenGL();

    return openGLSupported;
}