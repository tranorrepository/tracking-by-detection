#ifndef CPP_BOUNDINGBOX_H
#define CPP_BOUNDINGBOX_H


#include <ostream>

struct BoundingBox {
    BoundingBox(const int cx, const int cy, const int width, const int height);

    const int cx;
    const int cy;
    const int width;
    const int height;

    // (x1,y1) - top left
    // (x2,y2) - bottom right
    inline int x1() const { return int(cx - width / 2.); }
    inline int y1() const { return int(cy - height / 2.); }
    inline int x2() const { return int(cx + width / 2.); }
    inline int y2() const { return int(cy + height / 2.); }
    inline int area() const { return width * height; }
    inline double ratio() const { return width / double(height); }

    static double iou(const BoundingBox &a, const BoundingBox &b);
};

std::ostream &operator<<(std::ostream &os, const BoundingBox &bb);


#endif //CPP_BOUNDINGBOX_H