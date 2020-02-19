#include "stdafx.h"
#include "copy_files.h"

#include "../../utils/7z/7z.h"
#include "../tools.h"
#include "../../../common.shared/version_info_constants.h"
#include "../../../common.shared/common_defs.h"

namespace installer
{
    namespace logic
    {
        static auto icq_exe_short_filename() noexcept { return ql1s("icq.exe"); }

        static installer::error move_file_via_tmp(QString source, QString dest, QString tmp)
        {
            const auto isExists = QFile::exists(dest);
            source = QDir::toNativeSeparators(source);
            dest = QDir::toNativeSeparators(dest);
            tmp = QDir::toNativeSeparators(tmp);
            if (isExists)
            {
                if (!::MoveFileEx((const wchar_t*)dest.utf16(), (const wchar_t*)tmp.utf16(), MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
                    return installer::error(errorcode::move_file, ql1s("exists: move file: ") % dest % ql1c(' ') % tmp);
                if (!::MoveFileEx((const wchar_t*)source.utf16(), (const wchar_t*)dest.utf16(), MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
                    return installer::error(errorcode::move_file, ql1s("exists: move file: ") % source % ql1c(' ') % dest);
            }
            else
            {
                if (!::MoveFileEx((const wchar_t*)source.utf16(), (const wchar_t*)dest.utf16(), MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
                    return installer::error(errorcode::move_file, ql1s("move file: ") % source % ql1c(' ') % dest);
            }
            return installer::error();
        }

        static installer::error copy_self(const QString& _install_path)
        {
            wchar_t buffer[1025];
            if (!::GetModuleFileName(0, buffer, 1024))
                return installer::error(errorcode::get_module_name, ql1s("get this module name"));

            const auto guid = common::get_guid();
            if (guid.empty())
                return installer::error(errorcode::generate_guid, ql1s("copy installer: generate guid"));
            const QString tmp_path_for_move = get_installer_tmp_folder() % ql1c('/') % QString::fromStdWString(guid);

            QString tmp_file = tmp_path_for_move % ql1c('/') % get_installer_exe_short();

            const QString folder_path = QFileInfo(tmp_file).absoluteDir().absolutePath();
            if (QDir dir; !dir.mkpath(folder_path))
                return installer::error(errorcode::create_product_folder, ql1s("create folder: ") % folder_path);

            if (!::CopyFile(buffer, (const wchar_t*)(QDir::toNativeSeparators(tmp_file)).utf16(), FALSE))
                return installer::error(errorcode::copy_installer, ql1s("copy installer to tmp: ") % tmp_file);

            return move_file_via_tmp(tmp_file, _install_path % ql1c('/') % get_installer_exe_short(), tmp_file % ql1s(".tmp"));
        }

        static installer::error copy_files_to_folder(const QString& _install_path)
        {
            HINSTANCE instance = (HINSTANCE) ::GetModuleHandle(0);
            if (!instance)
                return installer::error(errorcode::invalid_installer_pack, ql1s("get module instance failed"));

            HRSRC hrsrc = ::FindResourceW(instance, L"FILES.7z", L"FILES");
            if (!hrsrc)
                return installer::error(errorcode::invalid_installer_pack, ql1s("FILES.7Z not found"));

            HGLOBAL hgres = ::LoadResource(instance, hrsrc);
            if (!hgres)
                return installer::error(errorcode::invalid_installer_pack, ql1s("LoadResource FILES.7Z failed"));

            const char* data = (const char*)::LockResource(hgres);
            if (!data)
                return installer::error(errorcode::invalid_installer_pack, ql1s("LockResource FILES.7Z failed"));

            unsigned int sz = ::SizeofResource(instance, hrsrc);
            if (!sz)
                return installer::error(errorcode::invalid_installer_pack, ql1s("SizeofResource FILES.7Z failed"));

            zip_pack zip_file;

            if (!zip_file.open(data, sz))
                return installer::error(errorcode::open_files_archive, ql1s("open archive: files.7z"));

            const auto guid = common::get_guid();
            if (guid.empty())
                return installer::error(errorcode::generate_guid, ql1s("generate guid"));

            const QString tmp_path_for_move = get_installer_tmp_folder() % ql1c('/') % QString::fromStdWString(guid);

            for (const auto& [name, archive_file] : zip_file.get_files())
            {
                QString file_name_short = name;
                if (file_name_short == icq_exe_short_filename())
                    file_name_short = get_icq_exe_short();

                QString file_name = _install_path % ql1c('/') % file_name_short;
                const QString folder_path = QFileInfo(file_name).absoluteDir().absolutePath();
                if (QDir dir; !dir.mkpath(folder_path))
                    return installer::error(errorcode::create_product_folder, ql1s("create folder: ") % folder_path);

                QString tmp_file_name = tmp_path_for_move % ql1c('/') % file_name_short;
                const QString tmp_folder_path = QFileInfo(tmp_file_name).absoluteDir().absolutePath();
                if (QDir dir; !dir.mkpath(tmp_folder_path))
                    return installer::error(errorcode::create_product_folder, ql1s("create folder: ") % tmp_folder_path);

                QFile file(tmp_file_name);
                if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
                    return installer::error(errorcode::open_file_for_write, ql1s("open file: ") % tmp_file_name);

                qint64 bytesToWrite = archive_file->file_size_;
                qint64 totalWritten = 0;

                // fix for https://bugreports.qt.io/browse/QTBUG-56616
                do
                {
                    const auto current = file.write(((const char*)archive_file->buffer_) + totalWritten, bytesToWrite);
                    if (current == -1)
                        return installer::error(errorcode::write_file, ql1s("write file: ") % file_name);
                    if (current == 0)
                        break;
                    totalWritten += current;
                    bytesToWrite -= current;
                } while (totalWritten < archive_file->file_size_);

                file.close();

                auto res = move_file_via_tmp(tmp_file_name, std::move(file_name), tmp_file_name % ql1s(".tmp"));

                if (!res.is_ok())
                    return res;
            }

            return copy_self(_install_path);
        }


        installer::error copy_files()
        {
            return copy_files_to_folder(get_install_folder());
        }

        installer::error copy_files_to_updates()
        {
            return copy_files_to_folder(get_updates_folder() % ql1c('/') % QString::fromUtf8(VER_FILEVERSION_STR));
        }

        installer::error delete_files()
        {
            QDir dir_product(get_product_folder());

            dir_product.removeRecursively();

            return installer::error();
        }

        installer::error delete_folder_as_temp(const QString& _folder)
        {
            wchar_t temp_path[1024];
            if (!::GetTempPath(1024, temp_path))
                return installer::error(errorcode::get_temp_path, ql1s("get temp folder"));

            wchar_t temp_file_name[1024];
            if (!::GetTempFileName(temp_path, L"icq", 0, temp_file_name))
                return installer::error(errorcode::get_temp_file_name, ql1s("get temp folder"));

            wchar_t this_module_name[1024];
            if (!::GetModuleFileName(0, this_module_name, 1024))
                return installer::error(errorcode::get_module_name, ql1s("get this module name"));

            if (!::CopyFile(this_module_name, temp_file_name, FALSE))
                return installer::error(errorcode::copy_installer, ql1s("copy installer to temp folder"));

            ::MoveFileEx(temp_file_name, NULL, MOVEFILE_DELAY_UNTIL_REBOOT | MOVEFILE_REPLACE_EXISTING);

            const QString command = QString::fromUtf16((const ushort*) temp_file_name) % ql1s(" -uninstalltmp ") % _folder;
            if (!QProcess::startDetached(command))
                return installer::error(errorcode::start_installer_from_temp, ql1s("start installer from temp folder"));

            return installer::error();
        }

        installer::error delete_self_and_product_folder()
        {
            return delete_folder_as_temp(get_product_folder());
        }

        installer::error delete_updates()
        {
            wchar_t temp_path[1024];
            if (!::GetTempPath(1024, temp_path))
                return installer::error(errorcode::get_temp_path, ql1s("get temp folder"));

            wchar_t temp_file_name[1024];
            if (!::GetTempFileName(temp_path, L"icq", 0, temp_file_name))
                return installer::error(errorcode::get_temp_file_name, ql1s("get temp folder"));

            wchar_t this_module_name[1024];
            if (!::GetModuleFileName(0, this_module_name, 1024))
                return installer::error(errorcode::get_module_name, ql1s("get this module name"));

            if (!::CopyFile(this_module_name, temp_file_name, FALSE))
                return installer::error(errorcode::copy_installer, ql1s("copy installer to temp folder"));

            ::MoveFileEx(temp_file_name, NULL, MOVEFILE_DELAY_UNTIL_REBOOT | MOVEFILE_REPLACE_EXISTING);

            const QString command = QString::fromUtf16((const ushort*) temp_file_name) % ql1c(' ') % delete_updates_command;
            if (!QProcess::startDetached(command))
                return installer::error(errorcode::start_installer_from_temp, ql1s("start installer from temp folder"));

            return installer::error();
        }

        installer::error copy_files_from_updates()
        {
            wchar_t this_module_name[1024];
            if (!::GetModuleFileName(0, this_module_name, 1024))
                return installer::error(errorcode::get_module_name, ql1s("get this module name"));

            const auto guid = common::get_guid();
            if (guid.empty())
                return installer::error(errorcode::generate_guid, ql1s("generate guid"));

            const QString tmp_path_for_move = get_installer_tmp_folder() % ql1c('/') % QString::fromStdWString(guid);

            QFileInfo module_info(QString::fromUtf16((const ushort*) this_module_name));

            QDir update_dir = module_info.absoluteDir();

            update_dir.setFilter(QDir::Files | QDir::Hidden | QDir::NoSymLinks);
            const QFileInfoList files_list = update_dir.entryInfoList();

            for (const auto& file_info : files_list)
            {
                if (QString file_path = file_info.absoluteFilePath(); file_path != module_info.absoluteFilePath())
                {
                    QString tmp_file_name = tmp_path_for_move % ql1c('/') % file_info.fileName() % ql1s(".tmp");
                    const QString tmp_folder_path = QFileInfo(tmp_file_name).absoluteDir().absolutePath();
                    if (QDir dir; !dir.mkpath(tmp_folder_path))
                        return installer::error(errorcode::create_product_folder, ql1s("create folder: ") % tmp_folder_path);

                    auto res = move_file_via_tmp(std::move(file_path), get_install_folder() % ql1c('/') % file_info.fileName(), std::move(tmp_file_name));
                    if (!res.is_ok())
                        return res; // TODO think about rollback
                }
            }

            return installer::error();
        }

        installer::error copy_self_from_updates()
        {
            return copy_self(get_install_folder());
        }
    }
}

