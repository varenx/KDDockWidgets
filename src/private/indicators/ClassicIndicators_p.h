/*
  This file is part of KDDockWidgets.

  SPDX-FileCopyrightText: 2019-2020 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.com>
  Author: Sérgio Martins <sergio.martins@kdab.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only

  Contact KDAB at <info@kdab.com> for commercial licensing options.
*/

#ifndef KD_INDICATORS_CLASSICINDICATORS_P_H
#define KD_INDICATORS_CLASSICINDICATORS_P_H

#include "DropIndicatorOverlayInterface_p.h"

namespace KDDockWidgets {

class IndicatorWindow;
class Indicator;

class ClassicIndicators : public DropIndicatorOverlayInterface
{
    Q_OBJECT
public:
    explicit ClassicIndicators(DropArea *dropArea);
    ~ClassicIndicators() override;
    Type indicatorType() const override;
    void hover(QPoint globalPos) override;
    QPoint posForIndicator(DropLocation) const override;
protected:
    void showEvent(QShowEvent *) override;
    void hideEvent(QHideEvent *) override;
    void resizeEvent(QResizeEvent *) override;
    void updateVisibility() override;
private:
    friend class KDDockWidgets::Indicator;
    friend class KDDockWidgets::IndicatorWindow;
    void raiseIndicators();
    void setDropLocation(DropLocation);
    QRect geometryForRubberband(QRect localRect) const;
    bool rubberBandIsTopLevel() const;

    QWidgetOrQuick *const m_rubberBand;
    IndicatorWindow *const m_indicatorWindow;
};

}

#endif
