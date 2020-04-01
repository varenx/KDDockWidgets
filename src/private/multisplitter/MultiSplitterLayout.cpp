/*
  This file is part of KDDockWidgets.

  Copyright (C) 2018-2020 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
  Author: Sérgio Martins <sergio.martins@kdab.com>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "MultiSplitterLayout_p.h"
#include "Logging_p.h"
#include "MultiSplitter_p.h"
#include "Frame_p.h"
#include "FloatingWindow_p.h"
#include "DockWidgetBase.h"
#include "LastPosition_p.h"
#include "DockRegistry_p.h"
#include "Config.h"
#include "Separator_p.h"
#include "FrameworkWidgetFactory.h"
#include "LayoutSaver.h"

#include <QAction>
#include <QEvent>
#include <QtMath>
#include <QScopedValueRollback>

#define INDICATOR_MINIMUM_LENGTH 100
#define KDDOCKWIDGETS_MIN_WIDTH 80
#define KDDOCKWIDGETS_MIN_HEIGHT 90

using namespace KDDockWidgets;

const QString MultiSplitterLayout::s_magicMarker = QStringLiteral("bac9948e-5f1b-4271-acc5-07f1708e2611");


MultiSplitterLayout::MultiSplitterLayout(MultiSplitter *parent)
    : QObject(parent)
    , m_multiSplitter(parent)
    , m_rootItem(new Item())
{
    Q_ASSERT(parent);

    DockRegistry::self()->registerLayout(this);

    setSize(parent->size());

    qCDebug(multisplittercreation()) << "MultiSplitter";
    connect(this, &MultiSplitterLayout::widgetCountChanged, this, [this] {
        Q_EMIT visibleWidgetCountChanged(visibleCount());
    });

    clear();

    // Initialize min size
    updateSizeConstraints();
    m_inCtor = false;
}

MultiSplitterLayout::~MultiSplitterLayout()
{
    qCDebug(multisplittercreation) << "~MultiSplitter" << this;
    m_inDestructor = true;
    const auto anchors = m_anchors;
    qDeleteAll(anchors);
    delete m_rootItem;
    DockRegistry::self()->unregisterLayout(this);
}

/**static*/
QSize MultiSplitterLayout::hardcodedMinimumSize()
{
    return QSize(KDDOCKWIDGETS_MIN_WIDTH, KDDOCKWIDGETS_MIN_HEIGHT);
}

MultiSplitter *MultiSplitterLayout::multiSplitter() const
{
    return m_multiSplitter;
}

bool MultiSplitterLayout::validateInputs(QWidgetOrQuick *widget,
                                         Location location,
                                         const Frame *relativeToFrame, AddingOption option) const
{
    if (!widget) {
        qWarning() << Q_FUNC_INFO << "Widget is null";
        return false;
    }

    const bool isDockWidget = qobject_cast<DockWidgetBase*>(widget);
    const bool isStartHidden = option & AddingOption_StartHidden;

    if (!qobject_cast<Frame*>(widget) && !qobject_cast<MultiSplitter*>(widget) && !isDockWidget) {
        qWarning() << "Unknown widget type" << widget;
        return false;
    }

    if (isDockWidget != isStartHidden) {
        qWarning() << "Wrong parameters" << isDockWidget << isStartHidden;
        return false;
    }

    if (relativeToFrame && relativeToFrame == widget) {
        qWarning() << "widget can't be relative to itself";
        return false;
    }

    Item *item = itemForFrame(qobject_cast<Frame*>(widget));

    if (contains(item)) {
        qWarning() << "MultiSplitterLayout::addWidget: Already contains" << widget;
        return false;
    }// TODO: check for widget changing parent

    if (location == Location_None) {
        qWarning() << "MultiSplitterLayout::addWidget: not adding to location None";
        return false;
    }

    const bool relativeToThis = relativeToFrame == nullptr;

    Item *relativeToItem = itemForFrame(relativeToFrame);
    if (!relativeToThis && !contains(relativeToItem)) {
        qWarning() << "MultiSplitterLayout::addWidget: Doesn't contain relativeTo:"
                   << relativeToFrame
                   << "; options=" << option;
        return false;
    }

    return true;
}

std::pair<int,int> MultiSplitterLayout::boundInterval(int newPos1, Anchor* anchor1, int newPos2, Anchor *anchor2) const
{
    const int bound1 = boundPositionForAnchor(anchor1, Anchor::Side1);
    const int bound2 = boundPositionForAnchor(anchor2, Anchor::Side2);

    if (newPos1 >= bound1 && newPos2 <= bound2) {
        // Simplest case, it's bounded.
        return { newPos1, newPos2 };
    }

    if (newPos1 < bound1) {
        // the anchor1 is out of bounds

        const int bythismuch = bound1 - newPos1;
        newPos1 = bound1;
        newPos2 = newPos2 + bythismuch;

        if (newPos2 > bound2) {
            qWarning() << "Adjusted interval still out of bounds. Not enough space. #1"
                       << "; newPos1=" << newPos1
                       << "; newPos2=" << newPos2
                       << "; bounds=" << bound1 << bound2
                       << "; anchor1=" << anchor1
                       << "; anchor2=" << anchor2
                       << "; size=" << size();
        }

        return { newPos1, newPos2 };
    } else if (newPos2 > bound2) {
        // the anchor2 is out of bounds

        const int bythismuch = newPos2 - bound2;
        newPos2 = bound2;
        newPos1 = newPos1 - bythismuch;

        if (newPos1 < bound1) {
            qWarning() << "Adjusted interval still out of bounds. Not enough space. #2"
                       << "; newPos1=" << newPos1
                       << "; newPos2=" << newPos2
                       << "; bounds=" << bound1 << bound2
                       << "; anchor1=" << anchor1
                       << "; anchor2=" << anchor2
                       << "; size=" << size();
        }

        return { newPos1, newPos2 };
    }

    return { newPos1, newPos2 };
}

void MultiSplitterLayout::addWidget(QWidgetOrQuick *w, Location location, Frame *relativeToWidget, AddingOption option)
{
    auto frame = qobject_cast<Frame*>(w);
    qCDebug(addwidget) << Q_FUNC_INFO << w
                       << "; location=" << locationStr(location)
                       << "; relativeTo=" << relativeToWidget
                       << "; size=" << size()
                       << "; w.size=" << w->size()
                       << "; w.min=" << widgetMinLength(w, orientationForLocation(location))
                       << "; frame=" << frame
                       << "; option=" << option;

    if (itemForFrame(frame) != nullptr) {
        // Item already exists, remove it.
        // Changing the frame parent will make the item clean itself up. It turns into a placeholder and is removed by unrefOldPlaceholders
        frame->setParent(nullptr); // so ~Item doesn't delete it
        frame->setLayoutItem(nullptr); // so Item is destroyed, as there's no refs to it
    }

    // Make some sanity checks:
    if (!validateInputs(w, location, relativeToWidget, option))
        return;

    unrefOldPlaceholders(framesFrom(w));
    //Item *relativeToItem = itemForFrame(relativeToWidget);

}

void MultiSplitterLayout::addItems_internal(const ItemList &items, bool updateConstraints, bool emitSignal)
{
    m_items << items;
    if (updateConstraints)
        updateSizeConstraints();

    for (auto item : items) {
        item->setLayout(this);
        if (item->frame()) {
            item->setVisible(true);
            item->frame()->installEventFilter(this);
            Q_EMIT widgetAdded(item);
        }
    }

    if (emitSignal)
        Q_EMIT widgetCountChanged(m_items.size());
}

void MultiSplitterLayout::addAsPlaceholder(DockWidgetBase *dockWidget, Location location, Item *relativeTo)
{
    if (!dockWidget) {
        qWarning() << Q_FUNC_INFO << "null dockwidget";
        return;
    }

    dockWidget->setParent(nullptr);

    auto result = createTargetAnchorGroup(location, relativeTo);
    AnchorGroup targetAnchorGroup = result.first;

    auto frame = Config::self().frameworkWidgetFactory()->createFrame(m_multiSplitter);
    auto item = new Item(frame, this);

    targetAnchorGroup.addItem(item);
    addItems_internal(ItemList{ item }, false);

    dockWidget->addPlaceholderItem(item);
    delete frame;

    updateAnchorFollowing();
    Q_ASSERT(!dockWidget->isVisible());
    maybeCheckSanity();
}

void MultiSplitterLayout::ensureEnoughSize(const QWidgetOrQuick *widget,
                                                   Location location, const Item *relativeToItem)
{
    const int neededAnchorThickness = isEmpty() ? 0 : Anchor::thickness(/*static=*/ false);
    const QSize available = availableSize();
    const QSize widgetMin = { widgetMinLength(widget, Qt::Vertical), widgetMinLength(widget, Qt::Horizontal) };
    const QSize oldSize = m_size;
    const int neededWidth = widgetMin.width() - available.width() + neededAnchorThickness;
    const int neededHeight = widgetMin.height() - available.height() + neededAnchorThickness;

    QSize newSize = m_size;
    if (neededWidth > 0)
        newSize.setWidth(newSize.width() + neededWidth);
    if (neededHeight > 0)
        newSize.setHeight(newSize.height() + neededHeight);

    if (newSize != m_size)
        setSize(newSize);

    // Just to make sure:
    if (lengthForDrop(widget, location, relativeToItem).isNull()) {
        qWarning() << Q_FUNC_INFO << "failed! Please report a bug."
                   << "; oldAvailable=" << available
                   << "; newAvailable=" << availableSize()
                   << "; newSize=" << newSize
                   << "; m_size=" << m_size
                   << "; oldSize=" << oldSize
                   << "; widgetMin=" << widgetMin
                   << "; isEmpty=" << isEmpty();
    }
}

void MultiSplitterLayout::ensureAnchorsBounded()
{
    //Ensures all separators are within their bounds, meaning all items obey their min size
    positionStaticAnchors();
    ensureItemsMinSize();
}

static Anchor::List removeSmallestPath(QVector<Anchor::List> &paths)
{
    // Removes and returns the smallest list
    Anchor::List smallestPath;
    int indexOfSmallest = 0;
    for (int i = 0, end = paths.size(); i < end; ++i) {
        const Anchor::List &path = paths.at(i);
        if (path.size() <= smallestPath.size() || smallestPath.isEmpty()) {
            smallestPath = path;
            indexOfSmallest = i;
        }
    }

    paths.removeAt(indexOfSmallest);
    return smallestPath;
}

void MultiSplitterLayout::propagateResize(int delta, Anchor *fromAnchor, Anchor::Side direction)
{
    if (delta < 0)
        qWarning() << Q_FUNC_INFO << "Invalid delta" << delta << fromAnchor << direction;

    if (delta <= 0 || fromAnchor->isStatic())
        return;

    QVector<Anchor::List> paths;
    collectPaths(paths, fromAnchor, direction);

    for (const Anchor::List &path : qAsConst(paths)) {
        qCDebug(sizing) << Q_FUNC_INFO << path;
    }

    Anchor::List anchorsThatAlreadyContributed;
    anchorsThatAlreadyContributed.push_back(fromAnchor);

    while (!paths.isEmpty()) {
        // Get smallest path:
        Anchor::List smallestPath = removeSmallestPath(/*by-ref*/paths);
        if (smallestPath.size() <= 1) {
            // Nothing to do, it has a single anchor, which was already adjusted in addWidget()
            continue;
        }

        const bool towardsSide1 = direction == Anchor::Side1;
        const bool towardsSide2 = !towardsSide1;

        const int sign = towardsSide1 ? -1 : 1;
        const int contributionPerAnchor = (delta / (smallestPath.size() - 1)) * sign; // n-1 because the initial anchor already contributed
        if (qAbs(contributionPerAnchor) < 5) {
            // Too small, don't bother
            continue;
        }

        // Now make those anchors contribute, skipping the first
        for (int i = 1, end = smallestPath.size(); i < end; ++i) {
            Anchor *a = smallestPath.at(i);
            if (!anchorsThatAlreadyContributed.contains(a)) {
                // When moving anchors don't allow widgets to go bellow their min size
                const int bound = boundPositionForAnchor(a, direction);
                int newPosition = a->position() + contributionPerAnchor;
                if ((towardsSide1 && newPosition < bound) ||
                    (towardsSide2 && newPosition > bound)) {
                    newPosition = bound;
                }

                if (a->position() != newPosition) {
                    a->setPosition(newPosition);
                    anchorsThatAlreadyContributed.push_back(a);
                }
            }
        }
    }
}

void MultiSplitterLayout::collectPaths(QVector<Anchor::List> &paths, Anchor *fromAnchor, Anchor::Side direction)
{
    if (fromAnchor->isStatic()) {
        // We've finally reached a border anchor, we can stop now.
        return;
    }

    if (paths.isEmpty())
        paths.push_back({});

    int currentPathIndex = paths.size() - 1; // Store the index instead of using "Anchor::List &currentPath = paths.last();" as the references are stable, as the paths vector reallocates

    paths[currentPathIndex].push_back(fromAnchor);

    const ItemList items = fromAnchor->items(direction);
    for (int i = 0, end = items.size(); i < end; ++i) {
        Anchor *nextAnchor = items[i]->anchorAtSide(direction, fromAnchor->orientation());
        if (i > 0) {
            Anchor::List newPath = paths[currentPathIndex];
            paths.push_back(newPath);
        }
        collectPaths(paths, nextAnchor, direction);
    }
}

void MultiSplitterLayout::resizeItem(Frame *frame, int newSize, Qt::Orientation orientation)
{
    // Used for unit-tests only
    Item *item = itemForFrame(frame);
    Q_ASSERT(item);
    Anchor *a = item->anchorAtSide(Anchor::Side2, orientation);
    Q_ASSERT(!a->isStatic());
    const int widgLength = item->length(orientation);
    const int delta = newSize - widgLength;
    qCDebug(::anchors) << Q_FUNC_INFO << "Old position:" << a->position() << "; old w.geo=" << item->geometry();
    a->setPosition(a->position() + delta);
    qCDebug(::anchors) << Q_FUNC_INFO << "New position:" << a->position() << "; new w.geo=" << item->geometry();
}

void MultiSplitterLayout::ensureItemsMinSize()
{
    for (Item *item : qAsConst(m_items)) {
        item->ensureMinSize(Qt::Vertical);
        item->ensureMinSize(Qt::Horizontal);
    }
}

QString MultiSplitterLayout::affinityName() const
{
    if (auto ms = multiSplitter()) {
        if (auto mainWindow = ms->mainWindow()) {
            return mainWindow->affinityName();
        } else if (auto fw = ms->floatingWindow()) {
            return fw->affinityName();
        }
    }

    return QString();
}

void MultiSplitterLayout::addMultiSplitter(MultiSplitter *sourceMultiSplitter,
                                           Location location,
                                           Frame *relativeTo)
{
    qCDebug(addwidget) << Q_FUNC_INFO << sourceMultiSplitter << location << relativeTo;
    addWidget(sourceMultiSplitter, location, relativeTo);
}

void MultiSplitterLayout::removeItem(Item *item)
{
    if (!item || m_inDestructor || !m_items.contains(item))
        return;

    maybeCheckSanity();

    if (!item->isPlaceholder())
        item->frame()->removeEventFilter(this);
    AnchorGroup anchorGroup = item->anchorGroup();
    anchorGroup.removeItem(item);
    m_items.removeOne(item);

    updateAnchorFollowing();

    Q_EMIT widgetRemoved(item);
    Q_EMIT widgetCountChanged(m_items.size());
}

bool MultiSplitterLayout::contains(const Item *item) const
{
    return m_items.contains(const_cast<Item*>(item));
}

bool MultiSplitterLayout::contains(const Frame *frame) const
{
    return itemForFrame(frame) != nullptr;
}

Item *MultiSplitterLayout::itemAt(QPoint p) const
{
    for (Item *item : m_items) {
        if (!item->isPlaceholder() && item->geometry().contains(p))
            return item;
    }

    return nullptr;
}

void MultiSplitterLayout::clear(bool alsoDeleteStaticAnchors)
{
    const int oldCount = count();
    const int oldVisibleCount = visibleCount();
    const auto items = m_items;
    m_items.clear(); // Clear the item list first, do avoid ~Item() triggering a removal from the list
    qDeleteAll(items);

    const auto anchors = m_anchors;
    m_anchors.clear();

    for (Anchor *anchor : qAsConst(anchors)) {
        anchor->clear();
        if (!anchor->isStatic() || alsoDeleteStaticAnchors) {
            delete anchor;
        }
    }

    if (alsoDeleteStaticAnchors) {
        m_anchors.clear();
        m_topAnchor = nullptr;
        m_bottomAnchor = nullptr;
        m_leftAnchor = nullptr;
        m_rightAnchor = nullptr;
        m_staticAnchorGroup.left = nullptr;
        m_staticAnchorGroup.top = nullptr;
        m_staticAnchorGroup.right = nullptr;
        m_staticAnchorGroup.bottom = nullptr;
    } else {
        m_anchors = { m_topAnchor, m_bottomAnchor, m_leftAnchor, m_rightAnchor };
    }

    if (oldCount > 0)
        Q_EMIT widgetCountChanged(0);
    if (oldVisibleCount > 0)
        Q_EMIT visibleWidgetCountChanged(0);

}

int MultiSplitterLayout::visibleCount() const
{
    int count = 0;
    for (auto item : m_items)
        if (!item->isPlaceholder())
            count++;
    return count;
}

int MultiSplitterLayout::placeholderCount() const
{
    return count() - visibleCount();
}



void MultiSplitterLayout::setAnchorBeingDragged(Anchor *anchor)
{
    m_anchorBeingDragged = anchor;
}

int MultiSplitterLayout::numVisibleAnchors() const
{
    int count = 0;
    for (Anchor *a : m_anchors) {
        if (a->separatorWidget()->isVisible())
            count++;
    }

    return count;
}

void MultiSplitterLayout::updateSizeConstraints()
{
    const QSize newMinSize = m_rootItem->minSize();
    qCDebug(sizing) << Q_FUNC_INFO << "Updating size constraints from" << m_minSize
                    << "to" << newMinSize;

    setMinimumSize(newMinSize);
}

void MultiSplitterLayout::emitVisibleWidgetCountChanged()
{
    if (!m_inDestructor)
        Q_EMIT visibleWidgetCountChanged(visibleCount());
}

Item *MultiSplitterLayout::itemForFrame(const Frame *frame) const
{
    if (!frame)
        return nullptr;

    for (Item *item : m_items) {
        if (item->frame() == frame)
            return item;
    }
    return nullptr;
}

Frame::List MultiSplitterLayout::framesFrom(QWidgetOrQuick *frameOrMultiSplitter) const
{
    if (auto frame = qobject_cast<Frame*>(frameOrMultiSplitter))
        return { frame };

    if (auto msw = qobject_cast<MultiSplitter*>(frameOrMultiSplitter))
        return msw->multiSplitterLayout()->frames();

    return {};
}

Frame::List MultiSplitterLayout::frames() const
{
    Frame::List result;

    for (Item *item : m_items) {
        if (auto f = static_cast<Frame*>(item->frame()))
            result.push_back(f);
    }

    return result;
}

QVector<DockWidgetBase *> MultiSplitterLayout::dockWidgets() const
{
    DockWidgetBase::List result;
    const Frame::List frames = this->frames();

    for (Frame *frame : frames)
        result << frame->dockWidgets();

    return result;
}

bool MultiSplitterLayout::checkSanity() const
{
    return m_rootItem->checkSanity();
}

void MultiSplitterLayout::unrefOldPlaceholders(const Frame::List &framesBeingAdded) const
{
    for (Frame *frame : framesBeingAdded) {
        for (DockWidgetBase *dw : frame->dockWidgets()) {
            if (Item *existingItem = dw->lastPosition()->layoutItem()) {
                if (contains(existingItem)) { // We're only interested in placeholders from this layout
                    dw->lastPosition()->removePlaceholders(this);
                }
            }
        }
    }
}

void MultiSplitterLayout::setSize(QSize size)
{
    if (size != m_size) {
        m_resizing = true;
        QSize oldSize = m_size;

        if (size.width() < m_minSize.width() || size.height() < m_minSize.height()) {
            qWarning() << Q_FUNC_INFO << "new size is smaller than min size. Size=" << size << "; min=" << m_minSize;
            return;
        }

#if defined(DOCKS_DEVELOPER_MODE)
        if (!m_inCtor && false) { // TODO Uncomment when it passes
            QSize minSizeCalculated = QSize(availableLengthForOrientation(Qt::Vertical), availableLengthForOrientation(Qt::Horizontal));
            if (size.width() < minSizeCalculated.width() || size.height() < minSizeCalculated.height()) {
                qWarning() << Q_FUNC_INFO << "new size is smaller than min size calculated" << size << minSizeCalculated;
            }
        }
#endif

        m_size = size;
        Q_EMIT sizeChanged(size);

        redistributeSpace(oldSize, size);
        m_resizing = false;

        if (!m_restoringPlaceholder) { // ensureAnchorsBounded() is run at the end of restorePlaceholder() already.
            ensureAnchorsBounded();
        }
    }
}

void MultiSplitterLayout::setContentLength(int value, Qt::Orientation o)
{
    if (o == Qt::Vertical) {
        // Setting the width
        setSize({value, m_size.height()});
    } else {
        // Setting the height
        setSize({m_size.width(), value});
    }
}

int MultiSplitterLayout::length(Qt::Orientation o) const
{
    return o == Qt::Vertical ? width()
                             : height();
}

void MultiSplitterLayout::setMinimumSize(QSize sz)
{
    if (sz != m_minSize) {
        m_minSize = sz;
        setSize(m_size.expandedTo(m_minSize)); // Increase size incase we need to
        Q_EMIT minimumSizeChanged(sz);
    }
    qCDebug(sizing) << Q_FUNC_INFO << "minSize = " << m_minSize;
}

const ItemList MultiSplitterLayout::items() const
{
    return m_items;
}

Item *MultiSplitterLayout::rootItem() const
{
    return m_rootItem;
}

bool MultiSplitterLayout::eventFilter(QObject *o, QEvent *e)
{
    if (m_inDestructor || e->spontaneous() || !m_multiSplitter)
        return false;

    if (!m_multiSplitter->isVisible()) {
        // The whole MultiSplitter isn't visible, don't bother. It probably even is being hidden by ~QMainWindow().
        return false;
    }

    QWidget *w = qobject_cast<QWidget*>(o);
    if (!w || !w->testAttribute(Qt::WA_WState_ExplicitShowHide)) {
        // We only care about explicit show/hide by the developer
        return false;
    }

    return false;
}

bool MultiSplitterLayout::deserialize(const LayoutSaver::MultiSplitterLayout &msl)
{
    clear(true);

    ItemList items;
    items.reserve(msl.items.size());
    for (const auto &i : qAsConst(msl.items)) {
        Item *item = Item::deserialize(i, this);
        items.push_back(item);
    }

    m_items = items; // Set the items, so Anchor::deserialize() can set the side1 and side2 items

    for (const auto &a : qAsConst(msl.anchors)) {
        Anchor *anchor = Anchor::deserialize(a, this); // They auto-register into m_anchors
        if (!anchor)
            return false;

        if (anchor->type() == Anchor::Type_LeftStatic) {
            Q_ASSERT(!m_leftAnchor);
            m_leftAnchor = anchor;
        } else if (anchor->type() == Anchor::Type_TopStatic) {
            Q_ASSERT(!m_topAnchor);
            m_topAnchor = anchor;
        } else if (anchor->type() == Anchor::Type_RightStatic) {
            Q_ASSERT(!m_rightAnchor);
            m_rightAnchor = anchor;
        } else if (anchor->type() == Anchor::Type_BottomStatic) {
            Q_ASSERT(!m_bottomAnchor);
            m_bottomAnchor = anchor;
        }
    }


    m_items.clear(); // Now properly set the items, which installs needed event filters, etc.
    addItems_internal(items, false, false); // Add the items only after we have the static anchors set

    for (Anchor *anchor : qAsConst(m_anchors)) {
        int indexFrom = anchor->property("indexFrom").toInt();
        int indexTo = anchor->property("indexTo").toInt();
        int indexFolowee = anchor->property("indexFolowee").toInt();
        anchor->setProperty("indexFrom", QVariant());
        anchor->setProperty("indexTo", QVariant());
        anchor->setProperty("indexFolowee", QVariant());

        anchor->setFrom(m_anchors.at(indexFrom));
        anchor->setTo(m_anchors.at(indexTo));
        if (indexFolowee != -1)
            anchor->setFollowee(m_anchors.at(indexFolowee));
    }

    m_size = msl.size;
    m_minSize = msl.minSize;

    // Now that the anchors were created we can add them to the items
    for (Item *item : qAsConst(m_items)) {
        const int leftIndex = item->property("leftIndex").toInt();
        const int topIndex = item->property("topIndex").toInt();
        const int rightIndex = item->property("rightIndex").toInt();
        const int bottomIndex = item->property("bottomIndex").toInt();

        AnchorGroup &group = item->anchorGroup();
        group.left = m_anchors.at(leftIndex);
        group.top = m_anchors.at(topIndex);
        group.right = m_anchors.at(rightIndex);
        group.bottom = m_anchors.at(bottomIndex);

        // Clear helper properties
        item->setProperty("leftIndex", QVariant());
        item->setProperty("topIndex", QVariant());
        item->setProperty("rightIndex", QVariant());
        item->setProperty("bottomIndex", QVariant());
    }

    if (!m_items.isEmpty())
        Q_EMIT widgetCountChanged(m_items.size());


    // The main window that we're restoring can have more stuff now (other-toolbars etc), so by
    // having restored its geometry it can mean our dockwidget layout is now different, so update
    // its content size if needed
    Q_EMIT minimumSizeChanged(m_minSize);

    if (m_size != multiSplitter()->size()) {
        setSize(multiSplitter()->size());
    }

    return true;
}

LayoutSaver::MultiSplitterLayout MultiSplitterLayout::serialize() const
{
    LayoutSaver::MultiSplitterLayout l;

    l.size = size();
    l.minSize = minimumSize();

    for (Item *item : m_items)
        l.items.push_back(item->serialize());

    for (Anchor *anchor : m_anchors)
        l.anchors.push_back(anchor->serialize());

    return l;
}
