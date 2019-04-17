#ifndef RECT_PACKER_HH
#define RECT_PACKER_HH
#include <vector>

// One-off rectangle packer. Attempts to fit as many as possible rectangles in
// the given size. This implementation aims for one-by-one packing and allows
// easy resizing.
//
// Please use stb_rect_pack instead if you don't need resizing or one-by-one
// packing. It's likely much better when packing multiple rectangles at once.
//
// This algorithm works by finding such a placing for the rectangle that it's
// edges are minimally exposed to the area left free. In other words, it
// maximizes contact surface area with previously allocated space. This packing
// algorithm is quite intuitive; I designed it based on what I would do if I was
// asked to pack arbitrary rectangles in a limited-size bin without knowledge
// of future rectangles.
//
// A naive way to do this would be to simply test all possible free positions.
// To speed up computation, the search space is limited to edges of the free
// space.
class rect_packer
{
public:
    rect_packer(int w = 0, int h = 0, bool open = false);

    void enlarge(int w, int h);
    void reset(int w, int h);
    void reset();

    // If open, cost approximation is adjusted such that packing after enlarge()
    // yields better results. Set this to true if you plan to enlarge(). If you
    // don't use enlarge(), this will cause packing results to be slightly
    // worse.
    void set_open(bool open);

    // Returns false if this rectangle could not be packed. In that case, use
    // enlarge() to make the canvas larger and retry. If succesful, the
    // coordinates of the corner closest to your origin are written to x and y.
    // (that corner typically means the top-left corner in rasterizing 2D apps,
    // but this class doesn't actually care about coordinate directions)
    bool pack(int w, int h, int& x, int& y, bool full_search = false);

    // pack(), but allows 90 degree rotation of the input rectangle. rotated is
    // set to true if that happened.
    bool pack_rotate(
        int w, int h, int& x, int& y, bool& rotated, bool full_search = false
    );

    struct rect
    {
        // Fill these in before calling.
        int w, h;

        // These are set by pack(). If (-1, -1), the rect placing was
        // unsuccessful.
        int x = -1, y = -1;
        // If you don't allow rotation, this will always be set to false and you
        // don't have to care about it.
        bool rotated = false;
    };

    // This is not a very smart algorithm. It just sorts the inputs by area
    // first. The results are surprisingly good, especially if rotation is 
    // enabled. The number of packed rects is returned.
    int pack(
        rect* rects, size_t count, bool allow_rotation = false,
        bool full_search = false
    );

private:
    struct free_edge
    {
        int x, y, length;
        bool vertical, up_right_inside;
        unsigned marker;
    };

    void recalc_edge_lookup();

    // Slow, but never misses possible rect placements.
    int find_max_score_full(
        int w, int h, int& x, int& y,
        std::vector<free_edge*>& affected_edges
    );

    // Fast, but may miss possible placements.
    int find_max_score_corner(
        int w, int h, int& x, int& y,
        std::vector<free_edge*>& affected_edges
    );

    // 0 if can't be placed here. Otherwise, number of blocked edges.
    int score_rect(
        int x, int y, int w, int h,
        std::vector<free_edge*>& affected_edges
    );

    // -1 if crosses.
    int score_rect_edge(int x, int y, int w, int h, free_edge* edge);

    void place_rect(
        int x, int y, int w, int h,
        std::vector<free_edge*>& affected_edges
    );

    void edge_clip(const free_edge& mask, std::vector<free_edge>& clipped);

    std::vector<free_edge> edges;
    int canvas_w, canvas_h;
    std::vector<
        std::vector<free_edge*>
    > edge_lookup;
    int cell_w, cell_h;
    int lookup_w, lookup_h;
    bool open;
    unsigned marker;

    // Stored here to avoid allocations.
    std::vector<free_edge*> tmp;
};

#endif
