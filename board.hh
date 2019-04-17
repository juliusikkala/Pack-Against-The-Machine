#ifndef RECT_PACKER_BOARD_HH
#define RECT_PACKER_BOARD_HH
#include <SFML/Graphics.hpp>
#include <vector>

class rect_packer;
class board
{
public:
    board(int w, int h);

    void resize(int w, int h);
    void reset();

    struct rect
    {
        int id;
        int x, y;
        int w, h;
    };

    void place(const rect& r);
    bool can_place(const rect& r) const;
    double coverage() const;

    void draw(
        sf::RenderWindow& win,
        int x, int y,
        int w, int h,
        bool draw_grid,
        sf::Font* number_font
    ) const;

    void draw_debug_edges(
        sf::RenderWindow& win,
        rect_packer& pack,
        int x, int y,
        int w, int h
    ) const;
private:
    int width, height;
    int covered;
    std::vector<rect> rects;
};

#endif
