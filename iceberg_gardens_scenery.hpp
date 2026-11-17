// RenderGrid members. Glass, irrigation and rootwork share the world camera.
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
        if(i%3==0)greenhouse(x-8,y-16);
        if(i%3==1)bed(x-5,y+2);
        if(i%3==2)world_art(x,y+2,{"╲       ╱"," ╲ ╭─╮ ╱","  ╲│ │╱","   │ │","  ╱│ │╲"," ╱ ╰─╯ ╲"},d.shadow);
      }
      int id=grid_.template_id;
      if(id==49||id==53)world_art(36,8,{"    ╭───╮","╭───┤   ├────╮","╰╮  │   │   ╭╯"," │╲ │   │ ╱ │"," │ ╲│   │╱  │"," │  │   │   │","╭╯  │   │   ╰╮","╰───┤   ├────╯","    │   │","    │   │","   ╱│   │╲","  ╱ ╰───╯ ╲"},d.stone);
      if(id==55||id==52)world_art(46,47,{"╭────────────────────────────────╮","│  ≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋  │","│     ╲        ╲        ╲         │","╰────────────────────────────────╯"},d.shadow);
      if(id==56)world_art(60,30,{"╭──────────────────────╮","│  ╭─╮   ╭─╮   ╭─╮     │","│  │⠿│   │⠿│   │⠿│     │","│  ╰─╯   ╰─╯   ╰─╯     │","├──────────────────────┤","│  01    02    03      │","╰──────────────────────╯"},d.stone);
      if(id==57||id==81)wheel(88,25);
      if(id==48||id==59)world_art(67,8,{"     ╭───────────────╮","  ╭──┤░░░░░░░░░░░░░░░├──╮","  ╰──┴───────────────┴──╯","    ╲       │       ╱","     ╲      │      ╱"},d.stone);
      if(id==51||id==54||id==58)for(int i=0;i<4;++i)world_art(35+i*21,22-i*2,{"┬       ┬","│ ╭─╮   │","│╭╯ ╰╮  │","╰┤   ├──╯"," ╰───╯"},d.stone);
    }
    for(const auto &bridge:grid_.growth) {
      bool alive=(grid_.switch_flags&bridge.flag)==bridge.flag;
      int width=alive?int(std::floor(bridge.area.w*bridge.amount)):0;
      world_art(bridge.area.x-2,bridge.area.y+2,{"╭─╮","│╭╯","╰╯"},d.stone);
      for(int i=0;i<width;i+=3)world_art(bridge.area.x+i,bridge.area.y+2,{"╲│╱"," │ "," ╰╮"},d.accent);
      if(alive&&bridge.amount<1)world_text(bridge.area.x+width,bridge.area.y,"╭╯",d.light);
    }
    for(const auto &platform:grid_.platforms){auto r=platform.at(state_.elapsed);world_art(r.x,r.y+.7,{"╲──────┬──────╱"," ╲─────┴─────╱"},d.stone);}
    // Pollen stays muted and sparse; collision edges remain the brightest layer.
    for(int i=0;i<8;++i)world_text(std::fmod(i*23.1+std::sin(clock*.3+i)*2+grid_.width(),double(grid_.width())),std::fmod(i*8.7+clock*.4,58.),i%2?"⠁":"·",d.shadow);
  }
