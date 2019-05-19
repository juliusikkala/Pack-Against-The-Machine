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
#include "rect_packer.hh"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <cmath>

namespace
{
    int calc_overlap(int x1, int w1, int x2, int w2)
    {
        return std::max(std::min(x1 + w1, x2 + w2) - std::max(x1, x2), 0);
    }
}

line_map::line_map(size_t line_size)
: lines(line_size+1, 0)
{
}

void line_map::reset(size_t new_lines)
{
    lines.resize(new_lines+1);
    clear();
}

void line_map::enlarge(size_t new_lines)
{
    lines.resize(new_lines+1, lines.back());
}

void line_map::insert(unsigned line, const edge& e, line_map& mask)
{
    thread_local std::vector<edge> scratch;
    scratch.clear();
    scratch.push_back(e);
    
    unsigned i = mask.lines[line];
    unsigned j = 0;
    int offset = 0;

    while(j < scratch.size() && i < mask.lines[line+1])
    {
        unsigned k = i + offset;
        edge& ei = scratch[j];
        edge& em = mask.edges[k];
        unsigned ei_top = ei.pos + ei.length;
        unsigned em_top = em.pos + em.length;

        if(em_top <= ei.pos){i++; continue;} // Go to next mask
        else if(ei_top <= em.pos){j++; continue;} // Go to next insert

        // Overlap is guaranteed. (otherwise one edge would have been fully
        // above another)

        // Mask above/below leftovers
        int ma_length = (int)em_top - (int)ei_top;
        int mb_length = (int)ei.pos - (int)em.pos;

        // Inserted above/below leftovers
        int ia_length = -ma_length;
        int ib_length = -mb_length;

        // Handle insertion cases.
        if(ia_length > 0 && ib_length > 0)
        {
            scratch[j].length = ib_length;
            scratch.insert(
                scratch.begin() + j + 1,
                edge{(unsigned)em_top, (unsigned)ia_length}
            );
        }
        else if(ia_length > 0)
        {
            scratch[j].pos = em_top;
            scratch[j].length = ia_length;
        }
        else if(ib_length > 0)
        {
            scratch[j].length = ib_length;
        }
        else//(ib_length <= 0 && ia_length <= 0)
        {
            scratch.erase(scratch.begin()+j);
        }

        // Handle mask cases.
        if(ma_length > 0 && mb_length > 0)
        {
            mask.edges[k].length = mb_length;
            mask.edges.insert(
                mask.edges.begin() + k + 1,
                edge{(unsigned)ei_top, (unsigned)ma_length}
            );
            offset++;
        }
        else if(ma_length > 0)
        {
            mask.edges[k].pos = ei_top;
            mask.edges[k].length = ma_length;
        }
        else if(mb_length > 0)
        {
            mask.edges[k].length = mb_length;
        }
        else//(mb_length <= 0 && ma_length <= 0)
        {
            mask.edges.erase(mask.edges.begin()+k);
            offset--;
            ++i;
        }
    }

    if(offset != 0) mask.shift_lines(line+1, offset);

    if(scratch.size() == 0) return;

    auto scratch_begin = scratch.begin();
    auto scratch_last = scratch.begin() + (scratch.size() - 1);
    auto scratch_end = scratch.end();
    auto edges_begin = edges.begin() + lines[line];
    auto edges_end = edges.begin() + lines[line+1];

    // Attempt to extend original below
    while(edges_begin < edges_end)
    {
        unsigned old_top = edges_begin->pos + edges_begin->length;
        if(old_top < scratch_begin->pos) edges_begin++;
        else if(old_top == scratch_begin->pos)
        {
            edges_begin->length += scratch_begin->length;
            edges_begin++;
            scratch_begin++;
            // TODO: Merge above too, this requires removal.
            break;
        }
        else break;
    }

    if(scratch_begin <= scratch_last)
    {
        unsigned last_top = scratch_last->pos + scratch_last->length;
        // Attempt to extend original above
        for(auto it = edges_begin; it < edges_end; ++it)
        {
            if(it->pos > last_top) break;
            if(it->pos == last_top)
            {
                it->pos -= scratch_last->length;
                it->length += scratch_last->length;
                scratch_end--;
                break;
            }
        }
    }

    edges.insert(edges_begin, scratch_begin, scratch_end);
    shift_lines(line+1, scratch_end - scratch_begin);
}

line_map::iterator line_map::operator[](unsigned line)
{
    return edges.begin() + lines[line];
}

line_map::const_iterator line_map::operator[](unsigned line) const
{
    return edges.begin() + lines[line];
}

void line_map::clear()
{
    edges.clear();
    memset(lines.data(), 0, lines.size()*sizeof(lines[0]));
}

void line_map::shift_lines(unsigned from_line, int offset) const
{
    for(unsigned i = from_line; i < lines.size(); ++i)
        lines[i] += offset;
}

rect_packer::rect_packer(unsigned w, unsigned h, bool open)
: canvas_w(w), canvas_h(h), open(open)
{
    reset(w, h);
}

void rect_packer::enlarge(unsigned w, unsigned h)
{
    w = std::max(w, canvas_w);
    h = std::max(h, canvas_h);

    right.enlarge(w+1);
    left.enlarge(w+1);
    up.enlarge(h+1);
    down.enlarge(h+1);

    up.insert(canvas_h, {0, canvas_w}, down);
    right.insert(canvas_w, {0, canvas_h}, left);

    right.insert(0, {canvas_h, h - canvas_h}, left);
    left.insert(w, {0, h}, right);
    up.insert(0, {canvas_w, w - canvas_w}, down);
    down.insert(h, {0, w}, up);

    canvas_w = w;
    canvas_h = h;
}

void rect_packer::reset(unsigned w, unsigned h)
{
    canvas_w = w;
    canvas_h = h;

    right.reset(w+1);
    left.reset(w+1);
    up.reset(h+1);
    down.reset(h+1);
    reset();
}

void rect_packer::reset()
{
    left.clear();
    right.clear();
    up.clear();
    down.clear();

    right.insert(0, {0, canvas_h}, left);
    left.insert(canvas_w, {0, canvas_h}, right);
    up.insert(0, {0, canvas_w}, down);
    down.insert(canvas_h, {0, canvas_w}, up);
}

void rect_packer::set_open(bool open)
{
    this->open = open;
}

bool rect_packer::pack(unsigned w, unsigned h, unsigned& x, unsigned& y)
{
    int score = 0;
    score = find_max_score(w, h, x, y);

    // No fit, fail.
    if(score <= 0) return false;

    place_rect(x, y, w, h);

    return true;
}

bool rect_packer::pack_rotate(
    unsigned w,
    unsigned h,
    unsigned& x,
    unsigned& y,
    bool& rotated
){
    // Fast path if we rotation is meaningless.
    if(w == h)
    {
        rotated = false;
        return pack(w, h, x, y);
    }

    // Try both orientations.
    unsigned rot_x, rot_y;
    int score = 0;
    int rot_score = 0;

    score = find_max_score(w, h, x, y);
    rot_score = find_max_score(h, w, rot_x, rot_y);
    if(score <= 0 && rot_score <= 0) return false;

    // Pick better orientation, preferring non-rotated version.
    if(score >= rot_score)
    {
        rotated = false;
        place_rect(x, y, w, h);
    }
    else
    {
        rotated = true;
        x = rot_x; y = rot_y;
        place_rect(x, y, h, w);
    }

    return true;
}

int rect_packer::pack(rect* rects, size_t count, bool allow_rotation)
{
    int packed = 0;

    std::vector<rect*> rr;
    rr.resize(count);
    for(unsigned i = 0; i < count; ++i)
    {
        rr[i] = rects + i;
        rr[i]->rotated = false;
    }

    std::sort(
        rr.begin(),
        rr.end(),
        [](const rect* a, const rect* b){
            return std::max(a->w, a->h) > std::max(b->w, b->h);
        }
    );

    for(rect* r: rr)
    {
        if(r->packed)
        {
            packed++;
            continue;
        }

        if(allow_rotation)
        {
            if(pack_rotate(r->w, r->h, r->x, r->y, r->rotated))
            {
                r->packed = true;
                packed++;
            }
        }
        else
        {
            if(pack(r->w, r->h, r->x, r->y))
            {
                r->packed = true;
                packed++;
            }
        }
    }
    return packed;
}

int rect_packer::find_max_score(
    unsigned w,
    unsigned h,
    unsigned& best_x,
    unsigned& best_y
){
    int best_score = 0;

    unsigned fast_tracks = 0;

    // Vertical scans
    for(unsigned x = 0; x + w <= canvas_w; ++x)
    {
        int y = scan_rect(x, canvas_h, w, h, best_score, right, left, up, down);
        if(y >= 0)
        {
            best_x = x;
            best_y = y;
        }
        if(y == -2) fast_tracks++;
    }

    // Horizontal scans
    for(unsigned y = 0; y + h <= canvas_h; ++y)
    {
        int x = scan_rect(y, canvas_w, h, w, best_score, up, down, right, left);
        if(x >= 0)
        {
            best_x = x;
            best_y = y;
        }
        if(x == -2) fast_tracks++;
    }

    return best_score;
}

int rect_packer::scan_rect(
    unsigned x, unsigned max_y_top, unsigned w, unsigned h, int& best_score,
    const line_map& vright, const line_map& vleft,
    const line_map& hup, const line_map& hdown
){
    auto left_start = vright[x], left_end = vright[x+1];
    auto right_start = vleft[x+w], right_end = vleft[x+w+1];

    // If no left or right edge, there can't be a good position along this
    // line.
    if(left_start == left_end && right_start == right_end) return -2;

    struct scan_state
    {
        line_map::const_iterator start;
        line_map::const_iterator end;
    };

    thread_local std::vector<scan_state> vblock;
    vblock.clear();

    // Init vertical blockers
    for(unsigned ix = x + 1; ix < x + w; ++ix)
    {
        auto start = vright[ix], end = vright[ix+1];
        if(start != end) vblock.push_back({start, end});
    }
    for(unsigned ix = x + 1; ix < x + w; ++ix)
    {
        auto start = vleft[ix], end = vleft[ix+1];
        if(start != end) vblock.push_back({start, end});
    }

    // Start scanning y-axis for positions
    unsigned y = 0;
    int best_y = -1;
    unsigned max_y = max_y_top - h;
    unsigned hblock_y = 0;

    while(y <= max_y)
    {
        unsigned top = y + h;
        unsigned populated_y = max_y;

        // Calculate score first, so that we can skip blocker checking if it
        // can't be the best score.
        int score = 0;

        // Vertical scoring
        // Left side of rectangle
        while(
            left_start < left_end &&
            left_start->pos + left_start->length <= y
        ) ++left_start;
        if(left_start != left_end)
        {
            if(left_start->pos < top) populated_y = y;
            else populated_y = std::min(populated_y, left_start->pos - h);
            auto vstart = left_start, vend = left_end;
            for(; vstart < vend && vstart->pos < top; ++vstart)
                score += calc_overlap(y, h, vstart->pos, vstart->length);
        }

        // Right side of rectangle
        while(
            right_start < right_end &&
            right_start->pos + right_start->length <= y
        ) ++right_start;
        if(right_start != right_end)
        {
            if(right_start->pos < top) populated_y = y;
            else populated_y = std::min(populated_y, right_start->pos - h);
            auto vstart = right_start, vend = right_end;
            for(; vstart < vend && vstart->pos < top; ++vstart)
                score += calc_overlap(y, h, vstart->pos, vstart->length);
        }

        if(populated_y != y)
        {
            y = populated_y + 1;
            continue;
        }

        // Horizontal scoring
        // Bottom side of rectangle
        auto hstart = hup[y], hend = hup[y+1];
        for(; hstart < hend && hstart->pos < x + w; ++hstart)
        {
            if(hstart->pos + hstart->length <= x) continue;
            score += calc_overlap(x, w, hstart->pos, hstart->length);
        }
        // Top side of rectangle
        hstart = hdown[y+h], hend = hdown[y+h+1];
        for(; hstart < hend && hstart->pos < x + w; ++hstart)
        {
            if(hstart->pos + hstart->length <= x) continue;
            score += calc_overlap(x, w, hstart->pos, hstart->length);
        }

        if(score <= best_score)
        {
            y++;
            continue;
        }

        unsigned blocked_y = y;

        // Check vertical blockers for this y
        for(scan_state& vline: vblock)
        {
            // Find next edge upwards if necessary.
            while(
                vline.start != vline.end &&
                vline.start->pos + vline.start->length <= y
            ) ++vline.start;

            // Skip this line if already at the end or it's fully above.
            if(
                vline.start == vline.end ||
                vline.start->pos >= top
            ) continue;

            blocked_y = std::max(
                blocked_y, vline.start->pos + vline.start->length
            );
        }

        // Check horizontal blockers for relevant range
        hblock_y = std::max(hblock_y, y + 1);
        while(hblock_y < top)
        {
            hstart = hup[hblock_y], hend = hup[hblock_y+1];
            for(; hstart != hend; ++hstart)
            {
                if(
                    hstart->pos + hstart->length > x && hstart->pos < x + w
                ) blocked_y = std::max(blocked_y, hblock_y);
            }
            hstart = hdown[hblock_y], hend = hdown[hblock_y+1];
            for(; hstart != hend; ++hstart)
            {
                if(
                    hstart->pos + hstart->length > x && hstart->pos < x + w
                ) blocked_y = std::max(blocked_y, hblock_y + 1);
            }
            ++hblock_y;
        }

        // If we ended up being blocked, jump beyond the blocker and retry.
        if(blocked_y != y)
        {
            y = blocked_y;
            continue;
        }
        else if(score > best_score)
        {
            best_score = score;
            best_y = y;
        }
        y++;
    }

    return best_y;
}

void rect_packer::place_rect(unsigned x, unsigned y, unsigned w, unsigned h)
{
    right.insert(x+w, {y, h}, left);
    left.insert(x, {y, h}, right);
    up.insert(y+h, {x, w}, down);
    down.insert(y, {x, w}, up);
}
