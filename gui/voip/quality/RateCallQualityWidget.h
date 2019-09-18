#pragma once

#include <QObject>
#include "controls/TextUnit.h"

class QVBoxLayout;

namespace Ui
{

class HorizontalImgList;
class PictureWidget;
class TextBrowserEx;
class DialogButton;

class RateCallQualityWidget: public QWidget
{
    Q_OBJECT

public:
    RateCallQualityWidget(const QString& _wTitle, QWidget *_parent = nullptr);

    void setOkCancelButton(DialogButton* _okButton, DialogButton* _cancelButton);

Q_SIGNALS:
    void ratingConfirmed(int _starsCount);
    void ratingCancelled();

public Q_SLOTS:
    void onCallRated(int _starIndex);
    void onCallRateHovered(int _starIndex);
    void onConfirmRating(bool _doConfirm, int _starsCount);

protected:
    virtual void paintEvent(QPaintEvent* _ev) override;

private:
    void updateCatImageFor(int _starsCount);

private:
    HorizontalImgList* starsWidget_;
    PictureWidget* catWidget_;
    TextBrowserEx* title_;
    QVBoxLayout* globalLayout_;

    DialogButton* okDialogButton_;
    DialogButton* cancelDialogButton_;

    int currentStarsCount_ = -1;

    QSize catSize_;
};

}


