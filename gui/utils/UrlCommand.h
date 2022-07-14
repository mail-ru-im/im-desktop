#pragma once

class UrlCommand;

namespace UrlCommandBuilder
{
    enum class Context {External, Internal};
    std::unique_ptr<UrlCommand> makeCommand(QString _url, Context _context = Context::Internal);
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

class OpenAppCommand : public UrlCommand
{
public:
    OpenAppCommand(const QString& _appType, const QString& _urlQuery, const QString& _urlFragment);
    void execute() override;
private:
    QString app_;
    QString urlQuery_;
    QString urlFragment_;
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
    OpenIdCommand(QString _id, UrlCommandBuilder::Context _context = UrlCommandBuilder::Context::Internal, std::optional<QString> _botParams = {});
    void execute() override;

private:
    QString id_;
    UrlCommandBuilder::Context context_;
    std::optional<QString> params_;
};

class PhoneAttachedCommand : public UrlCommand
{
public:
    PhoneAttachedCommand(const bool _success, const bool _cancel);
    void execute() override;

private:
    bool success_;
    bool cancel_;
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

class VCSCommand : public UrlCommand
{
public:
    VCSCommand(const QString& _url);
    void execute() override;
private:
    QString url_;
};

class AuthModalResult : public UrlCommand
{
public:
    AuthModalResult(const QString& _requestId, const QString& _urlQuery);
    void execute() override;
private:
    QString urlQuery_;
    QString requestId_;
};

class CreateChatCommand : public UrlCommand
{
public:
    CreateChatCommand(const QString& _url);
    void execute() override;
};

class CreateChannelCommand : public UrlCommand
{
public:
    CreateChannelCommand(const QString& _url);
    void execute() override;
};

class CopyToClipboardCommand : public UrlCommand
{
public:
    CopyToClipboardCommand(const QString& _text);
    void execute() override;
private:
    QString text_;
};
