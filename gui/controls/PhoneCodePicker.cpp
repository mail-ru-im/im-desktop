#include "stdafx.h"
#include "PhoneCodePicker.h"

#include "main_window/contact_list/CountryListModel.h"

#include "main_window/contact_list/SearchWidget.h"
#include "main_window/contact_list/ContactListWidget.h"
#include "main_window/contact_list/SelectionContactsForGroupChat.h"

#include "main_window/MainWindow.h"
#include "controls/DialogButton.h"
#include "controls/LabelEx.h"
#include "utils/InterConnector.h"
#include "utils/utils.h"
#include "../cache/emoji/EmojiDb.h"
#include "../utils/macos/mac_support.h"

using namespace Ui;

constexpr double heightPartOfMainWindowForFullView = 0.6;
constexpr int DIALOG_WIDTH = 360;
constexpr int search_padding_ver = 8;
constexpr int search_padding_hor = 4;
constexpr int itemHeight = 44;
constexpr int iconSize = 32;

namespace
{
    QString getDefaultCountry()
    {
#ifdef __APPLE__
        QString counryCode = MacSupport::currentRegion().right(2).toLower();;
#else
        QString counryCode = QLocale::system().name().right(2).toLower();
#endif

        return Utils::getCountryNameByCode(counryCode);
    }

    constexpr Emoji::EmojiCode unknownCode()
    {
        return Emoji::EmojiCode(0x1F4DE);
    }
}

PhoneCodePicker::PhoneCodePicker(QWidget * _parent)
    : QWidget(_parent)
    , isKnownCode_(true)
{
    code_ = Logic::getCountryModel()->getCountry(getDefaultCountry()).phone_code_;

    setIcon(getDefaultCountry());

    setCursor(Qt::PointingHandCursor);
}

const QString& Ui::PhoneCodePicker::getCode() const
{
    return code_;
}

void PhoneCodePicker::setCode(const QString & _code)
{
    if (code_ != _code && !_code.isEmpty())
    {
        code_ = _code;
        const auto name = Logic::getCountryModel()->getCountryByCode(_code).name_;
        isKnownCode_ = !name.isEmpty();
        setIcon(name);
    }
}

bool Ui::PhoneCodePicker::isKnownCode(const QString& _code) const
{
    if (_code.isEmpty())
        return isKnownCode_;
    else
        return !Logic::getCountryModel()->getCountryByCode(_code).name_.isEmpty();
}

void PhoneCodePicker::paintEvent(QPaintEvent * _event)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    const auto dr = (width() - Utils::scale_value(iconSize)) / 2;
    p.drawImage(QRect(dr, dr - Utils::scale_value(1), Utils::scale_value(iconSize), Utils::scale_value(iconSize)), icon_);
    QWidget::paintEvent(_event);
}

void PhoneCodePicker::mouseReleaseEvent(QMouseEvent * _event)
{
    openCountryPicker();
    QWidget::mouseReleaseEvent(_event);
}

void PhoneCodePicker::setIcon(const QString & _country)
{
    const auto isoCode = Utils::getCountryCodeByName(_country);
    auto emojicode = Emoji::GetEmojiInfoByCountryName(isoCode);
    if (emojicode.isNull())
        emojicode = unknownCode();

    icon_ = Emoji::GetEmoji(emojicode, Emoji::getEmojiSize());
    update();
}

void PhoneCodePicker::openCountryPicker()
{
    auto countrySearch_ = new SelectContactsWidget(
        nullptr,
        Logic::MembersWidgetRegim::COUNTRY_LIST,
        QT_TRANSLATE_NOOP("popup_window", "Choose country"),
        QT_TRANSLATE_NOOP("popup_window", "Cancel"),
        parentWidget());

    const auto result = countrySearch_->show();
    countrySearch_->deleteLater();

    if (result)
    {
        const auto c = Logic::getCountryModel()->getSelected();
        emit countrySelected(c.phone_code_);
        setIcon(c.name_);
    }
}
