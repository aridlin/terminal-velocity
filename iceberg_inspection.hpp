#pragma once
#include "iceberg_types.hpp"
#include "iceberg_traversal_art.hpp"
#include <algorithm>
#include <sstream>

namespace iceberg {
inline int cell_length(const std::string &s) {
  return int(std::count_if(s.begin(),s.end(),[](unsigned char c){return (c&0xc0)!=0x80;}));
}
inline std::vector<std::string> inspection_wrap(const std::string &text,int width) {
  std::istringstream input(text);std::string word,line;std::vector<std::string> out;
  while(input>>word){if(!line.empty()&&cell_length(line)+cell_length(word)+1>width){out.push_back(line);line.clear();}if(!line.empty())line+=' ';line+=word;}
  if(!line.empty())out.push_back(line);
  return out;
}
// These are separately drawn close-up studies, not enlarged world sprites.
// Every study fits 28 x 12 cells, including the narrow terminal layout.
inline std::vector<std::string> inspection_art(InspectionArt kind,int detail=0) {
  const char *drawing="";
  switch(kind) {
  case InspectionArt::Achievement: break;
  case InspectionArt::Parcel:drawing=R"ART(       ╭────┬────╮
      ╱     │     ╲
     ╱  ░░░ │ ░░░  ╲
    ╱───────┼───────╲
   │ ╲      │     ╱ │
   │  ╲  ╭──┼──╮ ╱  │
   │   ╳ │11│ ▰│╳   │
   │  ╱  ╰──┼──╯ ╲  │
   │ ╱  ·   │  ·  ╲ │
   │╱_______│______╲│
   ╰───────╲╱───────╯
      ░░░░░░░░░░░░)ART";break;
  case InspectionArt::Valve:drawing=R"ART(          ╭──────╮
       ╭──╯  ║   ╰──╮
      ╱  ╲   ║   ╱   ╲
     │    ╲  ║  ╱     │
═════╪═════╲╭╨╮╱══════╪═════
 ░░░ ║──────┤●├───────║ ░░░
═════╪═════╱╰╥╯╲══════╪═════
     │    ╱  ║  ╲     │
      ╲  ╱   ║   ╲   ╱
       ╰──╮  ║   ╭──╯
          ╰──╥───╯
          ───╨───)ART";break;
  case InspectionArt::Signal:drawing=R"ART(        ╭──────────╮
       ╱____________╲
       │  ╭──────╮  │
       │  │ ░██░ │  │
       │  ╰──────╯  │
       ├────────────┤
       │  ╭──────╮  │
       │  │  ◉   │  │
       │  ╰──────╯  │
       ╰─────╥──────╯
         ╭───╫───╮
         ╘═══╩═══╛)ART";break;
  case InspectionArt::Address:drawing=R"ART(   ╭────────────────────╮
   │○ ╔══════════════╗ ○│
   │  ║ ░░           ║  │
   │  ║    ▄█   ▄█   ║  │
   │  ║     █    █   ║  │
   │  ║     █    █   ║  │
   │  ║    ▀▀▀  ▀▀▀  ║  │
   │  ║          ░░  ║  │
   │○ ╚══════════════╝ ○│
   ╰────────────────────╯
       ╱  ·   ╱  ·
      ╱      ╱)ART";break;
  case InspectionArt::File:drawing=R"ART(      ╭────────╮
   ╭──╯        ╰────────╮
   │ ╭──────────────╮  │╲
   │ │  A / 00 07   │  │ │
   │ ╰──────────────╯  │ │
   │ ≋≋≋≋≋≋≋≋≋≋        │ │
───┼─────────╳─────────┼─┤
   │     ╱   │   ╲     │ │
   │    ╱    │    ╲    │ │
   │ ░░      │     ░░  │ │
   ╰─────────┴─────────╯ │
    ╰───────────────────╯)ART";break;
  case InspectionArt::Seeds:drawing=R"ART(         ╔════════╗
         ╠╤╤╤╤╤╤╤╤╣
        ╱╰────────╯╲
       ╱  ╱        ╲╲
      │  ╱   ╭╮     │
      │ │   ╱│╰╮    │
      │ │  ╰─┼─╯    │
      │ │    │      │
      │  · ▰ · ▰ ·  │
      │ ▰ · ▰ · ▰ · │
      ╰─────────────╯
        ░░░░░░░░░░░)ART";break;
  case InspectionArt::Speaker:drawing=R"ART(    ╭──────────────────╮
    │○ ╭────────────╮ ○│
    │  │ ╱╱╱╱╱╱╱╱╱╱ │  │
    │  │╱╱╱╱╭──╮╱╱╱╱│  │
    │  │╱╱╱╱│● │╱╱╱╱│  │
    │  │ ╱╱╱╰──╯╱╱╱ │  │
    │  ╰────────────╯  │
    ├──────────────────┤
    │  │ │┃│ ││┃│ ││   │
    │  ╭──╮     ▣      │
    │○ ╰─╱╯  07     ○  │
    ╰─────┬──────┬─────╯)ART";break;
  case InspectionArt::Thermometer:drawing=R"ART(    ╔══════════════════╗
    ║  ↑     ↑     ↑   ║
    ╠══════════════════╣
    ║ ╭──────────────╮ ║
    ║ │  ╱╲  ╭─╮ ╱╲ │ ║
    ║ │ ╱  ╲ │█├─╲  │ ║
    ║ │      │█├─   │ ║
    ║ │      │█├─   │ ║
    ║ │      (█)    │ ║
    ║ │ ╲  ╱ ╰─╯ ╲╱ │ ║
    ║ ╰──────────────╯ ║
    ╚══╧══════════╧════╝)ART";break;
  case InspectionArt::Sign:drawing=R"ART(    ╭───────────────────╮
    │● ┌─────────────┐ ●│
    │  │  ────────   │  │
    │  │  ─────────  │  │
    │  │  ──────     │  │
    │  └─────────────┘  │
    │●  ░░░░░░░░░░░    ●│
    ╰─────────┬┬────────╯
              ││
              ││
          ╱───┴┴───╲
         ╱__________╲)ART";break;
  case InspectionArt::Dedication:drawing=R"ART(  ╔══════════════════════╗
  ║ ░╭────────────────╮░ ║
  ║  │   JAN PAWEŁ II │  ║
  ║  │       ╬        │  ║
  ║  │    ▄████▄      │  ║
  ║  │   ▐█ ▄▄ █▌     │  ║
  ║  │    ▀█▄▄█▀      │  ║
  ║  │   ▄█╱╲╱╲█▄     │  ║
  ║  │                │  ║
  ║  │   21,37 MW     │  ║
  ║ ░╰────────────────╯░ ║
  ╚══════════════════════╝)ART";break;
  case InspectionArt::Board:drawing=R"ART( ╭────────────────────────╮
 │○ ┌──────┐ ┌─────────┐ ○│
 │  │≋≋≋≋  │ │ ▫───▫   │  │
 │  │≋≋≋≋≋ │ │     │   │  │
 │  └──────┘ │ ▫───▫──▫│  │
 │ ╭────────╮└─────────┘  │
 │ │≋≋≋≋≋  ▰│ ┌────────┐  │
 │ │≋≋≋≋≋≋  │ │ ≋≋≋≋   │  │
 │ ╰────────╯ └────────┘  │
 │○                      ○│
 ╰───┬┬──────────────┬┬───╯
     ╧╧              ╧╧)ART";break;
  case InspectionArt::Door:drawing=R"ART(     ╔════════════════╗
     ║╭──────────────╮║
     ║│ ╭──────────╮ │║
     ║│ │  ░░░░░░  │ │║
     ║│ ╰──────────╯ │║
     ║│              │║
     ║│  ╭────────╮  │║
     ║│  │≋≋≋≋≋≋  │ ▣│║
     ║│  ╰────────╯  │║
     ║│              │║
     ║╰──────────────╯║
   ══╩════════════════╩══)ART";break;
  case InspectionArt::Coin:drawing=R"ART(        ▄▄▄▄▄▄▄▄▄▄▄
     ▄██▀         ▀██▄
    ██╱  · · · · ·  ╲██
   ▐█│  ╭─────────╮  │█▌
   █▌│  │╲       ╱│  │▐█
   █▌│  │ ╲  ◇  ╱ │  │▐█
   █▌│  │  ╲___╱  │  │▐█
   ▐█│  ╰─────────╯  │█▌
    ██╲   · · · ·   ╱██
     ▀██▄         ▄██▀
        ▀▀▀▀▀▀▀▀▀▀▀
          ░░░░░░░)ART";break;
  case InspectionArt::Refill:drawing=R"ART(          ╔══════╗
         ╭╨──────╨╮
        ╱ ╱       ╲
       │ ╱ ╭────╮  │
       │ │ │ ╲  │  │
       │ │ │  ╲ │  │
       │ │ │ ╱  │  │
       │ │ │╱   │  │
       │   ╰────╯  │
        ╲_________╱
         ╘╤═════╤╛
          ┴     ┴)ART";break;
  case InspectionArt::Lamp:drawing=R"ART(            ╭──╮
         ╭──┴──┴──╮
        ╱__________╲
       ╭┬──────────┬╮
       ││ ╲   ╱    ││
       ││  ╲ ╱  ╱  ││
       ││   ▐█▌    ││
       ││  ╱ █ ╲   ││
       ╰┴────╥─────┴╯
          ╭──╨──╮
          ╰──╥──╯
          ═══╩═══)ART";break;
  case InspectionArt::Delivery:drawing=R"ART(   ╭─────────────────────╮
   │○  ╭─────────────╮  ○│
   │   │  ╱╲  ╱╲  ╱╲ │   │
   │   │ ─────────── │   │
   │   ╰─────────────╯   │
   ├─────────────────────┤
   │ ╭──────────╮ ╭───╮ │
   │ │≋≋≋≋≋≋≋   │ │ ▣ │ │
   │ ╰──────────╯ ╰───╯ │
   │○    ╭──────╮      ○│
   ╰─────┴──────┴────────╯
       ╧╧          ╧╧)ART";break;
  case InspectionArt::Traversal: return traversal_art(TraversalStudy(detail));
  case InspectionArt::Chart:
    return {"  ╭──────┬────────────────╮", "  │ ▥ ▥  ├─╮     ≈≈       │",
      "  │ ▥ ▥  │ ╰──╮    ≈≈     │", "  ├──────╯ ╭──╯           │",
      "  │    ╭───╯     01       │", "  │ ═══╯ ······· ◆        │",
      "  │              ·        │", "  │      02 ·····┘    ≈   │",
      "  │      ◆ ·······╮       │", "  │   ≈≈          03      │",
      detail?"  │               ⚑       │":"  │               ◇       │",
      "  ╰───────────────────────╯"};
  case InspectionArt::Character: {
    static const char *heads[]={
      "      ▄██▀▀▀▀▀▀▀██▄", "       ▄████████▄", "      ▄████▣█████▄", "      ▄███▀▀▀████▄",
      "         ▄████▄", "       ▄▄❧▄▄▄▄▄▄", "       ▄▄██████▄", "      ╔══════════╗"};
    static const char *brows[]={"      ██ ▄    ▄ ██","       █ ━    ━ █","      ▀█ ◉    ◉ █▀","      ██ ▣━━━━▣ ██","       █ ▣━━━━▣ █","       █ ◡    ◡ █","       █ ▰    ▰ █","      ║ ▤▤▤▤▤▤▤▤ ║"};
    if(detail==7)return inspection_art(InspectionArt::Speaker);
    return {heads[std::clamp(detail,0,7)],"      ▐██        ██▌",brows[std::clamp(detail,0,7)],
      "       █    ▵   █",detail==1?"       █▄██████▄█":"       █   ──   █","        ▀█▄▄▄▄█▀",
      "     ▄███╲    ╱███▄","   ▄████  ╲◇╱  ████▄","  ▐██ ▣█   │   █▣ ███▌",
      detail==4?"  ███  ╭────────╮ ████":"  ███  ║   │   ║  ████",
      detail==4?"  ███  │ ≋≋≋≋≋≋ │ ████":"  ███  ║   ▣   ║  ████","  ▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀"};
  }
  }
  std::vector<std::string> out;std::istringstream input(drawing);std::string line;
  while(std::getline(input,line))out.push_back(line);
  return out;
}
struct InspectionLayout {
  int art_x=2,art_y=5,text_x=2,text_y=18,text_width=30,text_rows=3;
  std::vector<std::string> title,body;
  int pages=1;
  // Offsets into the same whitespace-normalized text at any wrap width.
  std::vector<size_t> page_starts{0};
  int page_at(size_t offset) const {
    return int(std::upper_bound(page_starts.begin(),page_starts.end(),offset)-page_starts.begin())-1;
  }
};
inline InspectionLayout inspection_layout(const std::string &title,const std::string &body,int cols,int rows,bool achievement=false) {
  InspectionLayout l;l.title=inspection_wrap(title,cols-4);l.art_y=3+int(l.title.size())+1;
  if(cols>=56){l.art_x=2;l.text_x=33;l.text_y=l.art_y;l.text_width=cols-35;l.text_rows=rows-3-l.text_y;}
  else {l.art_x=(cols-28)/2;l.text_x=2;l.text_y=l.art_y+13;l.text_width=cols-4;l.text_rows=rows-3-l.text_y;}
  if(achievement){
    l.art_x=2;l.text_x=11;l.text_y=l.art_y;
    l.text_width=cols-13;l.text_rows=rows-3-l.text_y;
  }
  l.text_rows=std::max(1,l.text_rows);l.body=inspection_wrap(body,l.text_width);
  size_t offset=0;
  for(size_t line=0;line<l.body.size();++line) {
    if(line>0&&line%size_t(l.text_rows)==0)l.page_starts.push_back(offset);
    offset+=l.body[line].size()+1;
  }
  l.pages=int(l.page_starts.size());return l;
}
}
