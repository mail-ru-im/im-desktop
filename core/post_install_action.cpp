#include "stdafx.h"

#include "core.h"
#include "tools/system.h"
#include "tools/strings.h"
#include "post_install_action.h"

core::installer_services::post_install_action::post_install_action(std::wstring _file_name)
    : act_file_name_(std::move(_file_name))
{
}

bool core::installer_services::post_install_action::load()
{
    tools::binary_stream input;
    if (!input.load_from_file(act_file_name_))
        return false;

    tools::binary_stream_reader reader(input);
    while (!reader.eof())
    {
        auto line = reader.readline();
        boost::trim(line);
        if (line.empty())
            continue;

        try
        {
            tmp_dirs_.emplace(tools::from_utf8(line));
        }
        catch (...)
        {
            continue;
        }
    }

    return true;
}

void core::installer_services::post_install_action::delete_tmp_resources()
{
    if (!tmp_dirs_.empty())
        for (const auto& dir : tmp_dirs_)
            tools::system::delete_directory(dir);

    if (tools::system::is_exist(act_file_name_))
        tools::system::delete_file(act_file_name_);
}
