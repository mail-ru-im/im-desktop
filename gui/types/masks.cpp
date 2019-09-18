#include "stdafx.h"

#include "masks.h"

#include "utils/gui_coll_helper.h"

namespace Data
{
    std::vector<Mask> UnserializeMasks(core::coll_helper* _helper)
    {
        std::vector<Mask> result;

        const auto array = _helper->get_value_as_array("masks");

        result.reserve(array->size());

        for (int32_t i = 0; i < array->size(); ++i)
        {
            const auto val = array->get_at(i);
            Ui::gui_coll_helper mask_helper(val->get_as_collection(), false);

            Mask m;

            m.name_ = mask_helper.get<QString>("name");
            m.json_path_ = mask_helper.get<QString>("json_path");

            result.push_back(std::move(m));
        }

        return result;
    }

}
