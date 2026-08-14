#include "pin.h"
#include "component.h"

Pin::Pin(double x, double y, Component* owner) {
    localPos = Vector2D(x, y);
    this->owner = owner;
}

Vector2D Pin::worldPos() const {
    return owner->pinWorldPos(localPos);
}

bool Pin::checkMouseOver(const Vector2D& mousePos) {
    isHighlighted = worldPos().distanceTo(mousePos) <= sensitivityRadius;
    return isHighlighted;
}