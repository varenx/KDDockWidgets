/*
  This file is part of KDDockWidgets.

  SPDX-FileCopyrightText: 2019-2020 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.com>
  Author: Sérgio Martins <sergio.martins@kdab.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only

  Contact KDAB at <info@kdab.com> for commercial licensing options.
*/

#ifndef KD_INDICATORS_CLASSICINDICATORS_WINDOW_P_H
#define KD_INDICATORS_CLASSICINDICATORS_WINDOW_P_H

#include "DropIndicatorOverlayInterface_p.h"

#include <QImage>
#include <QWidget>
#include <QResizeEvent>

namespace KDDockWidgets {

class Indicator;
class ClassicIndicators;

class IndicatorWindow : public QWidget
{
    Q_OBJECT
public:
    explicit IndicatorWindow(ClassicIndicators *classicIndicators, QWidget * = nullptr);
    void hover(QPoint globalPos);
    void updatePositions();
    void updateIndicatorVisibility(bool visible);
    QPoint posForIndicator(DropIndicatorOverlayInterface::DropLocation) const;
private:
    void updatePosition();
    void resizeEvent(QResizeEvent *ev) override;
    bool event(QEvent *e) override;

    // When the compositor doesn't support translucency, we use a mask instead
    // Only happens on Linux
    void updateMask();

    Indicator *indicatorForLocation(DropIndicatorOverlayInterface::DropLocation loc) const;

    ClassicIndicators *const classicIndicators;
    Indicator *const m_center;
    Indicator *const m_left;
    Indicator *const m_right;
    Indicator *const m_bottom;
    Indicator *const m_top;
    Indicator *const m_outterLeft;
    Indicator *const m_outterRight;
    Indicator *const m_outterBottom;
    Indicator *const m_outterTop;
    QVector<Indicator *> m_indicators;
};

class Indicator : public QWidget
{
    Q_OBJECT
public:
    typedef QList<Indicator *> List;
    explicit Indicator(ClassicIndicators *classicIndicators, IndicatorWindow *parent,
                       DropIndicatorOverlayInterface::DropLocation location);
    void paintEvent(QPaintEvent *) override;

    void setHovered(bool hovered);
    QString iconName(bool active) const;
    QString iconFileName(bool active) const;

    QImage m_image;
    QImage m_imageActive;
    ClassicIndicators *const q;
    bool m_hovered = false;
    const DropIndicatorOverlayInterface::DropLocation m_dropLocation;
};
}

#endif
