#pragma once


namespace Utils
{
    struct CloseWindowInfo;
}

namespace Ui
{
    class qt_gui_settings;

    class SemitransparentWindowAnimated : public QWidget
    {
        Q_OBJECT

    public:
        SemitransparentWindowAnimated(QWidget* _parent, int _duration = 200, bool _forceIgnoreClicks = false);
        ~SemitransparentWindowAnimated();

        Q_PROPERTY(int step READ getStep WRITE setStep)

        void setStep(int _val) { Step_ = _val; update(); }
        int getStep() const { return Step_; }

        void Show();
        void Hide();
        void forceHide();
        bool isSemiWindowVisible() const;

        bool isMainWindow() const;
        void updateSize();

        bool clicksAreIgnored() const;
        void setForceIgnoreClicks(bool _forceIgnoreClicks);

        void setCloseWindowInfo(const Utils::CloseWindowInfo& _info);

    private Q_SLOTS:
        void finished();

    protected:
        void paintEvent(QPaintEvent*) override;
        void mousePressEvent(QMouseEvent *e) override;
        void hideEvent(QHideEvent * event) override;

        void decSemiwindowsCount();
        void incSemiwindowsCount();
        int  getSemiwindowsCount() const;
        bool isSemiWindowsTouchSwallowed() const;
        void setSemiwindowsTouchSwallowed(bool _val);

    private:
        QPropertyAnimation* Animation_;
        int Step_;
        bool main_;
        bool isMainWindow_;
        bool forceIgnoreClicks_;
        std::unique_ptr<Utils::CloseWindowInfo> closeInfo_;
    };
}
