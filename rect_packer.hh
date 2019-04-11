#ifndef RECT_PACKER_HH
#define RECT_PACKER_HH
#include <vector>

// One-off rectangle packer. Attempts to fit as many as possible rectangles in
// the given size. This implementation aims for one-by-one packing and allows
// easy resizing.
//
// Please use stb_rect_pack instead if you don't need resizing or one-by-one
// packing. It's likely much better when packing multiple rectangles at once.
class rect_packer
{
public:
    rect_packer();
    rect_packer(int w, int h);

    void enlarge(int w, int h);
    void reset(int w, int h);
    void reset();

    // Returns false if this rectangle could not be packed. In that case, use
    // enlarge() to make the canvas larger and retry. If succesful, the
    // coordinates of the corner closest to your origin are written to x and y.
    // (that corner typically means the top-left corner in rasterizing 2D apps,
    // but this class doesn't actually care about coordinate directions)
    bool push(int w, int h, int& x, int& y);

private:
    struct free_rect
    {
        // Only temporary storage, helps checking relevant neighbors
        std::vector<free_rect*> left, right;
        // Size and location.
        int x, y, w, h;
    };

    // Merges free rectangles.
    void merge();
    void update_neighbors();

    bool find_right_neighbor_path(
        int y, int w, int h,
        free_rect& left,
        std::vector<free_rect*>& path
    );

    bool find_left_neighbor_path(
        int y, int w, int h,
        free_rect& right,
        std::vector<free_rect*>& path
    );

    void place(int x, int y, int w, int h, std::vector<free_rect*>& path);
    int calculate_cost(int x, int y, int w, int h, const std::vector<free_rect*>& path);

    // This algorithm just stores the free space left as rectangles. Keep sorted
    // by x.
    std::vector<free_rect> free_space;
    int canvas_w, canvas_h;
};

#endif
