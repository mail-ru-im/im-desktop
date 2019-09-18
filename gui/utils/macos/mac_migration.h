//
//  mac_migration.h
//  ICQ
//
//  Created by Vladimir Kubyshev on 25/02/16.
//  Copyright © 2015 Mail.RU. All rights reserved.
//

enum class MacProfileType
{
    ICQ     = 0,
    Agent,
};

class MacProfile final
{
public:
    MacProfile(const MacProfileType &type, const QString &identifier, const QString &uin, const QString &pw = QString());
    MacProfile(const MacProfile &profile);
    MacProfile();
    ~MacProfile();

    void setName(const QString &name);
    void setToken(const QString &token);
    void setAimsid(const QString &aimsid);
    void setKey(const QString &key);
    void setFetchUrl(const QString &fetchUrl);
    void setTimeOffset(time_t timeOffset);
    void setSelected(bool selected);

    inline const MacProfileType &type() const { return type_; }
    const QString &name() const;
    const QString &uin() const;
    const QString &pw() const;
    const QString &identifier() const;
    const QString &token() const;
    const QString &aimsid() const;
    const QString &key() const;
    const QString &fetchUrl() const;
    time_t timeOffset() const;
    bool selected() const;


private:
    MacProfileType type_;
    QString uin_;
    QString pw_;
    QString name_;
    QString token_;
    QString key_;
    QString aimsid_;
    QString identifier_;
    QString fetchUrl_;
    time_t timeOffset_;
    bool selected_;
};

typedef QList<MacProfile> MacProfilesList;

class MacMigrationManager : public QObject
{
    Q_OBJECT
public:
    MacMigrationManager(QObject* _parent);
    virtual ~MacMigrationManager();

    void init(QString accountId);

    inline const MacProfilesList &getProfiles() const { return profiles_; }
    bool migrateProfile(const MacProfile &profile, bool isFirst);

    void loginProfile(const MacProfile &profile, const MacProfile& merge);

    static MacProfilesList profiles1(QString profilesPath, QString generalPath);
    static MacProfilesList profiles2(QString accountsDirectory, QString account);
    static QString canMigrateAccount();

private Q_SLOTS:
    void loginResult(int64_t, int);
    void removePassword(const MacProfile &profile);

private:
    void mergeProfile(const MacProfile &profile);

private:
    QString accountId_;
    MacProfilesList profiles_;

    QString passwordMD5(QString pass);
    int64_t loginSeq_;

    MacProfile profileToLogin_;
    MacProfile profileToMerge_;
};


