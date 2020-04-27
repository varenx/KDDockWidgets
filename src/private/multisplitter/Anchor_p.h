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

#include "Item_p.h"

#include <QObject>
#include <QPointer>
#include <QRect>
#include <QVector>

QT_BEGIN_NAMESPACE
class QRubberBand;
QT_END_NAMESPACE

namespace KDDockWidgets {
class MultiSplitterLayout;
}

namespace Layouting {

class Item;

/**
 * @brief An anchor is the vertical or horizontal (@ref orientation()) line that has an handle
 * so you can resize widgets with your mouse.
 * *
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
class Anchor : public QObject // clazy:exclude=ctor-missing-parent-argument
{
    Q_OBJECT

    Q_PROPERTY(QString debug_side1ItemNames READ debug_side1ItemNames NOTIFY debug_itemNamesChanged)
    Q_PROPERTY(QString debug_side2ItemNames READ debug_side2ItemNames NOTIFY debug_itemNamesChanged)

    Q_PROPERTY(QRect geometry READ geometry NOTIFY geometryChanged)
    Q_PROPERTY(Qt::Orientation orientation READ orientation CONSTANT)
public:

    enum class Option {
        None = 0,
        LazyResize
    };
    Q_DECLARE_FLAGS(Options, Option);

    typedef QVector<Anchor *> List;
    explicit Anchor(Qt::Orientation orientation, Options options, QWidget *hostWidget);

    ~Anchor() override;

    QWidget *hostWidget() const;

    Qt::Orientation orientation() const;
    void addItem(Item *, Side);
    void addItems(const QVector<Item*> &list, Side);
    void removeItem(Item *w);
    bool isVertical() const { return m_orientation == Qt::Vertical; }
    void setGeometry(int pos, int pos2, int length);
    void updatePositionPercentage();
    int position() const;

    void setVisible(bool);

    ///@brief returns the separator widget
    Separator* separatorWidget() const;

    bool isEmpty() const { return !hasItems(Side1) && !hasItems(Side2); }
    bool hasItems(Side) const;

    bool containsItem(const Item *w, Side side) const;

    const QVector<Item*> items(Side side) const;

    void setPositionOffset(int);
    bool isBeingDragged() const;

    ///@brief removes the side1 and side2 items. Doesn't delete them
    void clear();

    bool lazyResizeEnabled() const;

    void onMousePress();
    void onMouseReleased();
    void onMouseMoved(QPoint pt);
    void onWidgetMoved(int p);

    ///@brief Returns whether we're dragging a separator. Can be useful for the app to stop other work while we're not in the final size
    static bool isResizing();
    static void setSeparatorFactoryFunc(SeparatorFactoryFunc);

private:
    void setLazyPosition(int);
    void setGeometry(QRect);
Q_SIGNALS:
    void geometryChanged(QRect);
    void itemsChanged(Side);
    void debug_itemNamesChanged();

public:
    int position(QPoint) const;
    void updateSize();
    void updateItemSizes();
    void debug_updateItemNames();
    QString debug_side1ItemNames() const;
    QString debug_side2ItemNames() const;
    QRect geometry() const { return m_geometry; }

    const Qt::Orientation m_orientation;
    QVector<Item*> m_side1Items;
    QVector<Item*> m_side2Items;

    // Only set when anchor is moved through mouse. Side1 if going towards left or top, Side2 otherwise.
    Layouting::Side m_lastMoveDirection = Side1;

    QWidget *const m_hostWidget;
    bool m_showingSide1Rubberband = false;
    bool m_showingSide2Rubberband = false;
    bool m_initialized = false;
    static bool s_isResizing;

    QString m_debug_side1ItemNames;
    QString m_debug_side2ItemNames;
    Separator *const m_separatorWidget;
    QRect m_geometry;
    int m_lazyPosition = 0;
    const Options m_options;
    QRubberBand *const m_lazyResizeRubberBand;
};

}

#endif
