  void draw_estate_tram(const MovingPlatform &platform,const Rect &tram) {
    const double row=aspect_*scale_;
    const double left=platform.base.x-platform.dx;
    const double right=platform.base.right()+platform.dx;
    const double track_y=tram.y+3*row;
    std::string rail="╞";
    for(int i=1;i<int((right-left)/scale_)-1;++i)rail+=(i%4==0?"╪":"═");
    world_art(left,track_y,{rail+"╡"},0x71647d);
    const double travel=platform.offset(state_.elapsed);
    world_art(left+2*scale_,tram.y+row,{"╥","║",travel<=-1?"●":"○","╨"},travel<=-1?0xefcd74:0x71647d);
    world_art(right-scale_,tram.y+row,{"╥","║",travel>=1?"●":"○","╨"},travel>=1?0xefcd74:0x71647d);

    std::string roof="╭",windows="│",sill="╰";
    const int cells=std::max(4,int(tram.w/scale_));
    for(int i=1;i<cells-1;++i) {
      roof+="━";
      windows+=(i%5==0?"│":i%5==1?"▐":i%5==4?"▌":"░");
      sill+=(i==2||i==cells-3?"●":"─");
    }
    world_text(tram.x,tram.y,roof+"╮",0xe4ad68);
    world_art(tram.x,tram.y+row,{windows+"│",sill+"╯"},0xc0a487,true);
  }

// Repeated infrastructure follows the landings in expeditions. Keep the art
// behind the route so decorations never become apparent stepping stones.
  void draw_expedition_infrastructure() {
    if(state_.adventure.mode==iceberg::Mode::Campaign||grid_.district==0||
       grid_.room_role!=iceberg::RoomRole::Traverse)return;
    const double row=aspect_*scale_;
    const auto &district=iceberg::Districts[size_t(grid_.district)];
    for(size_t i=1;i+1<grid_.route_points.size();i+=2) {
      const auto [center,floor]=grid_.route_points[i];
      const int phase=int(std::floor(state_.adventure.presentation_time*2+i))%6;
      switch(grid_.district) {
      case 1: {
        const double pipe_y=floor+3*row;
        draw_scenery_pipe({{center-4,pipe_y},{center+3,pipe_y},{center+3,pipe_y+4*row},
          {center+6,pipe_y+4*row}},0x687481);
        world_art(center-3,pipe_y-2*row,{"╭─┬─╮","│ ◴ │","╰─╥─╯"},0x778591);
        draw_pipe_drip(center+5,pipe_y+4*row,pipe_y+8*row,i*.21);
        break;
      }
      case 2:
        world_art(center-4,floor-7*row,
          {"╥══════════════════╥","║   ╲          ╱   ║","║    ╲──┬─────╱    ║",
           "║       │          ║","║      ╔╧╗         ║","║      ║ ║         ║","╨      ╚═╝         ╨"},0x645e72);
        world_art(center-4+8*scale_,floor-2*row,{phase<2?"●":"○"},phase<2?0xb8c89c:0x65766d);
        break;
      case 3:
        world_art(center-3.5,floor+2*row,
          {"╥────────────────╥","║ ▤▤  ▤▤  ▤▤  ▤▤ ║","╟────────────────╢",
           "║ ▤▤  ▤▤  ▤▤  ▤▤ ║","╟────────────────╢","╨                ╨"},0x48626a);
        if(phase<3)world_art(center-1.5,floor+5*row,{"▤▤"},0x718883);
        break;
      case 4:
        world_art(center-4,floor-7*row,
          {"╭───┬───────┬───╮","│╲  │       │  ╱│","│ ╲ │       │ ╱ │","│   ╰───┬───╯   │",
           "│       │       │","│      ❧│♧      │","╨     ╰─┴─╯     ╨"},0x516e62);
        if(phase<3)world_art(center-.8,floor-(3-phase)*row,{"·"},0x80ab86);
        break;
      case 5:
        world_art(center-4,floor+2*row,
          {"╥━━━━━━━━━━━━━━━━╥","║  ╭──╮    ╭──╮  ║","║  │╱ │    │╱ │  ║",
           "║  ╰──╯    ╰──╯  ║","╨━━━━━━━━━━━━━━━━╨"},district.stone);
        if(phase==1||phase==2)world_art(center+scale_,floor+4*row,{"✦"},0x86aebd);
        break;
      }
    }
  }
