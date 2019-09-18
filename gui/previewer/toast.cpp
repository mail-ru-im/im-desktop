#include "stdafx.h"

#include "utils/utils.h"
#include "utils/InterConnector.h"
#include "main_window/MainWindow.h"
#include "main_window/MainPage.h"
#include "main_window/ContactDialog.h"
#include "main_window/history_control/HistoryControlPage.h"
#include "main_window/input_widget/InputWidget.h"
#include "Drawable.h"
#include "fonts.h"
#include "../styles/ThemeParameters.h"

#ifdef __APPLE__
#include "utils/macos/mac_support.h"
#endif

#include "toast.h"

using namespace Ui;

namespace
{
    int toastHeight()
    {
        return Utils::scale_value(40);
    }

    int toastMaxHeight()
    {
        return Utils::scale_value(62);
    }

    int maxToastWidth()
    {
        return Utils::scale_value(348);
    }

    int maxDownloadToastWidth()
    {
        return Utils::scale_value(392);
    }

    int widthLimit()
    {
        return Utils::scale_value(400);
    }

    int padding()
    {
        return Utils::scale_value(8);
    }

    int animationDuration()
    {
        return 300;
    }

    int toastRadius()
    {
        return Utils::scale_value(8);
    }

    int toastVerPadding()
    {
        return Utils::scale_value(16);
    };

    int getToastShift(Ui::MainPage* _mainPage)
    {
        auto shift = 0;

        const auto frameCount = _mainPage->getFrameCount();
        if (frameCount == FrameCountMode::_3)
        {
            shift += _mainPage->getCLWidth() / 2;
            shift -= _mainPage->getSidebarWidth() / 2;
        }
        else if (frameCount == FrameCountMode::_2 && !_mainPage->isSidebarVisible())
        {
            shift += _mainPage->getCLWidth() / 2;
        }
        return shift;
    }

    QColor textColor()
    {
        return Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID_PERMANENT);
    }

    QColor pathColor()
    {
        return Styling::getParameters().getColor(Styling::StyleVariable::TEXT_PRIMARY);
    }

    QColor pathHoverColor()
    {
        return Styling::getParameters().getColor(Styling::StyleVariable::TEXT_PRIMARY_HOVER);
    }

    QFont getToastFont()
    {
        return Fonts::appFontScaled(15, Fonts::FontWeight::Normal);
    }
}

ToastBase::ToastBase(QWidget* _parent) : QWidget(_parent)
{
    const auto visibilityDuration = 4000;

    hideTimer_.setSingleShot(true);
    hideTimer_.setInterval(visibilityDuration);

    startHideTimer_.setSingleShot(true);
    startHideTimer_.setInterval(visibilityDuration - animationDuration());

    connect(&hideTimer_, &QTimer::timeout, this, &ToastBase::deleteLater);
    connect(&startHideTimer_, &QTimer::timeout, this, &ToastBase::startHide);

    setFixedHeight(toastHeight());

    if (!_parent)
    {
        setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);

        if constexpr (platform::is_linux())
            setWindowFlags(windowFlags() | Qt::BypassWindowManagerHint);
    }
    else
    {
        _parent->installEventFilter(this);
    }

    setAttribute(Qt::WA_TranslucentBackground);

    setMouseTracking(true);
}

void ToastBase::showAt(const QPoint& _center, bool _onTop)
{
    move(_center - QPoint(width() / 2, height() / 2));

#ifdef __APPLE__
    if (isWindow())
        MacSupport::showOverAll(this);
    else
        QWidget::show();
#else
    QWidget::show();
#endif

    raise();

    if (!_onTop)
        opacityAnimation_.start([this](){ update(); }, 0, 1, 300);

    moveAnimation_.start([this, _center, _onTop]()
    {
        move(_center.x() - width() / 2, moveAnimation_.current() - (_onTop ? 0 : height()));
    }, _center.y() + (_onTop ? 0 : height() + padding()), _center.y(), animationDuration());

    hideTimer_.start();
    startHideTimer_.start();
}

void ToastBase::paintEvent(QPaintEvent* _event)
{
    QPainter p(this);
    p.setRenderHint(QPainter::HighQualityAntialiasing);

    p.setOpacity(opacityAnimation_.current());

    QPainterPath path;
    path.addRoundedRect(rect(), toastRadius(), toastRadius());

    static auto backgroundColor = Styling::getParameters().getColor(Styling::StyleVariable::GHOST_PRIMARY);
    backgroundColor.setAlpha(0.7 * 255);
    p.fillPath(path, backgroundColor);

    drawContent(p);
}

void ToastBase::enterEvent(QEvent* _event)
{
    hideTimer_.stop();
    startHideTimer_.stop();
}

void ToastBase::leaveEvent(QEvent* _event)
{
    handleMouseLeave();
}

bool ToastBase::eventFilter(QObject* _object, QEvent* _event)
{
    if (_object == parent() && _event->type() == QEvent::Resize)
    {
        auto resizeEvent = static_cast<QResizeEvent*>(_event);
        handleParentResize(resizeEvent->size(), resizeEvent->oldSize());
    }

    return false;
}

void ToastBase::handleParentResize(const QSize& _newSize, const QSize& _oldSize)
{
    const auto oldHeight = height();
    updateSize();
    const auto newWidth = _newSize.width();

    const auto shift = shiftToastPos();
    const auto x = (newWidth - width()) / 2 + shift.x();
    const auto y = pos().y() + (_newSize.height() - _oldSize.height()) + (oldHeight - height()) + shift.y();

    move(x, y);
}

void ToastBase::handleMouseLeave()
{
    hideTimer_.start();
    startHideTimer_.start();
}

void ToastBase::startHide()
{
    opacityAnimation_.start([this](){ update(); }, 1, 0, 300);
}

ToastManager* ToastManager::instance()
{
    static ToastManager manager;

    return &manager;
}

void ToastManager::showToast(ToastBase* _toast, QPoint _pos)
{
    if (_toast->keepRows() && (bottomToast_ && bottomToast_->keepRows()))
    {
        if (_toast->sameSavePath(bottomToast_))
            return;
        moveToast();
    }
    else
    {
        hideToast();
    }

    bottomToast_ = _toast;
    bottomToast_->installEventFilter(this);
    _toast->showAt(_pos);

    if (topToast_)
    {
        const auto mainWindow = Utils::InterConnector::instance().getMainWindow();
        const auto inputHeight = Utils::InterConnector::instance().getContactDialog()->getInputWidget()->height();
        topToast_->showAt(QPoint(_pos.x(), mainWindow->height() - inputHeight - bottomToast_->height() - padding() - toastVerPadding()), true);
    }
}

bool ToastManager::eventFilter(QObject* _object, QEvent* _event)
{
    if (_object == bottomToast_ && _event->type() == QEvent::Resize)
    {
        if (topToast_)
        {
            auto resizeEvent = static_cast<QResizeEvent*>(_event);
            const auto delta = (resizeEvent->size().height() - resizeEvent->oldSize().height());
            topToast_->move(topToast_->geometry().x(), topToast_->geometry().y() - delta);
        }
    }
    return false;
}

void ToastManager::hideToast()
{
    if (topToast_)
    {
        topToast_->hide();
        topToast_->deleteLater();
    }

    if (bottomToast_)
    {
        bottomToast_->hide();
        bottomToast_->deleteLater();
    }
}

void ToastManager::moveToast()
{
    if (topToast_)
    {
        topToast_->hide();
        topToast_->deleteLater();
    }

    if (bottomToast_)
    {
        topToast_ = bottomToast_;
        bottomToast_->hide();
    }
}

Toast::Toast(const QString& _text, QWidget* _parent, int _maxLineCount) : ToastBase(_parent)
{
    const auto msgFont = getToastFont();

    textUnit_ = TextRendering::MakeTextUnit(_text, {}, TextRendering::LinksVisible::DONT_SHOW_LINKS);
    textUnit_->init(msgFont, textColor(), QColor(), QColor(), QColor(), Ui::TextRendering::HorAligment::CENTER, _maxLineCount, TextRendering::LineBreakType::PREFER_SPACES);

    updateSize();
}

void Toast::drawContent(QPainter& _p)
{
    textUnit_->draw(_p);
}

void Toast::updateSize()
{
    static const auto xOffset = Utils::scale_value(12);

    const auto parent = parentWidget();
    const auto textDesiredWidth = textUnit_->desiredWidth();
    const auto textWidth = parent ? qMin(textDesiredWidth, parent->width() - 2 * (xOffset + padding())) : textDesiredWidth;
    const auto textHeight = textUnit_->getHeight(textWidth);

    textUnit_->setOffsets(xOffset, (height() - textHeight) / 2);

    setFixedWidth(textWidth + 2 * xOffset);
}

class Ui::SavedPathToast_p
{
public:
    Ui::TextRendering::TextUnitPtr messageUnit1_;
    Ui::TextRendering::TextUnitPtr messageUnit2_;

    std::unique_ptr<Label> pathLabel_;

    QString path_;
};

SavedPathToast::SavedPathToast(const QString& _path, QWidget* _parent)
    : ToastBase(_parent)
    , d(std::make_unique<SavedPathToast_p>())
{
    d->path_ = _path;

    const auto msgFont = getToastFont();
    const auto msgFontColor = textColor();
    const auto pathFontColor = pathColor();

    static const auto xOffset = Utils::scale_value(12);

    static QString pathPlaceholder = qsl("%PATH%");

    auto text = QT_TRANSLATE_NOOP("previewer", "Saved to %1").arg(pathPlaceholder);

    auto placeholderPos = text.indexOf(pathPlaceholder);

    auto firstPart = text.left(placeholderPos);
    auto secondPart = text.mid(placeholderPos + pathPlaceholder.size());
    if (!secondPart.isEmpty())
        secondPart.prepend(ql1c(' '));

    auto totalWidth = xOffset;

    d->messageUnit1_ = Ui::TextRendering::MakeTextUnit(firstPart);
    d->messageUnit1_->init(msgFont, msgFontColor, QColor(), QColor(), QColor(), Ui::TextRendering::HorAligment::CENTER, 1);
    const auto yOffset = (toastHeight() - d->messageUnit1_->getHeight(d->messageUnit1_->desiredWidth())) / 2;
    d->messageUnit1_->setOffsets(xOffset, yOffset);

    totalWidth += d->messageUnit1_->desiredWidth();

    const auto pathText = QDir::toNativeSeparators(QFileInfo(_path).absoluteDir().absolutePath());
    auto pathUnit = Ui::TextRendering::MakeTextUnit(pathText, Data::MentionMap(), Ui::TextRendering::LinksVisible::DONT_SHOW_LINKS);
    pathUnit->init(msgFont, pathFontColor, QColor(), QColor(), QColor(), Ui::TextRendering::HorAligment::CENTER, 1);
    auto pathWidth = pathUnit->desiredWidth();

    d->pathLabel_ = std::make_unique<Label>();
    d->pathLabel_->setTextUnit(std::move(pathUnit));
    d->pathLabel_->setYOffset(yOffset);
    d->pathLabel_->setRect(QRect(totalWidth, 0, pathWidth, height()));
    d->pathLabel_->setDefaultColor(pathFontColor);
    d->pathLabel_->setHoveredColor(pathHoverColor());

    totalWidth += pathWidth;

    d->messageUnit2_ = Ui::TextRendering::MakeTextUnit(secondPart);
    d->messageUnit2_->init(msgFont, msgFontColor, QColor(), QColor(), QColor(), Ui::TextRendering::HorAligment::CENTER, 1);

    if ((totalWidth + d->messageUnit2_->desiredWidth() + xOffset) > maxToastWidth())
    {
        totalWidth -= pathWidth;
        auto maxPathWidth = maxToastWidth() - (d->messageUnit1_->desiredWidth() + d->messageUnit2_->desiredWidth() + 2 * xOffset + 1);

        QFontMetrics fm(msgFont);
        d->pathLabel_->setText(fm.elidedText(pathText, Qt::ElideLeft, maxPathWidth));

        auto pathRect = d->pathLabel_->rect();
        pathRect.setWidth(maxPathWidth);
        d->pathLabel_->setRect(pathRect);

        totalWidth += maxPathWidth;
    }

    d->messageUnit2_->setOffsets(totalWidth, yOffset);
    d->messageUnit2_->evaluateDesiredSize();
    totalWidth += d->messageUnit2_->desiredWidth() + xOffset;

    d->pathLabel_->setUnderline(true);

    setFixedWidth(totalWidth);

    setMouseTracking(true);
}

void SavedPathToast::drawContent(QPainter& _p)
{
    d->messageUnit1_->draw(_p);
    d->messageUnit2_->draw(_p);
    d->pathLabel_->draw(_p);
}

void SavedPathToast::mouseMoveEvent(QMouseEvent* _event)
{
    const auto mouseOver = d->pathLabel_->rect().contains( _event->pos());

    d->pathLabel_->setUnderline(!mouseOver);
    d->pathLabel_->setHovered(mouseOver);

    setCursor(mouseOver ? Qt::PointingHandCursor : Qt::ArrowCursor);
    update();

    QWidget::mouseMoveEvent(_event);
}

void SavedPathToast::mousePressEvent(QMouseEvent* _event)
{
    const auto mouseOver = d->pathLabel_->rect().contains( _event->pos());
    if (mouseOver)
    {
        d->pathLabel_->setPressed(mouseOver);
        update();
    }
}

void SavedPathToast::mouseReleaseEvent(QMouseEvent* _event)
{
    auto click = false;
    if (d->pathLabel_->rect().contains(_event->pos()) && d->pathLabel_->pressed())
        click = true;

    d->pathLabel_->setPressed(false);

    if (click)
    {
        handleMouseLeave();
        Utils::openFileLocation(d->path_);
        Utils::InterConnector::instance().getMainWindow()->closeGallery();
    }
    update();

}

void SavedPathToast::leaveEvent(QEvent* _event)
{
    d->pathLabel_->setUnderline(true);
    d->pathLabel_->setHovered(false);
    update();

    ToastBase::leaveEvent(_event);
}

void Utils::showToastOverMainWindow(const QString &_text, int _bottomOffset, int _maxLineCount)
{
    const auto mainWindow = Utils::InterConnector::instance().getMainWindow();
    if (mainWindow)
    {
        auto toast = new Ui::Toast(_text, mainWindow, _maxLineCount);
        Ui::ToastManager::instance()->showToast(toast, QPoint(mainWindow->width() / 2, mainWindow->height() - toast->height() - _bottomOffset));
    }
}

void Utils::showDownloadToast(const Data::FileSharingDownloadResult& _result)
{
    const auto mainPage = Utils::InterConnector::instance().getMainPage();
    const auto mainWindow = Utils::InterConnector::instance().getMainWindow();
    const auto inputHeight = Utils::InterConnector::instance().getContactDialog()->getInputWidget()->height();
    const auto toast = new Ui::DownloadFinishedToast(_result, mainWindow);

    Ui::ToastManager::instance()->showToast(toast, QPoint(mainWindow->width()/ 2 + getToastShift(mainPage), mainWindow->height() - inputHeight - toastVerPadding()));
}

class Ui::DownloadFinishedToast_p
{
public:
    Ui::TextRendering::TextUnitPtr messageUnit1_;
    Ui::TextRendering::TextUnitPtr messageUnit2_;
    Ui::TextRendering::TextUnitPtr messageUnit3_;

    std::unique_ptr<Label> pathLabel_;
    std::unique_ptr<Label> fileLabel_;

    QString path_;
    QString folder_;
    QString file_;
};

DownloadFinishedToast::DownloadFinishedToast(const Data::FileSharingDownloadResult& _result, QWidget * _parent)
    : ToastBase(_parent)
    , d(std::make_unique<DownloadFinishedToast_p>())
    , xOffset_(0)
    , yOffset_(0)
    , fileWidth_(0)
    , pathWidth_(0)
    , totalWidth_(0)
{
    setWindowFlags(windowFlags() & (~Qt::WindowStaysOnTopHint));

    const auto fullPath = _result.filename_;
    QDir dir(fullPath);
    d->path_ = dir.absolutePath();

    const auto msgFont = getToastFont();
    const auto msgFontColor = textColor();
    const auto pathFontColor = pathColor();

    xOffset_ = Utils::scale_value(12);

    static QString filePlaceholder = qsl("%FILE%");
    static QString pathPlaceholder = qsl("%PATH%");

    auto text = QT_TRANSLATE_NOOP("previewer", "File %1 downloaded to %2").arg(filePlaceholder).arg(pathPlaceholder);

    auto firstPlaceholderPos = text.indexOf(filePlaceholder);
    auto secondPlaceholderPos = text.indexOf(pathPlaceholder);

    auto firstPart = text.left(firstPlaceholderPos);
    auto secondPart = text.left(secondPlaceholderPos).mid(firstPlaceholderPos + filePlaceholder.size());
    auto thirdPart = text.mid(secondPlaceholderPos + pathPlaceholder.size());
    if (!thirdPart.isEmpty())
        thirdPart.prepend(ql1c(' '));

    totalWidth_ = xOffset_;

    d->messageUnit1_ = Ui::TextRendering::MakeTextUnit(firstPart);
    d->messageUnit1_->init(msgFont, msgFontColor, QColor(), QColor(), QColor(), Ui::TextRendering::HorAligment::CENTER, 1);

    d->messageUnit2_ = Ui::TextRendering::MakeTextUnit(secondPart);
    d->messageUnit2_->init(msgFont, msgFontColor, QColor(), QColor(), QColor(), Ui::TextRendering::HorAligment::CENTER, 1);

    d->messageUnit3_ = Ui::TextRendering::MakeTextUnit(thirdPart);
    d->messageUnit3_->init(msgFont, msgFontColor, QColor(), QColor(), QColor(), Ui::TextRendering::HorAligment::CENTER, 1);

    const auto lineHeight = (firstPart.isEmpty()
            ? d->messageUnit2_->getHeight(d->messageUnit2_->desiredWidth())
            : d->messageUnit1_->getHeight(d->messageUnit1_->desiredWidth()));

    yOffset_ = (toastHeight() - lineHeight) / 2;
    d->messageUnit1_->setOffsets(xOffset_, yOffset_);

    totalWidth_ += d->messageUnit1_->desiredWidth();

    const auto absolutePath = QDir::toNativeSeparators(QFileInfo(fullPath).absoluteDir().absolutePath());
    d->folder_ = QDir(absolutePath).dirName();
    d->file_ = dir.dirName();

    auto fileUnit = Ui::TextRendering::MakeTextUnit(d->file_, Data::MentionMap(), Ui::TextRendering::LinksVisible::DONT_SHOW_LINKS);
    fileUnit->init(msgFont, pathFontColor, QColor(), QColor(), QColor(), Ui::TextRendering::HorAligment::CENTER, 1);
    fileWidth_ = fileUnit->desiredWidth();

    d->fileLabel_ = std::make_unique<Label>();
    d->fileLabel_->setTextUnit(std::move(fileUnit));
    d->fileLabel_->setYOffset(yOffset_);
    d->fileLabel_->setRect(QRect(totalWidth_, 0, fileWidth_, height()));
    d->fileLabel_->setDefaultColor(pathFontColor);
    d->fileLabel_->setHoveredColor(pathHoverColor());

    totalWidth_ += fileWidth_;

    auto pathUnit = Ui::TextRendering::MakeTextUnit(d->folder_, Data::MentionMap(), Ui::TextRendering::LinksVisible::DONT_SHOW_LINKS);
    pathUnit->init(msgFont, pathFontColor, QColor(), QColor(), QColor(), Ui::TextRendering::HorAligment::CENTER, 1);
    pathWidth_ = pathUnit->desiredWidth();
    d->pathLabel_ = std::make_unique<Label>();
    d->pathLabel_->setTextUnit(std::move(pathUnit));
    d->pathLabel_->setDefaultColor(pathFontColor);
    d->pathLabel_->setHoveredColor(pathHoverColor());

    updateSize();
    setMouseTracking(true);
}

DownloadFinishedToast::~DownloadFinishedToast()
{
}

void DownloadFinishedToast::updateSize()
{
    const auto pageWidth = Utils::InterConnector::instance().getContactDialog()->width();
    bool isMinimal = pageWidth < widthLimit();
    const auto maxWidth = isMinimal ? std::min(maxToastWidth(), pageWidth) : maxDownloadToastWidth();
    const auto currHeight = isMinimal ? toastMaxHeight() : toastHeight();
    const auto msgFont = getToastFont();
    QFontMetrics fm(msgFont);
    fileWidth_ = d->fileLabel_->desiredWidth();
    pathWidth_ = d->pathLabel_->desiredWidth();
    const auto lineHeight = (d->messageUnit1_->getText().isEmpty()
                             ? d->messageUnit2_->getHeight(d->messageUnit2_->desiredWidth())
                             : d->messageUnit1_->getHeight(d->messageUnit1_->desiredWidth()));

    yOffset_ = (toastHeight() - lineHeight) / 2;
    d->messageUnit1_->setOffsets(xOffset_, yOffset_);
    d->fileLabel_->setYOffset(yOffset_);
    d->fileLabel_->setRect(QRect(xOffset_ + d->messageUnit1_->desiredWidth(), 0, fileWidth_, height()));

    setFixedHeight(currHeight);
    totalWidth_ = xOffset_ + d->messageUnit1_->desiredWidth() + fileWidth_;
    compose(isMinimal, maxWidth, fm, fileWidth_, pathWidth_, totalWidth_);
}

QPoint DownloadFinishedToast::shiftToastPos()
{
    return QPoint(getToastShift(Utils::InterConnector::instance().getMainPage()), 0);
}

void DownloadFinishedToast::compose(bool _isMinimal, const int maxWidth, const QFontMetrics fm, int fileWidth, int pathWidth, int _totalWidth)
{
    if (!d->messageUnit2_->getText().startsWith(ql1c(' ')))
        d->messageUnit2_->setText(d->messageUnit2_->getText().prepend(ql1c(' ')));

    const auto textWidth1 = d->messageUnit1_->desiredWidth();
    const auto firstLineHeight = (d->messageUnit1_->getText().isEmpty()
                                  ? d->messageUnit2_->getHeight(d->messageUnit2_->desiredWidth())
                                  : d->messageUnit1_->getHeight(d->messageUnit1_->desiredWidth()));
    auto textWidth2 = d->messageUnit2_->desiredWidth();
    const auto textWidth3 = d->messageUnit3_->desiredWidth();
    const auto fullTextWidth = textWidth1 + textWidth2 + textWidth3 + 2 * xOffset_;
    const auto halfSpace = (maxWidth - fullTextWidth) / 2;

    const auto fileText = d->file_;
    const auto pathText = d->folder_;

    if (fileText != d->fileLabel_->getText())
    {
        d->fileLabel_->setText(fileText);
        _totalWidth -= fileWidth;
        d->fileLabel_->getHeight(d->fileLabel_->desiredWidth());
        fileWidth = d->fileLabel_->desiredWidth();
        _totalWidth += fileWidth;
    }
    if (pathText != d->pathLabel_->getText())
    {
        d->pathLabel_->setText(pathText);
        d->pathLabel_->getHeight(d->pathLabel_->desiredWidth());
        pathWidth = d->pathLabel_->desiredWidth();
    }

    if (fileWidth + pathWidth + fullTextWidth <= maxWidth)
    {
        d->messageUnit2_->setOffsets(_totalWidth, yOffset_);
        d->messageUnit2_->evaluateDesiredSize();
        _totalWidth += textWidth2;

        d->pathLabel_->setRect(QRect(_totalWidth, yOffset_, pathWidth, d->pathLabel_->getHeight(pathWidth)));
        _totalWidth += pathWidth;

        d->messageUnit3_->setOffsets(_totalWidth, yOffset_);
        d->messageUnit3_->evaluateDesiredSize();
        _totalWidth += textWidth3;

        setFixedHeight(toastHeight());
    }
    else
    {
        if (_isMinimal)
        {
            if (_totalWidth + textWidth2 + xOffset_ <= maxWidth)
            {
                d->messageUnit2_->setOffsets(_totalWidth, yOffset_);
                d->messageUnit2_->evaluateDesiredSize();
                _totalWidth += d->messageUnit2_->desiredWidth();

                if (pathWidth + textWidth3 + 2 * xOffset_ > maxWidth)
                {
                    const auto maxPathWidth = std::min(_totalWidth, maxWidth  - textWidth3 - 2 * xOffset_);
                    d->pathLabel_->setText(fm.elidedText(pathText.trimmed(), Qt::ElideLeft, maxPathWidth));
                    pathWidth = maxPathWidth;
                }

                d->pathLabel_->setRect(QRect(xOffset_, firstLineHeight + yOffset_, pathWidth, d->pathLabel_->getHeight(pathWidth)));

                d->messageUnit3_->setOffsets(xOffset_ + pathWidth, firstLineHeight + yOffset_);
                d->messageUnit3_->evaluateDesiredSize();
            }
            else
            {
                if (_totalWidth + xOffset_ > maxWidth)
                {
                    _totalWidth -= fileWidth;
                    auto maxFilePathWidth = maxWidth - (textWidth1 + 2 * xOffset_ + 1);
                    if (maxFilePathWidth <= 0)
                        maxFilePathWidth = std::min(fileWidth, halfSpace);

                    d->fileLabel_->setText(fm.elidedText(fileText.trimmed(), Qt::ElideMiddle, maxFilePathWidth));

                    auto fileRect = d->fileLabel_->rect();
                    fileRect.setWidth(maxFilePathWidth);
                    d->fileLabel_->setRect(fileRect);

                    _totalWidth += maxFilePathWidth;
                }

                if ((textWidth2 + textWidth3 + pathWidth + xOffset_) > maxWidth)
                {
                    auto maxPathWidth = maxWidth - (textWidth2 + textWidth3 + 2 * xOffset_ + 1);

                    d->pathLabel_->setText(fm.elidedText(pathText.trimmed(), Qt::ElideLeft, maxPathWidth));
                    pathWidth = maxPathWidth;
                }

                if (d->messageUnit2_->getText().startsWith(ql1c(' ')))
                    d->messageUnit2_->setText(d->messageUnit2_->getText().remove(0, 1));

                d->messageUnit2_->setOffsets(xOffset_, firstLineHeight + yOffset_);
                d->messageUnit2_->evaluateDesiredSize();
                textWidth2 = d->messageUnit2_->desiredWidth();

                d->pathLabel_->setRect(QRect(xOffset_ + textWidth2, firstLineHeight + yOffset_, pathWidth, d->pathLabel_->getHeight(pathWidth)));

                d->messageUnit3_->setOffsets(xOffset_ + pathWidth + textWidth2, firstLineHeight + yOffset_);
                d->messageUnit3_->evaluateDesiredSize();
            }
        }
        else
        {
            const auto potentialPathWidth = pathWidth > maxWidth ? halfSpace : pathWidth;
            if ((_totalWidth + textWidth2 + textWidth3 + potentialPathWidth + xOffset_) > maxWidth)
            {
                _totalWidth -= fileWidth;
                auto maxFilePathWidth = maxWidth - (fullTextWidth + potentialPathWidth + 1);
                if (maxFilePathWidth <= 0)
                    maxFilePathWidth = std::min(fileWidth, halfSpace);

                d->fileLabel_->setText(fm.elidedText(fileText.trimmed(), Qt::ElideMiddle, maxFilePathWidth));

                auto fileRect = d->fileLabel_->rect();
                fileRect.setWidth(maxFilePathWidth);
                d->fileLabel_->setRect(fileRect);

                _totalWidth += maxFilePathWidth;
            }

            if ((_totalWidth + textWidth2 + textWidth3 + pathWidth + xOffset_) > maxWidth)
            {

                auto maxFilePathWidth = maxWidth - (_totalWidth + textWidth2 + textWidth3 + 2 * xOffset_ + 1);
                d->pathLabel_->setText(fm.elidedText(pathText.trimmed(), Qt::ElideLeft, maxFilePathWidth));
                pathWidth = maxFilePathWidth;
            }

            d->messageUnit2_->setOffsets(_totalWidth, yOffset_);
            d->messageUnit2_->evaluateDesiredSize();
            _totalWidth += textWidth2;

            d->pathLabel_->setRect(QRect(_totalWidth, yOffset_, pathWidth, d->pathLabel_->getHeight(pathWidth)));
            _totalWidth += pathWidth;

            d->messageUnit3_->setOffsets(_totalWidth, yOffset_);
            d->messageUnit3_->evaluateDesiredSize();
            _totalWidth += textWidth3;
        }
    }

    d->pathLabel_->setUnderline(true);
    d->fileLabel_->setUnderline(true);

    _totalWidth += xOffset_;

    auto toastWidth = _totalWidth;
    if (_isMinimal)
    {
        auto secondLineWidth = textWidth3 + pathWidth + 2 * xOffset_;
        if (d->messageUnit2_->verOffset() != yOffset_)
            secondLineWidth += textWidth2;
        toastWidth = std::max(toastWidth, secondLineWidth);
    }
    setFixedWidth(std::min(toastWidth, maxWidth));
};

QString Ui::DownloadFinishedToast::getPath() const
{
    return d->path_;
}

bool Ui::DownloadFinishedToast::sameSavePath(const ToastBase * _other) const
{
    return _other != nullptr && getPath() == _other->getPath();
}

void DownloadFinishedToast::drawContent(QPainter & _p)
{
    d->messageUnit1_->draw(_p);
    d->fileLabel_->draw(_p);
    d->messageUnit2_->draw(_p);
    d->pathLabel_->draw(_p);
    d->messageUnit3_->draw(_p);
}

void DownloadFinishedToast::mouseMoveEvent(QMouseEvent * _event)
{
    const auto mouseOverPath = d->pathLabel_->rect().contains(_event->pos());
    const auto mouseOverFile = d->fileLabel_->rect().contains(_event->pos());

    d->pathLabel_->setUnderline(!mouseOverPath);
    d->fileLabel_->setUnderline(!mouseOverFile);
    d->pathLabel_->setHovered(mouseOverPath);
    d->fileLabel_->setHovered(mouseOverFile);

    setCursor((mouseOverPath || mouseOverFile) ? Qt::PointingHandCursor : Qt::ArrowCursor);
    update();

    QWidget::mouseMoveEvent(_event);
}

void DownloadFinishedToast::mousePressEvent(QMouseEvent * _event)
{
    const auto mouseOverPath = d->pathLabel_->rect().contains(_event->pos());
    const auto mouseOverFile = !mouseOverPath && d->fileLabel_->rect().contains(_event->pos());

    if (mouseOverPath || mouseOverFile)
    {
        d->pathLabel_->setPressed(mouseOverPath);
        d->fileLabel_->setPressed(mouseOverFile);
        setCursor(Qt::PointingHandCursor);
        update();
    }
}

void DownloadFinishedToast::mouseReleaseEvent(QMouseEvent * _event)
{
    auto clickPath = false;
    auto clickFile = false;

    if (d->pathLabel_->rect().contains(_event->pos()) && d->pathLabel_->pressed())
        clickPath = true;

    if (!clickPath && d->fileLabel_->rect().contains(_event->pos()) && d->fileLabel_->pressed())
        clickFile = true;

    d->pathLabel_->setPressed(false);
    d->fileLabel_->setPressed(false);

    if (clickPath || clickFile)
    {
        handleMouseLeave();
        Utils::openFileOrFolder(QStringRef(&d->path_), clickPath ? Utils::OpenAt::Folder : Utils::OpenAt::Launcher);
        Utils::InterConnector::instance().getMainWindow()->closeGallery();
    }
    update();
}

void DownloadFinishedToast::leaveEvent(QEvent * _event)
{
    d->pathLabel_->setUnderline(true);
    d->fileLabel_->setUnderline(true);
    d->pathLabel_->setHovered(false);
    d->fileLabel_->setHovered(false);
    update();

    ToastBase::leaveEvent(_event);
}
