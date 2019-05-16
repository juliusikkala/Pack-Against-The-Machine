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
#ifndef RECT_PACKER_BOARD_HH
#define RECT_PACKER_BOARD_HH
#include <SFML/Graphics.hpp>
#include <vector>

class rect_packer;
class board
{
public:
    board(unsigned w, unsigned h);

    void resize(unsigned w, unsigned h);
    void reset();

    struct rect
    {
        unsigned id;
        unsigned x, y;
        unsigned w, h;
    };

    void place(const rect& r);
    bool can_place(const rect& r) const;
    double coverage() const;

    void draw(
        sf::RenderWindow& win,
        unsigned x, unsigned y,
        unsigned w, unsigned h,
        bool draw_grid,
        sf::Font* number_font
    ) const;

    void draw_debug_edges(
        sf::RenderWindow& win,
        rect_packer& pack,
        unsigned x, unsigned y,
        unsigned w, unsigned h
    ) const;
private:
    unsigned width, height;
    unsigned covered;
    std::vector<rect> rects;
};

#endif
