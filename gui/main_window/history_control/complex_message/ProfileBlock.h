#pragma once

#include "GenericBlock.h"
#include "previewer/Drawable.h"
#include "types/idinfo.h"
#include "types/chat.h"

UI_NS_BEGIN
class ActionButtonWidget;
class UserMiniProfile;
UI_NS_END


UI_COMPLEX_MESSAGE_NS_BEGIN

class ProfileBlockLayout;

class ProfileBlockBase : public GenericBlock
{
    Q_OBJECT

public:
    ProfileBlockBase(ComplexMessageItem *_parent, const QString& _link);
    ~ProfileBlockBase() override;

    IItemBlockLayout* getBlockLayout() const override;

    bool isAllSelected() const override { return isSelected(); }

    void updateFonts() override;

    int desiredWidth(int _width = 0) const override;

    int getMaxWidth() const override;

    int updateSizeForWidth(int _width);

    QString formatRecentsText() const override;

    MediaType getMediaType() const override;

    bool pressed(const QPoint& _p) override;

    bool clicked(const QPoint& _p) override;

    bool isSizeLimited() const override { return true; }

    bool isNeedCheckTimeShift() const override { return true; }
    bool needStretchToOthers() const override { return true; };

    ContentType getContentType() const override { return ContentType::Profile; }

protected Q_SLOTS:
    void onNameTooltipShow(const QString& _text, const QRect& _nameRect);
    void onNameTooltipHide();
    virtual void onButtonPressed() {}
    virtual void onClickAreaPressed() {}

protected:
    void drawBlock(QPainter& _p, const QRect& _rect, const QColor& _quoteColor) override;

    void initialize() override;

    void mouseMoveEvent(QMouseEvent* _event) override;

    virtual QString getButtonText() const;

    void init(const QString& _name, const QString& _underName, const QString& _description);

    virtual QString sn() const { return QString(); }

protected:
    UserMiniProfile* userProfile_;
    ProfileBlockLayout* Layout_;
    QString name_;

    int64_t seq_ = 0;
    int64_t chatInfoSeq_ = 0;

    int cachedWidth_ = 0;
};

class ProfileBlock : public ProfileBlockBase
{
    Q_OBJECT

public:
    ProfileBlock(ComplexMessageItem *_parent, const QString& _link);
    ~ProfileBlock() override;

    static QString extractProfileId(const QString& _link);
    bool isSelected() const override;
    void selectByPos(const QPoint& _from, const QPoint& _to, bool _topToBottom) override;
    Data::FString getSelectedText(const bool _isFullSelect = false, const TextDestination _dest = TextDestination::selection) const override;
    void clearSelection() override;
    void releaseSelection() override;
    void doubleClicked(const QPoint& _p, std::function<void(bool)> _callback = std::function<void(bool)>()) override;

protected:
    QString getButtonText() const override;
    QString getUnderNameText();
    void initialize() override;
    QString sn() const override;

protected Q_SLOTS:
    void onIdInfo(const qint64 _seq, const Data::IdInfo& _idInfo);
    void onButtonPressed() override;
    void onClickAreaPressed() override;
    void chatInfo(qint64, const std::shared_ptr<Data::ChatInfo>&, const int);

private:
    void openChat() const;

private:
    Data::IdInfo info_;
    std::shared_ptr<Data::ChatInfo> chatInfo_;
    QString link_;
};

class PhoneProfileBlock : public ProfileBlockBase
{
    Q_OBJECT

public:
    PhoneProfileBlock(ComplexMessageItem *_parent, const QString& _name, const QString& _phone, const QString& _sn);
    ~PhoneProfileBlock() override;

    Data::FString getSourceText() const override;
    QString getTextForCopy() const override;

    Data::FString getSelectedText(const bool _isFullSelect = false, const TextDestination _dest = TextDestination::selection) const override;

    bool hasSourceText() const override { return false; }

protected:
    QString getButtonText() const override;
    void initialize() override;
    QString sn() const override;

protected Q_SLOTS:
    void onButtonPressed() override;
    void onClickAreaPressed() override;

private:
    QString phone_;
    QString sn_;
};

UI_COMPLEX_MESSAGE_NS_END
