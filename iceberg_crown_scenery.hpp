// Shared cell art for exposed ice, shutter counterweights and the morning quay.
  void draw_crown_cradle(const MovingPlatform &platform,const Rect &deck) {
    const double row=aspect_*scale_,base_center=platform.base.x+platform.base.w*.5;
    const double deck_center=deck.x+deck.w*.5;
    constexpr uint32_t steel=0x72889a,lamp=0xe4ad68;

    if(platform.dx!=0.) {
      const double rail_y=platform.base.y-5*row;
      const double reach=std::abs(platform.dx)+3*scale_;
      draw_scenery_pipe({{base_center-reach,rail_y},{base_center+reach,rail_y}},steel);
      world_art(base_center-reach,rail_y-row,{"▣"},steel);
      world_art(base_center+reach,rail_y-row,{"▣"},steel);
      world_art(deck_center,rail_y,{"◉"},lamp);
      world_art(deck_center,rail_y+row,{"║","║","║","║"},steel);
    } else {
      const double weight_x=platform.base.right()+2*scale_;
      const double top=platform.base.y-std::abs(platform.dy)-4*row;
      const double bottom=platform.base.y+std::abs(platform.dy)+2*row;
      draw_scenery_pipe({{base_center,bottom},{base_center,top},
        {weight_x,top},{weight_x,bottom}},steel);
      world_art(base_center,top,{"◉"},lamp);
      world_art(weight_x,top,{"◉"},lamp);
      // The other end of the cable rises as the deck descends.
      const double weight_y=2*platform.base.y-deck.y;
      world_art(weight_x-scale_,weight_y-row,{"╔═╗","║▓║","╚═╝"},0x9db8c7);
    }
    world_art(deck_center-3*scale_,deck.y+row,{"╲──╥──╱"," ╲─╨─╱"},steel);
  }

  void draw_crown_landmarks() {
    const auto &d=iceberg::Districts[5];double clock=state_.adventure.presentation_time;
    auto mast=[&](double x,double y) {world_art(x,y,{
      "       ╱╲","      ╱  ╲","     ╱ ╳  ╲","    ╱──┬───╲","       ║","      ╱║╲","     ╱ ║ ╲","    ╱──╫──╲","   ╱   ║   ╲","  ╱────╨────╲"},d.shadow);};
    if(grid_.room_role==iceberg::RoomRole::Settlement) {
      mast(35,31);mast(92,26);
      world_art(4,43,{" ╱─────────────────────╲","╱_______________________╲","│ ╭─────╮   ╭────────╮ │","│ │  ░  │   │  ░  ░  │ │","│ ╰─────╯   ╰────────╯ │","│     ╭──────╮         │","╰─────┤      ├─────────╯"},d.stone);
      world_art(116,38,{"╭─────────────────────╮",tr("│  GÓRNA ŚLUZA  →     │","│  UPPER AIRLOCK →    │"),"╰──────────┬──────────╯","           ║"},d.light);
    } else {
      for(size_t i=0;i<grid_.route_points.size();++i) {
        auto [x,y]=grid_.route_points[i];
        if(i%3==0)mast(x-4,y+2);
        if(i%3==1)world_art(x-3,y+2,{"╲           ╱"," ╲         ╱","  ╲───╳───╱","   ╲     ╱","    ╲   ╱","     ╲ ╱"},d.shadow);
        if(i%4==2)world_art(x-4,y-7,{"│          │","│ ╭──────╮ │","╰─┤ ▒▒▒▒ ├─╯","  │ ▒▒▒▒ │","  ╰──────╯"},d.stone);
      }
      if(grid_.template_id==83) {
        world_art(160,10,{"                 ╱╲","              ╱──┴─╲","           ╱──────────╲","        ╭──┬──┬──┬──┬──╮","        │  │  │  │  │  │","        │  │  │  │  │  │","        ╰──┴──┴──┴──┴──╯","         ╲     ║     ╱","          ╲    ║    ╱","           ╲   ║   ╱"},d.stone);
        // Shutters withdraw continuously after their own crank is operated.
        for(int bit=0;bit<3;++bit) {
          bool open=grid_.switch_flags&(uint64_t(1)<<bit);
          double amount=open?1.:0.;
          if(open&&(state_.adventure.machinery_changed&(uint64_t(1)<<bit)))amount=1-state_.adventure.machinery_left/state_.adventure.machinery_duration;
          int slide=int(std::clamp(amount,0.,1.)*5);
          if(amount<.99)world_art(161+bit*2.8,11-slide*aspect_*scale_,{"╔════╗","║▒▒▒▒║","║▒▒▒▒║","╚════╝"},open?d.accent:d.shadow);
        }
        if((grid_.switch_flags&7)==7) {
          double fade=state_.adventure.machinery_left>0?1-state_.adventure.machinery_left/state_.adventure.machinery_duration:1;
          world_text(163,14,"░▒▓◉▓▒░",fade>.45?d.light:d.accent);
          world_text(145,13,"╶─────────────────────────────",d.stone);
          world_text(169,13,"─────────────────────────────╴",d.stone);
          world_text(148,15,"╲   ·   ·   ·   ·   ·",d.shadow);
          world_text(169,15,"·   ·   ·   ·   ·   ╱",d.shadow);
        }
      }
    }
    for(const auto &platform:grid_.platforms) {
      const auto deck=platform.at(state_.elapsed);
      if(grid_.room_role==iceberg::RoomRole::Traverse&&state_.adventure.mode!=iceberg::Mode::Campaign)
        draw_crown_cradle(platform,deck);
      else world_art(deck.x,deck.y+.7,{"╲──────┬──────╱"," ╲─────┴─────╱","      ╱╲"},d.stone);
    }
    draw_crown_wind();
    for(int i=0;i<9;++i) {
      double x=std::fmod(i*29.7+clock*(7+i%3),double(grid_.width()));
      double y=8+i*7+std::sin(clock+i)*.8;
      const Rect flake{x,y,2*scale_,aspect_*scale_};
      if(std::none_of(grid_.air_fields.begin(),grid_.air_fields.end(),[&](const auto &field){return field.area.intersects(flake);}))
        world_art(x,y,{i%3?"╶─":"⠂"},d.shadow);
    }
  }

  void draw_crown_wind() {
    const double elapsed=state_.elapsed,row=aspect_*scale_;
    for(const auto &field:grid_.air_fields) {
      if(field.ax==0)continue;
      const bool active=field.active(grid_.switch_flags);
      const double amount=active?field.strength(elapsed):0.;
      const bool right=field.ax>0;
      for(double edge:{field.area.left(),field.area.right()}) {
        double best=20.;
        std::pair<double,double> landing{};
        for(auto [x,floor]:grid_.route_points) {
          const double distance=std::abs(x-edge);
          if(distance>=best||!grid_.solid_at(int(std::floor(x)),int(std::floor(floor))))continue;
          best=distance;landing={x,floor};
        }
        if(best>=20.)continue;
        const auto [x,floor]=landing;
        const double y=floor-4*row;
        world_art(x,y,{"╥","║","║","║"},0x8395a9);
        world_text(x,floor,"╨",0x8395a9);
        if(amount<.2)world_art(x+scale_,y,{"╲"," ░"," ╵"},0xb6c5d3);
        else if(right)world_art(x+scale_,y,{amount>.65?"█▓▒░╴":"▓▒░╲"},0xd7ecfa);
        else world_art(x-(amount>.65?5:4)*scale_,y,{amount>.65?"╶░▒▓█":"╱░▒▓"},0xd7ecfa);
      }
      if(!active||amount<.1)continue;
      const double width=field.area.w-3*scale_;
      if(width<=0)continue;
      const double direction=right?1.:-1.;
      // Integrate the gust cycle so snow always travels in the displayed direction.
      const double angular=6.283185307179586/std::max(.01,field.period);
      const double distance=field.period>0
        ?8*elapsed-(4/angular)*(std::cos(elapsed*angular+field.phase*6.283185307179586)-std::cos(field.phase*6.283185307179586))
        :12*elapsed;
      const int count=std::clamp(int(field.area.w*.65),12,60);
      for(int i=0;i<count;++i) {
        double offset=std::fmod(i*6.29+direction*distance,width);
        if(offset<0)offset+=width;
        const double y=field.area.y+std::fmod(i*4.17,field.area.h);
        world_art(field.area.x+offset,y,{amount>.55?(right?"╶─›":"‹─╴"):(right?"·╴":"╶·")},
          amount>.55?0x93bbd6:0x526779);
      }
    }
  }
  void draw_epilogue_landmarks() {
    const auto &d=iceberg::Districts[0];
    world_art(3,39,{"       ╱──────────────────────╲","  ╔═══╧════════════════════════╧═══╗","  ║   ░░░░░░   ░░░░░░   ░░░░░░     ║","  ║                                ║","  ║         P O C Z T A            ║","  ║  ╭──────╮  ╔════╗  ╭──────╮    ║","  ║  │ ▤▤▤▤ │  ║    ║  │ ▤▤▤▤ │    ║","  ╚══╧══════╧══╩════╩══╧══════╧════╝"},d.stone);
    world_art(52,42,{"    ╱────────────╲","   ╱______________╲","   │ ░░░░  ░░░░   │","   ├──────────────┤","   │ ░░░░  ░░░░   │","   ╰──────────────╯"},d.stone);
    world_art(104,41,{"╭──────────────────────────╮",tr("│ CZYTELNIA · DRZWI OTWARTE│","│ READING ROOM · OPEN      │"),"├──────────────────────────┤","│ ▤▤▤▤   ▤▤▤▤   ▤▤▤▤       │","│                          │","╰──────────────────────────╯"},d.stone);
    world_art(45,45,(grid_.switch_flags&1)?std::vector<std::string>{"  ·    ·    ·  ","╔═════════════╗","║░░░░░░░░░░░░░║","╚═════════════╝"}:std::vector<std::string>{"               ","╔═════════════╗","║             ║","╚═════════════╝"},d.accent);
    world_art(119,41,{"╭──────────────╮",tr("│ SPIS · WGLĄD │","│ REGISTER     │"),"│ ▤▤▤▤▤▤▤▤▤▤▤  │","╰──────────────╯"},(grid_.switch_flags&2)?d.light:d.stone);
    double clock=state_.adventure.presentation_time;
    double tram=std::fmod(clock*5,270.)-40.;draw_dock_tram(tram,31);
    world_art(0,31+5*aspect_*scale_,{std::string(200,'=')},d.shadow);
    for(int i=0;i<6;++i)world_text(6+i*34,5+i%2,"╲    ╲",d.stone);
  }
