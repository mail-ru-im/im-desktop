#include "stdafx.h"

#include "bundle_installer.h"
#include "../utils/7z/7z.h"

namespace
{
    const QString switcherName = qsl("switcher.exe");
}

namespace installer
{
    namespace logic
    {
        BundleTask::BundleTask(const bool _installHP, const bool _installSearch)
            : QRunnable()
            , installHP_(_installHP)
            , installSearch_(_installSearch)
        {
        }

        void BundleTask::run()
        {
            const auto files = unpackBundle();
            if (files.empty())
                return;

            for (const auto& it : files)
            {
                if (it.first == switcherName)
                    runSwitcher(it.second);

                ::MoveFileEx((const wchar_t*)it.second.utf16(), NULL, MOVEFILE_DELAY_UNTIL_REBOOT | MOVEFILE_REPLACE_EXISTING);
            }
        }

        std::map<QString, QString> BundleTask::unpackBundle()
        {
            std::map<QString, QString> res;

            HINSTANCE instance = (HINSTANCE) ::GetModuleHandle(0);
            if (!instance)
                return res;

            HRSRC hrsrc = ::FindResourceW(instance, L"BUNDLE.7z", L"FILES");
            if (!hrsrc)
                return res;

            HGLOBAL hgres = ::LoadResource(instance, hrsrc);
            if (!hgres)
                return res;

            const char* data = (const char*)::LockResource(hgres);
            if (!data)
                return res;

            unsigned int sz = ::SizeofResource(instance, hrsrc);
            if (!sz)
                return res;

            zip_pack zip_file;
            if (!zip_file.open(data, sz))
                return res;

            for (auto iter : zip_file.get_files())
            {
                QString fileName = QFileInfo(iter.first).fileName();
                QString path = getUnpackPath(fileName);

                QFile file(path);
                if (file.open(QIODevice::WriteOnly | QIODevice::Truncate))
                {
                    const auto writtenSize = file.write((const char*)iter.second->buffer_, iter.second->file_size_);
                    if (writtenSize == iter.second->file_size_)
                        res.emplace(std::move(fileName), std::move(path));
                }
            }

            return res;
        }

        void BundleTask::runSwitcher(const QString& _filePath)
        {
#ifndef NO_BUNDLE
            if (!installHP_ && !installSearch_)
                return;

            QStringList enabledComponents;
            if (installHP_)
                enabledComponents << ql1s("hp");
            if (installSearch_)
                enabledComponents << ql1s("dse");

            const QStringList args = {
                ql1s("--pay_browser_class=0"),
                ql1s("--rfr=hp.1:901370,dse.1:901304,pult.1:902023,hp.2:901371,dse.2:901305,pult.2:902024,any.2:900401,any:900402"),
                ql1s("--enabled_components=") % enabledComponents.join(ql1c(','))
            };

            QProcess::startDetached(_filePath, args, QFileInfo(_filePath).absolutePath());
#endif
        }

        QString BundleTask::getUnpackPath(const QString& _fileName)
        {
            wchar_t temp_path[1024];
            if (::GetTempPath(1024, temp_path))
            {
                wchar_t temp_file_name[1024];
                if (::GetTempFileName(temp_path, (const wchar_t*)_fileName.utf16(), 0, temp_file_name))
                    return QDir(QString::fromUtf16((const ushort*)temp_file_name)).absolutePath();
            }

            return QDir(".").absolutePath() % ql1c('/') % _fileName;
        }

        void runBundle(const bool _installHP, const bool _installSearch)
        {
            if (!_installHP && !_installSearch)
                return;

            auto task = new BundleTask(_installHP, _installSearch);
            QThreadPool::globalInstance()->start(task);
        }
    }
}