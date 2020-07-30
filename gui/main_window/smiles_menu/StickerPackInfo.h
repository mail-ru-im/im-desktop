#pragma once

#include "../../cache/stickers/stickers.h"

namespace Ui
{
    class CustomButton;
    class TextEditEx;
    class ContextMenu;

    namespace Stickers
    {
        class Set;
    };

    namespace Smiles
    {
        class StickersTable;
        class StickerPreview;
    }



    class GeneralDialog;


    class StickersView : public QWidget
    {
    Q_OBJECT

    private Q_SLOTS:

        void onStickerPreview(const QString& _stickerId);

    Q_SIGNALS :

        void stickerPreviewClose();
        void stickerPreview(const int32_t _setId, const QString& _stickerId);
        void stickerHovered(const int32_t _setId, const QString& _stickerId);

    private:

        Smiles::StickersTable* stickers_;

        bool previewActive_;

    public:

        StickersView(QWidget* _parent, Smiles::StickersTable* _stickers);

        void stickersPreviewClosed();

        virtual void mouseReleaseEvent(QMouseEvent* _e) override;
        virtual void mousePressEvent(QMouseEvent* _e) override;
        virtual void mouseMoveEvent(QMouseEvent* _e) override;
        virtual void leaveEvent(QEvent* _e) override;
    };



    class PackErrorWidget : public QWidget
    {
        Q_OBJECT

    public:

        PackErrorWidget(QWidget* _parent);
        virtual ~PackErrorWidget();

        void setErrorText(const QString& _text);

    protected:

        virtual void paintEvent(QPaintEvent* _e) override;

    private:

        QString errorText_;
    };



    class PackWidget : public QWidget
    {
        Q_OBJECT

        QVBoxLayout* rootVerticalLayout_;

        QVBoxLayout* stickersLayout_;

        QScrollArea* viewArea_;

        Smiles::StickersTable* stickers_;

        std::unique_ptr<Smiles::StickerPreview> stickerPreview_;

        StickersView* stickersView_;

        PackErrorWidget* errorWidget_;

        QPushButton* addButton_;

        QPushButton* removeButton_;

        qint32 setId_;

        QString storeId_;

        QString subtitle_;

        CustomButton* moreButton_;

        QLabel* loadingText_;

        TextEditEx* subtitleControl_;

        GeneralDialog* dialog_;

        QString name_;

        bool purchased_;

        Stickers::StatContext context_;


    public Q_SLOTS:

        void onDialogAccepted();

    private Q_SLOTS:

        void onAddButton(bool _checked);
        void onRemoveButton(bool _checked);
        void touchScrollStateChanged(QScroller::State _state);
        void onStickerPreviewClose();
        void onStickerPreview(const int32_t _setId, const QString& _stickerId);
        void onStickerHovered(const int32_t _setId, const QString& _stickerId);
        void contextMenuAction(QAction* _action);

    Q_SIGNALS:

        void buttonClicked();
        void forward();
        void report();

    public:

        PackWidget(Stickers::StatContext _context, QWidget* _parent);
        ~PackWidget();

        void onStickersPackInfo(std::shared_ptr<Ui::Stickers::Set> _set, const bool _result, const bool _purchased);
        void onStickerEvent(const qint32 _error, const qint32 _setId, const QString& _stickerId);

        void setParentDialog(GeneralDialog* _dialog);
        QString getName() const { return name_; }

        void setError(const QString& _text);

    protected:

        virtual void paintEvent(QPaintEvent* _e) override;
        virtual void resizeEvent(QResizeEvent* _e) override;

    private:

        void moveMoreButton();

        void sendAddPackStats();
    };



    class StickerPackInfo : public QWidget
    {
        Q_OBJECT

        PackWidget* pack_;

        int32_t set_id_;

        QString store_id_;

        QString file_id_;

        Stickers::StatContext context_;

    private Q_SLOTS:

        void onStickerpackInfo(const bool _result, const bool _exist, std::shared_ptr<Ui::Stickers::Set> _set);
        void stickerEvent(const qint32 _error, const qint32 _setId, const QString& _stickerId);
        void onShareClicked();
        void onReportClicked();

    public:

        StickerPackInfo(QWidget* _parent,
            const int32_t _set_id,
            const QString& _store_id,
            const QString& _file_id,
            Stickers::StatContext _context);

        virtual ~StickerPackInfo();

        void show();

    protected:

        virtual void paintEvent(QPaintEvent* _e) override;

    private:

        std::unique_ptr<GeneralDialog> parentDialog_;
    };
}
