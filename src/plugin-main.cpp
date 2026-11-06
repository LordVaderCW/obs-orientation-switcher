
#pragma warning(push)
#pragma warning(disable : 4201)

#include <obs-module.h>
#include <obs-frontend-api.h>
#include <util/config-file.h>
#include <util/platform.h>

#pragma warning(pop)

#include <QElapsedTimer>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QPushButton>
#include <QSizePolicy>
#include <QString>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

#include <cstdint>
#include <limits>

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <Windows.h>

#include "../resources/resource.h"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("obs-orientation-switcher", "en-US")

namespace {

    // ==================================================
    // Plugin metadata
    // ==================================================

    constexpr const char* kPluginName =
        "OBS Orientation Switcher";

    constexpr const char* kPluginVersion =
        "1.0.2";

    constexpr const char* kDeveloperName =
        "LordVaderCW";

    constexpr const char* kDeveloperUrl =
        "https://github.com/LordVaderCW";

    // ==================================================
    // Video configuration
    // ==================================================

    constexpr uint32_t kLandscapeWidth = 1920;
    constexpr uint32_t kLandscapeHeight = 1080;

    constexpr uint32_t kPortraitWidth = 1080;
    constexpr uint32_t kPortraitHeight = 1920;

    constexpr const char* kDockId =
        "obs_orientation_switcher_dock";

    constexpr int kRefreshIntervalMs = 750;

    // Repeat requests are ignored for 500 ms.
    // The button remains enabled during this interval.

    constexpr qint64 kOrientationCooldownMs = 500;

    constexpr int kAvatarSize = 16;
    constexpr int kAvatarScaleFactor = 2;

    // ==================================================
    // Qt widget references
    // ==================================================

    QWidget* g_dockWidget = nullptr;
    QPushButton* g_toggleButton = nullptr;
    QLabel* g_statusLabel = nullptr;

    // ==================================================
    // Orientation switching state
    // ==================================================

    bool g_orientationSwitchInProgress = false;
    QElapsedTimer g_orientationSwitchTimer;

    // ==================================================
    // Orientation types
    // ==================================================

    enum class Orientation {
        Landscape,
        Portrait,
        Custom
    };

    struct Resolution {
        uint32_t baseWidth = 0;
        uint32_t baseHeight = 0;
        uint32_t outputWidth = 0;
        uint32_t outputHeight = 0;
    };

    // ==================================================
    // OBS output state
    // ==================================================

    bool outputs_active()
    {
        return obs_frontend_streaming_active() ||
            obs_frontend_recording_active() ||
            obs_frontend_replay_buffer_active() ||
            obs_frontend_virtualcam_active();
    }

    // ==================================================
    // Orientation switch guard
    // ==================================================

    bool orientation_switch_locked()
    {
        if (g_orientationSwitchInProgress) {
            return true;
        }

        return g_orientationSwitchTimer.isValid() &&
            g_orientationSwitchTimer.elapsed() <
            kOrientationCooldownMs;
    }

    // ==================================================
    // Read OBS video configuration
    // ==================================================

    bool read_resolution(Resolution& resolution)
    {
        config_t* profile =
            obs_frontend_get_profile_config();

        if (profile == nullptr) {
            return false;
        }

        resolution.baseWidth = static_cast<uint32_t>(
            config_get_uint(profile, "Video", "BaseCX"));

        resolution.baseHeight = static_cast<uint32_t>(
            config_get_uint(profile, "Video", "BaseCY"));

        resolution.outputWidth = static_cast<uint32_t>(
            config_get_uint(profile, "Video", "OutputCX"));

        resolution.outputHeight = static_cast<uint32_t>(
            config_get_uint(profile, "Video", "OutputCY"));

        return true;
    }

    // ==================================================
    // Determine current orientation
    // ==================================================

    Orientation get_orientation()
    {
        Resolution resolution;

        if (!read_resolution(resolution)) {
            return Orientation::Custom;
        }

        if (resolution.baseWidth == kLandscapeWidth &&
            resolution.baseHeight == kLandscapeHeight &&
            resolution.outputWidth == kLandscapeWidth &&
            resolution.outputHeight == kLandscapeHeight) {

            return Orientation::Landscape;
        }

        if (resolution.baseWidth == kPortraitWidth &&
            resolution.baseHeight == kPortraitHeight &&
            resolution.outputWidth == kPortraitWidth &&
            resolution.outputHeight == kPortraitHeight) {

            return Orientation::Portrait;
        }

        return Orientation::Custom;
    }

    // ==================================================
    // Update orientation dock
    // ==================================================

    void update_dock()
    {
        if (g_toggleButton == nullptr ||
            g_statusLabel == nullptr) {
            return;
        }

        const Orientation orientation =
            get_orientation();

        const bool active =
            outputs_active();

        // Only an active OBS output disables the button.
        // The 500 ms cooldown does not affect its appearance.

        g_toggleButton->setEnabled(!active);

        switch (orientation) {

        case Orientation::Landscape:

            g_toggleButton->setText(
                QStringLiteral("Switch to Portrait"));

            g_statusLabel->setText(
                QStringLiteral(
                    "Landscape  |  1920 x 1080"));

            break;

        case Orientation::Portrait:

            g_toggleButton->setText(
                QStringLiteral("Switch to Landscape"));

            g_statusLabel->setText(
                QStringLiteral(
                    "Portrait  |  1080 x 1920"));

            break;

        case Orientation::Custom:
        default:

            g_toggleButton->setText(
                QStringLiteral("Switch to Landscape"));

            g_statusLabel->setText(
                QStringLiteral("Custom Resolution"));

            break;
        }

        if (active) {

            g_statusLabel->setText(
                QStringLiteral(
                    "Orientation locked - output active"));

            g_toggleButton->setToolTip(
                QStringLiteral(
                    "Stop streaming, recording, replay buffer "
                    "and virtual camera before switching."));
        }
        else {

            g_toggleButton->setToolTip(
                QStringLiteral(
                    "Switch OBS canvas and output orientation."));
        }
    }

    // ==================================================
    // Set OBS video resolution
    // ==================================================

    bool set_resolution(uint32_t width, uint32_t height)
    {
        // ----------------------------------------------
        // Ignore requests during an active output
        // ----------------------------------------------

        if (outputs_active()) {

            blog(
                LOG_WARNING,
                "[Orientation Switcher] Cannot change resolution "
                "while an OBS output is active.");

            update_dock();
            return false;
        }

        // ----------------------------------------------
        // Silently ignore repeated requests
        // ----------------------------------------------

        if (orientation_switch_locked()) {
            return false;
        }

        config_t* profile =
            obs_frontend_get_profile_config();

        if (profile == nullptr) {

            blog(
                LOG_ERROR,
                "[Orientation Switcher] Failed to obtain "
                "the active profile configuration.");

            update_dock();
            return false;
        }

        Resolution current;

        if (!read_resolution(current)) {

            blog(
                LOG_ERROR,
                "[Orientation Switcher] Failed to read "
                "the current resolution.");

            update_dock();
            return false;
        }

        // ----------------------------------------------
        // Avoid unnecessary video resets
        // ----------------------------------------------

        if (current.baseWidth == width &&
            current.baseHeight == height &&
            current.outputWidth == width &&
            current.outputHeight == height) {

            update_dock();
            return true;
        }

        // ----------------------------------------------
        // Begin protected orientation switch
        // ----------------------------------------------

        g_orientationSwitchInProgress = true;

        // Monotonic timer, unaffected by Windows clock
        // changes. Starts when the change is accepted.

        g_orientationSwitchTimer.restart();

        update_dock();

        blog(
            LOG_INFO,
            "[Orientation Switcher] Beginning resolution "
            "change to %ux%u.",
            width,
            height);

        // ----------------------------------------------
        // Update OBS profile configuration
        // ----------------------------------------------

        config_set_uint(
            profile,
            "Video",
            "BaseCX",
            width);

        config_set_uint(
            profile,
            "Video",
            "BaseCY",
            height);

        config_set_uint(
            profile,
            "Video",
            "OutputCX",
            width);

        config_set_uint(
            profile,
            "Video",
            "OutputCY",
            height);

        // ----------------------------------------------
        // Apply video configuration
        // ----------------------------------------------

        obs_frontend_reset_video();

        // ----------------------------------------------
        // Save updated OBS profile
        // ----------------------------------------------

        obs_frontend_save();

        blog(
            LOG_INFO,
            "[Orientation Switcher] Resolution reset "
            "requested: %ux%u.",
            width,
            height);

        // ----------------------------------------------
        // End active reset guard
        // Timer remains active for the 500 ms interval
        // ----------------------------------------------

        g_orientationSwitchInProgress = false;

        update_dock();

        return true;
    }

    // ==================================================
    // Tools menu callbacks
    // ==================================================

    void set_landscape(void*)
    {
        set_resolution(
            kLandscapeWidth,
            kLandscapeHeight);
    }

    void set_portrait(void*)
    {
        set_resolution(
            kPortraitWidth,
            kPortraitHeight);
    }

    // ==================================================
    // Dock orientation toggle
    // ==================================================

    void toggle_orientation()
    {
        if (orientation_switch_locked()) {
            return;
        }

        if (outputs_active()) {
            update_dock();
            return;
        }

        const Orientation orientation =
            get_orientation();

        if (orientation == Orientation::Landscape) {

            set_resolution(
                kPortraitWidth,
                kPortraitHeight);
        }
        else {

            set_resolution(
                kLandscapeWidth,
                kLandscapeHeight);
        }
    }

    // ==================================================
    // Load developer avatar from embedded DLL resource
    // ==================================================

    QPixmap load_developer_avatar()
    {
        HMODULE module = nullptr;

        // Obtain the DLL containing this function,
        // rather than the OBS executable.

        const BOOL moduleFound =
            GetModuleHandleExW(
                GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                reinterpret_cast<LPCWSTR>(
                    &load_developer_avatar),
                &module);

        if (!moduleFound || module == nullptr) {

            blog(
                LOG_WARNING,
                "[Orientation Switcher] Failed to locate "
                "the plugin DLL module handle.");

            return QPixmap();
        }

        // ----------------------------------------------
        // Locate embedded PNG resource
        // ----------------------------------------------

        const HRSRC resource =
            FindResourceW(
                module,
                MAKEINTRESOURCEW(IDR_DEVELOPER_AVATAR),
                RT_RCDATA);

        if (resource == nullptr) {

            blog(
                LOG_WARNING,
                "[Orientation Switcher] Embedded developer "
                "avatar resource was not found.");

            return QPixmap();
        }

        const DWORD resourceSize =
            SizeofResource(
                module,
                resource);

        if (resourceSize == 0 ||
            resourceSize > static_cast<DWORD>(
                (std::numeric_limits<int>::max)())) {

            blog(
                LOG_WARNING,
                "[Orientation Switcher] Embedded avatar "
                "resource size is invalid.");

            return QPixmap();
        }

        const HGLOBAL resourceData =
            LoadResource(
                module,
                resource);

        if (resourceData == nullptr) {

            blog(
                LOG_WARNING,
                "[Orientation Switcher] Failed to load "
                "the embedded avatar resource.");

            return QPixmap();
        }

        const void* resourceBytes =
            LockResource(resourceData);

        if (resourceBytes == nullptr) {

            blog(
                LOG_WARNING,
                "[Orientation Switcher] Failed to access "
                "the embedded avatar resource data.");

            return QPixmap();
        }

        // ----------------------------------------------
        // Decode embedded PNG
        // ----------------------------------------------

        QPixmap source;

        const bool loaded =
            source.loadFromData(
                static_cast<const uchar*>(resourceBytes),
                static_cast<uint>(resourceSize),
                "PNG");

        if (!loaded || source.isNull()) {

            blog(
                LOG_WARNING,
                "[Orientation Switcher] Failed to decode "
                "the embedded developer avatar PNG.");

            return QPixmap();
        }

        // ----------------------------------------------
        // Centre-crop image to square
        // ----------------------------------------------

        const int sourceSize =
            source.width() < source.height()
            ? source.width()
            : source.height();

        if (sourceSize <= 0) {
            return QPixmap();
        }

        const int sourceX =
            (source.width() - sourceSize) / 2;

        const int sourceY =
            (source.height() - sourceSize) / 2;

        const QPixmap square =
            source.copy(
                sourceX,
                sourceY,
                sourceSize,
                sourceSize);

        // ----------------------------------------------
        // Render circular avatar at 2x resolution
        // ----------------------------------------------

        const int physicalSize =
            kAvatarSize * kAvatarScaleFactor;

        QPixmap circular(
            physicalSize,
            physicalSize);

        circular.fill(Qt::transparent);

        QPainter painter(&circular);

        painter.setRenderHint(
            QPainter::Antialiasing,
            true);

        painter.setRenderHint(
            QPainter::SmoothPixmapTransform,
            true);

        QPainterPath clippingPath;

        clippingPath.addEllipse(
            0.0,
            0.0,
            static_cast<qreal>(physicalSize),
            static_cast<qreal>(physicalSize));

        painter.setClipPath(
            clippingPath);

        painter.drawPixmap(
            0,
            0,
            physicalSize,
            physicalSize,
            square);

        painter.end();

        circular.setDevicePixelRatio(
            static_cast<qreal>(kAvatarScaleFactor));

        return circular;
    }

    // ==================================================
    // About dialog
    // ==================================================

    void show_about_dialog()
    {
        QMessageBox dialog(g_dockWidget);

        dialog.setWindowTitle(
            QStringLiteral(
                "About OBS Orientation Switcher"));

        dialog.setIcon(
            QMessageBox::Information);

        const QString pluginTitle =
            QStringLiteral("<b>%1</b><br>Version %2")
            .arg(
                QString::fromUtf8(kPluginName),
                QString::fromUtf8(kPluginVersion));

        dialog.setText(pluginTitle);

        const QString developerInformation =
            QStringLiteral(
                "A native OBS Studio plugin for switching "
                "between landscape and portrait video "
                "orientations."
                "<br><br>"
                "<b>Developer:</b> %1"
                "<br>"
                "<b>GitHub:</b> "
                "<a href=\"%2\">%2</a>")
            .arg(
                QString::fromUtf8(kDeveloperName),
                QString::fromUtf8(kDeveloperUrl));

        dialog.setInformativeText(
            developerInformation);

        dialog.setTextInteractionFlags(
            Qt::TextBrowserInteraction);

        dialog.exec();
    }

    // ==================================================
    // Create developer footer
    // ==================================================

    void create_developer_footer(
        QWidget* widget,
        QVBoxLayout* mainLayout)
    {
        QHBoxLayout* footerLayout =
            new QHBoxLayout();

        footerLayout->setContentsMargins(
            0, 2, 0, 0);

        footerLayout->setSpacing(5);

        // ----------------------------------------------
        // Circular avatar label
        // ----------------------------------------------

        QLabel* avatarLabel =
            new QLabel(widget);

        avatarLabel->setObjectName(
            QStringLiteral(
                "OBSOrientationDeveloperAvatar"));

        avatarLabel->setFixedSize(
            kAvatarSize,
            kAvatarSize);

        avatarLabel->setAlignment(
            Qt::AlignCenter);

        avatarLabel->setStyleSheet(
            QStringLiteral(
                "QLabel {"
                "background: transparent;"
                "border: none;"
                "padding: 0px;"
                "margin: 0px;"
                "}"));

        const QPixmap avatar =
            load_developer_avatar();

        if (!avatar.isNull()) {
            avatarLabel->setPixmap(avatar);
        }

        footerLayout->addWidget(
            avatarLabel,
            0,
            Qt::AlignVCenter);

        // ----------------------------------------------
        // Developer name - non-clickable label
        // ----------------------------------------------

        QLabel* developerLabel =
            new QLabel(
                QString::fromUtf8(kDeveloperName),
                widget);

        developerLabel->setObjectName(
            QStringLiteral(
                "OBSOrientationDeveloperLabel"));

        developerLabel->setAlignment(
            Qt::AlignVCenter | Qt::AlignLeft);

        developerLabel->setToolTip(
            QStringLiteral(
                "Developed by LordVaderCW"));

        developerLabel->setStyleSheet(
            QStringLiteral(
                "QLabel {"
                "background: transparent;"
                "border: none;"
                "padding: 0px;"
                "margin: 0px;"
                "color: palette(window-text);"
                "}"));

        footerLayout->addWidget(
            developerLabel,
            0,
            Qt::AlignVCenter);

        footerLayout->addStretch();

        // ----------------------------------------------
        // About button
        // ----------------------------------------------

        QPushButton* aboutButton =
            new QPushButton(
                QStringLiteral("About"),
                widget);

        aboutButton->setObjectName(
            QStringLiteral(
                "OBSOrientationAboutButton"));

        aboutButton->setFlat(true);

        aboutButton->setCursor(
            Qt::PointingHandCursor);

        aboutButton->setToolTip(
            QStringLiteral(
                "About OBS Orientation Switcher"));

        QObject::connect(
            aboutButton,
            &QPushButton::clicked,
            widget,
            []() {
                show_about_dialog();
            });

        footerLayout->addWidget(
            aboutButton);

        mainLayout->addLayout(
            footerLayout);
    }

    // ==================================================
    // Create OBS dock
    // ==================================================

    bool create_dock()
    {
        if (g_dockWidget != nullptr) {
            return true;
        }

        QWidget* widget =
            new QWidget();

        widget->setObjectName(
            QStringLiteral(
                "OBSOrientationSwitcherWidget"));

        widget->setMinimumWidth(180);

        QVBoxLayout* layout =
            new QVBoxLayout(widget);

        layout->setContentsMargins(
            8, 8, 8, 8);

        layout->setSpacing(6);

        // ----------------------------------------------
        // Orientation toggle button
        // ----------------------------------------------

        QPushButton* button =
            new QPushButton(widget);

        button->setObjectName(
            QStringLiteral(
                "OBSOrientationToggleButton"));

        button->setMinimumHeight(32);

        button->setSizePolicy(
            QSizePolicy::Expanding,
            QSizePolicy::Preferred);

        // ----------------------------------------------
        // Orientation status label
        // ----------------------------------------------

        QLabel* status =
            new QLabel(widget);

        status->setObjectName(
            QStringLiteral(
                "OBSOrientationStatusLabel"));

        status->setAlignment(
            Qt::AlignCenter);

        layout->addWidget(button);
        layout->addWidget(status);

        // ----------------------------------------------
        // Developer signature and About button
        // ----------------------------------------------

        create_developer_footer(
            widget,
            layout);

        g_dockWidget = widget;
        g_toggleButton = button;
        g_statusLabel = status;

        // ----------------------------------------------
        // Orientation button callback
        // ----------------------------------------------

        QObject::connect(
            button,
            &QPushButton::clicked,
            widget,
            []() {
                toggle_orientation();
            });

        // ----------------------------------------------
        // Periodic orientation state refresh
        // ----------------------------------------------

        QTimer* refreshTimer =
            new QTimer(widget);

        refreshTimer->setInterval(
            kRefreshIntervalMs);

        QObject::connect(
            refreshTimer,
            &QTimer::timeout,
            widget,
            []() {
                update_dock();
            });

        update_dock();

        // ----------------------------------------------
        // Register dock with OBS
        // ----------------------------------------------

        const bool registered =
            obs_frontend_add_dock_by_id(
                kDockId,
                "Orientation Switcher",
                widget);

        if (!registered) {

            blog(
                LOG_ERROR,
                "[Orientation Switcher] Failed to register dock.");

            g_dockWidget = nullptr;
            g_toggleButton = nullptr;
            g_statusLabel = nullptr;

            delete widget;

            return false;
        }

        refreshTimer->start();

        blog(
            LOG_INFO,
            "[Orientation Switcher] Dock registered successfully.");

        return true;
    }

} // namespace

// ==================================================
// OBS module description
// ==================================================

extern "C" MODULE_EXPORT const char*
obs_module_description(void)
{
    return
        "OBS Orientation Switcher by LordVaderCW. "
        "Switch between landscape and portrait "
        "video resolutions using a dockable Qt panel. "
        "https://github.com/LordVaderCW";
}

// ==================================================
// OBS module load
// ==================================================

extern "C" MODULE_EXPORT bool obs_module_load(void)
{
    g_orientationSwitchInProgress = false;
    g_orientationSwitchTimer.invalidate();

    blog(
        LOG_INFO,
        "[Orientation Switcher] Loading %s version %s "
        "by %s.",
        kPluginName,
        kPluginVersion,
        kDeveloperName);

    obs_frontend_add_tools_menu_item(
        "Orientation: Landscape (1920 x 1080)",
        set_landscape,
        nullptr);

    obs_frontend_add_tools_menu_item(
        "Orientation: Portrait (1080 x 1920)",
        set_portrait,
        nullptr);

    if (!create_dock()) {

        blog(
            LOG_WARNING,
            "[Orientation Switcher] Dock unavailable; "
            "Tools menu controls remain registered.");
    }

    blog(
        LOG_INFO,
        "[Orientation Switcher] Plugin loaded.");

    return true;
}

// ==================================================
// OBS module unload
// ==================================================

extern "C" MODULE_EXPORT void obs_module_unload(void)
{
    if (g_dockWidget != nullptr) {

        obs_frontend_remove_dock(kDockId);

        g_dockWidget = nullptr;
        g_toggleButton = nullptr;
        g_statusLabel = nullptr;
    }

    g_orientationSwitchInProgress = false;
    g_orientationSwitchTimer.invalidate();

    blog(
        LOG_INFO,
        "[Orientation Switcher] Plugin unloaded.");
}
