#include "board.hh"
#include <cmath>
#include <algorithm>
#include <stdexcept>

namespace
{

int range_overlap(int x1, int w1, int x2, int w2)
{
    return std::max(std::min(x1 + w1, x2 + w2) - std::max(x1, x2), 0);
}

int rect_overlap(const board::rect& a, const board::rect& b)
{
    return
        range_overlap(a.x, a.w, b.x, b.w) * range_overlap(a.y, a.h, b.y, b.h);
}

unsigned hsv_to_rgb(float h, float s, float v)
{
    auto f = [h,s,v](int n){
        float k = fmod(n+h/60.0, 6.0);
        return v - v*s*std::clamp(std::min(k, 4-k), 0.0f, 1.0f);
    };

    unsigned r = std::clamp(int(f(5)*255.0f), 0, 255);
    unsigned g = std::clamp(int(f(3)*255.0f), 0, 255);
    unsigned b = std::clamp(int(f(1)*255.0f), 0, 255);
    return (r<<24)|(g<<16)|(b<<8)|0xFF;
}

float circle_sequence(unsigned n) {
    unsigned denom = n + 1;
    denom--;
    denom |= denom >> 1;
    denom |= denom >> 2;
    denom |= denom >> 4;
    denom |= denom >> 8;
    denom |= denom >> 16;
    denom++;
    unsigned num = 1 + (n - denom/2)*2;
    return num/(float)denom;
}

unsigned generate_color(int id, bool bounds)
{
    return hsv_to_rgb(
        360*circle_sequence(id),
        0.5,
        bounds ? 0.7 : 1.0
    );
}

}

board::board(int w, int h)
: width(w), height(h)
{
}

void board::resize(int w, int h)
{
    width = w;
    height = h;
}

void board::reset()
{
    rects.clear();
}

void board::place(const rect& r)
{
    rects.push_back(r);
}

bool board::can_place(const rect& r) const
{
    // Check bounds
    if(r.x < 0 || r.y < 0 || r.x + r.w > width || r.y + r.h > height)
        return false;

    // Check other rects
    for(const rect& o: rects)
    {
        if(rect_overlap(o, r)) return false;
    }
    return true;
}

void board::draw(
    sf::RenderWindow& win,
    int x, int y,
    int w, int h,
    bool draw_grid,
    sf::Font* number_font
) const
{
    // Draw bounds
    sf::Color bounds_color(0x3C3C3CFF);
    sf::Color background_color(0xFFFFFFFF);
    sf::Vertex bounds[] = {
        sf::Vertex(sf::Vector2f(x, y)),
        sf::Vertex(sf::Vector2f(x+w, y)),
        sf::Vertex(sf::Vector2f(x+w, y+h)),
        sf::Vertex(sf::Vector2f(x, y+h)),
        sf::Vertex(sf::Vector2f(x, y))
    };
    for(sf::Vertex& v: bounds) v.color = background_color;
    win.draw(bounds, 4, sf::Quads);
    for(sf::Vertex& v: bounds) v.color = bounds_color;
    win.draw(bounds, 5, sf::LineStrip);

    // Draw grid if asked to
    if(draw_grid)
    {
        for(int gy = 1; gy < height; ++gy)
        {
            int sy = y + gy * h / height;
            sf::Vertex grid_line[] = {
                sf::Vertex(sf::Vector2f(x, sy), bounds_color),
                sf::Vertex(sf::Vector2f(x + w, sy), bounds_color),
            };
            win.draw(grid_line, 2, sf::Lines);
        }

        for(int gx = 1; gx < width; ++gx)
        {
            int sx = x + gx * w / width;
            sf::Vertex grid_line[] = {
                sf::Vertex(sf::Vector2f(sx, y), bounds_color),
                sf::Vertex(sf::Vector2f(sx, y + h), bounds_color),
            };
            win.draw(grid_line, 2, sf::Lines);
        }
    }

    // Draw rects
    float outline_thickness = std::max(w/(float)width*0.2f, 1.0f);
    float font_size = std::max(w/(float)width*0.5f, 8.0f);
    for(const rect& r: rects)
    {
        sf::Color color(generate_color(r.id, false));
        sf::Color outline_color(generate_color(r.id, true));

        int sx1 = x + r.x * w / width;
        int sy1 = y + r.y * h / height+(draw_grid ? 1 : 0);
        int sx2 = x + (r.x+r.w) * w / width-(draw_grid ? 1 : 0);
        int sy2 = y + (r.y+r.h) * h / height;
        int sw = sx2-sx1;
        int sh = sy2-sy1;
        sf::RectangleShape rs(sf::Vector2f(sw, sh));
        rs.setPosition(sx1, sy1);
        rs.setOutlineThickness(-outline_thickness);
        rs.setFillColor(color);
        rs.setOutlineColor(outline_color);
        win.draw(rs);

        if(number_font)
        {
            sf::Text number(std::to_string(r.id), *number_font, font_size);
            number.setOutlineColor(sf::Color::Black);
            number.setFillColor(sf::Color::White);
            number.setOutlineThickness(font_size*0.1f);
            number.setPosition(sf::Vector2f(sx1+sx2, sy1+sy2)*0.5f);
            sf::FloatRect lb = number.getLocalBounds();
            number.setOrigin(lb.left + lb.width*0.5f, lb.top + lb.height*0.5f);
            win.draw(number);
        }
    }
}
