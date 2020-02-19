#include "webserver.h"
#include "stdafx.h"

#include <QTest>
#include <QtWidgets>
#include <QtGui>
#include <QSignalSpy>

#include "main_window/MainWindow.h"
#include "main_window/contact_list/ContactListModel.h"
#include "main_window/contact_list/ContactList.h"
#include "main_window/contact_list/ContactListWidget.h"
#include "main_window/contact_list/ContactListWithHeaders.h"
#include "main_window/contact_list/RecentsModel.h"
#include "main_window/contact_list/RecentItemDelegate.h"
#include "main_window/contact_list/SearchMembersModel.h"
#include "main_window/contact_list/SearchModel.h"

#include "main_window/input_widget/InputWidget.h"

#include "controls/ContextMenu.h"

#include "main_window/history_control/FileSharingInfo.h"
#include "main_window/history_control/VoipEventItem.h"

#include "main_window/history_control/MessagesScrollArea.h"

#include "main_window/history_control/complex_message/ComplexMessageItem.h"
#include "main_window/history_control/complex_message/ComplexMessageUtils.h"
#include "main_window/history_control/complex_message/FileSharingBlock.h"
#include "main_window/history_control/complex_message/FileSharingBlockBase.h"
#include "main_window/history_control/complex_message/FileSharingImagePreviewBlockLayout.h"
#include "main_window/history_control/complex_message/FileSharingPlainBlockLayout.h"
#include "main_window/history_control/complex_message/FileSharingUtils.h"

#include "main_window/history_control/complex_message/IItemBlock.h"

#include "main_window/history_control/complex_message/LinkPreviewBlock.h"

#include "main_window/history_control/complex_message/PttBlock.h"

#include "main_window/history_control/complex_message/QuoteBlock.h"
#include "main_window/history_control/complex_message/StickerBlock.h"
#include "main_window/history_control/complex_message/TextBlock.h"

#include "main_window/history_control/complex_message/TextChunk.h"

#include "previewer/GalleryWidget.h"

#include "utils/utils.h"
#include "utils/InterConnector.h"

QString returnNotFound()
{
    return qsl("404 Not found");
}

WebServer::WebServer()
{
    tcpServer = nullptr;
}

WebServer::~WebServer()
{
    server_status = 0;
}

void WebServer::startWebserver()
{
    qDebug() << "... WebServer::startWebserver()";
    tcpServer = new QTcpServer(this);
    connect(tcpServer, SIGNAL(newConnection()), this, SLOT(newUser()));
    if (!tcpServer->listen(QHostAddress::Any, 33333) && server_status == 0)
    {
        qDebug() << QObject::tr("Unable to start the server: %1.").arg(tcpServer->errorString());
    }
    else
    {
        server_status = 1;
        qDebug() << tcpServer->isListening() << "TCPSocket listen on port 33333";
        qDebug() << QString::fromUtf8("WebServer is Running!");
    }
}


void WebServer::newUser()
{
    qDebug() << "... WebServer::newUser()";
    if (server_status == 1)
    {
        QTcpSocket* clientSocket = tcpServer->nextPendingConnection();
        int idusersocs = clientSocket->socketDescriptor();
        SClients[idusersocs] = clientSocket;
        connect(SClients[idusersocs], SIGNAL(readyRead()), this, SLOT(slotReadClient()));
    }
}


static void dump_props(QObject *o)
{
    auto mo = o->metaObject();
    qDebug() << "## Properties of" << o << "##";
    do {
        qDebug() << "### Class" << mo->className() << "###";
        std::vector<std::pair<QString, QVariant> > v;
        v.reserve(mo->propertyCount() - mo->propertyOffset());
        for (int i = mo->propertyOffset(); i < mo->propertyCount();
            ++i)
            v.emplace_back(QString::fromUtf8(mo->property(i).name()),
                mo->property(i).read(o));
        std::sort(v.begin(), v.end());
        for (auto &i : v)
            qDebug() << i.first << "=>" << i.second;
    } while ((mo = mo->superClass()));
}


void printObjectChildren(QWidget* _parent)
{
    auto widgets = _parent->findChildren<QWidget *>();
    for (auto w : widgets)
    {
        if (!w->objectName().isEmpty())
            qDebug() << w->objectName();
        dump_props(w);
        if (!w->accessibleName().isEmpty())
            qDebug() << w->accessibleName();

        printObjectChildren(w);
    }
}


void printObjectAnyChildren(QObject* _parent)
{
    auto widgets = _parent->findChildren<QObject *>();
    for (auto w : widgets)
    {
        if (!w->objectName().isEmpty())
            qDebug() << w->objectName();
        dump_props(w);
        printObjectAnyChildren(w);
    }
}


void LeftClick()
{
    #ifdef _WIN32
        INPUT    Input = { 0 };                                     // Create our input.
        Input.type = INPUT_MOUSE;                                   // Let input know we are using the mouse.
        Input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;                    // We are setting left mouse button down.
        SendInput(1, &Input, sizeof(INPUT));                        // Send the input.
        ZeroMemory(&Input, sizeof(INPUT));                          // Fills a block of memory with zeros.
        Input.type = INPUT_MOUSE;                                   // Let input know we are using the mouse.
        Input.mi.dwFlags = MOUSEEVENTF_LEFTUP;                      // We are setting left mouse button up.
        SendInput(1, &Input, sizeof(INPUT));                        // Send the input.
    #endif
    #ifdef __APPLE__
        qDebug() << "\n";
        qDebug() << "!!! ATTENTION !!!";
        qDebug() << "LeftClick()!!! Not works on macOS !!! FIXME";
        qDebug() << "\n";
    #endif
    #ifdef __linux__
        qDebug() << "\n";
        qDebug() << "!!! ATTENTION !!!";
        qDebug() << "LeftClick()!!! Not works on Linux !!! FIXME";
    #endif
}


void RightClick()
{
#ifdef _WIN32
    INPUT    Input = { 0 };                                     // Create our input.
    Input.type = INPUT_MOUSE;                                   // Let input know we are using the mouse.
    Input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;                    // We are setting left mouse button down.
    SendInput(1, &Input, sizeof(INPUT));                        // Send the input.
    ZeroMemory(&Input, sizeof(INPUT));                          // Fills a block of memory with zeros.
    Input.type = INPUT_MOUSE;                                   // Let input know we are using the mouse.
    Input.mi.dwFlags = MOUSEEVENTF_RIGHTUP;                      // We are setting right mouse button up.
    SendInput(1, &Input, sizeof(INPUT));                        // Send the input.
    #endif
    #ifdef __APPLE__
        qDebug() << "!!! ATTENTION !!!";
        qDebug() << "RightClick()!!! Not works on macOS !!! FIXME";
    #endif
}


QWidget* findElementByAccessibleName(const QString& elementName)
{
    auto mainWidget_ = Utils::InterConnector::instance().getMainWindow(true);
    if (mainWidget_ != nullptr)
    {
        QList<QWidget *> widgets = mainWidget_->findChildren<QWidget *>();
        for (QWidget* b : widgets)
        {
            auto acName = b->accessibleName();
            if (acName == elementName && b->isVisible())
            {
                return b;
            }
        }
    }
    return nullptr;
}


QWidget* findElementInSidebar(const QString& elementName)
{
    auto mainWidget_ = Utils::InterConnector::instance().getMainWindow(true);
    if (mainWidget_ != nullptr)
    {
        QList<QWidget *> widgets = mainWidget_->findChildren<QWidget *>();
        for (QWidget* b : widgets)
        {
            if (b->accessibleName() == qsl("AS sb mainWidget_") && b->isVisible())
            {
                for (QWidget* sb : b->findChildren<QWidget *>())
                {
                    if (sb->accessibleName() == elementName && sb->isVisible())
                    {
                        return sb;
                    }
                }
            }
        }
    }
    return nullptr;
}


QString isElementInSidebar(const QString& elementName)
{
    auto mainWidget_ = Utils::InterConnector::instance().getMainWindow(true);
    if (mainWidget_ != nullptr)
    {
        QList<QWidget *> widgets = mainWidget_->findChildren<QWidget *>();
        for (QWidget* b : widgets)
        {
            auto acName = b->accessibleName();
            if (acName == qsl("AS sb mainWidget_") && b->isVisible())
            {
                for (QWidget* sb : b->findChildren<QWidget *>())
                {
                    if (sb->accessibleName() == elementName && sb->isVisible())
                    {
                        return qsl("True");
                    }
                }
            }
        }
    }
    return returnNotFound();
}


void clickToElement(const QString& elementName)
{
    if (findElementByAccessibleName(elementName) != nullptr)
    {
        QPoint widgetPos;
        if (elementName == qsl("AS lp keepLogged"))
        {
            widgetPos = QPoint(10, 20);
        }
        else
        {
            widgetPos = findElementByAccessibleName(elementName)->rect().center();
        }
        QTest::mouseMove(findElementByAccessibleName(elementName), widgetPos, 100);
        QTest::qWait(100);
        QTest::mouseClick(findElementByAccessibleName(elementName), Qt::LeftButton, Qt::NoModifier, widgetPos, 100);
    }
}


bool bringAppToFront()
{
    auto mainWidget_ = Utils::InterConnector::instance().getMainWindow(true);
    if (mainWidget_ != nullptr)
    {
        qDebug() << "... mainWidget_->activate();";
        mainWidget_->activate();
    }
    // qDebug() << "... clickToElement(AS title_);";
    // clickToElement(qsl("AS title_"));
    return true;
}


QString setScrollValueTo(const QString& elementName, const QString& elementValue)
{
    auto mainWidget_ = Utils::InterConnector::instance().getMainWindow(true);
    if (mainWidget_ != nullptr)
    {
        QList<QWidget *> widgets = mainWidget_->findChildren<QWidget *>();
        for (QWidget* b : widgets)
        {
            auto accName = b->accessibleName();
            if (!accName.isEmpty())
            {
                //auto scrollBlockArea = qobject_cast<Ui::ScrollAreaWithTrScrollBar*>(b);
                //auto scrollBlockArea = qobject_cast<Ui::FocusableListView*>(b);
                auto scrollBlockArea = qobject_cast<QAbstractScrollArea*>(b);
                if (scrollBlockArea)
                {
                    qDebug() << "... DEBUG: scrollBlockArea = " << scrollBlockArea->accessibleName();

                    if (accName == elementName && b->isVisible())
                    {
                        auto currentScrollArea = scrollBlockArea->verticalScrollBar()->value();
                        //scroll_top -> Going to begin of the scroll area
                        //scroll_bottom -> Going to the end of the scroll area
                        //scroll_down -> Scroll down the scroll area
                        //scroll_up -> Scroll down the scroll area
                        if (elementValue == qsl("scroll_top")) {
                            scrollBlockArea->verticalScrollBar()->setValue(scrollBlockArea->verticalScrollBar()->minimum());
                        }
                        else if (elementValue == qsl("scroll_bottom")) {
                            scrollBlockArea->verticalScrollBar()->setValue(scrollBlockArea->verticalScrollBar()->maximum());
                        }
                        else if (elementValue == qsl("scroll_down")) {
                            currentScrollArea += 100;
                            scrollBlockArea->verticalScrollBar()->setValue(currentScrollArea);
                        }
                        else if (elementValue == qsl("scroll_up")) {
                            currentScrollArea -= 100;
                            scrollBlockArea->verticalScrollBar()->setValue(currentScrollArea);
                        }
                        else {
                            return qsl("Parameters for this requests: [ scroll_top, scroll_bottom, scroll_down, scroll_up ]");
                        }
                        return qsl("True");
                    }
                }
            }
        }
    }
    return returnNotFound();
}


QString setHistoryScrollToBottom()
{
    auto mainWidget_ = Utils::InterConnector::instance().getMainWindow(true);
    if (mainWidget_ != nullptr)
    {
        QList<QWidget *> widgets = mainWidget_->findChildren<QWidget *>();
        for (QWidget* b : widgets)
        {
            auto accName = b->accessibleName();
            if (!accName.isEmpty())
            {
                auto scrollBlockArea = qobject_cast<Ui::MessagesScrollArea*>(b);
                if (scrollBlockArea)
                {
                    qDebug() << "... DEBUG: scrollBlockArea = " << scrollBlockArea->accessibleName();
                    if (accName == qsl("AS hcp messagesArea_") && b->isVisible())
                    {
                        scrollBlockArea->scrollToBottom();
                        return qsl("True");
                    }
                }
            }
        }
    }
    return returnNotFound();
}


void openItemFromSidebar(const QString& elementName)
{
    auto mainWidget_ = Utils::InterConnector::instance().getMainWindow(true);
    if (mainWidget_ != nullptr)
    {
        QList<QWidget *> widgets = mainWidget_->findChildren<QWidget *>();
        for (QWidget* b : widgets)
        {
            auto acName = b->accessibleName();
            if (acName == qsl("AS sb mainWidget_") && b->isVisible())
            {
                for (QWidget* sb : b->findChildren<QWidget *>())
                {
                    if (sb->accessibleName() == elementName && sb->isVisible())
                    {
                        const QPoint elementPos(sb->rect().center());
                        QTest::mouseMove(sb, elementPos, 1);
                        QTest::qWait(50);
                        QTest::mouseClick(sb, Qt::LeftButton, Qt::NoModifier, elementPos, 100);
                    }
                }
            }
        }
    }
}


QString isElementExists(const QString& elementName)
{
    auto mainWidget_ = Utils::InterConnector::instance().getMainWindow(true);
    if (mainWidget_ != nullptr)
    {
        QList<QWidget *> widgets = mainWidget_->findChildren<QWidget *>();
        for (QWidget* b : widgets)
        {
            auto accName = b->accessibleName();
            if (!accName.isEmpty())
            {
                if (accName == elementName && b->isVisible())
                {
                    return qsl("True");
                }
            }
        }
    }
    qDebug() << "... DEBUG: Element " << elementName << " is not exists";
    return returnNotFound();
}


QString clickItemInContextMenu(const QString& menuName, const QString& itemValue)
{
    auto mainWidget_ = Utils::InterConnector::instance().getMainWindow(true);
    if (mainWidget_ != nullptr)
    {
        QList<QMenu *> menus = mainWidget_->findChildren<QMenu *>();
        qDebug() << "... DEBUG: searching menu with name = " << menuName;
        qDebug() << "... DEBUG: searching item with name = " << itemValue;
        for (QWidget* m : menus)
        {
            auto accName = m->accessibleName();
            if (!accName.isEmpty())
            {
                qDebug() << "... DEBUG: qmenu = " << accName;
                if (accName == menuName && m->isVisible())
                {
                    qDebug() << "... DEBUG: found qmenu with name " << accName;
                    foreach(QAction *action, m->actions())
                    {
                        if (action->isSeparator())
                        {
                            qDebug() << "... DEBUG: this action is a separator";
                        }
                        else if (action->menu())
                        {
                            qDebug() << "... DEBUG: action = " << action->text();
                        }
                        else
                        {
                            qDebug() << "... DEBUG: action = " << action->text();
                            if (itemValue == action->text())
                            {
                                const auto actionIndex = m->actions().indexOf(action);

                                auto current_menu = qobject_cast<Ui::ContextMenu::QMenu*>(m);
                                const QRect actionRect = current_menu->actionGeometry(current_menu->actions().at(actionIndex));
                                //const QPoint actionPos(actionRect.topLeft() + QPoint(4, 15));
                                const QPoint actionPos(actionRect.center());

                                QTest::mouseMove(m, actionPos, 100);
                                QTest::qWait(300);
                                QTest::mouseClick(current_menu, Qt::LeftButton, 0, actionPos, 100);

                                return qsl("True");
                            }
                        }
                    }
                }
            }
        }
    }
    return returnNotFound();
}


QString isContactInRecents(const QString& contactName)
{
    auto mainWidget_ = Utils::InterConnector::instance().getMainWindow(true);
    if (mainWidget_ != nullptr)
    {
        QList<QWidget *> widgets = mainWidget_->findChildren<QWidget *>();
        for (QWidget* b : widgets)
        {
            auto acName = b->accessibleName();
            if (acName == qsl("AS recentsView_") && b->isVisible())
            {
                auto ab = qobject_cast<QListView*>(b);
                if (!ab)
                {
                    qDebug() << "... DEBUG: Qobject_cast failed";
                    continue;
                }

                auto ndx = Logic::getRecentsModel()->contactIndex(contactName);
                if (!ndx.isValid())
                {
                    qDebug() << "... DEBUG: Index is invalid";
                    return returnNotFound();
                }
                else
                {
                    return qsl("True");
                }
            }
        }
    }
    return returnNotFound();
}


QString isItemInSearchResults(const QString& itemName)
{
    auto mainWidget_ = Utils::InterConnector::instance().getMainWindow(true);
    if (mainWidget_ != nullptr)
    {
        QList<QListView *> listitems = mainWidget_->findChildren<QListView *>();
        for (QListView* b : listitems)
        {
            auto acName = b->accessibleName();
            if (acName == qsl("AS search view_") && b->isVisible())
            {
                const auto rc = b->model()->rowCount(QModelIndex());
                for (int i = 0; i < rc; ++i)
                    if (itemName == b->model()->data(b->model()->index(i, 0), Qt::AccessibleTextRole).toString())
                    {
                        return qsl("True");
                    }
            }
        }
    }
    return returnNotFound();
}


QString getContactListCounter()
{
    auto mainWidget_ = Utils::InterConnector::instance().getMainWindow(true);
    if (mainWidget_ != nullptr)
    {
        QList<QListView *> listitems = mainWidget_->findChildren<QListView *>();
        for (QListView* b : listitems)
        {
            auto acName = b->accessibleName();
            if (acName == qsl("AS search view_") && b->isVisible())
            {
                qDebug() << "... DEBUG Contact List Counter = " << b->model()->rowCount(QModelIndex());

                const auto rc = b->model()->rowCount(QModelIndex());
                if (rc >= 0)
                {
                    qDebug() << "... DEBUG Contact List Counter = " << rc;
                    return QString::number(rc);
                }
                else
                {
                    qDebug() << "... DEBUG Contact List is EMPTY";
                    return QString::number(0);
                }
            }
        }
    }
    qDebug() << "... DEBUG Widget <AS search view_> is not found.";
    return QString::number(0);
}


void clearTextField(const QString& elementName)
{
    auto mainWidget_ = Utils::InterConnector::instance().getMainWindow(true);
    if (mainWidget_ != nullptr)
    {
        QList<QLineEdit *> fields = mainWidget_->findChildren<QLineEdit *>();
        for (QLineEdit* f : fields)
        {
            auto acName = f->accessibleName();
            if (acName == elementName && f->isVisible())
            {
                f->clear();
            }
        }
    }
}


void copyTextToClipboard(const QString& inputText)
{
    QClipboard *clipboard = QGuiApplication::clipboard();
    clipboard->clear();
    clipboard->setText(inputText);
}


void setTextToField(const QString& textField, const QString& requestString)
{
    clearTextField(textField);
    copyTextToClipboard(requestString);

    if (findElementByAccessibleName(textField) != nullptr)
    {
        // Disabled, because it works only with ASCII text, not UTF-8
        // QTest::keyClicks(findElementByAccessibleName(textField), requestString, Qt::NoModifier, 1);
        //

        // It's too slow for large messages
        /*foreach(const QString &str, requestString)
        {
            copyTextToClipboard(str);
            QTest::qWait(50);
            QTest::keyClicks(findElementByAccessibleName(textField), QString::fromUtf8("v"), Qt::ControlModifier, 1);
        }*/
        auto requestedWidget = findElementByAccessibleName(textField);
        const QPoint widgetPos(requestedWidget->rect().center());
        QTest::mouseMove(requestedWidget, widgetPos, 100);
        QTest::qWait(100);
        QTest::keyClicks(requestedWidget, QString::fromUtf8("v"), Qt::ControlModifier, 100);

        //for new edit fields like login screen
        QList<QLineEdit *> fields = requestedWidget->findChildren<QLineEdit *>();
        qDebug() << "-> fields counter = " << fields.count();

        //smscode text field
        if (textField == qsl("AS lp codeEdit"))
        {
            if (fields.count() == 6 && fields.count() == requestString.count())
            {
                for (int i = 0; i < fields.count(); ++i)
                {
                    fields.at(i)->setText(requestString.at(i));
                }
            }
        }
        else
        {
            for (QLineEdit* f : fields)
            {
                f->clear();
                QTest::keyClicks(f, QString::fromUtf8("v"), Qt::ControlModifier, 100);
            }
        }
    }
}


void setSearchField(const QString& textField, const QString& requestString)
{
    auto mainWidget_ = Utils::InterConnector::instance().getMainWindow(true);
    if (mainWidget_ != nullptr)
    {
        QString search_type;
        QString search_field;

        if (textField == qsl("AS sw searchEdit_"))
        {
            qDebug() << "... DEBUG SEARCH VIA TOP SEARCH WIDGET";
            search_type = qsl("AS top searchWidget_");
            search_field = qsl("AS sw searchEdit_");
        }
        else if (textField == qsl("AS sidebar searchEdit_"))
        {
            qDebug() << "... DEBUG SEARCH IN SIDEBAR";
            search_type = qsl("AS sidebar searchWidget_");
            search_field = qsl("AS sw searchEdit_");
        }
        else if (textField == qsl("AS contacts searchEdit_"))
        {
            qDebug() << "... DEBUG SEARCH IN CONTACTS";
            search_type = qsl("AS cltp searchWidget_");
            search_field = qsl("AS sw searchEdit_");
        }
        else if (textField == qsl("AS selectmembers searchEdit_"))
        {
            qDebug() << "... DEBUG SEARCH IN SELECT MEMBERS";
            search_type = qsl("AS scfgc CreateGroupChat");
            search_field = qsl("AS sw searchEdit_");
        }

        QList<QWidget *> widgets = mainWidget_->findChildren<QWidget *>();
        for (QWidget* b : widgets)
        {
            auto acName = b->accessibleName();
            if (acName == search_type && b->isVisible())
            {
                const QList<QLineEdit *> fields = b->findChildren<QLineEdit *>();
                for (QLineEdit* f : fields)
                {
                    auto acName = f->accessibleName();
                    if (acName == search_field && f->isVisible())
                    {
                        qDebug() << "... DEBUG: SEARCH FIELD FOUND";
                        f->clear();

                        foreach(const QString &str, requestString)
                        {
                            copyTextToClipboard(str);
                            QTest::qWait(50);
                            QTest::keyClicks(f, QString::fromUtf8("v"), Qt::ControlModifier, 1);
                        }
                    }
                }
            }
        }
    }
}


QString getTextFieldContent(const QString& textFieldName)
{
    auto mainWidget_ = Utils::InterConnector::instance().getMainWindow(true);
    if (mainWidget_ != nullptr)
    {
        QList<QLineEdit *> fields = mainWidget_->findChildren<QLineEdit *>();
        for (QLineEdit* f : fields)
        {
            auto acName = f->accessibleName();
            if (acName == textFieldName && f->isVisible())
            {
                return f->text();
            }
        }
    }
    return returnNotFound();
}


QString getInputWidgetText(const QString& textFieldName)
{
    auto mainWidget_ = Utils::InterConnector::instance().getMainWindow(true);
    if (mainWidget_ != nullptr)
    {
        QList<QTextEdit *> fields = mainWidget_->findChildren<QTextEdit *>();
        for (QTextEdit* f : fields)
        {
            auto acName = f->accessibleName();
            if (acName == textFieldName && f->isVisible())
            {
                return f->toPlainText();
            }
        }
    }
    return returnNotFound();
}


QString getUinWithOpenedDialog()
{
    auto currentSelectedContact = Logic::getContactListModel()->selectedContact();

    if (!currentSelectedContact.isEmpty())
    {
        return currentSelectedContact;
    }
    return QString();
}


QString getLabelText(const QString& textFieldName)
{
    auto mainWidget_ = Utils::InterConnector::instance().getMainWindow(true);
    if (mainWidget_ != nullptr)
    {
        QList<QLabel *> labels = mainWidget_->findChildren<QLabel *>();
        for (QLabel* f : labels)
        {
            auto acName = f->accessibleName();
            if (acName == textFieldName && f->isVisible())
            {
                return f->text();
            }
        }
    }
    return returnNotFound();
}


QWidget* getLastMessageWidget()
{
    auto mainWidget_ = Utils::InterConnector::instance().getMainWindow(true);
    if (mainWidget_ != nullptr)
    {
        QList<QWidget *> widgets = mainWidget_->findChildren<QWidget *>();
        QString currentSelectedContact = qsl("AS HistoryControlPage") + getUinWithOpenedDialog();

        for (QWidget* b : widgets)
        {
            auto accName = b->accessibleName();
            if (!accName.isEmpty())
            {
                if (accName == currentSelectedContact && b->isVisible())
                {
                    const auto internalWidgets = b->findChildren<QWidget *>();
                    if (internalWidgets.isEmpty())
                    {
                        return nullptr;
                    }

                    QWidget* lastWidget = nullptr;
                    int message_geometry_value = std::numeric_limits<int>::min();

                    //Detecting last message item by geomtry
                    for (auto w : internalWidgets)
                    {
                        //auto widgetMessageItem = qobject_cast<Ui::MessageItem*>(w);
                        //if (widgetMessageItem)
                        //{
                        //    if (w->y() > message_geometry_value)
                        //    {
                        //        message_geometry_value = w->y();
                        //        lastWidget = widgetMessageItem;
                        //    }
                        //}

                        auto widgetComplexMessageItem = qobject_cast<Ui::ComplexMessage::ComplexMessageItem*>(w);
                        if (widgetComplexMessageItem)
                        {
                            if (w->y() > message_geometry_value)
                            {
                                message_geometry_value = w->y();
                                lastWidget = widgetComplexMessageItem;
                            }
                        }

                        auto widgetVoipEventItem = qobject_cast<Ui::VoipEventItem*>(w);
                        if (widgetVoipEventItem)
                        {
                            if (w->y() > message_geometry_value)
                            {
                                message_geometry_value = w->y();
                                lastWidget = widgetVoipEventItem;
                            }
                        }
                    }

                    //Working with latest message
                    if (lastWidget)
                    {
                        const QPoint widgetPos(lastWidget->rect().center());
                        QTest::mouseMove(lastWidget, widgetPos, 100);

                        //auto widgetMessageItem = qobject_cast<Ui::MessageItem*>(lastWidget);
                        //if (widgetMessageItem)
                        //{
                        //    qDebug() << "DEBUG: IT'S A TEXT WIDGET";
                        //}

                        auto widgetComplexMessageItem = qobject_cast<Ui::ComplexMessage::ComplexMessageItem*>(lastWidget);
                        if (widgetComplexMessageItem)
                        {
                            qDebug() << "... DEBUG: IT'S A COMPLEX WIDGET";
                        }

                        auto widgetVoipEventItem = qobject_cast<Ui::VoipEventItem*>(lastWidget);
                        if (widgetVoipEventItem)
                        {
                            qDebug() << "... DEBUG: IT'S A VOIP WIDGET";
                        }

                        return lastWidget;
                    }
                }
            }
        }
    }
    return nullptr;
}


void rightClickOnMessage()
{
    if (getLastMessageWidget() != nullptr)
    {
        const QPoint messagePos(getLastMessageWidget()->rect().center());
        QTest::mouseMove(getLastMessageWidget(), messagePos, 100);
        QTest::mouseClick(getLastMessageWidget(), Qt::RightButton, Qt::NoModifier, messagePos, 100);
    }
}


QWidget * getLastFileWidget()
{
    QWidget * not_found_widget = nullptr;
    auto mainWidget_ = Utils::InterConnector::instance().getMainWindow(true);
    if (mainWidget_ != nullptr)
    {
        QList<QWidget *> widgets = mainWidget_->findChildren<QWidget *>();
        QString currentSelectedContact = qsl("AS HistoryControlPage") + getUinWithOpenedDialog();
        for (QWidget* b : widgets)
        {
            auto accName = b->accessibleName();
            if (!accName.isEmpty() && b->isVisible())
            {
                if (accName == currentSelectedContact)
                {
                    const auto internalWidgets = b->findChildren<QWidget *>();
                    QWidget* latestImage = nullptr;
                    int message_geometry_value = std::numeric_limits<int>::min();
                    for (auto w : internalWidgets)
                    {
                        auto widgetFileBlock = qobject_cast<Ui::ComplexMessage::FileSharingBlock*>(w);
                        if (widgetFileBlock)
                        {
                            if (w->mapToGlobal(w->pos()).y() > message_geometry_value)
                            {
                                message_geometry_value = w->mapToGlobal(w->pos()).y();
                                latestImage = widgetFileBlock;
                            }
                        }
                    }
                    return latestImage;
                }
            }
        }
    }
    return not_found_widget;
}


void clickToFile()
{
    if (getLastFileWidget() != nullptr)
    {
        QTest::mouseMove(getLastFileWidget(), QPoint(45, 15), 100);
        QTest::qWait(200);
        QTest::mouseClick(getLastFileWidget(), Qt::LeftButton, Qt::NoModifier, QPoint(45, 15), 100);
    }
}


QString getLastFileInfo(const QString& requestInfo)
{
    auto mainWidget_ = Utils::InterConnector::instance().getMainWindow(true);
    if (mainWidget_ != nullptr)
    {
        QList<QWidget *> widgets = mainWidget_->findChildren<QWidget *>();
        const QString& currentSelectedContact = qsl("AS HistoryControlPage") + getUinWithOpenedDialog();
        for (QWidget* b : widgets)
        {
            auto accName = b->accessibleName();
            if (!accName.isEmpty() && b->isVisible())
            {
                if (accName == currentSelectedContact)
                {
                    const auto internalWidgets = b->findChildren<QWidget *>();
                    QWidget* itemWidget = nullptr;
                    int message_geometry_value = std::numeric_limits<int>::min();
                    for (auto w : internalWidgets)
                    {
                        auto widgetFileBlock = qobject_cast<Ui::ComplexMessage::FileSharingBlock*>(w);
                        if (widgetFileBlock)
                        {
                            if (w->mapToGlobal(w->pos()).y() > message_geometry_value)
                            {
                                message_geometry_value = w->mapToGlobal(w->pos()).y();
                                itemWidget = widgetFileBlock;
                            }
                        }
                    }

                    if (!itemWidget)
                    {
                        return returnNotFound();
                    }

                    if (QString::fromUtf8(itemWidget->metaObject()->className()) == qsl("Ui::ComplexMessage::FileSharingBlock"))
                    {
                        if (requestInfo == qsl("getFilename"))
                        {
                            return qobject_cast<Ui::ComplexMessage::FileSharingBlock*>(itemWidget)->getFilename();
                        }
                        else if (requestInfo == qsl("getLink"))
                        {
                            qDebug() << "getLink()";
                            return qobject_cast<Ui::ComplexMessage::FileSharingBlock*>(itemWidget)->getLink();
                        }
                        else if (requestInfo == qsl("getFileLocalPath"))
                        {
                            return qobject_cast<Ui::ComplexMessage::FileSharingBlock*>(itemWidget)->getFileLocalPath();
                        }
                        else if (requestInfo == qsl("getFileSize"))
                        {
                            return QString::number(qobject_cast<Ui::ComplexMessage::FileSharingBlock*>(itemWidget)->getFileSize());
                        }
                        else if (requestInfo == qsl("getDirectUri"))
                        {
                            return qobject_cast<Ui::ComplexMessage::FileSharingBlock*>(itemWidget)->getDirectUri();
                        }
                        else if (requestInfo == qsl("isPreviewReady"))
                        {
                            if (qobject_cast<Ui::ComplexMessage::FileSharingBlock*>(itemWidget)->isPreviewReady())
                            {
                                return qsl("True");
                            }
                            else
                            {
                                return returnNotFound();
                            }
                        }
                        else if (requestInfo == qsl("originSizeScaled"))
                        {
                            qDebug() << "DEBUG: IT'S originSizeScaled()";
                            auto preview_width = qobject_cast<Ui::ComplexMessage::FileSharingBlock*>(itemWidget)->originSizeScaled().width();
                            auto preview_height = qobject_cast<Ui::ComplexMessage::FileSharingBlock*>(itemWidget)->originSizeScaled().height();
                            qDebug() << "DEBUG: preview_width = " << preview_width;
                            qDebug() << "DEBUG: preview_height = " << preview_height;

                            if (preview_width && preview_width > 0) {
                                if (preview_height && preview_height > 0)
                                {
                                    return qsl("True");
                                }
                            }
                        }
                        else if (requestInfo == qsl("isFileDownloaded"))
                        {
                            qDebug() << "DEBUG: ->isFileDownloaded() = " << qobject_cast<Ui::ComplexMessage::FileSharingBlock*>(itemWidget)->isFileDownloaded();
                            auto status = qobject_cast<Ui::ComplexMessage::FileSharingBlock*>(itemWidget)->isFileDownloaded();
                            if (status)
                            {
                                return qsl("True");
                            }
                            else
                            {
                                return returnNotFound();
                            }
                        }
                        else if (requestInfo == qsl("isFileDownloading"))
                        {
                            qDebug() << "DEBUG: ->isFileDownloading() = " << qobject_cast<Ui::ComplexMessage::FileSharingBlock*>(itemWidget)->isFileDownloading();
                            auto status = qobject_cast<Ui::ComplexMessage::FileSharingBlock*>(itemWidget)->isFileDownloading();
                            if (status)
                            {
                                return qsl("True");
                            }
                            else
                            {
                                return returnNotFound();
                            }
                        }
                        else if (requestInfo == qsl("isFileUploading"))
                        {
                            qDebug() << "DEBUG: ->isFileUploading() = " << qobject_cast<Ui::ComplexMessage::FileSharingBlock*>(itemWidget)->isFileUploading();
                            auto status = qobject_cast<Ui::ComplexMessage::FileSharingBlock*>(itemWidget)->isFileUploading();
                            if (status)
                            {
                                return qsl("True");
                            }
                            else
                            {
                                return returnNotFound();
                            }
                        }
                        else if (requestInfo == qsl("uploaded"))
                        {
                            qDebug() << "DEBUG: ->isFileUploading() = " << qobject_cast<Ui::ComplexMessage::FileSharingBlock*>(itemWidget)->isFileUploading();
                            auto status = qobject_cast<Ui::ComplexMessage::FileSharingBlock*>(itemWidget)->isFileUploading();
                            if (status)
                            {
                                return returnNotFound();
                            }
                            else
                            {
                                return qsl("True");
                            }
                        }
                        else if (requestInfo == qsl("isGifImage"))
                        {
                            qDebug() << "DEBUG: ->isGifImage() = " << qobject_cast<Ui::ComplexMessage::FileSharingBlock*>(itemWidget)->isGifImage();
                            auto status = qobject_cast<Ui::ComplexMessage::FileSharingBlock*>(itemWidget)->isGifImage();
                            if (status)
                            {
                                return qsl("True");
                            }
                            else
                            {
                                return returnNotFound();
                            }
                        }
                        else if (requestInfo == qsl("isImage"))
                        {
                            qDebug() << "DEBUG: ->isImage() = " << qobject_cast<Ui::ComplexMessage::FileSharingBlock*>(itemWidget)->isImage();
                            auto status = qobject_cast<Ui::ComplexMessage::FileSharingBlock*>(itemWidget)->isImage();
                            if (status)
                            {
                                return qsl("True");
                            }
                            else
                            {
                                return returnNotFound();
                            }
                        }
                        else if (requestInfo == qsl("isPlainFile"))
                        {
                            qDebug() << "DEBUG: ->isPlainFile() = " << qobject_cast<Ui::ComplexMessage::FileSharingBlock*>(itemWidget)->isPlainFile();
                            auto status = qobject_cast<Ui::ComplexMessage::FileSharingBlock*>(itemWidget)->isPlainFile();
                            if (status)
                            {
                                return qsl("True");
                            }
                            else
                            {
                                return returnNotFound();
                            }
                        }
                        else if (requestInfo == qsl("isVideo"))
                        {
                            qDebug() << "DEBUG: ->isVideo() = " << qobject_cast<Ui::ComplexMessage::FileSharingBlock*>(itemWidget)->isVideo();
                            auto status = qobject_cast<Ui::ComplexMessage::FileSharingBlock*>(itemWidget)->isVideo();
                            if (status)
                            {
                                return qsl("True");
                            }
                            else
                            {
                                return returnNotFound();
                            }
                        }
                        else if (requestInfo == qsl("isPreviewable"))
                        {
                            qDebug() << "DEBUG: ->isPreviewable() = " << qobject_cast<Ui::ComplexMessage::FileSharingBlock*>(itemWidget)->isPreviewable();
                            auto status = qobject_cast<Ui::ComplexMessage::FileSharingBlock*>(itemWidget)->isPreviewable();
                            if (status)
                            {
                                return qsl("True");
                            }
                            else
                            {
                                return returnNotFound();
                            }
                        }
                        else if (requestInfo == qsl("isSelected"))
                        {
                            qDebug() << "DEBUG: ->isSelected() = " << qobject_cast<Ui::ComplexMessage::FileSharingBlock*>(itemWidget)->isSelected();
                            auto status = qobject_cast<Ui::ComplexMessage::FileSharingBlock*>(itemWidget)->isSelected();
                            if (status)
                            {
                                return qsl("True");
                            }
                            else
                            {
                                return returnNotFound();
                            }
                        }
                        else if (requestInfo == qsl("isAutoplay"))
                        {
                            qDebug() << "DEBUG: ->isAutoplay() = " << qobject_cast<Ui::ComplexMessage::FileSharingBlock*>(itemWidget)->isAutoplay();
                            auto status = qobject_cast<Ui::ComplexMessage::FileSharingBlock*>(itemWidget)->isAutoplay();
                            if (status)
                            {
                                return qsl("True");
                            }
                            else
                            {
                                return returnNotFound();
                            }
                        }
                        else if (requestInfo == qsl("isOutgoing"))
                        {
                            qDebug() << "DEBUG: ->isOutgoing() = " << qobject_cast<Ui::ComplexMessage::FileSharingBlock*>(itemWidget)->isOutgoing();
                            auto status = qobject_cast<Ui::ComplexMessage::FileSharingBlock*>(itemWidget)->isOutgoing();
                            if (status)
                            {
                                return qsl("True");
                            }
                            else
                            {
                                return returnNotFound();
                            }
                        }
                        else
                        {
                            return qsl("Parameters for this requests: [") +
                                qsl("getFilename, ") +
                                qsl("getLink, ") +
                                qsl("getFileLocalPath, ") +
                                qsl("getFileSize, ") +
                                qsl("getDirectUri, ") +
                                qsl("isPreviewReady, ") +
                                qsl("getOriginalPreviewSize, ") +
                                qsl("isFileDownloaded, ") +
                                qsl("isFileDownloading, ") +
                                qsl("isFileUploading, ") +
                                qsl("uploaded, ") +
                                qsl("isGifImage, ") +
                                qsl("isImage, ") +
                                qsl("isPlainFile, ") +
                                qsl("isVideo, ") +
                                qsl("isPreviewable, ") +
                                qsl("isSelected, ") +
                                qsl("isAutoplay, ") +
                                qsl("isOutgoing") +
                                qsl("]");
                        }
                    }

                    return returnNotFound();
                }
            }
        }
    }
    return returnNotFound();
}


QString getLastMessage(const QString& item)
{
    if (item == qsl("message_id"))
    {
        qDebug() << "DEBUG: looking for <message_id>";
    }
    else if (item == qsl("message_text"))
    {
        qDebug() << "DEBUG: looking for <message_text>";
    }
    else if (item == qsl("source"))
    {
        qDebug() << "DEBUG: looking for <source>";
    }
    else if (item == qsl("is_outgoing"))
    {
        qDebug() << "DEBUG: looking for <is_outgoing>";
    }
    else
    {
        qDebug() << "DEBUG: Incorrect request field";
        return qsl("Parameters for this requests: [ message_id, message_text, source, is_outgoing ]");
    }

    auto mainWidget_ = Utils::InterConnector::instance().getMainWindow(true);
    if (mainWidget_ != nullptr)
    {
        QList<QWidget *> widgets = mainWidget_->findChildren<QWidget *>();
        QString currentSelectedContact = qsl("AS HistoryControlPage") + getUinWithOpenedDialog();

        for (QWidget* b : widgets)
        {
            auto accName = b->accessibleName();
            if (!accName.isEmpty())
            {
                if (accName == currentSelectedContact && b->isVisible())
                {
                    const auto internalWidgets = b->findChildren<QWidget *>();
                    if (internalWidgets.isEmpty())
                    {
                        return QString();
                    }

                    QWidget* lastWidget = nullptr;
                    int message_geometry_value = std::numeric_limits<int>::min();

                    //Detecting last message item by geomtry
                    for (auto w : internalWidgets)
                    {

                        auto widgetComplexMessageItem = qobject_cast<Ui::ComplexMessage::ComplexMessageItem*>(w);
                        if (widgetComplexMessageItem)
                        {
                            if (w->y() > message_geometry_value)
                            {
                                message_geometry_value = w->y();
                                lastWidget = widgetComplexMessageItem;
                            }
                        }

                        auto widgetVoipEventItem = qobject_cast<Ui::VoipEventItem*>(w);
                        if (widgetVoipEventItem)
                        {
                            if (w->y() > message_geometry_value)
                            {
                                message_geometry_value = w->y();
                                lastWidget = widgetVoipEventItem;
                            }
                        }
                    }

                    //Working with latest message
                    if (lastWidget)
                    {
                        const QPoint widgetPos(lastWidget->rect().center());
                        QTest::mouseMove(lastWidget, widgetPos, 100);

                        auto widgetComplexMessageItem = qobject_cast<Ui::ComplexMessage::ComplexMessageItem*>(lastWidget);
                        if (widgetComplexMessageItem)
                        {
                            qDebug() << "DEBUG: IT'S A COMPLEX WIDGET";

                            if (item == qsl("is_outgoing"))
                            {
                                if (widgetComplexMessageItem->isOutgoing())
                                {
                                    return qsl("True");
                                }
                                else
                                {
                                    return returnNotFound();
                                }
                            }

                            if (item == qsl("message_id"))
                            {
                                qDebug() << "widgetComplexMessageItem->getId() = " << widgetComplexMessageItem->getId();
                                return QString::number(widgetComplexMessageItem->getId());
                            }

                            if (widgetComplexMessageItem->getBlockCount() < 2)
                            {
                                //Simple Message Item
                                if (item == qsl("message_text"))
                                {
                                    qDebug() << "widgetComplexMessageItem->getBlockWidgets()[0]->getSourceText() = " << widgetComplexMessageItem->getBlockWidgets()[0]->getSourceText();
                                    return widgetComplexMessageItem->getBlockWidgets()[0]->getSourceText();
                                }
                                else if (item == qsl("source"))
                                {
                                    auto source_block = qobject_cast<Ui::ComplexMessage::TextBlock *>(widgetComplexMessageItem->getBlockWidgets()[0]);
                                    if (source_block->getTextStyle())
                                    {
                                        return qsl("True");
                                    }
                                }
                            }
                            else
                            {
                                //Quotes, Forwards, Replies, etc
                                qDebug() << "IT'S COMPLEX MESSAGE WITH 2 OR MORE BLOCKS";
                                for (int i = 0; i < widgetComplexMessageItem->getBlockCount(); i++)
                                {
                                    qDebug() << "widgetComplexMessageItem->getBlockWidgets()[i] = " << widgetComplexMessageItem->getBlockWidgets()[i];
                                }
                                qDebug() << "DEBUG: NOT IMPLEMENTED!!!";
                                return returnNotFound();
                            }
                        }

                        auto widgetVoipEventItem = qobject_cast<Ui::VoipEventItem*>(lastWidget);
                        if (widgetVoipEventItem)
                        {
                            qDebug() << "DEBUG: IT'S A VOIP WIDGET";
                            if (item == qsl("is_outgoing"))
                            {
                                if (widgetVoipEventItem->isOutgoing())
                                {
                                    return qsl("True");
                                }
                                else
                                {
                                    return returnNotFound();
                                }
                            }
                            if (item == qsl("message_id"))
                            {
                                qDebug() << "widgetVoipEventItem->getId() = " << widgetVoipEventItem->getId();
                                return QString::number(widgetVoipEventItem->getId());
                            }
                            else if (item == qsl("message_text"))
                            {
                                return returnNotFound();
                            }
                            else if (item == qsl("source"))
                            {
                                return returnNotFound();
                            }
                        }
                    }
                }
            }
        }
    }
    return returnNotFound();
}


void printAllWidgetsFromMainWindow()
{
    auto mainWidget_ = Utils::InterConnector::instance().getMainWindow(true);
    if (mainWidget_ != nullptr)
    {
        QList<QWidget *> widgets = mainWidget_->findChildren<QWidget *>();
        for (QWidget* b : widgets)
        {
            auto accName = b->accessibleName();
            if (!accName.isEmpty())
            {
                if (accName == qsl("AS contactPage"))
                {
                    printObjectChildren(b);
                }
            }
        }
    }
}


void printAllMenuDialogs()
{
    auto mainWidget_ = Utils::InterConnector::instance().getMainWindow(true);
    if (mainWidget_ != nullptr)
    {
        QList<QMenu *> menus = mainWidget_->findChildren<QMenu *>();
        for (QWidget* m : menus)
        {
            auto accName = m->accessibleName();
            if (!accName.isEmpty())
            {
                qDebug() << "qmenu = " << accName;
                QTest::qWait(100);
            }
        }
    }
}


void clickOnDialogFromRecents(const QString& contactName, const QString& mouseButton)
{
    auto mainWidget_ = Utils::InterConnector::instance().getMainWindow(true);
    if (mainWidget_ != nullptr)
    {
        QList<QWidget *> widgets = mainWidget_->findChildren<QWidget *>();
        for (QWidget* b : widgets)
        {
            auto acName = b->accessibleName();
            if (acName == qsl("AS recentsView_") || acName == qsl("AS contactsTab_") && b->isVisible())
            {
                auto ab = qobject_cast<QListView*>(b);
                if (!ab)
                {
                    qDebug() << "DEBUG: QObject_cast failed " << b->accessibleName();
                    continue;
                }

                auto ndx = Logic::getRecentsModel()->contactIndex(contactName);
                if (!ndx.isValid())
                {
                    qDebug() << "DEBUG: Index is invalid " << b->accessibleName();
                    //break;
                }
                auto rect = ab->visualRect(ndx);
                auto pos = ab->mapToParent(rect.center());

                QTest::mouseMove(ab, pos, 300);
                QTest::qWait(1000);

                //QTestEventList events;
                if (mouseButton == qsl("LeftButton")) {
                    QSignalSpy spy(ab, SIGNAL(clicked(QModelIndex)));

                    qDebug() << "... DEBUG: LeftClick()";
                    //events.addMouseClick(Qt::LeftButton, Qt::NoModifier, pos);
                    //events.addDelay(500);
                    //events.simulate(ab->viewport());
                    QTest::mousePress(ab->viewport(), Qt::LeftButton, 0, pos, 300);
                    QTest::mouseRelease(ab->viewport(), Qt::LeftButton, 0, pos, 300);
                    qDebug() << "...DEBUG: spy.count() = " << spy.count();
                    //QCOMPARE(spy.count(), 1);
                    qDebug() << "... DEBUG: Clicked!";
                }
                else if (mouseButton == qsl("RightButton"))
                {
                    qDebug() << "... DEBUG: RightClick()";
                    QTest::mousePress(ab->viewport(), Qt::RightButton, 0, pos, 300);
                    QTest::mouseRelease(ab->viewport(), Qt::RightButton, 0, pos, 300);
                    qDebug() << "... DEBUG: Clicked!";
                }
            }
        }
    }
}


void clickToDialog(const QString& tabName, const QString& contactName, const QString& mouseButton)
{
    auto mainWidget_ = Utils::InterConnector::instance().getMainWindow(true);
    if (mainWidget_ != nullptr)
    {
        QString tabNameAccessible;
        if (tabName == qsl("contacts"))
        {
            tabNameAccessible = qsl("AS search view_");
        }
        else if (tabName == qsl("recents"))
        {
            tabNameAccessible = qsl("AS recentsView_");
        }
        else {
            qDebug() << "... DEBUG: tabName is not correct";
            qDebug() << "... DEBUG: Parameters for this <tabName>: [ contacts, recents ]";
        }

        QList<QListView *> listitems = mainWidget_->findChildren<QListView *>();
        for (QListView* b : listitems)
        {
            auto acName = b->accessibleName();

            if (acName == tabNameAccessible && b->isVisible())
            {
                const auto rc = b->model()->rowCount(QModelIndex());

                for (int i = 0; i < rc; ++i)
                {
                    if (contactName == b->model()->data(b->model()->index(i, 0), Qt::AccessibleTextRole).toString())
                    {
                        auto ndx = b->model()->index(i, 0);
                        if (!ndx.isValid())
                            qDebug() << "... DEBUG: Index is invalid";

                        auto rect = b->visualRect(ndx);
                        auto pos = b->mapToParent(rect.center());

                        QTest::mouseMove(b, pos, 100);
                        QTest::qWait(100);

                        //LeftClick();
                        //QTestEventList events;
                        //events.addMouseClick(Qt::LeftButton, Qt::NoModifier, pos);
                        //events.addDelay(500);
                        //events.simulate(b->viewport());
                        if (mouseButton == qsl("LeftButton")) {
                            QSignalSpy spy(b, SIGNAL(clicked(QModelIndex)));
                            qDebug() << "... DEBUG: LeftClick()";
                            //events.addMouseClick(Qt::LeftButton, Qt::NoModifier, pos);
                            //events.addDelay(500);
                            //events.simulate(b->viewport());
                            QTest::mousePress(b->viewport(), Qt::LeftButton, 0, pos, 300);
                            QTest::mouseRelease(b->viewport(), Qt::LeftButton, 0, pos, 300);
                            qDebug() << "... DEBUG: spy.count() = " << spy.count();
                            //QCOMPARE(spy.count(), 1);
                            qDebug() << "... DEBUG: Clicked!";
                        }
                        else if (mouseButton == qsl("RightButton"))
                        {
                            qDebug() << "... DEBUG: RightClick()";
                            QTest::mousePress(b->viewport(), Qt::RightButton, 0, pos, 300);
                            QTest::mouseRelease(b->viewport(), Qt::RightButton, 0, pos, 300);
                            qDebug() << "... DEBUG: Clicked!";
                        }
                    }
                }
            }
        }
    }
}


void clickToDialogFromContacts(const QString& contactName, const QString& mouseButton)
{
    qDebug() << "... DEBUG: clickToDialog(contacts, contactName, mouseButton)";
    clickToDialog(qsl("contacts"), contactName, mouseButton);
}


void clickToDialogFromRecents(const QString& contactName, const QString& mouseButton)
{
    qDebug() << "... DEBUG: clickToDialog(recents, contactName, mouseButton)";
    clickToDialog(qsl("recents"), contactName, mouseButton);
}


void selectLanguageFromSetting(const QString& languageName)
{
    auto mainWidget_ = Utils::InterConnector::instance().getMainWindow(true);
    if (mainWidget_ != nullptr)
    {
        QList<QWidget *> widgets = mainWidget_->findChildren<QWidget *>();
        for (QWidget* b : widgets)
        {
            auto acName = b->accessibleName();
            if (acName == qsl("AS settings languageList") && b->isVisible())
            {
                auto ab = qobject_cast<QListView*>(b);
                if (!ab)
                {
                    qDebug() << "DEBUG: QObject_cast failed";
                    continue;
                }

                //auto ndx = Logic::getRecentsModel()->contactIndex(languageName);
                //auto ndx = Ui::get
                //if (!ndx.isValid())
                //    qDebug() << "DEBUG: Index is invalid";

                //auto rect = ab->visualRect(ndx);
                //auto pos = ab->mapToParent(rect.center());

                QTest::mouseMove(ab, QPoint(0,0), 100);
                //QTest::qWait(1000);

                ////LeftClick();
                //QTestEventList events;
                //events.addMouseClick(Qt::LeftButton, Qt::NoModifier, pos);
                //events.addDelay(500);
                //events.simulate(ab->viewport());
            }
        }
    }
}


QString getSelectedChatId()
{
    auto mainWidget_ = Utils::InterConnector::instance().getMainWindow(true);
    if (mainWidget_ != nullptr)
    {
        QList<QWidget *> widgets = mainWidget_->findChildren<QWidget *>();
        for (QWidget* b : widgets)
        {
            auto acName = b->accessibleName();
            if (acName == qsl("AS recentsView_") && b->isVisible())
            {
                const QString selected_chat_id = Logic::getContactListModel()->selectedContact();
                qDebug() << "DEBUG: selected_chat_id = " << selected_chat_id;
                return selected_chat_id;
            }
        }
    }
    return returnNotFound();
}


void openItemFromRecentsDropDownMenu(const QString& serviceName)
{
    auto mainWidget_ = Utils::InterConnector::instance().getMainWindow(true);
    if (mainWidget_ != nullptr)
    {
        QList<QWidget *> widgets = mainWidget_->findChildren<QWidget *>();

        int service_index;
        qDebug() << "DEBUG: serviceName = " << serviceName;

        for (QWidget* b : widgets)
        {
            auto acName = b->accessibleName();
            if (acName == qsl("AS recentsView_") && b->isVisible())
            {
                auto ab = qobject_cast<QListView*>(b);
                if (!ab)
                {
                    qDebug() << "DEBUG: QObject_cast to <QListView> failed";
                    continue;
                }

                const auto clwhd = qobject_cast<const Logic::ContactListWithHeaders*>(ab->model());
                if (!clwhd)
                {
                    qDebug() << "DEBUG: QObject_cast to <ContactListWithHeaders> failed";
                    continue;
                }

                if (serviceName == qsl("add_contact"))
                {
                    service_index = clwhd->getIndexAddContact();
                    qDebug() << "DEBUG: add_contact_index = " << service_index;
                }

                if (serviceName == qsl("new_groupchat"))
                {
                    service_index = clwhd->getIndexCreateGroupchat();
                    qDebug() << "DEBUG: create_groupchat_index = " << service_index;
                }

                if (serviceName == qsl("new_channel"))
                {
                    service_index = clwhd->getIndexNewChannel();
                    qDebug() << "DEBUG: create_channel = " << service_index;
                }
                /*
                else
                {
                    qDebug() << "Parameters for this requests: [add_contact, new_groupchat, new_channel]";
                }*/

                qDebug() << "DEBUG: service_index << " << service_index;
                auto rect = ab->visualRect(clwhd->index(service_index));
                auto pos = ab->mapToParent(rect.center());

                QTest::mouseMove(ab, pos, 100);
                QTest::qWait(1000);
                //LeftClick();
                QTestEventList events;
                events.addMouseClick(Qt::LeftButton, Qt::NoModifier, pos);
                events.addDelay(500);
                events.simulate(ab->viewport());
            }
        }
    }
}


void openItemFromSearch(const QString& itemName)
{
    auto mainWidget_ = Utils::InterConnector::instance().getMainWindow(true);
    if (mainWidget_ != nullptr)
    {
        QList<QListView *> listitems = mainWidget_->findChildren<QListView *>();
        for (QListView* b : listitems)
        {
            auto acName = b->accessibleName();
            if (acName == qsl("AS search view_") && b->isVisible())
            {
                const auto rc = b->model()->rowCount(QModelIndex());

                for (int i = 0; i < rc; ++i)
                    if (itemName == b->model()->data(b->model()->index(i, 0), Qt::AccessibleTextRole).toString())
                    {

                        auto ndx = b->model()->index(i, 0);
                        if (!ndx.isValid())
                            qDebug() << "DEBUG: Index is invalid";

                        auto rect = b->visualRect(ndx);
                        auto pos = b->mapToParent(rect.center());

                        QTest::mouseMove(b, pos, 100);
                        QTest::qWait(1000);
                        //LeftClick();
                        QTestEventList events;
                        events.addMouseClick(Qt::LeftButton, Qt::NoModifier, pos);
                        events.addDelay(500);
                        events.simulate(b->viewport());

                    }
            }
        }
    }
}


void clickMoreInItemFromSidebarMembers(const QString& itemName)
{
    auto mainWidget_ = Utils::InterConnector::instance().getMainWindow(true);
    if (mainWidget_ != nullptr)
    {
        QList<QListView *> listitems = mainWidget_->findChildren<QListView *>();
        for (QListView* b : listitems)
        {
            auto acName = b->accessibleName();
            if (acName == qsl("AS search view_") && b->isVisible())
            {
                const auto rc = b->model()->rowCount(QModelIndex());

                for (int i = 0; i < rc; ++i)
                    if (itemName == b->model()->data(b->model()->index(i, 0), Qt::AccessibleTextRole).toString())
                    {

                        auto ndx = b->model()->index(i, 0);
                        if (!ndx.isValid())
                            qDebug() << "DEBUG: Index is invalid";

                        auto rect = b->visualRect(ndx);
                        qDebug() << "DEBUG: rect.topRight().x() = " << rect.topRight().x();
                        qDebug() << "DEBUG: rect.topRight().y() = " << rect.topRight().y();
                        auto pos = b->mapToParent(QPoint(int(rect.topRight().x() - 20), int(rect.topRight().y() + 20)));

                        QTest::mouseMove(b, pos, 100);
                        QTest::qWait(1000);
                        //LeftClick();
                        QTestEventList events;
                        events.addMouseClick(Qt::LeftButton, Qt::NoModifier, pos);
                        events.addDelay(500);
                        events.simulate(b->viewport());

                    }
            }
        }
    }
}


void sendFileToClipboard(const QString& fileName)
{
    QClipboard *clipboard = QGuiApplication::clipboard();

    QFile inputFile(fileName);
    if (!inputFile.open(QIODevice::ReadOnly))
    {
        qDebug() << "DEBUG: Unable to read file!";
    }
    else
    {
        QMimeData* mimeData = new QMimeData();
        mimeData->setUrls({ QUrl::fromLocalFile(fileName) });
        clipboard->setMimeData(mimeData);
    }
}


void attachFileFromClipboard()
{
    auto mainWidget_ = Utils::InterConnector::instance().getMainWindow(true);
    if (mainWidget_ != nullptr)
    {
        QList<QWidget *> widgets = mainWidget_->findChildren<QWidget *>();
        for (QWidget* b : widgets)
        {
            auto acName = b->accessibleName();
            if (acName == qsl("AS inputwidget textEdit_") && b->isVisible())
            {
                QTest::keyClicks(b, QString::fromUtf8("v"), Qt::ControlModifier, 1);
            }
        }
    }
}


void clickAddContact(const QString& contactAim)
{
    auto mainWidget_ = Utils::InterConnector::instance().getMainWindow(true);
    if (mainWidget_ != nullptr)
    {
        QList<QListView *> listitems = mainWidget_->findChildren<QListView *>();
        for (QListView* b : listitems)
        {
            auto acName = b->accessibleName();
            if (acName == qsl("AS search view_") && b->isVisible())
            {
                const auto rc = b->model()->rowCount(QModelIndex());
                for (int i = 0; i < rc; ++i)
                    if (contactAim == b->model()->data(b->model()->index(i, 0), Qt::AccessibleTextRole).toString())
                    {
                        auto ndx = b->model()->index(i, 0);
                        if (!ndx.isValid())
                            qDebug() << "DEBUG: Index is invalid";

                        auto rect = b->visualRect(ndx);
                        auto pos = b->mapToParent(rect.center());

                        //HardCode for positioning "+" element
#ifdef _WIN32
                        int button_plus_x = rect.right() - 46;
#endif
#ifdef __linux__
                        int button_plus_x = rect.right() - 20;
#endif
#ifdef __APPLE__
                        int button_plus_x = rect.right() - 20;
#endif

                        QPoint button_add(button_plus_x, pos.y());
                        QTest::mouseMove(b, button_add, 100);
                        QTest::qWait(1000);
                        //LeftClick();
                        QTestEventList events;
                        events.addMouseClick(Qt::LeftButton, Qt::NoModifier, button_add);
                        events.addDelay(500);
                        events.simulate(b->viewport());
        }
    }
}
    }
}


void quitFromApplication()
{
    auto mainWidget_ = Utils::InterConnector::instance().getMainWindow(true);
    if (mainWidget_ != nullptr)
    {
        QTest::keyClicks(mainWidget_, qsl("q"), Qt::ControlModifier, 1);
    }
}


void editLastMessage()
{
    auto mainWidget_ = Utils::InterConnector::instance().getMainWindow(true);
    if (mainWidget_ != nullptr)
    {
        QList<QWidget *> widgets = mainWidget_->findChildren<QWidget *>();
        QString currentSelectedContact = qsl("AS HistoryControlPage") + getUinWithOpenedDialog();
        for (QWidget* b : widgets)
        {
            auto accName = b->accessibleName();
            if (!accName.isEmpty())
            {
                if (accName == currentSelectedContact && b->isVisible())
                {
                    QTest::keyClick(b, Qt::Key_Up);
                }
            }
        }
    }
}


void closeGalleryPreviewer()
{
    qDebug() << "... DEBUG: closeGalleryPreviewer()";
    auto mainWidget_ = Utils::InterConnector::instance().getMainWindow(true);
    if (mainWidget_ != nullptr)
    {
        QList<QWidget *> widgets = mainWidget_->findChildren<QWidget *>();
        for (QWidget* b : widgets)
        {
            auto accName = b->accessibleName();
            if (!accName.isEmpty())
            {
                if (accName == qsl("AS gw imageViewer_"))
                {
                    qDebug() << "... DEBUG: AS gw imageViewer_ found!";
                    QTest::keyClick(b, Qt::Key_Escape);
                    break;
                }
            }
        }
    }
}


QString inputRequest(QByteArray strReply)
{
    const QString EMPTY_STRING = qsl("None");
    const QString OK_REQUEST = qsl("Ok");

    if (QString::fromUtf8(strReply).isEmpty())
    {
        return EMPTY_STRING;
    }


    //qDebug() << "raw strReply = " << strReply;
    QStringList tokens = QString::fromUtf8(strReply).split(QRegExp(qsl("[ \r\n][ \r\n]*")));
    qDebug() << "DEBUG: tokens = " << tokens;

    //
    // http://127.0.0.1:33333/webconsole?action=click&name=RecentsView&value=Qt%20ICQ
    //
    QUrlQuery request(tokens[1]);
    qDebug() << "DEBUG: request.queryItems()" << request.queryItems();

    auto param_action = QUrl(request.queryItemValue(qsl("/webconsole?action"), QUrl::FullyDecoded).toLower());
    auto param_name = QUrl(request.queryItemValue(qsl("name"), QUrl::FullyDecoded));
    auto param_value = QUrl(request.queryItemValue(qsl("value"), QUrl::FullyDecoded));

    auto action = QUrl::fromPercentEncoding(param_action.toEncoded());
    auto name = QUrl::fromPercentEncoding(param_name.toEncoded());
    auto value = QUrl::fromPercentEncoding(param_value.toEncoded());

    qDebug() << "\n";
    qDebug() << "DEBUG: URL action = " << action;
    qDebug() << "DEBUG: URL name = " << name;
    qDebug() << "DEBUG: URL value = " << value;
    qDebug() << "\n";

    if (action == qsl("click"))
    {
        clickToElement(name);
        return OK_REQUEST;
    }
    else if (action == qsl("set"))
    {
        setTextToField(name, value);
        return OK_REQUEST;
    }
    else if (action == qsl("bringapptofront"))
    {
        bringAppToFront();
        return OK_REQUEST;
    }
    else if (action == qsl("search"))
    {
        setSearchField(name, value);
        return OK_REQUEST;
    }
    else if (action == qsl("gettextfield"))
    {
        return getTextFieldContent(name);
    }
    else if (action == qsl("getinputwidgettext"))
    {
        return getInputWidgetText(name);
    }
    else if (action == qsl("getlabeltext"))
    {
        return getLabelText(name);
    }
    else if (action == qsl("getselectedchatid"))
    {
        return getSelectedChatId();
    }
    else if (action == qsl("clickondialogfromrecents"))
    {
        clickOnDialogFromRecents(name, value);
        return OK_REQUEST;
    }
    else if (action == qsl("clicktodialogfromrecents"))
    {
        clickToDialogFromRecents(name, value);
        return OK_REQUEST;
    }
    else if (action == qsl("clicktodialogfromcontacts"))
    {
        clickToDialogFromContacts(name, value);
        return OK_REQUEST;
    }

    else if (action == qsl("selectlanguagefromsetting"))
    {
        selectLanguageFromSetting(name);
        return OK_REQUEST;
    }
    else if (action == qsl("openitemfromrecentsdropdownmenu"))
    {
        openItemFromRecentsDropDownMenu(name);
        return OK_REQUEST;
    }
    else if (action == qsl("openitemfromsearch"))
    {
        openItemFromSearch(name);
        return OK_REQUEST;
    }
    else if (action == qsl("clickmoreinitemfromsidebarmembers"))
    {
        clickMoreInItemFromSidebarMembers(name);
        return OK_REQUEST;
    }
    else if (action == qsl("printallwidgetsfrommainwindow"))
    {
        printAllWidgetsFromMainWindow();
        return OK_REQUEST;
    }
    else if (action == qsl("printallmenudialogs"))
    {
        printAllMenuDialogs();
        return OK_REQUEST;
    }
    else if (action == qsl("getlastmessage"))
    {
        return getLastMessage(name);
    }
    else if (action == qsl("getlastfileinfo"))
    {
        return getLastFileInfo(name);
    }
    else if (action == qsl("getuinwithopeneddialog"))
    {
        getUinWithOpenedDialog();
        return OK_REQUEST;
    }
    else if (action == qsl("iselementexists"))
    {
        return isElementExists(name);
    }
    else if (action == qsl("clickitemincontextmenu"))
    {
        return clickItemInContextMenu(name, value);
    }
    else if (action == qsl("iscontactinrecents"))
    {
        return isContactInRecents(name);
    }
    else if (action == qsl("isiteminsearchresults"))
    {
        return isItemInSearchResults(name);
    }
    else if (action == qsl("iselementinsidebar"))
    {
        return isElementInSidebar(name);
    }
    else if (action == qsl("getcontactlistcounter"))
    {
        return getContactListCounter();
    }
    else if (action == qsl("openitemfromsidebar"))
    {
        openItemFromSidebar(name);
        return OK_REQUEST;
    }
    else if (action == qsl("quitfromapplication"))
    {
        quitFromApplication();
        return OK_REQUEST;
    }
    else if (action == qsl("editlastmessage"))
    {
        editLastMessage();
        return OK_REQUEST;
    }
    else if (action == qsl("closegallerypreviewer"))
    {
        closeGalleryPreviewer();
        return OK_REQUEST;
    }
    else if (action == qsl("clickaddcontact"))
    {
        clickAddContact(name);
        return OK_REQUEST;
    }
    else if (action == qsl("sendfiletoclipboard"))
    {
        sendFileToClipboard(name);
        return OK_REQUEST;
    }
    else if (action == qsl("attachfilefromclipboard"))
    {
        attachFileFromClipboard();
        return OK_REQUEST;
    }
    else if (action == qsl("clicktofile"))
    {
        clickToFile();
        return OK_REQUEST;
    }
    else if (action == qsl("rightclickonmessage"))
    {
        rightClickOnMessage();
        return OK_REQUEST;
    }
    else if (action == qsl("setscrollvalueto"))
    {
        return setScrollValueTo(name, value);
    }
    else if (action == qsl("sethistoryscrolltobottom"))
    {
        return setHistoryScrollToBottom();
    }
    return EMPTY_STRING;
}


void WebServer::slotReadClient()
{
    qDebug() << "DEBUG:  WebServer::slotReadClient()";
    //QTcpSocket* clientSocket = (QTcpSocket*)sender();
    QTcpSocket* clientSocket = qobject_cast<QTcpSocket*>(sender());
    int idusersocs = clientSocket->socketDescriptor();

    //QStringList list;
    //while (clientSocket->canReadLine())
    //{
    //    QString line = QString::fromUtf8(clientSocket->readLine().trimmed());
    //    list.append(line);
    //    qDebug() << "QString::fromUtf8(clientSocket->readLine()).trimmed() = " << line;
    //}
    //qDebug() << "list = " << list;
    ////qDebug() << "raw clientSocket->readAll() = " << clientSocket->readAll();

    ////QStringList to QByteArray
    //QByteArray strReply;
    //foreach(const QString &str, list)
    //{
    //    strReply.append(str.toUtf8());
    //}

    QByteArray strReply = clientSocket->readAll();
    QString s_data = QString::fromUtf8(strReply.data());

    qDebug() << "DEBUG: strReply = " << strReply;
    qDebug() << "DEBUG: s_data = " << s_data;

    QString soLongResponse = inputRequest(strReply);

    QTextStream os(clientSocket);
    //os.setAutoDetectUnicode(true);
    //os.setEncoding(QTextStream::UnicodeUTF8);

    os.setCodec("UTF-8");

    qDebug() << "soLongResponse = " << soLongResponse;
    //qDebug() << "returnNotFound() = " << returnNotFound();

    //If exit text is "404 not found" return 404 status_code
    if (soLongResponse == returnNotFound())
    {
        os << "HTTP/1.0 404 Ok\r\n"
            "Content-Type: text/html; charset=\"utf-8\"\r\n"
            "\r\n";
    }
    else
    {
        os << "HTTP/1.0 200 Ok\r\n"
            "Content-Type: text/html; charset=\"utf-8\"\r\n"
            "\r\n";
    }

    os << soLongResponse;

    // Close socket
    clientSocket->close();
    SClients.remove(idusersocs);
}
