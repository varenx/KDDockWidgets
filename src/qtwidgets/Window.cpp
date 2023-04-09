/*
  This file is part of KDDockWidgets.

  SPDX-FileCopyrightText: 2019-2023 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.com>
  Author: Sérgio Martins <sergio.martins@kdab.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only

  Contact KDAB at <info@kdab.com> for commercial licensing options.
*/


#include "Window.h"
#include "qtwidgets/views/ViewWrapper.h"

#include <QWidget>
#include <QVariant>
#include <QWindow>
#include <QDebug>

using namespace KDDockWidgets;
using namespace KDDockWidgets::qtwidgets;

Window::~Window() = default;

inline QWindow *windowForWidget(QWidget *topLevel)
{
    if (!topLevel->windowHandle())
        topLevel->winId();

    return topLevel->windowHandle();
}

Window::Window(QWidget *topLevel)
    : Window_qt(windowForWidget(topLevel))
{
    // QWidgetWindow is private API, we have no way for going from QWindow to the top-level QWidget
    // So set it as a property
    setProperty("kddockwidgets_qwidget", QVariant::fromValue<QWidget *>(topLevel));
}

std::shared_ptr<Core::View> Window::rootView() const
{
    if (!m_window)
        return {};

    if (QWidget *widget = m_window->property("kddockwidgets_qwidget").value<QWidget *>())
        return qtwidgets::ViewWrapper_qtwidgets::create(widget);

    qWarning() << Q_FUNC_INFO << "Window does not have a root";
    return nullptr;
}

Core::Window::Ptr Window::transientParent() const
{
    if (QWindow *w = m_window->transientParent())
        return Core::Window::Ptr(new Window(w));

    return nullptr;
}

void Window::setGeometry(QRect geo) const
{
    if (auto v = rootView()) {
        // In QWidget world QWidget interface is preferred over QWindow
        v->setGeometry(geo);
    } else {
        // Go via QWindow instead
        Window_qt::setGeometry(geo);
    }
}

void Window::setVisible(bool is)
{
    if (auto v = rootView()) {
        // In QWidget world QWidget interface is preferred over QWindow
        v->controller()->setVisible(is);
    } else {
        // Go via QWindow instead
        Window_qt::setVisible(is);
    }
}

bool Window::supportsHonouringLayoutMinSize() const
{
    // QWidget's QLayout does this for us and propagates the min-size up to QWindow
    return true;
}

void Window::destroy()
{
    if (auto v = rootView()) {
        // deleting the QWidget deletes its QWindow
        delete static_cast<qtwidgets::ViewWrapper_qtwidgets *>(v.get())->widget();
    } else {
        Window_qt::destroy();
    }
}
