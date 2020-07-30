#pragma once

namespace Ui
{

class ActiveCallPlate_p;
class ActiveCallPlate : public QWidget
{
    Q_OBJECT
public:
    ActiveCallPlate(const QString& _chatId, QWidget* _parent);
    ~ActiveCallPlate();

    void setParticipantsCount(int _count);
    int participantsCount() const;

protected:
    void paintEvent(QPaintEvent* _event) override;
    void resizeEvent(QResizeEvent* _event) override;

private Q_SLOTS:
    void onJoinClicked();

private:
    std::unique_ptr<ActiveCallPlate_p> d;
};

}
