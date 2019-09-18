#include "TextBlock.h"

UI_COMPLEX_MESSAGE_NS_BEGIN

class DebugTextBlock : public TextBlock
{
public:
    enum class Subtype
    {
        MessageId = 1
    };

    const qint64 INVALID_MESSAGE_ID = -1;

    explicit DebugTextBlock(ComplexMessageItem* _parent, qint64 _id, Subtype _subtype = Subtype::MessageId);
    virtual ~DebugTextBlock();

    Subtype getSubtype() const;
    qint64 getMessageId() const;

    QString getSourceText() const override;
    Data::Quote getQuote() const override;
    QString getSelectedText(const bool _isFullSelect = false, const TextDestination _dest = TextDestination::selection) const override;

    bool updateFriendly(const QString& _aimId, const QString& _friendly) override;
    bool isAllSelected() const override { return true; }

protected:
    virtual void drawBlock(QPainter &_p, const QRect& _rect, const QColor& _quoteColor) override;
    virtual ContentType getContentType() const override;

private:
    void setMessageId(qint64 _id);

private:
    Subtype subtype_;
    qint64 id_ = INVALID_MESSAGE_ID;
};

UI_COMPLEX_MESSAGE_NS_END