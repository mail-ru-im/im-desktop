#pragma once

namespace Styling
{
    class BaseStyleSheetGenerator;

    enum class StyleSheetPolicy
    {
        UseSetStyleSheet,
        ApplyFontsAndScale
    };

    class StyleSheetContainer : public QObject
    {
        Q_OBJECT

    public:
        StyleSheetContainer(QWidget* _styleTarget);
        ~StyleSheetContainer();

        void setGenerator(std::unique_ptr<BaseStyleSheetGenerator> _styleSheetGenerator);
        void usePolicy(StyleSheetPolicy _policy);

        bool eventFilter(QObject* _object, QEvent* _event) override;

    private:
        void updateStyle();

    private Q_SLOTS:
        void markNeedUpdateStyle();

    private:
        QWidget* styleTarget_;
        std::unique_ptr<BaseStyleSheetGenerator> styleSheetGenerator_;
        StyleSheetPolicy policy_ = StyleSheetPolicy::ApplyFontsAndScale;
        bool needUpdateStyle_ = false;
    };

    void setStyleSheet(QWidget* _styleTarget, std::unique_ptr<BaseStyleSheetGenerator> _styleSheetGenerator, StyleSheetPolicy _policy = StyleSheetPolicy::ApplyFontsAndScale);

} // namespace Styling
