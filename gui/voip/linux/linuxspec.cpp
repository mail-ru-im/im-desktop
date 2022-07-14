#ifdef __linux__
#include "linuxspec.h"

#include <memory>

#include <QX11Info>

#include <xcb/xcb.h>
#include <xcb/xcb_atom.h>
#include <xcb/xcbext.h>

#include <xcb/xcb_icccm.h>
#include <xcb/xcb_ewmh.h>

// X11 headers must be the last
// to avoid name clashing
#include <X11/Xlib.h>
#include <X11/keysym.h>

namespace platform_linux
{
    namespace detail
    {
        struct cdeleter
        {
            void operator()(void* __ptr) const { free(__ptr); }
        };

        template<class T>
        using cmem_ptr = std::unique_ptr<T, cdeleter>;
    }

    class XcbWindowManager
    {
        Q_DISABLE_COPY_MOVE(XcbWindowManager)

    public:
        static XcbWindowManager& instance()
        {
            static XcbWindowManager gInst;
            return gInst;
        }

        ~XcbWindowManager()
        {
            // wipe the emwh connection
            xcb_ewmh_connection_wipe(&mEwmh_);
        }

        xcb_connection_t* xcb() const
        {
            return mConn_;
        }

        xcb_ewmh_connection_t* ewmh()
        {
            return &mEwmh_;
        }

        bool isVisible(xcb_window_t _w)
        {
            detail::cmem_ptr<xcb_get_window_attributes_reply_t> attribs(xcb_get_window_attributes_reply(mConn_, xcb_get_window_attributes(mConn_, _w), nullptr));
            if (!attribs)
                return false;

            xcb_ewmh_get_atoms_reply_t state;
            xcb_get_property_cookie_t state_cookie = xcb_ewmh_get_wm_state(&mEwmh_, _w);
            const auto r = xcb_ewmh_get_wm_window_type_reply(&mEwmh_, state_cookie, &state, NULL);
            if (r == 1)
            {
                for (unsigned int i = 0; i < state.atoms_len; i++)
                {
                    if (state.atoms[i] == mEwmh_._NET_WM_STATE_HIDDEN ||
                        state.atoms[i] == mEwmh_._NET_WM_STATE_SKIP_TASKBAR ||
                        state.atoms[i] == mEwmh_._NET_WM_STATE_SKIP_PAGER)
                        return false;
                }
            }

            return attribs->_class == XCB_WINDOW_CLASS_INPUT_OUTPUT &&
                   attribs->map_state == XCB_MAP_STATE_VIEWABLE;
        }

        QRect geometry(xcb_window_t _w)
        {
            QRect windowRect;
            detail::cmem_ptr<xcb_get_geometry_reply_t> geometry(xcb_get_geometry_reply(mConn_, xcb_get_geometry(mConn_, _w), nullptr));
            if (geometry)
                windowRect.setRect(geometry->x, geometry->y, geometry->width, geometry->height);
            return windowRect;
        }

        // usefull for debugging purposes
        QString title(WId _w) const
        {
            xcb_get_property_cookie_t wname_cookie = xcb_get_property(mConn_, 0, _w, XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 0, 256);
            detail::cmem_ptr<xcb_get_property_reply_t> wname(xcb_get_property_reply(mConn_, wname_cookie, nullptr));
            if (!wname)
            {
                xcb_get_property_cookie_t wmname_cookie = xcb_icccm_get_wm_name(mConn_, _w);
                detail::cmem_ptr<xcb_get_property_reply_t> wmname(xcb_get_property_reply(mConn_, wmname_cookie, nullptr));
                if (!wmname)
                    return QString();

                const int len = xcb_get_property_value_length(wmname.get());
                const char* str = (const char*)xcb_get_property_value(wmname.get());
                if (len > 0 && str)
                    return QString::fromUtf8(str, len);
                else
                    return QString();
            }

            const int len = xcb_get_property_value_length(wname.get());
            const char* str = (const char*)xcb_get_property_value(wname.get());
            if (len > 0 && str)
                return QString::fromUtf8(str, len);
            else
                return QString();
        }

        bool isNetWmDelWinEvent(xcb_generic_event_t* _ev) const
        {
            constexpr auto kXcbEventResponseTypeMask = 0x7f;
            if (wmDeleteWindowAtom_ == 0)
                return false;

            const auto ev_type = _ev->response_type & kXcbEventResponseTypeMask;
            if (ev_type == XCB_CLIENT_MESSAGE)
            {
                xcb_client_message_event_t* msg = reinterpret_cast<xcb_client_message_event_t*>(_ev);
                if (msg->type != mEwmh_.WM_PROTOCOLS)
                    return false;
                return msg->data.data32[0] == wmDeleteWindowAtom_;
            }
            return false;
        }

    private:
        XcbWindowManager()
        {
            // Get xcb connection with a little help from Qt
            mConn_ = QX11Info::connection();
            if (mConn_ == nullptr)
            {
                qCritical("xcb connection failed");
                return;
            }

            if (!xcb_ewmh_init_atoms_replies(&mEwmh_, xcb_ewmh_init_atoms(mConn_, &mEwmh_), NULL))
                qCritical() << ("xcb emwh init failed");

            xcb_intern_atom_cookie_t wm_delete_window_cookie = xcb_intern_atom(mConn_, 0, 16, "WM_DELETE_WINDOW");
            detail::cmem_ptr<xcb_intern_atom_reply_t> wm_delete_window_cookie_reply(xcb_intern_atom_reply(mConn_, wm_delete_window_cookie, 0));
            if (wm_delete_window_cookie_reply)
                wmDeleteWindowAtom_ = wm_delete_window_cookie_reply->atom;
        }

    private:
        xcb_connection_t* mConn_;
        xcb_ewmh_connection_t mEwmh_;
        xcb_atom_t wmDeleteWindowAtom_ = 0;
    };

    bool isNetWmDeleteWindowEvent(void* _ev)
    {
        return XcbWindowManager::instance().isNetWmDelWinEvent(static_cast<xcb_generic_event_t*>(_ev));
    }

    void showInWorkspace(QWidget* _w, QWidget* _initiator)
    {
        // not implemented yet
    }

    bool windowIsOverlapped(QWidget* _window)
    {
        const WId winId = _window->effectiveWinId();

        XcbWindowManager& xwm = XcbWindowManager::instance();
        xcb_connection_t* conn = xwm.xcb();

        xcb_window_t root = xcb_setup_roots_iterator(xcb_get_setup(conn)).data->root;

        xcb_window_t* children;

        /* Get the tree of windows whose parent is the root window (= all) */
        detail::cmem_ptr<xcb_query_tree_reply_t> reply(xcb_query_tree_reply(conn, xcb_query_tree(conn, root), nullptr));
        if (!reply)
            return false;

        /* Request the children count */
        const int len = xcb_query_tree_children_length(reply.get());
        if (len < 1)
            return false;

        /* Request the window tree  */
        children = xcb_query_tree_children(reply.get());
        if (!children)
            return false;

        bool hasOverlapping = false;
        const QRect rect = _window->geometry();

        QRegion region = rect;

        // Travers the children in reverse order, that
        // implies top-to-bottom visiting of every window
        for (int i = (len - 1); i > 0; --i)
        {
            auto window = children[i];

            if (window == winId || region.isEmpty())
                break; // We got the self - stop traversing

            if (!xwm.isVisible(window))
                continue; // Invisible window - continue

            const QRect wRect = xwm.geometry(window);
            if (!region.intersects(wRect) || wRect.isEmpty())
                continue; // We hasn't got intersection with this window - continue

            region -= wRect;
            hasOverlapping = true;
        }

        if (!hasOverlapping) // We are a top-most window or no intersections found
            return false;
        else if (region.isEmpty()) // We are fully overlapped by other windows
            return true;

        // We are partially overlapped by other windows
        const int originalArea = rect.width() * rect.height();
        int overlapArea = 0;
        for (const auto& r : region)
            overlapArea += r.width() * r.height();

        return overlapArea < originalArea * 0.6;
    }

    bool isSpacebarDown()
    {
        if (!QX11Info::isPlatformX11())
            return false;

        Display* display = QX11Info::display();
        if (!display)
            return false;

        // Translate key symbol to key code
        const KeyCode kc = XKeysymToKeycode(display, XK_space);

        // From the XLib manual:
        // The XQueryKeymap() function returns a bit vector for the logical state of the keyboard,
        // where each bit set to 1 indicates that the corresponding key is currently pressed down.
        // The vector is represented as 32 bytes. Byte N (from 0) contains the bits for keys 8N to 8N + 7
        // with the least-significant bit in the byte representing key 8N.
        char keyState[32];
        XQueryKeymap(display, keyState);

        const int byteIdx = (kc / 8);
        const int bitMask = 1 << (kc % 8);
        const uint8_t keyByte = (uint8_t)keyState[byteIdx];

        return (keyByte & bitMask) != 0;
    }
}

#endif
