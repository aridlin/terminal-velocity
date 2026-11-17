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
      if(b%7==0)return c("─",shade);
      if(b%7>=3&&a%5<3)return c(a%2?"▥":"▤",shade);
      break;
    case 4:
      if(a%21==0)return c("│",shade);
      if((a+b*3)%23==0)return c("❧",d.stone);
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
  void world_art(double x,double y,const std::vector<std::string> &lines,uint32_t rgb) {
    // Decoration is behind collision and characters, even when the camera
    // exposes only a few cells of a large architectural landmark.
    auto [origin_x,origin_y]=screen(x,y);
    for(size_t r=0;r<lines.size();++r) {
      auto glyph=glyphs(lines[r]);
      for(size_t c=0;c<glyph.size();++c) {
        if(glyph[c]==" ")continue;
        double wx=x+c*scale_,wy=y+r*aspect_*scale_;
        if(grid_.solid_at(int(std::floor(wx)),int(std::floor(wy)))||grid_.hazard_at(int(std::floor(wx)),int(std::floor(wy)))||
           state_.player.rect().intersects({wx,wy,scale_,aspect_*scale_}))continue;
        int sx=origin_x+int(c),sy=origin_y+int(r);
        if(sy>=4&&sy<rows_-3)paint_text(sy,sx,glyph[c],rgb);
      }
    }
  }
#include "iceberg_scenery.hpp"
  void draw_iceberg_landmarks() {
    const auto &d=iceberg::Districts[size_t(grid_.district)];
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
      if(!(grid_.switch_flags&2))world_art(137,40,{"    ╭────────────────────────╮","    │ ▯▯  ▯▯  ▯▯  ▯▯  ▯▯    │","    ├────────────────────────┤","    │   T R A M W A J   02   │","    ╰───◉───────────────◉────╯","══════════════════════════════════"},d.accent);
      else world_art(137,42,{"══════════════════════════════════","  ·   ·   ·   ·   ·   ·   ·   ·"},d.stone);
      world_art(94,29,{"     ╭───╮","═════╡ ◉ ╞═════","     ╰─┬─╯","       │","       │"},d.stone);
    } else if(grid_.template_id==301) {
      world_art(4,44,{"╭──────────────────╮",tr("│ ↑ NIE PRZEWRACAĆ ↑│","│ ↑ KEEP UPRIGHT ↑  │"),"╰──────────────────╯"},d.accent);
      world_art(192,21,{"     ╔════════════╗","═════╣ ░░░░░░░░░░ ╠═════","     ║    P 01    ║","     ╚════╤══╤════╝","          │  │"},d.stone);
    } else if(grid_.template_id==302) {
      for(auto [x,y]:{std::pair{56.,40.},std::pair{114.,34.}})
        world_art(x,y,{"   ╭──────────────╮","═══╡  ╭────────╮  ╞═══","   │  │ ╲ │ ╱  │  │","   │  │ ─ ◉ ─  │  │","   │  │ ╱ │ ╲  │  │","   │  ╰────────╯  │","   ╰──────┬───────╯","          ║"},d.stone);
    }
    if(grid_.room_role==iceberg::RoomRole::Settlement&&grid_.district==1) {
      world_art(3,42,{"   ┌──────────────────────────────┐","   │     CIEPŁOWNIA ŁAZARZ         │","   ├──────────────────────────────┤","   │ ▤▤▤▤  ▤▤▤▤  ▤▤▤▤  ▤▤▤▤      │","   │                              │","   │  ╭───────────────╮   ╭────╮  │","   │  │   ◴     ◷     │   │ ░░ │  │","   │  │ ──╪─────╪──   │   │ ░░ │  │","   │  ╰───────────────╯   ╰────╯  │","   │                 ╭──────╮     │","   │   ▣▣▣           │      │     │","   └─────────────────┴──────┴─────┘"},d.stone);
      world_art(38,36,{"      ╭──────────────────╮","══════╡ ▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒ ╞══════","      │  ╭──────╮        │","      │  │  ◉   │        │","      │  ╰──────╯        │","      ╰──┬─────────┬─────╯","         ║         ║","         ║         ║","         ║         ║"},d.stone);
      world_art(64,43,{"╔════════════════════╗","║  ▣   ▣   ▣   ▣     ║","╚═══════════╤════════╝","            │","            │"},d.stone);
      world_art(12,47,{"╔══════════╗",tr("║ DYŻURNY  ║","║ OPERATOR ║"),"╚══════════╝"},d.light);
      return;
    }
    if(grid_.room_role==iceberg::RoomRole::Settlement) {
      world_art(3,39,{
        "       ╱──────────────────╲", "  ╔═══╧════════════════════╧═══╗", "  ║  ╭────╮  ╭────╮  ╭────╮   ║",
        "  ║  │ ░░ │  │ ░░ │  │ ░░ │   ║", "  ║  ╰────╯  ╰────╯  ╰────╯   ║", "  ╠═══════════════════════════╣",
        "  ║         P O C Z T A        ║", "  ║  ╭────╮  ╔════╗  ╭────╮   ║", "  ║  │ ▤▤ │  ║    ║  │ ▤▤ │   ║",
        "  ║  ╰────╯  ║  ○ ║  ╰────╯   ║", "  ╚══════════╩════╩═══════════╝"},d.stone);
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
  void draw_iceberg_world() {
    const auto &d=iceberg::Districts[size_t(grid_.district)];
    for(int y=0;y<play_rows_;++y)for(int x=0;x<cols_;++x) {
      auto cell=world_cell(x,y);
      cell.background=iceberg_light(state_.camera_x+x*scale_,state_.camera_y+y*aspect_*scale_);
      buffer_[y+4][x]=std::move(cell);
    }
    draw_iceberg_landmarks();
    for(size_t i=0;i<grid_.sweets.size();++i)if(!state_.collected[i]) {
      auto [x,y]=grid_.sweets[i];world_text(x,y,"◆",d.accent);
    }
    for(size_t i=0;i<grid_.refills.size();++i) {
      auto [x,y]=grid_.refills[i];world_text(x,y,state_.refill_cooldown[i]>0?"◇":"✦",0x8ee0dd);
    }
    for(auto platform:grid_.platforms) {
      auto r=platform.at(state_.elapsed);std::string line;
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
      world_text(x,y-1,"◈",int(i)<=state_.checkpoint?d.accent:d.stone);
      world_text(x,y-.4,"╽",d.stone);
    }
    for(const auto &gate:grid_.gates)if((grid_.switch_flags&gate.flag)!=gate.flag)
      for(double y=gate.area.y;y<gate.area.bottom();y+=.6)world_text(gate.area.x,y,"▒▒▒",d.accent);
    for(const auto &pulse:grid_.pulses) {
      bool hot=pulse.active(state_.elapsed),warn=pulse.warning(state_.elapsed);
      world_text(pulse.area.x-.3,pulse.area.bottom()-.2,"╧╧╧",warn?0xf1c66f:0x886b5b);
      if(hot)for(double y=pulse.area.y;y<pulse.area.bottom();y+=.6)
        world_text(pulse.area.x,y,int(state_.elapsed*18+y)%2?"♨┊":"┊♨",0xeb6a74);
      else if(warn)world_text(pulse.area.x,pulse.area.bottom()-.9,"···",0xf1c66f);
    }
    for(size_t i=0;i<grid_.enemies.size();++i)if(!state_.defeated[i]) {
      auto p=grid_.enemies[i].at(state_.elapsed);world_text(p.x,p.y,"◢▣◣",0xeb6a74);
    }
    for(const auto &node:grid_.interactions) {
      double x=node.area.x,y=node.area.bottom();
      if(node.kind==InteractionKind::Npc||node.kind==InteractionKind::Rescue) {
        if(node.kind==InteractionKind::Rescue&&(grid_.switch_flags&2))continue;
        auto rgb=iceberg::Npcs[size_t(node.id)].color;
        static const char *heads[]={"▄█▄","▟█▙","▔█▔","▟▓▙","▄▣▄","♣█▄","▄▓▙","┌▣┐"};
        static const char *poses[]={"▐█▱","╶█┐","▐█╴","▐▓▤","▐█▥","♧█▌","╴▓▌","│▤│"};
        auto [sx,sy]=screen(x,y-.001);++sy;
        if(sy-3>=4&&sy<rows_-2) {
          paint_text(sy-3,sx,heads[node.id],rgb);
          paint_text(sy-2,sx,int(state_.elapsed*.7+node.id)%4==0?poses[node.id]:node.id==7?"│▤│":"▐█▌",rgb);
          paint_text(sy-1,sx,node.id==7?"└┬┘":int(state_.elapsed*.7+node.id)%4==0?"╱ │":"│ ╲",rgb);
        }
      } else if(node.kind==InteractionKind::Carry) {
        if(!(grid_.switch_flags&1))world_art(x,y-2.4,{"╭─↑─╮","│ ▯ │","╰───╯"},d.accent);
      } else if(node.kind==InteractionKind::Deliver) {
        world_art(x,y-3.2,{"╭─────╮","│ ▥▥▥ │","│  ▱  │","╰─────╯"},d.accent);
      } else if(node.kind==InteractionKind::Board) {
        world_text(x,y-2.4,"╭────╮",d.accent);world_text(x,y-1.8,"│ ≋≋ │",d.accent);
        world_text(x,y-1.2,"╰┬──┬╯",d.accent);
      } else if(node.kind==InteractionKind::Valve)world_text(x,y-1.4,(grid_.switch_flags&(uint64_t(1)<<node.id))?"─◉─":"─⊗─",d.accent);
      else if(node.kind==InteractionKind::Note)world_text(x,y-1.5,"▤",d.light);
      else if(node.kind==InteractionKind::Task) {
        static const char *icons[]={"▣","─⊗─","┌◉┐","▤","▥","♧","▣","▤▤"};
        bool done=state_.adventure.task_items[size_t(node.detail/64)]&(uint64_t(1)<<(node.detail%64));
        world_text(x,y-1.3,node.id==1&&done?"─◉─":icons[node.id],done?d.shadow:d.accent);
      } else if(node.kind==InteractionKind::Shortcut||node.kind==InteractionKind::Replay||node.kind==InteractionKind::Return)world_text(x,y-2,"╭↶─╮",d.accent);
      else if(node.kind==InteractionKind::Secret&&node.id==1)world_art(x,y-2,{"╭─╮","│▥│","╰┬╯"},d.stone);
      else if(node.kind==InteractionKind::Secret&&node.id==2)world_art(x,y-3,{"┌──┐","│ 7│","└──┘"},d.stone);
    }
    // A small readable courier silhouette, with a scarf that follows motion.
    const auto &p=state_.player;
    static const uint32_t coats[]={0xe4ad68,0xdab0d5,0x88d3d9,0xc9bb85,0xabbcf4,0xb8dfb7,0xe99462,0xdce9ef};
    uint32_t coat=coats[std::clamp(state_.adventure.outfit,0,7)];
    auto [px,feet]=screen(p.x,p.y+kPlayerHeight-.001);++feet;
    if(feet-3>=4&&feet<rows_-2) {
      paint_text(feet-3,px,"▟▀▙",coat);
      paint_text(feet-2,px,p.facing>0?"▐█▖":"▗█▌",p.dash_available&&!grid_.dash_locked()?0xf5cd79:0x83c5ce);
      if(state_.adventure.cosmetics&0x3f0000u)paint_text(feet-2,px+1,"✦",0xf3ce82);
      paint_text(feet-1,px,!p.grounded?"╱╲":std::abs(p.vx)>2&&int(state_.elapsed*12)%2?"▗ ▖":"▐ ▌",coat);
    }
    if(std::abs(p.vx)>8)world_text(p.x+(p.facing>0?-.9:.9),p.y+.6,int(state_.elapsed*16)%2?"╴∽":"╶∼",coat);
    if(grid_.dash_locked())world_text(p.x+.8,p.y-.7,"▱",d.accent);
    auto goal=grid_.goal;
    world_text(goal.x,goal.y,"╭────╮",d.accent);
    for(double y=goal.y+.6;y<goal.bottom();y+=.6)world_text(goal.x,y,"│    │",d.accent);
    world_text(goal.x+.6,goal.y+1.8,(grid_.switch_flags&grid_.goal_requires)==grid_.goal_requires?"↑":"⊗",d.light);
    for(const auto &m:state_.motes)world_text(m.x,m.y,"·",m.gold?d.accent:d.light);
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
      v.trial_resume?tr("WZNÓW PRÓBĘ MISTRZOWSKĄ","RESUME MASTERY TRIAL"):tr("PRÓBA MISTRZOWSKA","MASTERY TRIAL"),tr("DŹWIĘK I JĘZYK","SOUND AND LANGUAGE"),
      tr("TRASY KLASYCZNE","CLASSIC COURSES")};
    auto menu_line=[&](int row,const std::string &line,Color color=Color::Text){
      if(wide)text(row,cols_/2+1,line,color);else center(row,line,color);
    };
    for(int i=0;i<6;++i)menu_line(first+i,(v.selected==i?"▸ ":"  ")+std::to_string(i+1)+"  "+labels[size_t(i)],v.selected==i?Color::Gold:Color::Text);
    if(v.selected==2||v.selected==3) {
      int r=std::min(first+7,rows_-7);
      menu_line(r,tr("J dzielnica: ","J district: ")+iceberg::Districts[size_t(v.run_district)].name.get(v.language),Color::Player);
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
      center(10,tr("DŹWIĘK I JĘZYK","SOUND AND LANGUAGE"),Color::Gold);
      center(12,tr("1  Muzyka: ","1  Music: ")+std::to_string(int(v.music_volume*100))+"%");
      center(13,tr("2  Efekty: ","2  Effects: ")+std::to_string(int(v.effects_volume*100))+"%");
      center(14,tr("3  Otoczenie: ","3  Ambience: ")+std::to_string(int(v.ambience_volume*100))+"%");
      center(15,tr("4  Strój: ","4  Outfit: ")+std::to_string(v.outfit+1));
      center(16,tr("L  Język: POLSKI","L  Language: ENGLISH"));
      center(18,tr("1/2/3 zmień głośność  ENTER wróć","1/2/3 cycle volume  ENTER return"));
    }
  }
  void draw_iceberg_journal() {
    const auto &v=state_.adventure;
    auto l=v.language;
    frame(4,2,cols_-4,rows_-7,0x637f8e);
    center(6,v.map?tr("DZIELNICE GÓRY LODOWEJ","DISTRICTS OF THE ICEBERG"):tr("DZIENNIK KURIERA","COURIER'S JOURNAL"),Color::Gold);
    if(v.map) {
      bool wide=cols_>=100&&rows_>=32;
      static const std::array<std::array<const char *,3>,6> strata{{
        {{"             ╱╲", "            ╱◈ ╲", "      ∿∿∿∿╱────╲∿∿∿∿"}},
        {{"          ╱ ♧ ♧  ╲", "         ╱────────╲", "        ╱  ♧   ♧   ╲"}},
        {{"       ╱ ▤▤ ▤▤ ▤▤ ▤▤ ╲", "       │─────────────│", "      ╱  ▤▤ ▤▤ ▤▤ ▤▤  ╲"}},
        {{"     ╱ ┌┐ ┌┐ ┌┐ ┌┐ ┌┐ ╲", "     │ └┘ └┘ └┘ └┘ └┘ │", "    ╱═══════════════════╲"}},
        {{"   ╱   ▥▥    ◉    ▥▥     ╲", "   │═════════════════════│", "   │   ║             ║   │"}},
        {{"    ╲  ║ ∿∿  ◈ ∿∿    ║  ╱", "     ╲─────────────────╱", "       ╲_____________╱"}}
      }};
      for(int i=5;i>=0;--i) {
        int y=8+(5-i)*(wide?3:2);
        if(y>=rows_-5)break;
        std::string label=(i==v.selected?"▸ ":"  ")+iceberg::Districts[size_t(i)].name.get(l);
        if(wide) {
          const auto &district=iceberg::Districts[size_t(i)];
          uint32_t shade=i<=v.unlocked?district.accent:0x34434d;
          for(int line=0;line<3;++line)paint_text(y+line,7,strata[size_t(5-i)][size_t(line)],shade);
          text(y,cols_/2,label,i==v.selected?Color::Gold:i<=v.unlocked?Color::Player:Color::Ice);
          paint_text(y+1,cols_/2+2,i==v.district?tr("tu jesteś","you are here"):
            i<=v.unlocked?tr("połączenie czynne","connection open"):tr("brak połączenia","not connected"),i<=v.unlocked?0xc8bbb8:0x63788b);
        } else center(y,label,i==v.selected?Color::Gold:i<=v.unlocked?Color::Player:Color::Ice);
        const auto &record=v.records[size_t(i)];
        if(record.complete) {
          std::string best=tr("Najlepiej: ","Best: ")+fixed(record.best_seconds)+"s / "+
            std::to_string(record.best_deaths)+tr(" powr. / "," retries / ")+std::to_string(record.best_treasures)+" ◆";
          if(wide)paint_text(y+2,cols_/2+2,best,iceberg::Districts[size_t(i)].accent);
          else center(y+1,best,Color::Text);
        }
      }
    } else {
      int q=std::clamp(v.selected,0,7);
      auto objective=iceberg::Districts[size_t(v.district)].objective.get(l);
      if(v.revisit)objective=tr("Powrót na dawną trasę. Brama przy wejściu wraca do osady.","Revisiting an earlier route. The entrance door returns to the settlement.");
      else if(v.visit_district>=0)objective=tr("Wizyta w osadzie. Tablica wypraw wraca na twoją trasę.","Visiting a settlement. Its expedition board returns to your route.");
      if(v.mode!=iceberg::Mode::Campaign)objective=v.mode==iceberg::Mode::Endless?tr("Wyprawa: wybieraj nowe trasy i pobij swoją głębokość.","Expedition: explore new routes and beat your depth record."):tr("Próba: osiem trudnych odcinków. Liczą się czas, powroty i zbiory.","Trial: eight expert rooms. Time, retries and collection count.");
      auto goal_lines=wrap(objective,cols_-12);
      for(size_t i=0;i<goal_lines.size()&&i<2;++i)text(8+int(i),6,goal_lines[i],Color::Player);
      text(10,6,iceberg::route_title(v.district,v.stage).get(l),Color::Gold);
      int visible=std::clamp(rows_-22,3,8),start=std::clamp(q-visible/2,0,8-visible);
      for(int i=start;i<start+visible;++i) {
        text(11+i-start,5,(q==i?"▸ ":"  ")+(v.quests[size_t(i)]?iceberg::Quests[size_t(i)].title.get(l):tr("Niepoznana historia","An untold story")),q==i?Color::Gold:Color::Text);
      }
      if(v.quests[size_t(q)]) {
        auto lines=wrap((v.quests[size_t(q)]==3?iceberg::Quests[size_t(q)].reward:iceberg::Quests[size_t(q)].objective).get(l),cols_-12);
        int y=rows_-8;
        for(const auto &line:lines)if(y<rows_-5)text(y++,6,line);
        text(rows_-5,6,v.quests[size_t(q)]==3?tr("UKOŃCZONE","COMPLETE"):
          std::to_string(v.quest_counts[size_t(q)])+" / "+std::to_string(iceberg::Quests[size_t(q)].required),Color::Gold);
      }
    }
    center(rows_-4,v.map?(v.mode==iceberg::Mode::Campaign?
      tr("W/S wybierz   ENTER podróż   J powrót","W/S select   ENTER travel   J return"):
      tr("W/S wybierz   ENTER / J wróć","W/S select   ENTER / J return")):
      tr("W/S wybierz   E czytaj nagrodę   TAB wróć","W/S select   E read reward   TAB return"));
  }
  void draw_iceberg_dialogue() {
    const auto &dialogue=state_.adventure.dialogue;
    if(!dialogue.active())return;
    const auto &line=dialogue.lines[dialogue.page];
    const auto &npc=iceberg::Npcs[size_t(std::clamp(line.speaker,0,7))];
    int top=rows_-15;
    frame(top,2,cols_-4,13,npc.color);
    paint_text(top+1,5,line.speaker<0?tr("DOKUMENT","DOCUMENT"):npc.name,npc.color,true);
    paint_text(top+2,5,line.speaker<0?tr("Odnaleziony zapis","Recovered record"):npc.title.get(state_.adventure.language),0xa9b8c2);
    for(int i=0;i<8;++i) {
      static const char *paper[]={"   ╭────────╮   ","   │ ≋≋≋≋≋  │   ","   │ ≋≋≋≋≋≋ │   ","   │ ≋≋≋≋   │   ","   │        │   ","   │ ≋≋≋≋≋  │   ","   │    ▣   │   ","   ╰────────╯   "};
      std::string portrait=line.speaker<0?paper[i]:npc.portrait[size_t(i)];
      if(grid_.room_role==iceberg::RoomRole::Secret && line.speaker==7 && dialogue.page>=2 && i==2)portrait="   │ │    │ │   ";
      paint_text(top+3+i,4,portrait,npc.color);
    }
    auto lines=wrap(line.text.get(state_.adventure.language),cols_-28);
    int y=top+4;
    for(auto &s:lines)if(y<top+11)paint_text(y++,24,s,0xeee7d6);
    paint_text(top+11,24,tr("E / ENTER  dalej","E / ENTER  continue"),npc.color);
  }
  void draw_iceberg() {
    draw_iceberg_world();
    const auto &v=state_.adventure;
    const auto &d=iceberg::Districts[size_t(grid_.district)];
    fill(0,0,cols_,4,Color::Default);
    paint_text(0,2,"◈  "+d.name.get(v.language),d.accent,true);
    paint_text(1,2,v.room_name,d.light);
    if(cols_>90)paint_text(0,cols_-27,tr("TRASA ","SEED ")+std::to_string(v.seed),d.stone);
    std::string mode=v.revisit?tr("POWRÓT NA TRASĘ","REVISIT"):v.mode==iceberg::Mode::Campaign?tr("PODRÓŻ","JOURNEY"):v.mode==iceberg::Mode::Endless?tr("BEZ KOŃCA","ENDLESS"):tr("PRÓBA","TRIAL");
    paint_text(2,2,mode+(grid_.sweets.empty()?"":"   ◆ "+std::to_string(state_.sweets_count())+"/"+std::to_string(grid_.sweets.size()))+
      tr("   Powroty ","   Retries ")+std::to_string(state_.deaths)+
      (v.mode==iceberg::Mode::Endless?tr("   Głębokość ","   Depth ")+std::to_string(v.depth):
       grid_.room_role==iceberg::RoomRole::Traverse?"   "+std::to_string(v.room+1)+"/"+std::to_string(iceberg::RoomsPerLeg):"")+
      (state_.multiplayer?"   ◌ "+std::to_string(state_.ghosts.size()):""),0xb3c3c8);
    for(int x=0;x<cols_;++x)paint_text(3,x,"─",d.stone);
    fill(rows_-3,0,cols_,3,Color::Default);
    text(rows_-3,2,state_.ssh_basic?tr("A/D bieg S stop SPACJA skok X zryw I/O ↑/↓ E rozmowa","A/D run S stop SPACE jump X dash I/O ↑/↓ E interact"):
      tr("A/D ruch  SPACJA skok  SHIFT/X zryw  E rozmowa","A/D move  SPACE jump  SHIFT/X dash  E interact"),Color::Player);
    text(rows_-2,2,tr("R powrót  TAB dziennik  J mapa  P pauza  M menu  U dźwięk","R retry  TAB journal  J map  P pause  M menu  U sound"));
    if(v.save_error)text(rows_-1,2,tr("Błąd zapisu — postęp pozostaje w tej sesji.","Save error — progress remains in this session."),Color::Hazard);
    else if(v.notice_left>0)text(rows_-1,2,v.notice,Color::Gold);
    else text(rows_-1,2,v.prompt,Color::Gold);
    if(state_.debug)text(5,2,"vx "+fixed(state_.player.vx)+" vy "+fixed(state_.player.vy)+" 120 Hz",Color::Gold);
    if(state_.menu){draw_iceberg_menu();return;}
    if(state_.journal||v.map)draw_iceberg_journal();
    if(state_.paused) {
      frame(8,5,cols_-10,9,d.stone);
      center(10,tr("CHWILA ODDECHU","A MOMENT TO BREATHE"),Color::Gold);
      center(12,tr("ENTER / P wznów   M menu","ENTER / P resume   M menu"));
      center(14,tr("Postęp zapisuje się przy lampach i wyjściu.","Progress saves at lamps and when you leave."));
    }
    if(state_.won&&!v.dialogue.active()) {
      frame(8,5,cols_-10,8,d.accent);
      center(10,tr("DROGA OTWARTA","THE WAY IS OPEN"),Color::Gold);
      center(12,tr("E / ENTER przejdź dalej","E / ENTER continue"));
      center(14,tr("R wróć do lampy, jeśli coś zostało z tyłu.","R returns to the lamp if you left something behind."));
    }
    draw_iceberg_dialogue();
  }
