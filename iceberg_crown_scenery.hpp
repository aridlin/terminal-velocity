// Shared cell art for exposed ice, shutter counterweights and the morning quay.
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
    for(const auto &platform:grid_.platforms) {auto r=platform.at(state_.elapsed);
      world_art(r.x,r.y+.7,{"╲──────┬──────╱"," ╲─────┴─────╱","      ╱╲"},d.stone);
    }
    for(const auto &field:grid_.air_fields) {
      if(field.disabled_by&&(grid_.switch_flags&field.disabled_by)==field.disabled_by)continue;
      double amount=.5+.5*std::sin((state_.elapsed/field.period+field.phase)*6.283185307);
      for(int i=0;i<10;++i) {
        double drift=clock*(field.ax>0?1:-1)*(4+amount*6);
        double x=field.area.x+std::fmod(i*7.7+drift+10000,field.area.w);
        double y=field.area.y+std::fmod(i*3.1,field.area.h);
        world_text(x,y,amount>.6?(field.ax>0?"╶─›":"‹─╴"):"·",amount>.6?d.stone:d.shadow);
      }
    }
    for(int i=0;i<9;++i) {
      double x=std::fmod(i*29.7+clock*(7+i%3),double(grid_.width()));
      world_text(x,8+i*7+(std::sin(clock+i)*.8),i%3?"╶─":"⠂",d.shadow);
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
