#pragma once

#include <memory>
#include <vector>

#include "rectangle.hpp"



class Widget {
public:
    Rectangle dimensions;
    std::vector<std::unique_ptr<Widget>> m_children;

public:
    virtual ~Widget() = default;

    virtual void draw(Rectangle root, Rectangle drawRegion);

    void removeAllChild();

    static Widget* addWidget(Widget* parent);
};
