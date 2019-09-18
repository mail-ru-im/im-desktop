#include "stdafx.h"
#include "typing.h"

#include "../main_window/friendly/FriendlyContainer.h"

QString Logic::TypingFires::getChatterName() const
{
    if (!chatterAimId_.isEmpty())
        return Logic::GetFriendlyContainer()->getFriendly(chatterAimId_);
    return Logic::GetFriendlyContainer()->getFriendly(aimId_);
}

void Logic::TypingFires::refreshTime()
{
    time_ = std::chrono::milliseconds(QDateTime::currentMSecsSinceEpoch());
}
