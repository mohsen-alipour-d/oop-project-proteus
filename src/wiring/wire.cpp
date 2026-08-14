#include "wire.h"

Wire::Wire(Pin* start, Pin* end) {
    startPin = start;
    endPin = end;
    route90();
}

void Wire::route90() {
    points.clear();
    Vector2D a = startPin->worldPos();
    Vector2D b = endPin->worldPos();
    points.push_back(a);
    if (a.x != b.x && a.y != b.y)
        points.push_back(Vector2D(b.x, a.y));
    points.push_back(b);
}

static double segDist(const Vector2D& p, const Vector2D& a, const Vector2D& b) {
    double dx = b.x - a.x;
    double dy = b.y - a.y;
    double len2 = dx * dx + dy * dy;
    double t = 0;
    if (len2 > 0)
        t = ((p.x - a.x) * dx + (p.y - a.y) * dy) / len2;
    if (t < 0)
        t = 0;
    if (t > 1)
        t = 1;
    double cx = a.x + t * dx;
    double cy = a.y + t * dy;
    double ex = p.x - cx;
    double ey = p.y - cy;
    return sqrt(ex * ex + ey * ey);
}

double Wire::distanceTo(const Vector2D& p) const {
    double best = 1e9;
    for (int i = 0; i + 1 < (int)points.size(); i++) {
        double d = segDist(p, points[i], points[i + 1]);
        if (d < best)
            best = d;
    }
    return best;
}

bool Wire::containsPoint(const Vector2D& p, double tol) const {
    return distanceTo(p) <= tol;
}