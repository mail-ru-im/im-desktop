#include "CountryListModel.h"

namespace Logic
{
    CountryListModel::CountryListModel(QObject* _parent)
        : AbstractSearchModel(_parent)
    {
        const auto cMap = Utils::getCountryCodes();
        countries_.reserve(cMap.size());
        for (auto c = cMap.begin(); c != cMap.end(); ++c)
        {
            countries_.push_back({ c.key(), c.value() });
        }
    }

    int CountryListModel::rowCount(const QModelIndex & _parent) const
    {
        return results_.size();
    }

    QVariant CountryListModel::data(const QModelIndex & _index, int _role) const
    {
        if (!_index.isValid() || (_role != Qt::DisplayRole && !Testing::isAccessibleRole(_role)))
            return QVariant();

        int cur = _index.row();
        if (cur >= (int)rowCount(_index))
            return QVariant();

        if (Testing::isAccessibleRole(_role))
            return results_[cur]->getAccessibleName();

        return QVariant::fromValue(results_[cur]);
    }

    bool CountryListModel::contains(const QString & _country) const
    {
        return std::any_of(countries_.cbegin(), countries_.cend(), [&_country](const auto& x) { return x.name_ == _country; });
    }

    void CountryListModel::setFocus()
    {
        results_.clear();
    }

    void CountryListModel::setSearchPattern(const QString & _pattern)
    {
        results_.clear();

        searchPattern_ = _pattern;

        if (countries_.empty())
            return;

        emit hideNoSearchResults();

        const auto addResult = [this](const auto& _item, const std::vector<QString>& _highlights)
        {
            if (std::none_of(results_.cbegin(), results_.cend(), [&_item](const auto& res) { return _item.name_ == res->getAimId(); }))
            {
                auto resItem = std::make_shared<Data::SearchResultCountry>();
                resItem->country_.name_ = _item.name_;
                resItem->country_.phone_code_ = _item.phone_code_;
                results_.push_back(std::move(resItem));
            }
        };

        const auto match_whole = [](const auto& source, const auto& search)
        {
            if (int(source.size()) != int(search.size()))
                return false;

            auto source_iter = source.cbegin();
            auto search_iter = search.cbegin();

            while (source_iter != source.cend() && search_iter != search.cend())
            {
                if (!source_iter->startsWith(*search_iter, Qt::CaseInsensitive))
                    return false;

                ++source_iter;
                ++search_iter;
            }

            return true;
        };

        const auto reverse_and_match_first_chars = [](const auto& source, const auto& search)
        {
            if (int(source.size()) != int(search.size()))
                return false;

            auto source_iter = source.crbegin();
            auto search_iter = search.crbegin();

            while (source_iter != source.crend() && search_iter != search.crend())
            {
                if (search_iter->length() > 1 || !source_iter->startsWith(*search_iter, Qt::CaseInsensitive))
                    return false;

                ++source_iter;
                ++search_iter;
            }

            return true;
        };

        const auto check_words = [&addResult, &match_whole, &reverse_and_match_first_chars](const auto& pattern, const auto& item, const auto& displayName)
        {
            if (const auto words = pattern.splitRef(QChar::Space, QString::SkipEmptyParts); words.size() > 1)
            {
                const auto displayWords = displayName.splitRef(QChar::Space, QString::SkipEmptyParts);
                if (match_whole(displayWords, words) || reverse_and_match_first_chars(displayWords, words))
                {
                    std::vector<QString> highlights;
                    for (const auto& s : words)
                        highlights.push_back(s.toString());

                    addResult(item, highlights);

                    return true;
                }
            }

            return false;
        };

        const auto checkSimple = [&addResult](const auto& _item, const auto& _pattern)
        {
            if (_item.name_.contains(_pattern, Qt::CaseInsensitive))
            {
                addResult(_item, { _pattern });
                return true;
            }
            return false;
        };

        if (_pattern.isEmpty())
        {
            for (const auto& item : countries_)
                addResult(item, {});
        }
        else
        {
            QString sp = _pattern.toLower();
            unsigned fixed_patterns_count = 0;
            auto searchPatterns = Utils::GetPossibleStrings(sp, fixed_patterns_count);
            for (const auto& item : countries_)
            {
                bool founded = false;
                const auto name = item.name_;
                unsigned i = 0;
                for (; i < fixed_patterns_count; ++i)
                {
                    QString pattern;
                    if (searchPatterns.empty())
                    {
                        pattern = sp;
                    }
                    else
                    {
                        pattern.reserve(searchPatterns.size());
                        for (const auto& iter : searchPatterns)
                        {
                            if (iter.size() > i)
                                pattern += iter.at(i);
                        }
                        pattern = std::move(pattern).trimmed();
                    }

                    if (!pattern.isEmpty())
                    {
                        if (checkSimple(item, pattern) || check_words(pattern, item, name))
                            founded = true;
                    }
                }

                int min = 0;
                for (const auto& s : searchPatterns)
                {
                    if (min == 0)
                        min = int(s.size());
                    else
                        min = std::min(min, int(s.size()));
                }

                QString pattern;
                pattern.reserve(searchPatterns.size());
                while (!searchPatterns.empty() && int(i) < min && !founded)
                {
                    pattern.resize(0);
                    for (const auto& iter : searchPatterns)
                    {
                        if (iter.size() > i)
                            pattern += iter.at(i);
                    }

                    pattern = std::move(pattern).toLower();

                    if (!pattern.isEmpty())
                    {
                        if (checkSimple(item, pattern) || check_words(pattern, item, name))
                            break;
                    }

                    ++i;
                }
            }
        }

        emit dataChanged(index(0), index(rowCount()));

        if (!searchPattern_.isEmpty() && results_.empty())
            emit showNoSearchResults();
        else
            emit hideNoSearchResults();
    }

    int CountryListModel::getCountriesCount() const
    {
        return countries_.size();
    }

    Data::Country CountryListModel::getCountry(const QString & _name) const
    {
        assert(!_name.isEmpty());
        const auto country = std::find_if(countries_.begin(), countries_.end(), [_name](const auto& c) { return c.name_ == _name; });
        if (country == countries_.end())
        {
            static Data::Country empty;
            return empty;
        }

        return *country;
    }

    Data::Country CountryListModel::getCountryByCode(const QString & _code) const
    {
        assert(!_code.isEmpty());
        // CRUTCH: to avoid Kazakhstan
        if (_code == qsl("+7"))
            return getCountry(QT_TRANSLATE_NOOP("countries", "Russia"));
        const auto country = std::find_if(countries_.begin(), countries_.end(), [_code](const auto& c) { return c.phone_code_ == _code; });
        if (country == countries_.end())
        {
            static Data::Country empty;
            return empty;
        }

        return *country;
    }

    void CountryListModel::setSelected(const QString & _name)
    {
        selectedCountry_ = _name;
    }

    Data::Country CountryListModel::getSelected() const
    {
        return getCountry(selectedCountry_);
    }

    bool CountryListModel::isCountrySelected() const
    {
        return !selectedCountry_.isEmpty();
    }

    void CountryListModel::clearSelected()
    {
        selectedCountry_ = QString();
    }

    CountryListModel* getCountryModel()
    {
        static auto countriesModel = std::make_unique<CountryListModel>(nullptr);
        return countriesModel.get();
    }
}