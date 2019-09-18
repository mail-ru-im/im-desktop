#pragma once

class UrlCommand;

namespace UrlCommandBuilder
{
    enum class Context {External, Internal};
    std::shared_ptr<UrlCommand> makeCommand(QString _url, Context _context = Context::Internal);
}

class UrlCommand
{
public:
    virtual ~UrlCommand() = default;

    bool isValid() const { return isValid_; }

    virtual void execute() {}
protected:
    bool isValid_ = false;
};

class OpenContactCommand : public UrlCommand
{
public:
    OpenContactCommand(const QString& _aimId);
    void execute() override;
private:
    QString aimId_;
};

class OpenChatCommand : public UrlCommand
{
public:
    OpenChatCommand(const QString& _stamp, bool _join);
    void execute() override;
private:
    QString stamp_;
    bool join_ = false;
};

class OpenIdCommand : public UrlCommand
{
public:
    OpenIdCommand(const QString& _id);
    void execute() override;

private:
    QString id_;
};

class ShowStickerpackCommand : public UrlCommand
{
public:
    ShowStickerpackCommand(const QString& _storeId, UrlCommandBuilder::Context _context);
    void execute() override;
private:
    QString storeId_;
    UrlCommandBuilder::Context context_;
};

class ServiceCommand : public UrlCommand
{
public:
    ServiceCommand(const QString& _url);
    void execute() override;
private:
    QString url_;
};
