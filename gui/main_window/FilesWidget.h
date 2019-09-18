#pragma once
#include "../controls/TextUnit.h"
#include "../controls/TextEditEx.h"

namespace Ui
{
    class TextEditEx;

    class InputEdit : public TextEditEx
    {
    public:
        InputEdit(QWidget* _parent, const QFont& _font, const QColor& _color, bool _input, bool _isFitToText);

        bool catchEnter(const int _modifiers) override;
        bool catchNewLine(const int _modifiers) override;
    };

    class FilesAreaItem : public QWidget
    {
        Q_OBJECT

    Q_SIGNALS:
        void needUpdate();

    public:
        FilesAreaItem(const QString& _filePath, int _filepath);
        bool operator==(const FilesAreaItem& _other) const;

        void draw(QPainter& _p, const QPoint& _at);
        QString path() const;

    private Q_SLOTS:
        void localPreviewLoaded(QPixmap pixmap, const QSize _originalSize);

    private:
        QPixmap preview_;
        std::unique_ptr<TextRendering::TextUnit> name_;
        std::unique_ptr<TextRendering::TextUnit> size_;
        QString path_;
        bool iconPreview_;
    };

    class FilesArea : public QWidget
    {
        Q_OBJECT

    Q_SIGNALS:
        void sizeChanged();

    public:
        FilesArea(QWidget* _parent);
        void addItem(FilesAreaItem* _item);
        QStringList getItems() const;

        int count() const;
        void clear();

    protected:
        virtual void paintEvent(QPaintEvent* _event) override;
        virtual void mouseMoveEvent(QMouseEvent* _event) override;
        virtual void leaveEvent(QEvent* _event) override;
        virtual void mousePressEvent(QMouseEvent* _event) override;
        virtual void mouseReleaseEvent(QMouseEvent* _event) override;

    private Q_SLOTS:
        void needUpdate();

    private:
        std::vector<std::unique_ptr<FilesAreaItem>> items_;
        QPixmap removeButton_;
        int removeIndex_;
        bool hovered_;
        bool dnd_;
    };


    class FilesScroll : public QScrollArea
    {
        Q_OBJECT
    public:
        FilesScroll(QWidget* _parent, FilesArea* _area, bool _acceptDrops = true);
        void setPlaceholder(bool _placeholder);

    protected:
        virtual void paintEvent(QPaintEvent* _event) override;
        virtual void dragEnterEvent(QDragEnterEvent* _event) override;
        virtual void dragLeaveEvent(QDragLeaveEvent* _event) override;
        virtual void dropEvent(QDropEvent* _event) override;

    private:
        bool isDragDisallowed(const QMimeData* _mimeData) const;

    private:
        bool dnd_;
        bool placeholder_;
        FilesArea* area_;
    };

    class FilesWidget : public QWidget
    {
        Q_OBJECT
    Q_SIGNALS:
        void setButtonActive(bool);

    public:
        FilesWidget(QWidget* _parent, const QStringList& _files, const QPixmap& _imageBuffer = QPixmap());
        ~FilesWidget();

        QStringList getFiles() const;
        QString getDescription() const;
        const Data::MentionMap& getMentions() const;
        void setFocusOnInput();
        void setDescription(const QString& _text, const Data::MentionMap& _mentions = {});

    protected:
        virtual void paintEvent(QPaintEvent* _event) override;

    private Q_SLOTS:
        void descriptionChanged();
        void updateFilesArea();
        void localPreviewLoaded(QPixmap pixmap, const QSize _originalSize);
        void enter();
        void duration(qint64 _duration);
        void scrollRangeChanged(int, int);

    private:
        void initPreview(const QStringList& _files, const QPixmap& _imageBuffer = QPixmap());
        void updateDescriptionHeight();
        void updateSize();
        void setDuration(const QString& _duration);

    private:
        std::unique_ptr<TextRendering::TextUnit> title_;
        std::unique_ptr<TextRendering::TextUnit> durationLabel_;
        InputEdit* description_;
        QPixmap preview_;
        QString previewPath_;
        FilesScroll* area_;
        FilesArea* filesArea_;
        int currentDocumentHeight_;
        int neededHeight_;
        bool drawDuration_;
        bool isGif_;
        int prevMax_;
    };
}
