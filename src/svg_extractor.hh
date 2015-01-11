#ifndef SVG_EXTRACTOR_HH_
#define SVG_EXTRACTOR_HH_

#include <stdexcept>
#include <limits>
#include <vector>
#include <pugixml.hpp> // definition of xml document (the svg files to read from)


// segments are either horizontal or vertical lines.
// Therefore x1 == x2, or y1 == y2
struct segment
{
    uint64_t x1;
    uint64_t y1;
    uint64_t x2;
    uint64_t y2;

    // initialize with invalid values by default to make bugs easier
    // to triggers, thus also to fix.
    segment()
      : x1 (std::numeric_limits<decltype(segment::x1)>::max())
      , y1 (std::numeric_limits<decltype(segment::y1)>::max())
      , x2 (std::numeric_limits<decltype(segment::x2)>::max())
      , y2 (std::numeric_limits<decltype(segment::y2)>::max())
    {
    }

    segment(decltype(segment::x1) _x1,
	    decltype(segment::y1) _y1,
	    decltype(segment::x2) _x2,
	    decltype(segment::y2) _y2)
      : x1 (_x1)
      , y1 (_y1)
      , x2 (_x2)
      , y2 (_y2)
    {
      // sanity check:
      if ((x1 != x2) and (y1 != y2))
      {
	throw std::runtime_error(std::string{
	    "Error: a segment should be either horizontal or vertical.\n"
	    "       in this case we found a segment with the following coordinates:\n      "
	    " (x1: " } + std::to_string(x1)
	  + ", y1: "   + std::to_string(y1) + "),"
	  + " (x2: "   + std::to_string(x2)
	  + ", y2: "   + std::to_string(y2) + ")");
      }
    }
};

struct staff
{
    uint32_t x; // top left point. point (0,0) represents the top left corner of the paper
    uint32_t y;
    uint32_t width;
    uint32_t height;
    uint32_t top_skyline;
    uint32_t bottom_skyline;
    std::vector<segment> full_top_skyline;
    std::vector<segment> full_bottom_skyline;
};


// a staff represents the surface of a staff on the music sheet.
// it is represented by the coordinates on the music sheet.
std::vector<staff> get_staves(const pugi::xml_document& svg_file);

#endif /* SVG_EXTRACTOR_HH_ */
