#pragma once

namespace Ui
{
    class CustomButton;

    namespace TextRendering
    {
        class TextUnit;
    }

    class SettingsHeader : public QWidget
    {
        Q_OBJECT;

    public:
        explicit SettingsHeader(QWidget* _parent = nullptr);
        ~SettingsHeader();

        void setText(const QString& _text);

    signals:
        void backClicked(QPrivateSignal) const;

    protected:
        void paintEvent(QPaintEvent* _event) override;

    private:
        std::unique_ptr<Ui::TextRendering::TextUnit> textUnit_;
        CustomButton* backButton_;
    };
}