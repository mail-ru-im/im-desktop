#pragma once
#include <QtWidgets/QOpenGLWidget>
#include <QtGui/QOpenGLFunctions>
#include <QtGui/QOpenGLBuffer>
#include "VoipController.h"
#include "IRender.h"
#include "layout.h"

QT_FORWARD_DECLARE_CLASS(QOpenGLShaderProgram);
QT_FORWARD_DECLARE_CLASS(QOpenGLTexture);
QT_FORWARD_DECLARE_CLASS(QMenu)

namespace Ui
{

    class OGLRenderWidget : public QOpenGLWidget, public Ui::IRender
    {
        Q_OBJECT
    public:
        OGLRenderWidget(QWidget* _parent);
        virtual ~OGLRenderWidget();

        QWidget* getWidget() override { return this; };
        void setFrame(const im::IVideoFrame* _frame, const std::string& _peerId) override;
        void clear() override;
        void setRenderLayout(bool _oneIsBig) override;
        void toggleRenderLayout(const QPoint& _clickPos) override;
        void setViewMode(ViewMode _mode) override;
        ViewMode getViewMode() const override;
        bool getRenderLayout() override { return oneIsBig_; };
        void setBigPeer(const std::string& _bigPeerName) override;
        void setVCS(bool _isVCS) override;
        void setPinnedRoom(bool _isPinnedRoom) override;
        void setIncoming(bool _isIncoming) override;
        void setMiniWindow(bool _isMiniWindow) override;
        void updatePeers(const voip_manager::ContactsList& _peerList) override;
        void localMediaChanged(bool _audio, bool _video) override;
        void setScreenSharing(bool _on) override;

    protected:
        void initializeGL() override;
        void paintGL() override;
        void mousePressEvent(QMouseEvent* _e) override;
        void mouseMoveEvent(QMouseEvent* _e) override;
        bool event(QEvent* _event) override;
        QMenu* createContextMenu(const QPoint& p) const override;

    private:
        enum class ImageFitPolicy
        {
            FitInside,
            FitOutside,
            FitIgnore
        };

        struct ImageOption
        {
            QRect pos_;
            float iRadius_ = 0.0f;
            float iBorder_ = 0.0f;
            float iStrength_ = 0.0f;
            float iScale_ = 1.0f;
            ImageFitPolicy fitPolicy_ = ImageFitPolicy::FitOutside;

            ImageOption() = default;

            ImageOption(const QRect& _pos,
                        float _radius = 0.0f,
                        float _border = 0.0f,
                        float _strength = 0.0f,
                        float _scale = 1.0f,
                        ImageFitPolicy _policy = ImageFitPolicy::FitOutside)
            {
                reset(_pos, _radius, _border, _strength, _scale, _policy);
            }

            void reset(const QRect& _pos,
                float _radius = 0.0f,
                float _border = 0.0f,
                float _strength = 0.0f,
                float _scale = 1.0f,
                ImageFitPolicy _policy = ImageFitPolicy::FitOutside)
            {
                pos_ = _pos;
                iRadius_ = std::max(_radius, 0.0f);
                iBorder_ = std::max(_border, 0.0f);
                iStrength_ = std::max(_strength, 0.0f);
                iScale_ = _scale == 0.0 ? 1.0 : _scale;
                fitPolicy_ = _policy;
            }
        };

        struct Frame
        {
            Frame();
            std::unique_ptr<QOpenGLTexture> texY_, texU_, texV_;
            std::unique_ptr<QOpenGLTexture> avatarBlur_;
            std::unique_ptr<QOpenGLTexture> avatarImage_;
            std::string peerId_;
            QString friendlyName_;
            int width_ = 0;
            int height_ = 0;
            int renderWidth_ = 0;
            int renderHeight_ = 0;
            int angle_ = 0;
            int voice_ = 0;
            float iStrength_ = 0.f;
            bool roundEdges_ = false;
            bool doMirroring_ = false;
            bool deleteMark_ = false;
            bool micMute_ = false;
            bool videoEnable_ = false;
            bool connectedState_ = true;
        };
        struct item
        {
            lay_id id = 0;
            Frame* peer = nullptr;
            QOpenGLTexture* tex = nullptr;
            std::string text;
            bool fitInside_ = false;
        };
        std::vector<item> items_;

        std::string_view peerAt(const QPoint& _pos) const;
        void showToolTip(const QRect& _area, const QRect& _frameRect, const QString& _text);

        void avatarChanged(const QString& _aimId);
        void doGridLayout();
        void doOneIsBigLayout();
        void drawParticipant(const Frame& _peer, const QRect& _pos, bool _fitInside, float _scale = 1.0);
        void drawImage(QOpenGLTexture* _tex, const ImageOption& _options);
        void drawCaption(const Frame& _peer, const QRect& _pos);
        QRect previewRect() const;
        QRect tooltipRect(const QRect& _frameRect, const Frame& _peer) const;
        QRect captionArea(const QRect& _frameRect) const;
        QRect renderTextSize(const QString& _text) const;
        void renderText(int _x, int _y, const QString& _text, bool _showBackground = true, QMargins _margins = QMargins(0, 0, 0, 0));
        void genAvatar(const std::string& _peerId, Frame* _peer);
        void onVoipVadInfo(const std::vector<im::VADInfo> &_peers);

        QWidget* parent_;
        QTimer* tooltipTimer_;
        lay_context ctx_;
        QFont font_;
        QTimer animationTimer_;
        std::string_view activeItemId_;
        std::unique_ptr<QOpenGLShaderProgram> prg_;
        std::unique_ptr<QOpenGLShaderProgram> prgRGB_;
        std::map<std::string, Frame> videoPeers_;
        std::map<std::string, QSize> videoPeerSizes_;
        std::unique_ptr<QOpenGLTexture> arrowLeft_;
        std::unique_ptr<QOpenGLTexture> arrowRight_;
        std::unique_ptr<QOpenGLTexture> arrowLeftH_;
        std::unique_ptr<QOpenGLTexture> arrowRightH_;
        std::unique_ptr<QOpenGLTexture> muteIcon_;
        std::unique_ptr<QOpenGLTexture> bgColor_;
        std::unique_ptr<QOpenGLTexture> icCalling_[3];
        voip_manager::ContactsList peerList_;
        float scaleX_ = 1.0f;
        float scaleY_ = 1.0f;
        std::string bigPeerName_;
        std::string maxVoiceName_;
        std::string previewRenderName_;
        GLuint vertexBuffer_ = 0;
        GLuint texBuffer_ = 0;
        int startPosition_ = 0;
        int numPeers_ = 0;
        int maxPeers_ = 0; // for one is big layout, number of videos in top line
        int mouseX_ = 0;
        int mouseY_ = 0;
        int animationFrame_ = 0;
        ViewMode viewMode_ = IRender::ViewMode::GridMode;
        bool initialized_ = false;
        bool currentCursorHand_ = false;
        bool displayPreview_ = false;
        bool previewInlayout_ = false;
        bool oneIsBig_ = false;
        bool isVCS_ = false;
        bool isPinnedRoom_ = false;
        bool isIncoming_ = false;
        bool isMiniWindow_ = false;
        bool localAudio_ = false;
        bool localVideo_ = false;
        bool isScreenSharing_ = false;
        bool updatingFlag_ = false;
    };

} // namespace Ui
