#pragma once

namespace core
{
    class coll_helper;

    namespace smartreply
    {
        enum class type;
    }
}

namespace Ui
{
    class gui_coll_helper;
}

namespace Data
{
    class SmartreplySuggest
    {
    public:
        SmartreplySuggest();

        bool operator==(const SmartreplySuggest& _other) const noexcept
        {
            return getType() == _other.getType() && getMsgId() == _other.getMsgId() && getData() == _other.getData();
        }

        void unserialize(core::coll_helper& _coll);
        void serialize(Ui::gui_coll_helper& _coll) const;

        core::smartreply::type getType() const noexcept { return type_; }
        qint64 getMsgId() const noexcept { return msgId_; }
        const QString& getContact() const noexcept { return contact_; }
        const QString& getData() const noexcept { return data_; }

        bool isStickerType() const noexcept;
        bool isTextType() const noexcept;

    private:
        core::smartreply::type type_;
        qint64 msgId_;
        QString contact_;
        QString data_;
    };
}