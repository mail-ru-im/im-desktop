#include "memory_utils.h"

#include "utils.h"

#include <QPixmap>
#include <qpa/qplatformpixmap.h>
#include <iostream>
#include <QDebug>

#if defined(__APPLE__)
#include<mach/mach.h>
#elif defined(__linux__)
#define STATM_FPATH "/proc/self/statm"
typedef struct
{
    unsigned long size, resident, share, text, lib, data, dt;
} StatM_t;
#endif

#include "styles/Theme.h"
#include "main_window/history_control/complex_message/ImagePreviewBlock.h"
#include "main_window/history_control/complex_message/LinkPreviewBlock.h"
#include "main_window/history_control/complex_message/StickerBlock.h"
#include "main_window/history_control/complex_message/FileSharingBlock.h"
#include "main_window/history_control/complex_message/QuoteBlock.h"
#include "main_window/history_control/complex_message/TextBlock.h"

namespace Utils
{

template<typename Tag, typename Tag::type M>
struct Rob
{
  friend typename Tag::type get(Tag) {
    return M;
  }
};

// tag used to access A::member
struct A_member {
  typedef QExplicitlySharedDataPointer<QPlatformPixmap> QPixmap::*type;
  friend type get(A_member);
};

template struct Rob<A_member, &QPixmap::data>;

template<typename Type>
qint64 getMemoryFootprint(Type _object)
{
    assert(!"not implemented");
    return 0;
}

template<>
qint64 getMemoryFootprint(const QPixmap* _pixmap)
{
    qint64 res = 0;
    if (!_pixmap || _pixmap->isNull())
        return res;

    QExplicitlySharedDataPointer<QPlatformPixmap> copy(_pixmap->*get(A_member()));
    res += copy->buffer()->byteCount();
//    res += _pixmap->width() * _pixmap->height() * (_pixmap->depth() / 8.);
    res += sizeof(QPixmap);

    return res;
}

template<>
qint64 getMemoryFootprint(const QPixmap& _pixmap)
{
    return getMemoryFootprint(const_cast<const QPixmap *>(&_pixmap));
}

template<>
qint64 getMemoryFootprint(QPixmap _pixmap)
{
    return getMemoryFootprint(const_cast<const QPixmap *>(&_pixmap));
}

template<>
qint64 getMemoryFootprint(Styling::WallpaperPtr _wallpaper)
{
    qint64 res = 0;

    if (!_wallpaper)
        return res;

    const auto& bg = _wallpaper->getWallpaperImage();
    res += getMemoryFootprint(bg);

    if (const auto& preview = _wallpaper->getPreviewImage(); preview.cacheKey() != bg.cacheKey())
        res += getMemoryFootprint(preview);

    res += _wallpaper->getWallpaperUrl().capacity();
    res += _wallpaper->getPreviewUrl().capacity();
    res += sizeof(Styling::ThemeWallpaper);

    return res;
}

template<>
qint64 getMemoryFootprint(const QImage& _image)
{
    qint64 res = 0;
    if (_image.isNull())
        return res;

    res += sizeof(QImage);
    res += _image.byteCount();

    return res;
}

template<>
qint64 getMemoryFootprint(QImage _image)
{
    const auto& imgRef = _image;
    return getMemoryFootprint<const QImage&>(imgRef);
}

template<>
qint64 getMemoryFootprint(Ui::ComplexMessage::ImagePreviewBlock* _block)
{
    qint64 res = 0;

    res += getMemoryFootprint(_block->getPreviewImage());

    return res;
}

template<>
qint64 getMemoryFootprint(Ui::ComplexMessage::LinkPreviewBlock* _block)
{
    qint64 res = 0;

    res += getMemoryFootprint(_block->getPreviewImage());

    return res;
}

template<>
qint64 getMemoryFootprint(Ui::ComplexMessage::StickerBlock* _block)
{
    qint64 res = 0;

    res += getMemoryFootprint(_block->getStickerImage());

    return res;
}

template<>
qint64 getMemoryFootprint(Ui::ComplexMessage::FileSharingBlock* _block)
{
    qint64 res = 0;

    res += getMemoryFootprint(_block->getPreview());

    return res;
}

template<>
qint64 getMemoryFootprint(Ui::ComplexMessage::TextBlock* _block)
{
    qint64 res = 0;

    res += _block->getSourceText().toUtf8().size();

    return res;
}

template<>
qint64 getMemoryFootprint(Ui::ComplexMessage::QuoteBlock* _block)
{
    qint64 res = 0;

    for (auto block: _block->getBlocks())
    {
        auto imgPreviewBlock = dynamic_cast<Ui::ComplexMessage::ImagePreviewBlock *>(block);
        if (imgPreviewBlock)
        {
            res += getMemoryFootprint(imgPreviewBlock);
            continue;
        }

        auto fileSharingBlock = dynamic_cast<Ui::ComplexMessage::FileSharingBlock *>(block);
        if (fileSharingBlock)
        {
            res += getMemoryFootprint(fileSharingBlock);
            continue;
        }

        auto stickerBlock = dynamic_cast<Ui::ComplexMessage::StickerBlock *>(block);
        if (stickerBlock)
        {
            res += getMemoryFootprint(stickerBlock);
            continue;
        }

        auto linkPreviewBlock = dynamic_cast<Ui::ComplexMessage::LinkPreviewBlock *>(block);
        if (linkPreviewBlock)
        {
            res += getMemoryFootprint(linkPreviewBlock);
            continue;
        }

        auto quotePreviewBlock = dynamic_cast<Ui::ComplexMessage::QuoteBlock *>(block);
        if (quotePreviewBlock)
        {
            res += /* recursion */ getMemoryFootprint(quotePreviewBlock);
            continue;
        }

        auto textBlock = dynamic_cast<Ui::ComplexMessage::TextBlock *>(block);
        if (textBlock)
        {
            res += getMemoryFootprint(textBlock);
            continue;
        }

    }

    return res;
}

uint64_t getCurrentProcessRamUsage() // phys_footprint on macos, "Mem" in Activity Monitor
{
#if defined(_WIN32)
    PROCESS_MEMORY_COUNTERS_EX pmc;
    ZeroMemory(&pmc, sizeof(PROCESS_MEMORY_COUNTERS_EX));
    GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS *)&pmc, sizeof(pmc));
    return static_cast<uint64_t>(pmc.PrivateUsage);
#elif defined(__APPLE__)
#if defined(TASK_VM_INFO) && TASK_VM_INFO >= 22
            task_vm_info vm_info = { 0, 0, 0 };
            mach_msg_type_number_t count = sizeof(vm_info) / sizeof(int);
            int err = task_info(mach_task_self(), TASK_VM_INFO, (int *)&vm_info, &count);
            UNUSED_ARG(err);
            return (uint64_t)(vm_info.internal + vm_info.compressed);
//            return (uint64_t)vm_info.phys_footprint;
#else
            return getCurrentProcessRealMemUsage();
#endif // TASK_VM_INFO
#else
    return getCurrentProcessRealMemUsage();
#endif
}

uint64_t getCurrentProcessRealMemUsage() // resident size on macos, "Real Mem" in Activity Monitor
{
#if defined(_WIN32)
    PROCESS_MEMORY_COUNTERS_EX pmc;
    ZeroMemory(&pmc, sizeof(PROCESS_MEMORY_COUNTERS_EX));
    GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS *)&pmc, sizeof(pmc));
    return static_cast<uint64_t>(pmc.WorkingSetSize);
#elif defined(__APPLE__)
    struct task_basic_info t_info;
    mach_msg_type_number_t t_info_count = TASK_BASIC_INFO_COUNT;

    if (KERN_SUCCESS != task_info(mach_task_self(), TASK_BASIC_INFO, reinterpret_cast<task_info_t>(&t_info), &t_info_count))
        return 0;

    return t_info.resident_size;
#else
    FILE *f = fopen(STATM_FPATH, "r");
    if (!f)
        return 0;

    const auto scopedExit = Utils::ScopeExitT([f]{ fclose(f); });

    StatM_t stat = {};
    if (7 != fscanf(f,"%ld %ld %ld %ld %ld %ld %ld", &stat.size, &stat.resident, &stat.share, &stat.text, &stat.lib, &stat.data, &stat.dt))
    {
        return 0;
    }

    return  stat.size * core::KILOBYTE;
#endif
}

}
