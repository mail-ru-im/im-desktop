#include "stdafx.h"

#include "FavoritesUtils.h"
#include "my_info.h"
#include "utils/utils.h"
#include "utils/translator.h"
#include "../GroupChatOperations.h"
#include "previewer/toast.h"
#include "gui_settings.h"
#include "styles/ThemeParameters.h"
#include "cache/emoji/EmojiCode.h"
#include "../containers/FriendlyContainer.h"
#include "utils/features.h"
#include "utils/InterConnector.h"
#include "controls/TextUnit.h"
#include "main_window/MainWindow.h"
#include "main_window/ContactDialog.h"
#include "main_window/contact_list/ContactListModel.h"
#include "fonts.h"
#include "../common.shared/config/config.h"
#include "previewer/Drawable.h"

namespace
{
    QString firstMessageImageLink()
    {
        const auto id = Utils::GetTranslator()->getLang() == u"ru" ? Features::favoritesImageIdRussian() : Features::favoritesImageIdEnglish();
        return Utils::replaceFilesPlaceholders(ql1s("[Photo: %1]").arg(id), {});
    }

    QString translateWithDefault(QTranslator& _translator, const char* _context, const char* _sourceText)
    {
        auto result = _translator.translate(_context, _sourceText);

        if (result.isNull())
            result = QString::fromUtf8(_sourceText);

        return result;
    }
}


namespace Favorites
{
    QString firstMessageText()
    {
        if (config::get().is_on(config::features::favorites_message_onpremise))
        {
            const auto friendly = Logic::GetFriendlyContainer()->getFriendly(aimId());
            const auto paperClipEmojiStr = Emoji::EmojiCode::toQString(Emoji::EmojiCode(0x1f4ce));
            const auto heavyCheckMarkEmojiStr = Emoji::EmojiCode::toQString(Emoji::EmojiCode(0x2714, 0xfe0f));
            const auto memoEmojiStr = Emoji::EmojiCode::toQString(Emoji::EmojiCode(0x1f4dd));

            return QT_TRANSLATE_NOOP("favorites", "%1, this is your work space!\n\n"
                                                  "%2 Save links, files and other messages\n"
                                                  "%3 Write notes, to-do lists, store photos and videos\n"
                                                  "%4 Draft messages and polls before sending them to chat with colleagues or to your channel").arg(friendly,
                                                                                                                                                    paperClipEmojiStr,
                                                                                                                                                    heavyCheckMarkEmojiStr,
                                                                                                                                                    memoEmojiStr);
        }
        else
        {
            const auto friendly = Logic::GetFriendlyContainer()->getFriendly(aimId());
            const auto starEmojiStr = Emoji::EmojiCode::toQString(Emoji::EmojiCode(0x2b50));
            const auto checkEmojiStr = Emoji::EmojiCode::toQString(Emoji::EmojiCode(0x2705));
            const auto nationalParkEmojiStr = Emoji::EmojiCode::toQString(Emoji::EmojiCode(0x1f3de, 0xfe0f));

            return firstMessageImageLink()
                    + QT_TRANSLATE_NOOP("favorites", "\n%1, this is your personal space!\n\n"
                                                     "%2 Save links, files and other messages\n"
                                                     "%3 Write notes, to-do lists, store photos and videos\n"
                                                     "%4 Draft messages and polls before sending them to chat with friends or to your channel").arg(friendly,
                                                                                                                                                    starEmojiStr,
                                                                                                                                                    checkEmojiStr,
                                                                                                                                                    nationalParkEmojiStr);
        }

    }

    QPixmap avatar(int32_t _size)
    {
        static const Utils::SvgLayers layers =
        {
            { qsl("circle"), Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY_LIGHT) },
            { qsl("star"),  Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID_PERMANENT)},
        };

        return Utils::renderSvgLayered(qsl(":/contact_list/favorites_avatar"), layers, Utils::unscale_bitmap(QSize(_size, _size)));
    }

    QColor nameColor()
    {
        return Styling::getParameters().getColor(Styling::StyleVariable::TEXT_PRIMARY);
    }

    bool isFavorites(const QString& _aimId)
    {
        return _aimId == aimId();
    }

    const QString& aimId()
    {
        return Ui::MyInfo()->aimId();
    }

    const std::vector<QString>& matchingWords()
    {
        static auto words = []()
        {
            std::vector<QString> list;

            QTranslator translator;
            for (auto& lang : Utils::GetTranslator()->getLanguages())
            {
                translator.load(lang, qsl(":/translations"));
                list.push_back(translateWithDefault(translator, "favorites", "favorites"));
                list.push_back(translateWithDefault(translator, "favorites", "save"));
                list.push_back(translateWithDefault(translator, "favorites", "saved"));
            }
            return list;
        }();

        return words;
    }

    QString name()
    {
        return QT_TRANSLATE_NOOP("favorites", "Favorites");
    }

    void addToFavorites(const Data::QuotesVec& _quotes)
    {
        sendForwardedMessages(_quotes, { Favorites::aimId() }, Ui::ForwardWithAuthor::Yes, Ui::ForwardSeparately::Yes);
    }

    QString addedToFavoritesToastText(int32_t _messagesCount)
    {
        return Utils::GetTranslator()->getNumberString(_messagesCount,
                                                       QT_TRANSLATE_NOOP3("chat_page", "Message added to ", "1"),
                                                       QT_TRANSLATE_NOOP3("chat_page", "Messages added to ", "2"),
                                                       QT_TRANSLATE_NOOP3("chat_page", "Messages added to ", "5"),
                                                       QT_TRANSLATE_NOOP3("chat_page", "Messages added to ", "21"));
    }

    bool pinnedOnStart()
    {
        return Ui::get_gui_settings()->get_value<bool>(favorites_pinned_on_start, false, Ui::MyInfo()->aimId());
    }

    void setPinnedOnStart(bool _pinned)
    {
        return Ui::get_gui_settings()->set_value<bool>(favorites_pinned_on_start, _pinned, Ui::MyInfo()->aimId());
    }

    void showSavedToast(int _count)
    {
        if (auto mainWindow = Utils::InterConnector::instance().getMainWindow())
            Utils::showToastOverContactDialog(new FavoritesToast(_count, mainWindow));
    }

    class FavoritesToast_p
    {
    public:
        Ui::TextRendering::TextUnitPtr message_;
        std::unique_ptr<Ui::Label> favoritesLabel_;
    };

    FavoritesToast::FavoritesToast(int _messageCount, QWidget* _parent)
        : Ui::ToastBase(_parent),
        d(std::make_unique<FavoritesToast_p>())
    {
        static const auto xOffset = Utils::scale_value(12);
        static const auto toastHeight = Utils::scale_value(40);
        const auto msgFont = Fonts::appFontScaled(15, Fonts::FontWeight::Normal);

        d->message_ = Ui::TextRendering::MakeTextUnit(addedToFavoritesToastText(_messageCount), {}, Ui::TextRendering::LinksVisible::DONT_SHOW_LINKS);
        d->message_->init(msgFont, Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID_PERMANENT), QColor(), QColor(), QColor(), Ui::TextRendering::HorAligment::CENTER);

        auto favoritesUnit = Ui::TextRendering::MakeTextUnit(name(), {}, Ui::TextRendering::LinksVisible::DONT_SHOW_LINKS);
        favoritesUnit->init(msgFont, nameColor(), QColor(), QColor(), QColor(), Ui::TextRendering::HorAligment::CENTER);

        const auto messageWidth = d->message_->desiredWidth();
        const auto favoritesWidth = favoritesUnit->desiredWidth();

        const auto messageHeight = d->message_->getHeight(messageWidth);

        d->favoritesLabel_ = std::make_unique<Ui::Label>();
        d->favoritesLabel_->setTextUnit(std::move(favoritesUnit));
        d->favoritesLabel_->setYOffset((toastHeight - messageHeight) / 2);
        d->favoritesLabel_->setRect(QRect(xOffset + messageWidth, 0, favoritesWidth, toastHeight));
        d->favoritesLabel_->setDefaultColor(nameColor());
        d->favoritesLabel_->setHoveredColor(Styling::getParameters().getColor(Styling::StyleVariable::TEXT_PRIMARY_HOVER));
        d->favoritesLabel_->setUnderline(true);

        d->message_->setOffsets(xOffset, (toastHeight - messageHeight) / 2);

        setFixedSize(2 * xOffset + messageWidth + favoritesWidth, toastHeight);
    }

    void FavoritesToast::drawContent(QPainter& _p)
    {
        d->message_->draw(_p);
        d->favoritesLabel_->draw(_p);
    }

    void FavoritesToast::mouseMoveEvent(QMouseEvent* _event)
    {
        const auto overFavorites = d->favoritesLabel_->rect().contains( _event->pos());
        setCursor(overFavorites ? Qt::PointingHandCursor : Qt::ArrowCursor);

        d->favoritesLabel_->setUnderline(!overFavorites);
        d->favoritesLabel_->setHovered(overFavorites);

        update();

        ToastBase::mouseMoveEvent(_event);
    }

    void FavoritesToast::mousePressEvent(QMouseEvent* _event)
    {
        const auto overFavorites = d->favoritesLabel_->rect().contains( _event->pos());
        if (overFavorites)
        {
            d->favoritesLabel_->setPressed(true);
            update();
        }
        ToastBase::mousePressEvent(_event);
    }

    void FavoritesToast::mouseReleaseEvent(QMouseEvent* _event)
    {
        auto click = false;
        if (d->favoritesLabel_->rect().contains(_event->pos()) && d->favoritesLabel_->pressed())
            click = true;

        d->favoritesLabel_->setPressed(false);

        if (click)
        {
            if (const auto& selected = Logic::getContactListModel()->selectedContact(); selected != aimId())
                Q_EMIT Utils::InterConnector::instance().addPageToDialogHistory(selected);
            Utils::InterConnector::instance().openDialog(aimId());
            Utils::InterConnector::instance().getMainWindow()->closeGallery();
        }
        update();

        ToastBase::mouseReleaseEvent(_event);
    }

    void FavoritesToast::leaveEvent(QEvent* _event)
    {
        d->favoritesLabel_->setUnderline(true);
        d->favoritesLabel_->setHovered(false);
        update();
        ToastBase::leaveEvent(_event);
    }

}
