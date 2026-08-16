#include "component.h"

Component::Component(string name, double x, double y) {
    this->name = name;
    position = Vector2D(x, y);
}

Component::~Component() {
}

void Component::addPin(double x, double y) {
    pins.push_back(Pin(x, y, this));
}

Vector2D Component::pinWorldPos(const Vector2D& local) const {
    double px = local.x;
    double py = local.y;

    // Horizontal mirroring reflects across the horizontal axis (Y changes),
    // while vertical mirroring reflects across the vertical axis (X changes).
    // This matches the editor terminology used by the UI.
    if (mirroredH)
        py = -py;
    if (mirroredV)
        px = -px;

    int steps = rotation / 90;
    for (int i = 0; i < steps; i++) {
        double t = -py;
        py = px;
        px = t;
    }

    return Vector2D(position.x + px, position.y + py);
}

void Component::rotate90() {
    rotation = (rotation + 90) % 360;
}

void Component::mirrorHorizontal() {
    mirroredH = !mirroredH;
}

void Component::mirrorVertical() {
    mirroredV = !mirroredV;
}

string Component::serialize() const {
    string s = typeTag() + " " + name + " " + to_string(position.x) + " " + to_string(position.y) + " " + to_string(rotation) + " " + (mirroredH ? "1" : "0") + " " + (mirroredV ? "1" : "0");
    string ex = extraData();
    if (ex != "")
        s += " " + ex;
    return s;
}

Rect Component::getBoundingBox() const {
    Rect r;
    if (pins.empty()) {
        r.minX = position.x - 10;
        r.maxX = position.x + 10;
        r.minY = position.y - 10;
        r.maxY = position.y + 10;
        return r;
    }
    Vector2D first = pinWorldPos(pins[0].localPos);
    r.minX = r.maxX = first.x;
    r.minY = r.maxY = first.y;
    for (const Pin& p : pins) {
        Vector2D wp = pinWorldPos(p.localPos);
        if (wp.x < r.minX) r.minX = wp.x;
        if (wp.x > r.maxX) r.maxX = wp.x;
        if (wp.y < r.minY) r.minY = wp.y;
        if (wp.y > r.maxY) r.maxY = wp.y;
    }
    r.minX -= 10;
    r.minY -= 10;
    r.maxX += 10;
    r.maxY += 10;
    return r;
}

void Component::snapToGrid(double gridSize) {
    if (gridSize <= 0)
        return;
    position.x = round(position.x / gridSize) * gridSize;
    position.y = round(position.y / gridSize) * gridSize;
}
