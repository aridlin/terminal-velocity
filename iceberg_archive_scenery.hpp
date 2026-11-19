  static constexpr double ArchiveCableY=21.;
  static constexpr uint32_t ArchiveRackMarker=0xe5b276;
  static constexpr uint32_t ArchiveTrolleyDocked=0xb5dba0;
  static const char *archive_rack_direction(const MovingPlatform &platform,double time) {
    const double motion=platform.at(time+.025).y-platform.at(time-.025).y;
    return motion<-.02?"▲":motion>.02?"▼":"■";
  }
  void draw_archive_rack(const MovingPlatform &platform,const Rect &deck) {
    const double row=aspect_*scale_;
    const int width=std::max(4,int(std::round(deck.w/scale_)));
    std::string surface="╔",files="║",base="╚";
    for(int i=1;i<width-1;++i) {
      surface+="═";files+=i%4==0?"│":i%4==1?" ":"▥";base+="═";
    }
    const double center=deck.x+deck.w*.5;
    world_text(deck.x,deck.y,surface+"╗",0xaecee0);
    world_text(deck.x,deck.y+row,files+"║",0x6d92a9);
    world_text(deck.x,deck.y+2*row,base+"╝",0x749cb3);
    world_text(center,deck.y-row*1.1,archive_rack_direction(platform,state_.elapsed),ArchiveRackMarker);
  }

  void draw_archive_stack_shuttle(const MovingPlatform &platform,const Rect &deck) {
    const double row=aspect_*scale_;
    const double track_y=deck.y+3*row;
    const double left=platform.base.x-platform.dx,right=platform.base.right()+platform.dx;
    const uint32_t rail=0x566f83,steel=0x87b3c5;
    draw_scenery_pipe({{left,track_y-row},{left,track_y},{right,track_y},{right,track_y-row}},rail);
    const double offset=platform.offset(state_.elapsed);
    const double stop_lamps[]={left+.8,right-.8};
    for(int stop=0;stop<2;++stop) {
      const auto x=stop_lamps[stop];
      const bool docked=stop==0?offset<=-1.:offset>=1.;
      world_art(x,deck.y-.8*row,{"│","╨"},rail);
      world_text(x,deck.y-1.6*row,docked?"●":"○",docked?0xb5dba0:rail);
    }

    const int width=std::max(4,int(std::round(deck.w/scale_)));
    std::string files="║";
    for(int i=1;i<width-1;++i)files+=i%5==0?"│":i%2==0?"▥":"▤";
    std::string surface="╞",base="╚";
    for(int i=1;i<width-1;++i){surface+="═";base+="═";}
    world_text(deck.x,deck.y,surface+"╡",0xbce0e6);
    world_art(deck.x,deck.y+row,{files+"║",base+"╝"},steel);
    for(double wheel:{deck.x+1.6,deck.right()-2.})
      world_text(wheel,track_y,"◉",steel);
  }

  void draw_archive_trolley(const Rect &deck) {
    const int width=std::max(3,int(std::round(deck.w/scale_)));
    std::string surface="╞",tray="╰";
    for(int i=1;i<width-1;++i){surface+="═";tray+=i%2?"▤":"─";}
    world_text(deck.x,deck.y,surface+"╡",0xaee1df);
    world_text(deck.x,deck.y+aspect_*scale_,tray+"╯",0x749da5);
  }

  void draw_envelope_canopy() {
    const double row=aspect_*scale_;
    // Paper hangs from steel ribs and runoff splashes on the lower walkways.
    world_art(44,24,{"╔══════════════════════════════════╗",
                     "╚══╤══════╤══════╤══════╤══════╤═══╝"},0x64798a);
    for(int i=0;i<5;++i)
      world_art(45+(2+7*i)*scale_,24+2*row,
        {"  │  ","┌─┴─┐","│╲ ╱│","└─V─┘"},0x64798a);
    draw_scenery_pipe({{48,25},{48,33},{63,33},{63,34},{70,34}},0x687e8b);
    draw_pipe_drip(70,34,42,.35);
    draw_scenery_pipe({{108,18},{113,18},{113,38},{116,38}},0x687e8b);
    draw_pipe_drip(116,38,50,1.1);
    for(const auto &platform:grid_.platforms)if(platform.archive_trolley) {
      const double center=platform.base.x+platform.base.w*.5;
      const double rail_y=platform.base.y-5.2;
      const auto [left,bottom]=screen(center-platform.dx,rail_y-platform.dy);
      const auto [right,top]=screen(center+platform.dx,rail_y+platform.dy);
      auto rail_cell=[&](int x,int y,const char *glyph) {
        world_art(state_.camera_x+(x+.1)*scale_,state_.camera_y+(y-4+.1)*row,{glyph},0x6d8892);
      };
      int y=bottom;
      rail_cell(left,y,"╞");
      for(int x=left+1;x<right;++x) {
        const int next=bottom-int(std::round(double(x-left)*(bottom-top)/(right-left)));
        if(next<y) {
          rail_cell(x,y,"╝");
          for(int bridge=y-1;bridge>next;--bridge)rail_cell(x,bridge,"║");
          rail_cell(x,next,"╔");
        } else rail_cell(x,y,"═");
        y=next;
      }
      rail_cell(right,y,"╡");
      const int docked=platform.stop(state_.elapsed);
      for(int side:{-1,1}) {
        const double stop_x=center+side*platform.dx;
        const double stop_y=rail_y+side*platform.dy;
        const double lamp_x=stop_x+side*1.6;
        constexpr uint32_t socket=0x6d8892;
        draw_scenery_pipe({{stop_x,stop_y},{lamp_x,stop_y},{lamp_x,stop_y+row}},socket);
        world_art(lamp_x-scale_,stop_y+row,{"╭┴╮","│ │","╰─╯"},socket);
        world_art(lamp_x,stop_y+2*row,{docked==side?"●":"○"},docked==side?ArchiveTrolleyDocked:socket);
      }
      const auto deck=platform.at(state_.elapsed);
      const double rider=deck.x+deck.w*.5;
      world_art(rider,deck.y-5.2,{"◉"},0x96b3b8);
      for(double y=deck.y-5.2+row;y<deck.y;y+=row)
        world_art(rider,y,{"│"},0x82a0ab);
    }
  }

// RenderGrid members. Shelves and catalogue rails belong to the room's
// architecture; moving carriages stay attached to their collision platform.
  void draw_archive_landmarks() {
    auto d=iceberg::Districts[3];
    // Archive decoration recedes behind pale collision edges and warm task props.
    d.stone=0x344253;d.shadow=0x253144;d.accent=0x4c6075;d.light=0x63738a;
    const double clock=state_.adventure.presentation_time;
    if(grid_.room_role==iceberg::RoomRole::Settlement) {
      world_art(3,32,{"          ╱────────────────────────╲","    ╔════╧══════════════════════════╧════╗","    ║  A R C H I W U M   M I E J S K I E ║","    ╠════════════════════════════════════╣","    ║ ╭────────╮  ╭────────╮ ╭────────╮  ║","    ║ │ ▥▥▥▥▥▥ │  │ ▥▥▥▥▥▥ │ │ ▥▥▥▥▥▥ │  ║","    ║ ├────────┤  ├────────┤ ├────────┤  ║","    ║ │ ▥▥▥▥▥▥ │  │ ▥▥▥▥▥▥ │ │ ▥▥▥▥▥▥ │  ║","    ║ ╰────────╯  ╰────────╯ ╰────────╯  ║","    ║      ╭────────────────────╮        ║","    ║      │     CZYTELNIA       │       ║","    ╚══════╧════════════════════╧════════╝"},d.stone);
      world_art(9,44,{"    ╭─────╮       ╭─╮","  ╭─┴─────┴─╮   ╭─┴─┴─╮","══╧═════════╧═══╧═════╧════","  │                   │"},d.accent);
      world_art(57,24,{"       ╱╲             ╱╲","╭─────╱──╲───────────╱──╲─────╮","│ ╭──╮ ╭──╮ ╭──╮ ╭──╮ ╭──╮  │","│ │01│ │02│ │03│ │04│ │05│  │","│ ╰──╯ ╰──╯ ╰──╯ ╰──╯ ╰──╯  │","├───────────────────────────┤","│ ╭──╮ ╭──╮ ╭──╮ ╭──╮ ╭──╮  │","│ │06│ │07│ │08│ │09│ │10│  │","│ ╰──╯ ╰──╯ ╰──╯ ╰──╯ ╰──╯  │","╰───────────────────────────╯"},d.shadow);
      world_art(95,31,{"       ╭─────╮","   ╭───┴─────┴───╮","   │  ╭───────╮  │","   │  │  ╱╲   │  │","   │  │ ╱  ╲  │  │","   │  ╰───────╯  │","   ╰───┬─────┬───╯","       │     │"},d.stone);
      world_art(104,43,{"╭──────────────────╮",tr("│ZWROTY BEZ KOLEJKI│","│  RETURNS DESK    │"),"╰─────────┬────────╯","          │"},d.light);
    } else {
      for(size_t i=0;i<grid_.route_points.size();++i) {
        auto [x,y]=grid_.route_points[i];
        if(i%2==0)world_art(x-7,y-12,{
          "   ╭───────────────────────╮","   │ ╭────╮ ╭────╮ ╭────╮  │",
          "   │ │▥▥▥▥│ │▥▥▥▥│ │▥▥▥▥│  │","   │ ╰────╯ ╰────╯ ╰────╯  │",
          "   ├───────────────────────┤","   │ ╭────╮ ╭────╮ ╭────╮  │",
          "   │ │▥▥▥▥│ │▥▥▥▥│ │▥▥▥▥│  │","   │ ╰────╯ ╰────╯ ╰────╯  │",
          "   ├───────────────────────┤","   │ ╭────╮ ╭────╮ ╭────╮  │",
          "   │ │▥▥▥▥│ │▥▥▥▥│ │▥▥▥▥│  │","   │ ╰────╯ ╰────╯ ╰────╯  │",
          "   ╰──┬────────────────┬───╯"},d.shadow);
        if(i%3==1)world_art(x-4,y+1.7,{"╥───────────────────╥","║ ▥▥▥  ▥▥▥▥  ▥▥▥▥▥  ║","╟───────────────────╢","║ ▥▥▥▥▥ ▥▥▥  ▥▥▥▥▥  ║","╟───────────────────╢","║  ▥▥▥▥▥▥ ▥▥▥▥▥▥▥   ║","╨───────────────────╨"},d.stone);
        if(i%3==0)world_art(x-3,y-5,{" ╭──────╮"," │ ════ │"," │ ════ │"," ╰──┬───╯","    │"},d.stone);
      }
      if(grid_.template_id>=36&&grid_.template_id<48) {
        const auto &layout=iceworld::pattern(grid_.template_id);auto p=layout.path[layout.path.size()/2];
        switch(grid_.template_id%12) {
        case 0:world_art(p.x-8,p.y-16,{"      ╱╲       ╱╲       ╱╲","     ╱┆ ╲     ╱┆ ╲     ╱┆ ╲","    ╱ ┆  ╲   ╱ ┆  ╲   ╱ ┆  ╲","═══╧══╧═══╧═╧══╧═══╧═╧══╧═══╧═══","    ╲      ╲      ╲      ╲"},d.stone);break;
        case 1:case 6:world_art(p.x-5,p.y-9,{"╭───────────────────╮","│ A–F │ G–M │ N–Ż   │","├─────┼─────┼───────┤","│  ▰  │  ▰  │  ▰    │","├─────┼─────┼───────┤","│  ▰  │  ▰  │  ▰    │","╰─────┴─────┴───────╯"},d.accent);break;
        case 2:world_art(p.x-3,p.y-9,{"╔════════════════════════╗","║ ╭────────────────────╮ ║","║ │  ▱       ▱         │ ║","║ ╰────────────────────╯ ║","╚══════════╦══╦══════════╝","           ║  ║","           ╚══╝"},d.stone);break;
        case 3:case 10:world_art(p.x-4,p.y-8,{"╭───────────────╮","│ ╭──────────╮  │","│ │    ▰     │  │","│ ╰──────────╯  │","│ ╭───────────────────╮","│ │         ▰         │","│ ╰───────────────────╯","╰───────────────╯"},d.accent);break;
        case 4:case 8:world_art(p.x-4,p.y-7,{"   ╭─────────────╮","   │ ═════  ═══  │","   │ ═════  ═══  │","   │ ═════       │","╭──┴─────────────┴──╮","╰─◉───────────────◉─╯"},d.stone);break;
        case 7:world_art(p.x-5,p.y-5,{"      ╭────╮","      │ ░░ │       ╱╲","╭─────┴────┴──────╱──╲──╮","│ ═══════   ═══════     │","╰──┬────────────────┬───╯","   │                │"},d.light);break;
        case 9:draw_envelope_canopy();break;
        default:world_art(p.x-4,p.y-10,{" ╥                     ╥"," ║  ╭───────────────╮  ║"," ║  │ ▥▥▥▥▥▥▥▥▥▥▥▥▥ │  ║"," ║  ├───────────────┤  ║"," ║  │ ▥▥▥▥▥▥▥▥▥▥▥▥▥ │  ║"," ║  ╰───────────────╯  ║"," ║                     ║"},d.stone);break;
        }
      }
      if(grid_.template_id==79) {
        double opening=(grid_.switch_flags&3)==3?1.:0.;
        const auto &v=state_.adventure;
        if(opening&&v.machinery_left>0&&(v.machinery_changed&2))opening=std::clamp((v.machinery_duration-v.machinery_left)/.7,0.,1.);
        world_art(170,22,{"╔═════════════════════════╗","║       PLAN  /  01       ║","║ ╭─────────────────────╮ ║","║ │                     │ ║","║ │                     │ ║","║ │                     │ ║","║ ╰─────────────────────╯ ║","╚═════════════════════════╝"},d.stone);
        world_art(170-opening*7,24,{"╭──────────╮","│ ░░░░░░░░ │","│ ░░░░░░░░ │","│ ░░░░░░░░ │","╰──────────╯"},d.shadow);
        world_art(176+opening*7,24,{"╭──────────╮","│ ░░░░░░░░ │","│ ░░░░░░░░ │","│ ░░░░░░░░ │","╰──────────╯"},d.shadow);
      }
    }
    const double row_height=aspect_*scale_;
    const MovingPlatform *first_rack=nullptr;
    for(const auto &platform:grid_.platforms) {
      if(platform.archive_trolley||platform.tram)continue;
      if(platform.archive_rack) {
        const double center=platform.base.x+platform.base.w*.5;
        if(first_rack) {
          const double left=first_rack->base.x+first_rack->base.w*.5;
          for(double x=left;x<center;x+=scale_)world_text(x,ArchiveCableY,"═",0x5c768b);
        } else first_rack=&platform;
        continue;
      }
      auto p=platform.at(state_.elapsed);
      world_art(p.x+.5,p.y+.7,{"╭─────────────────────╮","│ ▥▥▥ ▥▥▥▥ ▥▥▥ ▥▥▥▥▥  │","╰──◉───────────────◉──╯"},0x4c6075);
    }
    for(const auto &platform:grid_.platforms)if(platform.archive_rack) {
      const auto rack=platform.at(state_.elapsed);
      const double center=rack.x+rack.w*.5;
      world_art(center-2*scale_,ArchiveCableY-row_height,{"╔═══╗","╠═◉═╣","╚═╤═╝"},0x6b8ba0);
      for(double y=ArchiveCableY+2*row_height;y<rack.y;y+=row_height)
        world_text(center,y,"│",0x60788c);
    }
    for(const auto &belt:grid_.sorting_belts) {
      const double row=aspect_*scale_;
      const double speed=belt.speed_at(grid_.switch_flags);
      const bool forward=speed>0;
      const double loader=forward?belt.x:belt.x+belt.width-5*scale_;
      world_art(loader,belt.y-5*row,
        {"╔════╗","║ ▤▤ ║","╠════╣","╰─┬┬─╯","  ╵╵"},0x78929e);
      draw_belt_tread(belt,0xa0d6df);
      const double travel=belt.width-4*scale_;
      for(int i=0;i<2;++i) {
        const double offset=std::fmod(state_.elapsed*std::abs(speed)+i*travel*.5,travel);
        const double x=belt.x+(forward?offset:travel-offset);
        world_art(x,belt.y-2*row,{"╭──╮","╰≋─╯"},0xc9c9aa);
      }
    }
    // Fine frost drifts behind the walkable surfaces, without hiding hazards.
    for(int i=0;i<9;++i) {
      double x=std::fmod(i*19.7+clock*.65,double(grid_.width()));
      double y=std::fmod(i*7.1+clock*.43,58.);
      world_text(x,y,i%3?"·":"⠂",d.shadow);
    }
  }
