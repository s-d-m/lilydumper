#include <iostream>
#include <pugixml.hpp>

#include "svg_extractor.hh"

int main(int argc, const char * const * argv)
{
  if (argc == 1)
  {
    std::cerr << "Usage: " << argv[0] << " xml_file [xml_files... ]\n";
    return 1;
  }

  int res = 0;
  for (unsigned int i = 1; i < static_cast<decltype(i)>(argc); ++i)
  {
    pugi::xml_document doc;
    // the parse_eol option replaces \r\n and single \r by \n
    const auto parse_result = doc.load_file(argv[i], pugi::parse_minimal | pugi::parse_eol);
    if (parse_result.status not_eq pugi::status_ok)
    {
      std::cerr << "Error: Failed to parse file `" << argv[i] << "' ("
		<< parse_result.description() << ")\n";
      res = 1;
    }
    else
    {
      const auto staves = get_staves(doc);
      const auto systems = get_systems(doc, staves);
    }
  }

  return res;
}
