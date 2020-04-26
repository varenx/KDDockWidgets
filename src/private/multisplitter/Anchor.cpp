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

#include "Anchor_p.h"
#include "Logging_p.h"
#include "Separator_p.h"

#include <QRubberBand>
#include <QApplication>
#include <QDebug>

#ifdef Q_OS_WIN
# include <Windows.h>
#endif

using namespace KDDockWidgets;
using namespace Layouting;

typedef QWidget QWidgetAdapter ; // TODO

bool Anchor::s_isResizing = false;

Anchor::Anchor(Qt::Orientation orientation, Options options, QWidget *hostWidget)
    : QObject(hostWidget)
    , m_orientation(orientation)
    , m_hostWidget(hostWidget)
    , m_separatorWidget(/*Config::self().frameworkWidgetFactory()->createSeparator(this, m_hostWidget)*/ new Separator(this, m_hostWidget)) // TODO
    , m_options(options)
    , m_lazyResizeRubberBand((options & Option::LazyResize) ? new QRubberBand(QRubberBand::Line, hostWidget) : nullptr)
{
    connect(this, &QObject::objectNameChanged, m_separatorWidget, &QObject::setObjectName);
}

Anchor::~Anchor()
{
    m_separatorWidget->setEnabled(false);
    m_separatorWidget->deleteLater();
}

void Anchor::setGeometry(QRect r)
{
    if (r != m_geometry) {
        if (position() < 0) {
            qCDebug(separators) << Q_FUNC_INFO << "Old position was negative" << position() << "; new=" << r;
        }

        m_geometry = r;
        m_separatorWidget->setGeometry(r);
        m_separatorWidget->setVisible(true);
        Q_EMIT geometryChanged(r);
    }
}

void Anchor::debug_updateItemNames()
{
    // I call this in the unit-tests, when running them on gammaray

    m_debug_side1ItemNames.clear();
    m_debug_side2ItemNames.clear();

    for (Item *item : qAsConst(m_side1Items))
        m_debug_side1ItemNames += item->objectName() + QStringLiteral("; ");

    for (Item *item : qAsConst(m_side2Items))
        m_debug_side2ItemNames += item->objectName() + QStringLiteral("; ");

    Q_EMIT debug_itemNamesChanged();
}

QString Anchor::debug_side1ItemNames() const
{
    return m_debug_side1ItemNames;
}

QString Anchor::debug_side2ItemNames() const
{
    return m_debug_side2ItemNames;
}

Qt::Orientation Anchor::orientation() const
{
    return m_orientation;
}

void Anchor::setGeometry(int pos, int length)
{
    QRect newGeo = m_geometry;
    if (isVertical()) {
        // The separator itself is horizontal
        newGeo.setSize(QSize(Item::separatorThickness(), length));
        newGeo.moveTo(0, pos);
    } else {
        // The separator itself is vertical
        newGeo.setSize(QSize(length, Item::separatorThickness()));
        newGeo.moveTo(pos, 0);
    }

    setGeometry(newGeo);
}

int Anchor::position() const
{
    const QPoint topLeft = m_geometry.topLeft();
    return isVertical() ? topLeft.y() : topLeft.x();
}

bool Anchor::containsItem(const Item *item, Side side) const
{
    switch (side) {
    case Side1:
        return m_side1Items.contains(const_cast<Item *>(item));
    case Side2:
        return m_side2Items.contains(const_cast<Item *>(item));
    default:
        Q_ASSERT(false);
        return false;
    }
}

const QVector<Item *> Anchor::items(Side side) const
{
    switch (side) {
    case Side1:
        return m_side1Items;
    case Side2:
        return m_side2Items;
    default:
        Q_ASSERT(false);
        return {};
    }
}

bool Anchor::isBeingDragged() const
{
    return false; // TODO
    //return m_layout->anchorBeingDragged() == this;
}

void Anchor::clear()
{
    m_side1Items.clear();
    m_side2Items.clear();
}

bool Anchor::lazyResizeEnabled() const
{
    return m_options & Option::LazyResize;
}

Separator *Anchor::separatorWidget() const
{
    return m_separatorWidget;
}

void Anchor::setLazyPosition(int pos)
{
    if (m_lazyPosition != pos) {
        m_lazyPosition = pos;

        QRect geo = m_separatorWidget->geometry();
        if (isVertical()) {
            geo.moveLeft(pos);
        } else {
            geo.moveTop(pos);
        }

        m_lazyResizeRubberBand->setGeometry(geo);
    }
}

int Anchor::position(QPoint p) const
{
    return isVertical() ? p.x() : p.y();
}

void Anchor::onMousePress()
{
    s_isResizing = true;
    //m_layout->setAnchorBeingDragged(this); TODO
    qCDebug(separators) << "Drag started";

    if (lazyResizeEnabled()) {
        setLazyPosition(position());
        m_lazyResizeRubberBand->show();
    }
}

void Anchor::onMouseReleased()
{
    if (m_lazyResizeRubberBand) {
        m_lazyResizeRubberBand->hide();
        //setPosition(m_lazyPosition); TODO
    }

    s_isResizing = false;
    // m_layout->setAnchorBeingDragged(nullptr); TODO
}

void Anchor::onMouseMoved(QPoint)
{
    if (!isBeingDragged())
        return;

    if (!(qApp->mouseButtons() & Qt::LeftButton)) {
        qCDebug(separators) << Q_FUNC_INFO << "Ignoring spurious mouse event. Someone ate our ReleaseEvent";
        onMouseReleased();
        return;
    }

#ifdef Q_OS_WIN
    // Try harder, Qt can be wrong, if mixed with MFC
    const bool mouseButtonIsReallyDown = (GetKeyState(VK_LBUTTON) & 0x8000) || (GetKeyState(VK_RBUTTON) & 0x8000);
    if (!mouseButtonIsReallyDown) {
        qCDebug(mouseevents) << Q_FUNC_INFO << "Ignoring spurious mouse event. Someone ate our ReleaseEvent";
        onMouseReleased();
        return;
    }
#endif

  //  const int positionToGoTo = position(pt);
    /*auto bounds = m_layout->boundPositionsForAnchor(this);

    if (positionToGoTo < bounds.first || positionToGoTo > bounds.second) {
        // qDebug() << "Out of bounds" << bounds.first << bounds.second << positionToGoTo << "; currentPos" << position() << "; window size" << window()->size();
        return;
    }

    m_lastMoveDirection = positionToGoTo < position() ? Side1
                                                      : (positionToGoTo > position() ? Side2
                                                                                     : Side_None); // Side_None shouldn't happen though.

    if (m_lazyResize)
        setLazyPosition(positionToGoTo);
    else
        setPosition(positionToGoTo);*/
}

void Anchor::onWidgetMoved(int )
{
   /* if (m_layout->anchorBeingDragged() != this) // We only care if it's being dragged by mouse
        return;*/ // TODO


  //  setPosition(p); TODO
}

bool Anchor::isResizing()
{
    return s_isResizing;
}
