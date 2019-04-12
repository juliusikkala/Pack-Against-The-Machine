#ifndef RECT_PACKER_BOARD_HH
#define RECT_PACKER_BOARD_HH
#include <SFML/Graphics.hpp>
#include <vector>

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

    void draw(
        sf::RenderWindow& win,
        int x, int y,
        int w, int h,
        bool draw_grid,
        sf::Font* number_font
    ) const;
private:
    int width, height;
    std::vector<rect> rects;
};

#endif
