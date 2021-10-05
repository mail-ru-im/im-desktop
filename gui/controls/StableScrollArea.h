#pragma once

namespace Ui
{
class StableScrollArea_p;
class StableScrollArea : public QWidget
{
    Q_OBJECT
public:
    StableScrollArea(QWidget* _parent);
    ~StableScrollArea();

    void setWidget(QWidget* _w);
    QWidget* widget() const;
    void setAnchor(QWidget* _w);
    QWidget* anchor() const;
    void ensureWidgetVisible(QWidget* _w);
    void ensureVisible(int _y);
    int widgetPosition() const;
    void setScrollLocked(bool _locked);

public Q_SLOTS:
    void updateSize();

Q_SIGNALS:
    void scrolled(int _val);

protected:
    bool event(QEvent* _e) override;
    bool eventFilter(QObject* _o, QEvent* _e) override;
    void resizeEvent(QResizeEvent* _e) override;

private:
    std::unique_ptr<StableScrollArea_p> d;
};

}
