#pragma once

#include "../controls/TextUnit.h"

namespace Ui
{
    class ComboBoxList;

    enum class Reasons
    {
        SPAM = 0,
        PORN = 1,
        VIOLATION = 2,
        OTHER = 3,
    };

    enum class BlockActions
    {
        BLOCK_ONLY = 0,
        REPORT_SPAM = 1,
    };

    enum class BlockAndReportResult
    {
        CANCELED = 0,
        BLOCK = 1,
        SPAM = 2,
    };

    class ReportWidget : public QWidget
    {
        Q_OBJECT
    public:
        ReportWidget(QWidget* _parent, const QString& _title);
        Reasons getReason() const;

    protected:
        void paintEvent(QPaintEvent* _event) override;
        void resizeEvent(QResizeEvent* _event) override;

    private:
        ComboBoxList* reasons_;
        TextRendering::TextUnitPtr title_;
    };

    class BlockWidget : public QWidget
    {
        Q_OBJECT
    public:
        BlockWidget(QWidget* _parent, const QString& _title);
        BlockActions getAction() const;

    protected:
        void paintEvent(QPaintEvent* _event) override;
        void resizeEvent(QResizeEvent* _event) override;

    private:
        ComboBoxList * actions_;
        TextRendering::TextUnitPtr title_;
    };

    QString getReasonString(const Reasons _reason);

    const std::vector<Reasons>& getAllReasons();

    QString getActionString(const BlockActions _reason);

    std::vector<BlockActions> getAllActions();

    bool ReportStickerPack(qint32 _id, const QString& _title);

    bool ReportSticker(const QString& _aimid, const QString& _chatId, const QString& _stickerId);

    bool ReportContact(const QString& _aimid, const QString& _title = QString());

    bool ReportMessage(const QString& _aimId, const QString& _chatId, const int64_t _msgId, const QString& _msgText);

    BlockAndReportResult BlockAndReport(const QString& _aimId, const QString& _title);
}
