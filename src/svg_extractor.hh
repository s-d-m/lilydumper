#ifndef SVG_EXTRACTOR_HH_
#define SVG_EXTRACTOR_HH_

#include <stdexcept>
#include <limits>
#include <vector>
#include <pugixml.hpp> // definition of xml document (the svg files to read from)


// segments are used only to provide a full skyline.  Since a skyline
// is a contiguous line composed only of horizontal and vertical
// segments, let's just store the horizontal ones (y1 == y2) since
//
// 1/ I can find the vertical ones from there
// 2/ I'm not interested in the vertical part. The segments are only useful
//    to find out the top/bottom of a system
struct h_segment
{
    uint32_t x1;
    uint32_t x2;
    uint32_t y;
};

struct staff
{
    uint32_t x; // top left point. point (0,0) represents the top left corner of the paper
    uint32_t y;
    uint32_t width;
    uint32_t height;
    uint32_t top_skyline;
    uint32_t bottom_skyline;
    std::vector<h_segment> full_top_skyline;
    std::vector<h_segment> full_bottom_skyline;
};


// a system is a set of contiguous staves on the music sheet.
//
struct system_t
{
    uint8_t first;
    uint8_t last;
};

// a staff represents the surface of a staff on the music sheet.
// it is represented by the coordinates on the music sheet.
std::vector<staff> get_staves(const pugi::xml_document& svg_file);


// group staves into system
std::vector<system_t> get_systems(const pugi::xml_document& svg_file,
				const std::vector<staff>& staves);


#endif /* SVG_EXTRACTOR_HH_ */
