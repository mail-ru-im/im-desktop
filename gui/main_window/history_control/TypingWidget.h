#pragma once

#include "../../controls/TextUnit.h"

namespace Ui
{
    class TypingWidget : public QWidget
    {
        Q_OBJECT

    private Q_SLOTS:
        void onTypingStatus(const Logic::TypingFires& _typing, const bool _isTyping);

    public:
        TypingWidget(QWidget* _parent, const QString& _aimId);
        ~TypingWidget();

        void updateTheme();

    protected:
        void paintEvent(QPaintEvent* _event) override;
        void showEvent(QShowEvent* _event) override;
        void hideEvent(QHideEvent* _event) override;

    private:
        QString aimId_;
        std::map<QString, QString> typingChattersAimIds_;

        TextRendering::TextUnitPtr textUnit_;

        QVariantAnimation* anim_;
        int frame_;
        QColor animColor_;

        void updateText();
        void ensureAnimationInitialized();
        void startAnimation();
        void stopAnimation();

        Styling::ThemeColorKey getTextColor() const;
        Styling::ThemeColorKey getAnimColor() const;
    };
}
