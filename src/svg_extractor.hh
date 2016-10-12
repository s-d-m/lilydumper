#pragma once

#include <stdexcept>
#include <limits>
#include <vector>
#include <fstream>
#include "utils.hh"

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

struct staff_t
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

struct note_head_t
{
    // bounding box of the note in the svg file
    std::string id; // each note have an id (location in the source file)
    uint32_t left;
    uint32_t right;
    uint32_t top;
    uint32_t bottom;
    uint16_t bar_number;
};

struct svg_file_t
{
    const fs::path filename;
    std::vector<note_head_t> note_heads;
    std::vector<system_t> systems;
    std::vector<staff_t> staves;
};

svg_file_t get_svg_data(const fs::path& filename, std::ofstream& output_debug_file);
