#pragma once
#ifdef _WIN32
class QWidget;
namespace platform_win32
{
    void showInWorkspace(QWidget* _w, QWidget* _initiator);
    bool windowIsOverlapped(QWidget* _window);
    bool isSpacebarDown();
}
#endif
