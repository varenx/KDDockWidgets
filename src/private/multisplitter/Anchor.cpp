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
#include "MultiSplitterLayout_p.h"
#include "MultiSplitter_p.h"
#include "Logging_p.h"
#include "LayoutSaver.h"
#include "Config.h"
#include "Separator_p.h"
#include "FrameworkWidgetFactory.h"

#include <QRubberBand>
#include <QApplication>
#include <QDebug>

#ifdef Q_OS_WIN
# include <Windows.h>
#endif

using namespace KDDockWidgets;

bool Anchor::s_isResizing = false;

Anchor::Anchor(Qt::Orientation orientation, MultiSplitterLayout *multiSplitter)
    : QObject(multiSplitter->multiSplitter())
    , m_orientation(orientation)
    , m_layout(multiSplitter)
    , m_separatorWidget(Config::self().frameworkWidgetFactory()->createSeparator(this, multiSplitter->multiSplitter()))
    , m_lazyResize(Config::self().flags() & Config::Flag_LazyResize)
    , m_lazyResizeRubberBand(m_lazyResize ? new QRubberBand(QRubberBand::Line, multiSplitter->multiSplitter()) : nullptr)
{
    connect(this, &QObject::objectNameChanged, m_separatorWidget, &QObject::setObjectName);
}

Anchor::~Anchor()
{
    m_separatorWidget->setEnabled(false);
    m_separatorWidget->deleteLater();
    qCDebug(multisplittercreation) << "~Anchor; this=" << this << "; m_to=" << m_to << "; m_from=" << m_from;
}

void Anchor::setFrom(Anchor *from)
{
    if (from->orientation() == orientation() || from == this) {
        qWarning() << "Anchor::setFrom: Invalid from" << from->orientation() << m_orientation
                   << from << this;
        return;
    }

    if (m_from)
        disconnect(m_from, &Anchor::positionChanged, this, &Anchor::updateSize);
    m_from = from;
    connect(from, &Anchor::positionChanged, this, &Anchor::updateSize);
    updateSize();

    Q_EMIT fromChanged();
}

void Anchor::setTo(Anchor *to)
{
    Q_ASSERT(to);
    if (to->orientation() == orientation() || to == this) {
        qWarning() << "Anchor::setFrom: Invalid to" << to->orientation() << m_orientation
                   << to << this;
        return;
    }

    if (m_to)
        disconnect(m_to, &Anchor::positionChanged, this, &Anchor::updateSize);
    m_to = to;
    connect(to, &Anchor::positionChanged, this, &Anchor::updateSize);
    updateSize();

    Q_EMIT toChanged();
}

void Anchor::updateSize()
{
    if (isValid()) {
        if (isVertical()) {
            setGeometry(QRect(position(), m_from->geometry().bottom() + 1, thickness(), length()));
        } else {
            setGeometry(QRect(m_from->geometry().right() + 1, position(), length(), thickness()));
        }
    }

    qCDebug(anchors) << "Anchor::updateSize" << this << geometry();
}

void Anchor::setGeometry(QRect r)
{
    if (r != m_geometry) {

        if (position() < 0) {
            qCDebug(anchors) << Q_FUNC_INFO << "Old position was negative" << position() << "; new=" << r;
        }

        m_geometry = r;
        m_separatorWidget->setGeometry(r);
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

void Anchor::setPosition(int)
{
}

int Anchor::position() const
{
    const QPoint topLeft = m_geometry.topLeft();
    return isVertical() ? topLeft.x() : topLeft.y();
}

void Anchor::setVisible(bool v)
{
    m_separatorWidget->setVisible(v);
    if (v) {
        m_separatorWidget->setGeometry(m_geometry);
    }
}

int Anchor::length() const
{
    Q_ASSERT(m_to);
    Q_ASSERT(m_from);
    return m_to->position() - m_from->position();
}

bool Anchor::isValid() const
{
    return m_to && m_from && m_to != m_from && m_to != this && m_from != this;
}

int Anchor::thickness() const
{
    return isVertical() ? m_separatorWidget->width()
                        : m_separatorWidget->height();
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

const ItemList Anchor::items(Side side) const
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

void Anchor::removeAllItems()
{
    removeItems(Side1);
    removeItems(Side2);
}

bool Anchor::isBeingDragged() const
{
    return m_layout->anchorBeingDragged() == this;
}

void Anchor::clear()
{
    m_side1Items.clear();
    m_side2Items.clear();
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
    m_layout->setAnchorBeingDragged(this);
    qCDebug(anchors) << "Drag started";

    if (m_lazyResize) {
        setLazyPosition(position());
        m_lazyResizeRubberBand->show();
    }
}

void Anchor::onMouseReleased()
{
    if (m_lazyResize) {
        m_lazyResizeRubberBand->hide();
        setPosition(m_lazyPosition);
    }

    s_isResizing = false;
    m_layout->setAnchorBeingDragged(nullptr);
}

void Anchor::onMouseMoved(QPoint)
{
    if (!isBeingDragged())
        return;

    if (!(qApp->mouseButtons() & Qt::LeftButton)) {
        qCDebug(mouseevents) << Q_FUNC_INFO << "Ignoring spurious mouse event. Someone ate our ReleaseEvent";
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

void Anchor::onWidgetMoved(int p)
{
    if (m_layout->anchorBeingDragged() != this) // We only care if it's being dragged by mouse
        return;


    setPosition(p);
}

bool Anchor::isResizing()
{
    return s_isResizing;
}
