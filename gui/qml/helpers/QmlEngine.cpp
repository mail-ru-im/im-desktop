#include "stdafx.h"
#include "QmlEngine.h"

#include "../../styles/StyleVariable.h"
#include "../../styles/ThemesContainer.h"
#include "../../fonts.h"

#include "qmlstyling.h"
#include "qmlutils.h"
#include "qmlfonts.h"
#include "qmltesting.h"

#include "../models/RootModel.h"

#include "../components/SvgIcon.h"
#include "../components/CursorHandler.h"
#include "../components/WheelInverter.h"

Qml::Engine::Engine(QObject* _parent)
    : QQmlEngine(_parent)
    , rootModel_{ new RootModel(this) }
{
    registerHelpers();
    registerModels();
    registerComponents();
}

Qml::RootModel* Qml::Engine::rootModel() const
{
    return rootModel_;
}

void Qml::Engine::addQuickWidget(QQuickWidget* _widget)
{
    if (Q_UNLIKELY(!_widget))
        return;

    const auto status = _widget->status();
    switch (status)
    {
    case QQuickWidget::Status::Ready:
        addRootQuickItem(_widget->rootObject());
        [[fallthrough]];
    case QQuickWidget::Status::Error:
        _widget->disconnect(this);
        break;
    case QQuickWidget::Status::Loading:
    case QQuickWidget::Status::Null:
        connect(_widget, &QQuickWidget::statusChanged, this, &Engine::onQuickWidgetStatusChanged, Qt::UniqueConnection);
    }
}

QVector<QQuickItem*> Qml::Engine::rootQuickItems() const
{
    return rootItems_;
}

void Qml::Engine::addRootQuickItem(QQuickItem* _item)
{
    if (_item)
        rootItems_.push_back(_item);
}

void Qml::Engine::registerHelpers()
{
    auto context = rootContext();

    qmlRegisterUncreatableType<Styling::StylingParams>("Im.Styles", 1, 0, "StyleVariable", qsl("Access to StyleVariable enum only"));

    registerStyleHelper();
    connect(&Styling::getThemesContainer(), &Styling::ThemesContainer::globalThemeChanged, this, &Engine::registerStyleHelper);

    auto utils = new Utils::QmlHelper(this);
    context->setContextProperty(qsl("Utils"), utils);

    qmlRegisterUncreatableType<Fonts::FontFamilyParams>("Im.Fonts", 1, 0, "FontFamily", qsl("Access to FontFamily enum only"));
    qmlRegisterUncreatableType<Fonts::FontWeightParams>("Im.Fonts", 1, 0, "FontWeight", qsl("Access to FontWeight enum only"));

    auto fonts = new Fonts::QmlAppFontHelper(this);
    context->setContextProperty(qsl("Fonts"), fonts);

    auto testing = new Testing::QmlHelper(this);
    context->setContextProperty(qsl("Testing"), testing);
}

void Qml::Engine::registerModels()
{
    auto context = rootContext();
    context->setContextProperty(qsl("im"), rootModel_);
}

void Qml::Engine::registerComponents()
{
    qmlRegisterType<SvgIcon>("Im.Components", 1, 0, "SvgIcon");
    qmlRegisterType<CursorHandler>("Im.Components", 1, 0, "CursorHandler");
    qmlRegisterType<WheelInverter>("Im.Components", 1, 0, "WheelInverter");
}

void Qml::Engine::onQuickWidgetStatusChanged()
{
    const auto widget = qobject_cast<QQuickWidget*>(sender());
    addQuickWidget(widget);
}

void Qml::Engine::registerStyleHelper()
{
    auto context = rootContext();
    auto style = new Styling::QmlHelper(this);
    context->setContextProperty(qsl("Style"), style);
    if (styleHelper_)
        styleHelper_->deleteLater();
    styleHelper_ =  style;
}
