#pragma once

#include "../../../namespaces.h"
#include "../../../controls/ClickWidget.h"
#include "../../../controls/CustomButton.h"

#include "FileSharingBlockBase.h"

UI_NS_BEGIN

namespace TextRendering
{
    class TextUnit;
}

UI_NS_END

UI_COMPLEX_MESSAGE_NS_BEGIN

namespace PttDetails
{
    class PlayButton : public ClickableWidget
    {
        Q_OBJECT

    public:
        PlayButton(QWidget* _parent, const QString& _aimId);
        void setPressed(const bool _isPressed);

        enum class ButtonState
        {
            play,
            pause
        };
        void setState(const ButtonState _state);
        void updateStyle();

    protected:
        void paintEvent(QPaintEvent* _e) override;

    private:
        QString aimId_;
        bool isPressed_;
        ButtonState state_;

        QColor normal_;
        QColor hovered_;
        QColor pressed_;
    };

    class ButtonWithBackground : public CustomButton
    {
        Q_OBJECT

    public:
        ButtonWithBackground(QWidget* _parent, const QString& _icon, const QString& _aimId, bool _isOutgoing = false);
        void updateStyle();

    protected:
        void paintEvent(QPaintEvent* _e) override;
        QString aimId_;
        bool isOutgoing() const;

    private:
        const bool isOutgoing_;
    };

    class ProgressWidget : public QWidget
    {
    public:
        enum class ButtonType
        {
            play,
            text
        };

        ProgressWidget(QWidget* _parent, const ButtonType _type, bool _isOutgoing);
        ~ProgressWidget();

        void setProgress(const double _progress);
        void updateStyle();

    protected:
        void paintEvent(QPaintEvent* _e) override;

    private:
        ButtonType type_;
        double progress_;
        const bool isOutgoing_;
        QVariantAnimation* anim_;
    };
}

class PttBlockLayout;

class PttBlock final : public FileSharingBlockBase
{
    Q_OBJECT

public:
    PttBlock(
        ComplexMessageItem *_parent,
        const QString &_link,
        const int32_t _durationSec,
        int64_t _id,
        int64_t _prevId);

    PttBlock(ComplexMessageItem *_parent,
        const HistoryControl::FileSharingInfoSptr& _fsInfo,
        const int32_t _durationSec,
        int64_t _id,
        int64_t _prevId);

    virtual ~PttBlock() override;

    void clearSelection() override;

    QSize getCtrlButtonSize() const;

    QString getSelectedText(const bool _isFullSelect = false, const TextDestination _dest = TextDestination::selection) const override;

    bool updateFriendly(const QString& _aimId, const QString& _friendly) override;

    QSize getTextButtonSize() const;

    bool hasDecodedText() const;

    bool isDecodedTextCollapsed() const;

    void selectByPos(const QPoint& from, const QPoint& to, bool) override;

    void selectAll() override;

    void setCtrlButtonGeometry(const QRect &rect);

    int32_t setDecodedTextWidth(const int32_t _width);

    void setDecodedTextOffsets(int _x, int _y);

    void setTextButtonGeometry(const QPoint& _pos);

    int desiredWidth(int _width = 0) const override;

    bool isNeedCheckTimeShift() const override;

    bool isSelected() const override;

    bool clicked(const QPoint& _p) override;
    bool pressed(const QPoint& _p) override;

    void doubleClicked(const QPoint& _p, std::function<void(bool)> _callback = std::function<void(bool)>()) override;

    void releaseSelection() override;

    void onVisibleRectChanged(const QRect& _visibleRect) override;

    IItemBlock::MenuFlags getMenuFlags(QPoint p) const override;

    bool setProgress(const QString& _fsId, const int32_t _value) override;

    //void setEmojiSizeType(const TextRendering::EmojiSizeType& _emojiSizeType) {};

protected:
    void drawBlock(QPainter &_p, const QRect& _rect, const QColor& _quoteColor) override;

    void initializeFileSharingBlock() override;

    void onDataTransferStarted() override;

    void onDataTransferStopped() override;

    void onDownloaded() override;

    void onDownloadedAction() override;

    void onDataTransfer(const int64_t _bytesTransferred, const int64_t _bytesTotal) override;

    void onDownloadingFailed(const int64_t requestId) override;

    void onLocalCopyInfoReady(const bool isCopyExists) override;

    void onMetainfoDownloaded() override;

    void onPreviewMetainfoDownloaded(const QString &_miniPreviewUri, const QString &_fullPreviewUri) override;

    void onMenuOpenFolder() final override;

    void mouseMoveEvent(QMouseEvent* _e) override;

    void mousePressEvent(QMouseEvent* _e) override;

    void mouseReleaseEvent(QMouseEvent* _e) override;

    void leaveEvent(QEvent* _e) override;

    void enterEvent(QEvent* _e) override;

private:
    Q_PROPERTY(int32_t PlaybackProgress READ getPlaybackProgress WRITE setPlaybackProgress);

    enum class PlaybackState
    {
        Min,

        Stopped,
        Playing,
        Paused,

        Max
    };

    void connectSignals();

    void drawBubble(QPainter &_p, const QRect &_bubbleRect);

    void drawDuration(QPainter &_p);

    void drawPlaybackProgress(QPainter &_p, const int32_t _progressMsec, const int32_t _durationMsec);

    int32_t getPlaybackProgress() const;

    void setPlaybackProgress(const int32_t _value);

    void initializeDecodedTextCtrl();

    bool isDecodedTextVisible() const;

    bool isPaused() const;

    bool isPlaying() const;

    bool isStopped() const;

    bool isTextRequested() const;

    void renderClipPaths(const QRect &_bubbleRect);

    void requestText();

    void startTextRequestProgressAnimation();

    void stopTextRequestProgressAnimation();

    void startPlayback();
    void pausePlayback();

    enum class AnimationState
    {
        Play,
        Pause
    };

    void initPlaybackAnimation(AnimationState _state);
    void updatePlaybackAnimation(float _progress);

    void updateButtonsStates();
    void updatePlayButtonState();
    void updateCollapseState();

    void updateStyle() override;
    void updateFonts() override;

    void updateDecodedTextStyle();
    QColor getDecodedTextColor() const;
    QColor getProgressColor() const;
    QColor getPlaybackColor() const;

    bool isOutgoing() const;

    bool canShowButtonText() const;
    bool checkButtonsHovered() const;

    QRect getBulletRect() const;
    QRect getPlaybackRect() const;
    void updatePlaybackProgress(const QPoint& _curPos);
    double getPlaybackPercentProgress(const QPoint& _curPos) const;

    void showBulletTooltip(QPoint _curPos) const;

    QPainterPath bubbleClipPath_;

    QRect bubbleClipRect_;

    QString decodedText_;

    std::unique_ptr<TextRendering::TextUnit> decodedTextCtrl_;

    int32_t durationMSec_;

    QString durationText_;

    bool isDecodedTextCollapsed_;

    bool isPlayed_;

    bool isPlaybackScheduled_;

    PlaybackState playbackState_;

    int32_t playbackProgressMsec_;

    QPropertyAnimation *playbackProgressAnimation_;

    int playingId_;

    PttBlockLayout *pttLayout_;

    int64_t textRequestId_;

    qint64 id_;

    qint64 prevId_;

    bool isOutgoing_;

    PttDetails::PlayButton* buttonPlay_;
    PttDetails::ButtonWithBackground* buttonText_;
    PttDetails::ButtonWithBackground* buttonCollapse_;
    PttDetails::ProgressWidget* progressPlay_;
    PttDetails::ProgressWidget* progressText_;

    QTimer* downloadAnimDelay_;
    QTimer* tripleClickTimer_ = nullptr;

    QPoint bulletPos_;
    bool bulletHovered_ = false;
    bool bulletPressed_ = false;
    bool continuePlaying_ = false;

private Q_SLOTS:

    void showDownloadAnimation();

    void onPlayButtonClicked();
    void onTextButtonClicked();

    void onPttFinished(int _id, bool _byPlay);

    void onPttPaused(int _id);

    void onPttText(qint64 _seq, int _error, QString _text, int _comeback);

    void pttPlayed(qint64);

    void updateBulletHoveredState();

};

UI_COMPLEX_MESSAGE_NS_END
