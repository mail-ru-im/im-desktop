#include "stdafx.h"

#include "SearchPatternHistory.h"

#include "core_dispatcher.h"
#include "gui_coll_helper.h"

#include "../gui_settings.h"

namespace Logic
{
    SearchPatternHistory::SearchPatternHistory(QObject* _parent)
        : QObject(_parent)
    {
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::searchPatternsLoaded, this, &SearchPatternHistory::onPatternsLoaded);
    }

    void SearchPatternHistory::addPattern(const QString& _pattern, const QString& _contact)
    {
        if (_pattern.isEmpty())
            return;

        auto& hist = contactPatterns_[_contact];
        const auto pat = std::move(_pattern).simplified();

        hist.remove_if([&pat](const auto& p) { return p.compare(p, pat, Qt::CaseInsensitive) == 0; });
        hist.push_front(pat);

        if (hist.size() > core::search::max_patterns_count())
            hist.resize(core::search::max_patterns_count());

        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("pattern", pat);
        collection.set_value_as_qstring("contact", _contact);

        Ui::GetDispatcher()->post_message_to_core("dialogs/search/add_pattern", collection.get());
    }

    SearchPatterns SearchPatternHistory::getPatterns(const QString& _contact)
    {
        return contactPatterns_[_contact];
    }

    void SearchPatternHistory::removePattern(const QString& _contact, const QString& _pattern)
    {
        auto& hist = contactPatterns_[_contact];

        const auto pat = std::move(_pattern).simplified();
        hist.remove_if([&pat](const auto& p) { return p.compare(p, pat, Qt::CaseInsensitive) == 0; });

        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("pattern", pat);
        collection.set_value_as_qstring("contact", _contact);

        Ui::GetDispatcher()->post_message_to_core("dialogs/search/remove_pattern", collection.get());
    }

    void SearchPatternHistory::onPatternsLoaded(const QString& _contact, const QVector<QString>& _patterns)
    {
        auto& hist = contactPatterns_[_contact];
        hist.clear();

        for (const auto& p : _patterns)
            hist.push_back(p);
    }

    std::unique_ptr<SearchPatternHistory> g_search_patterns;
    SearchPatternHistory* getLastSearchPatterns()
    {
        if (!g_search_patterns)
            g_search_patterns = std::make_unique<SearchPatternHistory>(nullptr);

        return g_search_patterns.get();
    }

    void resetLastSearchPatterns()
    {
        g_search_patterns.reset();
    }
}
