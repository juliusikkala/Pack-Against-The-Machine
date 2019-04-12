#include "rect_packer.hh"
#include "board.hh"
#include <SFML/Graphics.hpp>
#include <cstdio>
#include <memory>
#include <random>
#include <algorithm>
#include <ctime>

static int initial_seed = 1555097569;//time(nullptr);

template<typename T>
void shuffle(std::vector<T>& v)
{
    std::mt19937 rng(initial_seed++);
    std::shuffle(v.begin(), v.end(), rng);
}

std::vector<board::rect> generate_guillotine_set(
    int w, int h, unsigned splits
){

    printf("Generating guillotine set for seed %d\n", initial_seed);
    std::mt19937 rng(initial_seed++);
    std::uniform_int_distribution<> bdis(0,1);

    struct node
    {
        int w, h;
        bool vertical;
        std::vector<node> children;

        bool atomic()
        {
            return (vertical && w == 1) || (!vertical && h == 1);
        }

        bool split(std::mt19937& rng)
        {
            if(atomic()) return false;
            if(children.size())
            {
                std::uniform_int_distribution<> bdis(0,1);
                int first = bdis(rng);
                int second = first^1;
                return children[first].split(rng) || children[second].split(rng);
            }

            if(vertical)
            {
                std::uniform_int_distribution<> wdis(1,w-1);
                int split = wdis(rng);
                children.push_back({split, h, false,{}});
                children.push_back({w-split, h, false,{}});
            }
            else
            {
                std::uniform_int_distribution<> hdis(1,h-1);
                int split = hdis(rng);
                children.push_back({w, split, true,{}});
                children.push_back({w, h-split, true,{}});
            }
            return true;
        }

        void traverse(int x, int y, std::vector<board::rect>& rects)
        {
            if(children.empty())
                rects.push_back({(int)rects.size(), x, y, w, h});
            else for(node& child: children)
            {
                child.traverse(x, y, rects);
                if(vertical) x += child.w;
                else y += child.h;
            }
        }
    };
    node root{w, h, bdis(rng), {}};
    for(unsigned i = 0; i < splits; ++i) root.split(rng);
    std::vector<board::rect> rects;
    root.traverse(0, 0, rects);
    return rects;
}

int main()
{
    sf::RenderWindow window(sf::VideoMode(400, 800), "Pack Against The Machine");

    sf::Font font;

    if(!font.loadFromFile("Inconsolata/Inconsolata-Bold.ttf"))
        throw std::runtime_error("Failed to load Inconsolata");

    unsigned w = 8;
    unsigned h = 8;

    board pack_board(w, h);
    board orig_board(w, h);
    rect_packer packer(w, h);
    int pack_index = 0;

    std::vector<board::rect> rects;

    auto reset = [&](){
        printf("Board reset!\n");
        pack_board.reset();
        orig_board.reset();
        packer.reset();
        pack_index = 0;

        rects = generate_guillotine_set(w, h, 12);
        shuffle(rects);
        for(unsigned i = 0; i < rects.size(); ++i)
        {
            rects[i].id = i;
            orig_board.place(rects[i]);
        }
    };

    auto step = [&](){
        if(pack_index < (int)rects.size())
        {
            board::rect r = rects[pack_index];
            bool rotated = false;
            if(packer.pack_rotate(r.w, r.h, r.x, r.y, rotated))
            {
                if(rotated)
                {
                    int tmp = r.w;
                    r.w = r.h;
                    r.h = tmp;
                }
                pack_board.place(r);
            }
            else
            {
                printf("Pack failed for %d\n", pack_index);
            }
        }
        pack_index++;
    };

    reset();

    window.setVerticalSyncEnabled(true);

    sf::Time total;
    sf::Time tick = sf::milliseconds(250);
    sf::Clock clock;

    bool paused = false;

    while(window.isOpen())
    {
        sf::Event event;
        while(window.pollEvent(event))
        {
            if(event.type == sf::Event::Closed)
                window.close();
            else if(event.type == sf::Event::KeyPressed)
            {
                if(event.key.code == sf::Keyboard::Escape)
                    window.close();
                if(
                    event.key.code == sf::Keyboard::Space ||
                    event.key.code == sf::Keyboard::Return
                ){
                    paused = !paused;
                    clock.restart();
                }
            }
        }

        if(!paused) total += clock.restart();
        while(total > tick)
        {
            total -= tick;
            step();
        }

        if(pack_index > (int)rects.size() + 10) reset();

        window.clear(sf::Color(0x1C675CFF));
        sf::Vector2u sz = window.getSize();
        pack_board.draw(window, 10, 10, sz.x-20, sz.y/2-20, true, &font);
        orig_board.draw(window, 10, sz.y/2+10, sz.x-20, sz.y/2-20, true, &font);

        if(pack_index >= (int)rects.size())
        {
            sf::Text finished("Finished", font, 32);
            finished.setOutlineColor(sf::Color::Black);
            finished.setFillColor(sf::Color::White);
            finished.setOutlineThickness(3);
            finished.setPosition(sf::Vector2f(sz.x, sz.y)*0.5f);
            sf::FloatRect lb = finished.getLocalBounds();
            finished.setOrigin(lb.left + lb.width*0.5f, lb.top + lb.height*0.5f);
            window.draw(finished);
        }

        if(paused)
        {
            sf::Text finished("|| Paused", font, 48);
            finished.setOutlineColor(sf::Color::Black);
            finished.setFillColor(sf::Color::Red);
            finished.setOutlineThickness(5);
            finished.setPosition(sf::Vector2f(10, 10));
            window.draw(finished);
        }

        window.display();
    }
    return 0;
}
