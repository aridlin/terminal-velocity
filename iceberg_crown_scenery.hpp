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

  void draw_crown_rib(const Rect &beam,double age) {
    const double row=aspect_*scale_;
    const int cells=std::max(4,int(beam.w/scale_));
    const double center=beam.x+beam.w*.5;
    constexpr uint32_t frame=0x6c8797,steel=0xa7c1ce,warning=0xffca85;
    const bool cracked=age<MovingPlatform::CrackTime;
    // Tie the brackets back toward the adjacent masonry rather than
    // drawing the rideable surface as an unsupported slab.
    world_art(beam.x-2.4,beam.y+row,{"     ╞═","    ╱","  ╱","╱"},frame);
    if(age>=MovingPlatform::CrackTime&&age<MovingPlatform::RestoreTime) {
      const double fall=age-MovingPlatform::CrackTime;
      if(fall<.7)for(int side:{-1,1}) {
        const double x=center+side*(.8+fall*.45)-scale_;
        world_art(x,beam.y+row+fall*fall*11,
          {side<0?"═╲":"╱═"},fall<.18?warning:frame);
      }
      return;
    }
    std::string lip="╞",web="╚";
    for(int i=1;i<cells-1;++i) {
      lip+=cracked&&i==cells/2?"╳":"═";
      web+=i%2==0?"╳":"─";
    }
    world_text(beam.x,beam.y,lip+"╡",cracked?warning:steel);
    world_art(beam.x,beam.y+row,{web+"╝"},frame);
    if(cracked)world_art(center-scale_,beam.y-row,{"╲╷╱"},warning);
  }

  void draw_crown_service_lift(const MovingPlatform &lift,const Rect &deck) {
    constexpr uint32_t frame=0x587185,cable=0x809bab,steel=0xa6bdc9,amber=0xe4ad68;
    const double row=aspect_*scale_,center=deck.x+deck.w*.5,weight_x=deck.right()+3*scale_;
    const double upper=lift.base.y-std::abs(lift.dy),lower=lift.base.y+std::abs(lift.dy);
    const double head=upper-4*row,foot=lower+1.5*row;
    const double left=lift.base.x-2*scale_,right=weight_x+2*scale_;

    draw_scenery_pipe({{left,foot},{left,head},{right,head},{right,foot}},frame);
    world_art(left-2*scale_,foot,{"╞═╧═══════════════╧══╡"," ▓▓                ▓▓ "},frame);
    // The outside rail is braced back to the masonry ledge.
    for(int brace=0;brace<7;++brace)
      world_art(right-(2*brace+1)*scale_,foot+(brace+1)*row,{"╱"},frame);
    for(double y=head+3*row;y<foot-2*row;y+=4*row) {
      world_art(left,y,{"╟─","║╲"},frame);
      world_art(right-scale_,y,{"─╢","╱║"},frame);
    }
    const double weight_y=2*lift.base.y-deck.y;
    draw_scenery_pipe({{center,deck.y},{center,head},{weight_x,head},{weight_x,weight_y}},cable);
    world_art(center-2*scale_,head-row,{"╭───╮","│ ◉ │","╰─┬─╯"},steel);
    world_art(weight_x-scale_,head,{"◉"},steel);
    world_art(weight_x-scale_,weight_y,{"┌─┐","│▓│","└─┘"},cable);
    world_art(center-3*scale_,deck.y+row,{"╲──┬──╱"," ╰─┴─╯"},steel);

    const double change=lift.at(state_.elapsed+.08).y-lift.at(state_.elapsed-.08).y;
    const auto signal=std::abs(change)<.14?"●":change<0?"↑":"↓";
    world_art(left-5*scale_,lower-3*row,{"┌─┐","│ │","└┬┘"," ╞═══╡"},steel);
    world_art(left-4*scale_,lower-2*row,{signal},amber);
    world_art(right,upper-3*row,{"╟──┌─┐","║  │ │","╟──└─┘"},steel);
    world_art(right+4*scale_,upper-2*row,{signal},amber);
  }

  void draw_crown_seam(const Rect &seam,double time) {
    const double row=aspect_*scale_;
    const double center=seam.x+seam.w*.5;
    world_art(seam.x,seam.y-row,{"╶─────╴"},0xa1bdcc);
    for(double y=seam.top()+row;y<seam.bottom();y+=2*row) {
      world_art(seam.left(),y,{"╟"},0x829fb4);
      world_art(seam.right()-scale_,y,{"╢"},0x829fb4);
    }
    // A narrow band of light and drifting ice show the channel's continuity.
    for(int i=0;i<9;++i) {
      const double y=seam.top()+std::fmod(i*2.7+time*2.1,seam.h);
      world_art(center+(i%3-1)*scale_,y,{i%3==1?"│":"·"},i%3==1?0x6e93af:0x577287);
    }
    world_art(seam.left(),seam.bottom(),{"◉","╨"},0xc4deeb);
    world_art(seam.right()-scale_,seam.bottom(),{"◉","╨"},0xc4deeb);
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
      if(platform.fragile)continue;
      const auto deck=platform.at(state_.elapsed);
      if(platform.service_lift)draw_crown_service_lift(platform,deck);
      else if(grid_.room_role==iceberg::RoomRole::Traverse&&state_.adventure.mode!=iceberg::Mode::Campaign)
        draw_crown_cradle(platform,deck);
      else world_art(deck.x,deck.y+.7,{"╲──────┬──────╱"," ╲─────┴─────╱","      ╱╲"},d.stone);
    }
    draw_crown_wind();
    if(grid_.crown_seam)draw_crown_seam(*grid_.crown_seam,clock);
    if(grid_.windbreak_roof) {
      const auto &roof=*grid_.windbreak_roof;
      for(double x=roof.left();x+scale_<roof.right();x+=scale_)
        world_text(x,roof.top(),"━",0xa5bdcf);
      for(double x=roof.left()+2.;x+6*scale_<roof.right();x+=7.)
        world_art(x,roof.bottom(),{"┬────┬"," ╲──╱ "},0x7890a3);
    }
    for(int i=0;i<9;++i) {
      double x=std::fmod(i*29.7+clock*(7+i%3),double(grid_.width()));
      double y=8+i*7+std::sin(clock+i)*.8;
      const Rect flake{x,y,2*scale_,aspect_*scale_};
      if(std::none_of(grid_.air_fields.begin(),grid_.air_fields.end(),[&](const auto &field){return field.area.intersects(flake);}))
        world_art(x,y,{i%3?"╶─":"⠂"},d.shadow);
    }
  }

  void draw_ridge_sensors(double amount,bool right) {
    const double row=aspect_*scale_;
    for(const auto &pad:iceworld::pattern(grid_.template_id).optional) {
      const double x=pad.x+pad.w-6*scale_,y=pad.y-5*row;
      world_art(x,y,{"  ╥  ","┌─╨─┐","│   │","└─┬─┘","  ║  "},0x8298ac);
      world_text(x+2*scale_,pad.y,"╨",0x8298ac);
      if(amount<.2)world_text(x+scale_,y+2*row,"·○·",0xa2b9c4);
      else world_text(x+scale_,y+2*row,right?"─▶·":"·◀─",right?0xf4d985:0xabdaee);
      if(amount>=.2) {
        const double tip=x+(right?3.5:.5)*scale_;
        world_text(tip,y-row,amount>.65?(right?"╶╴":"╴╶"):"·",0xb3c4d2);
      }
    }
  }

  void draw_crown_wind() {
    const double elapsed=state_.elapsed,row=aspect_*scale_;
    for(const auto &field:grid_.air_fields) {
      if(field.ax==0)continue;
      const bool active=field.active(grid_.switch_flags);
      const double force=active?field.strength(elapsed):0.;
      const double amount=std::abs(force);
      const bool right=field.ax*force>0;
      if(!field.reverses)for(double edge:{field.area.left(),field.area.right()}) {
        double best=20.;
        std::pair<double,double> landing{};
        for(auto [x,floor]:grid_.route_points) {
          const double distance=std::abs(x-edge);
          if(distance>=best||!grid_.solid_at(int(std::floor(x)),int(std::floor(floor))))continue;
          best=distance;landing={x,floor};
        }
        if(!grid_.windbreak_roof&&best>=20.)continue;
        const auto [x,floor]=grid_.windbreak_roof?
          std::pair{edge,grid_.windbreak_roof->top()}:landing;
        const double y=floor-4*row;
        world_art(x,y,{"╥","║","║","║"},0x8395a9);
        world_text(x,floor,"╨",0x8395a9);
        if(amount<.2)world_art(x+scale_,y,{"╲"," ░"," ╵"},0xb6c5d3);
        else if(right)world_art(x+scale_,y,{amount>.65?"█▓▒░╴":"▓▒░╲"},0xd7ecfa);
        else world_art(x-(amount>.65?5:4)*scale_,y,{amount>.65?"╶░▒▓█":"╱░▒▓"},0xd7ecfa);
      }
      const double width=field.area.w-3*scale_;
      const int count=active&&amount>=.1&&width>0
        ?std::clamp(int(field.area.w*.65),12,60):0;
      const double direction=field.ax>0?1.:-1.;
      // Integrate the gust cycle so snow always travels in the displayed direction.
      const double angular=6.283185307179586/std::max(.01,field.period);
      const double drift=field.period>0
        ?(4/angular)*(std::cos(field.phase*6.283185307179586)-std::cos(elapsed*angular+field.phase*6.283185307179586))
        :0.;
      const double distance=field.reverses?2*drift:field.period>0?8*elapsed+drift:12*elapsed;
      for(int i=0;i<count;++i) {
        double offset=std::fmod(i*6.29+direction*distance,width);
        if(offset<0)offset+=width;
        const double y=field.area.y+std::fmod(i*4.17,field.area.h);
        world_art(field.area.x+offset,y,{amount>.55?(right?"╶─›":"‹─╴"):(right?"·╴":"╶·")},
          amount>.55?0x93bbd6:0x526779);
      }
      if(field.reverses)draw_ridge_sensors(amount,right);
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
