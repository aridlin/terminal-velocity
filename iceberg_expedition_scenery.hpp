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

  void draw_hoist_lamp(double x,double y,bool ready) {
    world_art(x,y,{ready?"●":"○"},ready?0xb5d599:0xb58952);
  }

  void draw_heatworks_relay(double x,double floor,const MovingPlatform &hoist) {
    const double row=aspect_*scale_,top=floor-3.5*row;
    world_art(x,top,{"╭───╮","│   │","╰─┬─╯","  │  "},0xa08770);
    const double lower_dock=hoist.base.y+std::abs(hoist.dy);
    const bool ready=hoist.at(state_.elapsed).y==lower_dock&&hoist.at(state_.elapsed+.08).y==lower_dock;
    draw_hoist_lamp(x+2*scale_,top+row,ready);
  }

  void draw_heatworks_hoist(const MovingPlatform &hoist,const Rect &deck) {
    constexpr uint32_t rail_color=0x6f7776,cable_color=0xa5ada9,deck_color=0xefc37f;
    const double row=aspect_*scale_, top=hoist.base.y-hoist.dy-5.0;
    const int cells=std::max(4,int(deck.w/scale_));
    const double left=deck.x,right=left+(cells-1)*scale_;
    const double center=left+(cells/2)*scale_;
    std::string beam="╔",lip="╞",frame="╚";
    for(int i=1;i<cells-1;++i) {
      beam+="═";
      lip+=i%3==0?"╪":"━";
      frame+=i%2==0?"╲":"╱";
    }
    world_art(left,top,{beam+"╗"},rail_color);
    const double bottom=hoist.base.y+hoist.dy;
    for(double y=top+row;y<=bottom+row*.5;y+=row) {
      world_art(left,y,{"│"},rail_color);
      world_art(right,y,{"│"},rail_color);
    }
    for(double y=top+2*row;y<deck.y-row*.5;y+=row)
      world_art(center,y,{"║"},cable_color);
    const bool docked=hoist.offset(state_.elapsed)==hoist.offset(state_.elapsed+.08);
    draw_hoist_lamp(center,top+row,docked);
    world_text(left,deck.y,lip+"╡",deck_color);
    world_art(left,deck.y+row,{frame+"╝"},0xa08770);
  }

  void draw_pressure_piston(const MovingPlatform &piston,const Rect &deck) {
    const double row=aspect_*scale_,left=deck.x,center=deck.x+deck.w*.5;
    const double lower=piston.base.y+piston.dy,upper=piston.base.y-piston.dy;
    const double head=lower+1.5*row;
    constexpr uint32_t iron=0x786b62,ram=0xc7a481,amber=0xf0b966;
    for(double y=upper;y<=head;y+=row) {
      world_art(left-scale_,y,{"│"},iron);
      world_art(left+deck.w,y,{"│"},iron);
    }
    world_art(center-2*scale_,head,{"╔═╧═╗","║ ▒ ║","╚═╤═╝"},iron);
    for(double y=deck.y+row;y<head;y+=row)world_art(center,y,{"║"},ram);
    const int stop=piston.stop(state_.elapsed);
    draw_hoist_lamp(left-scale_,lower-row,stop==1);
    draw_hoist_lamp(left-scale_,upper-row,stop==-1);
    const int cells=std::max(2,int(deck.w/scale_));
    std::string cap="╞";
    for(int i=1;i<cells-1;++i)cap+=i%2?"━":"╪";
    world_text(left,deck.y,cap+"╡",stop==0?ram:amber);
  }

  void draw_archive_drawers() {
    const double row=aspect_*scale_;
    for(const auto &drawer:grid_.growth) {
      const auto &area=drawer.area;
      const double width=drawer.extended_width(grid_.switch_flags);
      world_art(area.x-2.8,area.y-3*row,
        {"╔══════╤═╗","║ ▥▥▥  │ ║","╟──┬───┤ ║","║ ▱▱▱  ▐ ║","╚══════╧═╝"},0x859bab);
      if(width<=0)continue;
      const int cells=std::max(1,int(width/scale_));
      std::string lip="╞",front="│";
      for(int i=1;i<cells-1;++i){lip+="═";front+=i%6==3?"▥":"─";}
      world_text(area.x,area.y,cells==1?"▰":lip+"╡",0xabc9e4);
      if(cells>1)world_text(area.x,area.y+row,front+"╯",0x6c879f);
    }
  }

  void draw_garden_ice() {
    const double row=aspect_*scale_;
    for(const auto &strip:grid_.ice_strips) {
      std::string rim="╶",facet="╲";
      const int cells=std::max(2,int(strip.w/scale_));
      for(int i=1;i<cells-1;++i) {
        rim+="━";
        facet+=i%7==2?"╱":i%7==3?"╲":i%4==0?"·":"─";
      }
      world_text(strip.x,strip.y,rim+"╴",0xa6e9ea);
      world_text(strip.x,strip.y+row,facet+"╱",0x578fa2);
      const int glint=int(state_.adventure.presentation_time*7+strip.x)%cells;
      world_text(strip.x+glint*scale_,strip.y,"✦",0xe1fbff);
    }
    const auto &player=state_.player;
    if(grid_.on_ice(player)&&std::abs(player.vx)>3.) {
      const double behind=player.vx>0?player.x-1.2:player.rect().right()+.4;
      world_text(behind,player.rect().bottom()-row,int(state_.elapsed*16)%2?"⠂·":"·⠂",0xb4dee6);
    }
  }

  void draw_reservoir_floe(const Rect &floe,double age) {
    const int cells=std::max(3,int(floe.w/scale_));
    const double row=aspect_*scale_,center=floe.x+floe.w*.5;
    constexpr uint32_t rim=0xb1e9e9,fracture=0xf5fdfd,foam=0xb9eef5;
    if(age>=MovingPlatform::CrackTime&&age<MovingPlatform::RestoreTime) {
      const double fall=age-MovingPlatform::CrackTime;
      if(fall<.65) {
        const double spread=std::min(.44*floe.w,.35+fall*4.6);
        for(int side:{-1,1}) {
          world_art(center+side*spread,floe.y,{fall<.38?"≈":"·"},foam);
          if(fall<.36)world_art(center+side*(.6+fall*3.),
            floe.y-row*(1.+8.*fall*(.36-fall)),{side<0?"╲":"╱"},foam);
        }
        for(int i=0;i<5;++i) {
          const double x=floe.x+floe.w*(.16+.17*i)+(i-2)*fall*.54;
          const double y=floe.y+row+fall*fall*10+(i%2)*row*.45;
          world_art(x,y,{i%2?"◇":"▴"},fall<.34?0x9bd8e6:0x6295ac);
        }
      }
      const double remaining=MovingPlatform::RestoreTime-age;
      if(remaining<.4)
        world_art(center-2*scale_,floe.y+row,{remaining<.18?"·◇·◇·":" ·◇· "},0x86c8d7);
      return;
    }

    const bool cracking=age<MovingPlatform::CrackTime;
    std::string lip="╶",ice="╲",underside=" ";
    for(int i=1;i<cells-1;++i) {
      const bool split=cracking&&(i==cells/2||
        (age>.19&&i==cells/4)||(age>.33&&i==cells-1-cells/4));
      lip+=split?"╳":"━";
      ice+=split?(i<cells/2?"╲":"╱"):i%3==0?"▓":"▒";
      underside+=split?" ":i%3==0?"▔":"▁";
    }
    world_text(floe.x,floe.y,lip+"╴",cracking?fracture:rim);
    world_art(floe.x,floe.y+row,{ice+"╱"},cracking?0xa0cedd:0x70abc1);
    world_art(floe.x,floe.y+2*row,{underside},0x4c819a);
  }

  void draw_fragile_planter(const Rect &pot,double age) {
    const double row=aspect_*scale_;
    const int cells=std::max(5,int(pot.w/scale_));
    const double right=pot.x+(cells-1)*scale_;
    const double center=pot.x+(cells/2)*scale_;
    constexpr uint32_t iron=0x638963,clay=0xc48257,crack=0xffcf88;
    world_art(pot.x,pot.y-5*row,{"╤"+std::string(size_t(cells-2),'=')+"╤"},iron);
    for(int i=1;i<5;++i) {
      world_art(pot.x,pot.y-(5-i)*row,{"╎"},iron);
      world_art(right,pot.y-(5-i)*row,{"╎"},iron);
    }
    if(age>=MovingPlatform::CrackTime&&age<MovingPlatform::RestoreTime) {
      const double fall=age-MovingPlatform::CrackTime;
      if(fall<.65) {
        for(int i=0;i<4;++i) {
          const double drift=(i-1.5)*fall;
          world_art(pot.x+pot.w*(.16+i*.23)+drift,pot.y+fall*fall*13+i*.13,
            {i%2?"▴":"·"},clay);
        }
      }
      return;
    }
    const bool cracking=age<MovingPlatform::CrackTime;
    std::string rim="╔",soil="╲",base=" ";
    for(int i=1;i<cells-1;++i) {
      rim+=cracking&&i==cells/2?"╳":"═";
      soil+=cracking&&i==cells/2?"╲":i%3==0?"∙":"░";
      if(i==1)base+="╰";else if(i==cells-2)base+="╯";else base+="─";
    }
    world_art(center-scale_,pot.y-2*row,{"❧"," ╵"},0x8eb979);
    world_text(pot.x,pot.y,rim+"╗",cracking?crack:clay);
    world_art(pot.x,pot.y+row,{soil+"╱",base},clay,true);
  }

  void draw_post_launcher(const SpringPad &launcher) {
    const double row=aspect_*scale_;
    const double floor=launcher.bottom();
    const double center=launcher.x+launcher.w*.5;
    const double duct_y=floor+4.3*row;
    const double tank_x=center-9*scale_;
    constexpr uint32_t pipe=0x77919f,active=0xc8e6b2;
    draw_scenery_pipe({{center,floor},{center,duct_y},{tank_x+5*scale_,duct_y}},pipe);
    world_art(tank_x,duct_y-row,{"╭────╮","│ ◴  ╞","╰─┬──╯","  ╵"},pipe,true);
    const double cycle=std::fmod(state_.adventure.presentation_time*.85+center*.07,1.);
    world_art(tank_x+(5+3*cycle)*scale_,duct_y,{"▤"},active);
    world_text(center-2*scale_,floor-row,"╔═›╗",active);
    const auto &courier=state_.player;
    if(courier.momentum_left>.31&&courier.vy<-18&&
       courier.x>=launcher.x-1.2&&courier.x<launcher.right()+5) {
      world_art(center+scale_,floor-2*row,{"⠠⠐"," ∿ "},active);
    } else if(cycle>.73)world_art(center+2*scale_,floor-2*row,{"›"},pipe);
  }

  void draw_heatworks_turbine(const AirField &fan) {
    const double row=aspect_*scale_,floor=fan.area.bottom(),time=state_.elapsed;
    const double x=fan.area.x-13*scale_,y=floor-9*row;
    const double pressure=fan.strength(time);
    const bool jump_now=fan.strength(time+.25)>=.86;
    constexpr uint32_t casing=0xab8a68,blades=0xc9a583,draft=0xe8ca9e,signal=0xcbe5a5;
    world_art(x,y,{"  ╔═══════╗  "," ╔╝       ╚╗ ","╔╝         ╚╦",
      "║           ╠","╚╗         ╔╩"," ╚╗       ╔╝ ","  ╚═╤═══╤═╝  ",
      "    ║   ║","    ║   ║"},casing);
    static constexpr std::array<std::array<const char *,3>,4> frames{{
      {{"   ▛   ","╺━━●━━╸","   ▟   "}},
      {{" ▚   ▞ ","  ╲●╱  "," ▞   ▚ "}},
      {{"   ▲   "," ▛ ● ▟ ","   ▼   "}},
      {{" ▞   ▚ ","  ╱●╲  "," ▚   ▞ "}}
    }};
    const auto &rotor=frames[size_t(int(time*10.)%frames.size())];
    world_art(x+2*scale_,y+2*row,{rotor[0],rotor[1],rotor[2]},blades);
    world_art(x+10*scale_,y+3*row,{jump_now?"↗":"○"},jump_now?signal:casing);
    if(iceworld::has_turbine_access(grid_)) {
      constexpr uint32_t railing=0xa58a71,access_arrow=0xe6b979;
      for(const auto &step:iceworld::TurbineAccessSteps) {
        const int width=int(step.w/scale_);
        std::string rail="╔";
        for(int cell=2;cell<width;++cell)rail+="═";
        world_art(step.x,step.y-2*row,{rail+"╗",
          "║"+std::string(width-2,' ')+"║"},railing);
        world_art(step.x+scale_,step.y+1.,{"║╲       ╱║","║ ╲     ╱ ║","╚══╧═══╧══╝"},railing);
      }
      prop_art(iceworld::TurbineAccessX,iceworld::TurbineAccessFloor,
        {"╭─────╮","│  ↖  │","╰──┬──╯","   │","  ╱┴╲"},access_arrow);
    }
    const int streaks=2+int(pressure*9.);
    for(int i=0;i<streaks;++i) {
      const double along=std::fmod(time*1.16+i/11.,1.);
      const double wx=fan.area.x+along*fan.area.w;
      const double wy=y+(2.+(i%3)*.84)*row-along*row;
      world_art(wx,wy,{jump_now&&i%3==0?"›":"─"},jump_now?draft:casing);
    }
  }

  void draw_expedition_steam_lift(const AirField &jet) {
    const auto &area=jet.area;
    const double row=aspect_*scale_,bottom=area.bottom(),center=area.x+area.w*.5;
    const double time=state_.elapsed;
    const bool powered=jet.active(grid_.switch_flags);
    const double pressure=powered?jet.strength(time):0.;
    constexpr uint32_t guide=0x4d7881,metal=0x739da3,steam=0xb4e3e3;
    if(jet.enabled_by) {
      const auto supply=std::find_if(grid_.interactions.begin(),grid_.interactions.end(),[&](const auto &node){
        return node.kind==InteractionKind::Valve&&(jet.enabled_by&(uint64_t(1)<<node.id));
      });
      if(supply!=grid_.interactions.end()) {
        const double valve_x=supply->area.x+supply->area.w*.5;
        const double pipe_y=std::max(bottom+5.,supply->area.bottom()+3.);
        draw_scenery_pipe({{valve_x,supply->area.bottom()},{valve_x,pipe_y},{center,pipe_y},{center,bottom}},
          powered?0x789da1:0x576f78);
        constexpr uint32_t idle=0xaba28b,live=0xbce3a5;
        const double gauge_y=bottom+row;
        world_art(center-2*scale_,gauge_y,{"╭─┴─╮","│   │","╰─┬─╯"},metal);
        const auto &view=state_.adventure;
        const bool charging=powered&&view.machinery_left>0&&(view.machinery_changed&jet.enabled_by);
        const double age=view.machinery_duration-view.machinery_left;
        const char *needle=!powered?"╲":charging&&age<.14?"│":"╱";
        world_art(center,gauge_y+row,{needle},powered?live:idle);
        if(charging)for(int i=0;i<16;++i) {
          const double puff=age-i*.018;
          if(puff<0||puff>.41)continue;
          const double x=center+(i%5-2)*(.22+puff*.94);
          world_art(x,bottom-row-17.*puff,{i%4?"⠤":"≈"},puff<.26?steam:metal);
        }
      }
    }

    // The fixed guides show the full width even at low pressure.
    for(double y=area.y+row;y<bottom-row;y+=3*row) {
      world_art(area.x,y,{"╎"},guide);
      world_art(area.right()-scale_,y,{"╎"},guide);
    }
    world_art(center-2*scale_,bottom-row,{"╞═╧═╡"},pressure>.55?steam:metal);
    world_art(center-2*scale_,bottom,{"╚═╤═╝","  ║  "},metal);
    const int wisps=int(pressure*12);
    for(int k=0;k<wisps;++k) {
      const double rise=std::fmod(time*.72+k/12.,1.);
      const double x=center+std::sin(k*2.4+rise*5)*area.w*.24;
      world_art(x,bottom-row-rise*(area.h-row),{pressure>.55&&k%3==0?"▴":"∿"},
        pressure>.55?steam:metal);
    }
  }

// Repeated infrastructure follows the landings in expeditions. Keep the art
// behind the route so decorations never become apparent stepping stones.
  void draw_expedition_infrastructure() {
    if(state_.adventure.mode==iceberg::Mode::Campaign||grid_.district==0||
       grid_.room_role!=iceberg::RoomRole::Traverse||grid_.template_id==50||grid_.template_id==57)return;
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
