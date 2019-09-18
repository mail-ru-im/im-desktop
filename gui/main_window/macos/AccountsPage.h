#pragma once

#ifndef MAC_MIGRATION
#define MAC_MIGRATION
#include "../../utils/macos/mac_migration.h"
#endif

namespace Ui
{
    class account_widget : public QPushButton
    {
        Q_OBJECT

    private Q_SLOTS:
        void avatar_loaded(const QString& uid);
        
    Q_SIGNALS:
        void checked();
        
    private:
        const MacProfile account_;
    public:
        account_widget(QWidget* _parent, const MacProfile &profile);
        virtual ~account_widget();
        
        QString uid() const;
        MacProfileType profileType() const;
    private:
        QWidget* check_;
        QLabel* avatar_;
        int avatar_h_;
        QString uid_;
        MacProfileType type_;
        
        void enterEvent(QEvent * event) override;
        void leaveEvent(QEvent * event) override;
    };

    class AccountsPage : public QWidget
    {
        Q_OBJECT

    private Q_SLOTS:
        void nextClicked(bool _checked);
        void skipClicked(bool _checked);
        void prevClicked(bool _checked);
        
    Q_SIGNALS:
        void account_selected();
        void loggedIn();
        
    private Q_SLOTS:
        void loginResult(int);
        void updateNextButtonState();

    private:
        int active_step_;
        int icq_accounts_count_;
        int agent_accounts_count_;
        int selected_icq_accounts_count_;
        int selected_agent_accounts_count_;
        
        QVBoxLayout* root_layout_;
        QHBoxLayout* buttonLayout_;
        
        QWidget* agentPageIcon_;
        QLabel* agentPageText_;
        QWidget* icqPageIcon_;
        QLabel* icqPageText_;
        
        QPushButton* button_prev_;
        QPushButton* button_next_;
        
        QPushButton* buttonSkip_;
        
        QWidget* accounts_area_widget_;
        QScrollArea* accounts_area_;
        QVBoxLayout* accounts_layout_;
        
        std::vector<account_widget*> account_widgets_;
        
    private:
        MacMigrationManager *manager_;
        MacProfilesList profiles_;
    public:
        void update_tabs();
        void update_buttons();
        void init_with_step(const int _step);
        void store_accounts_and_exit();
        
    protected:
        void paintEvent(QPaintEvent *) override;

    public:
        AccountsPage(QWidget* _parent, MacMigrationManager * manager);
        virtual ~AccountsPage();
        
        void summon();
    };

}


