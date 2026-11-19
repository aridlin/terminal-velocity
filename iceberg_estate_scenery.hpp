// RenderGrid member: suspended homes have lit windows, balcony rails and
// overhead cables; their silhouettes stay behind solid walking surfaces.
  void draw_estate_clothesline(const Rect &line,size_t index) {
    const double row=aspect_*scale_;
    const auto count=int(line.w/scale_);
    std::string rope="╞";
    for(int column=2;column<count;++column)rope+="─";
    rope+="╡";
    world_text(line.x,line.y,rope,0xe6bb8d);
    world_text(line.x,line.y-row,"┬",0x9c8198);
    world_text(line.right()-scale_,line.y-row,"┬",0x9c8198);
    for(int garment=0;garment<3;++garment) {
      const double x=line.x+line.w*(.18+.27*garment);
      const bool blown=std::sin(state_.adventure.presentation_time*3.+garment+index)>.05;
      if(garment==1)
        world_art(x,line.y+row,blown?std::vector<std::string>{"┌──┐"," ╲▒│","  ╲┘"}:
          std::vector<std::string>{"┌──┐","│▒▒│","└──┘"},0xc4b9a2);
      else
        world_art(x,line.y+row,blown?std::vector<std::string>{"╭┴┬╮","╰▥╲"," ╵ ╵"}:
          std::vector<std::string>{"╭┴┬╮","╰▥▥╯"," ╵╵ "},garment?0x9bb9c0:0xc38da7);
    }
  }
  void draw_estate_landmarks() {
    const auto &d=iceberg::Districts[2];
    if(grid_.room_role==iceberg::RoomRole::Settlement) {
      world_art(2,33,{"    ╭──────────────────────────╮","    │   OSIEDLE  P A R A P E T │","╭───┴──────────────────────────┴───╮","│  ╭───╮   ╭───╮   ╭───╮   ╭───╮   │","│  │ ░ │   │ ░ │   │ ░ │   │ ░ │   │","│  ╰───╯   ╰───╯   ╰───╯   ╰───╯   │","├──────────────────────────────────┤","│  ╭──────╮    ╭──────────────╮    │","│  │  02  │    │ ▯  ▯  ▯  ▯  │     │","│  ╰──────╯    ╰──────────────╯    │","╰──┬──────────────────────────┬────╯","   ║                          ║"},d.stone);
      world_art(39,26,{"       ╱╲                ╱╲","  ╭───╱──╲──────────────╱──╲──╮","  │  ╭────╮  ╭────╮  ╭────╮  │","  │  │ ░░ │  │ ░░ │  │ ░░ │  │","  │  ╰────╯  ╰────╯  ╰────╯  │","  ├───────────────────────────┤","  │   SPÓŁDZIELNIA PARAPET    │","  │   ╭──╮    ╭──────────╮    │","  │   │  │    │ ≋≋≋≋≋≋≋  │    │","  │   │○ │    ╰──────────╯    │","  ╰───┴──┴────────────────────╯"},d.stone);
      world_art(79,34,{"   ╭─────────────╮","   │  PĘTLA  02  │","   ╰──────┬──────╯","   ╭──────┴──────╮","   │  ▯ ▯ ▯ ▯ ▯  │","   ╰──◉──────◉───╯","══════════════════════"},d.accent);
      world_art(104,26,{"╭──────────────╮","│ ░░ ░░  ░░ ░░ │","│              │","│  ╭─╮    ╭─╮  │","╰──┴─┴────┴─┴──╯"},d.stone);
      world_art(20,32,{"╥───────────────╥","║  ╭╮   ╭──╮    ║","║  ││   │  │    ║","║  ╰╯   ╰──╯    ║"},int(state_.elapsed*2)%2?0x9c8198:0x887486);
      if(state_.adventure.quests[3]==3)world_art(55,28,{"╱╲   ╱╲   ╱╲","││   ││   ││","╨╨═══╨╨═══╨╨"},0xc7d6a4);
      return;
    }
    for(size_t i=0;i<grid_.route_points.size();++i) {
      auto [x,y]=grid_.route_points[i];
      if(i%2==0) {
        world_art(x-5,y-12.5,{
          "        ╱╲             ╱╲",
          "   ╭───╱──╲───────────╱──╲───╮",
          "   │ ╭────╮ ╭────╮ ╭────╮    │",
          "   │ │ ░░ │ │ ░░ │ │ ░░ │    │",
          "   │ ╰────╯ ╰────╯ ╰────╯    │",
          "   ├─────────────────────────┤",
          "   │ ╭────╮ ╭────╮ ╭────╮    │",
          "   │ │ ░░ │ │    │ │ ░░ │    │",
          "   │ ╰────╯ ╰────╯ ╰────╯    │",
          "   ├─────────────────────────┤",
          "   │ ╭──────╮  ╭──────╮      │",
          "   │ │      │  │  ░░  │      │",
          "   │ │    ○ │  ╰──────╯      │",
          "   ╰─┴──────┴────────────────╯"},d.shadow);
        if(i%4==0)world_art(x-2,y-9.9,{"╭────╮","│ ░░ │","╰────╯"},0x776677);
      }
      if(i%2==0)for(double cy=5;cy<y-2;cy+=.8)world_text(x-4,cy,"┆",d.shadow);
      if(i%3==0)world_art(x-5,y+1.8,{"╔═══════════════════════╗","║  ╭───╮  ╭───╮  ╭───╮  ║","║  │ ░ │  │ ░ │  │ ░ │  ║","║  ╰───╯  ╰───╯  ╰───╯  ║","╟───────────────────────╢","║ ╲                   ╱ ║","║  ╲                 ╱  ║","    ╲               ╱"},d.stone);
      else if(i%3==1)world_art(x-3,y+1.8,{"┬─────────────────┬","│ ┌─────┐ ┌─────┐ │","│ │ ░░░ │ │ ░░░ │ │","│ └─────┘ └─────┘ │","├─┬─┬─┬─┬─┬─┬─┬─┬─┤","╰─┴─┴─┴─┴─┴─┴─┴─┴─╯","  ╲             ╱"},d.shadow);
      if(i%4==2)world_art(x-3,y-3.5,{"╭──────╮ ╭──────╮","│ ▯  ▯ │ │ ▯  ▯ │","╰──┬───╯ ╰──┬───╯","   │        │"},d.stone);
    }
    for(const auto &platform:grid_.platforms)if(std::abs(platform.dx)>2) {
      auto car=platform.at(state_.elapsed);
      world_art(car.x+.5,car.y+.6,{"╭──────────────────────╮","│ ▯ ▯ ▯ ▯  ║  ▯ ▯ ▯ ▯  │","│          ║           │","╰──◉───────╨───────◉───╯"},d.stone);
    }
    if(grid_.template_id==76)world_art(3,47,{"╭─────────────────────────╮",tr("│  PRZYSTANEK SZKOLENIOWY │","│     PRACTICE STOP       │"),"╰───────────┬─────────────╯","            │"},d.light);
    if(grid_.template_id==77) {
      world_art(126,24,{"      ╭───────────────╮","  ╭───┤  ROZDZIELNIA  ├───╮","  │   ╰───────────────╯   │","  │ ╭────╮ ╭────╮ ╭────╮ │","══╡ │ ▯▯ │ │ ▯▯ │ │ ▯▯ │ ╞══","  │ ╰────╯ ╰────╯ ╰────╯ │","  ╰─────╥─────────╥──────╯","        ║         ║"},d.accent);
      if(grid_.switch_flags==7)world_art(51,13,{"╭────────────────────────╮",tr("│  NASTĘPNY: ARCHIWUM    │","│  NEXT STOP: ARCHIVE    │"),"╰────────────────────────╯"},d.light);
    }
    if(grid_.template_id>=24&&grid_.template_id<36) {
      const auto &layout=iceworld::pattern(grid_.template_id);auto p=layout.path[layout.path.size()/2];
      switch(grid_.template_id%12) {
      case 0:case 8:world_art(p.x,p.y-6,{"╭──────────────────────╮","│ 02  →  →  →  →  →  → │","├──────────────────────┤","│  ▥▥▥▥▥▥   ▥▥▥▥▥▥     │","╰───────────┬──────────╯","            │"},d.accent);break;
      case 1:
        if(grid_.solid_at(iceworld::EstateStairwellWalls.front()[0],iceworld::EstateStairwellWalls.front()[1])) {
          draw_estate_stairwells();
          break;
        }
        [[fallthrough]];
      case 6:world_art(p.x-2,p.y-6,{"╔════════════════╗","║  ╲ ▯ ▯ ▯ ▯ ╱   ║","║   ╲       ╱    ║","║    ╲     ╱     ║","║     ╲   ╱      ║","║      ╳         ║"},d.stone);break;
      case 2:world_art(p.x,p.y-7,{"╥─────────────────────────╥","║  ╭─╮  ╭──╮   ╭───╮      ║","║  │ │  │  │   │   │      ║","║  ╰─╯  ╰──╯   ╰───╯      ║"},0xb098aa);break;
      case 4:world_art(p.x,p.y-5,{"╭─────────────────────╮","│ ░░  11   ░░░░░░░░░░ │","├─┬─┬─┬─┬─┬─┬─┬─┬─┬─┬─┤","╰─┴─┴─┴─┴─┴─┴─┴─┴─┴─┴─╯"},d.light);break;
      case 5:world_art(p.x,p.y-6,{"  ╱╲╱╲╱╲╱╲╱╲╱╲╱╲╱╲"," ╱________________╲"," │ ▱▱ ▱▱ ▱▱ ▱▱ ▱▱ │"," ├────────────────┤"," │   │        │   │"},d.accent);break;
      case 7:world_art(p.x-3,p.y-9,{" ╥               ╥"," ║ ╭───────────╮ ║"," ║ │  ▒▒▒▒▒▒▒  │ ║"," ║ │  ▒▒▒▒▒▒▒  │ ║"," ║ ╰─────┬─────╯ ║"," ║       │       ║"},d.stone);break;
      case 9:
        if(iceworld::has_estate_gutter(grid_))draw_estate_gutters();
        else world_art(p.x-2,p.y-8,{"╔══╗","║  ║","║  ╚══════╗","╚══════╗  ║","       ║  ║","       ║  ║"},0x657f8a);
        break;
      default:world_art(p.x,p.y-4,{"  ╲│╱    ╲│╱    ╲│╱","   │      │      │","╭──┴──╮╭──┴──╮╭──┴──╮","╰─────╯╰─────╯╰─────╯"},0x9aab85);break;
      }
    }
  }
  void draw_estate_gutters() {
    constexpr uint32_t copper=0xa78e7d;
    const double row=aspect_*scale_;
    for(size_t i=0;i<iceworld::EstateGutterWalls.size();++i) {
      const auto [x,y,w,h]=iceworld::EstateGutterWalls[i];
      const double center=x+w*.5;
      const double inlet=y-row;
      draw_scenery_pipe({{center+(i?6.:-6.),inlet},{center,inlet},{center,double(y)}},copper);
      world_text(center,inlet,i?"╔":"╗",copper);
      for(int segment=0;segment*row<h;++segment)
        world_text(center,y+segment*row,segment>0&&segment%4==0?"╫":"║",copper);
      if(i)draw_pipe_drip(center,y+h+.05,41.,.8);
    }
  }

  void draw_estate_stairwells() {
    const double row=aspect_*scale_;
    constexpr uint32_t plaster=0x88788d,mortar=0xc5a6b9,window=0xc9c088;
    for(size_t tower=0;tower<iceworld::EstateStairwellWalls.size();++tower) {
      const auto [x,y,w,h]=iceworld::EstateStairwellWalls[tower];
      const double wall_face=x+.12,x_back=tower==0?x-9*scale_:x+w-scale_;
      const double cap=y+row*.2;
      const int floors=h>10?3:2;
      const double storey=(h-row)/floors;
      world_art(x_back,cap,{"╭────────╮"},plaster);
      for(int i=0;i<floors;++i) {
        const double floor=cap+row+i*storey;
        world_art(x_back,floor,{"│ ┌────┐ │","│ │░░░░│ │","│ └─┬──┘ │","├───┴────┤"},plaster);
        int phase=int(state_.adventure.presentation_time*.7+i+tower*2)%5;
        if(phase<3)world_art(x_back+3*scale_,floor+row,{phase==0?"░░▒░":"▒░░░"},window);
      }
      world_art(x_back,y+h-row,{"╰──┬───┬─╯"},plaster);
      for(int section=0;cap+section*row<y+h-row*.5;++section)
        world_text(wall_face,cap+section*row,section==0?"╥":section%4==0?"╫":"║",mortar);
    }
  }
