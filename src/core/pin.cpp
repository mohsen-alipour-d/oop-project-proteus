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

void Pin::setDirection(PinDirection newDirection) {
    direction = newDirection;

    if (newDirection == PinDirection::Output) {
        isOutput = true;
        driving = true;
    } else if (newDirection == PinDirection::Input) {
        isOutput = false;
        driving = false;
    } else {
        // A bidirectional pin only drives a net when setDriving(true) is used.
        isOutput = false;
        driving = false;
    }
}

void Pin::setDriving(bool enabled) {
    if (direction == PinDirection::Bidirectional)
        driving = enabled;
    else if (direction == PinDirection::Output)
        driving = true;
    else
        driving = false;
}

bool Pin::drivesNet() const {
    // isOutput keeps all existing Part 5/6/9/10/11 components compatible.
    return isOutput || direction == PinDirection::Output ||
           (direction == PinDirection::Bidirectional && driving);
}

bool Pin::needsInputConnection() const {
    // An actively-driving bidirectional pin should not be reported as a
    // floating input. When it stops driving, it behaves as an input again.
    if (drivesNet())
        return false;
    return direction == PinDirection::Input || direction == PinDirection::Bidirectional;
}
