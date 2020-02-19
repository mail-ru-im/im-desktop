#pragma once

#include "GenericBlock.h"
#include "types/poll.h"

UI_COMPLEX_MESSAGE_NS_BEGIN

class PollBlockLayout;

class PollBlock_p;

//////////////////////////////////////////////////////////////////////////
// PollBlock
//////////////////////////////////////////////////////////////////////////

class PollBlock : public GenericBlock
{
    Q_OBJECT

public:
    PollBlock(ComplexMessageItem* _parent, const Data::PollData& _poll, const QString& _text = QString());
    ~PollBlock() override;

    IItemBlockLayout* getBlockLayout() const override;

    bool isAllSelected() const override { return isSelected(); }
    QString getSelectedText(const bool _isFullSelect = false, const TextDestination _dest = TextDestination::selection) const override;

    bool updateFriendly(const QString& _aimId, const QString& _friendly) override { return true; }

    void updateStyle() override;
    void updateFonts() override;

    QSize setBlockSize(const QSize& _size);

    int desiredWidth(int _width = 0) const override;

    int getMaxWidth() const override;

    QString formatRecentsText() const override;

    bool clicked(const QPoint& _p) override;

    void updateWith(IItemBlock* _other) override;

    QString getSourceText() const override;

    bool onMenuItemTriggered(const QVariantMap& _params) override;
    MenuFlags getMenuFlags() const override;

    MediaType getMediaType() const override;

    ContentType getContentType() const override;

    bool isSharingEnabled() const override;

protected:
    void drawBlock(QPainter &_p, const QRect& _rect, const QColor& _quoteColor) override;
    void initialize() override;
    void mouseMoveEvent(QMouseEvent* _event) override;
    void leaveEvent(QEvent* _event) override;
    void mousePressEvent(QMouseEvent* _event) override;
    void mouseReleaseEvent(QMouseEvent* _event) override;

private Q_SLOTS:
    void onPollLoaded(const Data::PollData& _poll);
    void onVoteResult(int64_t, const Data::PollData& _poll, bool _success);
    void onRevokeResult(int64_t _seq, const QString& _pollId, bool _success);
    void onStopPollResult(int64_t _seq, const QString& _pollId, bool _success);

    void loadPoll();

private:
    std::unique_ptr<PollBlock_p> d;
};

UI_COMPLEX_MESSAGE_NS_END
