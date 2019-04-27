#include "rect_packer.hh"
#include "board.hh"
#include <SFML/Graphics.hpp>
#include <cstdio>
#include <memory>
#include <random>
#include <algorithm>
#include <ctime>

static int initial_seed = 0;//time(nullptr);

template<typename T>
void shuffle(std::vector<T>& v)
{
    std::mt19937 rng(initial_seed++);
    std::shuffle(v.begin(), v.end(), rng);
}

std::vector<board::rect> generate_guillotine_set(
    int w, int h, unsigned splits, bool quiet = false
){
    if(!quiet)
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

void measure_rate(
    int w,
    int h,
    unsigned splits,
    unsigned tests,
    bool at_once,
    bool allow_rotation
){
    board pack_board(w, h);
    rect_packer packer(w, h, false);
    std::vector<board::rect> rects;
    std::vector<rect_packer::rect> rects_queue;

    unsigned successes = 0;
    unsigned total_packed = 0;
    unsigned total_count = 0;
    double total_coverage = 0;
    sf::Time total_time;
    sf::Clock clock;
    
    for(unsigned i = 0; i < tests; ++i)
    {
        pack_board.reset();
        packer.reset();

        rects = generate_guillotine_set(w, h, splits, true);
        shuffle(rects);

        unsigned count = 0;
        
        if(at_once)
        {
            rects_queue.clear();
            rects_queue.reserve(rects.size());
            for(unsigned i = 0; i < rects.size(); ++i)
                rects_queue.push_back({rects[i].w, rects[i].h});
            clock.restart();
            count = packer.pack(
                rects_queue.data(),
                rects_queue.size(),
                allow_rotation
            );
            total_time += clock.getElapsedTime();
            for(rect_packer::rect& r: rects_queue)
            {
                if(r.x != -1)
                    pack_board.place({0, r.x, r.y, r.w, r.h});
            }
        }
        else
        {
            clock.restart();
            for(board::rect& r: rects)
            {
                if(allow_rotation)
                {
                    bool rotated = false;
                    if(packer.pack_rotate(r.w,r.h,r.x,r.y,rotated))
                    {
                        if(rotated)
                        {
                            int tmp = r.w;
                            r.w = r.h;
                            r.h = tmp;
                        }
                        pack_board.place(r);
                        count++;
                    }
                }
                else
                {
                    if(packer.pack(r.w,r.h,r.x,r.y))
                    {
                        pack_board.place(r);
                        count++;
                    }
                }
            }
            total_time += clock.getElapsedTime();
        }

        total_count += rects.size();
        total_packed += count;
        successes += count == rects.size() ? 1 : 0;
        total_coverage += pack_board.coverage();
    }

    printf("%u tests at %dx%d with %u splits\n", tests, w, h, splits);
    printf("Success rate: %f\n", successes/(double)tests);
    printf("Rect rate: %f\n", total_packed/(double)total_count);
    printf("Average coverage: %f\n", total_coverage/tests);
    float time = total_time.asSeconds();
    printf("Time: %f\n", time);
    printf("Time per rect^2: %f\n", 1e10*time/pow((double)total_count, 2));
}

int search_optimal_tile_size(
    int w,
    int h,
    unsigned splits,
    float time_limit,
    const std::vector<unsigned>& sizes,
    bool at_once,
    bool allow_rotation,
    bool quiet = false
){
    rect_packer packer(w, h, false);
    std::vector<board::rect> rects;
    std::vector<rect_packer::rect> rects_queue;

    sf::Clock clock;
    sf::Time limit(sf::seconds(time_limit));

    float best_finishes = 0;
    unsigned best_size = 0;

    if(!quiet) printf("Cell size search for %d, %d.\n", w, h);
    for(unsigned cell_size: sizes)
    {
        float finishes = 0;
        sf::Time total_time;

        packer.set_cell_size(cell_size);

        if(!quiet)
        {
            printf("Size %u: ", cell_size);
            fflush(stdout);
        }
        
        initial_seed = 0;
        while(total_time < limit)
        {
            packer.reset();

            rects = generate_guillotine_set(w, h, splits, true);
            shuffle(rects);

            sf::Time elapsed;
            if(at_once)
            {
                rects_queue.clear();
                rects_queue.reserve(rects.size());
                for(unsigned i = 0; i < rects.size(); ++i)
                    rects_queue.push_back({rects[i].w, rects[i].h});
                clock.restart();
                packer.pack(
                    rects_queue.data(),
                    rects_queue.size(),
                    allow_rotation
                );
                elapsed = clock.getElapsedTime();
            }
            else
            {
                clock.restart();
                for(board::rect& r: rects)
                {
                    if(allow_rotation)
                    {
                        bool rotated = false;
                        packer.pack_rotate(r.w,r.h,r.x,r.y,rotated);
                    }
                    else packer.pack(r.w,r.h,r.x,r.y);
                }
                elapsed = clock.getElapsedTime();
            }

            if(total_time + elapsed < limit)
                finishes += 1.0f;
            else
                finishes += (limit - total_time) / elapsed;
            total_time += elapsed;
        }

        if(finishes >= best_finishes)
        {
            best_finishes = finishes;
            best_size = cell_size;
        }
        if(!quiet) printf("%f\n", finishes);
    }
    if(!quiet) printf("Best cell size for %d, %d: %u\n", w, h, best_size);
    return best_size;
}

int main()
{
    unsigned window_size = 1920;
    sf::RenderWindow window(sf::VideoMode(window_size, window_size/2), "Pack Against The Machine");

    sf::Font font;

    if(!font.loadFromFile("Inconsolata/Inconsolata-Bold.ttf"))
        throw std::runtime_error("Failed to load Inconsolata");

    unsigned w = 16;
    unsigned h = 16;
    int splits = w * 2;

    bool at_once = true;
    bool allow_rotate = true;

    for(unsigned i = 1; ; i *= 2)
    {
        measure_rate(i, i, i*2, 100, at_once, allow_rotate);
    }

    /*
    int prev_optimal = 1;
    std::vector<std::pair<int, int>> results;
    for(int i = 1; i <= 24; ++i)
    {
        int n = 1<<(i>>1);
        if((i&1) == 1) n += n >> 1;

        std::vector<unsigned> candidates;
        for(int j = prev_optimal-3; j < prev_optimal + 6; ++j)
        {
            if(j < 1 || j > n) continue;
            candidates.push_back(j);
        }
        int optimal = search_optimal_tile_size(
            n, n, n*2, 2.0f, candidates, true, true, false
        );
        prev_optimal = optimal;
        results.push_back({n,optimal});
    }

    for(auto [n, cell]: results)
        printf("%d;%d\n", n, cell);
    return 0;
    */

    board pack_board(w, h);
    board orig_board(w, h);
    rect_packer packer(w, h, false);
    int pack_index = 0;
    int packed = 0;

    std::vector<board::rect> rects;
    std::vector<rect_packer::rect> rects_queue;

    auto reset = [&](){
        printf("Board reset!\n");
        pack_board.reset();
        orig_board.reset();
        packer.reset();
        pack_index = 0;
        packed = 0;

        rects = generate_guillotine_set(w, h, splits);
        shuffle(rects);

        if(at_once)
        {
            rects_queue.clear();
            for(unsigned i = 0; i < rects.size(); ++i)
                rects_queue.push_back({rects[i].w, rects[i].h});

            printf("Generating at-once solution!\n");
            packer.pack(rects_queue.data(), rects_queue.size(), allow_rotate);
            std::sort(
                rects_queue.begin(),
                rects_queue.end(),
                [](const rect_packer::rect& a, const rect_packer::rect& b){
                    return a.w + a.h > b.w + b.h;
                }
            );
            std::sort(
                rects.begin(),
                rects.end(),
                [](const board::rect& a, const board::rect& b){
                    return a.w + a.h > b.w + b.h;
                }
            );
        }

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
            bool success = false;
            if(at_once)
            {
                rect_packer::rect p = rects_queue[pack_index];
                r.w = p.w;
                r.h = p.h;
                r.x = p.x;
                r.y = p.y;
                rotated = p.rotated;
                success = r.x != -1;
            }
            else
            {
                success = allow_rotate ? 
                    packer.pack_rotate(r.w, r.h, r.x, r.y, rotated):
                    packer.pack(r.w, r.h, r.x, r.y);
            }

            if(success)
            {
                if(rotated)
                {
                    int tmp = r.w;
                    r.w = r.h;
                    r.h = tmp;
                }
                pack_board.place(r);
                packed++;
            }
        }
        pack_index++;
    };

    reset();

    window.setVerticalSyncEnabled(true);

    sf::Time total;
    sf::Time tick = sf::milliseconds(std::max(5000/splits,1));
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
                if(event.key.code == sf::Keyboard::D)
                {
                    w *= 2;
                    h *= 2;
                    pack_board.resize(w, h);
                    packer.enlarge(w, h);
                }
            }
        }

        if(!paused) total += clock.restart();
        if(pack_index < (int)rects.size())
        {
            while(total > tick)
            {
                total -= tick;
                step();
            }
            clock.restart();
        }
        else if(total > sf::milliseconds(4000))
        {
            if(packed != (int)rects.size())
                printf("Failure!\n");
            reset();
            total = sf::milliseconds(0);
            clock.restart();
        }

        window.clear(sf::Color(0x404040FF));
        sf::Vector2u sz = window.getSize();
        pack_board.draw(window, 10, 10, sz.x/2-20, sz.y-20, false, &font);
        orig_board.draw(window, sz.x/2+10, 10, sz.x/2-20, sz.y-20, true, &font);
        //pack_board.draw_debug_edges(window, packer, 10, 10, sz.x/2-20, sz.y-20);
        //pack_board.draw(window, 10, 10, sz.x/2-20, sz.y-20, false, nullptr);
        //orig_board.draw(window, sz.x/2+10, 10, sz.x/2-20, sz.y-20, false, nullptr);

        if(pack_index >= (int)rects.size())
        {
            sf::Text finished(
                "Rectangles: " +
                std::to_string(100.0*packed/(double)rects.size()) +
                "%\nArea: " + std::to_string(100.0*pack_board.coverage()) + "%",
                font, 32
            );
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
