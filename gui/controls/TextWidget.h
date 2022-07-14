#pragma once

#include "TextUnit.h"

namespace Ui
{
    class TextWidget : public QWidget
    {
        Q_OBJECT

    Q_SIGNALS:
        void linkActivated(const QString& _link, QPrivateSignal) const;

    public:
        template<typename... Args>
        TextWidget(QWidget* _parent, Args&&... args)
            : QWidget(_parent)
            , text_(TextRendering::MakeTextUnit(std::forward<Args>(args)...))
        {
            if (text_->linksVisibility() == TextRendering::LinksVisible::SHOW_LINKS)
                setMouseTracking(true);
        }

        void init(const TextRendering::TextUnit::InitializeParameters& _params);

        void setVerticalPosition(TextRendering::VerPosition _position);

        void applyLinks(const std::map<QString, QString>& _links);

        void setLineSpacing(int _v);

        int getDesiredWidth() const noexcept { return desiredWidth_; }

        void setMaxWidth(int _width);
        void setMaxWidthAndResize(int _width);

        void setText(const Data::FString& _text, const Styling::ThemeColorKey& _color = {});
        void setOpacity(qreal _opacity);
        void setColor(const Styling::ThemeColorKey& _color);
        void setAlignment(const TextRendering::HorAligment _align);

        QString getText() const;
        Data::FString getSelectedText(TextRendering::TextType _type) const;

        void elide(int _width);
        bool isElided() const;
        bool isEmpty() const;

        bool isSelected() const;
        bool isAllSelected() const;
        void select(const QPoint& _from, const QPoint& _to);
        void selectAll();
        void clearSelection();
        void releaseSelection();

        void clicked(const QPoint& _p);
        void doubleClicked(const QPoint& _p, bool _fixSelection, std::function<void(bool)> _callback);

    protected:
        void paintEvent(QPaintEvent* _e) override;
        void mouseMoveEvent(QMouseEvent* _e) override;
        void mouseReleaseEvent(QMouseEvent* _e) override;

    private:
        TextRendering::TextUnitPtr text_;
        qreal opacity_ = 1.0;
        int maxWidth_ = 0;
        int desiredWidth_ = 0;
        TextRendering::VerPosition verticalPosition_ = TextRendering::VerPosition::TOP;
    };
} // namespace Ui
