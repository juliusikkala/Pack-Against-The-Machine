#include "rect_packer.hh"
#include <algorithm>

namespace
{
    int calc_overlap(int x1, int w1, int x2, int w2)
    {
        return std::max(std::min(x1 + w1, x2 + w2) - std::max(x1, x2), 0);
    }
}

rect_packer::rect_packer(): canvas_w(0), canvas_h(0) {}

rect_packer::rect_packer(int w, int h)
{
    reset(w, h);
}

void rect_packer::enlarge(int w, int h)
{
    std::vector<free_rect> add;
    int handled_x = 0;
    // Loop all free space, looking for top-aligned rects
    for(unsigned i = 0; i < free_space.size(); ++i)
    {
        free_rect& r = free_space[i];
        // If aligned to top
        if(r.y + r.h == canvas_h)
        {
            // Handle adding un-extended space
            if(r.x != handled_x)
            {
                add.push_back(
                    {{}, {}, handled_x, canvas_h, r.x - handled_x, h - canvas_h}
                );
            }
            // Extend to new canvas size
            r.h += h - canvas_h;
            handled_x = r.x + r.w;
        }
    }

    if(handled_x != canvas_w)
    {
        add.push_back(
            {{}, {}, handled_x, canvas_h, canvas_w - handled_x, h - canvas_h}
        );
    }

    add.push_back({{}, {}, canvas_w, 0, w - canvas_w, h});

    // Add new rects
    size_t old_len = free_space.size();
    free_space.insert(free_space.end(), add.begin(), add.end());
    std::inplace_merge(
        free_space.begin(),
        free_space.begin() + old_len,
        free_space.end(),
        [](const free_rect& a, const free_rect& b){ return a.x < b.x; }
    );
    canvas_w = w;
    canvas_h = h;
    merge();
    update_neighbors();
}

void rect_packer::reset(int w, int h)
{
    canvas_w = w;
    canvas_h = h;
    reset();
}

void rect_packer::reset()
{
    free_space.clear();
    free_space.push_back({{}, {}, 0, 0, canvas_w, canvas_h});
}

bool rect_packer::push(int w, int h, int& best_x, int& best_y)
{
    int best_cost = -1;
    std::vector<free_rect*> path;
    std::vector<free_rect*> best_path;
    for(free_rect& r: free_space)
    {
        // No vertical fit
        if(h > r.h) continue;

        bool easy_fit = w <= r.w;

        // Left edge
        if(!r.right.empty() || r.left.empty())
        {
            int max_y = easy_fit && r.left.empty() ? r.y : r.y + r.h - h;
            int x = r.x;

            for(int y = r.y; y <= max_y; ++y)
            {
                if(!find_right_neighbor_path(y, w, h, r, path))
                    continue;
                int cost = calculate_cost(x, y, w, h, path);
                if(best_cost == -1 || cost < best_cost)
                {
                    best_cost = cost;
                    best_path = path;
                    best_x = x;
                    best_y = y;
                }
            }
        }

        // Right edge
        if(!r.left.empty())
        {
            int max_y = easy_fit && r.right.empty() ? r.y : r.y + r.h - h;
            int x = r.x + r.w - w;

            for(int y = r.y; y <= max_y; ++y)
            {
                if(!find_left_neighbor_path(y, w, h, r, path))
                    continue;
                int cost = calculate_cost(x, y, w, h, path);
                if(best_cost == -1 || cost < best_cost)
                {
                    best_cost = cost;
                    best_path = path;
                    best_x = x;
                    best_y = y;
                }
            }
        }
    }

    // No fit, fail.
    if(best_cost == -1) return false;

    place(best_x, best_y, w, h, best_path);

    return true;
}

void rect_packer::merge()
{
    // Can't trust neighbors here, unfortunately.
    for(unsigned i = 0; i < free_space.size(); ++i)
    {
        free_rect& r = free_space[i];
        // Check if a neighbor is lined up.
        for(unsigned j = i+1; j < free_space.size(); ++j)
        {
            free_rect& n = free_space[j];
            if(n.x == r.x) continue;
            if(n.x > r.x+r.w) break;

            // If lined up,
            if(n.y == r.y && n.y + n.h == r.y + r.h)
            {
                // Widen this rect, remove neighbor.
                r.w += n.w;
                free_space.erase(free_space.begin() + j);
                --j;
            }
        }
    }
}

void rect_packer::update_neighbors()
{
    for(unsigned i = 0; i < free_space.size(); ++i)
    {
        free_rect& a = free_space[i];
        int edge_x = a.x + a.w;
        for(unsigned j = i + 1; j < free_space.size(); ++j)
        {
            free_rect& b = free_space[j];
            if(edge_x < b.x) break;
            else if(edge_x == b.x && calc_overlap(a.y, a.h, b.y, b.h) >= 1)
            {
                b.left.push_back(&a);
                a.right.push_back(&b);
            }
        }
    }
}

bool rect_packer::find_right_neighbor_path(
    int y, int w, int h,
    free_rect& left,
    std::vector<free_rect*>& path
){
    path.clear();
    path.push_back(&left);

    free_rect* cur = &left;
    int total_w = left.w;
    while(total_w < w)
    {
        bool found = false;
        for(free_rect* r: cur->right)
        {
            if(r->y <= y && r->y + r->h >= y + h)
            {
                cur = r;
                found = true;
                total_w += r->w;
                path.push_back(r);
                break;
            }
        }
        if(!found) return false;
    }
    return true;
}

bool rect_packer::find_left_neighbor_path(
    int y, int w, int h,
    free_rect& right,
    std::vector<free_rect*>& path
){
    path.clear();
    path.push_back(&right);

    free_rect* cur = &right;
    int total_w = right.w;
    while(total_w < w)
    {
        bool found = false;
        for(free_rect* r: cur->left)
        {
            if(r->y <= y && r->y + r->h >= y + h)
            {
                cur = r;
                found = true;
                total_w += r->w;
                path.push_back(r);
                break;
            }
        }
        if(!found) return false;
    }
    std::reverse(path.begin(), path.end());
    return true;
}

void rect_packer::place(
    int x, int y, int w, int h,
    std::vector<free_rect*>& path
){
    // Used for transforming pointers in 'path' to indices, such that
    // reallocating free_space doesn't destroy everything.
    free_rect* base = free_space.data();
    for(free_rect& r: free_space)
    {
        r.left.clear();
        r.right.clear();
    }

    int split = free_space.size();
    // Create new split rects & push them to the space.
    for(free_rect* r: path)
    {
        int left_x = std::max(r->x, x);
        int right_x = std::min(r->x + r->w, x + w);
        free_rect left{{},{}, r->x, r->y, x - r->x, r->h};
        free_rect right{{},{}, x + w, r->y, r->x + r->w - x - w, r->h};
        free_rect above{{},{}, left_x, y + h, right_x - left_x, r->y + r->h - y - h};
        free_rect below{{},{}, left_x, r->y, right_x - left_x, y - r->y};

        if(left.w > 0) free_space.push_back(left);
        if(above.h > 0) free_space.push_back(above);
        if(below.h > 0) free_space.push_back(below);
        if(right.w > 0) free_space.push_back(right);
    }

    // Delete old path
    for(free_rect* r: path)
    {
        unsigned i = r - base;
        free_space.erase(free_space.begin()+i);
        base++;
        split--;
    }

    // Merge new rects
    std::inplace_merge(
        free_space.begin(),
        free_space.begin() + split,
        free_space.end(),
        [](const free_rect& a, const free_rect& b){ return a.x < b.x; }
    );
    merge();
    update_neighbors();
}

int rect_packer::calculate_cost(
    int x, int y, int w, int h,
    const std::vector<free_rect*>& path
){
    free_rect* left = path[0];
    free_rect* right = path[path.size()-1];

    int exposed = 0;

    // Left edge
    if(left->x == x)
    {
        for(free_rect* r: left->left)
            exposed += calc_overlap(y, h, r->y, r->h);
    }
    else exposed += h;

    // Top & bottom edges
    for(free_rect* r: path)
    {
        int overlap = calc_overlap(x, w, r->x, r->w);
        // No top contact
        if(r->y + r->h > y + h) exposed += overlap;

        // No bottom contact
        if(r->y < y) exposed += overlap;
    }

    // Right edge
    if(right->x + right->w == x + w)
    {
        for(free_rect* r: right->right)
            exposed += calc_overlap(y, h, r->y, r->h);
    }
    else exposed += h;

    return exposed;
}
