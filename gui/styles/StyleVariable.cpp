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
                { StyleVariable::BASE,                             u"base" },
                { StyleVariable::BASE_LIGHT,                       u"base_light" },
                { StyleVariable::BASE_BRIGHT,                      u"base_bright" },
                { StyleVariable::BASE_BRIGHT_ACTIVE,               u"base_bright_active" },
                { StyleVariable::BASE_BRIGHT_HOVER,                u"base_bright_hover" },
                { StyleVariable::BASE_BRIGHT_INVERSE,              u"base_bright_inverse" },
                { StyleVariable::BASE_BRIGHT_INVERSE_HOVER,        u"base_bright_inverse_hover" },
                { StyleVariable::BASE_GLOBALWHITE,                 u"base_globalwhite" },
                { StyleVariable::BASE_GLOBALWHITE_SECONDARY,       u"base_globalwhite_secondary" },
                { StyleVariable::BASE_GLOBALWHITE_PERMANENT,       u"base_globalwhite_permanent" },
                { StyleVariable::BASE_GLOBALWHITE_PERMANENT_HOVER, u"base_globalwhite_permanent_hover" },
                { StyleVariable::BASE_GLOBALBLACK_PERMANENT,       u"base_globalblack_permanent" },
                { StyleVariable::BASE_PRIMARY,                     u"base_primary" },
                { StyleVariable::BASE_PRIMARY_HOVER,               u"base_primary_hover" },
                { StyleVariable::BASE_PRIMARY_ACTIVE,              u"base_primary_active" },
                { StyleVariable::BASE_SECONDARY,                   u"base_secondary" },
                { StyleVariable::BASE_SECONDARY_ACTIVE,            u"base_secondary_active" },
                { StyleVariable::BASE_SECONDARY_HOVER,             u"base_secondary_hover" },
                { StyleVariable::BASE_TERTIARY,                    u"base_tertiary" },
                { StyleVariable::BASE_ULTRABRIGHT,                 u"base_ultrabright" },
                { StyleVariable::BASE_ULTRABRIGHT_HOVER,           u"base_ultrabright_hover" },
                { StyleVariable::CHAT_ENVIRONMENT,                 u"chat_environment" },
                { StyleVariable::CHAT_PRIMARY,                     u"chat_primary" },
                { StyleVariable::CHAT_PRIMARY_HOVER,               u"chat_primary_hover" },
                { StyleVariable::CHAT_PRIMARY_ACTIVE,              u"chat_primary_active" },
                { StyleVariable::CHAT_PRIMARY_MEDIA,               u"chat_primary_media" },
                { StyleVariable::CHAT_SECONDARY,                   u"chat_secondary" },
                { StyleVariable::CHAT_SECONDARY_MEDIA,             u"chat_secondary_media" },
                { StyleVariable::CHAT_TERTIARY,                    u"chat_tertiary" },
                { StyleVariable::GHOST_SECONDARY,                  u"ghost_secondary" },
                { StyleVariable::GHOST_SECONDARY_INVERSE,          u"ghost_secondary_inverse" },
                { StyleVariable::GHOST_SECONDARY_INVERSE_HOVER,    u"ghost_secondary_inverse_hover" },
                { StyleVariable::GHOST_LIGHT,                      u"ghost_light" },
                { StyleVariable::GHOST_LIGHT_INVERSE,              u"ghost_light_inverse" },
                { StyleVariable::GHOST_PRIMARY,                    u"ghost_primary" },
                { StyleVariable::GHOST_PRIMARY_INVERSE,            u"ghost_primary_inverse" },
                { StyleVariable::GHOST_PRIMARY_INVERSE_HOVER,      u"ghost_primary_inverse_hover" },
                { StyleVariable::GHOST_PRIMARY_INVERSE_ACTIVE,     u"ghost_primary_inverse_active" },
                { StyleVariable::GHOST_TERTIARY,                   u"ghost_tertiary" },
                { StyleVariable::GHOST_ULTRALIGHT,                 u"ghost_ultralight" },
                { StyleVariable::GHOST_ULTRALIGHT_INVERSE,         u"ghost_ultralight_inverse" },
                { StyleVariable::GHOST_ACCENT,                     u"ghost_accent" },
                { StyleVariable::GHOST_ACCENT_HOVER,               u"ghost_accent_hover" },
                { StyleVariable::GHOST_ACCENT_ACTIVE,              u"ghost_accent_active" },
                { StyleVariable::GHOST_OVERLAY,                    u"ghost_overlay" },
                { StyleVariable::GHOST_QUATERNARY,                 u"ghost_quaternary" },
                { StyleVariable::GHOST_PERCEPTIBLE,                u"ghost_perceptible" },
                { StyleVariable::LUCENT_TERTIARY,                  u"lucent_tertiary" },
                { StyleVariable::LUCENT_TERTIARY_HOVER,            u"lucent_tertiary_hover" },
                { StyleVariable::LUCENT_TERTIARY_ACTIVE,           u"lucent_tertiary_active" },
                { StyleVariable::PRIMARY,                          u"primary" },
                { StyleVariable::PRIMARY_ACTIVE,                   u"primary_active" },
                { StyleVariable::PRIMARY_BRIGHT,                   u"primary_bright" },
                { StyleVariable::PRIMARY_BRIGHT_ACTIVE,            u"primary_bright_active" },
                { StyleVariable::PRIMARY_BRIGHT_HOVER,             u"primary_bright_hover" },
                { StyleVariable::PRIMARY_HOVER,                    u"primary_hover" },
                { StyleVariable::PRIMARY_INVERSE,                  u"primary_inverse" },
                { StyleVariable::PRIMARY_PASTEL,                   u"primary_pastel" },
                { StyleVariable::PRIMARY_SELECTED,                 u"primary_selected" },
                { StyleVariable::PRIMARY_LIGHT,                    u"primary_light" },
                { StyleVariable::TEXT_PRIMARY,                     u"text_primary" },
                { StyleVariable::TEXT_PRIMARY_HOVER,               u"text_primary_hover" },
                { StyleVariable::TEXT_PRIMARY_ACTIVE,              u"text_primary_active" },
                { StyleVariable::TEXT_SOLID,                       u"text_solid" },
                { StyleVariable::TEXT_SOLID_PERMANENT,             u"text_solid_permanent" },
                { StyleVariable::TEXT_SOLID_PERMANENT_HOVER,       u"text_solid_permanent_hover" },
                { StyleVariable::TEXT_SOLID_PERMANENT_ACTIVE,      u"text_solid_permanent_active" },
                { StyleVariable::SECONDARY_ATTENTION,              u"secondary_attention" },
                { StyleVariable::SECONDARY_ATTENTION_ACTIVE,       u"secondary_attention_active" },
                { StyleVariable::SECONDARY_ATTENTION_HOVER,        u"secondary_attention_hover" },
                { StyleVariable::SECONDARY_INFO,                   u"secondary_info" },
                { StyleVariable::SECONDARY_RAINBOW_WARNING,        u"secondary_rainbow_warning" },
                { StyleVariable::SECONDARY_RAINBOW_MINT,           u"secondary_rainbow_mint" },
                { StyleVariable::SECONDARY_RAINBOW_COLD,           u"secondary_rainbow_cold" },
                { StyleVariable::SECONDARY_RAINBOW_WARM,           u"secondary_rainbow_warm" },
                { StyleVariable::SECONDARY_RAINBOW_ORANGE,         u"secondary_rainbow_orange" },
                { StyleVariable::SECONDARY_RAINBOW_AQUA,           u"secondary_rainbow_aqua" },
                { StyleVariable::SECONDARY_RAINBOW_PURPLE,         u"secondary_rainbow_purple" },
                { StyleVariable::SECONDARY_RAINBOW_PINK,           u"secondary_rainbow_pink" },
                { StyleVariable::SECONDARY_RAINBOW_BLUE,           u"secondary_rainbow_blue" },
                { StyleVariable::SECONDARY_RAINBOW_GREEN,          u"secondary_rainbow_green" },
                { StyleVariable::SECONDARY_RAINBOW_MAIL,           u"secondary_rainbow_mail" },
                { StyleVariable::SECONDARY_RAINBOW_YELLOW,         u"secondary_rainbow_yellow" },
                { StyleVariable::SECONDARY_RAINBOW_EMERALD,        u"secondary_rainbow_emerald" },
                { StyleVariable::SECONDARY_RAINBOW_EMERALD_HOVER,  u"secondary_rainbow_emerald_hover" },
                { StyleVariable::SECONDARY_RAINBOW_EMERALD_ACTIVE, u"secondary_rainbow_emerald_active" },
                { StyleVariable::APP_PRIMARY,                      u"app_primary" },

                { StyleVariable::TYPING_TEXT,                      u"typing_text" },
                { StyleVariable::TYPING_ANIMATION,                 u"typing_animation" },
                { StyleVariable::CHATEVENT_TEXT,                   u"chatevent_text" },
                { StyleVariable::CHATEVENT_BACKGROUND,             u"chatevent_background" },
                { StyleVariable::NEWPLATE_BACKGROUND,              u"newplate_background" },
                { StyleVariable::NEWPLATE_TEXT,                    u"newplate_text" },

                { StyleVariable::TASK_CHEAPS_CREATE,               u"task_cheaps_create"},
                { StyleVariable::TASK_CHEAPS_CREATE_HOVER,         u"task_cheaps_create_hover"},
                { StyleVariable::TASK_CHEAPS_CREATE_ACTIVE,        u"task_cheaps_create_active"},
                { StyleVariable::TASK_CHEAPS_IN_PROGRESS,          u"task_cheaps_in_progress"},
                { StyleVariable::TASK_CHEAPS_IN_PROGRESS_HOVER,    u"task_cheaps_in_progress_hover"},
                { StyleVariable::TASK_CHEAPS_IN_PROGRESS_ACTIVE,   u"task_cheaps_in_progress_active"},
                { StyleVariable::TASK_CHEAPS_DENIED,               u"task_cheaps_denied"},
                { StyleVariable::TASK_CHEAPS_DENIED_HOVER,         u"task_cheaps_denied_hover"},
                { StyleVariable::TASK_CHEAPS_DENIED_ACTIVE,        u"task_cheaps_denied_active"},
                { StyleVariable::TASK_CHEAPS_READY,                u"task_cheaps_ready"},
                { StyleVariable::TASK_CHEAPS_READY_HOVER,          u"task_cheaps_ready_hover"},
                { StyleVariable::TASK_CHEAPS_READY_ACTIVE,         u"task_cheaps_ready_active"},
                { StyleVariable::TASK_CHEAPS_CLOSED,               u"task_cheaps_closed"},
                { StyleVariable::TASK_CHEAPS_CLOSED_HOVER,         u"task_cheaps_closed_hover"},
                { StyleVariable::TASK_CHEAPS_CLOSED_ACTIVE,        u"task_cheaps_closed_active"}
            };

            if constexpr (build::is_debug())
            {
                QSet<QStringView> setPath;
                QSet<int> setEnum;
                for (const auto&[e, s] : props)
                {
                    if (!setEnum.contains((int)e))
                        setEnum.insert((int)e);
                    else
                        im_assert(!"repeating enum mappings in properties!");

                    if (!setPath.contains(s))
                        setPath.insert(s);
                    else
                        im_assert(!"repeating path mappings in properties!");
                }
            }

            return props;
        }();

        return properties;
    }

    QStringView variableToString(StyleVariable _var)
    {
        const auto& props = getVariableStrings();

        const auto it = std::find_if(props.begin(), props.end(), [_var](auto _p) { return _p.first == _var; });
        im_assert(it != props.end());
        if (it != props.end())
            return it->second;

        return {};
    }

    StyleVariable stringToVariable(QStringView _name)
    {
        const auto& props = getVariableStrings();

        const auto it = std::find_if(props.begin(), props.end(), [_name](auto _p) { return _p.second == _name; });
        im_assert(it != props.end());
        if (it != props.end())
            return it->first;

        return StyleVariable::BASE; //TODO add invalid variable
    }
}
