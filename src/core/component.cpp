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

    if (mirroredH)
        px = -px;
    if (mirroredV)
        py = -py;

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
    string s = typeTag() + " " + name + " " + to_string((int)position.x) + " " + to_string((int)position.y) + " " + to_string(rotation) + " " + (mirroredH ? "1" : "0") + " " + (mirroredV ? "1" : "0");
    string ex = extraData();
    if (ex != "")
        s += " " + ex;
    return s;
}