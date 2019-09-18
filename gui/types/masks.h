#pragma once

namespace core
{
    class coll_helper;
}

namespace Data
{

    struct Mask
    {
        QString name_;
        QString json_path_;
    };


    std::vector<Mask> UnserializeMasks(core::coll_helper* _helper);
}
