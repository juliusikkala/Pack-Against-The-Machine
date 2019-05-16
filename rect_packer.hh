/*
MIT License

Copyright (c) 2019 Julius Ikkala

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#ifndef RECT_PACKER_HH
#define RECT_PACKER_HH
#include <vector>

class line_map
{
public:
    explicit line_map(size_t initial_lines = 0);

    void reset(size_t lines);
    void enlarge(size_t new_lines);

    struct edge
    {
        unsigned pos, length;
    };

    using iterator = std::vector<edge>::iterator;
    using const_iterator = std::vector<edge>::const_iterator;

    int score(
        unsigned x,
        unsigned y,
        unsigned w,
        unsigned h,
        bool move_direction,
        bool scored_edge,
        int& min_skip,
        int& max_skip
    ) const;
    void insert(unsigned line, const edge& e, line_map& mask);

    iterator operator[](unsigned line);
    const_iterator operator[](unsigned line) const;
    void clear();

private:
    void shift_lines(unsigned from_line, int offset) const;

    mutable std::vector<edge> edges;
    mutable std::vector<unsigned> lines;
};

// This algorithm works by finding such a placing for the rectangle that it's
// edges are minimally exposed to the area left free. In other words, it
// maximizes contact surface area with previously allocated space. This packing
// algorithm is quite intuitive; I designed it based on what I would do if I was
// asked to pack arbitrary rectangles in a limited-size bin without knowledge
// of future rectangles.
//
// This algorithm is quite slow due to how extensively it searches. While
// this is consistently better than stb_rect_pack, I recommend you switch to it
// if you have a lot of rectangles to pack, have a large canvas and are under
// strict time limitations. Unlike stb_rect_pack, this also offers the ability
// to grow an existing placement area, without clearing already placed rects.
//
// To give an idea of what the packing difference to stb_rect_pack is, this
// algorithm is typically able to pack about 1-4% more rects than it when
// packing one-by-one, and 1-15% more when packing all at once. The numbers do
// vary a lot between use cases (see glyph_test() in
// https://github.com/juliusikkala/Pack-Against-The-Machine/blob/master/main.cc
// for how I got these numbers, it attempts to mimic packing font glyphs), but
// this algorithm wins almost always (just not necessarily by much).
//
// TL;DR; Caveat emptor, this is probably somewhat better but significantly
// slower than a run-of-the-mill rectangle packer.
class rect_packer
{
public:
    // Constructor for rect_packer. w and h determine the size of the packing
    // area. See set_open() for details about open.
    rect_packer(unsigned w = 0, unsigned h = 0, bool open = false);

    // Grows the packing area without clearing already packed rects. w and h
    // represent the new size. Shrinking is not allowed, so if w or h are
    // smaller than the current width or height, they are clamped.
    void enlarge(unsigned w, unsigned h);

    // Clears the packer state, and changes the size of the packing area.
    void reset(unsigned w, unsigned h);

    // Clears the packer state.
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
    bool pack(unsigned w, unsigned h, unsigned& x, unsigned& y);

    // pack(), but allows 90 degree rotation of the input rectangle. rotated is
    // set to true if that happened.
    bool pack_rotate(
        unsigned w,
        unsigned h,
        unsigned& x,
        unsigned& y,
        bool& rotated
    );

    struct rect
    {
        // Fill these in before calling.
        unsigned w, h;

        // These are set by pack().
        unsigned x = 0, y = 0;

        // This is set to true after being successfully packed.
        bool packed = false;

        // If you don't allow rotation, this will always be set to false and you
        // don't have to care about it.
        bool rotated = false;
    };

    // This is not a very smart algorithm. It just sorts the inputs by 
    // perimeter. The results are surprisingly good, especially if rotation is
    // enabled. The number of packed rects is returned. If a rect is already
    // packed, it is not packed again but does count towards the return value.
    int pack(rect* rects, size_t count, bool allow_rotation = false);

private:
    int find_max_score(unsigned w, unsigned h, unsigned& x, unsigned& y);

    // 0 if can't be placed here. Otherwise, number of blocked edges.
    // skip has two purposes. When calling, set it equal to 'vertical' of the
    // edge the rect is tracking. 'skip' is set to the number of steps that must
    // be moved towards the up or right direction until the result can be
    // better.
    int score_rect(unsigned x, unsigned y, unsigned w, unsigned h, int& skip);
    void place_rect(unsigned x, unsigned y, unsigned w, unsigned h);

    unsigned canvas_w, canvas_h;
    line_map right, left, up, down;
    bool open;
};

#endif
