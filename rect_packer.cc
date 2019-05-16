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

int line_map::score(
    unsigned x,
    unsigned y,
    unsigned w,
    unsigned h,
    bool move_direction,
    bool scored_edge,
    int& min_skip,
    int& max_skip
) const
{
    unsigned score_line = x + (scored_edge ? w : 0);
    int score = 0;
    
    unsigned i = lines[score_line];
    unsigned end = lines[score_line+1];
    unsigned top = y + h;

    bool first_above = false;
    // Calculate score
    for(; i < end; ++i)
    {
        edge& e = edges[i];
        unsigned et = e.pos + e.length;
        if(et <= y || top <= e.pos) continue;
        if(move_direction && !first_above && et > top && e.pos > y)
        {
            max_skip = std::min((int)e.pos - (int)y, max_skip);
            first_above = true;
        }
        score += calc_overlap(y, h, edges[i].pos, edges[i].length);
    }

    // Check blockers & calculate skip.
    unsigned line = x + 1;
    i = lines[line];
    unsigned line_end = lines[line+1];
    bool blocked = false;

    while(line < x + w)
    {
        for(; i < line_end; ++i)
        {
            edge& e = edges[i];
            unsigned et = e.pos + e.length;
            if(et <= y || top <= e.pos) continue;
            if(move_direction) min_skip = std::max((int)(et - y), min_skip);
            else min_skip = std::max((int)(line - x), min_skip);
            blocked = true;
        }
        line++;
        line_end = lines[line+1];
    }

    return blocked ? -1 : score;
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

    // Vertical scan
    auto right_it = right[0];
    auto left_it = left[w];
    unsigned tested_positions = 0;
    for(unsigned x = 0; x <= canvas_w - w; ++x)
    {
        auto right_line_end = right[x+1];
        auto left_line_end = left[x+w+1];
        unsigned y = 0;

        while(true)
        {
            unsigned min_y = canvas_h + h;
            while(left_it != left_line_end)
            {
                unsigned left_top = left_it->pos + left_it->length;
                if(left_top > y)
                {
                    min_y = left_it->pos;
                    break;
                }
                ++left_it;
            }
            while(right_it != right_line_end)
            {
                unsigned right_top = right_it->pos + right_it->length;
                if(right_top > y)
                {
                    min_y = std::min(min_y, right_it->pos);
                    break;
                }
                ++right_it;
            }

            y = std::max((int)y, (int)min_y - (int)h + 1);
            if(y + h > canvas_h) break;

            int skip = 1;
            int score = score_rect(x, y, w, h, skip);
            tested_positions++;
            if(score > best_score)
            {
                best_score = score;
                best_x = x;
                best_y = y;
            }
            y += skip;
        }
        left_it = left_line_end;
        right_it = right_line_end;
    }

    // Horizontal scan
    auto up_it = up[0];
    auto down_it = down[h];
    for(unsigned y = 0; y <= canvas_h - h; ++y)
    {
        auto up_line_end = up[y+1];
        auto down_line_end = down[y+h+1];
        unsigned x = 0;

        while(true)
        {
            unsigned min_x = canvas_w + w;
            while(down_it != down_line_end)
            {
                unsigned down_top = down_it->pos + down_it->length;
                if(down_top > x)
                {
                    min_x = down_it->pos;
                    break;
                }
                ++down_it;
            }
            while(up_it != up_line_end)
            {
                unsigned up_top = up_it->pos + up_it->length;
                if(up_top > x)
                {
                    min_x = std::min(min_x, up_it->pos);
                    break;
                }
                ++up_it;
            }

            x = std::max((int)x, (int)min_x - (int)w + 1);
            if(x + w > canvas_w) break;

            int skip = 0;
            int score = score_rect(x, y, w, h, skip);
            tested_positions++;
            if(score > best_score)
            {
                best_score = score;
                best_x = x;
                best_y = y;
            }
            x += skip;
        }
        down_it = down_line_end;
        up_it = up_line_end;
    }
    return best_score;
}

int rect_packer::score_rect(
    unsigned x,
    unsigned y,
    unsigned w,
    unsigned h,
    int& skip
){
    bool vertical = skip;
    int min_skip = 1;
    int max_skip = vertical ? canvas_h - y - h : canvas_w - x - w;
    int rscore = right.score(x, y, w, h, vertical, false, min_skip, max_skip);
    int lscore = left.score(x, y, w, h, vertical, true, min_skip, max_skip);
    int uscore = up.score(y, x, h, w, !vertical, false, min_skip, max_skip);
    int dscore = down.score(y, x, h, w, !vertical, true, min_skip, max_skip);
    skip = std::max(min_skip, max_skip);
    if(rscore < 0 || lscore < 0 || uscore < 0 || dscore < 0) return 0;
    return rscore + lscore + uscore + dscore;
}

void rect_packer::place_rect(unsigned x, unsigned y, unsigned w, unsigned h)
{
    right.insert(x+w, {y, h}, left);
    left.insert(x, {y, h}, right);
    up.insert(y+h, {x, w}, down);
    down.insert(y, {x, w}, up);
}
