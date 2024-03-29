#pragma once

class QuoteColorAnimation : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QColor quoteColor READ quoteColor WRITE setQuoteColor);

public:
    QuoteColorAnimation(QWidget* parent);

    QColor quoteColor() const;
    void setQuoteColor(QColor color);

    void setSemiTransparent();
    void startQuoteAnimation();

    bool isPlay() const;
    void deactivate();

    void setAimId(const QString& _aimId);

protected:
    QColor getStartColor() const;

    QString aimId_;
    QColor QuoteColor_;
    int Alpha_;
    bool IsActive_;
    bool bPlay_;
};
