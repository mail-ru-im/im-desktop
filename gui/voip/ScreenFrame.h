#pragma once
#include "VoipProxy.h"
#include "media/permissions/MediaCapturePermissions.h"
#include "ShapedWidget.h"

namespace Ui
{

class ScreenFrame : public ShapedWidget
{
    Q_OBJECT
public:
    enum Feature
    {
        NoFeatures = 0x0,
        FadeIn = 0x1,
        FadeOut = 0x2,
        ScreenPinned = 0x4,
        FadeMask = FadeIn | FadeOut
    };
    Q_DECLARE_FLAGS(Features, Feature)

    explicit ScreenFrame(QScreen* _screen, QWidget* _parent = nullptr);
    explicit ScreenFrame(QWidget* _parent);
    ~ScreenFrame();

    static Qt::WindowFlags preferredWindowFlags();

    void setFeatures(Features _features);
    Features features() const;

    void setScreen(QScreen* _screen);

private Q_SLOTS:
    void changeGeometry(const QRect& _screenRect);
    void fade(const QVariant& _opacity);
    void fadeIn();
    void fadeOut();
    void disappear();

Q_SIGNALS:
    void closed();

protected:
    void paintEvent(QPaintEvent*) override;
    void showEvent(QShowEvent* _event) override;
    void closeEvent(QCloseEvent* _event) override;

private:
    friend class ScreenFramePrivate;
    std::unique_ptr<class ScreenFramePrivate> d;
};
Q_DECLARE_OPERATORS_FOR_FLAGS(ScreenFrame::Features)

class SharingScreenFrame : public ScreenFrame
{
    Q_OBJECT
public:
    explicit SharingScreenFrame(QScreen* _screen, QWidget* _parent = nullptr);
    explicit SharingScreenFrame(QWidget* _parent);
    ~SharingScreenFrame();

public Q_SLOTS:
    void showMessage(const QString& _text, std::chrono::milliseconds _timeout);
    void clearMessage();
    void hideMessage();

protected:
    void resizeEvent(QResizeEvent* _event) override;

private:
    void init();

private:
    friend class SharingScreenFramePrivate;
    std::unique_ptr<class SharingScreenFramePrivate> d;
};

class ScreenSharingManager : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(ScreenSharingManager)

public:
    enum SharingState
    {
        Shared,
        NonShared
    };

    static ScreenSharingManager& instance();

    SharingState state() const;

public Q_SLOTS:
    void startSharing(int _screenIndex);
    void stopSharing();
    void toggleSharing(int _screenIndex);

Q_SIGNALS:
    void sharingStateChanged(SharingState _state, int _screenIndex);
    void needShowScreenPermissionsPopup(media::permissions::DeviceType type);

private Q_SLOTS:
    void onVoipVideoDeviceSelected(const voip_proxy::device_desc& _desc);

private:
    ScreenSharingManager();

private:
    QPointer<SharingScreenFrame> frame_;
    int currentIndex_;
    bool isCameraSelected_;
};

} // end namespace Ui
