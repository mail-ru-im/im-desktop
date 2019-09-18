#pragma once

namespace Logic
{
    using SearchPatterns = std::list<QString>;

    class SearchPatternHistory : public QObject
    {
        Q_OBJECT

    public Q_SLOTS:
        void removePattern(const QString& _contact, const QString& _pattern);

    private Q_SLOTS:
        void onPatternsLoaded(const QString& _contact, const QVector<QString>& _patterns);

    public:
        SearchPatternHistory(QObject* _parent);

        void addPattern(const QString& _pattern, const QString& _contact);
        SearchPatterns getPatterns(const QString& _contact);

    private:

        std::map<QString, SearchPatterns> contactPatterns_;
    };

    SearchPatternHistory* getLastSearchPatterns();
    void resetLastSearchPatterns();
}