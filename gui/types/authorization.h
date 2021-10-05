#pragma once
namespace core
{
    class coll_helper;
}


namespace Data
{
    struct MiniAppAuthParams
    {
        bool operator==(const MiniAppAuthParams& _other) const { return aimsid_ == _other.aimsid_; }
        QString aimsid_;
    };

    class AuthParams
    {
    public:
        AuthParams(const core::coll_helper& _coll);
        AuthParams() = default;
        AuthParams(AuthParams&&) noexcept = default ;
        AuthParams& operator=(AuthParams&&) noexcept = default;
        AuthParams(const AuthParams&) = default;
        AuthParams& operator=(const AuthParams&) noexcept = default;

        bool operator==(const AuthParams& _other) const
        {
            return getAimsid() == _other.getAimsid();
        }

        bool operator!=(const AuthParams& _other) const
        {
            return !(*this == _other);
        }

        bool isTheSameMiniApps(const AuthParams& _other) const
        {
            return miniApps_.size() == _other.miniApps_.size() && std::equal(miniApps_.begin(), miniApps_.end(), _other.miniApps_.begin());
        }

        bool isMiniAppsEmpty() const
        {
            return miniApps_.empty();
        }

        const QString& getAimsid() const noexcept { return aimsid_; }

        bool isEmpty() const noexcept { return aimsid_.isEmpty(); }

        const QString& getMiniAppAimsid(const QString& _miniAppId) const;

        void clearMiniapps() { miniApps_.clear(); }

    private:
        QString aimsid_;
        std::map<QString, MiniAppAuthParams> miniApps_;
    };
}

