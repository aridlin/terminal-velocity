// Included inside RenderGrid. The maps share the terrain and route state.
  void draw_room_map(int top,int height) {
    const bool has_water=!grid_.water_basins.empty();
    const bool has_boost=!grid_.springs.empty()||
      std::any_of(grid_.air_fields.begin(),grid_.air_fields.end(),[](const auto &field){return field.ay<0;});
    const auto wind=std::find_if(grid_.air_fields.begin(),grid_.air_fields.end(),[&](const auto &field){
      return (field.reverses||(grid_.windbreak_roof&&field.ax!=0))&&field.active(grid_.switch_flags);
    });
    const bool has_wind=wind!=grid_.air_fields.end();
    const char *wind_symbol=has_wind?(wind->reverses?"↔":wind->ax<0?"←":"→"):"";
    const bool has_ice=!grid_.ice_strips.empty()||
      std::any_of(grid_.platforms.begin(),grid_.platforms.end(),[](const auto &platform){return platform.icy&&!platform.fragile;});
    const bool has_fragile=std::any_of(grid_.platforms.begin(),grid_.platforms.end(),
      [](const auto &platform){return platform.fragile;});
    const bool has_racks=std::any_of(grid_.platforms.begin(),grid_.platforms.end(),
      [](const auto &platform){return platform.archive_rack;});
    const bool has_seam=grid_.crown_seam.has_value();
    const bool has_trolley=std::any_of(grid_.platforms.begin(),grid_.platforms.end(),
      [](const auto &platform){return platform.archive_trolley;});
    const auto wheel=std::find_if(grid_.platforms.begin(),grid_.platforms.end(),
      [](const auto &platform){return platform.irrigation_scoop;});
    const bool has_wheel=wheel!=grid_.platforms.end();
    const bool has_trams=std::any_of(grid_.platforms.begin(),grid_.platforms.end(),
      [](const auto &platform){return platform.tram;});
    const auto ferry=[&](size_t i){return quay_ferry(i)||grid_.template_id==50;};
    const bool has_ferries=!grid_.platforms.empty()&&ferry(0);
    const bool has_service_lift=std::any_of(grid_.platforms.begin(),grid_.platforms.end(),
      [](const auto &platform){return platform.service_lift;});
    const auto powered=[](const SortingBelt &belt){return belt.speed!=0;};
    const bool has_belts=std::any_of(grid_.sorting_belts.begin(),grid_.sorting_belts.end(),powered);
    const bool has_features=has_water||has_boost||has_wind||has_ice||has_racks||!grid_.sweets.empty();
    const auto branch=std::find_if(grid_.interactions.begin(),grid_.interactions.end(),[](const auto &node){
      return node.kind==InteractionKind::RouteChoice;
    });
    const bool fork=branch!=grid_.interactions.end();
    const bool settlement=grid_.room_role==iceberg::RoomRole::Settlement;
    const bool show_goal=!settlement&&grid_.goal.x>=0&&grid_.goal.x<grid_.width()&&
      grid_.goal.y>=0&&grid_.goal.y<grid_.height();
    const auto return_door=[](const auto &node){return node.kind==InteractionKind::Return;};
    const bool has_return=std::any_of(grid_.interactions.begin(),grid_.interactions.end(),return_door);
    const bool return_only=has_return&&!show_goal&&!settlement;
    constexpr uint32_t return_color=0x91d4ce;
    const auto missing=show_goal?grid_.goal_requires&~grid_.switch_flags:0;
    const auto required_valve=[missing](const auto &node) {
      return node.kind==InteractionKind::Valve&&(missing&(uint64_t(1)<<node.id));
    };
    const bool has_valves=std::any_of(grid_.interactions.begin(),grid_.interactions.end(),required_valve);
    uint64_t bridge_flags=0;
    for(const auto &bridge:grid_.growth)bridge_flags|=bridge.flag;
    const auto path_controls=bridge_flags&~grid_.switch_flags&~missing;
    const auto path_valve=[path_controls](const auto &node) {
      return node.kind==InteractionKind::Valve&&(path_controls&(uint64_t(1)<<node.id));
    };
    const bool has_path_control=std::any_of(grid_.interactions.begin(),grid_.interactions.end(),path_valve);
    const bool has_bridges=std::any_of(grid_.growth.begin(),grid_.growth.end(),
      [&](const auto &bridge){return bridge.extended_width(grid_.switch_flags)>0;});
    const bool bridge_legend=has_path_control||has_bridges;
    constexpr uint32_t valve_color=0xf0a277,bridge_color=0xabc9e4;
    constexpr uint32_t tram_color=0xefcd74,stop_color=0xc2accf;
    constexpr uint32_t ferry_color=0xe4ad68,ferry_route_color=0x6f9eaa;
    constexpr uint32_t belt_color=0xa0d6df,trolley_color=0xaee1df,wheel_color=0xc5a36d;
    constexpr uint32_t refill_color=0x8ee0dd,recharging_color=0x7895a0;
    const bool has_refills=!grid_.dash_locked()&&!grid_.refills.empty();
    const bool has_vents=!grid_.pulses.empty();
    constexpr uint32_t service_lift_color=0xe4ad68,seam_color=0xc4deeb;
    constexpr uint32_t fragile_color=0xd5aa80,cracking_color=0xf1c66f;
    struct FieldLandmark {const char *symbol;iceberg::Text legend;uint32_t color;};
    static constexpr std::array<FieldLandmark,3> field_landmarks{{
      {"▤",{"▤ Tabliczka","▤ Route sign"},0xe7d8b4},
      {"✉",{"✉ List","✉ Letter"},0xf2d0a4},
      {"◖",{"◖ Rura głosowa","◖ Speaking tube"},0xd4bc88}
    }};
    const auto field_landmark=[](const auto &node) {
      if(node.kind!=InteractionKind::Note)return -1;
      switch(node.id) {
      case iceberg::CrownShaftPlate:case iceberg::ArchiveTrolleyNotice:return 0;
      case iceberg::ArchiveDryingLetter:return 1;
      case iceberg::IrrigationCallBox:return 2;
      default:return -1;
      }
    };
    std::array<bool,field_landmarks.size()> has_landmark{};
    for(const auto &node:grid_.interactions)
      if(const int i=field_landmark(node);i>=0)has_landmark[size_t(i)]=true;
    const bool paper_landmarks=has_landmark[0]||has_landmark[1];
    const bool tube_landmark=has_landmark[2];
    const int legend_rows=1+int(has_features)+int(fork)+int(has_valves)+int(has_trams)+int(has_belts)
      +int(bridge_legend)+int(has_return&&!return_only)+int(has_refills)+int(has_vents)+int(has_service_lift)
      +int(has_fragile)+int(has_ferries)+int(has_trolley)+int(has_seam)+int(has_wheel)
      +int(paper_landmarks)+int(tube_landmark);
    const auto title=wrap(grid_.room_name.get(state_.adventure.language),cols_-6);
    const int title_rows=(has_ferries||paper_landmarks||tube_landmark)&&title.size()==1?1:2;
    const int width=std::min(cols_-8,72),map_rows=std::min(height-2-title_rows-legend_rows,18);
    const int left=(cols_-width)/2,first=top+title_rows+1;
    int title_row=top;
    for(const auto &line:title) {
      if(title_row>=top+title_rows)break;
      center(title_row++,line,Color::Gold);
    }
    frame(first-1,left-1,width+2,map_rows+2,0x637f8e);
    int terrain_top=0;
    for(;terrain_top<grid_.height();++terrain_top) {
      int x=0;
      while(x<grid_.width()&&!grid_.solid_at(x,terrain_top)&&!grid_.hazard_at(x,terrain_top))++x;
      if(x<grid_.width())break;
    }
    double upper=show_goal?std::min(double(terrain_top),grid_.goal.y):terrain_top;
    for(const auto &platform:grid_.platforms)upper=std::min(upper,platform.base.y-std::abs(platform.dy));
    const int world_top=std::max(0,int(upper)-6);
    auto column=[&](double x){return std::clamp(int(x/grid_.width()*width),0,width-1);};
    auto row=[&](double y){return std::clamp(int((y-world_top)/(grid_.height()-world_top)*map_rows),0,map_rows-1);};

    // Exposed edges keep solid foundations from filling the map at compact sizes.
    std::vector<std::string> map(size_t(map_rows),std::string(size_t(width),' '));
    for(int y=world_top;y<grid_.height();++y) {
      auto &line=map[size_t(row(y))];
      for(int x=0;x<grid_.width();++x) {
        auto &tile=line[size_t(column(x))];
        if(grid_.solid_at(x,y)) {
          const bool edge=!grid_.solid_at(x,y-1)||
            (y+1<grid_.height()&&!grid_.solid_at(x,y+1))||
            (x>0&&!grid_.solid_at(x-1,y))||
            (x+1<grid_.width()&&!grid_.solid_at(x+1,y));
          if(edge)tile='#';
        }
        else if(tile==' '&&grid_.hazard_at(x,y))tile='!';
      }
    }
    for(const auto &strip:grid_.ice_strips) {
      auto &line=map[size_t(row(strip.top()))];
      const int last=column(std::nextafter(strip.right(),strip.left()));
      for(int x=column(strip.left());x<=last;++x)line[size_t(x)]='i';
    }
    // Water is a background layer so ice, hazards and markers stay visible above it.
    for(const auto &basin:grid_.water_basins) {
      const int surface=row(basin.top()),bottom=row(std::nextafter(basin.bottom(),basin.top()));
      const int start=column(basin.left()),end=column(std::nextafter(basin.right(),basin.left()));
      for(int y=surface;y<=bottom;++y)for(int x=start;x<=end;++x) {
        auto &background=buffer_[first+y][left+x].background;
        if(y==surface) {
          background=water_surface_background;
          auto &tile=map[size_t(y)][size_t(x)];
          if(tile==' ')tile='~';
        } else if(background!=water_surface_background) {
          background=water_depth_background;
        }
      }
    }
    for(const auto &bridge:grid_.growth) {
      const double length=bridge.extended_width(grid_.switch_flags);
      if(length<=0)continue;
      auto &line=map[size_t(row(bridge.area.y))];
      const int last=column(std::nextafter(bridge.area.x+length,bridge.area.x));
      for(int x=column(bridge.area.x);x<=last;++x)line[size_t(x)]='g';
    }
    for(const auto &belt:grid_.sorting_belts)if(powered(belt)) {
      auto &line=map[size_t(row(belt.y))];
      const int last=column(std::nextafter(belt.x+belt.width,belt.x));
      for(int x=column(belt.x);x<=last;++x)line[size_t(x)]='b';
    }
    if(has_wheel)for(int i=0;i<32;++i) {
      const auto rim=wheel->at(wheel->period*i/32.);
      auto &cell=map[size_t(row(rim.y))][size_t(column(rim.x+rim.w*.5))];
      if(cell==' ')cell='s';
    }
    std::vector<std::array<int,3>> cracking_spans;
    for(size_t i=0;i<grid_.platforms.size();++i) {
      const auto &platform=grid_.platforms[i];
      if(platform.archive_rack||platform.service_lift) {
        const int x=column(platform.base.x+platform.base.w*.5);
        const double range=std::abs(platform.dy);
        for(int y=row(platform.base.y-range);y<=row(platform.base.y+range);++y)
          if(map[size_t(y)][size_t(x)]==' ')map[size_t(y)][size_t(x)]=':';
      }
      if(platform.archive_trolley) {
        const double center=platform.base.x+platform.base.w*.5;
        for(int n=0;n<=16;++n) {
          const double fraction=n/8.-1;
          auto &cell=map[size_t(row(platform.base.y+platform.dy*fraction))]
                        [size_t(column(center+platform.dx*fraction))];
          if(cell==' ')cell='s';
        }
      }
      if(platform.tram) {
        const double center=platform.base.x+platform.base.w*.5,range=std::abs(platform.dx);
        auto &rail=map[size_t(row(platform.base.y))];
        for(int x=column(center-range);x<=column(center+range);++x)
          if(rail[size_t(x)]==' ')rail[size_t(x)]='-';
      }
      if(ferry(i)) {
        const double center=platform.base.x+platform.base.w*.5,range=std::abs(platform.dx);
        auto &lane=map[size_t(std::max(0,row(platform.base.y)-1))];
        for(int x=column(center-range);x<=column(center+range);++x)
          if(lane[size_t(x)]==' ')lane[size_t(x)]='s';
      }
      if(!platform.available(state_,i,state_.elapsed))continue;
      const auto deck=platform.at(state_.elapsed);
      char tile=platform.icy?'i':'=';
      if(platform.fragile) {
        const bool cracking=platform.crack_age(state_,i,state_.elapsed)<MovingPlatform::CrackTime;
        tile=cracking?'c':platform.icy?'j':'f';
      }
      const int y=row(deck.y),start=column(deck.left());
      const int last=column(std::nextafter(deck.right(),deck.left()));
      if(tile=='c')cracking_spans.push_back({y,start,last});
      auto &line=map[size_t(y)];
      for(int x=start;x<=last;++x)line[size_t(x)]=tile;
    }
    for(int y=0;y<map_rows;++y)for(int x=0;x<width;++x) {
      switch(map[size_t(y)][size_t(x)]) {
      case '#':paint_text(first+y,left+x,"▪",0x8ba2ae);break;
      case '!':paint_text(first+y,left+x,"░",0xa26666);break;
      case '=':paint_text(first+y,left+x,"─",0x9dd4dd);break;
      case ':':paint_text(first+y,left+x,"┆",0x718c9d);break;
      case '-':paint_text(first+y,left+x,"┄",0x887b96);break;
      case 's':paint_text(first+y,left+x,"┄",ferry_route_color);break;
      case 'b':paint_text(first+y,left+x,"═",belt_color);break;
      case 'g':paint_text(first+y,left+x,"═",bridge_color);break;
      case 'i':paint_text(first+y,left+x,"━",0xa6e9ea);break;
      case 'f':paint_text(first+y,left+x,"┅",fragile_color);break;
      case 'j':paint_text(first+y,left+x,"┅",0xa6e9ea);break;
      case 'c':paint_text(first+y,left+x,"╍",cracking_color);break;
      case '~':paint_text(first+y,left+x,"≈",0x66bfd7);break;
      }
    }
    struct Marker {int x,y,direction;const char *symbol;uint32_t color;};
    std::vector<Marker> overlaps;
    auto place=[&](const Marker &marker,int dx,int dy){
      const int x=marker.x+dx,y=marker.y+dy;
      if(x<0||x>=width||y<0||y>=map_rows||map[size_t(y)][size_t(x)]=='@')return false;
      map[size_t(y)][size_t(x)]='@';
      paint_text(first+y,left+x,marker.symbol,marker.color,true);
      return true;
    };
    auto mark_cell=[&](Marker marker){
      if(!place(marker,0,0))overlaps.push_back(marker);
    };
    auto mark=[&](double x,double y,const char *symbol,uint32_t color,int direction=1){
      mark_cell({column(x),row(y),direction,symbol,color});
    };
    // Give the courier and doorways first choice of their exact cells.
    mark(state_.player.x+kPlayerWidth*.5,state_.player.y+kPlayerHeight*.5,"@",0xffd68a);
    if(show_goal) {
      mark(grid_.goal.x+grid_.goal.w*.5,grid_.goal.y+grid_.goal.h*.5,"▣",missing?0x7c8492:0xd7e8cd);
    }
    for(const auto &node:grid_.interactions) {
      if(required_valve(node)||path_valve(node))
        mark(node.area.x+node.area.w*.5,node.area.y+node.area.h*.5,"⊗",
          required_valve(node)?valve_color:bridge_color);
    }
    if(fork) {
      const auto &door=branch->area;
      mark(door.x+door.w*.5,door.y+door.h*.5,"▧",0xe8a758,-1);
    }
    const size_t active=size_t(state_.checkpoint);
    if(active<grid_.checkpoint_points.size()) {
      const auto [x,y]=grid_.checkpoint_points[active];
      mark(x+kPlayerWidth*.5,y,"◆",0x9dd4dd,-1);
    }
    for(const auto &node:grid_.interactions) {
      const bool board=settlement&&node.kind==InteractionKind::Board;
      if(board||return_door(node))
        mark(node.area.x+node.area.w*.5,node.area.y+node.area.h*.5,board?"▤":"↶",
          board?0xd7e8cd:return_color,board?1:-1);
    }
    for(size_t i=0;i<grid_.checkpoint_points.size();++i)if(i!=active) {
      const auto [x,y]=grid_.checkpoint_points[i];
      mark(x+kPlayerWidth*.5,y,"◇",0x9dd4dd);
    }
    for(const auto &platform:grid_.platforms)if(platform.tram) {
      const auto car=platform.at(state_.elapsed);
      const double center=platform.base.x+platform.base.w*.5,range=std::abs(platform.dx);
      mark(center-range,platform.base.y,"○",stop_color,-1);
      mark(center+range,platform.base.y,"○",stop_color);
      // The car sits above the track so a docked car leaves both stops visible.
      mark_cell({column(car.x+car.w*.5),std::max(0,row(car.y)-1),1,"▰",tram_color});
    }
    for(const auto &platform:grid_.platforms)if(platform.archive_trolley) {
      const auto deck=platform.at(state_.elapsed);
      const double center=platform.base.x+platform.base.w*.5;
      mark(center-platform.dx,platform.base.y-platform.dy,"○",stop_color,-1);
      mark(center+platform.dx,platform.base.y+platform.dy,"○",stop_color);
      mark_cell({column(deck.x+deck.w*.5),std::max(0,row(deck.y)-1),1,"▱",trolley_color});
    }
    if(has_wheel)mark(wheel->base.x+wheel->base.w*.5,wheel->base.y,"⊕",wheel_color);
    if(has_ferries)for(size_t i=0;i<grid_.platforms.size();++i)if(ferry(i)) {
      const auto deck=grid_.platforms[i].at(state_.elapsed);
      mark_cell({column(deck.x+deck.w*.5),std::max(0,row(deck.y)-1),1,"▱",ferry_color});
    }
    if(has_seam) {
      const auto &seam=*grid_.crown_seam;
      mark(seam.x+seam.w*.5,seam.bottom(),"↑",seam_color);
    }
    for(const auto &belt:grid_.sorting_belts)if(powered(belt))
      mark(belt.x+belt.width*.5,belt.y,belt.speed>0?"▶":"◀",belt_color,belt.speed>0?1:-1);
    for(const auto &platform:grid_.platforms)if(platform.service_lift) {
      const auto deck=platform.at(state_.elapsed);
      mark(deck.x+deck.w*.5,deck.y,"↕",service_lift_color);
    }
    bool recharging=false;
    if(has_refills)for(size_t i=0;i<grid_.refills.size();++i) {
      const auto [x,y]=grid_.refills[i];
      const bool spent=state_.refill_cooldown[i]>0;
      mark(x,y,spent?"▯":"▥",spent?recharging_color:refill_color);
      recharging|=spent;
    }
    for(const auto &vent:grid_.pulses) {
      const bool active=vent.active(state_.elapsed),warning=vent.warning(state_.elapsed);
      mark(vent.area.x+vent.area.w*.5,vent.area.bottom(),
        active?"♨":warning?"↑":"╧",active?vent_active:warning?vent_warning:vent_idle);
    }
    bool remaining_token=false;
    for(size_t i=0;i<grid_.sweets.size();++i)if(!state_.collected[i]) {
      const auto [x,y]=grid_.sweets[i];
      mark(x,y,"●",0xedc278);
      remaining_token=true;
    }
    std::string boost_symbols;
    auto boost=[&](double x,double y,const char *symbol) {
      mark(x,y,symbol,0x9dd39c);
      if(boost_symbols.find(symbol)==std::string::npos)boost_symbols+=symbol;
    };
    for(const auto &spring:grid_.springs)
      boost(spring.x+spring.w*.5,spring.y,spring.push_x>0?"↗":spring.push_x<0?"↖":"↑");
    for(const auto &field:grid_.air_fields)if(field.ay<0&&field.active(grid_.switch_flags))
      boost(field.area.x+field.area.w*.5,field.area.bottom()-1,"↑");
    if(has_wind)
      mark(wind->area.x+wind->area.w*.5,wind->area.y+3,wind_symbol,0xb4d7e4);
    for(const auto &platform:grid_.platforms)if(platform.archive_rack) {
      const auto rack=platform.at(state_.elapsed);
      mark(rack.x+rack.w*.5,rack.y,archive_rack_direction(platform,state_.elapsed),ArchiveRackMarker);
    }
    for(const auto &node:grid_.interactions)
      if(const int i=field_landmark(node);i>=0) {
        const auto &landmark=field_landmarks[size_t(i)];
        mark(node.area.x+node.area.w*.5,node.area.y+node.area.h*.5,landmark.symbol,landmark.color);
      }
    // Reserve uncontested positions before moving overlapping markers.
    // Nearby slots keep the displaced icons close to their real positions.
    static constexpr std::pair<int,int> nearby[]={
      {1,0},{-1,0},{0,-1},{0,1},{2,0},{-2,0},{1,-1},{-1,-1},{1,1},{-1,1},{0,-2},{0,2}
    };
    for(const auto &marker:overlaps)for(const auto &[dx,dy]:nearby)
      if(place(marker,dx*marker.direction,dy))break;
    // On a compact map an entire floe can sit under the courier marker.
    // Keep its warning visible above the deck if its footprint is covered.
    for(const auto &[y,start,last]:cracking_spans) {
      const auto &line=map[size_t(y)];
      if(line.find('c',size_t(start))<=size_t(last))continue;
      const Marker warning{(start+last)/2,y,1,"╍",cracking_color};
      if(!place(warning,0,-1))
        for(const auto &[dx,dy]:nearby)if(place(warning,dx,dy))break;
    }
    int legend=top+height-legend_rows;
    if(fork) {
      paint_text(legend++,2,tr("@ Ty · ◆ Lampa","@ You · ◆ Lamp"),0xc7d9df);
      const auto main=tr("▣ Główne · ","▣ Main · ");
      paint_text(legend,2,main,0xc7d9df);
      paint_text(legend++,2+int(glyphs(main).size()),tr("▧ Przejście B","▧ Passage B"),0xe8a758);
    } else if(return_only) {
      const auto base=tr("@ Ty · ◆ Lampa · ","@ You · ◆ Lamp · ");
      paint_text(legend,2,base,0xc7d9df);
      paint_text(legend++,2+int(glyphs(base).size()),tr("↶ Powrót","↶ Return"),return_color);
    } else {
      paint_text(legend++,2,settlement?tr("@ Ty · ◆ Lampa · ▤ Tablica","@ You · ◆ Lamp · ▤ Board"):
        tr("@ Ty · ◆ Lampa · ▣ Wyjście","@ You · ◆ Lamp · ▣ Exit"),0xc7d9df);
    }
    if(has_return&&!return_only)paint_text(legend++,2,tr("↶ Powrót","↶ Return"),return_color);
    if(has_valves)paint_text(legend++,2,tr("⊗ Wymagany zawór","⊗ Required valve"),valve_color);
    if(bridge_legend) {
      std::string label;
      if(has_path_control)label=tr("⊗ Wysuń pomost","⊗ Extend path");
      if(has_bridges) {
        if(!label.empty())label+=" · ";
        label+=tr("═ Pomost","═ Bridge");
      }
      paint_text(legend++,2,label,bridge_color);
    }
    if(has_trams) {
      const auto name=tr("▰ Tramwaj","▰ Tram");
      paint_text(legend,2,name,tram_color);
      const int separator=2+int(glyphs(name).size());
      paint_text(legend,separator," · ",0xc7d9df);
      paint_text(legend++,separator+3,tr("○ Przystanek","○ Stop"),stop_color);
    }
    if(has_ferries) {
      const auto name=tr("▱ Prom","▱ Ferry");
      paint_text(legend,2,name,ferry_color);
      const int separator=2+int(glyphs(name).size());
      paint_text(legend,separator," · ",0xc7d9df);
      paint_text(legend++,separator+3,tr("┄ Trasa","┄ Route"),ferry_route_color);
    }
    if(has_service_lift)paint_text(legend++,2,tr("↕ Winda serwisowa","↕ Service lift"),service_lift_color);
    if(has_seam)paint_text(legend++,2,tr("↑ Wąski szyb","↑ Wall-kick shaft"),seam_color);
    if(has_belts)paint_text(legend++,2,tr("◀ ▶ Taśmy","◀ ▶ Belts"),belt_color);
    if(has_trolley) {
      const auto name=tr("▱ Wózek","▱ Trolley");
      paint_text(legend,2,name,trolley_color);
      const int separator=2+int(glyphs(name).size());
      paint_text(legend,separator," · ",0xc7d9df);
      paint_text(legend++,separator+3,tr("○ Przystanek","○ Stop"),stop_color);
    }
    if(has_wheel)paint_text(legend++,2,tr("⊕ Koło nawadniające","⊕ Irrigation wheel"),wheel_color);
    if(paper_landmarks) {
      int x=2;
      if(has_landmark[0]) {
        const auto label=field_landmarks[0].legend.get(state_.adventure.language);
        paint_text(legend,x,label,field_landmarks[0].color);
        x+=iceberg::cell_length(label);
        if(has_landmark[1]){paint_text(legend,x," · ",0xc7d9df);x+=3;}
      }
      if(has_landmark[1])
        paint_text(legend,x,field_landmarks[1].legend.get(state_.adventure.language),field_landmarks[1].color);
      ++legend;
    }
    if(tube_landmark)
      paint_text(legend++,2,field_landmarks[2].legend.get(state_.adventure.language),field_landmarks[2].color);
    if(has_refills) {
      const auto name=tr("▥ Zapas zrywu","▥ Dash refill");
      paint_text(legend,2,name,refill_color);
      if(recharging) {
        const int separator=2+int(glyphs(name).size());
        paint_text(legend,separator," · ",0xc7d9df);
        paint_text(legend,separator+3,tr("▯ Ładuje się","▯ Recharging"),recharging_color);
      }
      ++legend;
    }
    if(has_vents) {
      int x=2;
      for(const auto &[label,color]:{std::pair{tr("╧ Dysza","╧ Vent"),vent_idle},
          std::pair{tr("↑ Uwaga","↑ Warning"),vent_warning},
          std::pair{tr("♨ Aktywna","♨ Active"),vent_active}}) {
        if(x>2){paint_text(legend,x," · ",0xc7d9df);x+=3;}
        paint_text(legend,x,label,color);
        x+=int(glyphs(label).size());
      }
      ++legend;
    }
    if(has_fragile) {
      const auto name=tr("┅ Kruche","┅ Fragile");
      const int separator=2+int(glyphs(name).size());
      paint_text(legend,2,name,fragile_color);
      paint_text(legend,separator," · ",0xc7d9df);
      paint_text(legend++,separator+3,tr("╍ Pękają","╍ Cracking"),cracking_color);
    }
    int feature_col=2;
    auto label=[&](const std::string &entry,uint32_t color) {
      if(feature_col>2){paint_text(legend,feature_col," · ",0xc7d9df);feature_col+=3;}
      paint_text(legend,feature_col,entry,color);
      feature_col+=int(glyphs(entry).size());
    };
    if(remaining_token)label(tr("● Żeton","● Token"),0xedc278);
    if(!boost_symbols.empty())label(boost_symbols+tr(" Wybicie"," Boost"),0x9dd39c);
    if(has_water)label(tr("≈ Woda","≈ Water"),0x66bfd7);
    if(has_ice)label(tr("━ Lód","━ Ice"),0xa6e9ea);
    if(has_wind)label(wind_symbol+tr(" Wiatr"," Wind"),0xb4d7e4);
    if(has_racks)label(tr("▲▼ Regały","▲▼ Racks"),ArchiveRackMarker);
  }

  void draw_iceberg_route_map(int top,int height) {
    const auto &v=state_.adventure;const auto &r=v.route;
    const int bottom=top+height-1;
    auto clipped=[&](int row,int col,const std::string &s,uint32_t color) {
      if(row<top||row>=bottom)return;
      std::string part;int width=std::max(0,cols_-col-4);auto chars=glyphs(s);
      bool shortened=int(chars.size())>width;
      for(int i=0;i<std::min(int(chars.size()),std::max(0,width-int(shortened)));++i)part+=chars[size_t(i)];
      if(shortened&&width>0)part+="…";
      paint_text(row,col,part,color);
    };
    const bool campaign=v.mode==iceberg::Mode::Campaign;
    center(top,campaign?tr("TRASA: POKÓJ PO POKOJU","ROUTE: ROOM BY ROOM"):
      v.mode==iceberg::Mode::Endless?tr("WYPRAWA BEZ KOŃCA","ENDLESS EXPEDITION"):tr("PRÓBA MISTRZOWSKA","MASTERY TRIAL"),Color::Gold);
    if(!r.available) {
      auto lines=wrap(tr("Wróć na swoją trasę, aby zobaczyć jej przebieg.","Return to your route to see its progress."),cols_-10);
      int y=top+3;for(const auto &line:lines)clipped(y++,5,line,0xc7d9df);
      return;
    }
    const int room_count=iceberg::RoomsPerLeg;
    const int leg=campaign?std::clamp(v.route_page<0?r.leg:v.route_page,0,2):0;
    const int offset=leg*room_count;
    const int endless_leg=v.depth/room_count+1;
    const bool browsing_other_leg=!r.browsed_rooms.empty();
    const auto &rooms=browsing_other_leg?r.browsed_rooms:r.rooms;
    const int selected=v.route_room_selection<0?-1:v.route_room_selection%room_count;
    const int current_room=browsing_other_leg?-1:r.current;
    std::string progress;
    if(campaign)progress=tr("Odcinek ","Leg ")+std::to_string(leg+1)+" / 3  ·  "+
      iceberg::route_title(v.district,2+2*leg).get(v.language);
    else if(r.browsed_leg<0)progress=tr("Ukończony odcinek ","Completed leg ")+std::to_string(endless_leg+r.browsed_leg);
    else if(r.browsed_leg>0)progress=tr("Następny odcinek ","Next leg ")+std::to_string(endless_leg+1);
    else if(v.mode==iceberg::Mode::Endless)progress=tr("Odcinek ","Leg ")+std::to_string(endless_leg)+
      tr(" · Głębokość "," · Depth ")+std::to_string(v.depth);
    else progress=tr("Pokój ","Room ")+std::to_string(r.current+1)+" / "+std::to_string(r.rooms.size());
    clipped(top+1,5,progress,0xc7d9df);
    if(rooms.empty()) {
      auto lines=wrap(tr("Trasa zaczyna się za bramą szkoleniową. Tablica wypraw prowadzi dalej.","The route begins beyond the training gate. The expedition board leads onward."),cols_-10);
      int y=top+4;for(const auto &line:lines)clipped(y++,5,line,0xc7d9df);
    } else {
      const bool wide=cols_>=56;
      const int stride=wide?6:3,node_width=wide?4:2;
      const int left=std::max(3,(cols_-((room_count-1)*stride+node_width))/2),y=top+3;
      for(int n=0;n<room_count&&offset+n<int(rooms.size());++n) {
        const int idx=offset+n;
        const bool branch=r.branching&&n==2;
        const bool alt=branch&&(r.choices&(1u<<leg));
        const bool current=current_room==idx&&!alt;
        const bool visited=(r.browsed_leg<0||(!browsing_other_leg&&(r.visited&(1u<<idx))))&&!alt;
        std::string number=wide?(idx+1<10?"0":"")+std::to_string(idx+1):std::to_string(n+1);
        const auto label=wide?(current?"<":visited?"[":"(")+number+(current?">":visited?"]":")"):
          (current?"▶":visited?"●":"○")+number;
        clipped(y,left+n*stride,label,current?0xffd68a:visited?0x91d4ce:0x607887);
        if(!campaign&&selected==idx&&!current)
          clipped(y+1,left+n*stride+(wide?1:0),"╰╯",0xc7d9df);
        if(n+1<room_count&&idx+1<int(rooms.size()))clipped(y,left+n*stride+node_width,wide?"──":"─",0x607887);
      }
      if(r.branching) {
        const int x=left+stride+node_width-1;
        clipped(y+1,x,"╲",0x607887);
        clipped(y+1,left+3*stride,"╱",0x607887);
        const bool alt=r.choices&(1u<<leg),current=alt&&r.current==offset+2;
        const bool visited=alt&&(r.visited&(1u<<(offset+2)));
        clipped(y+2,left+2*stride,(wide?(current?"<B>":visited?"[B]":"(B)"):(current?"▶B":visited?"●B":"○B")),current?0xffd68a:visited?0x91d4ce:0x607887);
        clipped(y+3,5,tr("B: ","B: ")+r.alternatives[size_t(leg)].get(v.language),0x91d4ce);
      }
      const int legend=y+(r.branching?4:2);
      clipped(legend,5,(wide?tr("< > tutaj   [ ] odwiedzone","< > here   [ ] visited"):tr("▶ tutaj   ● odwiedzone","▶ here   ● visited")),0xc7d9df);
      if(legend+1<bottom-1)clipped(legend+1,5,(wide?tr("( ) nieodwiedzone","( ) not recorded as visited"):tr("○ nieodwiedzone","○ no visit recorded")),0x607887);
    }
    const auto current=tr("Teraz: ","Now: ")+r.place.get(v.language);
    if(campaign) {
      if(bottom-1>top+4)clipped(bottom-1,5,current,0xffd68a);
      return;
    }
    // Put the room details just below the route and wrap at terminal-cell width.
    int row=top+7;
    if(browsing_other_leg) {
      const auto location=tr("Teraz: odc. ","Now: leg ")+std::to_string(endless_leg)+
        tr(", pok. ",", room ")+std::to_string(r.current+1);
      for(const auto &line:wrap(location,cols_-9))clipped(row++,5,line,0xffd68a);
    }
    const auto place=browsing_other_leg?r.place.get(v.language):current;
    for(const auto &line:wrap(place,cols_-9))clipped(row++,5,line,0xffd68a);
    const int count=int(rooms.size());
    const bool browsing=selected>=0&&selected<count&&selected!=current_room;
    if(browsing) {
      const auto detail=tr("Pokój ","Room ")+std::to_string(selected+1)+": "+rooms[size_t(selected)].get(v.language);
      for(const auto &line:wrap(detail,cols_-9))clipped(row++,5,line,0xc7d9df);
    }
    if(count>0&&(browsing?selected:current_room)==count-1) {
      const auto exit=v.mode==iceberg::Mode::Endless
        ?tr("Wyjście do odcinka ","Exit starts leg ")+std::to_string(endless_leg+r.browsed_leg+1)
        :tr("Wyjście kończy próbę","Exit completes the trial");
      for(const auto &line:wrap(exit,cols_-9))clipped(row++,5,line,0xc7d9df);
    } else if(!browsing&&current_room>=0&&current_room+1<count) {
      const auto next=tr("Plan: ","Planned: ")+rooms[size_t(current_room+1)].get(v.language);
      for(const auto &line:wrap(next,cols_-9))clipped(row++,5,line,0xc7d9df);
    }
  }
