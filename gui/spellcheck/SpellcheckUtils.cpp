#include "stdafx.h"

#include "SpellcheckUtils.h"

namespace spellcheck
{
namespace utils
{
    namespace
    {
        bool isAcuteAccentChar(QChar c) noexcept
        {
            static const QChar list[] = { // TODO use constexpr after qt update
                QChar(769),	QChar(833), QChar(714),	QChar(779),	QChar(733), QChar(758),	QChar(791),	QChar(719)
            };

            return std::any_of(std::begin(list), std::end(list), [c](auto x){ return c == x; });
        }

        constexpr bool isBlackListed(QChar::Script s) noexcept
        {
            constexpr QChar::Script list[] = {
                QChar::Script_Katakana,
                QChar::Script_Han,
            };

            for (auto x : list) // use std::any_of from c++20
                if (x == s)
                    return true;
            return false;
        }


        struct SubtagScript
        {
            QStringView subtag;
            QChar::Script script = QChar::Script_Common;
        };

        constexpr SubtagScript localeScriptList[] = {
            {u"aa", QChar::Script_Latin},       {u"ab", QChar::Script_Cyrillic},
            {u"ady", QChar::Script_Cyrillic},   {u"aeb", QChar::Script_Arabic},
            {u"af", QChar::Script_Latin},       {u"ak", QChar::Script_Latin},
            {u"am", QChar::Script_Ethiopic},    {u"ar", QChar::Script_Arabic},
            {u"arq", QChar::Script_Arabic},     {u"ary", QChar::Script_Arabic},
            {u"arz", QChar::Script_Arabic},     {u"as", QChar::Script_Bengali},
            {u"ast", QChar::Script_Latin},      {u"av", QChar::Script_Cyrillic},
            {u"ay", QChar::Script_Latin},       {u"az", QChar::Script_Latin},
            {u"azb", QChar::Script_Arabic},     {u"ba", QChar::Script_Cyrillic},
            {u"bal", QChar::Script_Arabic},     {u"be", QChar::Script_Cyrillic},
            {u"bej", QChar::Script_Arabic},     {u"bg", QChar::Script_Cyrillic},
            {u"bi", QChar::Script_Latin},       {u"bn", QChar::Script_Bengali},
            {u"bo", QChar::Script_Tibetan},     {u"bqi", QChar::Script_Arabic},
            {u"brh", QChar::Script_Arabic},     {u"bs", QChar::Script_Latin},
            {u"ca", QChar::Script_Latin},       {u"ce", QChar::Script_Cyrillic},
            {u"ceb", QChar::Script_Latin},      {u"ch", QChar::Script_Latin},
            {u"chk", QChar::Script_Latin},      {u"cja", QChar::Script_Arabic},
            {u"cjm", QChar::Script_Arabic},     {u"ckb", QChar::Script_Arabic},
            {u"cs", QChar::Script_Latin},       {u"cy", QChar::Script_Latin},
            {u"da", QChar::Script_Latin},       {u"dcc", QChar::Script_Arabic},
            {u"de", QChar::Script_Latin},       {u"doi", QChar::Script_Arabic},
            {u"dv", QChar::Script_Thaana},      {u"dyo", QChar::Script_Arabic},
            {u"dz", QChar::Script_Tibetan},     {u"ee", QChar::Script_Latin},
            {u"efi", QChar::Script_Latin},      {u"el", QChar::Script_Greek},
            {u"en", QChar::Script_Latin},       {u"es", QChar::Script_Latin},
            {u"et", QChar::Script_Latin},       {u"eu", QChar::Script_Latin},
            {u"fa", QChar::Script_Arabic},      {u"fi", QChar::Script_Latin},
            {u"fil", QChar::Script_Latin},      {u"fj", QChar::Script_Latin},
            {u"fo", QChar::Script_Latin},       {u"fr", QChar::Script_Latin},
            {u"fur", QChar::Script_Latin},      {u"fy", QChar::Script_Latin},
            {u"ga", QChar::Script_Latin},       {u"gaa", QChar::Script_Latin},
            {u"gba", QChar::Script_Arabic},     {u"gbz", QChar::Script_Arabic},
            {u"gd", QChar::Script_Latin},       {u"gil", QChar::Script_Latin},
            {u"gl", QChar::Script_Latin},       {u"gjk", QChar::Script_Arabic},
            {u"gju", QChar::Script_Arabic},     {u"glk", QChar::Script_Arabic},
            {u"gn", QChar::Script_Latin},       {u"gsw", QChar::Script_Latin},
            {u"gu", QChar::Script_Gujarati},    {u"ha", QChar::Script_Latin},
            {u"haw", QChar::Script_Latin},      {u"haz", QChar::Script_Arabic},
            {u"he", QChar::Script_Hebrew},      {u"hi", QChar::Script_Devanagari},
            {u"hil", QChar::Script_Latin},      {u"hnd", QChar::Script_Arabic},
            {u"hno", QChar::Script_Arabic},     {u"ho", QChar::Script_Latin},
            {u"hr", QChar::Script_Latin},       {u"ht", QChar::Script_Latin},
            {u"hu", QChar::Script_Latin},       {u"hy", QChar::Script_Armenian},
            {u"id", QChar::Script_Latin},       {u"ig", QChar::Script_Latin},
            {u"ii", QChar::Script_Yi},          {u"ilo", QChar::Script_Latin},
            {u"inh", QChar::Script_Cyrillic},   {u"is", QChar::Script_Latin},
            {u"it", QChar::Script_Latin},       {u"iu", QChar::Script_CanadianAboriginal},
            {u"ja", QChar::Script_Katakana},    // or Script_Hiragana.
            {u"jv", QChar::Script_Latin},       {u"ka", QChar::Script_Georgian},
            {u"kaj", QChar::Script_Latin},      {u"kam", QChar::Script_Latin},
            {u"kbd", QChar::Script_Cyrillic},   {u"kha", QChar::Script_Latin},
            {u"khw", QChar::Script_Arabic},     {u"kk", QChar::Script_Cyrillic},
            {u"kl", QChar::Script_Latin},       {u"km", QChar::Script_Khmer},
            {u"kn", QChar::Script_Kannada},     {u"ko", QChar::Script_Hangul},
            {u"kok", QChar::Script_Devanagari}, {u"kos", QChar::Script_Latin},
            {u"kpe", QChar::Script_Latin},      {u"krc", QChar::Script_Cyrillic},
            {u"ks", QChar::Script_Arabic},      {u"ku", QChar::Script_Arabic},
            {u"kum", QChar::Script_Cyrillic},   {u"kvx", QChar::Script_Arabic},
            {u"kxp", QChar::Script_Arabic},     {u"ky", QChar::Script_Cyrillic},
            {u"la", QChar::Script_Latin},       {u"lah", QChar::Script_Arabic},
            {u"lb", QChar::Script_Latin},       {u"lez", QChar::Script_Cyrillic},
            {u"lki", QChar::Script_Arabic},     {u"ln", QChar::Script_Latin},
            {u"lo", QChar::Script_Lao},         {u"lrc", QChar::Script_Arabic},
            {u"lt", QChar::Script_Latin},       {u"luz", QChar::Script_Arabic},
            {u"lv", QChar::Script_Latin},       {u"mai", QChar::Script_Devanagari},
            {u"mdf", QChar::Script_Cyrillic},   {u"mfa", QChar::Script_Arabic},
            {u"mg", QChar::Script_Latin},       {u"mh", QChar::Script_Latin},
            {u"mi", QChar::Script_Latin},       {u"mk", QChar::Script_Cyrillic},
            {u"ml", QChar::Script_Malayalam},   {u"mn", QChar::Script_Cyrillic},
            {u"mr", QChar::Script_Devanagari},  {u"ms", QChar::Script_Latin},
            {u"mt", QChar::Script_Latin},       {u"mvy", QChar::Script_Arabic},
            {u"my", QChar::Script_Myanmar},     {u"myv", QChar::Script_Cyrillic},
            {u"mzn", QChar::Script_Arabic},     {u"na", QChar::Script_Latin},
            {u"nb", QChar::Script_Latin},       {u"ne", QChar::Script_Devanagari},
            {u"niu", QChar::Script_Latin},      {u"nl", QChar::Script_Latin},
            {u"nn", QChar::Script_Latin},       {u"nr", QChar::Script_Latin},
            {u"nso", QChar::Script_Latin},      {u"ny", QChar::Script_Latin},
            {u"oc", QChar::Script_Latin},       {u"om", QChar::Script_Latin},
            {u"or", QChar::Script_Oriya},       {u"os", QChar::Script_Cyrillic},
            {u"pa", QChar::Script_Gurmukhi},    {u"pag", QChar::Script_Latin},
            {u"pap", QChar::Script_Latin},      {u"pau", QChar::Script_Latin},
            {u"pl", QChar::Script_Latin},       {u"pon", QChar::Script_Latin},
            {u"prd", QChar::Script_Arabic},     {u"prs", QChar::Script_Arabic},
            {u"ps", QChar::Script_Arabic},      {u"pt", QChar::Script_Latin},
            {u"qu", QChar::Script_Latin},       {u"rm", QChar::Script_Latin},
            {u"rmt", QChar::Script_Arabic},     {u"rn", QChar::Script_Latin},
            {u"ro", QChar::Script_Latin},       {u"ru", QChar::Script_Cyrillic},
            {u"rw", QChar::Script_Latin},       {u"sa", QChar::Script_Devanagari},
            {u"sah", QChar::Script_Cyrillic},   {u"sat", QChar::Script_Latin},
            {u"sd", QChar::Script_Arabic},      {u"sdh", QChar::Script_Arabic},
            {u"se", QChar::Script_Latin},       {u"sg", QChar::Script_Latin},
            {u"shi", QChar::Script_Arabic},     {u"si", QChar::Script_Sinhala},
            {u"sid", QChar::Script_Latin},      {u"sk", QChar::Script_Latin},
            {u"skr", QChar::Script_Arabic},     {u"sl", QChar::Script_Latin},
            {u"sm", QChar::Script_Latin},       {u"so", QChar::Script_Latin},
            {u"sq", QChar::Script_Latin},       {u"sr", QChar::Script_Cyrillic},
            {u"ss", QChar::Script_Latin},       {u"st", QChar::Script_Latin},
            {u"su", QChar::Script_Latin},       {u"sus", QChar::Script_Arabic},
            {u"sv", QChar::Script_Latin},       {u"sw", QChar::Script_Latin},
            {u"swb", QChar::Script_Arabic},     {u"syr", QChar::Script_Arabic},
            {u"ta", QChar::Script_Tamil},       {u"te", QChar::Script_Telugu},
            {u"tet", QChar::Script_Latin},      {u"tg", QChar::Script_Cyrillic},
            {u"th", QChar::Script_Thai},        {u"ti", QChar::Script_Ethiopic},
            {u"tig", QChar::Script_Ethiopic},   {u"tk", QChar::Script_Latin},
            {u"tkl", QChar::Script_Latin},      {u"tl", QChar::Script_Latin},
            {u"tn", QChar::Script_Latin},       {u"to", QChar::Script_Latin},
            {u"tpi", QChar::Script_Latin},      {u"tr", QChar::Script_Latin},
            {u"trv", QChar::Script_Latin},      {u"ts", QChar::Script_Latin},
            {u"tt", QChar::Script_Cyrillic},    {u"ttt", QChar::Script_Arabic},
            {u"tvl", QChar::Script_Latin},      {u"tw", QChar::Script_Latin},
            {u"ty", QChar::Script_Latin},       {u"tyv", QChar::Script_Cyrillic},
            {u"udm", QChar::Script_Cyrillic},   {u"ug", QChar::Script_Arabic},
            {u"uk", QChar::Script_Cyrillic},    {u"und", QChar::Script_Latin},
            {u"ur", QChar::Script_Arabic},      {u"uz", QChar::Script_Cyrillic},
            {u"ve", QChar::Script_Latin},       {u"vi", QChar::Script_Latin},
            {u"wal", QChar::Script_Ethiopic},   {u"war", QChar::Script_Latin},
            {u"wo", QChar::Script_Latin},       {u"xh", QChar::Script_Latin},
            {u"yap", QChar::Script_Latin},      {u"yo", QChar::Script_Latin},
            {u"za", QChar::Script_Latin},       {u"zdj", QChar::Script_Arabic},
            {u"zh", QChar::Script_Han},         {u"zu", QChar::Script_Latin},
            // Encompassed languages within the Chinese macrolanguage.
            // http://www-01.sil.org/iso639-3/documentation.asp?id=zho
            // http://lists.w3.org/Archives/Public/public-i18n-cjk/2016JulSep/0022.html
            // {"cdo", USCRIPT_SIMPLIFIED_HAN},
            // {"cjy", USCRIPT_SIMPLIFIED_HAN},
            // {"cmn", USCRIPT_SIMPLIFIED_HAN},
            // {"cpx", USCRIPT_SIMPLIFIED_HAN},
            // {"czh", USCRIPT_SIMPLIFIED_HAN},
            // {"czo", USCRIPT_SIMPLIFIED_HAN},
            // {"gan", USCRIPT_SIMPLIFIED_HAN},
            // {"hsn", USCRIPT_SIMPLIFIED_HAN},
            // {"mnp", USCRIPT_SIMPLIFIED_HAN},
            // {"wuu", USCRIPT_SIMPLIFIED_HAN},
            // {"hak", USCRIPT_TRADITIONAL_HAN},
            // {"lzh", USCRIPT_TRADITIONAL_HAN},
            // {"nan", USCRIPT_TRADITIONAL_HAN},
            // {"yue", USCRIPT_TRADITIONAL_HAN},
            // {"zh-cdo", USCRIPT_SIMPLIFIED_HAN},
            // {"zh-cjy", USCRIPT_SIMPLIFIED_HAN},
            // {"zh-cmn", USCRIPT_SIMPLIFIED_HAN},
            // {"zh-cpx", USCRIPT_SIMPLIFIED_HAN},
            // {"zh-czh", USCRIPT_SIMPLIFIED_HAN},
            // {"zh-czo", USCRIPT_SIMPLIFIED_HAN},
            // {"zh-gan", USCRIPT_SIMPLIFIED_HAN},
            // {"zh-hsn", USCRIPT_SIMPLIFIED_HAN},
            // {"zh-mnp", USCRIPT_SIMPLIFIED_HAN},
            // {"zh-wuu", USCRIPT_SIMPLIFIED_HAN},
            // {"zh-hak", USCRIPT_TRADITIONAL_HAN},
            // {"zh-lzh", USCRIPT_TRADITIONAL_HAN},
            // {"zh-nan", USCRIPT_TRADITIONAL_HAN},
            // {"zh-yue", USCRIPT_TRADITIONAL_HAN},
            // // Chinese with regions. Logically, regions should be handled
            // // separately, but this works for the current purposes.
            // {"zh-hk", USCRIPT_TRADITIONAL_HAN},
            // {"zh-mo", USCRIPT_TRADITIONAL_HAN},
            // {"zh-tw", USCRIPT_TRADITIONAL_HAN},
        };

        //bool isSupported

    }
    QChar::Script wordScript(QStringView word) noexcept
    {
        if (const auto it = std::find_if(word.begin(), word.end(), [](auto c) { return c.isLetter(); }); it != word.end())
            return it->script();
        return QChar::Script_Common;
    }

    QChar::Script localeToScriptCode(const QStringRef& locale) noexcept
    {
        const auto subtag = locale.left(std::max(locale.indexOf(u'_'), locale.indexOf(u'-')));
        for (auto x : localeScriptList)
            if (subtag == x.subtag)
                return x.script;

        return QChar::Script_Common;
    }

    std::vector<QString> systemLanguages()
    {
        const static auto languages = []()
        {
            std::vector<QString> res;
            const auto uiLanguages = QLocale::system().uiLanguages();
            res.reserve(uiLanguages.size());

            for (const auto& x : uiLanguages)
                res.push_back(x.left(std::max(x.indexOf(u'_'), x.indexOf(u'-'))));
            std::sort(res.begin(), res.end());
            res.erase(std::unique(res.begin(), res.end()), res.end());
            return res;
        }();
        return languages;
    }

    std::vector<QChar::Script> supportedScripts()
    {
        const static auto scripts = []()
        {
            const auto languages = systemLanguages();
            std::vector<QChar::Script> res;
            res.reserve(languages.size());
            for (const auto& l : languages)
            {
                const auto s = utils::localeToScriptCode(QStringRef(&l));
                if (s == QChar::Script_Common || isBlackListed(s))
                    continue;
                res.push_back(s);
            }

            if (res.empty())
            {
                res.push_back(QChar::Script_Common);
            }
            else
            {
                std::sort(res.begin(), res.end());
                res.erase(std::unique(res.begin(), res.end()), res.end());
            }

            return res;
        }();

        return scripts;
    }

    Misspellings checkText(QStringView text, CheckFunction f)
    {
        Misspellings misspellings;
        if (!f || text.isEmpty())
            return misspellings;

        auto finder = QTextBoundaryFinder(QTextBoundaryFinder::Word, text.data(), text.size());

        auto atEnd = [&finder]
        {
            return finder.toNextBoundary() == -1;
        };

        const auto textSize = text.size();
        while (finder.position() < textSize)
        {
            if (!finder.boundaryReasons().testFlag(QTextBoundaryFinder::StartOfItem))
            {
                if (atEnd())
                    break;
                continue;
            }

            const auto start = finder.position();
            const auto end = finder.toNextBoundary();
            if (end == -1)
                break;
            const auto length = end - start;
            if (length < 1)
                continue;

            if (!f(text.mid(start, length)))
                misspellings.push_back(std::make_pair(start, length));

            if (atEnd())
                break;
        }
        return misspellings;
    }

    bool isWordSkippable(QStringView word)
    {
        return word.isEmpty() || std::any_of(word.begin(), word.end(), [s = wordScript(word)](auto c)
        {
            return c.script() != s && !isAcuteAccentChar(c);
        });
    }
}
}
