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
