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

#include <memory>

class TestMultiSplitter;

namespace Layouting {

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

enum class SizingOption {
    Calculate,
    UseProvided
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

struct SizingInfo {
    QSize minSize = QSize(40, 40); // TODO: Hardcoded
    QSize maxSize = QSize(16777215, 16777215); // TODO: Not supported yet
    QSize proposedSize;
    bool isBeingInserted = false;
};

class ItemContainer;

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

    explicit Item(ItemContainer *parent = nullptr);

    bool isRoot() const;

    bool isVertical() const;
    bool isHorizontal() const;
    virtual int visibleCount_recursive() const;

    virtual void insertItem(Item *item, Location, SizingOption = SizingOption::Calculate);

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
    virtual bool checkSanity() const;
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
    int length(Qt::Orientation) const;
    int availableLength(Qt::Orientation) const;
    bool isPlaceholder() const;

    void ref() {}
    void unref() {}

    bool isVisible() const;
    void setIsVisible(bool);
    virtual void setGeometry_recursive(QRect rect);
    Item* neighbour(Side) const;

    virtual void dumpLayout(int level = 0);
    void setGeometry(QRect rect);
    SizingInfo m_sizingInfo;
    int availableOnSide(Side, Qt::Orientation) const;
    QSize missingSize() const;
    bool isBeingInserted() const;
    void setBeingInserted(bool);
    ItemContainer *root() const;
    void deserialize() {}
    void serialize() {}
    QRect mapToRoot(QRect) const;
    QPoint mapToRoot(QPoint) const;
    QPoint mapFromParent(QPoint) const;

    QWidget *frame() const { return m_widget; } // TODO: rename
    void setFrame(QWidget *w) { m_widget = w; } // TODO rename
    QWidget *window() const {
        return m_widget ? m_widget->window()
                        : nullptr;
    }

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
    explicit Item(bool isContainer, ItemContainer *parent);
    const bool m_isContainer;
    Qt::Orientation m_orientation = Qt::Vertical;

    ItemContainer *m_parent = nullptr;
    QRect m_geometry;
private:
    bool m_isVisible = false;
    QWidget *m_widget = nullptr; // TODO: Make generic
};

class ItemContainer : public Item {
    Q_OBJECT
    Q_PROPERTY(QVariantList items READ items NOTIFY itemsChanged)
public:
    explicit ItemContainer(ItemContainer *parent = nullptr);
    void insertItem(Item *item, int index, bool growItem = true);
    bool checkSanity() const override;
    bool hasOrientation() const;
    int numChildren() const;
    int numVisibleChildren() const;
    bool hasChildren() const;
    bool hasVisibleChildren() const;
    int indexOfChild(const Item *) const;
    void removeItem(Item *);
    bool isEmpty() const;
    void setGeometry_recursive(QRect rect) override;

    ItemContainer *convertChildToContainer(Item *leaf);
    void insertItem(Item *item, Location, SizingOption = SizingOption::Calculate) override;
    bool hasOrientationFor(Location) const;
    Item::List children() const;
    Item::List visibleChildren() const;
    int usableLength() const;
    bool hasSingleVisibleItem() const;
    bool contains(const Item *item) const;
    bool contains_recursive(const Item *item) const;
    void setChildren(const Item::List children);
    QSize minSize() const override;
    QSize maxSize() const override;
    void resize(QSize newSize) override;
    int length() const;
    QRect rect() const;
    QVariantList items() const;
    void dumpLayout(int level = 0);
    void updateChildPercentages();
    void restorePlaceholder(Item *);
    void growNeighbours(Item *side1Neighbour, Item *side2Neighbour);
    void growItem(Item *, int amount, GrowthStrategy);
    void growItem(Item *, int side1Growth, int side2Growth);
    Item *neighbourFor(const Item *, Side) const;
    Item *visibleNeighbourFor(const Item *item, Side side) const;
    QSize availableSize() const;
    int availableLength() const;
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
    QVector<int> availableLengthPerNeighbour(Item *item, Side) const;
    static QVector<int> calculateSqueezes(QVector<int> availabilities, int needed);
    QRect suggestedDropRect(Item *newItem, Item *relativeTo, Location) const;
    void positionItems();
    bool isResizing() const { return m_isResizing; }
    void clear();
    Item* itemForFrame(const QWidget *w) const; // TODO: Rename
    int visibleCount_recursive() const override;
    int count_recursive() const;
    Item *itemAt(QPoint p) const;
    Item *itemAt_recursive(QPoint p) const;
    Item::List items_recursive() const;

Q_SIGNALS:
    void itemsChanged();
public:
    QVector<qreal> m_childPercentages;
    Item::List m_children;
    bool m_isResizing = false;
};

}
