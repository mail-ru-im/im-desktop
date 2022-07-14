#pragma once
#include "previewer/Drawable.h"

namespace Ui
{
    class TextWidget;

    enum MiniProfileFlag
    {
        None = 0,
        isOutgoing = 1 << 1,
        isStandalone = 1 << 2,
        isInsideQuote = 1 << 3,
        isTooltip = 1 << 4,
    };

    Q_DECLARE_FLAGS (MiniProfileFlags, MiniProfileFlag)

    class UserMiniProfile : public QWidget
    {
        Q_OBJECT

    Q_SIGNALS:
        void showNameTooltip(const QString& _text, const QRect& _nameRect);
        void hideNameTooltip();
        void onButtonClicked();
        void onClickAreaClicked();

    public:
        UserMiniProfile(QWidget* _parent, const QString _aimId, MiniProfileFlags _flags = MiniProfileFlag::None);
        ~UserMiniProfile() = default;

        void init(const QString& _sn, const QString& _name, const QString& _underName, const QString& _description, const QString& _buttonText);
        void updateFlags(bool _isStandalone, bool _isInQuote);
        void loadAvatar();

        void updateFonts();
        void updateSize(int _width = -1);

        void pressed(const QPoint& _p, bool _hasMultiselect);
        bool clicked(const QPoint& _p, bool _hasMultiselect);

        int getDesiredWidth();
        void selectDescription(QPoint _from, QPoint _to);
        Data::FString getSelectedDescription();
        bool isSelected();
        void clearSelection();
        void releaseSelection();
        void doubleClicked(const QPoint& _p, std::function<void(bool)> _callback);

    protected Q_SLOTS:
        void onAvatarChanged(const QString& aimId);

    protected:
        void paintEvent(QPaintEvent* _e) override;
        void mouseMoveEvent(QMouseEvent* _e) override;
        void mouseReleaseEvent(QMouseEvent* _event) override;
        void leaveEvent(QEvent* _event) override;

    private:
        bool useAccentColors() const;
        bool isStandalone() const;
        bool isInsideQuote() const;
        bool hasArrow() const;

        bool isExtended() const;

        void initTextUnits();
        QRect getClickArea() const;
        void updateStatusText();
        int getMaximumWidth() const;

    private:
        QString aimId_;
        QString underNameText_;
        QString name_;
        QPixmap avatar_;
        std::unique_ptr<BLabel> button_;

        QWidget* quoteMarginWidget_{nullptr};
        QWidget* nameWidget_{nullptr};
        QWidget* separatorWidget_{nullptr};

        QWidget* separator_{nullptr};
        TextWidget* nameUnit_{nullptr};
        TextWidget* statusUnit_{nullptr};
        TextWidget* lastSeenUnit_{nullptr};
        TextWidget* statusLifeTimeUnit_{nullptr};
        TextWidget* underNameUnit_{nullptr};

        TextWidget* descriptionUnit_{nullptr};

        bool loaded_{false};
        MiniProfileFlags flags_;

        bool clickAreaHovered_{false};
        bool clickAreaPressed_{false};
    };
}

Q_DECLARE_OPERATORS_FOR_FLAGS(Ui::MiniProfileFlags)
