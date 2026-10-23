#include "widget.h"



void Widget::draw(Rectangle root, Rectangle drawRegion) {
    for (auto& child : m_children) {
        const Rectangle childOverlap = drawRegion / child->dimensions;
        if (childOverlap.width == 0 || childOverlap.height == 0) {
            continue;
        }

        const Rectangle childRegion = childOverlap - child->dimensions.toPoint();
        const Rectangle childRoot = root + child->dimensions;

        child->draw(childRoot, childRegion);
    }
}


void Widget::removeAllChild() {
    m_children.clear();
}

Widget *Widget::addWidget(Widget *parent) {
    Widget* newWidget = new Widget();

    std::unique_ptr<Widget> ptr{newWidget};

    parent->m_children.push_back(std::move(ptr));

    return newWidget;
}


