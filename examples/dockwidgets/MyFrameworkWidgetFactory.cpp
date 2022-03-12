/*
  This file is part of KDDockWidgets.

  SPDX-FileCopyrightText: 2019-2022 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.com>
  Author: Sérgio Martins <sergio.martins@kdab.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only

  Contact KDAB at <info@kdab.com> for commercial licensing options.
*/

#include "MyFrameworkWidgetFactory.h"
#include "MyTitleBar_CSS.h"

#include <kddockwidgets/FrameworkWidgetFactory.h>

#include <kddockwidgets/private/TabWidget_p.h>
#include <kddockwidgets/private/FloatingWindow_p.h>
#include <kddockwidgets/private/widgets/FrameWidget_p.h>
#include <kddockwidgets/private/widgets/TabBarWidget_p.h>
#include <kddockwidgets/private/widgets/TabWidgetWidget_p.h>
#include <kddockwidgets/private/widgets/TitleBarWidget_p.h>
#include <kddockwidgets/private/multisplitter/Separator_qwidget.h>

#include <QApplication>

// clazy:excludeall=missing-qobject-macro,ctor-missing-parent-argument

class MyTitleBar : public KDDockWidgets::TitleBarWidget
{
public:
    bool m_isSpecialType = false;
    explicit MyTitleBar(KDDockWidgets::Frame *frame)
        : KDDockWidgets::TitleBarWidget(frame)
    {
        init();
    }

    explicit MyTitleBar(KDDockWidgets::FloatingWindow *fw)
        : KDDockWidgets::TitleBarWidget(fw)
    {
        init();
    }

    ~MyTitleBar() override;

    void init()
    {
        setFixedHeight(60);

        if (auto tb = tabBar()) {
            if (auto tbWidget = qobject_cast<KDDockWidgets::TabBarWidget *>(tb->asWidget())) {
                // 1.  Be notified when dock widgets are added/removed from the tabbar and when current changes

                connect(tbWidget, &KDDockWidgets::TabBarWidget::dockWidgetInserted, this, &MyTitleBar::updateType);
                connect(tbWidget, &KDDockWidgets::TabBarWidget::dockWidgetRemoved, this, &MyTitleBar::updateType);
                connect(tbWidget, &KDDockWidgets::TabBarWidget::currentChanged, this, &MyTitleBar::updateType);
            }
        }

        if (KDDockWidgets::FloatingWindow *fw = floatingWindow()) {
            // 2. Floating Windows with more than 1 Frame (more than 1 tabbar) won't have a special titlebar.
            // during runtime a FloatingWindow can have frames removed/added, so update its title bar type.
            connect(fw, &KDDockWidgets::FloatingWindow::numFramesChanged, this, &MyTitleBar::updateType);
        }
    }

    void updateType()
    {
        m_isSpecialType = false;

        if (tabBar() != nullptr) { // Will be null for floating windows with several frames.
            const auto dws = dockWidgets();
            for (auto dw : dws) {
                // 3. If this TitleBar contains dock widget #1 or #2 it's special and will have custom painting
                if (dw->uniqueName() == QLatin1String("DockWidget #1") ||
                    dw->uniqueName() == QLatin1String("DockWidget #2")) {
                    m_isSpecialType = true;
                    break;
                }
            }

            QWidget::update();
        }
    }

    void paintEvent(QPaintEvent *ev) override
    {
        KDDockWidgets::DockWidgetBase *currentDW = nullptr;
        if (auto tb = tabBar()) {
            if (auto tbWidget = qobject_cast<KDDockWidgets::TabBarWidget *>(tb->asWidget())) {
                // 4. Know the current tab, so it can influence the title bar
                currentDW = tbWidget->currentDockWidget();
            }
        }

        if (m_isSpecialType) {
            QPainter p(this);
            QPen pen(Qt::black);
            const bool isDw2 = currentDW && currentDW->uniqueName() == QLatin1String("DockWidget #2");
            const QColor focusedBackgroundColor = isDw2 ? Qt::cyan : Qt::yellow;

            const QColor backgroundColor = focusedBackgroundColor.darker(115);
            QBrush brush(isFocused() ? focusedBackgroundColor : backgroundColor);
            pen.setWidth(4);
            p.setPen(pen);
            p.setBrush(brush);
            p.drawRect(rect().adjusted(4, 4, -4, -4));
            QFont f = qApp->font();
            f.setPixelSize(30);
            f.setBold(true);
            p.setFont(f);
            p.drawText(QPoint(10, 40), title());
        } else {
            KDDockWidgets::TitleBarWidget::paintEvent(ev);
        }
    }
};

MyTitleBar::~MyTitleBar() = default;

// Inheriting from SeparatorWidget instead of Separator as it handles moving and mouse cursor changing
class MySeparator : public Layouting::SeparatorWidget
{
public:
    explicit MySeparator(Layouting::Widget *parent)
        : Layouting::SeparatorWidget(parent)
    {
    }

    ~MySeparator() override;

    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.fillRect(QWidget::rect(), Qt::cyan);
    }
};

MySeparator::~MySeparator() = default;

KDDockWidgets::TitleBar * CustomWidgetFactory::createTitleBar(KDDockWidgets::Frame *frame) const
{
    // Feel free to return MyTitleBar_CSS here instead, but just for education purposes!
    return new MyTitleBar(frame);
}

KDDockWidgets::TitleBar * CustomWidgetFactory::createTitleBar(KDDockWidgets::FloatingWindow *fw) const
{
    // Feel free to return MyTitleBar_CSS here instead, but just for education purposes!
    return new MyTitleBar(fw);
}

Layouting::Separator * CustomWidgetFactory::createSeparator(Layouting::Widget *parent) const
{
    return new MySeparator(parent);
}
