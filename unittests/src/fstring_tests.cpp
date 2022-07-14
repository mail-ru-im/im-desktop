#include "stdafx.h"
#include "common.h"
#include "../../gui/types/fstring.h"


TEST(FStringViewTests, FStringViewEmptyness)
{
    Data::FStringView v;
    EXPECT_TRUE(v.isEmpty());
    EXPECT_TRUE(v.size() == 0);
}

TEST(FStringViewTests, FStringViewOutOfScope)
{
    QString string = qsl("Hello World!");
    Data::FStringView view;
    {
        Data::FString source(string);
        view = source;
        EXPECT_TRUE(!view.isEmpty());
        EXPECT_EQ(view.size(), source.size());
        EXPECT_TRUE(view.string() == source.string());
    }

    EXPECT_TRUE(view.isEmpty());
    EXPECT_TRUE(view.size() == 0);
}

TEST(FStringViewTests, FStringViewsOutOfScope)
{
    QString string = qsl("Hello World!");
    Data::FStringView views[4];
    {
        Data::FString source(string);
        views[0] = source;
        views[1] = views[0].mid(1, source.size()-1);
        views[2] = views[1].mid(1, 6);
        views[3] = views[2].mid(0, 3);

        EXPECT_TRUE(!views[0].isEmpty());
        EXPECT_EQ(views[0].size(), source.size());
        EXPECT_TRUE(views[0].string() == source.string());

        QString s1 = source.string().mid(1, source.size() - 1);
        QString s2 = s1.mid(1, 6);
        QString s3 = s2.mid(0, 3);
        EXPECT_TRUE(views[1].string() == s1);
        EXPECT_TRUE(views[2].string() == s2);
        EXPECT_TRUE(views[3].string() == s3);
    }

    for (size_t i = 0; i < std::size(views); ++i)
    {
        EXPECT_TRUE(views[0].isEmpty());
        EXPECT_TRUE(views[0].size() == 0);
    }
}



TEST(FStringViewTests, FStringViewSourceChanged)
{
    QString helloStr = qsl("Hello World!");
    QString goodbyeStr = qsl("Goodbye World!");
    Data::FStringView views[4];
    {
        Data::FString source(helloStr);
        Data::FString other(goodbyeStr);
        views[0] = source;
        views[1] = views[0].mid(1, source.size() - 1);
        views[2] = views[1].mid(1, 6);
        views[3] = views[2].mid(0, 3);

        source = other;

        QString s1 = source.string().mid(1, source.size() - 1);
        QString s2 = s1.mid(1, 6);
        QString s3 = s2.mid(0, 3);
        EXPECT_FALSE(views[1].string() == s1);
        EXPECT_FALSE(views[2].string() == s2);
        EXPECT_FALSE(views[3].string() == s3);
    }

    for (size_t i = 0; i < std::size(views); ++i)
    {
        EXPECT_TRUE(views[0].isEmpty());
        EXPECT_TRUE(views[0].size() == 0);
    }
}
