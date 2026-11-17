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
  void draw_quay_ferry(const Rect &deck) {
    const double row=aspect_*scale_;
    int width=std::max(6,int(deck.w/scale_));
    std::string hull="╲";
    for(int i=2;i<width;++i)hull+=i%7==0?"○":"━";
    world_art(deck.x,deck.y+row,{hull+"╱"},0xa58460);
    world_art(deck.x+deck.w*.38,deck.y-6*row,
      {"     ▆▆","     ▓▓","  ▟██████▙","  ▌▥ ▥ ▥ ▐","╔═╧══════╧══╗","║ ○       ○ ║"},0x82999a);
    int wake=int(state_.elapsed*4)%3;
    world_art(deck.x-3*scale_,deck.y+2*row,{wake==0?"∿  ∿":" ∿  ∿"},0x527a89);
  }
  void draw_pipe_drip(double x,double roof,double floor,double offset) {
    double row=aspect_*scale_;
    world_art(x-12*scale_,roof,{"══╪╪════╪╪══╗","──┼┼────┼┼─╮║","           ╰╨"},0xa1a8a1);
    double phase=std::fmod(state_.adventure.presentation_time+offset,2.35);
    double lip=roof+2*row;
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
      draw_quay_hoist();
      world_art(106,48-6*aspect_*scale_,{" ╱▔▔╲╱▔▔╲╱▔▔╲╱▔▔╲","│                 │","│  ♧  ▰  ♧  ▰    │","├─────────────────┤","│                 │","╰───┬─────────┬───╯"},d.stone);
      world_art(94+std::sin(time*.7)*1.5,48-3*aspect_*scale_,{" ▄ ",int(time*2)%2?"╭▤╯":"╰▤╮","╱ ╲"},0x728b8b);
      for(int x=83;x<123;x+=12)world_art(x,58,{"║","║","╨"},d.stone);
      world_art(112,48-8*aspect_*scale_,{"╭─────╮","│  ◷  │","╰──┬──╯","   │","   │"},d.accent);
      return;
    }
    if(id==601) {
      world_art(8,55-8*aspect_*scale_,{"╭─────────────────────────╮",tr("│ SZKOŁA NA NABRZEŻU      │","│ LESSONS ON THE QUAY     │"),"├─────────────────────────┤","│ ▤▤ ▤▤ ▤▤  ╭─────╮  ▤▤   │","│           │     │       │","│           │  ○  │       │","╰───────────┴─────┴───────╯"},d.stone);
      draw_quay_ferry(grid_.platforms.front().at(state_.elapsed));
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
          if(i>0)world_art(a.x+4+std::sin(time*.7+i)*1.4,a.y-3*aspect_*scale_,
            {" ▄ ",int(time*2)%2?"╭▤╯":"╰▤╮","╱ ╲"},0x6a8587);
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
        for(const auto &platform:grid_.platforms) {
          auto boat=platform.at(state_.elapsed);
          draw_quay_ferry(boat);
        }
        world_art(126,41-6*aspect_*scale_,{"    ╱╲","   ╱  ╲","  ╱    ╲"," ╱______╲","     │","─────┴──────"},d.stone);
      } else if(id==505) {
        draw_pipe_drip(44,34.5,41,0);
        draw_pipe_drip(78,38.5,45,1.1);
        draw_pipe_drip(102,31,48,.3);
        for(int x=72;x<grid_.width()-12;x+=25)world_art(x,28,{"╔══════════════════╗","╚══════════════════╝","     ║       ║"},0x38535a);
        world_art(98,48-5*aspect_*scale_,{"╭─────────────╮","│  ◴     ◷    │","╰──┬─────┬────╯","   ║     ║","═══╩═════╩════"},d.stone);
      } else if(id==506) {
        for(size_t i=1;i<p.path.size();i+=2) {
          auto a=p.path[i];world_art(a.x+5,a.y-7*aspect_*scale_,{"╭───╮","│ ▣ │","╰─┬─╯","  │","  │","  │","══╧══"},0xa18d67);
        }
      } else if(id==507) {
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
