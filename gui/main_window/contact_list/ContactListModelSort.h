#pragma once

#include "ContactItem.h"
#include "utils/utils.h"
#include "FavoritesUtils.h"

namespace Logic
{
    namespace ContactListSorting
    {
        constexpr int maxTopContactsByOutgoing = 5;
        using contact_sort_pred = std::function<bool (const Logic::ContactItem&, const Logic::ContactItem&)>;

        struct ItemLessThanDisplayName
        {
            ItemLessThanDisplayName(bool _force_cyrillic = false)
                : force_cyrillic_(_force_cyrillic)
                , collator_(Utils::GetTranslator()->getLocale())
            {
                collator_.setCaseSensitivity(platform::is_linux() ? Qt::CaseSensitive : Qt::CaseInsensitive);
            }

            inline bool operator() (const Logic::ContactItem& _first, const Logic::ContactItem& _second)
            {
                auto getDisplayName = [](const Logic::ContactItem& _item)
                {
                    return Favorites::isFavorites(_item.get_aimid()) ? Favorites::name() : _item.Get()->GetDisplayName();
                };

                const auto& firstName = getDisplayName(_first);
                const auto& secondName = getDisplayName(_second);

                const auto firstNotLetter = Utils::startsNotLetter(firstName);
                const auto secondNotLetter = Utils::startsNotLetter(secondName);
                if (firstNotLetter != secondNotLetter)
                    return secondNotLetter;

                if (force_cyrillic_)
                {
                    const auto firstCyrillic = Utils::startsCyrillic(firstName);
                    const auto secondCyrillic = Utils::startsCyrillic(secondName);

                    if (firstCyrillic != secondCyrillic)
                        return firstCyrillic;
                }

                return collator_(firstName, secondName);
            }

            bool force_cyrillic_;
            QCollator collator_;
        };

        struct ItemLessThan
        {
            inline bool operator() (const Logic::ContactItem& _first, const Logic::ContactItem& _second)
            {
                if (_first.Get()->GroupId_ == _second.Get()->GroupId_)
                {
                    if (_first.is_group() && _second.is_group())
                        return false;

                    if (_first.is_group())
                        return true;

                    if (_second.is_group())
                        return false;

                    return ItemLessThanDisplayName()(_first, _second);
                }

                return _first.Get()->GroupId_ < _second.Get()->GroupId_;
            }
        };

        struct ItemLessThanNoGroups
        {
            ItemLessThanNoGroups(const QDateTime& _current)
                : current_(_current)
            {
            }

            inline bool operator() (const Logic::ContactItem& _first, const Logic::ContactItem& _second)
            {
                if (_first.Get()->IsChecked_ != _second.Get()->IsChecked_)
                    return _first.Get()->IsChecked_;

                const auto firstIsActive = _first.is_active(current_);
                const auto secondIsActive = _second.is_active(current_);
                if (firstIsActive != secondIsActive)
                    return firstIsActive;

                return ItemLessThanDisplayName()(_first, _second);
            }

            QDateTime current_;
        };
    }
}