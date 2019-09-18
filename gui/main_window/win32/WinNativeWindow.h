#pragma once

namespace Ui
{
    const QEvent::Type NativeWindowMoved = static_cast<QEvent::Type>(QEvent::User + 1);

    class NativeWindowMovedEvent : public QEvent
    {
    public:
        NativeWindowMovedEvent(int _x, int _y) : QEvent(NativeWindowMoved), x_(_x), y_(_y) {}

        QPoint getPoint() const { return QPoint(x_, y_); }

    private:
        int x_;
        int y_;
    };

    constexpr int NativeWindowBorderWidth = 4;

    bool isNativeWindowShowCmd(HWND _hwnd, UINT _cmd);

    class WinNativeWindow
    {
        enum class Style : DWORD
        {
            // WS_THICKFRAME: without this the window cannot be resized and so aero snap, de-maximizing and minimizing won't work
            // WS_CAPTION: enables aero minimize animation/transition
            BasicBorderless = WS_POPUP | WS_THICKFRAME | WS_SYSMENU | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_CLIPCHILDREN
            | WS_CAPTION, // hot fix for win xp. TODO fix rounded corners on xp.
            AeroBorderless = BasicBorderless | WS_CAPTION
        };

    public:
        WinNativeWindow();
        ~WinNativeWindow();

        static LRESULT CALLBACK wndProc(HWND _hWnd, UINT _message, WPARAM _wParam, LPARAM _lParam);

        bool init(const QString& _title);

        void setMinimumSize(int _width, int _height);
        int getMinimumHeight() const;
        int getMinimumWidth() const;

        void setMaximumSize(int _width, int _height);
        int getMaximumHeight() const;
        int getMaximumWidth() const;

        void setGeometry(const QRect& _rect);
        void setGeometry(int _x, int _y, int _width, int _height) { setGeometry(QRect(_x, _y, _width, _height)); };

        HWND hwnd() const;

        void setChildWindow(HWND _hWnd);
        void setChildWidget(QWidget* _widget);

    private:
        static bool isAeroEnabled_;
        static HWND childWindow_;
        static QWidget* childWidget_;

        HWND hWnd_;

        struct sizeType
        {
            sizeType() : required_(false), width_(0), height_(0) {}
            bool required_;
            int width_;
            int height_;
        };
        sizeType minimumSize_;
        sizeType maximumSize_;
    };
}
