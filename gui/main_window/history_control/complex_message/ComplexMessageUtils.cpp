#include "stdafx.h"

#include "ComplexMessageUtils.h"
#include "../../../utils/utils.h"
#include "IItemBlock.h"
#include "TextBlock.h"

UI_COMPLEX_MESSAGE_NS_BEGIN

QSize limitSize(const QSize& _size, const QSize& _maxSize, const QSize& _minSize)
{
    im_assert(!_size.isEmpty());
    im_assert(_maxSize.width() > 0 || _maxSize.height() > 0);

    if (_size.isEmpty())
        return _size;

    if (_size.width() > _maxSize.width() || _size.height() > _maxSize.height())
    {
        if (Utils::isPanoramic(_size) && _minSize.isValid())
        {
            const auto scale = [&_maxSize, &_size](const auto _imgSide, const int _preSide)
            {
                const auto ratio = double(std::max(_imgSide, _preSide)) / std::min(_imgSide, _preSide);
                if (_imgSide > _preSide)
                    return (_size / ratio).boundedTo(_maxSize);
                else
                    return (_size * ratio).boundedTo(_maxSize);
            };

            if (_size.height() > _size.width())
                return scale(_size.width(), _minSize.width());
            else
                return scale(_size.height(), _minSize.height());
        }
        else
        {
            return _size.scaled(_maxSize, Qt::KeepAspectRatio);
        }
    }

    return _size;
}

template <typename T>
bool mergeReplaceByTextImpl(std::vector<T*>& _blocks, IItemBlock* _base, ComplexMessageItem* _parent, ReplaceReason _reason)
{
    auto iter = std::find(_blocks.begin(), _blocks.end(), _base);
    if (iter == _blocks.end())
    {
        im_assert(!"block is missing");
        return false;
    }

    auto& existingBlock = *iter;
    im_assert(existingBlock);

    const auto blockText = (_reason == ReplaceReason::NoMeta)
        ? existingBlock->getPlaceholderText()
        : existingBlock->getSourceText();
    auto replaceByTextBlock = [_parent](auto& _blockToReplaceRef, Data::FStringView _text)
    {
        TextBlock* newBlock = new TextBlock(_parent, _text);
        newBlock->onActivityChanged(true);
        newBlock->show();

        _blockToReplaceRef->deleteLater();
        _blockToReplaceRef = newBlock;
    };

    replaceByTextBlock(existingBlock, blockText);

    if (_blocks.size() < 2)
        return true;

    auto it = _blocks.begin();
    auto next = std::next(it);
    auto last = _blocks.end();
    for (; next < last; it = next, next = std::next(it))
    {
        Data::FString text;
        auto& currentBlock = *it;
        auto& nextBlock = *next;
        while (currentBlock->getContentType() == IItemBlock::ContentType::Text &&
               nextBlock->getContentType() == IItemBlock::ContentType::Text)
        {
            text += nextBlock->getSourceText();

            nextBlock->deleteLater();

            std::copy(std::next(next), last, next);
            --last;
            *last = nextBlock;

            next = std::next(it);
            if (next == last)
                break;

            nextBlock = *next;
        }

        if (!text.isEmpty())
        {
            Data::FString replacementText = currentBlock->getSourceText();
            replacementText += text;
            replaceByTextBlock(currentBlock, replacementText);
        }

        text.clear();

        if (next >= last)
            break;
    }

    _blocks.erase(last, _blocks.end());

    return true;
}

bool mergeReplaceByText(std::vector<IItemBlock*>& _blocks, IItemBlock* _base, ComplexMessageItem* _parent, ReplaceReason _reason)
{
    return mergeReplaceByTextImpl(_blocks, _base, _parent, _reason);
}

bool mergeReplaceByText(std::vector<GenericBlock*>& _blocks, IItemBlock* _base, ComplexMessageItem* _parent, ReplaceReason _reason)
{
    return mergeReplaceByTextImpl(_blocks, _base, _parent, _reason);
}

UI_COMPLEX_MESSAGE_NS_END
