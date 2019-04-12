#include "rect_packer.hh"
#include <vector>
#include <cstdio>


int main()
{
    rect_packer p(8, 8);
    rect_packer::rect rects[13] = {
        {2,3},
        {4,1},
        {4,2},
        {2,3},
        {4,1},
        {5,1},
        {1,3},
        {2,2},
        {3,3},
        {1,1},
        {2,4},
        {2,2},
        {2,1},
    };

    printf("push() one by one\n");
    for(int i = 0; i < 13; ++i)
    {
        int x, y;
        bool rotated;
        if(p.push_rotate(rects[i].w, rects[i].h, x, y, rotated))
        {
            printf("%d: (%d, %d) %s\n", i+1, x, y, rotated ? "(rotated)" : "");
        }
        else
        {
            printf("Failed to push index %d!\n", i+1);
        }
    }

    printf("push() all at once\n");
    p.reset();
    p.push(rects, sizeof(rects)/sizeof(*rects), true);

    for(int i = 0; i < 13; ++i)
    {
        if(rects[i].x >= 0)
        {
            printf(
                "%d: (%d, %d) %s\n",
                i+1, rects[i].x, rects[i].y,
                rects[i].rotated ? "(rotated)" : ""
            );
        }
        else
        {
            printf("Failed to push index %d!\n", i+1);
        }
    }

    printf("push_slow() all at once\n");
    p.reset();
    p.push_slow(rects, sizeof(rects)/sizeof(*rects), true);

    for(int i = 0; i < 13; ++i)
    {
        if(rects[i].x >= 0)
        {
            printf(
                "%d: (%d, %d) %s\n",
                i+1, rects[i].x, rects[i].y,
                rects[i].rotated ? "(rotated)" : ""
            );
        }
        else
        {
            printf("Failed to push index %d!\n", i+1);
        }
    }

    return 0;
}
