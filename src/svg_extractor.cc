#include <cstring>
#include <cstdlib>
#include <stdexcept>
#include "svg_extractor.hh"

static
inline bool is_digit(char c)
{
  return (c >= '0') and (c <= '9');
}

static inline
bool begins_by(const char* s1, const char* s2)
{
  return std::strncmp(s1, s2, std::strlen(s2)) == 0;
}

static inline
bool str_equal(const char* s1, const char* s2)
{
  return std::strcmp(s1, s2) == 0;
}

// str must be of format "-?[[:digit:]]+\.[[:digit:]]{4}"
static bool is_valid_number(const char* str)
{
  if (str == nullptr)
  {
    return false;
  }

  unsigned int pos = 0;
  if (str[0] == '-')
  {
    pos++;
  }

  do
  {
    // must start with at least one digit
    if (not is_digit(str[pos]))
    {
      return false;
    }

    pos++;
  } while (str[pos] != '.');

  pos++;

  // must have 4 digits, and then finish
  for (unsigned int i = 0; i < 4; i++)
  {
    // must start with at least one digit
    if (not is_digit(str[pos]))
    {
      return false;
    }

    pos++;
  }

  // must be finished now
  return (str[pos] == '\0');
}

#define JOIN(a,b) a#b
#define JOIN2(a,b) JOIN(a,b)

template <typename T>
static T to_int_decimal_shift(const char* str)
{
  // sanity check:
  if (not is_valid_number(str))
  {
    throw std::runtime_error(JOIN2("Error: invalid param in function ", __func__ ));
  }

  const bool is_neg = (str[0] == '-');
  const uint8_t pos = (is_neg ? 1 : 0);
  char* point_pos;
  const T int_part = static_cast<T>( std::strtoul(&(str[pos]), &point_pos, 10) );
  const T dec_part = static_cast<T>( std::strtoul(&(point_pos[1]), nullptr, 10) );

  const T num = 10000 * int_part + dec_part;
  return is_neg ? -num : num;
}

// precondition svg_file is already parsed
std::vector<staff> get_staves(const pugi::xml_document& svg_file)
{
  std::vector<staff> res;

  struct line
  {
      uint32_t x1;
      uint32_t y1;
      uint32_t x2;
      uint32_t y2;
  };

  std::vector<line> lines;

  // staves are composed by 5 equaly distanced lines,
  // these lines are not part of a <g color=...>...</g> node
  // Xpath -> '//*[not(self::g)]/line'
  for (const auto& xpath_node : svg_file.select_nodes("//*[not(self::g)]/line"))
  {
    const std::string transform { xpath_node.node().attribute("transform").value() };
    const auto x1 = xpath_node.node().attribute("x1").value();
    const auto x2 = xpath_node.node().attribute("x2").value();
    const auto y1 = xpath_node.node().attribute("y1").value();
    const auto y2 = xpath_node.node().attribute("y2").value();

    // the lines composing the staves all have y1 == y2 (of course as
    // they are horizontal). also, they have y1 == "-0.0000".
    if ((not str_equal(y1, "-0.0000")) or (not str_equal(y2, "-0.0000")))
    {
      throw std::runtime_error("Error: invalid input file. Unexpected value for y coordinate");
    }

    const auto translate_str = "translate(";
    if (not begins_by(transform.c_str(), translate_str))
    {
      throw std::runtime_error("Error: lines must have a translate transformation");
    }

    // x_tr and y_tr are initialised with the part inside translate=(...)
    // e.g. if transform == "translate(14.2264, 33.0230)"
    // x_tr will be "14.2264" and y_tr "33.0230"
    const auto separation_pos = transform.find(", ");
    if (separation_pos == std::string::npos)
    {
      throw std::runtime_error("Error: coordinates in translate must be separated by ', '");
    }

    const std::string x_tr (std::begin(transform) + static_cast<int>(std::strlen(translate_str)),
			    std::begin(transform) + static_cast<int>(separation_pos));
    const std::string y_tr (std::begin(transform) + static_cast<int>(separation_pos + 2) /* 2 for COMMA SPACE */,
			    std::end(transform) - 1 /* -1 to remove the ')' */);

    // one could see a potential problem in case of invalid in translate
    // e.g. with "translate(14.2264, ", or "translate(14.2264, 33.0230[dd" y_tr
    // would have a wrong value. But this will be checked by the
    // to_int_decimal_shift function
    const auto x_tr_value = to_int_decimal_shift<decltype(line::x1)>(x_tr.c_str());
    const auto y_tr_value = to_int_decimal_shift<decltype(line::y1)>(y_tr.c_str());

    lines.emplace_back(line{
	  .x1 = (to_int_decimal_shift<decltype(line::x1)>(x1) + x_tr_value),
	  .y1 = y_tr_value,
	  .x2 = (to_int_decimal_shift<decltype(line::x2)>(x2) + x_tr_value),
	  .y2 = y_tr_value });
  }




  // g nodes contains the skylines
  // Xpath -> '//g/line'


  return res;
}
