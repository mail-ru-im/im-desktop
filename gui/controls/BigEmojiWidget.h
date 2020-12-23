#pragma once

namespace Ui
{
    enum class ConnectionState;

    class BigEmojiWidget_p;

    //////////////////////////////////////////////////////////////////////////
    // BigEmojiWidget
    //////////////////////////////////////////////////////////////////////////

    class BigEmojiWidget : public QWidget
    {
        Q_OBJECT
    public:
        BigEmojiWidget(const QString& _code, int _size, QWidget* _parent);
        ~BigEmojiWidget();

        void setOpacity(double _opacity);

    protected:
        void paintEvent(QPaintEvent* _event) override;

    private Q_SLOTS:
        void onGetEmojiResult(int64_t _seq, const QImage& _emoji);
        void onGetEmojiFailed(int64_t _seq, bool networkError);
        void onConnectionState(const ConnectionState& _state);

    private:
        std::unique_ptr<BigEmojiWidget_p> d;
    };

}
