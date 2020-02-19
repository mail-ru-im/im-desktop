#include "stdafx.h"
#include "tools.h"
#include "../../common.shared/config/config.h"

#ifdef _WIN32
#include <windows.h>
#include <Shlobj.h>
#endif //_WIN32

namespace installer
{
    namespace logic
    {
        QString install_folder;
        QString product_folder;
        QString updates_folder;
        install_config config;

        QString get_appdata_folder()
        {
            QString folder;
#ifdef _WIN32
            wchar_t buffer[1024];

            if (::SHGetFolderPath( NULL, CSIDL_APPDATA, NULL, 0, buffer) == S_OK)
                folder = QString::fromUtf16(reinterpret_cast<const unsigned short*>(buffer));
#endif //_WIN32

            folder = folder.replace(ql1c('\\'), ql1c('/'));

            return folder;
        }

        QString get_product_folder()
        {
            if (product_folder.isEmpty())
            {
                const auto path = config::get().string(config::values::product_path);
                product_folder = get_appdata_folder() % ql1c('/') % QString::fromUtf8(path.data(), path.size());
            }

            return product_folder;
        }

        QString get_updates_folder()
        {
            if (updates_folder.isEmpty())
            {
                updates_folder = get_product_folder() % ql1c('/') % updates_folder_short;
            }

            return updates_folder;
        }

        QString get_install_folder()
        {
            if (install_folder.isEmpty())
            {
                install_folder = get_product_folder() % ql1s("/bin");
            }

            return install_folder;
        }

        QString get_installer_tmp_folder()
        {
            return get_product_folder() % ql1s("/install");
        }

        QString get_icq_exe()
        {
            return get_install_folder() % ql1c('/') % get_icq_exe_short();
        }

        QString get_icq_exe_short()
        {
            const auto product = config::get().string(config::values::installer_product_exe_win);
            return QString::fromUtf8(product.data(), product.size());
        }

        QString get_installer_exe_short()
        {
            const auto exe = config::get().string(config::values::installer_exe_win);
            return QString::fromUtf8(exe.data(), exe.size());
        }

        QString get_installer_exe()
        {
            return get_install_folder() % ql1c('/') % get_installer_exe_short();
        }

        QString get_exported_account_folder()
        {
            return get_product_folder() % ql1s("/0001/key");
        }

        QString get_exported_settings_folder()
        {
            return get_product_folder() % ql1s("/settings");
        }

        QString get_product_name()
        {
            const auto product = config::get().string(config::values::product_name_short);
            return QString::fromUtf8(product.data(), product.size()) % ql1s(".desktop");
        }

        QString get_product_display_name()
        {
            const auto product = config::get().string(config::values::product_name_full);
            return QString::fromUtf8(product.data(), product.size());
        }

        QString get_product_menu_folder()
        {
            const auto menu = config::get().string(config::values::installer_menu_folder_win);
            return QString::fromUtf8(menu.data(), menu.size());
        }

        QString get_company_name()
        {
            const auto company_name = config::get().string(config::values::company_name);
            return QString::fromUtf8(company_name.data(), company_name.size());
        }

        QString get_installed_product_path()
        {
            QString path;

#ifdef _WIN32

            CRegKey key;
            if (key.Open(HKEY_CURRENT_USER, (LPCTSTR)(QString(ql1s("Software\\") % get_product_name()).utf16()), KEY_READ) == ERROR_SUCCESS)
            {
                wchar_t registry_path[1025];
                DWORD needed = 1024;
                if (key.QueryStringValue(L"path", registry_path, &needed) == ERROR_SUCCESS)
                {
                    path = QString::fromUtf16((const ushort*) registry_path);
                }
            }

#endif //_WIN32
            return path;
        }

        const install_config& get_install_config()
        {
            return config;
        }

        void set_install_config(const install_config& _config)
        {
            config = _config;
        }

        translate::translator_base* get_translator()
        {
            static translate::translator_base translator;

            return &translator;
        }

        std::string_view get_crossprocess_mutex_name()
        {
            return config::get().string(config::values::main_instance_mutex_win);
        }

        QString get_crossprocess_pipe_name()
        {
            const auto name = config::get().string(config::values::crossprocess_pipe);
            return QString::fromUtf8(name.data(), name.size());
        }
    }
}

void installer::draw::animateButton(QWidget * _button, bool _state, int _minWidth, int _maxWidth)
{
    const auto startOpacity = _state ? 0.0 : 1.0;
    const auto endOpacity = _state ? 1.0 : 0.0;

    auto effect = new ButtonEffect(_button, _minWidth, _maxWidth);
    effect->setOpacity(startOpacity);
    _button->setGraphicsEffect(effect);
    _button->setVisible(true);

    auto opacityAnim = new QPropertyAnimation(effect, "opacity");
    opacityAnim->setDuration(std::chrono::milliseconds(animationDuration).count());
    opacityAnim->setEasingCurve(QEasingCurve::InOutQuad);
    opacityAnim->setStartValue(startOpacity);
    opacityAnim->setEndValue(endOpacity);
    QObject::connect(opacityAnim, &QPropertyAnimation::finished, _button, [_button, _state, effect]()
    {
        if (_button->graphicsEffect() == effect)
        {
            _button->setVisible(_state);
            _button->setGraphicsEffect(nullptr);
            _button->update();
        }
    });
    opacityAnim->start(QAbstractAnimation::DeleteWhenStopped);
}

void installer::draw::ButtonEffect::draw(QPainter * _painter)
{
    const auto r = boundingRect();
    const auto val = opacity();
    const auto range = maxWidth_ - minWidth_;
    const auto iconWidth = val * range + minWidth_;
    QRectF pmRect(0, 0, iconWidth, iconWidth);
    pmRect.moveCenter(r.center());

    _painter->setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
    _painter->setOpacity(val);

    const QPixmap pixmap = sourcePixmap(Qt::DeviceCoordinates);
    _painter->drawPixmap(pmRect, pixmap, pixmap.rect());
}
