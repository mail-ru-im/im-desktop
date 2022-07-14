#pragma once

#include "GenericBlock.h"
#include "types/task.h"

UI_NS_BEGIN

namespace TextRendering
{
    class TextUnit;
    using TextUnitPtr = std::unique_ptr<TextUnit>;
} // namespace TextRendering

UI_NS_END

UI_COMPLEX_MESSAGE_NS_BEGIN

enum class StatusChipState
{
    None,
    Hovered,
    Pressed
};

class TaskBlock : public GenericBlock
{
    Q_OBJECT

public:
    TaskBlock(ComplexMessageItem* _parent, const Data::TaskData& _task, const QString& _text = {});
    ~TaskBlock() override;

    IItemBlockLayout* getBlockLayout() const override;
    Data::FString getSelectedText(const bool _isFullSelect = false, const TextDestination _dest = TextDestination::selection) const override;
    bool updateFriendly(const QString& _aimId, const QString& _friendly) override;
    void updateWith(IItemBlock* _other) override;
    Data::FString getSourceText() const override;
    bool isSelected() const override;
    bool isAllSelected() const override;
    void selectByPos(const QPoint& _from, const QPoint& _to, bool) override;
    void clearSelection() override;
    void releaseSelection() override;
    bool isDraggable() const override;
    bool isSharingEnabled() const override;
    QString formatRecentsText() const override;
    Data::Quote getQuote() const override;

    bool onMenuItemTriggered(const QVariantMap& _params) override;
    MenuFlags getMenuFlags(QPoint _p) const override;

    ContentType getContentType() const override;
    MediaType getMediaType() const override;

    bool clicked(const QPoint& _p) override;
    void doubleClicked(const QPoint& _p, std::function<void(bool)> _callback = std::function<void(bool)>()) override;

    Data::LinkInfo linkAtPos(const QPoint& _globalPos) const override;

    int desiredWidth(int _width = 0) const override;

    void onVisibleRectChanged(const QRect& _visibleRect) override;

    QRect setBlockGeometry(const QRect& _rect) override;
    QRect getBlockGeometry() const override;

    QRect statusBubbleRect() const;

protected:
    void drawBlock(QPainter& _painter, const QRect& _rect, const QColor& _quoteColor) override;
    void initialize() override;

    void mouseMoveEvent(QMouseEvent* _event) override;
    void mousePressEvent(QMouseEvent* _event) override;
    void mouseReleaseEvent(QMouseEvent* _event) override;
    void leaveEvent(QEvent* _event) override;

private:
    void updateFonts() override;
    void initTextUnits();
    void reinitTextUnits();
    void updateStatusChipState(StatusChipState _statusChipState);

    void updateContent();
    int updateTaskContentSize(int width);
    int updateSkeletonSize(int width);
    int evaluateDebugIdHeight(int _availableWidth, int _verticalOffset);

    int timeTextLeftOffset() const;
    int taskStatusBubbleWidth() const;
    QRect profileClickAreaRect() const;
    int assigneeAvailableWidth() const;

    bool needSkeleton() const;
    bool canModifyTask() const;
    bool canChangeTaskStatus() const;
    bool needDrawStatusArrow() const;

    void syncronizeTaskFromContainer();

private Q_SLOTS:
    void onTaskUpdated(const QString& _taskId);

private:
    TextRendering::TextUnitPtr titleTextUnit_;
    TextRendering::TextUnitPtr assigneeTextUnit_;
    TextRendering::TextUnitPtr statusTextUnit_;
    TextRendering::TextUnitPtr timeTextUnit_;
    TextRendering::TextUnitPtr debugIdTextUnit_;
    TextRendering::TextUnitPtr debugCreatorTextUnit_;
    TextRendering::TextUnitPtr debugThreadIdTextUnit_;

    QVariantAnimation* skeletonAnimation_ = nullptr;
    Data::TaskData task_;
    QRect contentRect_;
    StatusChipState statusChipState_ = StatusChipState::None;
    int blockWidth_ = 0;
};

UI_COMPLEX_MESSAGE_NS_END
