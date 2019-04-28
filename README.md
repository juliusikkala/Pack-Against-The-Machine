Pack Against The Machine
========================

This is a rectangle packing algorithm under the MIT License.
You can incorporate it in your project simply by copying `rect\_packer.cc` and
`rect\_packer.hh`. It is written in C++11 and should be portable (no promises 
or warranties though).

(The repo name came from a puzzle game idea that I didn't actually end up making
 :P)

It allows you to:
* Pack 1 to N rectangles at once, more at once => better packing.
  * `bool rect_packer::pack(int w, int h, int& x, int& y)`
  * `bool rect_packer::pack_rotate(int w, int h, int& x, int& y, bool& rotated)`
  * `int rect_packer::pack(rect* rects, size_t count, bool allow_rotation = false)`
* Enlarge packing area without clearing already packed rectangles
  * `void rect_packer::enlarge(int w, int h)`
* Achieve good packing taking into account packing area resizing.
  * `void rect_packer::set_open(bool open)`
  * If "open", packing results are worse unless the area is enlargened when out
    of space. You can disable it at any time (e.g. once you have hit the maximum
    packing area size.)
* Choose whether to allow rectangle rotation when packing.
  (allow rotation => better packing)
  * This is the difference between `rect_packer::pack` and
    `rect_packer::pack_rotate`
  * The array version of `rect_packer::pack` has a parameter for this,
    `allow_rotation`
* Adjust the internals.
  * `void rect_packer::set_cell_size(int cell_size = -1)`
  * You don't need to think about this really. It only affects performance
    slightly, and the default automatic mode is pretty good.

Compared to [stb\_rect\_pack.h](https://github.com/nothings/stb/blob/master/stb_rect_pack.h),
this algorithm is:
* Consistently better at packing.
  * See `glyph_test()` in main.cc.
  * One-by-one packing w/o rotation: usually 1-4% more rects packed.
  * All-at-once packing w/o rotation: usually 1-15% more rects packed.
  * Of course, heavily dependent on use case (example mimics text glyphs)
  * Difference is usually very small when packing is easy (lots of small rects
    in a massive area), gets bigger when rectangle size is larger.
* Slower, it gets worse when packing smaller tiles into a larger area.
  * Extreme example: packing ~7000 rects into a 2048x2048 area takes ~3.5
    seconds on my PC, whereas stb takes just 7 ms! 
* More flexible, because it allows resizing the packing area and rotating
  rectangles.

In short, this algorithm is probably better suited for packing lightmaps or
texture atlases than text glyphs. Anyhow, this is better than `stb_rect_pack.h`
unless you have a real-time time constraint.

# Algorithm

This algorithm searches for a place where the packed rectangle would be
"minimally exposed", or exposes the minimum number of its edges to the free
area. This is equivalent to attempting to maximize contact area with already
placed rectangles. There are lots of small optimizations in the codebase so that
the search wouldn't be annoyingly slow, but it still takes plenty of time due to
how extensive the search is.

In practice, the implementation stores the edges of the free area and scores
potential candidate places that lie along them. For scoring, there is an
acceleration structure that limits the number of edges that need to be
considered in scoring.
