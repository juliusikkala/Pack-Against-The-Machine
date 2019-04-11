#include "rect_packer.hh"
#include <vector>
#include <cstdio>


int main()
{
    rect_packer p(8, 8);
    int rects[13][2] = {
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

    for(int i = 0; i < 13; ++i)
    {
        int x, y;
        if(p.push(rects[i][0], rects[i][1], x, y))
        {
            printf("%d: (%d, %d)\n", i+1, x, y);
        }
        else
        {
            printf("Failed to push index %d!\n", i+1);
        }
    }

    return 0;
}
