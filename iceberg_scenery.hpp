// RenderGrid members: authored silhouettes, positioned in world space and
// painted behind gameplay objects. A room's landmark is not a collision mask.
  void draw_quay_hoist() {
    const auto &d=iceberg::Districts[0];
    const double row=aspect_*scale_;
    for(double x=81;x<=106;x+=scale_)
      world_art(x,25,{"═","╳","═"},0x748a8c);
    for(double y=25;y<48;y+=row)
      world_art(105,y,{"╫╳╫"},0x72878b);
    for(const auto &sling:grid_.platforms) {
      const Rect deck=sling.at(state_.elapsed);
      double center=deck.x+deck.w*.5;
      world_art(center-2*scale_,25-row,{"╭───╮","│ ◉ │","╰─┬─╯"},d.accent);
      for(double y=25+row;y<deck.y-3*row;y+=row)
        world_art(center,y,{"│"},0x889a95);
      world_art(center-3*scale_,deck.y-3*row,{"   ∧","  ╱ ╲"," ╱   ╲"},0x9daaa3);
      int width=std::max(4,int(deck.w/scale_));
      std::string edge="╲";
      for(int i=2;i<width;++i)edge+=i%4==0?"┴":"─";
      world_art(deck.x,deck.y+row,{edge+"╱"},d.accent);
      world_art(center-5*scale_,deck.y+2*row,{"┏━━━━┳━━━━┓","┃╲   ┃   ╱┃","┃ ╲  ┃  ╱ ┃","┗━━╧━┻━╧━━┛"},0xbd9364);
    }
  }
  bool quay_ferry(size_t platform) const {
    return grid_.template_id==504||(grid_.template_id==601&&platform==0);
  }
  void draw_ferry_gunwale(const Rect &deck,int first_col,int last_col) {
    const auto [left,y]=screen(deck.x,deck.y-.001);
    if(y<4||y>=rows_-3)return;
    const int right=screen(deck.right(),deck.y).first;
    for(int x=std::max({0,left,first_col});x<std::min({cols_,right,last_col});++x) {
      const int wx=int(std::floor(deck.x+(x-left)*scale_));
      const int wy=int(std::floor(deck.y-.001));
      if(grid_.solid_at(wx,wy)||grid_.hazard_at(wx,wy))continue;
      put(y,x,{"▀",Color::Decor,false,false,0,0xb18b61,0x6d513f});
    }
  }
  void draw_quay_ferry(size_t platform,const Rect &deck) {
    const double row=aspect_*scale_;
    const auto [left,deck_row]=screen(deck.x,deck.y-.001);
    const int width=screen(deck.right(),deck.y).first-left;
    if(width<4)return;
    world_art(deck.x+deck.w*.38,deck.y-6*row,
      {"     ▆▆","     ▓▓","  ▟██████▙","  ▌▥ ▥ ▥ ▐","╔═╧══════╧══╗","║ ○       ○ ║"},0x82999a);

    const Rect &berth=grid_.platforms[platform].base;
    const double center=berth.x+berth.w*.5;
    const auto basin=std::find_if(grid_.water_basins.begin(),grid_.water_basins.end(),
      [center](const Rect &water){return center>=water.x&&center<water.right();});
    if(basin==grid_.water_basins.end())return;
    const int surface=screen(deck.x,basin->y+.5*row).second;
    draw_ferry_gunwale(deck,0,cols_);
    for(int y=deck_row+1;y<=surface+1;++y) {
      const int inset=std::min((width-4)/2,y-deck_row-1);
      const int inner=width-2*inset-2;
      const bool submerged=y>=surface;
      const bool keel=y==surface+1;
      std::string body=keel?"╰":"╲";
      for(int i=0;i<inner;++i)
        body+=keel?"━":y==deck_row+1&&i%7==3?"○":"▒";
      body+=keel?"╯":"╱";
      const double wx=state_.camera_x+(left+inset+.5)*scale_;
      const double wy=state_.camera_y+(y-4+.5)*row;
      world_art(wx,wy,{body},submerged?0x477080:0xa58460);
    }
  }
  void draw_ferry_foreground() {
    const int platform=state_.player.support;
    if(platform<0||size_t(platform)>=grid_.platforms.size()||!quay_ferry(size_t(platform)))return;
    const Rect deck=grid_.platforms[size_t(platform)].at(state_.elapsed);
    const int rider=screen(state_.player.x,deck.y).first;
    draw_ferry_gunwale(deck,rider,rider+4);
  }
  void draw_scenery_pipe(std::initializer_list<std::pair<double,double>> path,uint32_t color) {
    auto paint=[&](int x,int y,const char *glyph) {
      if(x<0||x>=cols_||y<4||y>=rows_-3)return;
      const double wx=state_.camera_x+x*scale_, row=aspect_*scale_;
      const double wy=state_.camera_y+(y-4)*row;
      if(grid_.solid_at(int(std::floor(wx+.5*scale_)),int(std::floor(wy+.5*row)))||
         grid_.hazard_at(int(std::floor(wx)),int(std::floor(wy)))||
         state_.player.rect().intersects({wx,wy,scale_,row}))return;
      put(y,x,{glyph,Color::Decor,false,false,0,color});
    };
    // Each turn is snapped once so both runs share the same corner cell.
    for(auto a=path.begin(),b=a+1;b!=path.end();++a,++b) {
      auto [ax,ay]=screen(a->first,a->second);
      auto [bx,by]=screen(b->first,b->second);
      const bool horizontal=a->second==b->second;
      if(horizontal&&ay>=4&&ay<rows_-3)
        for(int x=std::max(0,std::min(ax,bx)+1);x<=std::min(cols_-1,std::max(ax,bx)-1);++x)
          paint(x,ay,std::abs(x-ax)%13==0?"╪":"═");
      if(!horizontal&&ax>=0&&ax<cols_)
        for(int y=std::max(4,std::min(ay,by)+1);y<=std::min(rows_-4,std::max(ay,by)-1);++y)
          paint(ax,y,std::abs(y-ay)%6==0?"╫":"║");
      if(a==path.begin())paint(ax,ay,horizontal?"╪":"╫");
      if(b+1==path.end()){paint(bx,by,horizontal?"╪":"╫");continue;}
      const auto &next=*(b+1);
      const bool right=horizontal?a->first>b->first:next.first>b->first;
      const bool down=horizontal?next.second>b->second:a->second>b->second;
      paint(bx,by,down?(right?"╔":"╗"):(right?"╚":"╝"));
    }
  }

  void draw_pipe_drip(double x,double lip,double floor,double offset) {
    double row=aspect_*scale_;
    world_art(x,lip,{"╨"},0xa1a8a1);
    double phase=std::fmod(state_.adventure.presentation_time+offset,2.35);
    if(phase<.6)world_art(x,lip+row,{"·"},0x8baab6);
    else if(phase<1.45) {
      double fall=(phase-.6)/.85;
      world_art(x,lip+row+(floor-lip-2*row)*fall*fall,{"╵"},0xa0c4d0);
    } else if(phase<1.8)
      world_art(x-2*scale_,floor-row,{phase<1.60?"  ˇ":"· ∙ ·"},0x8baab6);
  }
  void draw_sorting_belts() {
    for(const auto &belt:grid_.sorting_belts) {
      const double row=aspect_*scale_;
      const int count=std::max(3,int(belt.width/scale_));
      world_art(belt.speed>0?belt.x:belt.x+belt.width-3,belt.y-6*row,
        {"▌     ▐","╲     ╱"," ╲   ╱","  ║ ║","  ╚═╝"},0x7e9297);
      const int travel=int(std::floor(state_.elapsed*belt.speed/scale_));
      std::string tread="◉";
      for(int i=2;i<count;++i)tread+=(i-travel)%5==0?(belt.speed>0?"›":"‹"):"─";
      world_text(belt.x,belt.y+row,tread+"◉",0xab946f);
      double pos=std::fmod(state_.elapsed*std::abs(belt.speed),belt.width-2);
      if(belt.speed<0)pos=belt.width-2-pos;
      world_art(belt.x+pos,belt.y-3*row,{"┏━┯━┓","┃╲│╱┃","┗━┷━┛"},0xc6a078);
    }
  }
  void draw_dock_worker(double x,double feet,double phase) {
    const double row=aspect_*scale_;
    const double top=feet-4*row;
    world_art(x,top,{" ▄▄▄▄"},0x849ca2);
    world_art(x,top+row,{"  ██ "},0xb98b64);
    world_art(x,top+2*row,{" ▐██▌"," ▐▌▐▌"},0x7d91a5);
    world_art(x+4*scale_,top+2*row,{phase<.45?"▄":"╶"},0xb98b64);
    world_art(x+5*scale_,feet-2*row,{"┌─┬─┐","└─┴─┘"},0x897253);
  }

  void draw_harbor_landmarks() {
    const auto &d=iceberg::Districts[0];const int id=grid_.template_id;
    const double time=state_.adventure.presentation_time;
    // Masonry courses and steel uprights are drawn into interior solid cells,
    // with the same stone backing. They cannot look like holes in the route.
    for(int sy=4;sy<rows_-3;++sy)for(int sx=0;sx<cols_;++sx) {
      double wx=state_.camera_x+sx*scale_,wy=state_.camera_y+(sy-4)*aspect_*scale_;
      int x=int(std::floor(wx)),y=int(std::floor(wy));
      if(x<2||x>=grid_.width()-2||y<2||y>=grid_.height()-2)continue;
      bool inside=true;
      for(int dy=-1;dy<=1;++dy)for(int dx=-1;dx<=1;++dx)inside=inside&&grid_.solid_at(x+dx,y+dy);
      if(!inside)continue;
      bool course=y%8==0,upright=x%18==0;
      if(course||upright)put(sy,sx,{course?(upright?"┼":"─"):"│",Color::Decor,false,false,0,0x25424d,d.stone});
    }
    // Windows occupy the interiors of real collision buildings. Keep every
    // outside edge intact, so an occupied facade never hides a landing.
    for(int y=7;y<grid_.height()-3;y+=5)for(int x=6;x<grid_.width()-5;x+=8) {
      bool interior=true;
      for(int dy=-2;dy<=2;++dy)for(int dx=-2;dx<=2;++dx)
        interior=interior&&grid_.solid_at(x+dx,y+dy);
      if(interior)world_text(x,y,((x/8+y/5)%4==0)?"▥▥":"┌─┐",((x/8+y/5)%4==0)?0x886c48:0x29434d);
    }
    if(id==600) {
      world_art(3,48-9*aspect_*scale_,{"      ╱──────────────────╲"," ╔═══╧════════════════════╧═══╗",tr(" ║       P O C Z T A          ║"," ║         P O S T            ║")," ╟────────────────────────────╢"," ║ ▤▤  ▤▤  ▤▤   ╭─────╮  ▤▤   ║"," ║              │ ░░░ │       ║"," ║  ▱▱▱         │ ░░░ │       ║"," ╚══════════════╧═════╧═══════╝"},d.stone);
      // A letter feeds under the descending stamp inside the postal window.
      double feed=std::fmod(time,3.);
      double row=aspect_*scale_;
      world_art(4,48-5*row,{"╔══════╗","║      ║","║      ║","╠══════╩══════╗","╨             ╨"},0x947959);
      world_art(4+5*scale_,48-(feed>1.5&&feed<2.?3:4)*row,{"▐█▌"},0xb39878);
      world_art(4+std::min(feed,2.5)*3*scale_,48-3*row,{"✉"},d.light);
      world_art(40,39-7*aspect_*scale_,{"    ╱────────────╲","╭──╧──────────────╧──╮","│  ▣   ▣   ▣   ▣     │","├────────────────────┤","│  ╭──╮    ╭──╮      │","│  │  │    │  │      │","╰──┴──┴────┴──┴──────╯"},d.stone);
      world_art(59,43-5*aspect_*scale_,{"╭───────────────────╮",tr("│ KAWA • NAPRAWY    │","│ COFFEE • REPAIRS  │"),"├───────────────────┤","│ ▤▤ ▤▤   ╭────╮    │","╰─────────┴────┴────╯"},0x5f777b,true);
      draw_scenery_pipe({{57,40.5},{57,34},{76,34},{76,43.5}},0x596d73);
      draw_scenery_pipe({{76,35.6},{81,35.6},{81,44}},0x52686f);
      draw_quay_hoist();
      world_art(106,48-6*aspect_*scale_,{" ╱▔▔╲╱▔▔╲╱▔▔╲╱▔▔╲","│                 │","│  ♧  ▰  ♧  ▰    │","├─────────────────┤","│                 │","╰───┬─────────┬───╯"},d.stone);
      for(int x=83;x<123;x+=12)world_art(x,58,{"║","║","╨"},d.stone);
      world_art(112,48-8*aspect_*scale_,{"╭─────╮","│  ◷  │","╰──┬──╯","   │","   │"},d.accent);
      return;
    }
    if(id==601) {
      world_art(8,55-8*aspect_*scale_,{"╭─────────────────────────╮",tr("│ SZKOŁA NA NABRZEŻU      │","│ LESSONS ON THE QUAY     │"),"├─────────────────────────┤","│ ▤▤ ▤▤ ▤▤  ╭─────╮  ▤▤   │","│           │     │       │","│           │  ○  │       │","╰───────────┴─────┴───────╯"},d.stone);
      world_art(70,49-5*aspect_*scale_,{"╭──────────────────╮","│ ▥▥▥  ▥▥▥  ▥▥▥    │","├──────────────────┤","│  ▱▱▱  ▱▱▱  ▱▱▱   │","╰──────────────────╯"},d.stone);
      world_art(137,43-7*aspect_*scale_,{"        ╔══════════════╗","        ║              │","        ║              │","        ║             ╱╲","        ║            ▐▣▣▌","        ║","════════╩═══════════════"},d.stone);
      world_art(171,40-5*aspect_*scale_,{"╭───────────────────╮",tr("│ DO DOKÓW       →  │","│ TO THE DOCKS   →  │"),"╰──┬─────────────┬──╯","   │             │","═══╧═════════════╧═══"},d.stone);
      return;
    }
    if(iceworld::harbor_v4_room(id)) {
      const auto &p=iceworld::pattern(id);
      auto entrance=p.path.front();
      if(id!=503&&id!=504&&id!=505&&id!=509)world_art(entrance.x+3,entrance.y-6*aspect_*scale_,
        {"╭─────────────╮","│ ▣▣   ╭───╮  │","│      │   │  │","│  ◴   │ ○ │  │","│      │   │  │","╰──────┴───┴──╯"},d.stone);
      for(size_t i=0;i<p.path.size();++i) {
        auto a=p.path[i];
        if(a.kind!='p')continue;
        if(i+1<p.path.size()) {
          auto b=p.path[i+1];
          if(b.x>a.x+a.w) {
            double wire_y=std::min(a.y,b.y)-7.;
            for(double x=a.x+a.w-2;x<b.x+2;x+=scale_)world_art(x,wire_y,{"─"},0x29434d);
            world_art(a.x+a.w-2,wire_y,{"┬","│","│"},0x29434d);
          }
        }
        if(i%3==0) {
          world_art(a.x+a.w-7,a.y-5*aspect_*scale_,{"╭──╮","│▥▥│","╰┬─╯"," │ "," │ "},d.stone);
        }
      }
      const auto &middle=p.path[p.path.size()/2];
      if(id==500) {
        world_art(36,49-7*aspect_*scale_,{"╭──────────────────╮",tr("│ P O C Z T A      │","│ P O S T          │"),"├──────────────────┤","│ ▤▤  ▤▤  ▤▤  ▤▤   │","│                  │","│     ╭────╮       │","╰─────┴────┴───────╯"},d.stone);
      } else if(id==501) {
        world_art(middle.x,middle.y-8*aspect_*scale_,{"       ╭──╮","       │  │","╭──────┴──┴──────────╮",tr("│ MLEKO   06:00      │","│ MILK    06:00      │"),"│  ▣   ▣   ▣   ▣     │","│       ╭────╮       │","╰───────┴────┴───────╯"},d.stone);
        world_art(middle.x+8,middle.y-10*aspect_*scale_,{int(time*2)%2?" ∿":"∿ "},0x49616b);
      } else if(id==502) {
        world_art(52,10,{"                ╔══════════════════╗","               ╱║                  │","              ╱ ║                  │","             ╱  ║                 ╱╲","            ╱   ║                ▐▣▣▌","           ╱    ║","          ╱     ║","         ╱      ║","        ╱       ║","       ╱        ║","      ╱         ║"},d.stone);
      } else if(id==503) {
        world_art(88,43-6*aspect_*scale_,{" ╭──────────────────╮",tr(" │ SORTOWNIA  02    │"," │ SORTING    02    │")," │ ▯ ▯ ▯ ▯ ▯ ▯ ▯ ▯  │"," ╰───╥────────╥─────╯","     ║        ║","═════╩════════╩══════"},d.stone);
        draw_sorting_belts();
      } else if(id==504) {
        world_art(126,41-6*aspect_*scale_,{"    ╱╲","   ╱  ╲","  ╱    ╲"," ╱______╲","     │","─────┴──────"},d.stone);
      } else if(id==505) {
        draw_scenery_pipe({{39,32},{39,35.2},{44,35.2},{44,37.3}},0x879396);
        draw_pipe_drip(44,37.3,41,0);
        draw_scenery_pipe({{72,36.5},{72,39.2},{78,39.2},{78,41.4}},0x879396);
        draw_pipe_drip(78,41.4,45,1.1);
        draw_scenery_pipe({{72,26},{91,26},{91,28.4},{102,28.4},{102,34}},0x788d92);
        draw_pipe_drip(102,34,48,.3);
        draw_scenery_pipe({{69,24},{69,27.6},{126,27.6},{126,32.8},{163,32.8},{163,40.8}},0x49656d);
        draw_scenery_pipe({{164,41},{164,35.4},{202,35.4},{202,38}},0x49656d);
        world_art(98,48-5*aspect_*scale_,{"╭─────────────╮","│  ◴     ◷    │","╰──┬─────┬────╯","   ║     ║","═══╩═════╩════"},d.stone);
      } else if(id==506) {
        for(size_t i=1;i<p.path.size();i+=2) {
          auto a=p.path[i];world_art(a.x+5,a.y-7*aspect_*scale_,{"╭───╮","│ ▣ │","╰─┬─╯","  │","  │","  │","══╧══"},0xa18d67);
        }
      } else if(id==507) {
        draw_scenery_pipe({{54,48.5},{54,36},{80,36},{80,33.6},{104,33.6},{104,42.5}},0x61757a);
        draw_scenery_pipe({{112,42.6},{112,34.4},{139,34.4},{139,39.5}},0x516a71);
        world_art(74,45-6*aspect_*scale_,{" ╱▔▔╲╱▔▔╲╱▔▔╲╱▔▔╲","│                 │","│  ♧  ▰  ♧  ▰    │","├─────────────────┤","│  ╲    ╱ ╲    ╱  │","╰───╲──╱───╲──╱───╯"},d.stone);
      } else if(id==508) {
        world_art(90,47-8*aspect_*scale_,{"                ┃","          ╭─────┼─────╮","          │ ▣▣  ┃  ▣▣ │","     ╭────┴─────┻─────┴────╮","     │  ○  ○  ○  ○  ○  ○   │","  ╲══╧═════════════════════╧══╱","   ╲_________________________╱","      ║                 ║"},d.stone);
      } else if(id==509) {
        for(auto lift:grid_.platforms) {
          auto at=lift.at(state_.elapsed);
          for(double y=12;y<at.y-1;y+=aspect_*scale_)world_art(at.x+at.w*.5,y,{int(y*2+time*4)%2?"╫":"║"},d.stone);
          world_art(at.x-1,at.y-4*aspect_*scale_,{"╭──────────────────╮","│                  │","│  ▱▱        ▱▱    │","╰──────────────────╯"},d.stone);
        }
      } else if(id==510) {
        world_art(93,47-11*aspect_*scale_,{"         ╱╲","      ╭─╱──╲─╮","      │  ◴   │","      ╰──┬───╯","         ║","╭────────╨──────────╮","│ ▣  ▣  ▣  ▣  ▣  ▣   │","├───────────────────┤","│  ╭───╮     ╭───╮  │","│  │   │     │   │  │","╰──┴───┴─────┴───┴──╯"},d.stone);
      } else if(id==511) {
        world_art(105,42-7*aspect_*scale_,{"╭────────────────────╮",tr("│ PRALNIA            │","│ LAUNDRY            │"),"├────────────────────┤","│  ◉   ◉   ◉   ◉     │","│                    │","│       ╭────╮       │","╰───────┴────┴───────╯"},d.stone);
        world_art(46,28,{"┬─────┬─────┬─────┬─────┬","│ ▒▒▒ │ ▒▒▒ │ ▒▒▒ │ ▒▒▒ │",int(time)%2?"  ▒▒     ▒▒     ▒▒    ▒▒":"   ▒▒     ▒▒    ▒▒   ▒▒"},0x647b7b);
      }
      if(iceworld::harbor_mail_item(id)>=0) {
        world_art(grid_.goal.x-4,grid_.goal.bottom()-5*aspect_*scale_,
          {"╭─────╮","│ ━━━ │","│  ✉  │","╰──┬──╯","   │"},d.accent);
      }
      return;
    }
    if(id==310) {
      world_art(7,22-7*aspect_*scale_,
        {"╭────────────────────╮","│    TRAM 02         │","├────────────────────┤","│  ◴   ◷   ╭────╮    │","│  ╚═══╝   │    │    │","│          │ ○  │    │","╰──────────┴────┴────╯"},d.stone);
      for(size_t i=1;i<=5;++i) {
        auto [ax,ay]=grid_.route_points[i-1];auto [bx,by]=grid_.route_points[i];
        int steps=std::max(1,int(std::ceil(std::abs(bx-ax)/scale_)));
        for(int j=0;j<=steps;++j) {
          double fraction=double(j)/steps;
          world_art(ax+(bx-ax)*fraction,ay+(by-ay)*fraction-6,{"═"},0x2a4652);
        }
        for(double y=by-6;y<by-1;y+=aspect_*scale_)world_art(bx,y,{"│"},0x2a4652);
      }
      for(auto [x,y,flag]:{std::tuple{57.,30.,uint64_t(1)},std::tuple{107.,38.,uint64_t(4)}}) {
        const bool live=grid_.switch_flags&flag;
        world_text(x+2,y-.3,"╞════════════╡",live?0x9ed49d:d.accent);
        world_art(x+5,y-5*aspect_*scale_,{"╭──╮",live?"│● │":"│○ │","╰┬─╯"," │"," │"},live?0x9ed49d:d.accent);
      }
      world_art(113,47-4*aspect_*scale_,{"╭─╮","│A│","╰┬╯"," │"},d.accent);
      world_art(64,55-4*aspect_*scale_,{"╭─╮","│B│","╰┬╯"," │"},d.accent);
      world_art(12,48-6*aspect_*scale_,{"╭────────────────╮",tr("│ PRZYSTANEK     │","│ TRAM STOP      │"),"├────────────────┤","│                │","│                │","╰────────────────╯"},d.stone);
      double tx=143,ty=42;
      // Project onto the real lower rail continuously. Passing a lamp changes
      // mission state, never the tram's location; jumping does not make it fly.
      if(grid_.switch_flags&8) {
        tx=std::clamp(state_.player.x+3.,22.,143.);
        static const std::array<std::pair<double,double>,6> rail{{{22,48},{46,52},{71,55},{96,51},{121,47},{143,42}}};
        for(size_t i=1;i<rail.size();++i)if(tx<=rail[i].first) {
          auto [ax,ay]=rail[i-1];auto [bx,by]=rail[i];
          ty=ay+(by-ay)*(tx-ax)/(bx-ax);break;
        }
      }
      world_art(tx,ty-4*aspect_*scale_,{"╭──────────╮","│ ▯▯ ▄ ▯▯  │","│    ▤     │","╰─◉────◉──╯"},d.accent);
    } else if(id==311) {
      world_art(86,44-6*aspect_*scale_,{"╔════════════╗","║  16° / 8°  ║","╚══╤════╤════╝","   ║    ║","═══╩════╩══════"},d.stone);
      world_art(185,32-7*aspect_*scale_,{"   ╔══════════════╗",tr("   ║ ŚLUZA POCZTY ║","   ║ POST AIRLOCK ║"),"═══╣              ╠═══","   ║   ╭──────╮   ║","   ║   │ ░░░░ │   ║","   ║   │ ░░░░ │   ║","   ╚═══╧══════╧═══╝"},d.stone);
    } else if(id==312) {
      draw_scenery_pipe({{10,53},{10,42.4},{67,42.4},{67,45.6}},0x526971);
      draw_scenery_pipe({{74,46.5},{74,36.4},{138,36.4},{138,39.5}},(grid_.switch_flags&4)?0x86ab9c:0x526971);
      draw_scenery_pipe({{143,39.2},{143,33.4},{171,33.4},{171,35.6}},(grid_.switch_flags&8)?0xc0956e:0x526971);
      world_art(7,55-7*aspect_*scale_,{"╔════════════╗","║ 16 →   8 ← ║","╚══╤════╤════╝","   ║    ║","═══╩════╩══════"},d.accent);
      world_art(68,47-7*aspect_*scale_,{"╔══════════════╗","║   ╭────╮     ║","║   │ ◉  │     ║","║   ╰────╯     ║","╚═══╤════╤═════╝","    ║    ║","════╩════╩══════"},d.stone);
      world_art(136,39-6*aspect_*scale_,{"   ╱╲"," ╱────╲"," │ ▣▣ │"," ╰─┬──╯","   ║","═══╩════"},(grid_.switch_flags&4)?0xb8dfb7:d.accent);
      world_art(169,35-5*aspect_*scale_,{"╭──────╮","│ ∿∿∿∿ │","╰──┬───╯","   ║","═══╩════"},(grid_.switch_flags&8)?0xe99462:d.stone);
    }
  }
  void draw_room_identity() {
    int id=grid_.template_id;
    if(id>=400){draw_revision_landmark();return;}
    static const std::vector<std::vector<std::string>> landmarks{
      {"      ╭───────╮", "──────┤ POCZTA├──────", "      ╰───┬───╯", "   ╭──────┴──────╮", "   │ ▣   ▣   ▣   │", "   ╰──────┬──────╯", "          │"},
      {"   ║      ║", "╭──╨──────╨──╮", "│  ≋≋≋≋≋≋≋≋  │", "│  ╭──────╮  │", "│  │  ◉   │  │", "│  ╰──────╯  │", "╰──╥──────╥──╯", "   ║      ║"},
      {"╔════════════════════╗", "║ ▱ ▱ ▱ │ ▱ ▱ ▱ │ ▱  ║", "╟───────┼───────┼────╢", "║ ▱ ▱ ▱ │ ▱ ▱ ▱ │ ▱  ║", "╚═══════╧═══════╧════╝", "  ╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱", " ◉────────────────◉"},
      {"   ╭────╮          ╭────╮", "   │ ▥▥ │──────────│ ▥▥ │", "   ╰─┬──╯          ╰──┬─╯", "     │    ┌────┐      │", "     │    │ ▒▒ │      │", " ────┴────┴────┴──────┴──"},
      {"            ┃", "       ╭────╂──╮", "       │ ▯  ┃ ▯│", "   ╭───┴────┻──┴───╮", "   │ ○   ○   ○   ○ │", "╲══╧═══════════════╧══╱", " ╲___________________╱", "   ∿∿∿∿∿∿∿∿∿∿∿∿∿∿∿"},
      {"  ╔═══════╗", "══╣       ╠════════════", "  ║   ◉   ║", "══╣       ╠════════════", "  ╚═══╤═══╝", "      │", "     ╱╲"},
      {"╭───╮    ╭───╮    ╭───╮", "│ ◉ │    │ ○ │    │ ◉ │", "│ ○ │    │ ◉ │    │ ○ │", "╰─┬─╯    ╰─┬─╯    ╰─┬─╯", "  ├────────┼────────┤", "  │        │        │"},
      {"        ╱╲", "   ╱───╱──╲───╲", "  ╱____________╲", "  │ ▤  ▤  ▤  ▤ │", "  │   ╭────╮   │", "  │ ▣ │ ▒▒ │ ▣ │", "  └───┴────┴───┘"},
      {"╭───┬───────┬───────┬───╮", "│   │       │       │   │", "│  ╭┴╮     ╭┴╮     ╭┴╮  │", "│  │◈│     │◈│     │◈│  │", "│  ╰─╯     ╰─╯     ╰─╯  │", "│                       │"},
      {"  ║                 ║", "  ║    ╭───────╮    ║", "  ║    │ ▤▤▤▤▤ │    ║", "  ╠════╡   ◉   ╞════╣", "  ║    ╰───┬───╯    ║", "  ║        │        ║", "  ║       ╱╲        ║"},
      {"       ╱╲", "      ╱◈ ╲", "     ╔════╗", "     ║ ░░ ║", "     ╚═╤╤═╝", "       ││", "      ╱││╲", "     ╱ ││ ╲"},
      {"╭────────────────────╮", "│        ◷           │", "├────────────────────┤", "│  ▤▤  ▤▤  ▤▤  ▤▤    │", "│      ╭────╮        │", "╰──────┤ ▱▱ ├────────╯", "       ╰────╯"},
      {"    ║           ║", " ╭──╨──╮     ╭──╨──╮", " │ ◉   │═════│   ◉ │", " │ ▒▒▒ │     │ ▒▒▒ │", " │ ▒▒▒ │═════│ ▒▒▒ │", " ╰──┬──╯     ╰──┬──╯", "    ║           ║"},
      {"╔══════════════════════╗", "║      ◴        ◷      ║", "╚══╤═══════════════╤═══╝", "   │               │", "  ▐▤▌    ╭───╮    ▐▤▌", "  ▐▤▌    │ ░ │    ▐▤▌", "  ▐▤▌    ╰───╯    ▐▤▌"},
      {"╭─────────────────────╮", "│ ╔═══╗   ╔══════╗    │", "│ ║ ○ ╚═══╝  ◉   ║    │", "│ ╚═══╗   ╔══╗   ║    │", "│   ◴ ╚═══╝  ╚═══╝ ◷  │", "╰─────────────────────╯"},
      {"     ╭──────────╮", "  ╔══╡ ╱╲  ╱╲   ╞══╗", "══╣  │ ╲ ◉ ╱    │  ╠══", "  ╚══╡ ╱  ╲     ╞══╝", "     ╰────┬─────╯", "          │", "          ║"},
      {"  ╔══════════╗", "══╝          ║   ╔══════", "   ╔══════╗  ║   ║", "═══╝      ║  ╚═══╝", "          ║", "       ╭──╨──╮", "       │  ◉  │", "       ╰─────╯"},
      {"╭──────────────────────╮", "│  0018    0021    0007│", "├──────┬──────┬────────┤", "│  ◴   │  ◷   │   ◉    │", "╰───┬──┴───┬──┴────┬───╯", "    ║      ║       ║", "════╩══════╩═══════╩════"},
      {" ║ ║ ║ ║ ║ ║ ║ ║ ║", "╭╨─╨─╨─╨─╨─╨─╨─╨─╨╮", "│ ▣ ▣ ▣ ▣ ▣ ▣ ▣ ▣ ▣│", "│                   │", "│ ▒▒▒▒  ▒▒▒▒  ▒▒▒▒  │", "╰───╥─────────╥─────╯", "    ║         ║"},
      {"╭────────────────────╮", "│  PRZERWA TECHNICZNA│", "╰────────────────────╯", "          ∿", "       ╭────╮", "       │    ├╮", "═══════╧════╧╧═══════", "   │             │"},
      {"    ║              ║", "╔═══╩══════════════╩═══╗", "║ ▥▥▥▥▥▥▥▥▥▥▥▥▥▥▥▥▥▥▥  ║", "║ ▥▥▥▥▥▥▥▥▥▥▥▥▥▥▥▥▥▥▥  ║", "╚═══╦══════════════╦═══╝", "    ║              ║"},
      {"╭─────────╮  ╭─────────╮", "│    ◴    │  │    ◷    │", "├─────────┤  ├─────────┤", "│  ░░░░░  │══│  ▒▒▒▒▒  │", "│  ░░░░░  │  │  ▒▒▒▒▒  │", "╰────┬────╯  ╰────┬────╯", "     ║            ║"},
      {"     │         │", "   ╭─┼─╮     ╭─┼─╮", "   │ ◉ │─────│ ◉ │", "   ╰─┬─╯     ╰─┬─╯", "     │         │", "╔════╧═════════╧════╗", "║ ▤▤▤▤▤▤▤▤▤▤▤▤▤▤▤▤  ║", "╚═══════════════════╝"},
      {"╔══════════════════════╗", "║ ▣ ▣ ▣ ▣ │ ━━━ ━━━    ║", "║ ▣ ▣ ▣ ▣ │ ━━━ ━━━    ║", "╟─────────┼────────────╢", "║  ◉  ○   │  ◴    ◷    ║", "╚═════════╧════════════╝", "  ║                 ║"}
    };
    if(id<0||size_t(id)>=landmarks.size())return;
    const auto &pattern=iceworld::pattern(id);
    const auto &pad=pattern.path[pattern.path.size()/2];
    const auto &d=iceberg::Districts[size_t(grid_.district)];
    const auto &art=landmarks[size_t(id)];
    world_art(pad.x-1.,pad.y-double(art.size())*aspect_*scale_-.3,art,d.stone);
    // The route is built into occupied infrastructure. Muted walls and
    // supports stay below the bright, collidable walking surfaces.
    for(size_t i=0;i<pattern.path.size();++i) {
      const auto &deck=pattern.path[i];
      if(deck.kind=='m')continue;
      if(grid_.district==0) {
        if(i%3==0)world_art(deck.x+1.,deck.y+1.7,{
          "╟──┬──────────┬──╢", "║  │ ▤▤  ▤▤   │  ║", "║  │          │  ║",
          "╟──┴──────────┴──╢", "║ ╲            ╱ ║", "║  ╲          ╱  ║", "║   ╲        ╱   ║"},d.shadow);
        else world_art(deck.x+1.,deck.y+1.7,{
          "╥════════════╥", "║ ╲        ╱ ║", "║  ╲      ╱  ║", "║   ╲    ╱   ║", "║    ╲  ╱    ║", "║     ╳      ║"},d.shadow);
        if(i%4==1)world_art(deck.x+1.,deck.y-2.2,{
          "╭──╮ ╭──╮", "│╳ │ │╳ │", "╰──╯ ╰──╯"},d.stone);
      } else if(grid_.district==1) {
        world_art(deck.x+1.,deck.y+1.7,{
          "╠══╤═══════╤══╣", "║  ║       ║  ║", "╚══╣       ╠══╝", "   ║       ║", "   ╚═══╦═══╝", "       ║"},d.shadow);
        if(i%4==0)world_art(deck.x+2.,deck.y-3.2,{
          "╭─────────╮", "│  ◴   ◷  │", "╰──┬───┬──╯", "   ║   ║"},d.stone);
      }
    }
    // A small, common service plate gives the authored location a human scale.
    auto first=pattern.path.front();
    world_art(first.x+2.,first.y-5.5,{"╭──────╮","│  ▱▱  │","╰──┬───╯","   │"},d.stone);
  }
