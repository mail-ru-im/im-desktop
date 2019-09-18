#ifndef __MPLAYER_H_
#define __MPLAYER_H_

#pragma once

namespace Ui
{
    class implayer
    {
    public:

        implayer()

        {

        }

        virtual ~implayer()
        {

        }

        virtual void show(const QRect& _rect) = 0;
    };
}


#endif //__MPLAYER_H_
