#pragma once

#include "controls/TextUnit.h"
#include "main_window/input_widget/FileToSend.h"

namespace Ui
{
    class TextEditEx;
    class ThreadPlate;

    class FilesAreaItem : public QWidget
    {
        Q_OBJECT

    Q_SIGNALS:
        void needUpdate();

    public:
        FilesAreaItem(const FileToSend& _file, int _width);
        bool operator==(const FilesAreaItem& _other) const;

        void draw(QPainter& _p, const QPoint& _at);
        const FileToSend& getFile() const noexcept { return file_; }

    private Q_SLOTS:
        void localPreviewLoaded(QPixmap pixmap, const QSize _originalSize);

    private:
        void initText(const QString& _name, int64_t _size, int _width);

    private:
        QPixmap preview_;
        std::unique_ptr<TextRendering::TextUnit> name_;
        std::unique_ptr<TextRendering::TextUnit> size_;
        FileToSend file_;
        bool iconPreview_ = false;
    };

    class FilesArea : public QWidget
    {
        Q_OBJECT

    Q_SIGNALS:
        void sizeChanged();

    public:
        FilesArea(QWidget* _parent);
        void addItem(FilesAreaItem* _item);

        FilesToSend getItems() const;

        int count() const;
        void clear();

    protected:
        void paintEvent(QPaintEvent* _event) override;
        void mouseMoveEvent(QMouseEvent* _event) override;
        void leaveEvent(QEvent* _event) override;
        void mousePressEvent(QMouseEvent* _event) override;
        void mouseReleaseEvent(QMouseEvent* _event) override;

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
        void paintEvent(QPaintEvent* _event) override;
        void dragEnterEvent(QDragEnterEvent* _event) override;
        void dragLeaveEvent(QDragLeaveEvent* _event) override;
        void dropEvent(QDropEvent* _event) override;

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
        enum class Target
        {
            Chat,
            Thread,
        };

        FilesWidget(const FilesToSend& _files, Target _target, QWidget* _parent);
        ~FilesWidget();

        FilesToSend getFiles() const;
        QString getDescription() const;
        int getCursorPos() const;
        const Data::MentionMap& getMentions() const;
        void setFocusOnInput();
        void setDescription(const QString& _text, const Data::MentionMap& _mentions = {});

    protected:
        void paintEvent(QPaintEvent* _event) override;
        void resizeEvent(QResizeEvent* _event) override;

    private Q_SLOTS:
        void descriptionChanged();
        void updateFilesArea();
        void localPreviewLoaded(QPixmap pixmap, const QSize _originalSize);
        void enter();
        void duration(qint64 _duration);
        void scrollRangeChanged(int, int);

    private:
        void initPreview(const FilesToSend& _files);
        void updateDescriptionHeight();
        void updateTitle();
        void updateSize();
        void setDuration(const QString& _duration);

    private:
        std::unique_ptr<TextRendering::TextUnit> title_;
        std::unique_ptr<TextRendering::TextUnit> durationLabel_;
        TextEditEx* description_;

        FileToSend singleFile_;
        QPixmap singlePreview_;

        FilesScroll* area_;
        FilesArea* filesArea_;
        ThreadPlate* threadPlate_ = nullptr;
        int currentDocumentHeight_;
        int neededHeight_;
        bool drawDuration_;
        bool isGif_;
        int prevMax_;
    };
}
