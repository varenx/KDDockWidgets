/*
  This file is part of KDDockWidgets.

  Copyright (C) 2020 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
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

#pragma once

#include <QObject>
#include <QVector>
#include <QRect>
#include <QVariant>
#include <QWidget> // TODO: remove
#include <QDebug>

#include <memory>

#define KDDOCKWIDGETS_MIN_WIDTH 80
#define KDDOCKWIDGETS_MIN_HEIGHT 90

class TestMultiSplitter;

namespace Layouting {

class ItemContainer;
class Item;
class Anchor;

enum Location {
    Location_None,
    Location_OnLeft, ///> Left docking location
    Location_OnTop,  ///> Top docking location
    Location_OnRight, ///> Right docking location
    Location_OnBottom ///> Bottom docking location
};

///@internal
inline Location oppositeLocation(Location loc)
{
    switch (loc) {
    case Location_OnLeft:
        return Location_OnRight;
    case Location_OnTop:
        return Location_OnBottom;
    case Location_OnRight:
        return Location_OnLeft;
    case Location_OnBottom:
        return Location_OnTop;
    default:
        Q_ASSERT(false);
        return Location_None;
    }
}

///@internal
inline Location adjacentLocation(Location loc)
{
    switch (loc) {
    case Location_OnLeft:
        return Location_OnTop;
    case Location_OnTop:
        return Location_OnRight;
    case Location_OnRight:
        return Location_OnBottom;
    case Location_OnBottom:
        return Location_OnLeft;
    default:
        Q_ASSERT(false);
        return Location_None;
    }
}

enum Side {
    Side1,
    Side2
};

enum class GrowthStrategy {
    BothSidesEqually
};

inline Qt::Orientation oppositeOrientation(Qt::Orientation o) {
    return o == Qt::Vertical ? Qt::Horizontal
                             : Qt::Vertical;
}

inline int pos(QPoint p, Qt::Orientation o) {
    return o == Qt::Vertical ? p.y()
                             : p.x();
}

inline int length(QSize sz, Qt::Orientation o) {
    return o == Qt::Vertical ? sz.height()
                             : sz.width();
}

inline bool locationIsVertical(Location loc)
{
    return loc == Location_OnTop || loc == Location_OnBottom;
}

inline bool locationIsHorizontal(Location loc)
{
    return !locationIsVertical(loc);
}

inline bool locationIsSide1(Location loc)
{
    return loc == Location_OnLeft || loc == Location_OnTop;
}

inline bool locationIsSide2(Location loc)
{
    return loc == Location_OnRight || loc == Location_OnBottom;
}

inline QRect adjustedRect(QRect r, Qt::Orientation o, int p1, int p2)
{
    if (o == Qt::Vertical) {
        r.adjust(0, p1, 0, p2);
    } else {
        r.adjust(p1, 0, p2, 0);
    }

    return r;
}

inline Qt::Orientation orientationForLocation(Location loc)
{
    switch (loc) {
    case Location_OnLeft:
    case Location_OnRight:
        return Qt::Horizontal;
    case Location_None:
    case Location_OnTop:
    case Location_OnBottom:
        return Qt::Vertical;
    }

    return Qt::Vertical;
}

inline Side sideForLocation(Location loc)
{
    switch (loc) {
    case Location_OnLeft:
    case Location_OnTop:
        return Side::Side1;
    case Location_OnRight:
    case Location_OnBottom:
        return Side::Side2;
    default:
        return Side::Side1;
    }
}

struct SizingInfo {
    QSize size() const {
        return geometry.size();
    }

    int length(Qt::Orientation o) const {
        return Layouting::length(size(), o);
    }

    int minLength(Qt::Orientation o) const {
        return Layouting::length(minSize, o);
    }

    int availableLength(Qt::Orientation o) const {
        return qMax(0, length(o) - minLength(o));
    }

    int missingLength(Qt::Orientation o) const {
        return qMax(0,  minLength(o) - length(o));
    }

    QPoint pos() const {
        return geometry.topLeft();
    }

    int position(Qt::Orientation o) const {
        return Layouting::pos(pos(), o);
    }

    int edge(Qt::Orientation o) const {
        return o == Qt::Vertical ? geometry.bottom()
                                 : geometry.right();
    }

    void setLength(int l, Qt::Orientation o) {
        if (o == Qt::Vertical) {
            geometry.setHeight(l);
        } else {
            geometry.setWidth(l);
        }
    }

    void setPos(int p, Qt::Orientation o) {
        if (o == Qt::Vertical)
            geometry.moveTop(p);
        else
            geometry.moveLeft(p);
    }

    bool isNull() const {
        return geometry.isNull();
    }

    void setGeometry(QRect geo) {
        geometry = geo;
    }

    typedef QVector<SizingInfo> List;
    QRect geometry;
    QSize minSize = QSize(40, 40); // TODO: Hardcoded
    QSize maxSize = QSize(16777215, 16777215); // TODO: Not supported yet
    QSize proposedSize;
    double percentageWithinParent = 0.0;
    bool isBeingInserted = false;
};

class GuestInterface
{
public:
    virtual void setLayoutItem(Item *) = 0;
    virtual QWidget *asWidget() = 0;
};

class Item : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int x READ x NOTIFY xChanged)
    Q_PROPERTY(int y READ y NOTIFY yChanged)
    Q_PROPERTY(int width READ width NOTIFY widthChanged)
    Q_PROPERTY(int height READ height NOTIFY heightChanged)
    Q_PROPERTY(QRect geometry READ geometry NOTIFY geometryChanged)
    Q_PROPERTY(bool isContainer READ isContainer CONSTANT)
public:
    typedef QVector<Item*> List;

    explicit Item(QWidget *hostWidget, ItemContainer *parent = nullptr);
    ~Item() override;

    bool isRoot() const;
    virtual int visibleCount_recursive() const;

    virtual void insertItem(Item *item, Location);

    /**
     * @brief No widget can have a minimum size smaller than this, regardless of their minimum size.
     */
    static QSize hardcodedMinimumSize();

    int x() const;
    int y() const;
    int width() const;
    int height() const;
    QSize size() const;
    void setSize(QSize);
    QPoint pos() const;
    int pos(Qt::Orientation) const;
    QRect geometry() const;

    bool isContainer() const;
    bool isWidget() const { return !isContainer(); }

    Qt::Orientation orientation() const;
    static int separatorThickness();
    virtual bool checkSanity();
    void setParentContainer(ItemContainer *parent); // TODO: Make private
    ItemContainer *parentContainer() const;
    void setPos(QPoint); // TODO: Make private
    void setPos(int pos, Qt::Orientation);
    int position(Qt::Orientation) const;
    const ItemContainer *asContainer() const;
    ItemContainer *asContainer();
    void setMinSize(QSize);
    void setMaxSize(QSize);
    virtual QSize minSize() const;
    virtual QSize maxSize() const;
    virtual void resize(QSize newSize);
    int minLength(Qt::Orientation) const;
    void setLength(int length, Qt::Orientation);
    virtual void setLength_recursive(int length, Qt::Orientation);
    int length(Qt::Orientation) const;
    int availableLength(Qt::Orientation) const;
    bool isPlaceholder() const;

    virtual bool isVisible() const;
    virtual void setIsVisible(bool);
    virtual void setGeometry_recursive(QRect rect);
    Item* neighbour(Side) const;

    virtual void dumpLayout(int level = 0);
    void setGeometry(QRect rect);
    SizingInfo m_sizingInfo;
    QSize missingSize() const;
    int missingLength(Qt::Orientation) const;
    bool isBeingInserted() const;
    void setBeingInserted(bool);
    ItemContainer *root() const;
    void deserialize() {}
    void serialize() {}
    QRect mapToRoot(QRect) const;
    QPoint mapToRoot(QPoint) const;
    QPoint mapFromRoot(QPoint) const;
    QRect mapFromRoot(QRect) const;
    QPoint mapFromParent(QPoint) const;

    QWidget *frame() const { return m_guest ? m_guest->asWidget() : nullptr; } // TODO: rename
    GuestInterface *guest() const { return m_guest; }
    void setFrame(GuestInterface *);
    QWidget *window() const {
        return m_guest ? frame()->window() : nullptr;
    }

    void ref();
    void unref();
    int refCount() const;

    QWidget *hostWidget() const;
    void restore(GuestInterface *guest);
    virtual void setHostWidget(QWidget *);
    virtual void updateWidgetGeometries();

Q_SIGNALS:
    void geometryChanged();
    void xChanged();
    void yChanged();
    void widthChanged();
    void heightChanged();
    void visibleChanged(Item *thisItem, bool visible);
    void minSizeChanged(Item *thisItem);
protected:
    friend class ::TestMultiSplitter;
    explicit Item(bool isContainer, QWidget *hostWidget, ItemContainer *parent);
    const bool m_isContainer;
    ItemContainer *m_parent = nullptr;
private Q_SLOTS:
    void onWidgetLayoutRequested();

private:
    void turnIntoPlaceholder();
    bool eventFilter(QObject *o, QEvent *event) override;
    int m_refCount = 0;
    void updateObjectName();
    void onWidgetDestroyed();
    bool m_isVisible = false;
    bool m_destroying = false; // TODO: Remove and check if unit-tests pass
    QWidget * m_hostWidget = nullptr;
    GuestInterface *m_guest = nullptr;
};

class ItemContainer : public Item {
    Q_OBJECT
    Q_PROPERTY(QVariantList items READ items NOTIFY itemsChanged)
public:

    struct LengthOnSide {
        int length = 0;
        int minLength = 0;

        int available() const {
            return qMax(0, length - minLength);
        }

        int missing() const {
            return qMax(0, minLength - length);
        }
    };

    explicit ItemContainer(QWidget *hostWidget, ItemContainer *parent);
    explicit ItemContainer(QWidget *parent);
    void insertItem(Item *item, int index, bool growItem = true);
    bool checkSanity() override;
    void scheduleCheckSanity() const;
    bool hasOrientation() const;
    int numChildren() const;
    int numVisibleChildren() const;
    bool hasChildren() const;
    bool hasVisibleChildren() const;
    int indexOfChild(const Item *) const;
    int indexOfVisibleChild(const Item *) const;
    void removeItem(Item *, bool hardRemove = true);
    bool isEmpty() const;
    void setGeometry_recursive(QRect rect) override;

    ItemContainer *convertChildToContainer(Item *leaf);
    void insertItem(Item *item, Location) override;
    bool hasOrientationFor(Location) const;
    Item::List children() const;
    Item::List visibleChildren(bool includeBeingInserted = false) const;
    int usableLength() const;
    bool hasSingleVisibleItem() const;
    bool contains(const Item *item) const;
    bool contains_recursive(const Item *item) const;
    void setChildren(const Item::List children, Qt::Orientation o);
    void updateMinSize();
    void setOrientation(Qt::Orientation);
    QSize minSize() const override;
    QSize maxSize() const override;
    void resize(QSize newSize) override;
    int length() const;
    QRect rect() const;
    QVariantList items() const;
    void dumpLayout(int level = 0) override;
    void updateChildPercentages();
    void restoreChild(Item *);
    void updateWidgetGeometries() override;

    ///@brief Grows the side1Neighbour to the right and the side2Neighbour to the left
    ///So they occupy the empty space that's between them (or bottom/top if Qt::Vertical).
    ///This is useful when an Item is removed. Its neighbours will occupy its space.
    ///side1Neighbour or side2Neighbour are allowed to be null, in which case the non-null one
    ///will occupy the entire space.
    void growNeighbours(Item *side1Neighbour, Item *side2Neighbour);

    ///@brief grows an item by @p amount. It calculates how much to grow on side1 and on side2
    ///Then calls growItem(item, side1Growth, side2Growth) which will effectively grow it,
    ///and shrink the neighbours which are donating the size.
    void growItem(Item *, int amount, GrowthStrategy);
    void growItem(int index, SizingInfo::List &sizes, int amount, GrowthStrategy);

    ///@brief Grows an item by @p side1Growth on the left and @p side2Growth on the right
    ///(or top/bottom if Qt::Vertical). Squeezes the neighbours (not just the immediate ones).
    ///at the end positions all items.
    void growItem(int index, SizingInfo::List &sizes, int side1Growth, int side2Growth);

    Item *neighbourFor(const Item *, Side) const;
    Item *visibleNeighbourFor(const Item *item, Side side) const;
    QSize availableSize() const;
    int availableLength() const;
    LengthOnSide lengthOnSide(const SizingInfo::List &sizes, int fromIndex, Side, Qt::Orientation) const;
    int minLength(int fromIndex, Side, Qt::Orientation) const;
    int neighboursLengthFor(const Item *item, Side, Qt::Orientation) const;
    int neighboursLengthFor_recursive(const Item *item, Side, Qt::Orientation) const;
    int neighboursMinLengthFor(const Item *item, Side, Qt::Orientation) const;
    int neighboursMinLengthFor_recursive(const Item *item, Side, Qt::Orientation) const;
    int neighbourSeparatorWaste(const Item *item, Side, Qt::Orientation) const;
    int neighbourSeparatorWaste_recursive(const Item *item, Side, Qt::Orientation) const;
    int availableOnSide(Item *child, Side) const;
    QSize missingSizeFor(Item *item, Qt::Orientation) const;
    void onChildMinSizeChanged(Item *child);
    void onChildVisibleChanged(Item *child, bool visible);
    SizingInfo::List sizingInfosPerNeighbour(Item *item, Side) const;
    SizingInfo::List sizes(bool ignoreBeingInserted = false) const;
    QVector<int> calculateSqueezes(SizingInfo::List::ConstIterator begin, SizingInfo::List::ConstIterator end, int needed) const;
    QRect suggestedDropRect(QSize minSize, const Item *relativeTo, Location) const;
    void positionItems();
    void positionItems(SizingInfo::List &sizes);
    bool isResizing() const { return m_isResizing; }
    void clear();
    Item* itemForFrame(const QWidget *w) const; // TODO: Rename
    int visibleCount_recursive() const override;
    int count_recursive() const;
    Item *itemAt(QPoint p) const;
    Item *itemAt_recursive(QPoint p) const;
    Item::List items_recursive() const;
    void setHostWidget(QWidget *) override;
    void setIsVisible(bool) override;
    bool isVisible() const override;
    void setLength_recursive(int length, Qt::Orientation) override;
    void applySizes(const SizingInfo::List &sizes);
    void applyPositions(const SizingInfo::List &sizes);
    Qt::Orientation orientation() const;
    bool isVertical() const;
    bool isHorizontal() const;
Q_SIGNALS:
    void itemsChanged();
    void numVisibleItemsChanged(int);
    void numItemsChanged();
public:
    Item::List m_children;
    bool m_isResizing = false;
    bool m_blockUpdatePercentages = false;
    QVector<Layouting::Anchor*> separators() const;
    Qt::Orientation m_orientation = Qt::Vertical;
private:
    void createSeparators();
    QVector<double> childPercentages() const;
    mutable bool m_checkSanityScheduled = false;
    QVector<Layouting::Anchor*> m_separators;
    bool m_convertingItemToContainer = false;
};

/**
 * Returns the widget's min size
 */
inline QSize widgetMinSize(const QWidget *w)
{
    const int minW = w->minimumWidth() > 0 ? w->minimumWidth()
                                           : w->minimumSizeHint().width();

    const int minH = w->minimumHeight() > 0 ? w->minimumHeight()
                                            : w->minimumSizeHint().height();

    return QSize(minW, minH).expandedTo(Item::hardcodedMinimumSize());
}

inline int widgetMinLength(const QWidget *w, Qt::Orientation o) {
    return length(widgetMinSize(w), o);
}

}
