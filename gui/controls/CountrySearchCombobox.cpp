#include "stdafx.h"
#include "CountrySearchCombobox.h"

#include "CustomButton.h"
#include "LineEditEx.h"
#include "PictureWidget.h"
#include "../utils/utils.h"
#include "../styles/ThemeParameters.h"
#ifdef __APPLE__
#   include "../utils/macos/mac_support.h"
#endif

namespace Ui
{
    SearchComboboxView::SearchComboboxView(QWidget* _parent)
        : QTreeView(_parent)
    {
        Utils::ApplyStyle(this, Styling::getParameters().getPhoneComboboxQss());
    }

    void SearchComboboxView::paintEvent(QPaintEvent* _event)
    {
        int expectedWidth = width() * 0.65;
        if (header()->sectionSize(0) != expectedWidth)
            header()->resizeSection(0, expectedWidth);
        QTreeView::paintEvent(_event);
    }

    CountrySearchCombobox::CountrySearchCombobox(QWidget* _parent)
        : Edit_(new LineEditEx(_parent))
        , Completer_(new QCompleter(_parent))
        , ComboboxView_(new SearchComboboxView(_parent))
    {
        Utils::setDefaultBackground(this);

        initLayout();

        ComboboxView_->setSelectionBehavior(QAbstractItemView::SelectRows);
        ComboboxView_->setAllColumnsShowFocus(true);
        ComboboxView_->setRootIsDecorated(false);
        ComboboxView_->header()->hide();
        ComboboxView_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

        searchGlass_ = new PictureWidget(Edit_);
        searchGlass_->setImage(Utils::renderSvgScaled(qsl(":/search_icon"), QSize(20, 20), Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY)));
        searchGlass_->setFixedSize(Utils::scale_value(QSize(24, 24)));
        searchGlass_->hide();
        searchGlass_->setAttribute(Qt::WA_TransparentForMouseEvents);

        dropDown_ = new CustomButton(Edit_, qsl(":/controls/down_icon"), QSize(16, 16));
        Styling::Buttons::setButtonDefaultColors(dropDown_);
        dropDown_->setFixedSize(Utils::scale_value(QSize(20, 20)));
        dropDown_->setAttribute(Qt::WA_TransparentForMouseEvents);

        connect(Edit_, &Ui::LineEditEx::focusIn, this, &CountrySearchCombobox::setFocusIn, Qt::QueuedConnection);
        connect(Edit_, &Ui::LineEditEx::focusOut, this, &CountrySearchCombobox::setFocusOut, Qt::QueuedConnection);
        connect(Edit_, &Ui::LineEditEx::clicked, this, &CountrySearchCombobox::editClicked, Qt::QueuedConnection);
        connect(Edit_, &Ui::LineEditEx::textEdited, this, &CountrySearchCombobox::editTextChanged, Qt::QueuedConnection);
        connect(Edit_, &Ui::LineEditEx::editingFinished, this, &CountrySearchCombobox::editCompleted, Qt::QueuedConnection);
        connect(Completer_, static_cast<void(QCompleter::*)(const QString&)>(&QCompleter::activated), this, &CountrySearchCombobox::completerActivated, Qt::QueuedConnection);

        Edit_->setAttribute(Qt::WA_MacShowFocusRect, false);
        this->setAttribute(Qt::WA_MacShowFocusRect, false);
    }

    void CountrySearchCombobox::setFocusIn()
    {
        searchGlass_->show();
    }

    void CountrySearchCombobox::setFocusOut()
    {
        searchGlass_->hide();
    }

    void CountrySearchCombobox::resizeEvent(QResizeEvent *_e)
    {
        QWidget::resizeEvent(_e);
        QRect r = rect();
        searchGlass_->move(r.x() + Utils::scale_value(4), r.y() + Utils::scale_value(14));
        dropDown_->move(r.width() - dropDown_->width(), r.y() + Utils::scale_value(16));
    }

    void CountrySearchCombobox::initLayout()
    {
        QHBoxLayout* mainLayout = Utils::emptyHLayout(this);
        Testing::setAccessibleName(Edit_, qsl("AS csc Edit_"));
        mainLayout->addWidget(Edit_);
        QSpacerItem* editLayoutSpacer = new QSpacerItem(0, 0, QSizePolicy::Minimum);
        mainLayout->addSpacerItem(editLayoutSpacer);
    }

    void CountrySearchCombobox::setComboboxViewClass(const char* _className)
    {
        ComboboxView_->setProperty(_className, true);
    }

    void CountrySearchCombobox::setClass(const char* _className)
    {
        setProperty(_className, true);
    }

    void CountrySearchCombobox::setSources(const QMap<QString, QString>& _sources)
    {
        Sources_ = _sources;
        QStandardItemModel* model = new QStandardItemModel(this);
        model->setColumnCount(2);
        int i = 0;
        for (auto it = _sources.begin(), end = _sources.end(); it != end; ++it)
        {
            QStandardItem* firstCol = new QStandardItem(it.key());
            QStandardItem* secondCol = new QStandardItem(it.value());
            secondCol->setTextAlignment(Qt::AlignRight);
            model->setItem(i, 0, firstCol);
            model->setItem(i, 1, secondCol);
            ++i;
        }

        Completer_->setCaseSensitivity(Qt::CaseInsensitive);
        Completer_->setModel(model);
        Completer_->setPopup(ComboboxView_);
        Completer_->setCompletionColumn(0);
        Completer_->setCompletionMode(QCompleter::PopupCompletion);
        Edit_->setCompleter(Completer_);
    }

    void CountrySearchCombobox::setPlaceholder(const QString& _placeholder)
    {
        Edit_->setPlaceholderText(_placeholder);
    }

    void CountrySearchCombobox::editClicked()
    {
        if (Edit_->completer())
        {
            OldEditValue_ = Edit_->text();
            Edit_->clear();
            Completer_->setCompletionPrefix(QString());
            Edit_->completer()->complete();
        }
    }

    void CountrySearchCombobox::completerActivated(const QString& _text)
    {
        Edit_->clearFocus();
    }

    void CountrySearchCombobox::editCompleted()
    {
        QString value;
        if (Completer_->completionColumn() == 0)
        {
            value = getValue(Edit_->text());
        }
        else
        {
            static const QRegularExpression re(
                qsl("^\\d*$"),
                QRegularExpression::UseUnicodePropertiesOption | QRegularExpression::OptimizeOnFirstUsageOption
            );

            value = Edit_->text();
            if (re.match(value).hasMatch())
                value.prepend(ql1c('+'));

            bool doClassicRoutine = true;
            if (auto selectionModel = ComboboxView_->selectionModel())
            {
                auto selectedRows = selectionModel->selectedRows();
                if (!selectedRows.isEmpty())
                {
                    doClassicRoutine = false;
                    auto rowModel = selectedRows.first();
                    auto country = rowModel.data().toString();
                    Edit_->setText(country);
                }
            }
            if (doClassicRoutine)
            {
                if (value == Utils::GetTranslator()->getCurrentPhoneCode())
                {
#ifdef __APPLE__
                    QString counryCode = MacSupport::currentRegion().right(2).toLower();;
#else
                    QString counryCode = QLocale::system().name().right(2).toLower();
#endif
                    const auto mustBe = Utils::getCountryNameByCode(counryCode).toLower();
                    const auto stdMap = Sources_.toStdMap();
                    const auto it = std::find_if(
                        stdMap.begin(),
                        stdMap.end(),
                        [mustBe](const std::pair<QString, QString> &p)
                    {
                        return p.first.toLower() == mustBe;
                    });
                    if (it != stdMap.end())
                        Edit_->setText(it->first);
                    else
                        Edit_->setText(Sources_.key(value));
                }
                else
                {
                    Edit_->setText(Sources_.key(value));
                }
            }
            Completer_->setCompletionColumn(0);
        }

        if (value.isEmpty())
        {
            Completer_->setCompletionPrefix(QString());
            Edit_->setText(OldEditValue_);
        }
        else
        {
            emit selected(value);
        }
    }

    void CountrySearchCombobox::editTextChanged(const QString& _text)
    {
        int completionColumn = Completer_->completionColumn();
        int newCompletionColumn;

        static const QRegularExpression re(
            qsl("^[\\+\\d]\\d*$"),
            QRegularExpression::UseUnicodePropertiesOption | QRegularExpression::OptimizeOnFirstUsageOption
        );

        if (re.match(_text).hasMatch())
        {
            QString completion = Completer_->completionPrefix();
            if (!completion.startsWith(ql1c('+')))
                Completer_->setCompletionPrefix(ql1c('+') + completion);
            newCompletionColumn = 1;
        }
        else
        {
            newCompletionColumn = 0;
        }

        if (completionColumn != newCompletionColumn)
        {
            Completer_->setCompletionColumn(newCompletionColumn);
        }

        if (_text.isEmpty())
        {
            Completer_->setCompletionPrefix(QString());
        }
        else
        {
            Completer_->complete();
        }
    }

    QString CountrySearchCombobox::getValue(const QString& _key) const
    {
        for (auto it = Sources_.cbegin(), end = Sources_.cend(); it != end; ++it)
        {
            if (it.key().compare(_key, Qt::CaseInsensitive) == 0)
                return it.value();
        }

        return QString();
    }

    bool CountrySearchCombobox::selectItem(QString _item)
    {
        static const QRegularExpression re(
            qsl("^[\\+\\d]\\d*$"),
            QRegularExpression::UseUnicodePropertiesOption | QRegularExpression::OptimizeOnFirstUsageOption
        );

        if (re.match(_item).hasMatch())
        {
            if (!_item.startsWith(ql1c('+')))
                _item.prepend(ql1c('+'));
            if (_item == Utils::GetTranslator()->getCurrentPhoneCode())
            {
#ifdef __APPLE__
                QString counryCode = MacSupport::currentRegion().right(2).toLower();;
#else
                QString counryCode = QLocale::system().name().right(2).toLower();
#endif
                const auto mustBe = Utils::getCountryNameByCode(counryCode).toLower();
                const auto stdMap = Sources_.toStdMap();
                const auto it = std::find_if(stdMap.begin(), stdMap.end(), [mustBe](const std::pair<QString, QString> &p){ return p.first.toLower() == mustBe; });
                if (it != stdMap.end())
                    _item = it->first;
                else
                    _item = Sources_.key(_item);
            }
            else
            {
                _item = Sources_.key(_item);
            }
        }

        auto value = getValue(_item);
        if (!value.isEmpty())
        {
            Edit_->setText(_item);
            OldEditValue_ = _item;
            emit selected(value);
            return true;
        }
        return false;
    }

    bool CountrySearchCombobox::containsCode(const QString& _code) const
    {
        for (const auto& iter : Sources_)
        {
            if (iter.contains(_code))
                return true;
        }

        return false;
    }
}
