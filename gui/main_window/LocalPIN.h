#pragma once

namespace Ui
{

using DeferredCallback = std::function<void()>;

class DeferredAction : public QObject
{
    Q_OBJECT
public:

    DeferredAction(DeferredCallback&& _callback, QObject* _parent = nullptr)
        : QObject(_parent)
        , callback_(std::move(_callback))
    {}

    void execute()
    {
        if (callback_)
            callback_();
    }

private:
    DeferredCallback callback_;
};


class LocalPINWidget_p;

class LocalPINWidget : public QFrame
{
    Q_OBJECT

public:

    enum class Mode
    {
        SetPIN,
        ChangePIN,
        VerifyPIN,
        RemovePIN
    };

    LocalPINWidget(Mode _mode, QWidget* _parent);

    ~LocalPINWidget();

    bool eventFilter(QObject* _object, QEvent* _event) override;

private Q_SLOTS:
    void onLogOut();
    void onCancel();
    void onOk();
    void onInputChanged(const QString& _text);
    void onPINChecked(qint64 _seq, bool _result);
    void onMyInfoLoaded();

protected:
    void keyPressEvent(QKeyEvent* _event) override;
    void showEvent(QShowEvent* _event) override;

private:
    std::unique_ptr<LocalPINWidget_p> d;
};

class LocalPIN : public QObject
{
    Q_OBJECT
public:
    static LocalPIN* instance();

    void setEnabled(bool _enabled);

    void setLocked(bool _locked);

    void lock();
    void unlock();

    bool locked() const;
    bool enabled() const;
    bool autoLockEnabled() const;

    void setLockTimeout(std::chrono::minutes _minutes);
    std::chrono::minutes lockTimeoutMinutes() const;

    void setDeferredAction(DeferredAction* _action);

private Q_SLOTS:
    void onCheckActivityTimeout();

private:
    LocalPIN();
    LocalPIN(const LocalPIN&) = delete;
    LocalPIN(const LocalPIN&&) = delete;

    bool enabled_ = false;
    bool locked_ = false;

    QPointer<DeferredAction> deferrredAction_;

    QTimer checkActivityTimer_;
};

class FooterWidget_p;

class FooterWidget : public QWidget
{
    Q_OBJECT
public:
    FooterWidget(QWidget* _parent);
    ~FooterWidget();

    QSize sizeHint() const override;

Q_SIGNALS:
    void logOutClicked();

protected:
    void paintEvent(QPaintEvent* _event) override;
    void mouseMoveEvent(QMouseEvent* _event) override;
    void mousePressEvent(QMouseEvent* _event) override;
    void mouseReleaseEvent(QMouseEvent* _event) override;
    void resizeEvent(QResizeEvent* _event) override;

private:
    std::unique_ptr<FooterWidget_p> d;
};

}
