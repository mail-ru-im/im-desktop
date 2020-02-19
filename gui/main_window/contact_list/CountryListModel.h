#pragma once

#include "AbstractSearchModel.h"
#include "../../types/search_result.h"
#include "../../types/chat.h"

namespace Logic
{
    using CountryMap = std::vector<Data::Country>;

    class CountryListModel : public AbstractSearchModel
    {
      Q_OBJECT

    Q_SIGNALS:
          void countrySelected(const QString&);
    public:
        CountryListModel(QObject* _parent);

        int rowCount(const QModelIndex& _parent = QModelIndex()) const override;
        QVariant data(const QModelIndex& _index, int _role) const override;
        bool contains(const QString& _country) const override;

        void setFocus() override;
        void setSearchPattern(const QString& _pattern) override;

        int getCountriesCount() const;
        Data::Country getCountry(const QString& _name) const;
        Data::Country getCountryByCode(const QString& _code) const;

        void setSelected(const QString& _name);
        Data::Country getSelected() const;
        bool isCountrySelected() const;
        void clearSelected();

    private:
        mutable Data::SearchResultsV results_;
        CountryMap countries_;
        QString selectedCountry_;
    };

    CountryListModel* getCountryModel();
}