#pragma once

namespace core
{
    namespace tools
    {
        //removes all exif data except the orientation tag; creates a new file and returns it's path if there is exif and stripping is succeded, otherwise returns _filename
        std::wstring strip_exif(const std::wstring& _filename, int _orientation);
    }
}
