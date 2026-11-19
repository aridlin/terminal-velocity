// RenderGrid members. Glass, irrigation and rootwork share the world camera.
  void draw_leaf_ferry(const MovingPlatform &platform,const Rect &leaf) {
    const int cells=std::max(4,int(leaf.w/scale_));
    const int middle=cells/2;
    const double row=aspect_*scale_;
    std::string rim="╶",veins="╲";
    for(int i=1;i<cells-1;++i) {
      rim+="━";
      veins+=i==middle?"╿":i%4==2?(i<middle?"╲":"╱"):"▒";
    }
    world_text(leaf.x,leaf.y,rim+"╴",0xb6d778);
    world_art(leaf.x,leaf.y+row,{veins+"╱"},0x638b55);
    const int inset=std::max(1,cells/6);
    std::string underside="╰";
    for(int i=inset+1;i<cells-inset-1;++i)underside+=i==middle?"┬":"─";
    world_art(leaf.x+inset*scale_,leaf.y+2*row,{underside+"╯"},0x789759);
    const double travel=(1+platform.offset(state_.elapsed))*.5;
    const int glint=1+int(travel*(cells-3));
    world_text(leaf.x+glint*scale_,leaf.y,"━",0xd5eda9);
  }

  void draw_garden_pod(const Rect &pod) {
    const double row=aspect_*scale_;
    const int cells=std::max(3,int(std::round(pod.w/scale_)));
    const double x=pod.x+(pod.w-cells*scale_)*.5;
    std::string shell,stem;
    for(int i=1;i<cells-1;++i) {
      shell+=i==cells/2?"●":"─";
      stem+=i==cells/2?"┬":"─";
    }
    for(double y=pod.y+row;y<pod.bottom();y+=row)
      world_art(x+(cells/2)*scale_,y,{"│"},0x6d935c);
    world_art(x,pod.y,{"╰"+stem+"╯"},0x6d935c);
    world_art(x,pod.y-row,{"╭"+shell+"╮"},0xb9d77b);
  }

  bool draw_irrigation_wheel() {
    const auto first=std::find_if(grid_.platforms.begin(),grid_.platforms.end(),
      [](const MovingPlatform &platform){return platform.irrigation_scoop;});
    if(first==grid_.platforms.end())return false;
    const double x=first->base.x+first->base.w*.5,y=first->base.y;
    if(screen(22,y).first>=cols_||screen(54,y).first<0)return true;
    constexpr uint32_t iron=0x668e86, brass=0xc5a36d;
    const double row=aspect_*scale_;
    world_art(26-4*scale_,32-3*row,{"╭───────╮","│≈≈≈≈≈≈≈│","╰───╥───╯"},iron);
    draw_scenery_pipe({{26,32},{46,32},{46,34}},iron);
    draw_pipe_drip(46,34,y+first->dy+1,.27);
    draw_scenery_pipe({{x-4,y+first->dy+3},{x-4,y+first->dy+5},{53,y+first->dy+5}},iron);
    const auto stroke=[](double dx,double dy) {
      return std::abs(dx)>2*std::abs(dy)?"━":std::abs(dy)>2*std::abs(dx)?"┃":dx*dy>0?"╲":"╱";
    };
    for(int i=0;i<64;++i) {
      const auto rim=first->at(first->period*i/64.);
      const auto next=first->at(first->period*(i+1)/64.);
      world_text(rim.x+rim.w*.5,rim.y,stroke((next.x-rim.x)/scale_,(next.y-rim.y)/row),iron);
    }
    for(const auto &platform:grid_.platforms)if(platform.irrigation_scoop) {
      const auto deck=platform.at(state_.elapsed);
      const double dx=deck.x+deck.w*.5-x,dy=deck.y-y;
      const int cells=int(std::ceil(std::max(std::abs(dx)/scale_,std::abs(dy)/row)));
      const char *spoke=stroke(dx/scale_,dy/row);
      for(int cell=1;cell<cells;++cell) {
        const double fraction=double(cell)/cells;
        world_text(x+fraction*dx,y+fraction*dy,spoke,brass);
      }
    }
    world_text(x,y,"⊕",brass);
    return true;
  }

  void draw_irrigation_scoop(const Rect &deck) {
    const int cells=std::max(3,int(std::round(deck.w/scale_)));
    std::string rim="╓",body="╰";
    for(int cell=1;cell<cells-1;++cell){rim+="━";body+="≈";}
    world_text(deck.x,deck.y,rim+"╖",0xc7e3a5);
    world_art(deck.x,deck.y+aspect_*scale_,{body+"╯"},0x82bec0);
  }

  void draw_irrigation_bucket(const MovingPlatform &platform,const Rect &deck) {
    const double row=aspect_*scale_, elapsed=state_.elapsed;
    const double left=deck.x+2*scale_, right=deck.right()-3*scale_;
    const double head=platform.base.y-std::abs(platform.dy)-2*row;
    const double drain=platform.base.y+std::abs(platform.dy)+5*row;
    constexpr uint32_t frame_color=0x547b73, rim_color=0xc7e3a5, water_color=0x70afb0;
    draw_scenery_pipe({{left,drain},{left,head},{right,head},{right,drain}},frame_color);
    const int span=std::max(2,int(std::round((right-left)/scale_))-1);
    std::string trough="╰";
    for(int cell=0;cell<span;++cell)trough+="─";
    world_art(left,drain,{trough+"╯"},frame_color);
    world_art(left,head,{"◉"},frame_color);
    world_art(right,head,{"◉"},frame_color);

    const double center=deck.x+deck.w*.5;
    world_art(center-7*scale_,deck.y+row,{"╲            ╱"," ╰────┬─────╯","      ╨"},frame_color);
    const double age=std::fmod(elapsed+platform.phase,.87);
    const double lip=platform.at(std::max(0.,elapsed-age)).y+3*row;
    const double drop=lip+4*age*age;
    if(drop>deck.y+3.5*row&&drop<drain)
      world_art(center-scale_,drop,{age<.45?"·":"╵"},water_color);
    world_art(center-5*scale_,deck.y+row,{"≈≈≈≈≈≈≈≈≈"},water_color);
    std::string rim="╞";
    for(int cell=1;cell<int(deck.w/scale_)-1;++cell)rim+="━";
    world_text(deck.x,deck.y,rim+"╡",rim_color);
    const double rise=deck.y-platform.at(elapsed+.05).y;
    world_art(right,deck.y+row,{rise>.025?"↑":rise<-.025?"↓":"◆"},rim_color);
  }

  void draw_garden_landmarks() {
    const auto &d=iceberg::Districts[4];double clock=state_.adventure.presentation_time;
    auto greenhouse=[&](double x,double y){world_art(x,y,{
      "          ╭───────────╮","      ╭───┼─────┬─────┼───╮","   ╭──┤   │     │     │   ├──╮",
      "  ╱   │   │     │     │   │   ╲"," ╱────┼───┼─────┼─────┼───┼────╲",
      "│     │   │     │     │   │     │","│     │   │     │     │   │     │",
      "├─────┼───┼─────┼─────┼───┼─────┤","│     │   │     │     │   │     │",
      "│     │   │     │     │   │     │","╰─────┴───┴─────┴─────┴───┴─────╯"},d.shadow);};
    auto bed=[&](double x,double y){world_art(x,y,{
      "    ╭╮     ╭─╮      ╭╮","  ╭─╯╰╮  ╭─╯ ╰╮  ╭─╯╰╮","   ╲│╱     ╲│╱     ╲│╱",
      "╭───┴───────┴───────┴───╮","│ ░░░░░░░░░░░░░░░░░░░░░ │","╰──┬─────────────────┬──╯"},d.stone);};
    auto wheel=[&](double x,double y){
      world_art(x,y,{"      ╭────────╮","   ╭──┤        ├──╮","  ╱   │        │   ╲"," │    ╰────────╯    │"," │                  │","  ╲   ╭────────╮   ╱","   ╰──┤        ├──╯","      ╰────────╯"},d.stone);
      static const char *spokes[]={"╲ │ ╱","─ ◉ ─","╱ │ ╲","╶ ⊕ ╴"};world_text(x+7,y+3.5*aspect_*scale_,spokes[int(clock*3)%4],d.accent);
    };
    if(grid_.room_role==iceberg::RoomRole::Settlement) {
      greenhouse(3,29);greenhouse(70,13);greenhouse(110,27);
      bed(9,43);bed(29,33);bed(73,27);bed(116,33);
      world_art(49,30,{"    ╭──────╮","╭───┤ ░░░░ ├───╮","│   │ ░░░░ │   │","│   ╰──┬───╯   │","│      ║       │","╰══════╩═══════╯"},d.stone);
      world_art(96,39,{"╭────────────────────╮",tr("│  ZAMYKAĆ ZA SOBĄ   │","│ CLOSE BEHIND YOU   │"),"╰─────────┬──────────╯","          │"},d.light);
    } else {
      for(size_t i=0;i<grid_.route_points.size();++i){auto [x,y]=grid_.route_points[i];
        if((grid_.template_id==50||grid_.template_id==57)&&std::any_of(grid_.platforms.begin(),grid_.platforms.end(),[&](const MovingPlatform &p){
          return p.base.y==y&&p.base.x<=x&&x<p.base.right();
        }))continue;
        if(i%3==0)greenhouse(x-8,y-16);
        if(i%3==1)bed(x-5,y+2);
        if(i%3==2)world_art(x,y+2,{"╲       ╱"," ╲ ╭─╮ ╱","  ╲│ │╱","   │ │","  ╱│ │╲"," ╱ ╰─╯ ╲"},d.shadow);
      }
      int id=grid_.template_id;
      if(id==49||id==53)world_art(36,8,{"    ╭───╮","╭───┤   ├────╮","╰╮  │   │   ╭╯"," │╲ │   │ ╱ │"," │ ╲│   │╱  │"," │  │   │   │","╭╯  │   │   ╰╮","╰───┤   ├────╯","    │   │","    │   │","   ╱│   │╲","  ╱ ╰───╯ ╲"},d.stone);
      if(id==55||id==52)world_art(46,47,{"╭────────────────────────────────╮","│  ≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋   │","│     ╲        ╲        ╲        │","╰────────────────────────────────╯"},d.shadow);
      if(id==52&&!grid_.growth.empty()) {
        draw_scenery_pipe({{81.5,40.5},{81.5,44},{86,44},{86,42}},0x5f8790);
        world_art(85,41.5,{"╔═╗","║░║","╚╤╝"},0x8bb3a2);
      }
      if(id==55&&!grid_.water_basins.empty()) {
        draw_scenery_pipe({{53,50},{53,53},{74,53},{74,48},{76,48}},0x5f8790);
        world_text(74,47.2,"╔╤╗",0x9dbbb7);
        if(std::fmod(clock,1.25)<.7)world_text(75.2,48.5,"·",0x92e1ec);
      }
      if(id==56)world_art(60,30,{"╭──────────────────────╮","│  ╭─╮   ╭─╮   ╭─╮     │","│  │⠿│   │⠿│   │⠿│     │","│  ╰─╯   ╰─╯   ╰─╯     │","├──────────────────────┤","│  01    02    03      │","╰──────────────────────╯"},d.stone);
      if((id==57&&!draw_irrigation_wheel())||id==81)wheel(88,25);
      if(id==48||id==59)world_art(67,8,{"     ╭───────────────╮","  ╭──┤░░░░░░░░░░░░░░░├──╮","  ╰──┴───────────────┴──╯","    ╲       │       ╱","     ╲      │      ╱"},d.stone);
      if(id==51||id==54||id==58)for(int i=0;i<4;++i)world_art(35+i*21,22-i*2,{"┬       ┬","│ ╭─╮   │","│╭╯ ╰╮  │","╰┤   ├──╯"," ╰───╯"},d.stone);
    }
    for(const auto &bridge:grid_.growth) {
      const auto &area=bridge.area;
      const double width=bridge.extended_width(grid_.switch_flags);
      const double row=aspect_*scale_;
      world_art(area.x-2,area.bottom(),{"╭─╮","│╭╯","╰╯"},d.stone);
      if(width<=0)continue;
      const double tip=area.x+width;
      constexpr uint32_t leaf=0xa6c780,flower=0xe5d48f;
      for(double x=area.x+1;x+3*scale_<tip;x+=3) {
        world_art(x,area.y-row,{"❧"},leaf);
        world_art(x,area.bottom()+row,{"╲│╱"," │ "," ╰╮"},d.accent);
      }
      const bool complete=width>=std::floor(area.w);
      world_art(tip-scale_,area.y-2*row,{complete?"✿":"❧","╵"},complete?flower:leaf);
    }
    if(grid_.template_id!=50&&grid_.template_id!=57)
      for(const auto &platform:grid_.platforms)if(!platform.icy) {
        const auto r=platform.at(state_.elapsed);
        world_art(r.x,r.y+.7,{"╲──────┬──────╱"," ╲─────┴─────╱"},d.stone);
      }
    // Pollen stays muted and sparse; collision edges remain the brightest layer.
    for(int i=0;i<8;++i)world_text(std::fmod(i*23.1+std::sin(clock*.3+i)*2+grid_.width(),double(grid_.width())),std::fmod(i*8.7+clock*.4,58.),i%2?"⠁":"·",d.shadow);
  }
