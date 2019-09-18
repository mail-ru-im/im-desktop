#include "stdafx.h"

#include "StyleVariable.h"

namespace Styling
{
    const StyleVariableStrings& getVariableStrings()
    {
        static const StyleVariableStrings properties = []()
        {
            StyleVariableStrings props =
            {
                { StyleVariable::BASE,                          qsl("base") },
                { StyleVariable::BASE_LIGHT,                    qsl("base_light") },
                { StyleVariable::BASE_BRIGHT,                   qsl("base_bright") },
                { StyleVariable::BASE_BRIGHT_ACTIVE,            qsl("base_bright_active") },
                { StyleVariable::BASE_BRIGHT_HOVER,             qsl("base_bright_hover") },
                { StyleVariable::BASE_BRIGHT_INVERSE,           qsl("base_bright_inverse") },
                { StyleVariable::BASE_BRIGHT_INVERSE_HOVER,     qsl("base_bright_inverse_hover") },
                { StyleVariable::BASE_GLOBALWHITE,              qsl("base_globalwhite") },
                { StyleVariable::BASE_PRIMARY,                  qsl("base_primary") },
                { StyleVariable::BASE_PRIMARY_HOVER,            qsl("base_primary_hover") },
                { StyleVariable::BASE_PRIMARY_ACTIVE,           qsl("base_primary_active") },
                { StyleVariable::BASE_SECONDARY,                qsl("base_secondary") },
                { StyleVariable::BASE_SECONDARY_ACTIVE,         qsl("base_secondary_active") },
                { StyleVariable::BASE_SECONDARY_HOVER,          qsl("base_secondary_hover") },
                { StyleVariable::BASE_TERTIARY,                 qsl("base_tertiary") },
                { StyleVariable::BASE_ULTRABRIGHT,              qsl("base_ultrabright") },
                { StyleVariable::BASE_ULTRABRIGHT_HOVER,        qsl("base_ultrabright_hover") },
                { StyleVariable::CHAT_PRIMARY,                  qsl("chat_primary") },
                { StyleVariable::CHAT_PRIMARY_HOVER,            qsl("chat_primary_hover") },
                { StyleVariable::CHAT_PRIMARY_ACTIVE,           qsl("chat_primary_active") },
                { StyleVariable::CHAT_SECONDARY,                qsl("chat_secondary") },
                { StyleVariable::CHAT_ENVIRONMENT,              qsl("chat_environment") },
                { StyleVariable::GHOST_SECONDARY,               qsl("ghost_secondary") },
                { StyleVariable::GHOST_SECONDARY_INVERSE,       qsl("ghost_secondary_inverse") },
                { StyleVariable::GHOST_SECONDARY_INVERSE_HOVER, qsl("ghost_secondary_inverse_hover") },
                { StyleVariable::GHOST_LIGHT,                   qsl("ghost_light") },
                { StyleVariable::GHOST_LIGHT_INVERSE,           qsl("ghost_light_inverse") },
                { StyleVariable::GHOST_PRIMARY,                 qsl("ghost_primary") },
                { StyleVariable::GHOST_PRIMARY_INVERSE,         qsl("ghost_primary_inverse") },
                { StyleVariable::GHOST_PRIMARY_INVERSE_HOVER,   qsl("ghost_primary_inverse_hover") },
                { StyleVariable::GHOST_PRIMARY_INVERSE_ACTIVE,  qsl("ghost_primary_inverse_active") },
                { StyleVariable::GHOST_TERTIARY,                qsl("ghost_tertiary") },
                { StyleVariable::GHOST_ULTRALIGHT,              qsl("ghost_ultralight") },
                { StyleVariable::GHOST_ULTRALIGHT_INVERSE,      qsl("ghost_ultralight_inverse") },
                { StyleVariable::GHOST_ACCENT,                  qsl("ghost_accent") },
                { StyleVariable::GHOST_ACCENT_HOVER,            qsl("ghost_accent_hover") },
                { StyleVariable::GHOST_ACCENT_ACTIVE,           qsl("ghost_accent_active") },
                { StyleVariable::GHOST_OVERLAY,                 qsl("ghost_overlay") },
                { StyleVariable::GHOST_QUATERNARY,              qsl("ghost_quaternary") },
                { StyleVariable::PRIMARY,                       qsl("primary") },
                { StyleVariable::PRIMARY_ACTIVE,                qsl("primary_active") },
                { StyleVariable::PRIMARY_BRIGHT,                qsl("primary_bright") },
                { StyleVariable::PRIMARY_BRIGHT_ACTIVE,         qsl("primary_bright_active") },
                { StyleVariable::PRIMARY_BRIGHT_HOVER,          qsl("primary_bright_hover") },
                { StyleVariable::PRIMARY_HOVER,                 qsl("primary_hover") },
                { StyleVariable::PRIMARY_INVERSE,               qsl("primary_inverse") },
                { StyleVariable::PRIMARY_PASTEL,                qsl("primary_pastel") },
                { StyleVariable::PRIMARY_SELECTED,              qsl("primary_selected") },
                { StyleVariable::TEXT_PRIMARY,                  qsl("text_primary") },
                { StyleVariable::TEXT_PRIMARY_HOVER,            qsl("text_primary_hover") },
                { StyleVariable::TEXT_PRIMARY_ACTIVE,           qsl("text_primary_active") },
                { StyleVariable::TEXT_SOLID,                    qsl("text_solid") },
                { StyleVariable::TEXT_SOLID_PERMANENT,          qsl("text_solid_permanent") },
                { StyleVariable::TEXT_SOLID_PERMANENT_HOVER,    qsl("text_solid_permanent_hover") },
                { StyleVariable::TEXT_SOLID_PERMANENT_ACTIVE,   qsl("text_solid_permanent_active") },
                { StyleVariable::SECONDARY_ATTENTION,           qsl("secondary_attention") },
                { StyleVariable::SECONDARY_ATTENTION_ACTIVE,    qsl("secondary_attention_active") },
                { StyleVariable::SECONDARY_ATTENTION_HOVER,     qsl("secondary_attention_hover") },
                { StyleVariable::SECONDARY_INFO,                qsl("secondary_info") },
                { StyleVariable::SECONDARY_RAINBOW_WARNING,     qsl("secondary_rainbow_warning") },
                { StyleVariable::SECONDARY_RAINBOW_MINT,        qsl("secondary_rainbow_mint") },
                { StyleVariable::SECONDARY_RAINBOW_COLD,        qsl("secondary_rainbow_cold") },
                { StyleVariable::SECONDARY_RAINBOW_ORANGE,      qsl("secondary_rainbow_orange") },
                { StyleVariable::SECONDARY_RAINBOW_AQUA,        qsl("secondary_rainbow_aqua") },
                { StyleVariable::SECONDARY_RAINBOW_PURPLE,      qsl("secondary_rainbow_purple") },
                { StyleVariable::SECONDARY_RAINBOW_PINK,        qsl("secondary_rainbow_pink") },
                { StyleVariable::SECONDARY_RAINBOW_BLUE,        qsl("secondary_rainbow_blue") },
                { StyleVariable::SECONDARY_RAINBOW_GREEN,       qsl("secondary_rainbow_green") },
                { StyleVariable::SECONDARY_RAINBOW_MAIL,        qsl("secondary_rainbow_mail") },
                { StyleVariable::APP_PRIMARY,                   qsl("app_primary") },

                { StyleVariable::TYPING_TEXT,                   qsl("typing_text") },
                { StyleVariable::TYPING_ANIMATION,              qsl("typing_animation") },
                { StyleVariable::CHATEVENT_TEXT,                qsl("chatevent_text") },
                { StyleVariable::CHATEVENT_BACKGROUND,          qsl("chatevent_background") },
                { StyleVariable::NEWPLATE_BACKGROUND,           qsl("newplate_background") },
                { StyleVariable::NEWPLATE_TEXT,                 qsl("newplate_text") },
            };

            if constexpr (build::is_debug())
            {
                QSet<QString> setPath;
                QSet<int> setEnum;
                for (const auto&[e, s] : props)
                {
                    if (!setEnum.contains((int)e))
                    {
                        setEnum.insert((int)e);
                    }
                    else
                    {
                        assert(!"repeating enum mappings in properties!");
                    }

                    if (!setPath.contains(s))
                    {
                        setPath.insert(s);
                    }
                    else
                    {
                        assert(!"repeating path mappings in properties!");
                    }
                }
            }

            return props;
        }();

        return properties;
    }

    QString variableToString(const StyleVariable _var)
    {
        const auto& props = getVariableStrings();

        QString str;

        const auto it = std::find_if(props.begin(), props.end(), [_var](const auto& _p) { return _p.first == _var; });
        assert(it != props.end());
        if (it != props.end())
            str = it->second;

        return str;
    }

    StyleVariable stringToVariable(const QString& _name)
    {
        const auto& props = getVariableStrings();

        const auto it = std::find_if(props.begin(), props.end(), [&_name](const auto& _p) { return _p.second == _name; });
        assert(it != props.end());
        if (it != props.end())
            return it->first;

        return StyleVariable::BASE; //TODO add invalid variable
    }
}
