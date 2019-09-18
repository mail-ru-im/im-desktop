#pragma once

namespace Ui
{
    class core_dispatcher;
}

namespace voip_masks
{
    class Mask
        : public QObject
    {
        Q_OBJECT
    public:
        Mask(const QString& _name, QObject* _parent);

        const QString& id() const;

        void setPreview(const QString _localPath);
        const QPixmap& preview() const;

        void setJsonPath(const QString _localPath);
        const QString& jsonPath() const;

        void setProgressSeq(quint64 _seq);

    signals:
        void previewLoaded();

        void loaded();
        void loadingProgress(unsigned _percent);

    public slots:
        void onLoadingProgress(quint64 _seq, unsigned _percent);

    private:
        const QString name_;
        QPixmap preview_;
        QString jsonPath_;
        quint64 progressSeq_;
    };

    using MaskList = std::vector<QPointer<Mask>>;
}
