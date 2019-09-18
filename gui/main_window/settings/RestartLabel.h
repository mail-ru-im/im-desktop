#pragma once

namespace Ui
{
    namespace TextRendering
    {
        class TextUnit;
    }

    class RestartLabel : public QWidget
    {
        Q_OBJECT;

    public:
        explicit RestartLabel(QWidget* _parent = nullptr);
        ~RestartLabel();

    protected:
        void paintEvent(QPaintEvent* _event) override;

    private:
        std::unique_ptr<Ui::TextRendering::TextUnit> textUnit_;
    };
}