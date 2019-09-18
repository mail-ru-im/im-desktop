#pragma once

#include <QWidget>

#include "animation/animation.h"
#include "../controls/TextUnit.h"
#include "../types/filesharing_download_result.h"

namespace Ui
{

class ToastBase : public QWidget
{
    Q_OBJECT
public:
    explicit ToastBase(QWidget *parent = nullptr);

    void showAt(const QPoint& _center, bool _onTop = false);
    virtual bool keepRows() const { return false;}
    virtual bool sameSavePath(const ToastBase* _other) const { return true;}
    virtual QString getPath() const { return QString(); }
signals:
    void dissapeared();

protected:
    void paintEvent(QPaintEvent* _event) override;
    void enterEvent(QEvent* _event) override;
    void leaveEvent(QEvent* _event) override;

    bool eventFilter(QObject* _object, QEvent* _event) override;

    virtual void drawContent(QPainter& _p) = 0;

    void handleParentResize(const QSize& _newSize, const QSize& _oldSize);
    void handleMouseLeave();
    virtual void updateSize() {}

    virtual QPoint shiftToastPos() { return QPoint(); }

private Q_SLOTS:
    void startHide();

private:
    QTimer hideTimer_;
    QTimer startHideTimer_;

    anim::Animation opacityAnimation_;
    anim::Animation moveAnimation_;

    QGraphicsOpacityEffect* opacity_;
};

class ToastManager : public QObject
{
    Q_OBJECT
public:
    static ToastManager* instance();

    void showToast(ToastBase* _toast, QPoint _pos);

    void hideToast();
    void moveToast();

protected:
    bool eventFilter(QObject* _object, QEvent* _event) override;

private:
    ToastManager() = default;
    ToastManager(const ToastManager& _other) = delete;

    QPointer<ToastBase> bottomToast_;
    QPointer<ToastBase> topToast_;
};

class Toast : public ToastBase // simple toast with text
{
public:
    Toast(const QString& _text, QWidget* _parent = nullptr, int _maxLineCount = 1);

protected:
    void drawContent(QPainter& _p) override;
    void updateSize() override;

private:
    Ui::TextRendering::TextUnitPtr textUnit_;
};

class SavedPathToast_p;

class SavedPathToast : public ToastBase
{
public:
    SavedPathToast(const QString& _path, QWidget* _parent = nullptr);

protected:
    void drawContent(QPainter& _p) override;

    void mouseMoveEvent(QMouseEvent* _event) override;
    void mousePressEvent(QMouseEvent* _event) override;
    void mouseReleaseEvent(QMouseEvent* _event) override;
    void leaveEvent(QEvent* _event) override;

private:
    std::unique_ptr<SavedPathToast_p> d;
};

class DownloadFinishedToast_p;

class DownloadFinishedToast : public ToastBase
{
public:
    DownloadFinishedToast(const Data::FileSharingDownloadResult& _result, QWidget* _parent = nullptr);
    ~DownloadFinishedToast();


    QString getPath() const override;
    bool keepRows() const override { return true;}
    bool sameSavePath(const ToastBase* _other) const override;
protected:
    void drawContent(QPainter& _p) override;

    void mouseMoveEvent(QMouseEvent* _event) override;
    void mousePressEvent(QMouseEvent* _event) override;
    void mouseReleaseEvent(QMouseEvent* _event) override;
    void leaveEvent(QEvent* _event) override;
    void updateSize() override;

    QPoint shiftToastPos() override;

private:
    void compose(bool _isMinimal, const int maxWidth, const QFontMetrics fm, int fileWidth, int pathWidth, int _totalWidth);
private:
    std::unique_ptr<DownloadFinishedToast_p> d;
    int xOffset_;
    int yOffset_;
    int fileWidth_;
    int pathWidth_;
    int totalWidth_;
};

}

namespace Utils
{
    void showToastOverMainWindow(const QString& _text, int _bottomOffset, int _maxLineCount = 1); // show text toast over main window
    void showDownloadToast(const Data::FileSharingDownloadResult& _result);
}
