/*
  This file is part of KDDockWidgets.

  SPDX-FileCopyrightText: 2019-2023 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.com>
  Author: Sérgio Martins <sergio.martins@kdab.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only

  Contact KDAB at <info@kdab.com> for commercial licensing options.
*/

#ifndef KD_LAYOUTSAVER_P_H
#define KD_LAYOUTSAVER_P_H

#include "KDDockWidgets_p.h"
#include "kddockwidgets/KDDockWidgets.h"
#include "kddockwidgets/LayoutSaver.h"
#include "kddockwidgets/core/Platform.h"
#include "core/Window.h"

#include <QDebug>
#include <QRect>
#include <QSettings>

#include <memory>

/**
 * Bump whenever the format changes, so we can still load old layouts.
 * version 1: Initial version
 * version 2: Introduced MainWindow::screenSize and FloatingWindow::screenSize
 * version 3: New layouting engine
 */
#define KDDOCKWIDGETS_SERIALIZATION_VERSION 3


namespace KDDockWidgets {

namespace Core {
class FloatingWindow;
class View;
}

class DockRegistry;

/// @brief A more granular version of KDDockWidgets::RestoreOption
/// There's some granularity that we don't want to expose to all users but want to allow some users
/// to use. We might make more options public once they've proven themselves, so for now they are
/// internal
enum class InternalRestoreOption {
    None = 0,
    SkipMainWindowGeometry = 1, ///< Don't reposition the main window's geometry when restoring.
    RelativeFloatingWindowGeometry =
        2 ///< FloatingWindow's are repositioned relatively to the new MainWindow's size
};
Q_DECLARE_FLAGS(InternalRestoreOptions, InternalRestoreOption)


struct LayoutSaver::Placeholder
{
    typedef QVector<LayoutSaver::Placeholder> List;

    QVariantMap toVariantMap() const;
    void fromVariantMap(const QVariantMap &map);

    bool isFloatingWindow;
    int indexOfFloatingWindow;
    int itemIndex;
    QString mainWindowUniqueName;
};

///@brief contains info about how a main window is scaled.
/// Used for RestoreOption_RelativeToMainWindow
struct LayoutSaver::ScalingInfo
{
    ScalingInfo() = default;
    explicit ScalingInfo(const QString &mainWindowId, QRect savedMainWindowGeo, int screenIndex);

    bool isValid() const
    {
        return heightFactor > 0 && widthFactor > 0
            && !((qFuzzyCompare(widthFactor, 1) && qFuzzyCompare(heightFactor, 1)));
    }

    void translatePos(QPoint &) const;
    void applyFactorsTo(QPoint &) const;
    void applyFactorsTo(QSize &) const;
    void applyFactorsTo(QRect &) const;

    QString mainWindowName;
    QRect savedMainWindowGeometry;
    QRect realMainWindowGeometry;
    double heightFactor = -1;
    double widthFactor = -1;
    bool mainWindowChangedScreen = false;
};

struct LayoutSaver::Position
{
    QRect lastFloatingGeometry;
    int tabIndex;
    bool wasFloating;
    LayoutSaver::Placeholder::List placeholders;
    QHash<SideBarLocation, QRect> lastOverlayedGeometries;

    /// Iterates through the layout and patches all absolute sizes. See
    /// RestoreOption_RelativeToMainWindow.
    void scaleSizes(const ScalingInfo &scalingInfo);
};

struct DOCKS_EXPORT LayoutSaver::DockWidget
{
    // Using shared ptr, as we need to modify shared instances
    typedef std::shared_ptr<LayoutSaver::DockWidget> Ptr;
    typedef QVector<Ptr> List;
    static QHash<QString, Ptr> s_dockWidgets;

    bool isValid() const;

    /// Iterates through the layout and patches all absolute sizes. See
    /// RestoreOption_RelativeToMainWindow.
    void scaleSizes(const ScalingInfo &scalingInfo);

    static Ptr dockWidgetForName(const QString &name)
    {
        auto dw = s_dockWidgets.value(name);
        if (dw)
            return dw;

        dw = Ptr(new LayoutSaver::DockWidget);
        s_dockWidgets.insert(name, dw);
        dw->uniqueName = name;

        return dw;
    }

    bool skipsRestore() const;

    QString uniqueName;
    QStringList affinities;
    LayoutSaver::Position lastPosition;

private:
    DockWidget()
    {
    }
};

inline QVariantList dockWidgetNames(const LayoutSaver::DockWidget::List &list)
{
    QVariantList result;
    result.reserve(list.size());
    for (auto &dw : list)
        result.push_back(dw->uniqueName);

    return result;
}

struct LayoutSaver::Group
{
    bool isValid() const;

    bool hasSingleDockWidget() const;
    bool skipsRestore() const;

    /// @brief in case this group only has one group, returns the name of that dock widget
    LayoutSaver::DockWidget::Ptr singleDockWidget() const;

    bool isNull = true;
    QString objectName;
    QRect geometry;
    QFlags<FrameOption>::Int options;
    int currentTabIndex;
    QString id; // for coorelation purposes

    /// Might be empty if not in a main window. Used so we don't create a group when restoring
    /// the persistent central group, that's never deleted when restoring
    QString mainWindowUniqueName;

    LayoutSaver::DockWidget::List dockWidgets;
};

struct LayoutSaver::MultiSplitter
{
    bool isValid() const;

    bool hasSingleDockWidget() const;
    LayoutSaver::DockWidget::Ptr singleDockWidget() const;
    bool skipsRestore() const;

    QVariantMap layout;
    QHash<QString, LayoutSaver::Group> groups;
};

struct LayoutSaver::FloatingWindow
{
    typedef QVector<LayoutSaver::FloatingWindow> List;

    bool isValid() const;

    bool hasSingleDockWidget() const;
    LayoutSaver::DockWidget::Ptr singleDockWidget() const;
    bool skipsRestore() const;

    /// Iterates through the layout and patches all absolute sizes. See
    /// RestoreOption_RelativeToMainWindow.
    void scaleSizes(const ScalingInfo &);

    LayoutSaver::MultiSplitter multiSplitterLayout;
    QStringList affinities;
    int parentIndex = -1;
    QRect geometry;
    QRect normalGeometry;
    int screenIndex;
    int flags = -1;
    QSize screenSize; // for relative-size restoring
    bool isVisible = true;

    // The instance that was created during a restore:
    Core::FloatingWindow *floatingWindowInstance = nullptr;
    KDDockWidgets::WindowState windowState = KDDockWidgets::WindowState::None;
};

struct LayoutSaver::MainWindow
{
public:
    typedef QVector<LayoutSaver::MainWindow> List;

    bool isValid() const;

    /// Iterates through the layout and patches all absolute sizes. See
    /// RestoreOption_RelativeToMainWindow.
    void scaleSizes();

    QHash<SideBarLocation, QStringList> dockWidgetsPerSideBar;
    KDDockWidgets::MainWindowOptions options;
    LayoutSaver::MultiSplitter multiSplitterLayout;
    QString uniqueName;
    QStringList affinities;
    QRect geometry;
    QRect normalGeometry;
    int screenIndex;
    QSize screenSize; // for relative-size restoring
    bool isVisible;
    KDDockWidgets::WindowState windowState = KDDockWidgets::WindowState::None;

    ScalingInfo scalingInfo;
};

///@brief we serialize some info about screens, so eventually we can make restore smarter when
/// switching screens
/// Not used currently, but nice to have in the json already
struct LayoutSaver::ScreenInfo
{
    typedef QVector<LayoutSaver::ScreenInfo> List;

    int index;
    QRect geometry;
    QString name;
    double devicePixelRatio;
};

struct LayoutSaver::Layout
{
public:
    Layout()
    {
        s_currentLayoutBeingRestored = this;

        const auto screens = Core::Platform::instance()->screens();
        const int numScreens = screens.size();
        screenInfo.reserve(numScreens);
        for (int i = 0; i < numScreens; ++i) {
            ScreenInfo info;
            info.index = i;
            info.geometry = screens[i]->geometry();
            info.name = screens[i]->name();
            info.devicePixelRatio = screens[i]->devicePixelRatio();
            screenInfo.push_back(info);
        }
    }

    ~Layout()
    {
        s_currentLayoutBeingRestored = nullptr;
    }

    bool isValid() const;

    QByteArray toJson() const;
    bool fromJson(const QByteArray &jsonData);

    /// Iterates through the layout and patches all absolute sizes. See
    /// RestoreOption_RelativeToMainWindow.
    void scaleSizes(KDDockWidgets::InternalRestoreOptions);

    static LayoutSaver::Layout *s_currentLayoutBeingRestored;

    LayoutSaver::MainWindow mainWindowForIndex(int index) const;
    LayoutSaver::FloatingWindow floatingWindowForIndex(int index) const;

    QStringList mainWindowNames() const;
    QStringList dockWidgetNames() const;
    QStringList dockWidgetsToClose() const;
    bool containsDockWidget(const QString &uniqueName) const;

    int serializationVersion = KDDOCKWIDGETS_SERIALIZATION_VERSION;
    LayoutSaver::MainWindow::List mainWindows;
    LayoutSaver::FloatingWindow::List floatingWindows;
    LayoutSaver::DockWidget::List closedDockWidgets;
    LayoutSaver::DockWidget::List allDockWidgets;
    ScreenInfo::List screenInfo;

private:
    Q_DISABLE_COPY(Layout)
};

class LayoutSaver::Private
{
public:
    struct RAIIIsRestoring
    {
        RAIIIsRestoring();
        ~RAIIIsRestoring();
        Q_DISABLE_COPY(RAIIIsRestoring)
    };

    explicit Private(RestoreOptions options);

    bool matchesAffinity(const QStringList &affinities) const;
    void floatWidgetsWhichSkipRestore(const QStringList &mainWindowNames);
    void floatUnknownWidgets(const LayoutSaver::Layout &layout);

    template<typename T>
    void deserializeWindowGeometry(const T &saved, Core::Window::Ptr);
    void deleteEmptyGroups();
    void clearRestoredProperty();

    std::unique_ptr<QSettings> settings() const;
    DockRegistry *const m_dockRegistry;
    InternalRestoreOptions m_restoreOptions = {};
    QStringList m_affinityNames;

    static bool s_restoreInProgress;
};
}

#endif