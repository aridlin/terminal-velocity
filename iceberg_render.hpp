// RenderGrid member functions: one Unicode cell framebuffer for terminal and SDL.
  uint32_t iceberg_base_background() const {
    if(!grid_.adventure_level)return 0x191418;
    if(grid_.room_role==iceberg::RoomRole::Secret)return 0x090e13;
    static const uint32_t colors[]={0x101923,0x20181b,0x1c1825,0x101622,0x11201d,0x161c28};
    return colors[std::clamp(grid_.district,0,5)];
  }
  static uint32_t blend_light(uint32_t from,uint32_t to,double amount) {
    uint32_t result=0;
    for(int shift:{0,8,16})result|=uint32_t(((from>>shift)&255)*(1-amount)+((to>>shift)&255)*amount)<<shift;
    return result;
  }
  uint32_t iceberg_light(double wx,double wy) const {
    uint32_t base=iceberg_base_background();
    if(grid_.room_role==iceberg::RoomRole::Secret)return base;
    double warm=0,cold=0;
    for(auto [x,y]:grid_.checkpoint_points) {
      double dx=wx-x,dy=wy-(y-.8);
      warm=std::max(warm,std::max(0.,1-(dx*dx+dy*dy*.7)/23));
    }
    for(size_t i=0;i<grid_.refills.size();++i)if(state_.refill_cooldown[i]<=0) {
      auto [x,y]=grid_.refills[i];double dx=wx-x,dy=wy-y;
      cold=std::max(cold,std::max(0.,1-(dx*dx+dy*dy)/8));
    }
    warm=std::floor(warm*6)/6;cold=std::floor(cold*4)/4;
    return blend_light(blend_light(base,0x68502b,warm*.23),0x214d58,cold*.25);
  }
  std::string tr(const char *pl,const char *en) const {
    return iceberg::tr(state_.adventure.language,pl,en);
  }
  std::string save_failure_hint() const {
    return tr("Błąd zapisu — postęp pozostaje w tej sesji.","Save error — progress remains in this session.");
  }
  void paint_text(int row,int col,const std::string &s,uint32_t rgb,bool bold=false) {
    for(auto ch:glyphs(s))put(row,col++,{ch,Color::Text,false,bold,0,rgb});
  }
  void world_text(double x,double y,const std::string &s,uint32_t rgb) {
    auto [sx,sy]=screen(x,y);
    if(sy>=4&&sy<rows_-3)paint_text(sy,sx,s,rgb);
  }
  std::vector<std::string> wrap(const std::string &s,int width) const {
    std::vector<std::string> result;
    std::istringstream words(s);std::string word,line;
    while(words>>word) {
      if(!line.empty()&&int(glyphs(line).size()+1+glyphs(word).size())>width){result.push_back(line);line.clear();}
      if(!line.empty())line+=' ';
      line+=word;
    }
    if(!line.empty())result.push_back(line);
    return result;
  }
  void frame(int top,int left,int width,int height,uint32_t rgb) {
    fill(top,left,width,height,Color::Default);
    paint_text(top,left,"╭",rgb);paint_text(top,left+width-1,"╮",rgb);
    paint_text(top+height-1,left,"╰",rgb);paint_text(top+height-1,left+width-1,"╯",rgb);
    for(int x=1;x<width-1;++x){paint_text(top,left+x,"─",rgb);paint_text(top+height-1,left+x,"─",rgb);}
    for(int y=1;y<height-1;++y){paint_text(top+y,left,"│",rgb);paint_text(top+y,left+width-1,"│",rgb);}
  }
  Cell iceberg_background(double wx,double wy,int x,int y) const {
    const auto &d=iceberg::Districts[size_t(grid_.district)];
    int a=int(std::floor(wx*.65)),b=int(std::floor(wy));
    uint32_t shade=d.shadow;
    auto c=[&](const char *glyph,uint32_t color){return Cell{glyph,Color::Decor,false,false,0,color};};
    // Distant architecture moves more slowly than foreground geometry.
    if(grid_.room_role==iceberg::RoomRole::Settlement) {
      if(b>=31&&b<49) {
        if(a%14==0)return c("│",shade);
        if(b==33||b==40)return c("─",shade);
        if(b%7==2&&a%7==3)return c("▪",d.stone);
      }
    }
    switch(grid_.district) {
    case 0:
      if(b>42&&(a*7+b+int(state_.elapsed*2))%79==0)return c("∘",d.stone);
      if(b>48&&a%9==0)return c("║",shade);
      if(b>55&&((a+b+int(state_.elapsed*.7))%11==0))return c("∿",d.stone);
      if(b==39&&a%17<8)return c("─",shade);
      break;
    case 1:
      if(a%18<3&&(b+int(state_.elapsed*3))%9==0)return c("░",shade);
      if(a%18==0||a%18==1)return c("│",shade);
      if(b%11==0)return c("═",shade);
      if(a%18==8&&b%11==5)return c("◉",d.stone);
      break;
    case 2:
      if(b%14==0&&a%23<17)return c("▁",shade);
      if(a%23==0)return c("┊",shade);
      if(b%14==7&&a%5==1)return c("▪",d.stone);
      break;
    case 3:
      if(a%23==0)return c("│",shade);
      if(b%13==0&&a%23<16)return c("─",shade);
      if((b%13==4||b%13==8)&&a%23>=3&&a%23<13&&a%2)return c("▥",shade);
      break;
    case 4:
      if(a%21==0)return c("│",shade);
      if(a%21==10&&b%16==3)return c("┼",shade);
      if((a-b)%19==0)return c("╱",shade);
      break;
    default:
      if((a+b)%27==0)return c("╱",shade);
      if((a-b+300)%29==0)return c("╲",shade);
      break;
    }
    if((x*37+y*19+int(state_.elapsed*.5))%251==0)return c("·",d.stone);
    if(wy<6+std::sin(wx*.06)*2)return c("░",shade);
    return {};
  }
  void world_art(double x,double y,const std::vector<std::string> &lines,uint32_t rgb,bool opaque=false) {
    // Decoration is behind collision and characters, even when the camera
    // exposes only a few cells of a large architectural landmark.
    auto [origin_x,origin_y]=screen(x,y);
    if(origin_x>=cols_||origin_y>=rows_-3||origin_y+int(lines.size())<=4)return;
    for(size_t r=0;r<lines.size();++r) {
      int sy=origin_y+int(r);
      if(sy<4||sy>=rows_-3)continue;
      // UTF-8 byte length is a conservative upper bound on occupied cells.
      if(origin_x+int(lines[r].size())<=0)continue;
      auto glyph=glyphs(lines[r]);
      for(size_t c=0;c<glyph.size();++c) {
        int sx=origin_x+int(c);
        if(sx<0||sx>=cols_||(!opaque&&glyph[c]==" "))continue;
        double wx=x+c*scale_,wy=y+r*aspect_*scale_;
        if(grid_.solid_at(int(std::floor(wx)),int(std::floor(wy)))||grid_.hazard_at(int(std::floor(wx)),int(std::floor(wy)))||
           state_.player.rect().intersects({wx,wy,scale_,aspect_*scale_}))continue;
        put(sy,sx,{glyph[c],Color::Text,false,false,0,rgb});
      }
    }
  }
#include "iceberg_revision_scenery.hpp"
#include "iceberg_scenery.hpp"
#include "iceberg_expedition_scenery.hpp"
#include "iceberg_sprites.hpp"
#include "iceberg_props.hpp"
#include "iceberg_jubilee_scenery.hpp"
#include "iceberg_estate_scenery.hpp"
#include "iceberg_archive_scenery.hpp"
#include "iceberg_gardens_scenery.hpp"
#include "iceberg_crown_scenery.hpp"
  void draw_iceberg_landmarks() {
    const auto &d=iceberg::Districts[size_t(grid_.district)];
    if(grid_.template_id==330){draw_epilogue_landmarks();return;}
    if(iceworld::harbor_v4_room(grid_.template_id)||iceworld::harbor_v4_mission(grid_.template_id)||grid_.template_id==600||grid_.template_id==601){draw_harbor_landmarks();return;}
    if(grid_.district==5&&grid_.room_role!=iceberg::RoomRole::Secret){draw_crown_landmarks();return;}
    if(grid_.template_id==303){draw_jubilee_monument();return;}
    if(grid_.district==2&&grid_.room_role!=iceberg::RoomRole::Secret){draw_estate_landmarks();return;}
    if(grid_.district==3&&grid_.room_role!=iceberg::RoomRole::Secret){draw_archive_landmarks();return;}
    if(grid_.district==4&&grid_.room_role!=iceberg::RoomRole::Secret){draw_garden_landmarks();return;}
    bool secret=grid_.room_role==iceberg::RoomRole::Secret;
    if(secret) {
      if(grid_.template_id==200) {
        for(int i=0;i<4;++i)world_art(21+i*12,39.5,{
          "┌───────────┐", "│ ╭───╮     │", "│ │ ░ │  ▯  │", "│ ╰───╯     │", "│      ○    │", "│           │", "└───────────┘"},0x52626a);
        world_art(22,36,{"11   12   13   15","╶────────────────╴"},0x87989c);
        world_art(64,44,{"╭──╮","│≋ │","╰──╯"},0xaca79b);
      } else if(grid_.template_id==201) {
        world_art(46,37,{"┌──────────────┐","│ ▤▤▤▤▤▤▤▤▤▤▤▤ │","│ ▤▤▤▤▤▤▤▤▤▤▤▤ │","│              │",(state_.adventure.discoveries&4)?"│  41  /  41   │":"│  41  /  40   │","│              │","│    ○    ▣    │","└──────┬───────┘","       │"},0x7e939b);
        world_art(8,32,{"┌─────────────────────────────────────────────────────────────────┐","│                                                                 │"},0x28363c);
      } else {
        world_art(35,30,{"       ╭─────────────────────────╮","   ╭───┤   ▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒   ├───╮","   │   │   ▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒   │   │","═══╡   │          00:07          │   ╞═══","   │   │                         │   │","   ╰───┤     ┌─────────────┐     ├───╯","       ╰─────┴─────────────┴─────╯"},0x55646b);
      }
      if(grid_.template_id==202&&(state_.adventure.discoveries&4)&&std::abs(state_.player.x-18)>3)
        world_art(18,42,{" ▄██▄ "," █  █ "," ▐██▌ "," ▐  ▌ "," ╱  ╲ "},0x3b4a51);
      world_art(4,42,{"┌───┐","│   │","│  ○│","│   │","└───┘"},d.stone);
      return;
    }
    if(grid_.template_id==300) {
      if(!(grid_.switch_flags&2))draw_dock_tram(137,40);
      world_art(137,40+5*aspect_*scale_,{"══════════════════════════════════","  ·   ·   ·   ·   ·   ·   ·   ·"},d.stone);
      world_art(94,29,{"     ╭───╮","═════╡ ◉ ╞═════","     ╰─┬─╯","       │","       │"},d.stone);
    } else if(grid_.template_id==301) {
      world_art(4,44,{"╭──────────────────╮",tr("│↑ NIE PRZEWRACAĆ ↑│","│ ↑ KEEP UPRIGHT ↑ │"),"╰──────────────────╯"},d.accent);
      world_art(192,21,{"     ╔════════════╗","═════╣ ░░░░░░░░░░ ╠═════","     ║    P 01    ║","     ╚════╤══╤════╝","          │  │"},d.stone);
    } else if(grid_.template_id==302) {
      for(auto [x,y]:{std::pair{56.,40.},std::pair{114.,34.}})
        world_art(x,y,{"   ╭──────────────╮","═══╡  ╭────────╮  ╞═══","   │  │ ╲ │ ╱  │  │","   │  │ ─ ◉ ─  │  │","   │  │ ╱ │ ╲  │  │","   │  ╰────────╯  │","   ╰──────┬───────╯","          ║"},d.stone);
    }
    if(grid_.room_role==iceberg::RoomRole::Settlement&&grid_.district==1) {
      draw_scenery_pipe({{0,37.6},{21,37.6},{21,33.6},{40.2,33.6},{40.2,40}},0x626c73);
      draw_scenery_pipe({{48,43},{48,32},{65,32},{65,36},{94,36},{94,49}},0x606970);
      world_art(3,42,{"   ┌──────────────────────────────┐","   │     CIEPŁOWNIA ŁAZARZ        │","   ├──────────────────────────────┤","   │ ▤▤▤▤  ▤▤▤▤  ▤▤▤▤  ▤▤▤▤       │","   │                              │","   │  ╭───────────────╮   ╭────╮  │","   │  │   ◴     ◷     │   │ ░░ │  │","   │  │ ──╪─────╪──   │   │ ░░ │  │","   │  ╰───────────────╯   ╰────╯  │","   │                 ╭──────╮     │","   │   ▣▣▣           │      │     │","   └─────────────────┴──────┴─────┘"},d.stone);
      world_art(38,36,{"      ╭──────────────────╮","══════╡ ▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒ ╞══════","      │  ╭──────╮        │","      │  │  ◉   │        │","      │  ╰──────╯        │","      ╰──┬─────────┬─────╯","         ║         ║","         ║         ║","         ║         ║"},d.stone);
      world_art(64,43,{"╔════════════════════╗","║  ▣   ▣   ▣   ▣     ║","╚═══════════╤════════╝","            │","            │"},d.stone);
      world_art(12,47,{"╔══════════╗",tr("║ DYŻURNY  ║","║ OPERATOR ║"),"╚══════════╝"},d.light);
      return;
    }
    if(grid_.room_role==iceberg::RoomRole::Settlement) {
      world_art(3,39,{
        "       ╱──────────────────╲", "  ╔═══╧════════════════════╧═══╗", "  ║  ╭────╮  ╭────╮  ╭────╮    ║",
        "  ║  │ ░░ │  │ ░░ │  │ ░░ │    ║", "  ║  ╰────╯  ╰────╯  ╰────╯    ║", "  ╠════════════════════════════╣",
        "  ║         P O C Z T A        ║", "  ║  ╭────╮  ╔════╗  ╭────╮    ║", "  ║  │ ▤▤ │  ║    ║  │ ▤▤ │    ║",
        "  ║  ╰────╯  ║  ○ ║  ╰────╯    ║", "  ╚══════════╩════╩════════════╝"},d.stone);
      world_art(22,38,{"  ╭────────────╮","  │  11½   ▯   │","  │ ░░░░░░░░░░ │","  ╰──┬──────┬──╯","     │      │"},d.stone);
      world_art(38,33,{"         ╱╲", "   ┌────╱──╲────┐", "   │  ▤▤    ▤▤  │", "   ├────────────┤", "   │  ╭──╮  ▤▤  │", "   │  │  │      │", "   └──┴──┴──────┘"},d.stone);
      world_art(58,38,{"  ╭──────────────╮","  │ ░░░░  ░░░░   │","  │ ░░░░  ░░░░   │","  ├──────────────┤","  │    ○   ▤▤    │","  ╰──────────────╯"},d.stone);
      world_art(31,32,{"┬─────────┬─────────┬─────────┬","│ ▒▒  ▒▒  │ ▒▒▒ ▒▒  │ ▒▒▒ ▒▒ │","  ▒▒  ▒▒    ▒▒▒ ▒▒    ▒▒▒ ▒▒"},0x6d6170);
      if(state_.adventure.quests[5]==3)world_art(69,45,{"♧ ❧ ♧ ❧ ♧","▔▔▔▔▔▔▔▔▔"},0xb8dfb7);
      world_art(68,42,{"   ▄▄▄      ▄▄▄", "  ▐▤▤▤▌    ▐▤▤▤▌", "══╧═══╧════╧═══╧══"},d.stone);
      world_art(12,42,{"╔════════════╗",tr("║ DYŻUR  24h ║","║ ON DUTY 24h║"),"╚════════════╝"},d.light);
    } else {
      draw_room_identity();
      // Structural supports and props follow authored pads rather than a
      // screen-space pattern, giving every route a recognizable silhouette.
      for(size_t i=0;i<grid_.route_points.size();++i) {
        auto [x,y]=grid_.route_points[i];
        if(i%3==0)world_art(x-1,y+2,{"╲     ╱"," ╲   ╱","  ╲ ╱","   ╳","  ╱ ╲"," ╱   ╲"},d.shadow);
        if(i%4==1)world_art(x-2,y-4,{" ╭────╮"," │ ▥▥ │"," │ ▥▥ │"," ╰─┬──╯","   │"},d.stone);
        if(i%5==2)world_art(x-3,y-3,{"╔═══════╗","║ ▣ ▣ ▣ ║","╚═══════╝"},d.stone);
      }
      if(grid_.room_role==iceberg::RoomRole::Finale)
        world_art(55,32,{"      ╭──────────────╮","══════╡ ▓▓▓▓▓▓▓▓▓▓▓▓ ╞══════","      │   ╭────╮     │","      │   │ ◉  │     │","      │   ╰────╯     │","      ╰───┬────┬─────╯","          │    │","          │    │"},d.stone);
    }
  }
#include "iceberg_effects.hpp"
  void draw_iceberg_world() {
    const auto &d=iceberg::Districts[size_t(grid_.district)];
    for(int y=0;y<play_rows_;++y)for(int x=0;x<cols_;++x) {
      auto cell=world_cell(x,y);
      if(!cell.background)cell.background=iceberg_light(state_.camera_x+x*scale_,state_.camera_y+y*aspect_*scale_);
      buffer_[y+4][x]=std::move(cell);
    }
    draw_iceberg_landmarks();
    draw_expedition_infrastructure();
    if(grid_.district==3)draw_archive_drawers();
    if(!grid_.ice_strips.empty())draw_garden_ice();
    if(grid_.district==1)for(const auto &jet:grid_.air_fields) {
      if(grid_.room_role==iceberg::RoomRole::Traverse&&state_.adventure.mode==iceberg::Mode::Endless) {
        draw_expedition_steam_lift(jet);
        continue;
      }
      double x=jet.area.x, bottom=jet.area.bottom(), strength=jet.strength(state_.elapsed);
      world_art(x,bottom,{"╔╧╧╧╧╧╗","╚══╤══╝","╭──┴──╮","│ ◴   │","╞═════╡","│░░░░░│","╰─┬─┬─╯"},d.accent);
      for(int k=0;k<12;++k) {
        double rise=std::fmod(state_.elapsed*.75+k/12.,1.);
        if(rise>strength)continue;
        world_art(x+jet.area.w*(.4+.35*std::sin(k*3.+rise*6)),bottom-rise*jet.area.h,
          {rise>.65?"∿":k%2?"╰∿":"∿╯"},strength>.6?0xc6b9ab:d.stone);
      }
    }
    for(size_t i=0;i<grid_.sweets.size();++i)if(!state_.collected[i]) {
      auto [x,y]=grid_.sweets[i];token_art(x,y,d.accent);
    }
    for(size_t i=0;i<grid_.refills.size();++i) {
      auto [x,y]=grid_.refills[i];refill_art(x,y,state_.refill_cooldown[i]>0);
    }
    for(size_t i=0;i<grid_.platforms.size();++i) {
      const auto &platform=grid_.platforms[i];
      const auto r=platform.at(state_.elapsed);
      if(platform.tram){draw_estate_tram(platform,r);continue;}
      if(quay_ferry(i)){draw_quay_ferry(i,r);continue;}
      if(grid_.template_id==50){draw_leaf_ferry(platform,r);continue;}
      std::string line;
      int n=std::max(2,int(r.w/scale_));
      for(int i=0;i<n;++i)line+=(i==0?"╞":i==n-1?"╡":"═");
      world_text(r.x,r.y,line,d.accent);
      if(grid_.template_id==302) {
        std::string hull="╲";for(int i=0;i<n-2;++i)hull+="▁";hull+="╱";
        world_text(r.x,r.y+.8,hull,d.stone);
      }
    }
    for(auto spring:grid_.springs)world_text(spring.x,spring.y-.5,"╱╲╱╲",0xd8cc87);
    for(size_t i=0;i<grid_.checkpoint_points.size();++i) {
      auto [x,y]=grid_.checkpoint_points[i];
      prop_art(x,y+kPlayerHeight,{"╭─╮",int(i)<=state_.checkpoint?"│▣│":"│□│","╰┬╯"," │ "},int(i)<=state_.checkpoint?d.accent:d.stone);
      if(int(i)==state_.checkpoint&&state_.adventure.lamp_left>0) {
        double radius=(.8-state_.adventure.lamp_left)*5;
        world_text(x-radius,y-1.2,"·",d.light);world_text(x+radius,y-1.2,"·",d.light);
        world_text(x,y-1.2-radius*.5,"⠂",d.light);
      }
    }
    for(const auto &gate:grid_.gates) {
      bool closed=(grid_.switch_flags&gate.flag)!=gate.flag;
      const auto &v=state_.adventure;
      double left=closed?1.:((gate.flag&v.machinery_changed)&&v.machinery_left>0?std::max(0.,1-(v.machinery_duration-v.machinery_left)/.7):0.);
      for(double y=gate.area.y;y<gate.area.y+gate.area.h*left;y+=.6)world_text(gate.area.x,y,"▒▒▒",d.accent);
    }
    for(const auto &pulse:grid_.pulses)draw_pulse_vent(pulse);
    for(size_t i=0;i<grid_.enemies.size();++i)if(!state_.defeated[i]) {
      auto p=grid_.enemies[i].at(state_.elapsed);prop_art(p.x+p.w*.5,p.bottom(),{" ▄▄▄ ","◢▐▣▌◣"," ╱ ╲ "},0xeb6a74);
    }
    for(const auto &node:grid_.interactions) {
      double x=node.area.x,y=node.area.bottom();
      if(node.kind==InteractionKind::Npc||node.kind==InteractionKind::Rescue) {
        if(node.kind==InteractionKind::Rescue&&(grid_.switch_flags&(grid_.template_id==310?8:2)))continue;
        auto [sx,sy]=screen(x,y-.001);++sy;
        resident_sprite(node.id,sx,sy);
      } else if(node.kind==InteractionKind::DockWorker) {
        draw_dock_worker(x,y,std::fmod(state_.adventure.presentation_time+node.id,2.4));
      } else if(node.kind==InteractionKind::Carry) {
        if(!(grid_.switch_flags&1))world_art(x,y-2.4,{"╭─↑─╮","│ ▯ │","╰───╯"},d.accent);
      } else if(node.kind==InteractionKind::Evidence) {
        if(!(grid_.switch_flags&4))prop_art(x+1.5,y,{"╭─────╮","│┌─┬─┐│","│├─┘ ││","│└───┘│","╰─────╯"},d.light);
      } else if(node.kind==InteractionKind::Epilogue) {
        bool done=grid_.switch_flags&(uint64_t(1)<<node.id);
        if(node.id==0)prop_art(x+1.5,y,done?std::vector<std::string>{" · · · ","╔═════╗","║░░░░░║","╚═════╝"}:std::vector<std::string>{" ╭─╮ "," │⠿│ "," ╰─╯ "},d.accent);
        else prop_art(x+1.5,y,{"╭─────╮","│▤▤▤▤▤│","╰─────╯"},done?d.light:d.accent);
      } else if(node.kind==InteractionKind::Deliver) {
        world_art(x,y-3.2,{"╭─────╮","│ ▥▥▥ │","│  ▱  │","╰─────╯"},d.accent);
      } else if(node.kind==InteractionKind::RouteChoice) {
        prop_art(x+node.area.w*.5,y,{"╭─B─╮","│   │","│ ↗ │","│   │","╰───╯"},0x91d4ce);
      } else if(node.kind==InteractionKind::Board) {
        prop_art(x+node.area.w*.5,y,{"╭─────╮","│ ≋≋≋ │","│ ≋≋  │","╰┬───┬╯"," │   │ "},d.accent);
      } else if(node.kind==InteractionKind::Valve)valve_art(x+node.area.w*.5,y,bool(grid_.switch_flags&(uint64_t(1)<<node.id)));
      else if(node.kind==InteractionKind::Note)sign_art(x+node.area.w*.5,y,node.id,d.light);
      else if(node.kind==InteractionKind::Task) {
        bool done=(state_.adventure.task_items[size_t(node.detail/64)]&(uint64_t(1)<<(node.detail%64)))||
          (node.id==0&&iceworld::harbor_v4_room(grid_.template_id)&&(grid_.switch_flags&(uint64_t(1)<<20)));
        if(done&&node.id!=1&&node.id!=2&&node.id!=3&&node.id!=7)continue;
        task_art(node,done);
      } else if(node.kind==InteractionKind::Shortcut||node.kind==InteractionKind::Replay||node.kind==InteractionKind::Return)world_text(x,y-2,"╭↶─╮",d.accent);
      else if(node.kind==InteractionKind::Secret&&node.id==1)world_art(x,y-2,{"╭─╮","│▥│","╰┬╯"},d.stone);
      else if(node.kind==InteractionKind::Secret&&node.id==2)world_art(x,y-3,{"┌──┐","│ 7│","└──┘"},d.stone);
    }
    draw_machinery_reaction();
    // A small readable courier silhouette, with a scarf that follows motion.
    const auto &p=state_.player;
    static const uint32_t coats[]={0xe4ad68,0xdab0d5,0x88d3d9,0xc9bb85,0xabbcf4,0xb8dfb7,0xe99462,0xdce9ef};
    uint32_t coat=coats[std::clamp(state_.adventure.outfit,0,7)];
    auto [px,feet]=screen(p.x,p.y+kPlayerHeight-.001);++feet;
    courier_sprite(px,feet,coat);
    draw_ferry_foreground();
    if(std::abs(p.vx)>8)world_text(p.x+(p.facing>0?-.9:.9),p.y+.6,int(state_.elapsed*16)%2?"╴∽":"╶∼",coat);
    if(grid_.dash_locked()){world_text(p.x+.5,p.y-.8,"╭─╮",d.accent);world_text(p.x+.5,p.y,"╰↑╯",d.accent);}
    if(state_.adventure.pickup_left>0) {
      const auto &v=state_.adventure;double progress=std::clamp(1-v.pickup_left/.65,0.,1.);
      if(v.pickup_kind==8) {
        double y=p.y-1.-progress*1.7;
        token_art(p.x+.5,y-.5,d.accent);
      } else if(v.pickup_kind==1||v.pickup_kind==7) {
        world_text(v.pickup_x,v.pickup_y-1.2,int(progress*8)%2?"╲│╱":" ╷ ",d.accent);
        world_text(v.pickup_x,v.pickup_y-.4,int(progress*8)%2?"─◉─":"╶⊕╴",d.accent);
        world_text(v.pickup_x,v.pickup_y+.4,int(progress*8)%2?"╱│╲":" ╵ ",d.accent);
      } else {
        double travel=std::clamp((progress-.25)/.75,0.,1.);
        double fraction=v.pickup_kind==9?1-travel:travel;
        double x=v.pickup_x+(p.x+.3-v.pickup_x)*fraction;
        double y=v.pickup_y+(p.y+.5-v.pickup_y)*fraction-std::sin(travel*3.14159)*1.4;
        if(progress<.85){world_text(x-.4,y-.8,"╭─╮",d.accent);world_text(x-.4,y,"╰╳╯",d.accent);}
        else {world_text(x-.4,y-.8,"╲ ╱",d.accent);world_text(x-.4,y,"╱ ╲",d.accent);}
      }
    }
    auto goal=grid_.goal;
    double opening=state_.adventure.door_open;
    bool unlocked=(grid_.switch_flags&grid_.goal_requires)==grid_.goal_requires;
    double center=goal.x+goal.w*.5,door_feet=goal.bottom();
    if(!unlocked)prop_art(center,door_feet,{"╔═══════╗","║┌──┬──┐║","║│  │  │║","║│ ▪│▪ │║","║│  │  │║","╚╧══╧══╧╝"},d.stone);
    if(unlocked) {
      const char *face=opening>.66?"║       ║":opening>.2?"║│     │║":"║│ ▪│▪ │║";
      prop_art(center,door_feet,{"╔═══════╗",face,face,face,"║       ║","╚═══════╝"},d.accent);
      if(opening>.66)world_text(center-.5,goal.y+1.6,"▱",d.light);
    }

  }
  void draw_iceberg_inspection() {
    const auto &v=state_.adventure;
    auto layout=iceberg::inspection_layout(v.inspect_title,v.inspect_body,cols_,rows_);
    frame(0,0,cols_,rows_,0x8c7559);
    paint_text(1,2,tr("OGLĘDZINY","INSPECT"),0x9dd4dd,true);
    for(size_t i=0;i<layout.title.size();++i)paint_text(3+int(i),2,layout.title[i],0xf5cd79,true);
    auto art=iceberg::inspection_art(v.inspect_art,v.inspect_detail);
    for(size_t y=0;y<art.size();++y) {
      auto line=glyphs(art[y]);
      for(size_t x=0;x<line.size();++x) {
        uint32_t color=line[x]=="░"?0x716352:line[x]=="█"||line[x]=="▌"||line[x]=="▐"?0xd8ae76:0xcfc2a7;
        if(v.inspect_art==iceberg::InspectionArt::Character)color=iceberg::Npcs[size_t(std::clamp(v.inspect_detail,0,7))].color;
        paint_text(layout.art_y+int(y),layout.art_x+int(x),line[x],color);
      }
    }
    int page=std::clamp(v.inspect_page,0,layout.pages-1);
    for(int i=0;i<layout.text_rows;++i) {
      int at=page*layout.text_rows+i;if(at>=int(layout.body.size()))break;
      paint_text(layout.text_y+i,layout.text_x,layout.body[size_t(at)],0xeee7d6);
    }
    if(layout.pages>1)paint_text(1,cols_-9,std::to_string(page+1)+" / "+std::to_string(layout.pages),0x9dd4dd);
    paint_text(rows_-2,2,page+1<layout.pages?tr("E dalej · B zamknij","E next · B close"):tr("B / E / ENTER — zamknij","B / E / ENTER — close"),0xe4ad68);
  }
  void draw_iceberg_menu() {
    auto &v=state_.adventure;
    const auto &d=iceberg::Districts[size_t(grid_.district)];
    frame(4,2,cols_-4,rows_-7,d.stone);
    center(6,"T E R M I N A L   V E L O C I T Y",Color::Gold);
    center(8,tr("P O D   P O W I E R Z C H N I Ą","B E N E A T H   T H E   S U R F A C E"),Color::Player);
    bool wide=rows_>=30&&cols_>=100;
    int first=wide?13:rows_>=30?16:10;
    if(wide) {
      const char *city[]={
        "                    ▄╱╲▄",
        "               ▄▄▄╱░░░░╲▄▄▄",
        "        ∿∿∿∿∿╱░░░░░░░░░░░░╲∿∿∿∿∿",
        "             ╲░░╭───╮░╭───╮░╱",
        "              ╲▒│ ▣ │─│ ▣ │▒╱",
        "              ▐▒╰─┬─╯ ╰─┬─╯▒▌",
        "              ▐▒══╪═════╪══▒▌",
        "              ▐▒╭─┴─────┴─╮▒▌",
        "               ╲│ ▤ ▤ ▤ ▤ │╱",
        "                ╲─┬───┬───╱",
        "                 ▐│ ◉ │ ▣▌",
        "                  ╲═══╪═╱",
        "                   ╲▒▒│╱",
        "                    ╲▒╱",
        "                     ▽"};
      for(int r=0;r<15;++r) {
        auto line=glyphs(city[r]);
        for(size_t c=0;c<line.size();++c) {
          uint32_t rgb=line[c]=="▣"||line[c]=="▤"||line[c]=="◉"?d.accent:
            line[c]=="░"||line[c]=="▒"?d.stone:line[c]=="∿"?0x477b91:d.light;
          paint_text(10+r,4+int(c),line[c],rgb);
        }
      }
      paint_text(26,11,tr("Miasto czeka na ciepło.","The city waits for warmth."),d.accent);
    } else if(rows_>=30) {
      const char *art[]={"             ▄╱╲▄             ","         ▄▄╱░░░░╲▄▄           ","  ∿∿∿∿∿╱░░░░░░░░░░╲∿∿∿∿∿    ","       ╲▒▤▒▒░╬░▒▒▤▒╱          ","        ╲▒▒▒▒▒▒▒▒▒╱           "};
      for(int i=0;i<5;++i)center(10+i,art[i],Color::Ice);
    }
    std::array<std::string,6> labels{
      v.can_continue?tr("KONTYNUUJ PODRÓŻ","CONTINUE JOURNEY"):tr("ROZPOCZNIJ PODRÓŻ","BEGIN JOURNEY"),
      tr("NOWA HISTORIA","NEW STORY"),v.endless_resume?tr("WZNÓW WYPRAWĘ BEZ KOŃCA","RESUME ENDLESS EXPEDITION"):tr("WYPRAWA BEZ KOŃCA","ENDLESS EXPEDITION"),
      v.trial_resume?tr("WZNÓW PRÓBĘ MISTRZOWSKĄ","RESUME MASTERY TRIAL"):tr("PRÓBA MISTRZOWSKA","MASTERY TRIAL"),tr("USTAWIENIA","SETTINGS"),
      tr("TRASY KLASYCZNE","CLASSIC COURSES")};
    auto menu_line=[&](int row,const std::string &line,Color color=Color::Text){
      if(wide)text(row,cols_/2+1,line,color);else center(row,line,color);
    };
    for(int i=0;i<6;++i)menu_line(first+i,(v.selected==i?"▸ ":"  ")+std::to_string(i+1)+"  "+labels[size_t(i)],v.selected==i?Color::Gold:Color::Text);
    if(v.selected==2||v.selected==3) {
      int r=std::min(first+7,rows_-7);
      menu_line(r,tr("J zmień dzielnicę · ","J change district · ")+iceberg::Districts[size_t(v.run_district)].name.get(v.language),Color::Player);
      const auto &record=v.trials[size_t(v.run_district)];
      if(v.selected==3&&record.complete)menu_line(r+1,tr("Rekordy: ","Records: ")+fixed(record.best_seconds)+"s  /  "+
        std::to_string(record.best_deaths)+tr(" powrotów  /  "," retries  /  ")+std::to_string(record.best_treasures)+" ◆",Color::Gold);
      else if(v.selected==2)menu_line(r+1,tr("Najdalsza wyprawa: ","Farthest expedition: ")+std::to_string(v.endless_best),Color::Gold);
    }
    center(rows_-5,tr("W/S wybór  ENTER graj  R nowa wyprawa  L język  Q wyjście","W/S choose  ENTER play  R new expedition  L language  Q quit"),Color::Text);
    if(v.notice_left>0)center(rows_-4,v.notice,Color::Gold);
    if(v.confirm_new) {
      frame(9,5,cols_-10,8,d.accent);
      center(11,tr("ZACZĄĆ OD NOWA?","START OVER?"),Color::Gold);
      center(13,tr("Zastąpi to zapis obecnej podróży.","This replaces the current journey save."));
      center(15,tr("ENTER potwierdź   P anuluj","ENTER confirm   P cancel"));
    }
    if(v.settings) {
      frame(8,5,cols_-10,12,d.accent);
      center(10,tr("USTAWIENIA","SETTINGS"),Color::Gold);
      center(12,tr("1  Muzyka: ","1  Music: ")+std::to_string(int(v.music_volume*100))+"%");
      center(13,tr("2  Efekty: ","2  Effects: ")+std::to_string(int(v.effects_volume*100))+"%");
      center(14,tr("3  Otoczenie: ","3  Ambience: ")+std::to_string(int(v.ambience_volume*100))+"%");
      center(15,tr("4  Strój: ","4  Outfit: ")+std::to_string(v.outfit+1));
      center(16,tr("5  Pauza po utracie fokusu ","5  Pause on focus loss ")+
        (v.pause_on_focus_loss?tr("wł.","on"):tr("wył.","off")));
      center(17,tr("L  Język: POLSKI","L  Language: ENGLISH"));
      center(18,tr("1/2/3 zmień głośność  ENTER wróć","1/2/3 cycle volume  ENTER return"));
    }
  }
#include "iceberg_route_render.hpp"
  void draw_iceberg_journal() {
    const auto &v=state_.adventure;auto l=v.language;
    const auto &d=iceberg::Districts[size_t(grid_.district)];
    frame(0,0,cols_,rows_,0x637f8e);
    paint_text(1,2,v.map?tr("MAPA PODRÓŻY","JOURNEY MAP"):tr("KSIĘGA ZADAŃ","QUEST BOOK"),d.accent,true);
    if(v.map) {
      if(v.mode!=iceberg::Mode::Campaign) {
        draw_iceberg_route_map(3,rows_-7);
        paint_text(rows_-2,2,tr("J/P wróć","J/P return"),d.light);
        return;
      }
      paint_text(3,2,v.map_route?tr("  Dzielnice  ·  [Trasa]","  Districts  ·  [Route]"):tr("[Dzielnice]  ·   Trasa","[Districts]  ·   Route"),d.light);
      if(v.map_route)draw_iceberg_route_map(5,rows_-9);
      else {
        bool roomy=rows_>=30;int first=5,spacing=roomy?2:1;
        for(int i=5;i>=0;--i) {
          int y=first+(5-i)*spacing;
          std::string marker=i==v.district?"● ":i>v.unlocked?"□ ":"○ ";
          paint_text(y,2,(v.selected==i?"▸ ":"  ")+marker+iceberg::Districts[size_t(i)].name.get(l),v.selected==i?d.accent:i<=v.unlocked?d.light:0x7c8492);
        }
        int y=first+6*spacing+1;
        for(const auto &line:wrap(iceberg::district_travel_status(v,std::clamp(v.selected,0,5)),cols_-4))if(y<rows_-4)paint_text(y++,2,line,0xeee7d6);
      }
      paint_text(rows_-3,2,v.map_route?tr("↑↓ odcinek · B dzielnice","↑↓ leg · B districts"):tr("↑↓ wybór · ENTER podróż","↑↓ select · ENTER travel"),d.light);
      paint_text(rows_-2,2,v.map_route?tr("J/P wróć","J/P return"):tr("B trasa · J/P wróć","B route · J/P return"),d.light);
      return;
    }
    std::array<std::string,3> tabs{tr("Aktywne","Active"),tr("Ukończone","Completed"),tr("Notatki","Notes")};
    std::string tabline;for(int i=0;i<3;++i){if(i)tabline+=" · ";tabline+=i==v.journal_tab?"["+tabs[size_t(i)]+"]":tabs[size_t(i)];}
    paint_text(3,2,tabline,d.light);
    auto entries=iceberg::journal_entries(v);
    if(entries.empty()) {
      int y=7;for(const auto &line:wrap(v.journal_tab==1?tr("Ukończone zadania i ich skutki pojawią się tutaj.","Completed tasks and their outcomes will appear here."):tr("Znalezione dokumenty możesz tutaj przeczytać ponownie.","Documents you find can be read again here."),cols_-4))paint_text(y++,2,line,0xeee7d6);
    } else {
      int choice=std::clamp(v.selected,0,int(entries.size())-1),selected=entries[size_t(choice)];
      auto title=[&](int entry){return entry<0?tr("Główna podróż","Main journey"):entry==200?tr("Oryginalny plan sieci","Original network plan"):entry==100?tr("Dziecięcy plan mieszkania","A child's apartment plan"):entry==101?tr("Nagranie z klatki jedenastej","Stairwell eleven recording"):entry==102?tr("Protokół zamknięcia","Closure log"):iceberg::Quests[size_t(entry)].title.get(l);};
      bool wide=cols_>=100&&rows_>=26;int detail_x=wide?cols_/2:2,detail_width=cols_-detail_x-3,y=5;
      if(wide) {
        int visible=rows_-10,start=std::clamp(choice-visible/2,0,std::max(0,int(entries.size())-visible));
        for(int i=start;i<std::min(start+visible,int(entries.size()));++i) {
          auto label=(i==choice?"▸ ":"  ")+title(entries[size_t(i)]);
          auto chars=glyphs(label);if(int(chars.size())>detail_x-4){label.clear();for(int j=0;j<detail_x-5;++j)label+=chars[size_t(j)];label+="…";}
          paint_text(5+i-start,2,label,i==choice?d.accent:0xc8bbb8);
        }
      } else {paint_text(y++,2,std::to_string(choice+1)+" / "+std::to_string(entries.size()),0x9aa9b6);}
      auto paragraph=[&](const std::string &value,uint32_t color){for(const auto &line:wrap(value,detail_width))if(y<rows_-4)paint_text(y++,detail_x,line,color);};
      paragraph(title(selected),d.accent);++y;
      if(selected<0) {
        paragraph(v.quest_focus<0?tr("◆ BIEŻĄCY CEL","◆ CURRENT FOCUS"):tr("PODRÓŻ","JOURNEY"),d.light);
        paragraph(iceberg::journey_next_step(v),0xeee7d6);
      } else if(selected>=100) {
        paragraph(tr("E / ENTER — otwórz zapis","E / ENTER — open record"),0xeee7d6);
        paragraph(selected==200?tr("Plan łączy archiwum z ogrodami. Obejście biegnie pod szklarniami.","The plan connects the archive to the gardens. The bypass runs beneath the greenhouses."):tr("Zapis zachowany po odkryciu na trasie.","A record kept after finding it on the route."),0xc8bbb8);
      } else {
        const auto &quest=iceberg::Quests[size_t(selected)];
        paragraph(std::string(iceberg::Npcs[size_t(quest.owner)].name)+" · "+(v.quests[size_t(selected)]==3?tr("UKOŃCZONE","COMPLETE"):v.quests[size_t(selected)]==2?tr("WRÓĆ DO ZLECENIODAWCY","RETURN TO TASK GIVER"):std::to_string(v.quest_counts[size_t(selected)])+" / "+std::to_string(quest.required)),d.light);
        if(v.quest_focus==selected&&v.quests[size_t(selected)]<3)paragraph(tr("◆ BIEŻĄCY CEL","◆ CURRENT FOCUS"),d.accent);
        paragraph(iceberg::quest_next_step(v,selected),0xeee7d6);
        if(v.quests[size_t(selected)]==3){++y;paragraph(tr("E — listy, podziękowania i nagroda","E — letters, thanks and reward"),d.light);}
      }
    }
    paint_text(rows_-3,2,tr("↑↓ wybór · E cel/czytaj","↑↓ select · E focus/read"),d.light);
    paint_text(rows_-2,2,tr("B zakładka · TAB/P wróć","B tab · TAB/P return"),d.light);
  }
  std::string quest_focus_hint() const {
    const auto &v=state_.adventure;int q=v.quest_focus;
    if(q<0||q>=8||v.quests[size_t(q)]==0||v.quests[size_t(q)]==3)return "";
    return "◆ "+iceberg::Quests[size_t(q)].title.get(v.language)+"  "+std::to_string(v.quest_counts[size_t(q)])+"/"+std::to_string(iceberg::Quests[size_t(q)].required);
  }
  std::string dialogue_fragment(const std::string &line,size_t &remaining) const {
    std::string shown;
    for(const auto &ch:glyphs(line)) {
      if(ch==" "){shown+=' ';continue;}
      if(remaining){shown+=ch;--remaining;}else shown+=' ';
    }
    return shown;
  }
  void draw_iceberg_dialogue() {
    const auto &dialogue=state_.adventure.dialogue;if(!dialogue.active())return;
    const auto &line=dialogue.lines[dialogue.page];
    const auto &npc=iceberg::Npcs[size_t(std::clamp(line.speaker,0,7))];
    int courier=screen(state_.player.x,state_.player.y).second;
    auto layout=iceberg::dialogue_layout(line.text.get(state_.adventure.language),cols_,rows_,courier,dialogue.segment);
    frame(layout.top,layout.left,layout.width,layout.height,npc.color);
    std::string name=line.speaker<0?tr("DOKUMENT","DOCUMENT"):npc.name;
    if(layout.pages>1)name+="  "+std::to_string(dialogue.segment+1)+"/"+std::to_string(layout.pages);
    paint_text(layout.top+1,3,name,npc.color,true);
    if(layout.portrait)for(int i=0;i<8;++i) {
      static const char *paper[]={"   ╭────────╮   ","   │ ≋≋≋≋≋  │   ","   │ ≋≋≋≋≋≋ │   ","   │ ≋≋≋≋   │   ","   │        │   ","   │ ≋≋≋≋≋  │   ","   │    ▣   │   ","   ╰────────╯   "};
      std::string portrait=line.speaker<0?paper[i]:npc.portrait[size_t(i)];
      if(grid_.room_role==iceberg::RoomRole::Secret&&line.speaker==7&&dialogue.page>=2&&i==2)portrait="   │ │    │ │   ";
      paint_text(layout.top+2+i,3,portrait,npc.color);
    }
    size_t remaining=dialogue.visible_characters;int first=std::clamp(dialogue.segment,0,layout.pages-1)*layout.text_rows;
    for(int i=first;i<std::min(first+layout.text_rows,int(layout.body.size()));++i)
      paint_text(layout.text_y+i-first,layout.text_x,dialogue_fragment(layout.body[size_t(i)],remaining),0xeee7d6);
    paint_text(layout.top+layout.height-2,layout.text_x,state_.paused&&!state_.journal?tr("E/ENTER wznów","E/ENTER resume"):dialogue.revealing?tr("E/ENTER: tekst od razu","E/ENTER: instant text"):tr("E/ENTER: dalej","E/ENTER: continue"),npc.color);
  }
#include "iceberg_compact_render.hpp"
  void draw_iceberg() {
    draw_iceberg_world();
    draw_room_shutter();
    if(cols_<76||rows_<24){draw_iceberg_compact();return;}
    const auto &v=state_.adventure;
    const auto &d=iceberg::Districts[size_t(grid_.district)];
    fill(0,0,cols_,4,Color::Default);
    paint_text(0,2,d.name.get(v.language),d.accent,true);
    paint_text(1,2,v.room_name,d.light);
    if(cols_>90)paint_text(0,cols_-27,tr("TRASA ","SEED ")+std::to_string(v.seed),d.stone);
    std::string mode=v.revisit?tr("POWRÓT NA TRASĘ","REVISIT"):v.mode==iceberg::Mode::Campaign?tr("PODRÓŻ","JOURNEY"):v.mode==iceberg::Mode::Endless?tr("BEZ KOŃCA","ENDLESS"):tr("PRÓBA","TRIAL");
    paint_text(2,2,mode+(grid_.sweets.empty()?"":tr("   Żetony ","   Tokens ")+std::to_string(state_.sweets_count())+"/"+std::to_string(grid_.sweets.size()))+
      tr("   Powroty ","   Retries ")+std::to_string(state_.deaths)+
      (v.mode==iceberg::Mode::Endless?tr("   Głębokość ","   Depth ")+std::to_string(v.depth):
       grid_.room_role==iceberg::RoomRole::Traverse?"   "+std::to_string(v.room+1)+"/"+std::to_string(iceberg::RoomsPerLeg):"")+
      (state_.multiplayer?"   ◌ "+std::to_string(state_.ghosts.size()):""),0xb3c3c8);
    for(int x=0;x<cols_;++x)paint_text(3,x,"─",d.stone);
    fill(rows_-3,0,cols_,3,Color::Default);
    text(rows_-3,2,state_.ssh_basic?tr("A/D bieg S stop SPACJA skok X zryw I/O ↑/↓ E rozmowa","A/D run S stop SPACE jump X dash I/O ↑/↓ E interact"):
      tr("A/D ruch  SPACJA skok  X zryw  E użyj  B obejrzyj","A/D move  SPACE jump  X dash  E use  B inspect"),Color::Player);
    text(rows_-2,2,tr("R powrót  TAB zadania  J mapa  P pauza  M menu  U dźwięk","R retry  TAB quests  J map  P pause  M menu  U sound"));
    if(v.save_error)text(rows_-1,2,save_failure_hint(),Color::Hazard);
    else if(v.notice_left>0)text(rows_-1,2,v.notice,Color::Gold);
    else text(rows_-1,2,v.prompt.empty()?quest_focus_hint():v.prompt,Color::Gold);
    if(state_.debug)text(5,2,"vx "+fixed(state_.player.vx)+" vy "+fixed(state_.player.vy)+" 120 Hz",Color::Gold);
    if(state_.menu){draw_iceberg_menu();return;}
    if(state_.journal||v.map)draw_iceberg_journal();
    else if(state_.paused) {
      frame(8,5,cols_-10,9,d.stone);
      center(10,tr("CHWILA ODDECHU","A MOMENT TO BREATHE"),Color::Gold);
      center(12,tr("ENTER / P wznów   M menu","ENTER / P resume   M menu"));
      center(14,tr("Postęp zapisuje się przy lampach i wyjściu.","Progress saves at lamps and when you leave."));
    }
    if(state_.won&&!state_.paused&&!state_.journal&&!v.map&&!v.inspecting&&!v.dialogue.active()) {
      fill(rows_-3,0,cols_,2,Color::Default);
      paint_text(rows_-3,2,tr("E / ENTER — przejdź przez drzwi","E / ENTER — go through the door"),d.accent,true);
      paint_text(rows_-2,2,tr("R — wróć do lampy","R — return to the lamp"),d.light);
    }
    draw_iceberg_dialogue();
    if(v.inspecting)draw_iceberg_inspection();
  }
