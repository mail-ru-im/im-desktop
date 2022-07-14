#pragma once

#include "Common.h"
#include "../../types/message.h"

namespace Logic
{
    class IconsDelegate
    {
        public:
            virtual ~IconsDelegate(){};
            QRect getDraftIconRect() const { return draftIconRect_; };
            virtual bool needDrawDraft(const Ui::ViewParams& _viewParams, const Data::DlgState& _state) const{ return false; };


        protected:
            virtual bool needDrawUnreads(const Data::DlgState& _state) const{ return false; };
            virtual bool needDrawMentions(const Ui::ViewParams& _viewParams, const Data::DlgState& _state) const{ return false; };
            virtual bool needDrawAttention(const Data::DlgState& _state) const{ return false; };


            virtual void drawDraft(QPainter* _painter, const Data::DlgState& _state, bool _isSelected, const QRect& _rect, const Ui::ViewParams& _viewParams, int& _rightAreaRight) const{};
            virtual void drawUnreads(QPainter* _painter, const Data::DlgState& _state, bool _isSelected, const QRect& _rect, const Ui::ViewParams& _viewParams, int& _unreadsX, int& _unreadsY) const{};
            virtual void drawMentions(QPainter* _painter, const Data::DlgState& _state, bool _isSelected, const QRect& _rect, const Ui::ViewParams& _viewParams, int& _unreadsX, int& _unreadsY) const{};
            virtual void drawAttention(QPainter* _painter, const Data::DlgState& _state, bool _isSelected, const QRect& _rect, const Ui::ViewParams& _viewParams, int& _unreadsX, int& _unreadsY) const{};

            void fixMaxWidthIfIcons(int& _maxWidth) const;
            bool isInDraftIconRect(QPoint _posCursor) const;

            mutable QRect draftIconRect_;

    };
}