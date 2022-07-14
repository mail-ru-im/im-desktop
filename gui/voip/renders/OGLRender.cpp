#include "stdafx.h"
#include "OGLRender.h"
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QtGui/private/qscreen_p.h>
#include "../../../common.shared/string_utils.h"
#include "cache/avatars/AvatarStorage.h"
#include "main_window/MainWindow.h"
#include "main_window/containers/FriendlyContainer.h"
#include "../../previewer/GalleryFrame.h"
#include "controls/TextEmojiWidget.h"
#include "controls/TooltipWidget.h"
#include "my_info.h"
#include "fonts.h"
#include "utils/utils.h"
#include "utils/InterConnector.h"
#include "core_dispatcher.h"
#ifdef __APPLE__
#include "../macos/macosspec.h"
#endif
#define LAY_IMPLEMENTATION
#include "layout.h"

#define ENABLE_VIRTUAL_PEERS 0

namespace
{
    constexpr int kMaxVisiblePeers = 6;
    constexpr std::chrono::milliseconds kAnimationDuration(500);
    constexpr std::chrono::milliseconds kTooltipDelay(300);
    constexpr std::chrono::milliseconds kMacOSTooltipHideDelay(10);
    constexpr int kSmallAvatarSize = 64;
    constexpr int kLargeAvatarSize = 120;
    constexpr double kAvatarScaleFactor = float(kSmallAvatarSize) / float(kLargeAvatarSize);
    constexpr QSize kAvatarBlurSize(213, kLargeAvatarSize);
    constexpr float kFrameBorderWidth = 3.f;
    constexpr int kBlurFactor = 6;
    constexpr double kBlurOpacity = 0.8;
    constexpr QColor kBlurColor(24, 24, 24);
    constexpr double kDownsamplingFactor = 1.0 / 5.0; // 1/5 from the original size
    constexpr QMargins kCaptionMargins(4, 4, 4, 4);
    constexpr QMargins kCaptionItemMargins(4, 4, 4, 4);
    constexpr int kCaptionIconSpacing = 2; // space between icon and text
    constexpr int kCaptionHeight = 24;

    constexpr QSize kMiniWindowSize(284, 160);
    constexpr QSize kPreviewSize(200, 112);
    constexpr QSize kPreviewMargin(12, 42);

    constexpr QSize kTopLinePeerSize(140, 78);
    constexpr int kTopLineHeight = 86;
    constexpr int kTopLineItemWidth = 144;
    constexpr int kTopLineHMargin = 30;

    constexpr int kButtonTextOffset = 36;

    constexpr int kAspectEpsilon = 2;
}

Ui::OGLRenderWidget::Frame::Frame()
{
    texY_ = std::make_unique<QOpenGLTexture>(QOpenGLTexture::Target2D);
    texU_ = std::make_unique<QOpenGLTexture>(QOpenGLTexture::Target2D);
    texV_ = std::make_unique<QOpenGLTexture>(QOpenGLTexture::Target2D);
    texY_->create();
    texU_->create();
    texV_->create();
}

Ui::OGLRenderWidget::OGLRenderWidget(QWidget* _parent)
    : QOpenGLWidget(_parent)
    , parent_(_parent)
    , previewRenderName_(PREVIEW_RENDER_NAME)
{
    tooltipTimer_ = new QTimer(this);
    tooltipTimer_->setSingleShot(true);
    tooltipTimer_->setInterval(kTooltipDelay);

    setMouseTracking(true);
    lay_init_context(&ctx_);
    lay_reserve_items_capacity(&ctx_, 128);
    font_ = Fonts::appFont(14, Fonts::FontWeight::SemiBold);
    connect(Logic::GetAvatarStorage(), &Logic::AvatarStorage::avatarChanged, this, &Ui::OGLRenderWidget::avatarChanged);
    setCursor(Qt::PointingHandCursor);
    setAttribute(Qt::WA_Hover);
}

Ui::OGLRenderWidget::~OGLRenderWidget()
{
    makeCurrent();
    lay_destroy_context(&ctx_);
}

void Ui::OGLRenderWidget::initializeGL()
{
    if (!isValid())
    {
        qWarning() << "GL init error.";
        return;
    }
    QSurfaceFormat format = QOpenGLContext::currentContext()->format();
    const bool gl3_1plus_ = (format.majorVersion() > 3 || (3 == format.majorVersion() && format.minorVersion() >= 1));
    std::string shaderPrefixPix;
    std::string shaderPrefixVert;
    std::string attribute = "attribute";
    std::string varying_in = "varying";
    std::string varying_out = "varying";
    if (gl3_1plus_)
    {
        shaderPrefixPix = "#version 150 core\n#extension GL_ARB_explicit_attrib_location : enable\n#define texture2D texture\n#define gl_FragColor fragColor\nlayout(location = 0) out vec4 fragColor;\n";
        shaderPrefixVert = "#version 150 core\n";
        attribute = "in";
        varying_in = "in";
        varying_out = "out";
    }
    QOpenGLShader* vshader = new QOpenGLShader(QOpenGLShader::Vertex, this);
    std::string fsrc = shaderPrefixVert + "#ifdef GL_ES\n\
precision highp float;\n\
#endif\n\
uniform mat4 modelViewProjectionMatrix;\n" +
attribute + " vec2 position;\n" +
attribute + " vec2 TexCoord;\n" +
varying_out + " vec2 v_TexCoord; \
void main(void) \
{ \
    gl_Position = modelViewProjectionMatrix * vec4(position, 0.0, 1.0); \
    v_TexCoord = TexCoord; \
}";
    if (!vshader->compileSourceCode(fsrc.c_str()))
    {
        qWarning() << "Shader Error: " << vshader->log();
        assert(0);
        return;
    }

    QOpenGLShader* fshader = new QOpenGLShader(QOpenGLShader::Fragment, this);
    fsrc = shaderPrefixPix + "#ifdef GL_ES\n\
precision highp float;\n\
#endif\n" +
varying_in + " vec2 v_TexCoord; \
uniform sampler2D tex_y; \
uniform sampler2D tex_u; \
uniform sampler2D tex_v; \
uniform float iRadius; \
uniform float iBorder; \
uniform float iStrength; \
uniform vec3 iPosition; \
uniform vec3 iResolution; \
void main(void) \
{ \
    vec3 yuv; \
    vec3 rgb; \
    yuv.x = texture2D(tex_y, v_TexCoord.xy).r; \
    yuv.y = texture2D(tex_u, v_TexCoord.xy).r - 0.5; \
    yuv.z = texture2D(tex_v, v_TexCoord.xy).r - 0.5; \
    rgb = mat3( 1.0,     1.0,       1.0, \
                0.0,     -0.39465,  2.03211, \
                1.13983, -0.58060,  0.0) * yuv; \
    vec2 halfRes = iResolution.xy*0.5; \
    vec2 border = vec2(iBorder, iBorder); \
    float b = length(max(abs(gl_FragCoord.xy - iPosition.xy - halfRes) - halfRes + 0.8 + iRadius, 0.0)) - iRadius; \
    float b2 = length(max(abs(gl_FragCoord.xy - iPosition.xy - halfRes) - halfRes + border + iRadius, 0.0)) - iRadius; \
    rgb = mix(rgb, vec3(0.0,iStrength,0.0), smoothstep(0.0,1.0,b2)); \
    gl_FragColor = vec4(rgb, smoothstep(1.0, 0.0, b)); \
}";
    if (!fshader->compileSourceCode(fsrc.c_str()))
    {
        qWarning() << "Shader Error: " << fshader->log();
        assert(0);
        return;
    }
    prg_ = std::make_unique<QOpenGLShaderProgram>(this);
    prg_->addShader(vshader);
    prg_->addShader(fshader);
    prg_->link();
    prg_->bind();
    prg_->setUniformValue("tex_y", 0);
    prg_->setUniformValue("tex_u", 1);
    prg_->setUniformValue("tex_v", 2);
    prg_->release();

    fshader = new QOpenGLShader(QOpenGLShader::Fragment, this);
    fsrc = shaderPrefixPix + "#ifdef GL_ES\n\
precision highp float;\n\
#define TexCoord v_TexCoord\n\
#endif\n" +
varying_in + " vec2 v_TexCoord; \
uniform sampler2D tex; \
uniform float iRadius; \
uniform float iBorder; \
uniform float iStrength; \
uniform vec3 iPosition; \
uniform vec3 iResolution; \
void main(void) \
{ \
    vec4 rgb = texture2D(tex, v_TexCoord.xy); \
    vec2 halfRes = iResolution.xy*0.5; \
    vec2 border = vec2(iBorder, iBorder); \
    float b = length(max(abs(gl_FragCoord.xy - iPosition.xy - halfRes) - halfRes + 0.8 + iRadius, 0.0)) - iRadius; \
    float b2 = length(max(abs(gl_FragCoord.xy - iPosition.xy - halfRes) - halfRes + border + iRadius, 0.0)) - iRadius; \
    rgb.xyz = mix(rgb.xyz, vec3(0.0,iStrength,0.0), smoothstep(0.0,1.0,b2)); \
    gl_FragColor = vec4(rgb.xyz, min(rgb.a, smoothstep(1.0, 0.0, b))); \
}";
    if (!fshader->compileSourceCode(fsrc.c_str()))
    {
        qWarning() << "Shader Error: " << fshader->log();
        assert(0);
        return;
    }
    prgRGB_ = std::make_unique<QOpenGLShaderProgram>(this);
    prgRGB_->addShader(vshader);
    prgRGB_->addShader(fshader);
    prgRGB_->link();
    prgRGB_->bind();
    prgRGB_->setUniformValue("tex", 0);
    prgRGB_->release();

    QOpenGLFunctions* f = QOpenGLContext::currentContext()->functions();
    f->glGenBuffers(1, &vertexBuffer_);
    f->glGenBuffers(1, &texBuffer_);

    arrowLeft_ = std::make_unique<QOpenGLTexture>(QImage(qsl(":/voip/arrow_left")));
    arrowRight_ = std::make_unique<QOpenGLTexture>(QImage(qsl(":/voip/arrow_right")));
    arrowLeftH_ = std::make_unique<QOpenGLTexture>(QImage(qsl(":/voip/arrow_left_h")));
    arrowRightH_ = std::make_unique<QOpenGLTexture>(QImage(qsl(":/voip/arrow_right_h")));
    muteIcon_ = std::make_unique<QOpenGLTexture>(QImage(qsl(":/voip/mute_icon")));
    icCalling_[0] = std::make_unique<QOpenGLTexture>(QImage(qsl(":/voip/ic_calling_1")));
    icCalling_[1] = std::make_unique<QOpenGLTexture>(QImage(qsl(":/voip/ic_calling_2")));
    icCalling_[2] = std::make_unique<QOpenGLTexture>(QImage(qsl(":/voip/ic_calling_3")));
    arrowLeft_->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
    arrowRight_->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
    arrowLeftH_->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
    arrowRightH_->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
    muteIcon_->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
    icCalling_[0]->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
    icCalling_[1]->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
    icCalling_[2]->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);

    QImage image(QSize(1, 1), QImage::Format_RGBA8888);
    image.fill(QColor(0, 0, 0, 128));
    bgColor_ = std::make_unique<QOpenGLTexture>(image);
    bgColor_->setMinMagFilters(QOpenGLTexture::Nearest, QOpenGLTexture::Nearest);

    connect(&Ui::GetDispatcher()->getVoipController(), &voip_proxy::VoipController::onVoipVadInfo, this, &Ui::OGLRenderWidget::onVoipVadInfo);
    connect(&animationTimer_, &QTimer::timeout, this, [this]()
        {
            animationFrame_ = (animationFrame_ + 1) % 3;
            update();
        });
    animationTimer_.setInterval(kAnimationDuration);
    animationTimer_.start();

    initialized_ = true;
    updatePeers(peerList_);
}

void Ui::OGLRenderWidget::doGridLayout()
{
    QScopedValueRollback locker(updatingFlag_, true);
    items_.clear();
    lay_reset_context(&ctx_);
    lay_id root = lay_item(&ctx_);
    lay_set_size_xy(&ctx_, root, width(), height());
    if (isMiniWindow_ && previewInlayout_ && !isVCS_)
    {
        std::string showedMiniPeer = bigPeerName_.empty() ? PREVIEW_RENDER_NAME : bigPeerName_;
        if (videoPeers_.find(showedMiniPeer) == videoPeers_.end())
            return;
        item it;
        it.id = lay_item(&ctx_);
        it.peer = &videoPeers_[showedMiniPeer];
        lay_insert(&ctx_, root, it.id);
        lay_set_behave(&ctx_, it.id, LAY_VFILL | LAY_HFILL);
        items_.push_back(it);
        lay_run_context(&ctx_);
        return;
    }
    lay_set_contain(&ctx_, root, LAY_ROW);
    lay_id lines_row = lay_item(&ctx_);
    lay_set_contain(&ctx_, lines_row, LAY_COLUMN);
    lay_set_behave(&ctx_, lines_row, LAY_HFILL | LAY_VFILL);

    const int maxPeers = isIncoming_ ? 4 : kMaxVisiblePeers;
    std::vector<lay_id> lines;
    std::vector<Frame*> peers;
    int n = 0;
    for (auto& p : videoPeers_)
    {
        Frame& peer = p.second;
        if (!previewInlayout_ && !isMiniWindow_ && previewRenderName_ == p.first)
            continue;
        n++;
        if (n <= startPosition_)
            continue;
        if ((int)peers.size() < maxPeers)
            peers.push_back(&peer);
    }
    numPeers_ = n;

    const bool enableArrows = n > kMaxVisiblePeers && !isIncoming_;
    const int page = startPosition_ / kMaxVisiblePeers + 1;
    const int pages = (n + kMaxVisiblePeers - 1) / kMaxVisiblePeers;
    lay_id left_arrow = lay_item(&ctx_);
    lay_id right_arrow = lay_item(&ctx_);
    lay_set_margins_ltrb(&ctx_, left_arrow, 4, 0, 2, 0);
    lay_set_margins_ltrb(&ctx_, right_arrow, 2, 0, 4, 0);
    lay_set_size_xy(&ctx_, left_arrow, arrowLeft_->width() / 2, arrowLeft_->height() / 2);
    lay_set_size_xy(&ctx_, right_arrow, arrowRight_->width() / 2, arrowRight_->height() / 2);
    if (enableArrows)
    {
        lay_insert(&ctx_, root, left_arrow);
        item it;
        it.id = left_arrow;
        it.tex = arrowLeft_.get();
        it.text = su::concat(std::to_string(page), "/", std::to_string(pages));
        items_.push_back(it);
    }
    lay_insert(&ctx_, root, lines_row);
    if (enableArrows)
    {
        lay_insert(&ctx_, root, right_arrow);
        item it;
        it.id = right_arrow;
        it.tex = arrowRight_.get();
        it.text = su::concat(std::to_string(page), "/", std::to_string(pages));
        items_.push_back(it);
    }

    static const int lines_tbl[kMaxVisiblePeers + 1] = { 0, 1, 1, 2, 2, 2, 2 /*, 3, 3, 3*/ };
    static const int per_line_tbl[kMaxVisiblePeers + 1] = { 0, 1, 2, 2, 2, 3, 3 /*, 3, 3, 3*/ };
    const int nlines = lines_tbl[peers.size()];
    const int perline = per_line_tbl[peers.size()];
    int x_size = perline ? (width() - (enableArrows ? (arrowLeft_->width() + 12) : 0)) / perline : 0;
    int y_size = x_size * 9 / 16;
    if (y_size * nlines > height())
    {
        y_size = nlines ? height() / nlines : 0;
        x_size = y_size * 16 / 9;
    }
    const bool fitFullSpace = isIncoming_ || isMiniWindow_;
    for (int i = 0; i < nlines; i++)
    {
        lay_id id = lay_item(&ctx_);
        lay_insert(&ctx_, lines_row, id);
        lay_set_contain(&ctx_, id, LAY_ROW);
        lay_set_behave(&ctx_, id, LAY_HFILL | (fitFullSpace ? LAY_VFILL : 0));
        if (!fitFullSpace)
            lay_set_size_xy(&ctx_, id, 0, y_size);
        lines.push_back(id);
    }

    n = 0;
    int line = 0;
    for (auto& p : peers)
    {
        item it;
        it.id = lay_item(&ctx_);
        it.peer = p;
        it.fitInside_ = !isMiniWindow_;
        lay_insert(&ctx_, lines[line], it.id);
        lay_set_behave(&ctx_, it.id, LAY_VFILL | (fitFullSpace ? LAY_HFILL : 0));
        if (!fitFullSpace)
        {
            lay_set_margins_ltrb(&ctx_, it.id, 2, 2, 1, 1);
            lay_set_size_xy(&ctx_, it.id, x_size - 4, 0);
        }
        items_.push_back(it);
        n++;
        if (n == perline)
        {
            n = 0;
            line++;
        }
    }

    lay_run_context(&ctx_);
}

void Ui::OGLRenderWidget::doOneIsBigLayout()
{
    QScopedValueRollback locker(updatingFlag_, true);
    items_.clear();
    lay_reset_context(&ctx_);
    lay_id root = lay_item(&ctx_);
    lay_set_size_xy(&ctx_, root, width(), height());
    lay_set_contain(&ctx_, root, LAY_COLUMN);

    lay_id topline = lay_item(&ctx_);
    lay_set_margins_ltrb(&ctx_, topline, 0, 1, 0, 0);
    lay_set_size_xy(&ctx_, topline, width(), kTopLineHeight);
    lay_set_contain(&ctx_, topline, LAY_ROW);
    lay_insert(&ctx_, root, topline);

    lay_id bigone = lay_item(&ctx_);
    lay_set_behave(&ctx_, bigone, LAY_HFILL | LAY_VFILL);
    lay_insert(&ctx_, root, bigone);

    int w = 0;
    numPeers_ = 0;
    maxPeers_ = 0;
    for (auto& p : videoPeers_)
    {
        if (!previewInlayout_ && previewRenderName_ == p.first)
            continue;
        numPeers_++;
        w += kTopLineItemWidth;
        if (maxPeers_ < kMaxVisiblePeers && w <= (width() - arrowLeft_->width() - kTopLineHMargin * 2))
            maxPeers_++;
    }
    if (startPosition_ >= numPeers_)
        startPosition_ = std::max(0, numPeers_ - 1);
    if ((numPeers_ - startPosition_) < maxPeers_)
        startPosition_ = std::max(0, numPeers_ - maxPeers_);
    bool enableArrows = startPosition_ > 0;
    numPeers_ = 0;
    maxPeers_ = 0;
    w = 0;
    for (auto& p : videoPeers_)
    {
        if (!previewInlayout_ && previewRenderName_ == p.first)
            continue;
        numPeers_++;
        if (numPeers_ <= startPosition_)
            continue;
        w += kTopLineItemWidth;
        if (maxPeers_ < kMaxVisiblePeers && w <= (width() - arrowLeft_->width() - kTopLineHMargin * 2))
            maxPeers_++;
        else
            enableArrows = true;
    }
    lay_id left_arrow = lay_item(&ctx_);
    lay_id right_arrow = lay_item(&ctx_);
    lay_set_margins_ltrb(&ctx_, left_arrow, 5, 0, 5, 0);
    lay_set_margins_ltrb(&ctx_, right_arrow, 5, 0, 5, 0);
    lay_set_size_xy(&ctx_, left_arrow, arrowLeft_->width() / 2, arrowLeft_->height() / 2);
    lay_set_size_xy(&ctx_, right_arrow, arrowRight_->width() / 2, arrowRight_->height() / 2);
    if (enableArrows)
    {
        lay_insert(&ctx_, topline, left_arrow);
        item it;
        it.id = left_arrow;
        it.tex = arrowLeft_.get();
        items_.push_back(it);
    }

    Frame* bigPeer = nullptr;
    int n = 0;
    int peers = 0;
    for (auto& p : videoPeers_)
    {
        Frame& peer = p.second;
        if (!previewInlayout_ && previewRenderName_ == p.first)
            continue;
        if (!bigPeer || bigPeerName_ == p.first)
            bigPeer = &peer;
        n++;
        if (n <= startPosition_)
            continue;
        if (peers < maxPeers_)
        {
            peers++;
            item it;
            it.id = lay_item(&ctx_);
            it.peer = &peer;
            lay_insert(&ctx_, topline, it.id);
            lay_set_margins_ltrb(&ctx_, it.id, 2, 0, 1, 0);
            lay_set_size_xy(&ctx_, it.id, kTopLinePeerSize.width(), kTopLinePeerSize.height());
            lay_set_behave(&ctx_, it.id, LAY_HCENTER | LAY_VCENTER);
            items_.push_back(it);
        }
    }

    if (enableArrows)
    {
        lay_insert(&ctx_, topline, right_arrow);
        item it;
        it.id = right_arrow;
        it.tex = arrowRight_.get();
        items_.push_back(it);
    }

    if (bigPeer)
    {
        item it;
        it.id = bigone;
        it.peer = bigPeer;
        it.fitInside_ = true;
        items_.push_back(it);
    }

    lay_run_context(&ctx_);
}

static void ortho(GLfloat m[4][4], GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat nearVal, GLfloat farVal)
{
   m[0][0] = 2.0F / (right - left);
   m[1][0] = 0.0F;
   m[2][0] = 0.0F;
   m[3][0] = -(right + left) / (right - left);

   m[0][1] = 0.0F;
   m[1][1] = 2.0F / (top - bottom);
   m[2][1] = 0.0F;
   m[3][1] = -(top + bottom) / (top - bottom);

   m[0][2] = 0.0F;
   m[1][2] = 0.0F;
   m[2][2] = -2.0F / (farVal - nearVal);
   m[3][2] = -(farVal + nearVal) / (farVal - nearVal);

   m[0][3] = 0.0F;
   m[1][3] = 0.0F;
   m[2][3] = 0.0F;
   m[3][3] = 1.0F;
}

void Ui::OGLRenderWidget::paintGL()
{
    if (!initialized_)
        return;
#ifdef __APPLE__
    QSize fbSize = platform_macos::framebufferSize(window());
    scaleX_ = ((float)fbSize.width()) / window()->width();
    scaleY_ = ((float)fbSize.height()) / window()->height();
#endif
    QOpenGLFunctions* f = QOpenGLContext::currentContext()->functions();
    f->glClearColor(34.0 / 256.0, 34.0 / 256.0, 34.0 / 256.0, 1);
    f->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    GLfloat projection[4][4];
    ortho(projection, 0, floor(width() * scaleX_ + 0.5f), floor(height() * scaleY_ + 0.5f), 0, -999999, 999999);
    prg_->bind();
    prg_->setUniformValue("modelViewProjectionMatrix", projection);
    prg_->release();
    prgRGB_->bind();
    prgRGB_->setUniformValue("modelViewProjectionMatrix", projection);
    prgRGB_->release();

    if (videoPeers_.begin() == videoPeers_.end())
        return;

    for (auto& p : videoPeers_)
    {
        Frame& peer = p.second;
        peer.renderWidth_  = 1;
        peer.renderHeight_ = 1;
    }
    if (oneIsBig_)
    {
        if (!isVCS_ && !isMiniWindow_ && peerList_.contacts.size() < 2)
            doGridLayout();
        else
            doOneIsBigLayout();
    }
    else
    {
        doGridLayout();
    }

    for (auto& p : items_)
    {
        lay_vec4 r = lay_get_rect(&ctx_, p.id);
        const QRect rect{ r[0], r[1], r[2], r[3] };
        if (p.peer)
        {
            if (p.peer->renderWidth_ < rect.width() || p.peer->renderHeight_ < rect.height())
            {
                p.peer->renderWidth_  = rect.width();
                p.peer->renderHeight_ = rect.height();
            }
            drawParticipant(*p.peer, rect, p.fitInside_, p.fitInside_ ? 1.0 : kAvatarScaleFactor);
        }
        if (p.tex)
        {
            QOpenGLTexture* tex = p.tex;
            if (rect.contains(mouseX_, mouseY_))
            {
                if (tex == arrowLeft_.get())
                    tex = arrowLeftH_.get();
                else if (tex == arrowRight_.get())
                    tex = arrowRightH_.get();
            }
            drawImage(tex, ImageOption(rect));
        }
        if (!p.text.empty())
        {
            QString s = QString::fromStdString(p.text);
            QRect txt_r = renderTextSize(s);
            renderText(r[0] + arrowLeft_->width() / 4 - txt_r.width() / 2, r[1] + kButtonTextOffset, s, false);
        }
    }

    if (!isMiniWindow_ && !isIncoming_)
    {
        /* Send render sizes to voip lib */
        bool sizesChanged = false;
        size_t count = 0;
        for (auto& p : videoPeers_)
        {
            if (PREVIEW_RENDER_NAME == p.first || !p.second.videoEnable_)
                continue;
            count++;
            QSize size = QSize(0, 0);
            if (videoPeerSizes_.find(p.first) != videoPeerSizes_.end())
                size = videoPeerSizes_[p.first];
            Frame& peer = p.second;
            if (maxVoiceName_ == p.first && peer.renderWidth_ <= 1 && peer.renderHeight_ <= 1)
            {
                peer.renderWidth_ = kMiniWindowSize.width();
                peer.renderHeight_ = kMiniWindowSize.height();
            }
            if (size.width() != peer.renderWidth_ || size.height() != peer.renderHeight_)
            {
                sizesChanged = true;
                Ui::GetDispatcher()->getVoipController().setRenderResolution(p.first, peer.renderWidth_, peer.renderHeight_);
            }
        }
        if (sizesChanged || videoPeerSizes_.size() != count)
        {
            videoPeerSizes_.clear();
            for (auto& p : videoPeers_)
            {
                if (PREVIEW_RENDER_NAME == p.first || !p.second.videoEnable_)
                    continue;
                Frame& peer = p.second;
                videoPeerSizes_[p.first] = QSize(peer.renderWidth_, peer.renderHeight_);
            }
        }
    }

    if (displayPreview_ && !previewInlayout_)
    {
        auto it = videoPeers_.find(previewRenderName_);
        if (it != videoPeers_.end())
            drawParticipant(it->second, previewRect(), true, kAvatarScaleFactor);
    }
}

void Ui::OGLRenderWidget::drawParticipant(const Frame& _peer, const QRect& _pos, bool _fitInside, float _scale)
{
    const QRect wndRect = QHighDpi::toNativePixels(rect(), window()->windowHandle());
    const QRect drawRect = QHighDpi::toNativePixels(_pos, window()->windowHandle());
    int width = _peer.width_, height = _peer.height_;
    ImageOption imageOption(_pos);
    if (!width || !height || !_peer.videoEnable_)
    {
        imageOption.iRadius_ = (isIncoming_ || isVCS_ || isMiniWindow_) ? 0.f : 12.f * scaleX_;
        imageOption.iBorder_ = _peer.voice_ ? kFrameBorderWidth * std::max(scaleX_, scaleY_) : 0.f;
        imageOption.iStrength_ = _peer.iStrength_;
        if (_peer.avatarBlur_)
        {
            imageOption.fitPolicy_ = ImageFitPolicy::FitOutside;
            drawImage(_peer.avatarBlur_.get(), imageOption);
        }
        if (_peer.avatarImage_)
        {
            imageOption.iBorder_ = 0.0;
            imageOption.iStrength_ = 0.0;
            imageOption.fitPolicy_ = ImageFitPolicy::FitIgnore;
            imageOption.iScale_ = _scale;
            drawImage(_peer.avatarImage_.get(), imageOption);
        }

        imageOption.iStrength_ = _peer.iStrength_;
        drawCaption(_peer, _pos);
        return;
    }

    imageOption.reset(_pos, 12.0f * scaleX_);
    drawImage(bgColor_.get(), imageOption);

    if (90 == _peer.angle_ || 270 == _peer.angle_)
        std::swap(width, height);

    QRect r;
    const float scalex = (float)drawRect.width() / width;
    const float scaley = (float)drawRect.height() / height;
    float left, top, right, bottom;
    if (_fitInside)
    {
        const float bestScale = std::min(scalex, scaley);
        left = top = 0.0f;
        right = bottom = 1.0f;
        r = QRect(drawRect.left() + (drawRect.width() - width * bestScale) / 2 + 0.5f, drawRect.top() + (drawRect.height() - height * bestScale) / 2 + 0.5f, width * bestScale, height * bestScale);
        if (std::abs(r.left() - drawRect.left()) <= kAspectEpsilon && std::abs(r.right() - drawRect.right()) <= kAspectEpsilon &&
            std::abs(r.top() - drawRect.top()) <= kAspectEpsilon && std::abs(r.bottom() - drawRect.bottom()) <= kAspectEpsilon)
            r = drawRect; // use all space if aspect ratio is very close
    }
    else
    {
        const float bestScale = std::max(scalex, scaley);
        left = ((float)(width * bestScale) - drawRect.width()) / (width * bestScale) / 2.0f;
        top = ((float)(height * bestScale) - drawRect.height()) / (height * bestScale) / 2.0f;
        if (90 == _peer.angle_ || 270 == _peer.angle_)
            std::swap(left, top);
        right = 1.0f - left;
        bottom = 1.0f - top;
        r = drawRect;
    }

    GLfloat texVerts[] = {
        left,
        top,
        right,
        top,
        left,
        bottom,
        right,
        bottom,
    };

    GLfloat verts[8];
    left = r.left() * scaleX_ + 0.5f;
    top = r.top() * scaleY_ + 0.5f;
    right = (r.right() + 1) * scaleX_ - 1.0f + 0.5f;
    bottom = (r.bottom() + 1) * scaleY_ - 1.0f + 0.5f;
    if (_peer.doMirroring_)
        std::swap(left, right);
    switch (_peer.angle_)
    {
        default:
        case 0:
            verts[0] = left, verts[1] = top;
            verts[2] = right, verts[3] = top;
            verts[4] = left, verts[5] = bottom;
            verts[6] = right, verts[7] = bottom;
            break;
        case 90:
            verts[0] = right, verts[1] = top;
            verts[2] = right, verts[3] = bottom;
            verts[4] = left, verts[5] = top;
            verts[6] = left, verts[7] = bottom;
            break;
        case 180:
            verts[0] = right, verts[1] = bottom;
            verts[2] = left, verts[3] = bottom;
            verts[4] = right, verts[5] = top;
            verts[6] = left, verts[7] = top;
            break;
        case 270:
            verts[0] = left, verts[1] = bottom;
            verts[2] = left, verts[3] = top;
            verts[4] = right, verts[5] = bottom;
            verts[6] = right, verts[7] = top;
            break;
    }

    QOpenGLFunctions* f = QOpenGLContext::currentContext()->functions();
    f->glActiveTexture(GL_TEXTURE0);
    _peer.texY_->bind();
    f->glActiveTexture(GL_TEXTURE1);
    _peer.texU_->bind();
    f->glActiveTexture(GL_TEXTURE2);
    _peer.texV_->bind();

    f->glDisable(GL_DEPTH_TEST);
    f->glEnable(GL_BLEND);
    f->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    prg_->bind();
    prg_->setUniformValue("iRadius", (isIncoming_ || isVCS_ || isMiniWindow_) ? 0.f : 12.f * scaleX_);
    prg_->setUniformValue("iBorder", _peer.voice_ ? kFrameBorderWidth * std::max(scaleX_, scaleY_) : 0.f);
    prg_->setUniformValue("iStrength", _peer.iStrength_);
    prg_->setUniformValue("iPosition", r.x() * scaleX_ - 0.5f, (wndRect.height() - 1.0f - r.bottom()) * scaleY_ - 0.5f, 0.f);
    prg_->setUniformValue("iResolution", r.width() * scaleX_, r.height() * scaleY_, 0.f);
    int loc_pos = prg_->attributeLocation("position");
    int loc_tex = prg_->attributeLocation("TexCoord");
    f->glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer_);
    f->glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_DYNAMIC_DRAW);
    f->glVertexAttribPointer(loc_pos, 2, GL_FLOAT, false, 0, nullptr);
    f->glEnableVertexAttribArray(loc_pos);
    f->glBindBuffer(GL_ARRAY_BUFFER, texBuffer_);
    f->glBufferData(GL_ARRAY_BUFFER, sizeof(texVerts), texVerts, GL_DYNAMIC_DRAW);
    f->glVertexAttribPointer(loc_tex, 2, GL_FLOAT, false, 0, nullptr);
    f->glEnableVertexAttribArray(loc_tex);
    f->glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    f->glDisableVertexAttribArray(loc_pos);
    f->glDisableVertexAttribArray(loc_tex);
    prg_->release();
    f->glBindBuffer(GL_ARRAY_BUFFER, 0);

    f->glBindTexture(GL_TEXTURE_2D, 0);
    f->glActiveTexture(GL_TEXTURE1);
    f->glBindTexture(GL_TEXTURE_2D, 0);
    f->glActiveTexture(GL_TEXTURE0);
    f->glBindTexture(GL_TEXTURE_2D, 0);

    f->glDisable(GL_BLEND);
    f->glFinish();

    drawCaption(_peer, r);
}

void Ui::OGLRenderWidget::drawImage(QOpenGLTexture* _tex, const ImageOption& _options)
{
    const QRect wndRect = QHighDpi::toNativePixels(rect(), window()->windowHandle());
    const QRect drawRect = QHighDpi::toNativePixels(_options.pos_, window()->windowHandle());
    const float width = _tex->width() * _options.iScale_, height = _tex->height() * _options.iScale_;

    QRect r;
    const float scalex = (float)drawRect.width() / width;
    const float scaley = (float)drawRect.height() / height;
    float left, top, right, bottom;
    float bestScale = 1.0f;
    switch (_options.fitPolicy_)
    {
    case ImageFitPolicy::FitOutside:
        bestScale = std::max(scalex, scaley);
        left = ((float)(width * bestScale) - drawRect.width()) / (width * bestScale) * 0.5f;
        right = 1.0f - left;
        top = ((float)(height * bestScale) - drawRect.height()) / (height * bestScale) * 0.5f;
        bottom = 1.0f - top;
        r = drawRect;
        break;
    case ImageFitPolicy::FitInside:
        bestScale = std::min(scalex, scaley);
        left = top = 0.0f;
        right = bottom = 1.0f;
        r = QRect(drawRect.left() + (drawRect.width() - width * bestScale) / 2 + 0.5f, drawRect.top() + (drawRect.height() - height * bestScale) / 2 + 0.5f, width * bestScale, height * bestScale);
        break;
    case ImageFitPolicy::FitIgnore:
    default:
        r = QRect(0, 0, width, height);
        r.moveCenter(drawRect.center());
        left = top = 0.0f;
        right = bottom = 1.0f;
        break;
    }

    GLfloat texVerts[] = {
        left,
        top,
        right,
        top,
        left,
        bottom,
        right,
        bottom,
    };

    left = r.left() * scaleX_ + 0.5f;
    top = r.top() * scaleY_ + 0.5f;
    right = (r.right() + 1) * scaleX_ - 1.0f + 0.5f;
    bottom = (r.bottom() + 1) * scaleY_ - 1.0f + 0.5f;
    GLfloat verts[8];
    verts[0] = left, verts[1] = top;
    verts[2] = right, verts[3] = top;
    verts[4] = left, verts[5] = bottom;
    verts[6] = right, verts[7] = bottom;

    QOpenGLFunctions* f = QOpenGLContext::currentContext()->functions();
    f->glActiveTexture(GL_TEXTURE0);
    _tex->bind();

    f->glDisable(GL_DEPTH_TEST);
    f->glEnable(GL_BLEND);
    f->glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE);
    prgRGB_->bind();
    prgRGB_->setUniformValue("iRadius", _options.iRadius_);
    prgRGB_->setUniformValue("iBorder", _options.iBorder_);
    prgRGB_->setUniformValue("iStrength", _options.iStrength_);
    prgRGB_->setUniformValue("iPosition", r.x() * scaleX_ - 0.5f, (wndRect.height() - 1.0f - r.bottom()) * scaleY_ - 0.5f, 0.f);
    prgRGB_->setUniformValue("iResolution", r.width() * scaleX_, r.height() * scaleY_, 0.f);
    int loc_pos = prgRGB_->attributeLocation("position");
    int loc_tex = prgRGB_->attributeLocation("TexCoord");
    f->glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer_);
    f->glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_DYNAMIC_DRAW);
    f->glVertexAttribPointer(loc_pos, 2, GL_FLOAT, false, 0, nullptr);
    f->glEnableVertexAttribArray(loc_pos);
    f->glBindBuffer(GL_ARRAY_BUFFER, texBuffer_);
    f->glBufferData(GL_ARRAY_BUFFER, sizeof(texVerts), texVerts, GL_DYNAMIC_DRAW);
    f->glVertexAttribPointer(loc_tex, 2, GL_FLOAT, false, 0, nullptr);
    f->glEnableVertexAttribArray(loc_tex);
    f->glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    f->glDisableVertexAttribArray(loc_pos);
    f->glDisableVertexAttribArray(loc_tex);
    prgRGB_->release();
    _tex->release();
    f->glBindBuffer(GL_ARRAY_BUFFER, 0);
    f->glDisable(GL_BLEND);
    f->glFinish();
}

void Ui::OGLRenderWidget::drawCaption(const Frame& _peer, const QRect& _pos)
{
    QString text = _peer.friendlyName_;
    if (text.isEmpty() || isVCS_ || isIncoming_)
        return;

    const QRect boundsRect = _pos.marginsRemoved(kCaptionMargins);
    QOpenGLTexture* currentIcon = nullptr;
    QRect iconRect;
    QRect textRect;
    QRect captionRect = captionArea(boundsRect);

    // calculate icon rect
    if (_peer.micMute_ || !_peer.connectedState_)
    {
        const int w = std::max(muteIcon_->width() / 2, icCalling_[0]->width() / 2);
        const int h = std::max(muteIcon_->height() / 2, icCalling_[0]->height() / 2);
        iconRect.setSize({ w, h });
        iconRect.moveLeft(captionRect.left() + kCaptionItemMargins.left());
        iconRect.moveTop(captionRect.top() + kCaptionItemMargins.top());
        if (!_peer.connectedState_)
            currentIcon = icCalling_[animationFrame_].get();
        else if (_peer.micMute_)
            currentIcon = muteIcon_.get();
    }

    // calculate text rect
    textRect = captionRect.marginsRemoved(kCaptionItemMargins);
    if (!iconRect.isEmpty())
        textRect.setLeft(iconRect.right() + kCaptionIconSpacing);

    QString elided = text;
    QRect bound;
    while (elided.length() && (((bound = renderTextSize(text)).width()) > textRect.width()))
    {
        elided.remove(elided.length() - 1, 1);
        text = elided + qsl("...");
    }
    bound.setTop(bound.top() + 4);
    bound.moveTo(textRect.topLeft());

    textRect = bound;

    if (text != _peer.friendlyName_)
    {
        // adjust icon and text rects if text was elided
        const QPoint center = boundsRect.center();
        const QPoint d = center - captionRect.center();
        textRect.translate(d.x(), 0);
        iconRect.translate(d.x(), 0);
    }
    else
    {
        // adjust caption rect
        captionRect.setRight(textRect.right() + kCaptionItemMargins.right());
    }

    // draw background
    drawImage(bgColor_.get(), ImageOption(captionRect, 9.5f * scaleX_));

    // draw icon
    if (currentIcon && !iconRect.isEmpty())
    {
        QRect rc{ 0, 0, currentIcon->width() / 2, currentIcon->height() / 2 };
        rc.moveCenter(iconRect.center());
        drawImage(currentIcon, rc);
    }

    // draw text
    if (!textRect.isEmpty())
        renderText(textRect.left(), textRect.bottom(), text);
}

QRect Ui::OGLRenderWidget::captionArea(const QRect& _frameRect) const
{
    QRect captionArea{ 0, 0, _frameRect.width(), kCaptionHeight };
    captionArea.moveBottomLeft(_frameRect.bottomLeft());
    return captionArea;
}

QRect Ui::OGLRenderWidget::previewRect() const
{
    return QRect(width() - kPreviewSize.width() - kPreviewMargin.width(), kPreviewMargin.height(), kPreviewSize.width(), kPreviewSize.height());
}

QRect Ui::OGLRenderWidget::tooltipRect(const QRect& _frameRect, const Frame& _peer) const
{
    if (_peer.friendlyName_.isEmpty())
        return QRect{};

    const QRect captionRect = captionArea(_frameRect.marginsRemoved(kCaptionMargins));

    QRect textRect = captionRect.marginsRemoved(kCaptionItemMargins);
    if (_peer.micMute_ || !_peer.connectedState_)
    {
        const int w = std::max(muteIcon_->width() / 2, icCalling_[0]->width() / 2);
        const int h = std::max(muteIcon_->height() / 2, icCalling_[0]->height() / 2);
        QRect iconRect{ 0, 0, w, h };
        iconRect.moveLeft(captionRect.left() + kCaptionItemMargins.left());
        iconRect.moveTop(captionRect.top() + kCaptionItemMargins.top());
        textRect.setLeft(iconRect.right() + kCaptionIconSpacing);
    }
    QString text = _peer.friendlyName_;
    QString elided = text;
    QRect bound;
    while (elided.length() && (((bound = renderTextSize(text)).width()) > textRect.width()))
    {
        elided.remove(elided.length() - 1, 1);
        text = elided + qsl("...");
    }

    if (text == _peer.friendlyName_)
        return QRect{};
    else
        return captionRect;
}

QRect Ui::OGLRenderWidget::renderTextSize(const QString& _text) const
{
    const QFontMetrics fontMetrics(font_);
    Ui::CompiledText compiledText;
    Ui::CompiledText::compileText(_text, compiledText, false, false);
    return QRect(0, 0, compiledText.width(fontMetrics), compiledText.height(fontMetrics));
}

void Ui::OGLRenderWidget::renderText(int _x, int _y, const QString& _text, bool _showBackground, QMargins _margins)
{
    const QFontMetrics fontMetrics(font_);
    Ui::CompiledText compiledText;
    Ui::CompiledText::compileText(_text, compiledText, false, true);

    const int w = compiledText.width(fontMetrics);

    QPainter painter(this);
    painter.setPen(Qt::white);
    painter.setFont(font_);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    if constexpr (platform::is_apple())
    {
        QTransform t;
        // make qFuzzyIsNull fail and choose TxProject type and workaround macos Qt bug with external display with different scale
        t.setMatrix(t.m11(), t.m12(), t.m13(), t.m21(), t.m22(), t.m23(), t.m31(), t.m32(), t.m33() + 0.000000000002);
        painter.setTransform(t);
    }

    compiledText.draw(painter, _x, _y, w);
}

void Ui::OGLRenderWidget::setFrame(const im::IVideoFrame* _frame, const std::string& _peerId)
{
    if (!initialized_ || (videoPeers_.find(_peerId) == videoPeers_.end()))
        return;

    im::YuvBuffer buffer = _frame->getYUV(); // call before makeCurrent() because of potention texture operations

    makeCurrent();
    assert(QOpenGLContext::currentContext()->isValid());

    Frame& peer = videoPeers_[_peerId];
    peer.width_ = _frame->width();
    peer.height_ = _frame->height();
    peer.angle_ = (int)_frame->getRotation();
    peer.doMirroring_ = !isScreenSharing_ && _peerId == PREVIEW_RENDER_NAME;

    QOpenGLFunctions* f = QOpenGLContext::currentContext()->functions();
    f->glActiveTexture(GL_TEXTURE0);
    peer.texY_->bind();
    f->glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    f->glPixelStorei(GL_UNPACK_ROW_LENGTH, buffer.strideY);
    f->glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, _frame->width(), _frame->height(), 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, buffer.y);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    f->glActiveTexture(GL_TEXTURE1);
    peer.texU_->bind();
    f->glPixelStorei(GL_UNPACK_ROW_LENGTH, buffer.strideU);
    f->glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, _frame->width() / 2, _frame->height() / 2, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, buffer.u);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    f->glActiveTexture(GL_TEXTURE2);
    peer.texV_->bind();
    f->glPixelStorei(GL_UNPACK_ROW_LENGTH, buffer.strideV);
    f->glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, _frame->width() / 2, _frame->height() / 2, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, buffer.v);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    f->glBindTexture(GL_TEXTURE_2D, 0);
    f->glActiveTexture(GL_TEXTURE1);
    f->glBindTexture(GL_TEXTURE_2D, 0);
    f->glActiveTexture(GL_TEXTURE0);
    f->glBindTexture(GL_TEXTURE_2D, 0);

    f->glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    f->glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    f->glFinish();
    doneCurrent();
    update();
}

void Ui::OGLRenderWidget::clear()
{
    Tooltip::hide();
    QScopedValueRollback guard(updatingFlag_);
    if (!initialized_)
        return;
    makeCurrent();
    videoPeers_.clear();
    items_.clear();
    startPosition_ = 0;
    numPeers_ = 0;
    displayPreview_ = false;
    oneIsBig_ = false;
    viewMode_ = ViewMode::GridMode;
    isVCS_ = false;
    previewRenderName_ = PREVIEW_RENDER_NAME;
    doneCurrent();
    update();
}

void Ui::OGLRenderWidget::setRenderLayout(bool _oneIsBig)
{
    oneIsBig_ = _oneIsBig;
    if (!oneIsBig_)
        startPosition_ = startPosition_ / kMaxVisiblePeers * kMaxVisiblePeers;

    if (!isVCS_ && !isMiniWindow_ && peerList_.contacts.size() < 2)
    {
        if (!oneIsBig_)
        {
            displayPreview_ = false;
            previewInlayout_ = true;
        }
        else
        {
            displayPreview_ = true;
            previewInlayout_ = false;
        }
    }

    viewMode_ = oneIsBig_ ? ViewMode::SpeakerMode : ViewMode::GridMode;
    update();
}

void Ui::OGLRenderWidget::toggleRenderLayout(const QPoint& _clickPos)
{
    if (peerList_.contacts.empty())
        return;

    if (displayPreview_ && !previewInlayout_)
    {
        if (previewRect().contains(_clickPos))
            previewRenderName_ = (previewRenderName_ == PREVIEW_RENDER_NAME) ? bigPeerName_ : PREVIEW_RENDER_NAME;
        else
            setRenderLayout(!oneIsBig_);

        Tooltip::hide();
        update();
        return;
    }

    if (oneIsBig_ && QRect(0, 0, width(), kTopLineHeight).contains(_clickPos))
        return;

    std::string_view bigPeer = peerAt(_clickPos);
    if (!isVCS_ && !isMiniWindow_ && peerList_.contacts.size() == 1)
    {
        setRenderLayout(!oneIsBig_);
        if (displayPreview_ && !previewInlayout_)
        {
            bigPeerName_ = bigPeer;
            if (bigPeerName_ == PREVIEW_RENDER_NAME)
                previewRenderName_ = peerList_.contacts.front().contact;
            else
                previewRenderName_ = PREVIEW_RENDER_NAME;
        }
        update();
        return;
    }

    if (!oneIsBig_ && bigPeer.empty())
        return;

    setRenderLayout(!oneIsBig_);
    bigPeerName_ = bigPeer;
    Tooltip::hide();
    update();
}

void Ui::OGLRenderWidget::setViewMode(ViewMode _mode)
{
    if (_mode == viewMode_)
        return;

    viewMode_ = _mode;
    switch (viewMode_)
    {
    case ViewMode::SpeakerMode:
        setRenderLayout(true);
        break;
    case ViewMode::GridMode:
        setRenderLayout(false);
        break;
    }
    update();
}

Ui::IRender::ViewMode Ui::OGLRenderWidget::getViewMode() const
{
    return viewMode_;
}

void Ui::OGLRenderWidget::setBigPeer(const std::string& _bigPeerName)
{
    bigPeerName_ = _bigPeerName;
    update();
}

void Ui::OGLRenderWidget::setVCS(bool _isVCS)
{
    Tooltip::hide();
    QScopedValueRollback guard(updatingFlag_);
    isVCS_ = _isVCS;
    displayPreview_ = !_isVCS && !isMiniWindow_;
    makeCurrent();
    videoPeers_.clear();
    items_.clear();
    doneCurrent();
    if (peerList_.contacts.size())
        updatePeers(peerList_);
    update();
}

void Ui::OGLRenderWidget::setPinnedRoom(bool _isPinnedRoom)
{
    isPinnedRoom_ = _isPinnedRoom;
    previewInlayout_ = _isPinnedRoom;
    update();
}

void Ui::OGLRenderWidget::setIncoming(bool _isIncoming)
{
    isIncoming_ = _isIncoming;
}

void Ui::OGLRenderWidget::setMiniWindow(bool _isMiniWindow)
{
    isMiniWindow_ = _isMiniWindow;
}

void Ui::OGLRenderWidget::mousePressEvent(QMouseEvent* _e)
{
    if (_e->button() == Qt::RightButton || updatingFlag_)
        return;

    QOpenGLWidget::mousePressEvent(_e);
    const int maxPeers = oneIsBig_ ? maxPeers_ : kMaxVisiblePeers;
    const int increment = oneIsBig_ ? 1 : kMaxVisiblePeers;
    for (auto& p : items_)
    {
        if (p.tex)
        {
            lay_vec4 r = lay_get_rect(&ctx_, p.id);
            QRect rect = QRect(r[0], r[1], r[2], r[3]);
            if (rect.contains(_e->x(), _e->y()))
            {
                if (p.tex == arrowLeft_.get())
                    startPosition_ = std::max(0, startPosition_ - increment);
                else if ((startPosition_ + maxPeers) < numPeers_)
                    startPosition_ += increment;
                update();
            }
        }
    }

    const auto peerId = peerAt(_e->pos());
    if (peerId.empty())
        return;

    bigPeerName_ = peerId;
    update();
}

void Ui::OGLRenderWidget::mouseMoveEvent(QMouseEvent* _e)
{
    if (updatingFlag_)
    {
        QOpenGLWidget::mouseMoveEvent(_e);
        return;
    }

    QOpenGLWidget::mouseMoveEvent(_e);
    mouseX_ = _e->x();
    mouseY_ = _e->y();
    bool cursorHand = false;

    QString tooltip;
    QRect frameRect;
    QRect tooltipArea;
    std::string_view currentActiveId;
    const QRect previewRc = previewRect();
    if (displayPreview_ && !previewInlayout_)
    {
        if (!peerAt(_e->pos()).empty())
            cursorHand = true;

        auto it = videoPeers_.find(previewRenderName_);
        if (it != videoPeers_.end() && previewRc.contains(_e->pos()))
        {
            cursorHand = true;
            tooltipArea = tooltipRect(previewRc, it->second);
            if (!tooltipArea.isNull() && tooltipArea.contains(_e->pos()))
            {
                currentActiveId = previewRenderName_;
                if (activeItemId_ != currentActiveId)
                {
                    tooltip = it->second.friendlyName_;
                    frameRect = previewRc;
                }
            }
        }
    }
    else
    {
        for (auto& p : items_)
        {
            lay_vec4 r = lay_get_rect(&ctx_, p.id);
            frameRect.setRect(r[0], r[1], r[2], r[3]);
            if (!frameRect.contains(_e->pos()) || !p.peer)
                continue;

            cursorHand = true;
            tooltipArea = tooltipRect(frameRect, *p.peer);
            if (tooltipArea.isNull() || !tooltipArea.contains(_e->pos()))
                continue;

            currentActiveId = p.peer->peerId_;
            if (activeItemId_ == currentActiveId)
                continue;

            tooltip = p.peer->friendlyName_;
            break;
        }
    }

    if (activeItemId_ != currentActiveId && !tooltip.isEmpty())
    {
        Tooltip::hide();
        connect(tooltipTimer_, &QTimer::timeout, this, [this, tooltipArea, frameRect, tooltip]()
        {
            showToolTip(tooltipArea, frameRect, tooltip);
        }, Qt::UniqueConnection);
        tooltipTimer_->start();
    }

    activeItemId_ = currentActiveId;

    if (currentCursorHand_ != cursorHand)
    {
        currentCursorHand_ = cursorHand;
        setCursor(cursorHand ? Qt::PointingHandCursor : Qt::ArrowCursor);
        update();
    }
}

bool Ui::OGLRenderWidget::event(QEvent* _event)
{
    switch (_event->type())
    {
    case QEvent::Hide:
    case QEvent::Leave:
    case QEvent::HoverLeave:
        if (Tooltip::isVisible())
            Tooltip::hide();
        break;
    default:
        break;
    }
    return QOpenGLWidget::event(_event);
}

void Ui::OGLRenderWidget::showToolTip(const QRect& _area, const QRect& _frameRect, const QString& _text)
{
    if ((!(displayPreview_ && !previewInlayout_) && activeItemId_ != peerAt(_frameRect.center())) || activeItemId_.empty())
    {
        Tooltip::hide();
        return;
    }
    QRect area = _area;
    QString text = _text;
    area.moveTo(mapToGlobal(area.topLeft()));
    Tooltip::show(text.replace(ql1c(' '), ql1c('\n')), area, {}, Tooltip::ArrowDirection::Down, Tooltip::ArrowPointPos::Top, Ui::TextRendering::HorAligment::CENTER, _frameRect, Tooltip::TooltipMode::Multiline, this);
}


QMenu* Ui::OGLRenderWidget::createContextMenu(const QPoint& _pos) const
{
    std::string_view aimid;
    if (displayPreview_ && !previewInlayout_ && previewRect().contains(_pos))
        aimid = previewRenderName_;
    else
        aimid = peerAt(_pos);

    if (aimid.empty() || aimid == PREVIEW_RENDER_NAME || isVCS_)
        return nullptr;

    const QString contact = QString::fromUtf8(aimid.data(), aimid.size());
    Previewer::CustomMenu* menu = new Previewer::CustomMenu(const_cast<OGLRenderWidget*>(this));
    menu->setArrowSize({});
    menu->setAttribute(Qt::WA_DeleteOnClose);

    QAction* gotoChatAct = new QAction(QT_TRANSLATE_NOOP("voip_video_panel", "Go to the chat"), menu);
    connect(gotoChatAct, &QAction::triggered, this, [contact]()
    {
        Utils::InterConnector::instance().getMainWindow()->activate();
        Utils::InterConnector::instance().openDialog(contact);
    });
    menu->addAction(gotoChatAct, QPixmap{});

    QAction* profileAct = new QAction(QT_TRANSLATE_NOOP("voip_video_panel", "Open the profile"), menu);
    connect(profileAct, &QAction::triggered, this, [contact]()
    {
        Utils::InterConnector::instance().getMainWindow()->activate();
        Utils::InterConnector::instance().openDialog(contact);
        Utils::InterConnector::instance().showSidebar(contact);
    });
    menu->addAction(profileAct, QPixmap{});

    connect(menu, &QMenu::aboutToShow, this, [this]() { tooltipTimer_->stop(); Tooltip::hide(); });

    return menu;
}

static QImage blurred(const QImage& image, const QRect& rect, int radius, bool alphaOnly = false)
{
    int tab[] = { 14, 10, 8, 6, 5, 5, 4, 3, 3, 3, 3, 2, 2, 2, 2, 2, 2 };
    int alpha = (radius < 1) ? 16 : (radius > 17) ? 1
                                                  : tab[radius - 1];

    QImage result = image.convertToFormat(QImage::Format_ARGB32_Premultiplied);
    const int r1 = rect.top();
    const int r2 = rect.bottom();
    const int c1 = rect.left();
    const int c2 = rect.right();

    const int bpl = result.bytesPerLine();
    int rgba[4];
    unsigned char* p;

    int i1 = 0;
    int i2 = 3;

    if (alphaOnly)
        i1 = i2 = (QSysInfo::ByteOrder == QSysInfo::BigEndian ? 0 : 3);

    for (int col = c1; col <= c2; col++)
    {
        p = result.scanLine(r1) + col * 4;
        for (int i = i1; i <= i2; i++)
            rgba[i] = p[i] << 4;

        p += bpl;
        for (int j = r1; j < r2; j++, p += bpl)
            for (int i = i1; i <= i2; i++)
                p[i] = (rgba[i] += ((p[i] << 4) - rgba[i]) * alpha / 16) >> 4;
    }
    for (int row = r1; row <= r2; row++)
    {
        p = result.scanLine(row) + c1 * 4;
        for (int i = i1; i <= i2; i++)
            rgba[i] = p[i] << 4;

        p += 4;
        for (int j = c1; j < c2; j++, p += 4)
            for (int i = i1; i <= i2; i++)
                p[i] = (rgba[i] += ((p[i] << 4) - rgba[i]) * alpha / 16) >> 4;
    }
    for (int col = c1; col <= c2; col++)
    {
        p = result.scanLine(r2) + col * 4;
        for (int i = i1; i <= i2; i++)
            rgba[i] = p[i] << 4;

        p -= bpl;
        for (int j = r1; j < r2; j++, p -= bpl)
            for (int i = i1; i <= i2; i++)
                p[i] = (rgba[i] += ((p[i] << 4) - rgba[i]) * alpha / 16) >> 4;
    }
    for (int row = r1; row <= r2; row++)
    {
        p = result.scanLine(row) + c2 * 4;
        for (int i = i1; i <= i2; i++)
            rgba[i] = p[i] << 4;

        p -= 4;
        for (int j = c1; j < c2; j++, p -= 4)
            for (int i = i1; i <= i2; i++)
                p[i] = (rgba[i] += ((p[i] << 4) - rgba[i]) * alpha / 16) >> 4;
    }
    return result;
}

void Ui::OGLRenderWidget::genAvatar(const std::string& _peerId, Frame* _peer)
{
    const auto contact = QString::fromStdString(_peerId);
    const bool isSelf = Ui::MyInfo()->aimId() == contact;
    QString friendContact = isSelf ? Ui::MyInfo()->friendly() : Logic::GetFriendlyContainer()->getFriendly(contact);
    if (friendContact.isEmpty())
        friendContact = contact;

    bool defaultAvatar = false;
    QPixmap realAvatar;
    if (isVCS_ && !isSelf)
    {
        Utils::loadPixmap(qsl(":/voip/videoconference"), realAvatar);
        realAvatar = realAvatar.scaledToWidth(kLargeAvatarSize, Qt::SmoothTransformation);
    }
    else
    {
        realAvatar = Logic::GetAvatarStorage()->Get(contact, friendContact, kLargeAvatarSize, defaultAvatar, false);
    }

    const auto rawAvatar = realAvatar.toImage();

    QImage blurredImage(kAvatarBlurSize, QImage::Format_RGBA8888);
    blurredImage.fill(kBlurColor);

    QPainter blurPainter(&blurredImage);
    blurPainter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);
    blurPainter.setPen(Qt::NoPen);
    blurPainter.setBrush(Qt::NoBrush);
    blurPainter.setOpacity(kBlurOpacity);

    QImage downsampled = rawAvatar.scaled((QSizeF(rawAvatar.size()) * kDownsamplingFactor).toSize());
    QImage blur = blurred(downsampled, downsampled.rect(), kBlurFactor, false);
    blurPainter.drawImage(blurredImage.rect(), std::move(blur.scaled(rawAvatar.size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation)));

    QImage avatarImage(kLargeAvatarSize, kLargeAvatarSize, QImage::Format_RGBA8888);
    avatarImage.fill(Qt::transparent);

    const QBrush brush(rawAvatar);
    QPainter avatarPainter(&avatarImage);
    avatarPainter.setRenderHint(QPainter::Antialiasing);
    avatarPainter.setPen(Qt::NoPen);
    avatarPainter.setOpacity(0.01);
    avatarPainter.setBrush(brush);
    avatarPainter.drawEllipse(avatarImage.rect());
    avatarPainter.setOpacity(1.0);
    avatarPainter.setBrush(brush);
    avatarPainter.drawEllipse(rawAvatar.rect().adjusted(2, 2, -2, -2));

    _peer->avatarImage_ = std::make_unique<QOpenGLTexture>(avatarImage);
    _peer->avatarImage_->setMinMagFilters(QOpenGLTexture::LinearMipMapLinear, QOpenGLTexture::Linear);

    _peer->avatarBlur_ = std::make_unique<QOpenGLTexture>(blurredImage);
    _peer->avatarBlur_->setMinMagFilters(QOpenGLTexture::LinearMipMapLinear, QOpenGLTexture::Linear);
    _peer->friendlyName_ = friendContact;
}

void Ui::OGLRenderWidget::updatePeers(const voip_manager::ContactsList& _peerList)
{
    peerList_ = _peerList;
    if (!initialized_)
        return;

    if (std::find_if(_peerList.contacts.cbegin(), _peerList.contacts.cend(), [this](const auto& _p) { return _p.contact == activeItemId_; }) == _peerList.contacts.cend())
    {
        if constexpr (platform::is_apple())
            QTimer::singleShot(kMacOSTooltipHideDelay, this, []{ Tooltip::hide(); });
        else
            Tooltip::hide();
    }

    makeCurrent();
    for (auto& p : videoPeers_)
    {
        Frame& peer = p.second;
        peer.deleteMark_ = true;
    }

    for (auto& p : _peerList.contacts)
    {
        auto it = videoPeers_.find(p.contact);
        if (it != videoPeers_.end())
            it->second.deleteMark_ = false;
        Frame& peer = videoPeers_[p.contact];
        peer.peerId_ = p.contact;
        peer.micMute_ = !p.remote_mic_enabled && p.connected_state;
        peer.videoEnable_ = p.remote_cam_enabled;
        peer.connectedState_ = p.connected_state;
        if (!peer.avatarImage_)
            genAvatar(p.contact, &peer);
    }

    if ((!isVCS_ && !isMiniWindow_ && peerList_.contacts.size() == 1))
    {
        if (!oneIsBig_)
        {
            displayPreview_ = false;
            previewInlayout_ = true;
        }
        else
        {
            displayPreview_ = true;
            previewInlayout_ = false;
        }
    }
    else
    {
        previewInlayout_ = (1 != _peerList.contacts.size()) || isIncoming_;
    }

    if (1 == _peerList.contacts.size())
        bigPeerName_ = _peerList.contacts[0].contact;
    else
        previewRenderName_ = PREVIEW_RENDER_NAME;

    if (!isVCS_)
    {
        Frame& peer = isIncoming_ ? videoPeers_[Ui::MyInfo()->aimId().toStdString()] : videoPeers_[PREVIEW_RENDER_NAME];
        peer.peerId_ = PREVIEW_RENDER_NAME;
        peer.deleteMark_ = false;
        peer.micMute_ = !localAudio_;
        peer.videoEnable_ = localVideo_;
        if (_peerList.contacts.empty())
        {
            peer.voice_ = 0;
            peer.iStrength_ = 0.f;
        }
        if (!peer.avatarImage_)
            genAvatar(Ui::MyInfo()->aimId().toStdString(), &peer);
    }
#if ENABLE_VIRTUAL_PEERS
    if (!isVCS_)
    {
        for (int i = 0; i < 20; i++)
        {
            std::string p = su::concat("peer", std::to_string(i));
            Frame& peer = videoPeers_[p];
            peer.peerId_ = p;
            peer.deleteMark_ = false;
            if (!peer.avatarImage_)
                genAvatar(p, &peer);
        }
        previewInlayout_ = true;
    }
#endif
    for (auto it = videoPeers_.begin(); it != videoPeers_.end();)
    {
        if (it->second.deleteMark_)
            it = videoPeers_.erase(it);
        else
            ++it;
    }
    doneCurrent();
    update();
}

std::string_view Ui::OGLRenderWidget::peerAt(const QPoint& _pos) const
{
    for (const auto& p : items_)
    {
        if (p.peer && !p.peer->peerId_.empty())
        {
            lay_vec4 r = lay_get_rect(&ctx_, p.id);
            const QRect rc{ r[0], r[1], r[2], r[3] };
            if (rc.contains(_pos.x(), _pos.y()))
                return p.peer->peerId_;
        }
    }
    return std::string_view{};
}

void Ui::OGLRenderWidget::avatarChanged(const QString& _aimId)
{
    bool needToUpdate = false;
    std::string key = _aimId.toStdString();

    if (videoPeers_.find(key) != videoPeers_.end())
        needToUpdate = true;

    if (Ui::MyInfo()->aimId() == _aimId)
    {
        needToUpdate = true;
        key = PREVIEW_RENDER_NAME;
    }

    if (!initialized_ || !needToUpdate)
        return;
    makeCurrent();
    Frame& peer = videoPeers_[key];
    peer.avatarImage_.reset(nullptr);
    peer.avatarBlur_.reset(nullptr);
    genAvatar(_aimId.toStdString(), &peer);
    doneCurrent();
    update();
}

void Ui::OGLRenderWidget::localMediaChanged(bool _audio, bool _video)
{
    const bool videoChange = localVideo_ != _video;
    localAudio_ = _audio;
    localVideo_ = _video;
    if (!initialized_ || (videoPeers_.find(PREVIEW_RENDER_NAME) == videoPeers_.end()))
        return;
    makeCurrent();
    Frame& peer = videoPeers_[PREVIEW_RENDER_NAME];
    peer.micMute_ = !localAudio_;
    peer.videoEnable_ = localVideo_;
    if (videoChange)
    {
        peer.width_ = 0;
        peer.height_ = 0;
    }
    doneCurrent();
    update();
}

void Ui::OGLRenderWidget::setScreenSharing(bool _on)
{
    isScreenSharing_ = _on;
}

void Ui::OGLRenderWidget::onVoipVadInfo(const std::vector<im::VADInfo> &_peers)
{
    if (isIncoming_)
        return;
    const std::string self = Ui::MyInfo()->aimId().toStdString();
    for (auto& p : videoPeers_)
    {
        Frame& peer = p.second;
        peer.voice_ = 0;
        peer.iStrength_ = 0.f;
    }
    Frame *maxVoicePeer = nullptr;
    for (auto& p : _peers)
    {
        Frame *peer = nullptr;
        if (videoPeers_.find(p.peerId) != videoPeers_.end())
            peer = &videoPeers_[p.peerId];
        else if (self == p.peerId && videoPeers_.find(PREVIEW_RENDER_NAME) != videoPeers_.end())
            peer = &videoPeers_[PREVIEW_RENDER_NAME];
        if (peer && p.level > 0)
        {
            peer->voice_ = p.level;
            peer->iStrength_ = 1.f;
            if (!maxVoicePeer || peer->voice_ > maxVoicePeer->voice_)
                maxVoicePeer = peer;
        }
    }
    if (maxVoicePeer)
    {
        maxVoiceName_ = maxVoicePeer->peerId_;
        if (isMiniWindow_)
            bigPeerName_ = maxVoicePeer->peerId_;
    }
    update();
}
