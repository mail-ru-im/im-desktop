#include "stdafx.h"

#include "../cache/countries.h"
#include "../gui_settings.h"
#include "../utils/gui_coll_helper.h"
#include "../core_dispatcher.h"

#ifdef __APPLE__
#include "macos/mac_support.h"
#endif

namespace
{
    inline QString getPlusString()
    {
        return qsl("+");
    }
}

namespace Utils
{
    void Translator::init()
    {
        const auto lang = getLang();
        Ui::get_gui_settings()->set_value<QString>(settings_language, lang);

        installTranslator(lang);
        updateLocale();
    }

    void Translator::updateLocale()
    {
        QLocale locale(getLang());
        QString localeStr = locale.name();
        localeStr.replace(ql1c('_'), ql1c('-'));

        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("locale", std::move(localeStr).toLower());
        Ui::GetDispatcher()->post_message_to_core("set_locale", collection.get());
    }


    QString Translator::getCurrentPhoneCode() const
    {
#ifdef __APPLE__
        QString searchedCounry = MacSupport::currentRegion().right(2).toLower();
#else
        QString searchedCounry = QLocale::system().name().right(2).toLower();
#endif

        const auto& cntrs = Ui::countries::get();
        const auto it = find_if(
            cntrs.begin(),
            cntrs.end(),
            [&searchedCounry](const auto& _country) { return _country.iso_code_ == searchedCounry; }
        );

        if (it != cntrs.end())
            return getPlusString() % QString::number(it->phone_code_);

        return getPlusString();
    }

    QString Translator::getLang() const
    {
        return Ui::get_gui_settings()->get_value<QString>(settings_language, translate::translator_base::getLang());
    }

    Translator* GetTranslator()
    {
        static auto translator = std::make_unique<Translator>();
        return translator.get();
    }
}
