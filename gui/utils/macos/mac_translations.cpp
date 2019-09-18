//
//  mac_translations.cpp
//  ICQ
//
//  Created by g.ulyanov on 14/06/16.
//  Copyright © 2016 Mail.RU. All rights reserved.
//

#include "stdafx.h"
#include "../../utils/utils.h"

#include "mac_translations.h"

namespace Utils
{
    const QString &Translations::Get(const QString &key)
    {
        static Translations translations_;
        if (translations_.strings_.find(key) != translations_.strings_.end())
        {
            return translations_.strings_[key];
        }
        assert(!">> TRANSLATIONS: unknown key");
        qWarning() << "TRANSLATIONS: unknown key" << key;
        return key;
    }

    Translations::Translations()
    {
        // Main Menu
        strings_.insert(std::make_pair(qsl("&Edit"), QT_TRANSLATE_NOOP("macos_menu", "&Edit")));
        strings_.insert(std::make_pair(qsl("Undo"), QT_TRANSLATE_NOOP("macos_menu", "Undo")));
        strings_.insert(std::make_pair(qsl("Redo"), QT_TRANSLATE_NOOP("macos_menu", "Redo")));
        strings_.insert(std::make_pair(qsl("Cut"), QT_TRANSLATE_NOOP("macos_menu", "Cut")));
        strings_.insert(std::make_pair(qsl("Copy"), QT_TRANSLATE_NOOP("macos_menu", "Copy")));
        strings_.insert(std::make_pair(qsl("Paste"), QT_TRANSLATE_NOOP("macos_menu", "Paste")));
        strings_.insert(std::make_pair(qsl("Paste as Quote"), QT_TRANSLATE_NOOP("macos_menu", "Paste as Quote")));
        strings_.insert(std::make_pair(qsl("Emoji && Symbols"), QT_TRANSLATE_NOOP("macos_menu", "Emoji && Symbols")));
        strings_.insert(std::make_pair(qsl("Contact"), QT_TRANSLATE_NOOP("macos_menu", "Contact")));
        strings_.insert(std::make_pair(qsl("Add Buddy"), QT_TRANSLATE_NOOP("macos_menu", "Add Buddy")));
        strings_.insert(std::make_pair(qsl("Profile"), QT_TRANSLATE_NOOP("macos_menu", "Profile")));
        strings_.insert(std::make_pair(qsl("Close"), QT_TRANSLATE_NOOP("macos_menu", "Close")));
        strings_.insert(std::make_pair(qsl("View"), QT_TRANSLATE_NOOP("macos_menu", "View")));
        strings_.insert(std::make_pair(qsl("Next Unread Message"), QT_TRANSLATE_NOOP("macos_menu", "Next Unread Message")));
        strings_.insert(std::make_pair(qsl("Enter Full Screen"), QT_TRANSLATE_NOOP("macos_menu", "Enter Full Screen")));
        strings_.insert(std::make_pair(qsl("Exit Full Screen"), QT_TRANSLATE_NOOP("macos_menu", "Exit Full Screen")));
        strings_.insert(std::make_pair(qsl("Window"), QT_TRANSLATE_NOOP("macos_menu", "Window")));
        strings_.insert(std::make_pair(qsl("Select Next Chat"), QT_TRANSLATE_NOOP("macos_menu", "Select Next Chat")));
        strings_.insert(std::make_pair(qsl("Select Previous Chat"), QT_TRANSLATE_NOOP("macos_menu", "Select Previous Chat")));
        strings_.insert(std::make_pair(qsl("Minimize"), QT_TRANSLATE_NOOP("macos_menu", "Minimize")));
        strings_.insert(std::make_pair(qsl("Main Window"), QT_TRANSLATE_NOOP("macos_menu", "Main Window")));
        strings_.insert(std::make_pair(qsl("Check For Updates..."), QT_TRANSLATE_NOOP("macos_menu", "Check For Updates...")));
        strings_.insert(std::make_pair(qsl("About..."), QT_TRANSLATE_NOOP("macos_menu", "About...")));
        strings_.insert(std::make_pair(qsl("Settings"), QT_TRANSLATE_NOOP("macos_menu", "Settings")));
        strings_.insert(std::make_pair(qsl("Zoom"), QT_TRANSLATE_NOOP("macos_menu", "Zoom")));
        strings_.insert(std::make_pair(qsl("Bring All To Front"), QT_TRANSLATE_NOOP("macos_menu", "Bring All To Front")));
        strings_.insert(std::make_pair(qsl("ICQ VOIP"), QT_TRANSLATE_NOOP("macos_menu", "Current call")));

        //strings_.insert(std::make_pair("", QT_TRANSLATE_NOOP("macos_menu", "")));

    }
}
