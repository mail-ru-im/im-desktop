#include "stdafx.h"

#include "core_dispatcher.h"
#include "cache/emoji/Emoji.h"
#include "BigEmojiWidget.h"

namespace
{
    static constexpr auto maxEmojiSize = 102;
}

namespace Ui
{

//////////////////////////////////////////////////////////////////////////
// BigEmojiWidget_p
//////////////////////////////////////////////////////////////////////////

class BigEmojiWidget_p
{
public:
    QImage emojiImage_;
    int64_t seq_ = -1;
    Emoji::EmojiCode code_;
    bool waitingForNetwork_ = false;

    void loadServerEmoji()
    {
        seq_ = GetDispatcher()->getEmojiImage(code_.toHexString(), core_dispatcher::ServerEmojiSize::_160);
    }
};

//////////////////////////////////////////////////////////////////////////
// BigEmojiWidget
//////////////////////////////////////////////////////////////////////////

BigEmojiWidget::BigEmojiWidget(const QString& _code, int _size, QWidget* _parent)
    : QWidget(_parent),
      d(std::make_unique<BigEmojiWidget_p>())
{
    d->code_ = Emoji::EmojiCode::fromQString(_code);
    d->emojiImage_ = Emoji::GetEmoji(d->code_, Utils::scale_bitmap(_size));
    setFixedSize(_size, _size);

    if (_size > maxEmojiSize || d->emojiImage_.isNull())
        d->loadServerEmoji();

    connect(GetDispatcher(), &core_dispatcher::getEmojiResult, this, &BigEmojiWidget::onGetEmojiResult);
    connect(GetDispatcher(), &core_dispatcher::getEmojiFailed, this, &BigEmojiWidget::onGetEmojiFailed);
    connect(GetDispatcher(), &core_dispatcher::connectionStateChanged, this, &BigEmojiWidget::onConnectionState);
}

BigEmojiWidget::~BigEmojiWidget() = default;

void BigEmojiWidget::paintEvent(QPaintEvent* _event)
{
    QPainter p(this);
    p.drawImage(rect(), d->emojiImage_);
}

void BigEmojiWidget::onGetEmojiResult(int64_t _seq, const QImage& _emoji)
{
    if (_seq != d->seq_)
        return;

    d->emojiImage_ = _emoji.scaled(size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    update();
}

void BigEmojiWidget::onGetEmojiFailed(int64_t _seq, bool networkError)
{
    d->waitingForNetwork_ = (d->seq_ == _seq && networkError);
}

void BigEmojiWidget::onConnectionState(const ConnectionState& _state)
{
    if (_state == ConnectionState::stateOnline && std::exchange(d->waitingForNetwork_, false))
        d->loadServerEmoji();
}

}
