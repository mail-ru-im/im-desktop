#pragma once

#include "../animation/animation.h"

namespace Ui
{
    enum class ConnectionState;

    class ProgressAnimation : public QWidget
    {
        Q_OBJECT

    private:

        anim::Animation animation_;
        bool started_;
        int rate_;
        qreal progressWidth_;
        QPen pen_;

        void startAnimation();
        void stopAnimation();

    protected:
        void paintEvent(QPaintEvent* _e) override;
        void showEvent(QShowEvent* _event) override;
        void hideEvent(QHideEvent* _event) override;

    public:
        void start();
        void stop();
        bool isStarted() const;

        void setProgressWidth(qreal _width);
        void setProgressPenWidth(qreal _width);
        void setProgressPenColor(QColor _color);

        ProgressAnimation(QWidget* _parent);
        virtual ~ProgressAnimation();
    };

    class ConnectionStateWatcher : public QObject
    {
        Q_OBJECT

    public:
        explicit ConnectionStateWatcher(QObject *_parent = nullptr);

    public Q_SLOTS:
        void setState(ConnectionState _state);

    private Q_SLOTS:
        void onActivationChanged(bool _isActiveNow);

    private:
        class Aggregator
        {
        public:
            using duration_t = std::chrono::milliseconds;
            using time_point_t = std::chrono::system_clock::time_point;

            Aggregator() = default;

            bool isStarted() const;

            void start();
            void pause();

            void reset();

            duration_t finish();

        private:
            void resetStart();

        private:
            time_point_t start_time_;
            duration_t total_duration_;
        };

    private:
        void handleStateChanged(ConnectionState _old, ConnectionState _new);
        bool shouldStartAggregator() const;
        bool shouldStartAggregator(ConnectionState _state) const;

    private:
        ConnectionState state_;
        Aggregator aggregator_;
        bool isActive_;
    };

    class ConnectionWidget : public QWidget
    {
        Q_OBJECT;

        QLabel* stateTextLabel_;

        ConnectionState state_;

        ProgressAnimation* animationWidget_;

        bool suspended_;

        void setState(const ConnectionState& _state);

    private Q_SLOTS:

        void connectionStateChanged(const ConnectionState& _state);

    public:
        ConnectionWidget(QWidget* _parent, const QColor& _textColor = QColor());
        virtual ~ConnectionWidget();

        void setTextColor(const QColor& _color);

        void suspend();
        void resume();
    };
}
