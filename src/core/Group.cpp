/*
  This file is part of KDDockWidgets.

  SPDX-FileCopyrightText: 2020-2023 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.com>
  Author: Sérgio Martins <sergio.martins@kdab.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only

  Contact KDAB at <info@kdab.com> for commercial licensing options.
*/

#include "Group.h"

#include "kddockwidgets/Config.h"
#include "core/ViewFactory.h"

#include "Controller.h"
#include "View.h"
#include "Layout_p.h"
#include "FloatingWindow_p.h"
#include "ScopedValueRollback_p.h"
#include "Platform.h"
#include "views/GroupViewInterface.h"

#include "core/TitleBar.h"
#include "core/Stack.h"
#include "core/FloatingWindow.h"
#include "core/MDILayout.h"
#include "core/DropArea.h"
#include "core/Layout.h"
#include "core/MainWindow.h"
#include "core/TabBar_p.h"
#include "core/Group_p.h"

#include "DockRegistry.h"
#include "DockWidget_p.h"
#include "ObjectGuard_p.h"

#include "core/Logging_p.h"
#include "core/Utils_p.h"
#include "core/View_p.h"
#include "core/LayoutSaver_p.h"
#include "core/Position_p.h"
#include "core/WidgetResizeHandler_p.h"
#include "core/DelayedCall_p.h"
#include "core/layouting/Item_p.h"

#include "kdbindings/signal.h"

#include <utility>

#define MARGIN_THRESHOLD 100

static int s_dbg_numFrames = 0;

using namespace KDDockWidgets;
using namespace KDDockWidgets::Core;

namespace KDDockWidgets {

static FrameOptions actualOptions(FrameOptions options)
{
    if (Config::self().flags() & Config::Flag_AlwaysShowTabs)
        options |= FrameOption_AlwaysShowsTabs;

    return options;
}

static StackOptions tabWidgetOptions(FrameOptions options)
{
    if (options & FrameOption_NonDockable) {
        /// If we can't tab things into this Frame then let's not draw the QTabWidget group either
        return StackOption_DocumentMode;
    }

    return StackOption_None;
}

}

Group::Group(View *parent, FrameOptions options, int userType)
    : Controller(ViewType::Frame, Config::self().viewFactory()->createGroup(this, parent))
    , FocusScope(view())
    , d(new Private())
    , m_stack(new Core::Stack(this, tabWidgetOptions(options)))
    , m_tabBar(m_stack->tabBar())
    , m_titleBar(new Core::TitleBar(this))
    , m_options(actualOptions(options))
    , m_userType(userType)
{
    s_dbg_numFrames++;
    DockRegistry::self()->registerGroup(this);

    m_tabBar->dptr()->currentDockWidgetChanged.connect([this] {
        updateTitleAndIcon();
    });

    setLayout(parent ? parent->asLayout() : nullptr);
    m_stack->setTabBarAutoHide(!alwaysShowsTabs());
    view()->init();
    view()->d->closeRequested.connect([this](CloseEvent *ev) { onCloseEvent(ev); });

    // NOLINTNEXTLINE(cppcoreguidelines-prefer-member-initializer)
    m_inCtor = false;
}

Group::~Group()
{
    m_inDtor = true;
    s_dbg_numFrames--;
    if (d->m_layoutItem)
        d->m_layoutItem->unref();

    delete m_resizeHandler;
    m_resizeHandler = nullptr;

    DockRegistry::self()->unregisterGroup(this);

    // Run some disconnects() too, so we don't receive signals during destruction:
    setLayout(nullptr);
    delete m_titleBar;
    delete m_stack;
    delete d;
}

void Group::onCloseEvent(CloseEvent *e)
{
    e->accept(); // Accepted by default (will close unless ignored)
    const DockWidget::List docks = dockWidgets();
    for (DockWidget *dock : docks) {
        dock->view()->d->requestClose(e);
        if (!e->isAccepted())
            break; // Stop when the first dockwidget prevents closing
    }
}

void Group::setLayout(Layout *dt)
{
    if (dt == m_layout)
        return;

    const bool wasInMainWindow = dt && isInMainWindow();

    m_layout = dt;
    delete m_resizeHandler;
    m_resizeHandler = nullptr;

    if (m_layout) {
        if (isMDI())
            m_resizeHandler = new WidgetResizeHandler(WidgetResizeHandler::EventFilterMode::Global,
                                                      WidgetResizeHandler::WindowMode::MDI, view());

        // We keep the connect result so we don't dereference m_layout at shutdown
        d->m_visibleWidgetCountChangedConnection->disconnect(); // TODOm3: Remove if tests pass.
                                                                // It's a KDBindings bug.
        d->m_visibleWidgetCountChangedConnection =
            m_layout->d_ptr()->visibleWidgetCountChanged.connect(&Group::updateTitleBarVisibility, this);
        updateTitleBarVisibility();
        if (wasInMainWindow != isInMainWindow())
            d->isInMainWindowChanged.emit();
    }

    d->isMDIChanged.emit();
}

void Group::renameTab(int index, const QString &title)
{
    m_tabBar->renameTab(index, title);
}

void Group::changeTabIcon(int index, const Icon &icon)
{
    m_tabBar->changeTabIcon(index, icon);
}

int Group::nonContentsHeight() const
{
    return dynamic_cast<Core::GroupViewInterface *>(view())->nonContentsHeight();
}

Core::Stack *Group::stack() const
{
    return m_stack;
}

Core::TabBar *Group::tabBar() const
{
    return m_stack->tabBar();
}

void Group::updateTitleAndIcon()
{
    if (DockWidget *dw = currentDockWidget()) {
        m_titleBar->setTitle(dw->title());
        m_titleBar->setIcon(dw->icon());

        if (auto fw = floatingWindow()) {
            if (fw->hasSingleFrame()) {
                fw->updateTitleAndIcon();
            }
        }

        setObjectName(dw->uniqueName());

    } else if (currentTabIndex() != -1) {
        KDDW_ERROR("Invalid dock widget for group. index={}", currentTabIndex());
    }
}

void Group::onDockWidgetTitleChanged(DockWidget *dw)
{
    updateTitleAndIcon();

    if (!m_inCtor) { // don't call pure virtual in ctor
        int index = indexOfDockWidget(dw);
        renameTab(index, dw->title());
        changeTabIcon(index, dw->icon(IconPlace::TabBar));
    }
}

void Group::addTab(DockWidget *dockWidget, InitialOption addingOption)
{
    insertWidget(dockWidget, dockWidgetCount(), addingOption); // append
}

void Group::addTab(Group *group, InitialOption addingOption)
{
    if (group->isEmpty()) {
        KDDW_ERROR("Group::addTab: group is empty. group={}", ( void * )group);
        return;
    }

    const auto &docks = group->dockWidgets();
    for (DockWidget *dockWidget : docks)
        addTab(dockWidget, addingOption);
}

void Group::addTab(FloatingWindow *floatingWindow, InitialOption addingOption)
{
    assert(floatingWindow);
    for (Group *f : floatingWindow->groups())
        addTab(f, addingOption);
}

void Group::insertWidget(DockWidget *dockWidget, int index, InitialOption addingOption)
{
    assert(dockWidget);
    if (containsDockWidget(dockWidget)) {
        if (!dockWidget->isPersistentCentralDockWidget())
            KDDW_ERROR("Group::addTab dockWidget already exists. this={} ; dockWidget={}", ( void * )this, ( void * )dockWidget);
        return;
    }
    if (d->m_layoutItem)
        dockWidget->d->addPlaceholderItem(d->m_layoutItem);

    const int originalCurrentIndex = currentIndex();
    insertDockWidget(dockWidget, index);

    if (addingOption.startsHidden()) {
        dockWidget->view()->close(); // Ensure closed. TODOm4: Call controller instead.
    } else {
        if (hasSingleDockWidget()) {
            setObjectName(dockWidget->uniqueName());

            if (!d->m_layoutItem) {
                // When adding the 1st dock widget of a fresh group, let's give the group the size
                // of the dock widget, so that when adding it to the main window, the main window
                // can use that size as the initial suggested size.
                view()->resize(dockWidget->size());
            }
        } else if (addingOption.preservesCurrentTab() && originalCurrentIndex != -1) {
            setCurrentTabIndex(originalCurrentIndex);
        }

        dockWidget->d->setIsOpen(true);
    }

    KDBindings::ScopedConnection titleChangedConnection = dockWidget->d->titleChanged.connect(
        [this, dockWidget] { onDockWidgetTitleChanged(dockWidget); });

    KDBindings::ScopedConnection iconChangedConnection = dockWidget->d->iconChanged.connect(
        [this, dockWidget] { onDockWidgetTitleChanged(dockWidget); });

    d->titleChangedConnections[dockWidget] = std::move(titleChangedConnection);
    d->iconChangedConnections[dockWidget] = std::move(iconChangedConnection);
}

void Group::removeWidget(DockWidget *dw)
{
    auto it = d->titleChangedConnections.find(dw);
    if (it != d->titleChangedConnections.end())
        d->titleChangedConnections.erase(it);

    it = d->iconChangedConnections.find(dw);
    if (it != d->iconChangedConnections.end())
        d->iconChangedConnections.erase(it);

    dynamic_cast<Core::GroupViewInterface *>(view())->removeDockWidget(dw);
}

FloatingWindow *Group::detachTab(DockWidget *dockWidget)
{
    if (m_inCtor || m_inDtor)
        return nullptr;

    dockWidget->d->saveTabIndex();

    Rect r = dockWidget->geometry();
    removeWidget(dockWidget);

    auto newFrame = new Group();
    const Point globalPoint = mapToGlobal(Point(0, 0));
    newFrame->addTab(dockWidget);

    // We're potentially already dead at this point, as groups with 0 tabs auto-destruct. Don't
    // access members from this point.

    auto floatingWindow = new FloatingWindow(newFrame, {});
    r.moveTopLeft(globalPoint);
    floatingWindow->setSuggestedGeometry(r, SuggestedGeometryHint_GeometryIsFromDocked);
    floatingWindow->view()->show();

    return floatingWindow;
}

int Group::indexOfDockWidget(const DockWidget *dw)
{
    if (m_inCtor || m_inDtor)
        return -1;

    return m_tabBar->indexOfDockWidget(dw);
}

int Group::currentIndex() const
{
    if (m_inCtor || m_inDtor)
        return -1;

    return m_tabBar->currentIndex();
}

void Group::setCurrentTabIndex(int index)
{
    if (m_inCtor || m_inDtor)
        return;

    m_tabBar->setCurrentIndex(index);
}

void Group::setCurrentDockWidget(DockWidget *dw)
{
    if (m_inCtor || m_inDtor)
        return;

    m_tabBar->setCurrentDockWidget(dw);
}

void Group::insertDockWidget(DockWidget *dw, int index)
{
    if (m_inCtor || m_inDtor)
        return;

    dynamic_cast<Core::GroupViewInterface *>(view())->insertDockWidget(dw, index);
    dw->d->onParentChanged();
}

Core::DockWidget *Group::dockWidgetAt(int index) const
{
    if (m_inCtor || m_inDtor)
        return nullptr;

    return m_tabBar->dockWidgetAt(index);
}

Core::DockWidget *Group::currentDockWidget() const
{
    if (m_inCtor || m_inDtor)
        return nullptr;

    return m_tabBar->currentDockWidget();
}

int Group::dockWidgetCount() const
{
    if (m_inCtor || m_inDtor)
        return 0;

    return m_stack->numDockWidgets();
}

void Group::onDockWidgetCountChanged()
{
    if (isEmpty() && !isCentralFrame()) {
        scheduleDeleteLater();
    } else {
        updateTitleBarVisibility();

        // We don't really keep track of the state, so emit even if the visibility didn't change. No
        // biggie.
        if (!(m_options & FrameOption_AlwaysShowsTabs))
            d->hasTabsVisibleChanged.emit();

        const DockWidget::List docks = dockWidgets();
        for (DockWidget *dock : docks)
            dock->d->updateFloatAction();

        if (auto fw = floatingWindow()) {
            fw->dptr()->numDockWidgetsChanged.emit();
        }
    }

    d->numDockWidgetsChanged.emit();
}

void Group::isFocusedChangedCallback()
{
    d->isFocusedChanged.emit();
}

void Group::focusedWidgetChangedCallback()
{
    d->focusedWidgetChanged.emit();
}

void Group::updateTitleBarVisibility()
{
    if (m_updatingTitleBar || m_beingDeleted) {
        // To break a cyclic dependency
        return;
    }

    ScopedValueRollback guard(m_updatingTitleBar, true);

    bool visible = false;
    if (isCentralFrame()) {
        visible = false;
    } else if ((Config::self().flags() & Config::Flag_HideTitleBarWhenTabsVisible)
               && hasTabsVisible()) {
        visible = false;
    } else if (FloatingWindow *fw = floatingWindow()) {
        // If there's nested groups then show each Frame's title bar
        visible = !fw->hasSingleFrame();
    } else if (isMDIWrapper()) {
        auto dropArea = this->mdiDropAreaWrapper();
        visible = !dropArea->hasSingleFrame();
    } else {
        visible = true;
    }

    const bool wasVisible = m_titleBar->isVisible();
    m_titleBar->setVisible(visible);

    if (wasVisible != visible) {
        d->actualTitleBarChanged.emit();
        for (auto dw : dockWidgets())
            dw->d->actualTitleBarChanged.emit();
    }

    if (auto fw = floatingWindow()) {
        // Update the floating window which might be using Flag_HideTitleBarWhenTabsVisible
        // In that case it might not show title bar depending on the number of tabs that the group
        // has
        fw->updateTitleBarVisibility();
    }
}

void Group::updateFloatingActions()
{
    const Vector<DockWidget *> widgets = dockWidgets();
    for (DockWidget *dw : widgets)
        dw->d->updateFloatAction();
}

bool Group::containsMouse(Point globalPos) const
{
    return rect().contains(view()->mapFromGlobal(globalPos));
}

Core::TitleBar *Group::titleBar() const
{
    return m_titleBar;
}

Core::TitleBar *Group::actualTitleBar() const
{
    if (FloatingWindow *fw = floatingWindow()) {
        // If there's nested groups then show each Group's title bar
        if (fw->hasSingleFrame())
            return fw->titleBar();
    } else if (auto mdiDropArea = mdiDropAreaWrapper()) {
        if (mdiDropArea->hasSingleFrame()) {
            return mdiFrame()->titleBar();
        }
    }

    return titleBar();
}

QString Group::title() const
{
    return m_titleBar->title();
}

Icon Group::icon() const
{
    return m_titleBar->icon();
}

const Core::DockWidget::List Group::dockWidgets() const
{
    if (m_inCtor || m_inDtor)
        return {};

    DockWidget::List dockWidgets;
    const int count = dockWidgetCount();
    dockWidgets.reserve(count);
    for (int i = 0; i < count; ++i)
        dockWidgets.push_back(dockWidgetAt(i));

    return dockWidgets;
}

bool Group::containsDockWidget(DockWidget *dockWidget) const
{
    const int count = dockWidgetCount();
    for (int i = 0, e = count; i != e; ++i) {
        if (dockWidget == dockWidgetAt(i))
            return true;
    }
    return false;
}

FloatingWindow *Group::floatingWindow() const
{
    // Returns the first FloatingWindow* parent in the hierarchy.
    // However, if there's a MainWindow in the hierarchy it stops, which can
    // happen with nested main windows.

    auto p = view()->parentView();
    while (p) {
        if (p->is(ViewType::MainWindow))
            return nullptr;

        if (auto fw = p->asFloatingWindowController())
            return fw;

        if (p->equals(view()->rootView())) {
            // We stop at the window. (top-levels can have parent, but we're not interested)
            return nullptr;
        }

        p = p->parentView();
    }

    return nullptr;
}

void Group::restoreToPreviousPosition()
{
    if (hasSingleDockWidget()) {
        KDDW_ERROR("Invalid usage, there's no tabs");
        return;
    }

    if (!d->m_layoutItem) {
        KDDW_DEBUG("Group::restoreToPreviousPosition: There's no previous position known");
        return;
    }

    if (!d->m_layoutItem->isPlaceholder()) {
        // Maybe in this case just fold the group into the placeholder, which probably has other
        // dockwidgets which were added meanwhile. TODO
        KDDW_DEBUG("Group::restoreToPreviousPosition: Previous position isn't a placeholder");
        return;
    }

    d->m_layoutItem->restore(view());
}

int Group::currentTabIndex() const
{
    return currentIndex();
}

bool Group::anyNonClosable() const
{
    for (auto dw : dockWidgets()) {
        if ((dw->options() & DockWidgetOption_NotClosable)
            && !Platform::instance()->isProcessingAppQuitEvent())
            return true;
    }

    return false;
}

bool Group::anyNonDockable() const
{
    for (auto dw : dockWidgets()) {
        if (dw->options() & DockWidgetOption_NotDockable)
            return true;
    }

    return false;
}

void Group::setLayoutItem(Item *item)
{
    if (item == d->m_layoutItem)
        return;

    if (d->m_layoutItem)
        d->m_layoutItem->unref();

    if (item)
        item->ref();

    d->m_layoutItem = item;
    if (item) {
        for (DockWidget *dw : dockWidgets())
            dw->d->addPlaceholderItem(item);
    } else {
        for (DockWidget *dw : dockWidgets())
            dw->d->lastPosition()->removePlaceholders();
    }
}

Item *Group::layoutItem() const
{
    return d->m_layoutItem;
}

int Group::dbg_numFrames()
{
    return s_dbg_numFrames;
}

bool Group::beingDeletedLater() const
{
    return m_beingDeleted;
}

bool Group::hasTabsVisible() const
{
    if (m_beingDeleted)
        return false;

    return alwaysShowsTabs() || dockWidgetCount() > 1;
}

Vector<QString> Group::affinities() const
{
    if (isEmpty()) {
        if (auto m = mainWindow())
            return m->affinities();
        return {};
    } else {
        return dockWidgetAt(0)->affinities();
    }
}

bool Group::isTheOnlyGroup() const
{
    return m_layout && m_layout->visibleCount() == 1;
}

bool Group::isOverlayed() const
{
    return m_options & FrameOption_IsOverlayed;
}

void Group::unoverlay()
{
    m_options &= ~FrameOption_IsOverlayed;
}

bool Group::isFloating() const
{
    if (isInMainWindow() || isMDI())
        return false;

    return isTheOnlyGroup();
}

bool Group::isInFloatingWindow() const
{
    return floatingWindow() != nullptr;
}

bool Group::isInMainWindow() const
{
    return mainWindow() != nullptr;
}

Group *Group::deserialize(const LayoutSaver::Group &f)
{
    if (!f.isValid())
        return nullptr;

    const FrameOptions options = FrameOptions(f.options);
    Group *group = nullptr;
    const bool isPersistentCentralFrame = options & FrameOption::FrameOption_IsCentralFrame;

    if (isPersistentCentralFrame) {
        // Don't create a new Group if we're restoring the Persistent Central group (the one created
        // by MainWindowOption_HasCentralFrame). It already exists.

        if (f.mainWindowUniqueName.isEmpty()) {
            // Can happen with older serialization formats
            KDDW_ERROR("Frame is the persistent central group but doesn't have"
                       "an associated window name");
        } else {
            if (MainWindow *mw = DockRegistry::self()->mainWindowByName(f.mainWindowUniqueName)) {
                group = mw->dropArea()->centralGroup();
                if (!group) {
                    // Doesn't happen...
                    KDDW_ERROR("Main window {} doesn't have central group", f.mainWindowUniqueName);
                }
            } else {
                // Doesn't happen...
                KDDW_ERROR("Couldn't find main window {}", f.mainWindowUniqueName);
            }
        }
    }

    if (!group)
        group = new Group(nullptr, options);

    group->setObjectName(f.objectName);

    for (const auto &savedDock : std::as_const(f.dockWidgets)) {
        if (DockWidget *dw = DockWidget::deserialize(savedDock)) {
            group->addTab(dw);
        }
    }

    group->setCurrentTabIndex(f.currentTabIndex);
    group->view()->setGeometry(f.geometry);

    return group;
}

LayoutSaver::Group Group::serialize() const
{
    LayoutSaver::Group group;
    group.isNull = false;

    const DockWidget::List docks = dockWidgets();

    group.objectName = objectName();
    group.geometry = geometry();
    group.options = options();
    group.currentTabIndex = currentTabIndex();
    group.id = view()->d->id(); // for coorelation purposes

    if (MainWindow *mw = mainWindow()) {
        group.mainWindowUniqueName = mw->uniqueName();
    }

    for (DockWidget *dock : docks)
        group.dockWidgets.push_back(dock->d->serialize());

    return group;
}

void Group::scheduleDeleteLater()
{
    KDDW_TRACE("Group::scheduleDeleteLater: {}", ( void * )this);
    m_beingDeleted = true;

    // Can't use deleteLater() here due to QTBUG-83030 (deleteLater() never delivered if
    // triggered by a sendEvent() before event loop starts)
    destroyLater();
}

Size Group::dockWidgetsMinSize() const
{
    Size size = Item::hardcodedMinimumSize;
    for (DockWidget *dw : dockWidgets()) {
        if (!dw->inDtor())
            size = size.expandedTo(dw->view()->minSize());
    }


    return size;
}

Size Group::biggestDockWidgetMaxSize() const
{
    Size size = Item::hardcodedMaximumSize;
    for (DockWidget *dw : dockWidgets()) {
        if (dw->inDtor())
            continue;
        const Size dwMax = dw->view()->maxSizeHint();
        if (size == Item::hardcodedMaximumSize) {
            size = dwMax;
            continue;
        }

        const bool hasMaxSize = dwMax != Item::hardcodedMaximumSize;
        if (hasMaxSize)
            size = dwMax.expandedTo(size);
    }

    // Interpret 0 max-size as not having one too.
    if (size.width() == 0)
        size.setWidth(Item::hardcodedMaximumSize.width());
    if (size.height() == 0)
        size.setHeight(Item::hardcodedMaximumSize.height());

    return size;
}

Rect Group::dragRect() const
{
    Rect rect;
    if (m_titleBar->isVisible()) {
        rect = m_titleBar->view()->rect();
        rect.moveTopLeft(m_titleBar->view()->mapToGlobal(Point(0, 0)));
    }

    if (rect.isValid())
        return rect;

    return dynamic_cast<Core::GroupViewInterface *>(view())->dragRect();
}

MainWindow *Group::mainWindow() const
{
    return m_layout ? m_layout->mainWindow() : nullptr;
}

///@brief Returns whether all dock widgets have the specified option set
bool Group::allDockWidgetsHave(DockWidgetOption option) const
{
    const DockWidget::List docks = dockWidgets();
    return std::all_of(docks.cbegin(), docks.cend(),
                       [option](DockWidget *dw) { return dw->options() & option; });
}

///@brief Returns whether at least one dock widget has the specified option set
bool Group::anyDockWidgetsHas(DockWidgetOption option) const
{
    const DockWidget::List docks = dockWidgets();
    return std::any_of(docks.cbegin(), docks.cend(),
                       [option](DockWidget *dw) { return dw->options() & option; });
}

bool Group::allDockWidgetsHave(LayoutSaverOption option) const
{
    const DockWidget::List docks = dockWidgets();
    return std::all_of(docks.cbegin(), docks.cend(),
                       [option](DockWidget *dw) { return dw->layoutSaverOptions() & option; });
}

bool Group::anyDockWidgetsHas(LayoutSaverOption option) const
{
    const DockWidget::List docks = dockWidgets();
    return std::any_of(docks.cbegin(), docks.cend(),
                       [option](DockWidget *dw) { return dw->layoutSaverOptions() & option; });
}

void Group::setAllowedResizeSides(CursorPositions sides)
{
    if (sides) {
        delete m_resizeHandler;
        m_resizeHandler = new WidgetResizeHandler(WidgetResizeHandler::EventFilterMode::Global,
                                                  WidgetResizeHandler::WindowMode::MDI, view());
        m_resizeHandler->setAllowedResizeSides(sides);
    } else {
        delete m_resizeHandler;
        m_resizeHandler = nullptr;
    }
}

bool Group::isMDI() const
{
    return mdiLayout() != nullptr;
}

bool Group::isMDIWrapper() const
{
    return mdiDropAreaWrapper() != nullptr;
}

Group *Group::mdiFrame() const
{
    if (auto dwWrapper = mdiDockWidgetWrapper()) {
        return dwWrapper->d->group();
    }

    return nullptr;
}

Core::DockWidget *Group::mdiDockWidgetWrapper() const
{
    if (auto dropArea = mdiDropAreaWrapper())
        return dropArea->view()->parentView()->asDockWidgetController();

    return nullptr;
}

DropArea *Group::mdiDropAreaWrapper() const
{
    auto p = view()->parentView();
    auto dropArea = p ? p->asDropAreaController() : nullptr;
    if (dropArea && dropArea->isMDIWrapper())
        return dropArea;
    return nullptr;
}

MDILayout *Group::mdiLayout() const
{
    return m_layout ? m_layout->asMDILayout() : nullptr;
}

bool Group::hasNestedMDIDockWidgets() const
{
    if (!isMDI() || dockWidgetCount() == 0)
        return false;

    if (dockWidgetCount() != 1) {
        KDDW_ERROR("Expected a single dock widget wrapper as group child");
        return false;
    }

    return dockWidgetAt(0)->d->isMDIWrapper();
}

int Group::userType() const
{
    return m_userType;
}

WidgetResizeHandler *Group::resizeHandler() const
{
    return m_resizeHandler;
}

void Group::setParentView_impl(View *parent)
{
    Controller::setParentView_impl(parent);
    setLayout(parent ? parent->asLayout() : nullptr);
}

FloatingWindowFlags Group::requestedFloatingWindowFlags() const
{
    const auto dockwidgets = this->dockWidgets();
    if (!dockwidgets.isEmpty())
        return dockwidgets.first()->floatingWindowFlags();

    return FloatingWindowFlag::FromGlobalConfig;
}

Group::Private *Group::dptr() const
{
    return d;
}
