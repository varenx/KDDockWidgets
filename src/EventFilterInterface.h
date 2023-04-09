/*
  This file is part of KDDockWidgets.

  SPDX-FileCopyrightText: 2020-2023 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.com>
  Author: Sérgio Martins <sergio.martins@kdab.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only

  Contact KDAB at <info@kdab.com> for commercial licensing options.
*/

#pragma once

#include "NonQtCompat_p.h"

#include <memory>

namespace KDDockWidgets {

namespace Core {
class Window;
class View;
}

class EventFilterInterface
{
public:
    virtual ~EventFilterInterface();

    /// @brief Override to handle expose events for a certain window
    virtual bool onExposeEvent(std::shared_ptr<Core::Window>)
    {
        return false;
    }

    /// @brief Override to handle when a view receives a mouse press event
    virtual bool onMouseButtonPress(Core::View *, MouseEvent *)
    {
        return false;
    }

    /// @brief Override to handle when a view receives a mouse press event
    virtual bool onMouseButtonRelease(Core::View *, MouseEvent *)
    {
        return false;
    }

    /// @brief Override to handle when a view receives a mouse press event
    virtual bool onMouseButtonMove(Core::View *, MouseEvent *)
    {
        return false;
    }

    /// @brief Override to handle when a view receives a mouse double click event
    virtual bool onMouseDoubleClick(Core::View *, MouseEvent *)
    {
        return false;
    }

    /// @brief Provided for convenience, aggregates all other overloads
    /// receives all mouse event types, if you return true here then the specialized counterparts
    /// won't be called Example, if true is returned here for a mouse press, then
    /// onMouseButtonPress() won't be called
    virtual bool onMouseEvent(Core::View *, MouseEvent *)
    {
        return false;
    }

    /// @brief Override to handle drag enter, drag leave, drag move and drop events
    virtual bool onDnDEvent(Core::View *, Event *)
    {
        return false;
    }

    /// @brief Override to handle a move event
    virtual bool onMoveEvent(Core::View *)
    {
        return false;
    }
};

}
