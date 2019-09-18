#pragma once

namespace Ui {
    class MainWindow;
}

struct MacTitlebarPrivate;

class MacTitlebar
{
public:
    MacTitlebar(Ui::MainWindow* _mainWindow);
    virtual ~MacTitlebar();

    void setup();
    void setTitleText(const QString& _text);

    void updateTitleTextColor();
    void updateTitleBgColor();

private:
    void initWindowTitle();

    Ui::MainWindow* mainWindow_;
    std::unique_ptr<MacTitlebarPrivate> d;
};
