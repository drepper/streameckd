#include <stdexcept>

#include "ftlibrary.hh"

using namespace std::string_literals;


ftlibrary::ftlibrary()
{
  auto error = FT_Init_FreeType(&library);
  if (error)
    throw std::runtime_error("failed to initialize freetype2");
  fcconfig = FcInitLoadConfigAndFonts();
}


ftlibrary::~ftlibrary()
{
  FcConfigDestroy(fcconfig);
  FcFini();
  FT_Done_FreeType(library);
}


ftface& ftlibrary::find_font(const std::string& fontface)
{
  auto it = faces.find(fontface);
  if (it == faces.end()) {
    auto [it2,inserted] = faces.emplace(std::piecewise_construct, std::forward_as_tuple(fontface), std::forward_as_tuple(*this, fontface));
    it = it2;
  }
  return it->second;
}



ftface::ftface(ftlibrary& library_, const std::string& facename)
: library(library_)
{
  auto fname = find_face_path(facename);
  if (! fname.empty()) {
    auto error = FT_New_Face(library.library, fname.c_str(), 0, &face);
    if (! error) {
      use_kerning = FT_HAS_KERNING(face);
      return;
    }
  }
  throw std::runtime_error("cannot find font face "s + facename);
}


ftface::~ftface()
{
}


std::filesystem::path ftface::find_face_path(const std::string& facename)
{
  auto pat = FcNameParse((const FcChar8*) facename.c_str());
  FcConfigSubstitute(library.fcconfig, pat, FcMatchPattern);
  FcDefaultSubstitute(pat);

  std::filesystem::path res;
  FcResult fcres = FcResultNoMatch;
  if (auto font = FcFontMatch(library.fcconfig, pat, &fcres); font) {
    FcChar8* fname = NULL;
    if (FcPatternGetString(font, FC_FILE, 0, &fname) == FcResultMatch)
      res = (char*) fname;
    FcPatternDestroy(font);
  }
  FcPatternDestroy(pat);

  return res;
}


bool convert_string(const std::string& s, std::vector<utf8proc_int32_t>& wbuf)
{
  wbuf.resize(s.size() + 1);
  auto wlen = utf8proc_decompose(reinterpret_cast<const utf8proc_uint8_t*>(s.c_str()), s.size(), wbuf.data(), wbuf.size(), UTF8PROC_NULLTERM);
  if (wlen < 0)
    return false;
  wbuf.resize(wlen);
  return true;
}
