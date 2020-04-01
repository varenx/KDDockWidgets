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

#ifndef KD_MULTISPLITTER_ANCHOR_P_H
#define KD_MULTISPLITTER_ANCHOR_P_H

#include "docks_export.h"
#include "LayoutSaver_p.h"
#include "Item_p.h"

#include <QObject>
#include <QPointer>
#include <QRect>
#include <QVector>

QT_BEGIN_NAMESPACE
class QRubberBand;
QT_END_NAMESPACE

namespace KDDockWidgets {

class Item;
class MultiSplitterLayout;
class Separator;

typedef QVector<Item*> ItemList;

/**
 * @brief An anchor is the vertical or horizontal (@ref orientation()) line that has an handle
 * so you can resize widgets with your mouse.
 *
 * A MultiSplitter comes with 4 static anchors (@ref isStatic()), that represent the top, left, right
 * and bottom borders. A static anchor means it can't change position, doesn't display the handle and
 * will have the same lifetime has the MultiSplitter.
 *
 * Each anchor has two properties indicating in which anchor it starts and where it ends, @ref from(), to().
 * For example, the top static horizontal anchor starts at the left anchor and ends at the right static anchor.
 * If this anchor is vertical, then from()/to() return horizontal anchors, and vice-versa.
 *
 * An anchor has a length, which is to()->pos() - from()->pos(). The length of a vertical anchor is,
 * thus, its vertical extent (Likewise for horizontal anchors).
 *
 * An anchor controls two groups of widgets: side1 and side2 widgets. When an anchor is dragged with mouse
 * it will resize those widgets. The widgets always start or end at the position where the anchor lives.
 * For vertical anchors, side1 means "the widgets at its left" and side2 means "the widgets at its right",
 * Same principle for horizontal anchors, but for top/bottom instead.
 * Static anchors only have 1 side with widgets. For example the left static anchor only has widgets at its
 * right, so side1Widgets is empty.
 * Non-static anchors, always have side1 and side2 widgets. If not then they are considered unneeded
 * and are deleted.
 *
 * Example:
 *
 * +--------------------+
 * |          |         |
 * |          |         |
 * |          |         |
 * | Foo      |   Bar   |
 * |          |         |
 * |          |         |
 * +--------------------+
 *
 * In the above example we have 5 anchors. 4 of them are static (left, right, top, bottom) and there's
 * a non-static one, in the middle. It's vertical, and can be dragged left and right, resizing its
 * side1Widgets (Foo) and side2Widgets (Bar). This non-static anchors has from=top anchor, and to=bottom anchor.
 *
 */
class DOCKS_EXPORT_FOR_UNIT_TESTS Anchor : public QObject // clazy:exclude=ctor-missing-parent-argument
{
    Q_OBJECT

    // properties for GammaRay
    Q_PROPERTY(KDDockWidgets::ItemList side1Items READ side1Items NOTIFY itemsChanged)
    Q_PROPERTY(KDDockWidgets::ItemList side2Items READ side2Items NOTIFY itemsChanged)

    Q_PROPERTY(QString debug_side1ItemNames READ debug_side1ItemNames NOTIFY debug_itemNamesChanged)
    Q_PROPERTY(QString debug_side2ItemNames READ debug_side2ItemNames NOTIFY debug_itemNamesChanged)

    Q_PROPERTY(Anchor* from READ from WRITE setFrom NOTIFY fromChanged)
    Q_PROPERTY(Anchor* to READ to WRITE setTo NOTIFY toChanged)
    Q_PROPERTY(int position READ position WRITE setPosition NOTIFY positionChanged)
    Q_PROPERTY(Qt::Orientation orientation READ orientation CONSTANT)
public:

    typedef QVector<Anchor *> List;
    explicit Anchor(Qt::Orientation orientation, MultiSplitterLayout *multiSplitter);
    ~Anchor() override;

    void setFrom(Anchor *);
    Anchor *from() const { return m_from; }
    Anchor *to() const { return m_to; }
    void setTo(Anchor *);
    Qt::Orientation orientation() const;
    void addItem(Item *, Side);
    void addItems(const ItemList &list, Side);
    void removeItem(Item *w);
    void removeItems(Side);
    bool isVertical() const { return m_orientation == Qt::Vertical; }
    void setPosition(int);
    void updatePositionPercentage();
    int position() const;

    void setVisible(bool);

    /**
     * @brief Sets the new layout. Called when we're dropping a source layout into a target one.
     * The target one will steal the separators of the source one.
     */
    void setLayout(MultiSplitterLayout *);

    ///@brief returns the separator widget
    Separator* separatorWidget() const;

    /**
     * @brief The length of this anchor. The distance between @ref from and @ref to.
     * @return the anchor's length
     */
    int length() const;

    /**
     * @brief Checks if this anchor is valid. It's valid if @ref from and @ref to are non-null, and not the same.
     * @return true if this anchor is valid.
     */
    bool isValid() const;

    /**
     * @brief The width of a vertical anchor, or height of an horizontal anchor.
     */
    int thickness() const;

    bool isEmpty() const { return !hasItems(Side1) && !hasItems(Side2); }
    bool hasItems(Side) const;

    bool containsItem(const Item *w, Side side) const;

    const ItemList items(Side side) const;
    const ItemList side1Items() const { return m_side1Items; }
    const ItemList side2Items() const { return m_side2Items; }

    void removeAllItems();

    void setPositionOffset(int);
    bool isBeingDragged() const;

    ///@brief removes the side1 and side2 items. Doesn't delete them
    void clear();

    void onMousePress();
    void onMouseReleased();
    void onMouseMoved(QPoint pt);
    void onWidgetMoved(int p);

    ///@brief Returns whether we're dragging a separator. Can be useful for the app to stop other work while we're not in the final size
    static bool isResizing();

private:
    void setLazyPosition(int);
Q_SIGNALS:
    void positionChanged(int pos);
    void itemsChanged(Side);
    void fromChanged();
    void toChanged();
    void debug_itemNamesChanged();

public:
    int position(QPoint) const;
    void updateSize();
    void updateItemSizes();
    void debug_updateItemNames();
    QString debug_side1ItemNames() const;
    QString debug_side2ItemNames() const;
    void setGeometry(QRect);
    QRect geometry() const { return m_geometry; }

    const Qt::Orientation m_orientation;
    ItemList m_side1Items;
    ItemList m_side2Items;
    QPointer<Anchor> m_from;// QPointer just so we can assert. They should never be null.
    QPointer<Anchor> m_to;

    // Only set when anchor is moved through mouse. Side1 if going towards left or top, Side2 otherwise.
    Side m_lastMoveDirection = Side1;

    MultiSplitterLayout *m_layout = nullptr;
    bool m_showingSide1Rubberband = false;
    bool m_showingSide2Rubberband = false;
    bool m_initialized = false;
    static bool s_isResizing;

    QString m_debug_side1ItemNames;
    QString m_debug_side2ItemNames;
    Separator *const m_separatorWidget;
    QRect m_geometry;
    const bool m_lazyResize;
    int m_lazyPosition = 0;
    QRubberBand *const m_lazyResizeRubberBand;
};

}

Q_DECLARE_METATYPE(KDDockWidgets::ItemList)
Q_DECLARE_METATYPE(KDDockWidgets::Item*)

#endif
