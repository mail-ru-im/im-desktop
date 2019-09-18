#include "stdafx.h"
#include "ContactAvatarWidget.h"

#include "AvatarPreview.h"

#include "GeneralDialog.h"
#include "ImageCropper.h"
#include "DialogButton.h"
#include "../core_dispatcher.h"
#include "../my_info.h"
#include "../cache/avatars/AvatarStorage.h"
#include "../main_window/MainWindow.h"
#include "../main_window/friendly/FriendlyContainer.h"
#include "../main_window/Placeholders.h"
#include "../main_window/contact_list/ContactListModel.h"
#include "../utils/InterConnector.h"
#include "../utils/utils.h"
#include "../styles/ThemeParameters.h"

namespace
{
    const auto MIN_AVATAR_SIZE = 200;
    const auto AVATAR_CROP_SIZE = 1024;
    const int ADD_PHOTO_FONTSIZE = 24;
    const int CHANGE_ICON_SIZE = 32;

    QByteArray processImage(const QPixmap &_avatar)
    {
        auto avatar = _avatar;
        if (std::max(avatar.width(), avatar.height()) > AVATAR_CROP_SIZE)
        {
            if (avatar.width() > avatar.height())
                avatar = avatar.scaledToWidth(AVATAR_CROP_SIZE, Qt::SmoothTransformation);
            else
                avatar = avatar.scaledToHeight(AVATAR_CROP_SIZE, Qt::SmoothTransformation);
        }

        auto quality = 75; // 75 is a default quality for jpeg
        QByteArray result;
        do                 // if result image is too big (which is not supposed to happen), try smaller quality values
        {
            result.clear();
            QBuffer buffer(&result);
            avatar.save(&buffer, "jpg", quality);
            if (quality > 0)
                quality -= 10;
            else
                avatar = avatar.scaled(avatar.size() / 2, Qt::KeepAspectRatio, Qt::SmoothTransformation); // should never happen, only for loop to not be potentially infinite
        }
        while (result.size() > 8 * 1024 * 1024);

        return result;
    }

    QSize getCamIconSize() noexcept
    {
        return Utils::scale_value(QSize(32, 32));
    }
}

namespace Ui
{
    ContactAvatarWidget::ContactAvatarWidget(QWidget* _parent, const QString& _aimid, const QString& _displayName, const int _size, const bool _autoUpdate, const bool _officialOnly, const bool _isVisibleBadge)
        :  QWidget(_parent)
        , size_(_size)
        , aimid_(_aimid)
        , displayName_(_displayName)
        , imageCropHolder_(nullptr)
        , imageCropSize_(QSize())
        , mode_(Mode::Common)
        , isVisibleShadow_(false)
        , isVisibleOutline_(false)
        , isVisibleBadge_(_isVisibleBadge)
        , connected_(false)
        , seq_(-1)
        , defaultAvatar_(true)
        , officialBadgeOnly_(_officialOnly)
        , spinner_(nullptr)
        , hovered_(false)
        , pressed_(false)
    {
        setFixedSize(Utils::avatarWithBadgeSize(_size));

        if (_autoUpdate)
        {
            connect(Logic::GetAvatarStorage(), &Logic::AvatarStorage::avatarChanged, this, &ContactAvatarWidget::avatarChanged);
            connect(Logic::getContactListModel(), &Logic::ContactListModel::contactChanged, this, &ContactAvatarWidget::avatarChanged);
            connect(Logic::GetFriendlyContainer(), &Logic::FriendlyContainer::friendlyChanged, this, [this](const QString& _aimid, const QString& _friendly)
            {
                if (_aimid == aimid_)
                {
                    Logic::GetAvatarStorage()->UpdateDefaultAvatarIfNeed(aimid_);
                    UpdateParams(aimid_, _friendly);
                }
            });
        }

        infoForSetAvatar_.currentDirectory = QDir::homePath();
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::setAvatar, this, &ContactAvatarWidget::setAvatar, Qt::QueuedConnection);

        connect(this, &ContactAvatarWidget::summonSelectFileForAvatar, this, &ContactAvatarWidget::selectFileForAvatar, Qt::QueuedConnection);
    }

    ContactAvatarWidget::~ContactAvatarWidget()
    {
    }

    QString ContactAvatarWidget::GetState()
    {
        if (mode_ == Mode::MyProfile || mode_ == Mode::Introduce)
        {
            return isVisibleShadow_ ? qsl("photo enter") : qsl("photo leave");
        }
        return QString();
    }

    void ContactAvatarWidget::paintEvent(QPaintEvent* _e)
    {
        QPainter p(this);
        if (aimid_.isEmpty() && displayName_.isEmpty() && (!infoForSetAvatar_.croppedImage.isNull() || !infoForSetAvatar_.roundedCroppedImage.isNull()))
        {
            p.setRenderHint(QPainter::Antialiasing);
            p.setRenderHint(QPainter::SmoothPixmapTransform);
            if (infoForSetAvatar_.roundedCroppedImage.isNull())
            {
                auto scaled = infoForSetAvatar_.croppedImage.scaled(QSize(size_, size_), Qt::KeepAspectRatio, Qt::SmoothTransformation);
                infoForSetAvatar_.roundedCroppedImage = Utils::roundImage(scaled, QString(), false, false);
            }
            p.drawPixmap(0, 0, size_, size_, infoForSetAvatar_.roundedCroppedImage);
            return QWidget::paintEvent(_e);
        }

        const auto &scAvatar = Logic::GetAvatarStorage()->GetRounded(aimid_, displayName_, isVisibleOutline_ ? Utils::scale_bitmap(size_ - Utils::scale_value(4)) : Utils::scale_bitmap(size_), GetState(), defaultAvatar_, false, false);

        if (scAvatar->isNull())
            return;

        QPixmap avatar = *scAvatar;

        if ((mode_ == Mode::MyProfile || mode_ == Mode::Introduce) && hovered_ && !defaultAvatar_)
        {
            static const auto icon = Utils::renderSvg(qsl(":/cam_icon"), getCamIconSize(), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID_PERMANENT));

            QPainter pAvatar(&avatar);
            QPainterPath path;
            path.addEllipse(QRectF(0, 0, size_, size_));

            pAvatar.setPen(Qt::NoPen);
            pAvatar.setRenderHint(QPainter::Antialiasing);
            pAvatar.setRenderHint(QPainter::SmoothPixmapTransform);
            pAvatar.fillPath(path, Styling::getParameters().getColor(Styling::StyleVariable::GHOST_SECONDARY));

            const auto x = (width() - icon.width() / Utils::scale_bitmap_ratio()) / 2;
            const auto y = (height() - icon.height() / Utils::scale_bitmap_ratio()) / 2;
            pAvatar.drawPixmap(x, y, icon);
        }

        if (isVisibleOutline_)
        {
            p.setRenderHint(QPainter::Antialiasing);
            QPen pen;
            pen.setBrush(Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY));
            pen.setWidth(Utils::scale_value(4));
            p.setPen(pen);
            p.drawEllipse(QPointF(size_ / 2, size_ / 2), size_ / 2 - Utils::scale_value(2), size_ / 2 - Utils::scale_value(2));
        }
        if ((mode_ == Mode::MyProfile || mode_ == Mode::Introduce) && defaultAvatar_)
        {
            p.setPen(Qt::NoPen);
            p.setBrush(Qt::NoBrush);
            p.setRenderHint(QPainter::Antialiasing);
            p.setRenderHint(QPainter::SmoothPixmapTransform);

            QPen pen(Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY), Utils::scale_value(1), Qt::DashLine, Qt::RoundCap);
            p.setPen(pen);
            p.drawRoundedRect(
                pen.width(),
                pen.width(),
                size_ - (pen.width() * 2),
                size_ - (pen.width() * 2),
                (size_ / 2),
                (size_ / 2)
            );

            const QPixmap* icon = nullptr;
            auto bgColor = Styling::StyleVariable::BASE;
            if (pressed_)
            {
                static const auto pm = Utils::renderSvg(qsl(":/cam_icon"), getCamIconSize(), Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY_ACTIVE));
                bgColor = Styling::StyleVariable::GHOST_ACCENT_ACTIVE;
                icon = &pm;
            }
            else if (hovered_)
            {
                static const auto pm = Utils::renderSvg(qsl(":/cam_icon"), getCamIconSize(), Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY_HOVER));
                bgColor = Styling::StyleVariable::GHOST_ACCENT_HOVER;
                icon = &pm;
            }
            else
            {
                static const auto pm = Utils::renderSvg(qsl(":/cam_icon"), getCamIconSize(), Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY));
                bgColor = Styling::StyleVariable::GHOST_ACCENT;
                icon = &pm;
            }

            QPainterPath path;
            path.addEllipse(QRectF(pen.width() * 1.5, pen.width() * 1.5, size_ - (pen.width() * 3), size_ - (pen.width() * 3)));
            p.setPen(Qt::NoPen);
            p.fillPath(path, Styling::getParameters().getColor(bgColor));

            const auto x = (width() - icon->width() / Utils::scale_bitmap_ratio()) / 2;
            const auto y = (height() - icon->height() / Utils::scale_bitmap_ratio()) / 2;
            p.drawPixmap(x, y, *icon);
        }
        else if (mode_ == Mode::ChangeAvatar || mode_ == Mode::Introduce)
        {
            if (infoForSetAvatar_.croppedImage.isNull())
            {
                Utils::drawAvatarWithBadge(p, QPoint(), avatar, aimid_, officialBadgeOnly_);
            }
            else
            {
                if (infoForSetAvatar_.roundedCroppedImage.isNull())
                {
                    auto scaled = infoForSetAvatar_.croppedImage.scaled(QSize(size_, size_), Qt::KeepAspectRatio, Qt::SmoothTransformation);
                    infoForSetAvatar_.roundedCroppedImage = Utils::roundImage(scaled, QString(), false, false);
                }
                p.drawPixmap(0, 0, size_, size_, infoForSetAvatar_.roundedCroppedImage);
            }

            if (isVisibleShadow_)
            {
                p.setPen(Qt::NoPen);
                p.setRenderHint(QPainter::Antialiasing);
                p.setRenderHint(QPainter::SmoothPixmapTransform);
                p.setBrush(QBrush(Styling::getParameters().getColor(Styling::StyleVariable::GHOST_SECONDARY)));
                p.drawEllipse(0, 0, size_, size_);
                static auto change_icon = Utils::renderSvgScaled(qsl(":/cam_icon"), QSize(CHANGE_ICON_SIZE, CHANGE_ICON_SIZE), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID_PERMANENT));
                p.drawPixmap(width() / 2 - Utils::scale_value(CHANGE_ICON_SIZE) / 2, height() / 2 - Utils::scale_value(CHANGE_ICON_SIZE) / 2, change_icon);
            }
        }
        else if (isVisibleBadge_ && !aimid_.isEmpty())
        {
            Utils::drawAvatarWithBadge(p, QPoint(), avatar, aimid_, officialBadgeOnly_);
        }
        else
        {
            Utils::drawAvatarWithoutBadge(p, QPoint(), avatar);
        }

        return QWidget::paintEvent(_e);
    }

    void ContactAvatarWidget::mousePressEvent(QMouseEvent *)
    {
        pressed_ = true;
        update();
    }

    void ContactAvatarWidget::UpdateParams(const QString& _aimid, const QString& _displayName)
    {
        aimid_ = _aimid;
        displayName_ = _displayName;
        update();
    }

    void ContactAvatarWidget::avatarChanged(const QString& _aimId)
    {
        if (_aimId == aimid_)
            update();
    }

    void ContactAvatarWidget::mouseReleaseEvent(QMouseEvent* _event)
    {
        pressed_ = false;

        if (_event->button() == Qt::LeftButton)
        {
            emit leftClicked();
            _event->accept();
        } else if (_event->button() == Qt::RightButton)
        {
            emit rightClicked();
            _event->accept();
        }

        update();
    }

    void ContactAvatarWidget::enterEvent(QEvent* /*_event*/)
    {
        hovered_ = true;
        emit mouseEntered();
        update();
    }

    void ContactAvatarWidget::leaveEvent(QEvent* /*_event*/)
    {
        hovered_ = false;
        emit mouseLeft();
        update();
    }

    void ContactAvatarWidget::resizeEvent(QResizeEvent*)
    {
        updateSpinnerPos();
    }

    bool ContactAvatarWidget::isDefault()
    {
        return defaultAvatar_;
    }

    void ContactAvatarWidget::SetImageCropHolder(QWidget *_holder)
    {
        imageCropHolder_ = _holder;
    }

    void ContactAvatarWidget::SetImageCropSize(const QSize &_size)
    {
        imageCropSize_ = _size;
    }

    void ContactAvatarWidget::SetMode(ContactAvatarWidget::Mode _mode)
    {
        mode_ = _mode;
        if (mode_ == Mode::Introduce)
        {
            if (!connected_)
            {
                connected_ = true;
                setMouseTracking(true);
                connect(this, &ContactAvatarWidget::leftClicked, this, &ContactAvatarWidget::selectFileForAvatar);
                connect(this, &ContactAvatarWidget::rightClicked, this, &ContactAvatarWidget::selectFileForAvatar);
                connect(this, &ContactAvatarWidget::mouseEntered, this, &ContactAvatarWidget::avatarEnter);
                connect(this, &ContactAvatarWidget::mouseLeft, this, &ContactAvatarWidget::avatarLeave);
                this->setCursor(Qt::CursorShape::PointingHandCursor);
            }
        }
        else if (mode_ == Mode::CreateChat || mode_ == Mode::ChangeAvatar)
        {
            if (!connected_)
            {
                connected_ = true;
                connect(this, &ContactAvatarWidget::leftClicked, this, &ContactAvatarWidget::selectFileForAvatar);
                connect(this, &ContactAvatarWidget::rightClicked, this, &ContactAvatarWidget::selectFileForAvatar);
                connect(this, &ContactAvatarWidget::mouseEntered, this, &ContactAvatarWidget::avatarEnter);
                connect(this, &ContactAvatarWidget::mouseLeft, this, &ContactAvatarWidget::avatarLeave);
                this->setCursor(Qt::CursorShape::PointingHandCursor);
            }
        }
        else
        {
            connected_ = false;
            disconnect(this, &ContactAvatarWidget::leftClicked, this, &ContactAvatarWidget::selectFileForAvatar);
            disconnect(this, &ContactAvatarWidget::rightClicked, this, &ContactAvatarWidget::selectFileForAvatar);
            disconnect(this, &ContactAvatarWidget::mouseEntered, this, &ContactAvatarWidget::avatarEnter);
            disconnect(this, &ContactAvatarWidget::mouseLeft, this, &ContactAvatarWidget::avatarLeave);
            this->setCursor(Qt::CursorShape::ArrowCursor);
        }

    }

    void ContactAvatarWidget::SetVisibleShadow(bool _isVisibleShadow)
    {
        isVisibleShadow_ = _isVisibleShadow;
    }

    void ContactAvatarWidget::SetVisibleSpinner(bool _isVisibleSpinner)
    {
        if (_isVisibleSpinner)
        {
            if (!spinner_)
                spinner_ = new RotatingSpinner(this);

            updateSpinnerPos();
            spinner_->startAnimation();
            spinner_->show();
        }
        else if (spinner_)
        {
            spinner_->hide();
            spinner_->stopAnimation();
        }
    }

    void ContactAvatarWidget::SetOutline(bool _isVisibleOutline)
    {
        isVisibleOutline_ = _isVisibleOutline;
        update();
    }

    void ContactAvatarWidget::applyAvatar(const QPixmap &alter)
    {
        if (alter.isNull())
            postSetAvatarToCore(infoForSetAvatar_.croppedImage);
        else
            postSetAvatarToCore(alter);
    }

    const QPixmap &ContactAvatarWidget::croppedImage() const
    {
        return infoForSetAvatar_.croppedImage;
    }

    void ContactAvatarWidget::postSetAvatarToCore(const QPixmap& _avatar)
    {
        const auto byteArray = processImage(_avatar);

        Ui::gui_coll_helper helper(GetDispatcher()->create_collection(), true);

        core::ifptr<core::istream> data_stream(helper->create_stream());
        if (!byteArray.isEmpty())
            data_stream->write((const uint8_t*)byteArray.data(), (uint32_t)byteArray.size());
        helper.set_value_as_stream("avatar", data_stream.get());
        if (aimid_.isEmpty())
            helper.set_value_as_bool("chat", true);
        else if (aimid_ != MyInfo()->aimId())
            helper.set_value_as_qstring("aimid", aimid_);

        seq_ = GetDispatcher()->post_message_to_core("set_avatar", helper.get());
    }

    void ContactAvatarWidget::selectFileForAvatar()
    {
        ResetInfoForSetAvatar();
        QFileDialog fileDialog(platform::is_linux() ? nullptr : this);
        fileDialog.setDirectory(infoForSetAvatar_.currentDirectory);
        fileDialog.setFileMode(QFileDialog::ExistingFiles);
        fileDialog.setNameFilter(QT_TRANSLATE_NOOP("avatar_upload", "Images (*.jpg *.jpeg *.png *.bmp)"));

        bool isContinue = true;
        QImage newAvatar;

        while (isContinue)
        {
            isContinue = false;
            if (fileDialog.exec())
            {
                infoForSetAvatar_.currentDirectory = fileDialog.directory().path();

                {
                    QFile file(fileDialog.selectedFiles().constFirst());
                    if (!file.open(QIODevice::ReadOnly))
                        return;

                    QPixmap preview;
                    Utils::loadPixmap(file.readAll(), Out preview);

                    if (preview.isNull())
                        return;

                    newAvatar = preview.toImage();
                }

                if (newAvatar.height() < MIN_AVATAR_SIZE || newAvatar.width() < MIN_AVATAR_SIZE)
                {
                    if (Utils::GetErrorWithTwoButtons(
                        QT_TRANSLATE_NOOP("popup_window", "CANCEL"),
                        QT_TRANSLATE_NOOP("avatar_upload", "CHOOSE FILE"),
                        QString(),
                        QT_TRANSLATE_NOOP("avatar_upload", "Upload photo"),
                        QT_TRANSLATE_NOOP("avatar_upload", "Image should be at least 200x200 px"),
                        nullptr))
                    {
                        isContinue = true;
                    }
                    else
                    {
                        return;
                    }
                }
            }
            else
            {
                emit cancelSelectFileForAvatar();
                return;
            }
        }

        infoForSetAvatar_.image = newAvatar;
        cropAvatar();
    }

    void ContactAvatarWidget::ResetInfoForSetAvatar()
    {
        infoForSetAvatar_.image = QImage();
        infoForSetAvatar_.croppingRect = QRectF();
        infoForSetAvatar_.croppedImage = QPixmap();
    }

    void ContactAvatarWidget::updateSpinnerPos()
    {
        if (spinner_)
        {
            auto spRect = spinner_->rect();
            spRect.moveCenter(rect().center());
            spinner_->move(spRect.topLeft());
        }
    }

    void ContactAvatarWidget::cropAvatar()
    {
        QWidget *hostWidget = nullptr;
        QLayout *hostLayout = nullptr;

        QWidget *mainWidget = nullptr;
        QLayout *mainLayout = nullptr;

        if (imageCropHolder_)
        {
            hostWidget = new QWidget(this);
            hostLayout = new QHBoxLayout(hostWidget);
            hostWidget->setSizePolicy(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Fixed);
            hostWidget->setFixedHeight(imageCropSize_.height());
            hostLayout->setContentsMargins(Utils::scale_value(16), Utils::scale_value(12), Utils::scale_value(16), 0);

            mainWidget = new QWidget(hostWidget);
            mainLayout = Utils::emptyHLayout(mainWidget);
            mainWidget->setSizePolicy(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Preferred);
            Testing::setAccessibleName(mainWidget, qsl("AS caw mainWidget"));
            hostLayout->addWidget(mainWidget);
        }
        else
        {
            mainWidget = new QWidget(this);
            mainLayout = new QHBoxLayout(mainWidget);
            mainLayout->setContentsMargins(Utils::scale_value(16), Utils::scale_value(12), Utils::scale_value(16), 0);
        }

        auto avatarCropper = new Ui::ImageCropper(mainWidget, imageCropSize_);
        avatarCropper->setProportion(QSizeF(1.0, 1.0));
        avatarCropper->setProportionFixed(true);
        avatarCropper->setBackgroundColor(QColor(ql1s("#ffffff")));
        if (!infoForSetAvatar_.croppingRect.isNull())
        {
            avatarCropper->setCroppingRect(infoForSetAvatar_.croppingRect);
        }
        avatarCropper->setImage(QPixmap::fromImage(infoForSetAvatar_.image));

        Testing::setAccessibleName(avatarCropper, qsl("AS avatarCropper"));
        mainLayout->addWidget(avatarCropper);

        if (!imageCropHolder_)
            hostWidget = mainWidget;

        GeneralDialog imageCropDialog(hostWidget, imageCropHolder_ ? imageCropHolder_ : Utils::InterConnector::instance().getMainWindow(), false, false);
        imageCropDialog.setObjectName(qsl("image.cropper"));
        if (imageCropHolder_)
        {
            imageCropDialog.setShadow(false);
            imageCropDialog.connect(&imageCropDialog, &GeneralDialog::moved, this, [=](QWidget *dlg)
            {
                emit Utils::InterConnector::instance().imageCropDialogMoved(dlg);
            });
            imageCropDialog.connect(&imageCropDialog, &GeneralDialog::resized, this, [=](QWidget *dlg)
            {
                emit Utils::InterConnector::instance().imageCropDialogResized(dlg);
            });
            imageCropDialog.connect(&imageCropDialog, &GeneralDialog::shown, this, [=](QWidget *dlg)
            {
                emit Utils::InterConnector::instance().imageCropDialogIsShown(dlg);
            });
            imageCropDialog.connect(&imageCropDialog, &GeneralDialog::hidden, this, [=](QWidget *dlg)
            {
                emit Utils::InterConnector::instance().imageCropDialogIsHidden(dlg);
            });
        }

        imageCropDialog.addLabel((mode_ == Mode::CreateChat || mode_ == Mode::ChangeAvatar) ? QT_TRANSLATE_NOOP("avatar_upload", "Edit photo") : QT_TRANSLATE_NOOP("avatar_upload", "Upload photo"));
        imageCropDialog.addButtonsPair(QT_TRANSLATE_NOOP("popup_window", "BACK"), QT_TRANSLATE_NOOP("popup_window", "CONTINUE"), true, false, false);
        imageCropDialog.setRightButtonDisableOnClicked(true);

        QObject::connect(&imageCropDialog, &GeneralDialog::leftButtonClicked, this, [=, &imageCropDialog]()
        {
            imageCropDialog.reject();
            QTimer::singleShot(500, this, [=](){ emit summonSelectFileForAvatar(); });
        },
        Qt::QueuedConnection);
        QObject::connect(&imageCropDialog, &GeneralDialog::rightButtonClicked, this, [=, &imageCropDialog]()
        {
            auto croppedImage = avatarCropper->cropImage();
            auto croppingRect = avatarCropper->getCroppingRect();

            infoForSetAvatar_.croppedImage = croppedImage;
            infoForSetAvatar_.croppingRect = croppingRect;
            imageCropDialog.accept();

            if (auto p = imageCropDialog.takeAcceptButton())
                p->setEnabled(true);
        },
        Qt::QueuedConnection);
        QObject::connect(&imageCropDialog, &GeneralDialog::shown, this, [=](QWidget *dlg)
        {
            //
        },
        Qt::QueuedConnection);

        if (!imageCropDialog.showInCenter())
            return;

        if (mode_ == Mode::CreateChat || mode_ == Mode::ChangeAvatar)
        {
            infoForSetAvatar_.roundedCroppedImage = QPixmap();
            emit avatarDidEdit();
        }
        else
        {
            openAvatarPreview();
        }
    }

    void ContactAvatarWidget::openAvatarPreview()
    {
        auto layout = new QHBoxLayout();

        auto spacerLeft = new QSpacerItem(Utils::scale_value(12), 1, QSizePolicy::Expanding);
        layout->addSpacerItem(spacerLeft);

        auto croppedAvatar = infoForSetAvatar_.croppedImage;
        auto previewAvatarWidget = new AvatarPreview(croppedAvatar, nullptr);
        previewAvatarWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Minimum);

        Testing::setAccessibleName(previewAvatarWidget, qsl("AS previewAvatarWidget"));
        layout->addWidget(previewAvatarWidget);

        auto spacerRight = new QSpacerItem(Utils::scale_value(12), 1, QSizePolicy::Expanding);
        layout->addSpacerItem(spacerRight);

        auto avatarPreviewHost = new QWidget();
        avatarPreviewHost->setLayout(layout);

        Ui::GeneralDialog previewDialog(avatarPreviewHost, Utils::InterConnector::instance().getMainWindow());
        previewDialog.addLabel(QT_TRANSLATE_NOOP("avatar_upload", "Preview"));
        previewDialog.addButtonsPair(QT_TRANSLATE_NOOP("popup_window", "BACK"), QT_TRANSLATE_NOOP("popup_window", "SAVE"), true);

        QObject::connect(&previewDialog, &GeneralDialog::leftButtonClicked, this, &ContactAvatarWidget::cropAvatar, Qt::QueuedConnection);

        if (previewDialog.showInCenter())
        {
            SetVisibleSpinner(true);
            postSetAvatarToCore(croppedAvatar);
        }
    }

    void ContactAvatarWidget::setAvatar(qint64 _seq, int _error)
    {
        if (_seq != seq_)
            return;

        SetVisibleSpinner(false);

        if (_error != 0)
        {
             if (Utils::GetErrorWithTwoButtons(
                QT_TRANSLATE_NOOP("popup_window", "CANCEL"),
                QT_TRANSLATE_NOOP("avatar_upload", "CHOOSE FILE"),
                QString(),
                QT_TRANSLATE_NOOP("avatar_upload", "Upload photo"),
                QT_TRANSLATE_NOOP("avatar_upload", "Avatar was not uploaded due to server error"),
                nullptr))
             {
                selectFileForAvatar();
             }
        }
        else
        {
            if (build::is_agent() || build::is_dit())
            {
                Logic::GetAvatarStorage()->SetAvatar(aimid_, infoForSetAvatar_.croppedImage);
            }

            emit afterAvatarChanged();
        }
    }

    void ContactAvatarWidget::avatarEnter()
    {
        SetVisibleShadow(true);
        update();
    }

    void ContactAvatarWidget::avatarLeave()
    {
        SetVisibleShadow(false);
        update();
    }
}
