#pragma once

namespace Detail
{
    template <typename Base, typename ...Args>
    class AbstractCreator
    {
    public:
        virtual ~AbstractCreator() = default;
        virtual Base* create(Args... args) const = 0;
        virtual std::unique_ptr<Base> createUnique(Args... args) const = 0;
        virtual std::shared_ptr<Base> createShared(Args... args) const = 0;
    };

    template <typename Object, typename Base, typename ...Args>
    class Creator : public AbstractCreator<Base, Args...>
    {
    public:
        Base* create(Args... args) const override
        {
            static_assert(std::is_base_of_v<Base, Object>);
            return new Object(args ...);
        }

        std::unique_ptr<Base> createUnique(Args... args) const override
        {
            static_assert(std::is_base_of_v<Base, Object>);
            return std::make_unique<Object>(args ...);
        }

        std::shared_ptr<Base> createShared(Args... args) const override
        {
            static_assert(std::is_base_of_v<Base, Object>);
            return std::make_shared<Object>(args ...);
        }
    };
} // namespace Detail

namespace Utils
{
    template <typename Base, typename Id, typename ...Args>
    class ObjectFactory
    {
        using Factory = Detail::AbstractCreator<Base, Args...>;

    public:
        Base* create(const Id& _id, Args... args) const
        {
            if (const auto& f = findFactory(_id))
                return f->create(args ...);
            return nullptr;
        }

        std::unique_ptr<Base> createUnique(const Id& _id, Args... args) const
        {
            if (const auto& f = findFactory(_id))
                return f->createUnique(args ...);
            return {};
        }

        std::shared_ptr<Base> createShared(const Id& _id, Args... args) const
        {
            if (const auto& f = findFactory(_id))
                return f->createShared(args ...);
            return {};
        }

        bool isRegistered(const Id& _id) const
        {
            return factories_.find(_id) != factories_.end();
        }

        template <typename T>
        void registerClass(const Id& _id)
        {
            if (!isRegistered(_id))
                factories_[_id] = std::make_unique<Detail::Creator<T, Base, Args...>>();
        }

        template <typename T>
        void registerDefaultClass()
        {
            defaultFactory_ = std::make_unique<Detail::Creator<T, Base, Args...>>();
        }

    private:
        const std::unique_ptr<Factory>& findFactory(const Id& _id) const
        {
            if (const auto it = factories_.find(_id); it != factories_.end())
                return it->second;
            return defaultFactory_;
        }

    private:
        std::unordered_map<Id, std::unique_ptr<Factory>> factories_;
        std::unique_ptr<Factory> defaultFactory_;
    };

} // namespace Utils
