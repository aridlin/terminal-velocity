// Included inside RenderGrid. The maps share the terrain and route state.
  void draw_room_map(int top,int height) {
    const int width=std::min(cols_-8,72), map_rows=std::min(height-5,18);
    const int left=(cols_-width)/2, first=top+3;
    int title_row=top;
    for(const auto &line:wrap(grid_.room_name.get(state_.adventure.language),cols_-6)) {
      if(title_row>=top+2)break;
      center(title_row++,line,Color::Gold);
    }
    frame(first-1,left-1,width+2,map_rows+2,0x637f8e);
    int terrain_top=0;
    for(;terrain_top<grid_.height();++terrain_top) {
      int x=0;
      while(x<grid_.width()&&!grid_.solid_at(x,terrain_top)&&!grid_.hazard_at(x,terrain_top))++x;
      if(x<grid_.width())break;
    }
    const bool settlement=grid_.room_role==iceberg::RoomRole::Settlement;
    const bool show_goal=!settlement&&grid_.goal.x>=0&&grid_.goal.x<grid_.width()&&
      grid_.goal.y>=0&&grid_.goal.y<grid_.height();
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
    for(size_t i=0;i<grid_.platforms.size();++i) {
      const auto &platform=grid_.platforms[i];
      if(!platform.available(state_,i,state_.elapsed))continue;
      const auto deck=platform.at(state_.elapsed);
      auto &line=map[size_t(row(deck.y))];
      for(int x=column(deck.x);x<=column(deck.right());++x)line[size_t(x)]='=';
    }
    for(int y=0;y<map_rows;++y)for(int x=0;x<width;++x) {
      switch(map[size_t(y)][size_t(x)]) {
      case '#':paint_text(first+y,left+x,"▪",0x8ba2ae);break;
      case '!':paint_text(first+y,left+x,"░",0xa26666);break;
      case '=':paint_text(first+y,left+x,"─",0x9dd4dd);break;
      }
    }
    auto mark=[&](double x,double y,const char *symbol,uint32_t color){
      paint_text(first+row(y),left+column(x),symbol,color,true);
    };
    for(size_t i=0;i<grid_.checkpoint_points.size();++i) {
      const auto [x,y]=grid_.checkpoint_points[i];
      mark(x+kPlayerWidth*.5,y,i==size_t(state_.checkpoint)?"◆":"◇",0x9dd4dd);
    }
    if(show_goal) {
      const bool open=(grid_.switch_flags&grid_.goal_requires)==grid_.goal_requires;
      mark(grid_.goal.x+grid_.goal.w*.5,grid_.goal.y+grid_.goal.h*.5,"▣",open?0xd7e8cd:0x7c8492);
    }
    for(const auto &node:grid_.interactions) {
      const bool board=settlement&&node.kind==InteractionKind::Board;
      if(board||node.kind==InteractionKind::Return)
        mark(node.area.x+node.area.w*.5,node.area.y+node.area.h*.5,board?"▤":"▣",0xd7e8cd);
    }
    mark(state_.player.x+kPlayerWidth*.5,state_.player.y+kPlayerHeight*.5,"@",0xffd68a);
    paint_text(top+height-1,2,settlement?tr("@ Ty · ◆ Lampa · ▤ Tablica","@ You · ◆ Lamp · ▤ Board"):
      tr("@ Ty · ◆ Lampa · ▣ Wyjście","@ You · ◆ Lamp · ▣ Exit"),0xc7d9df);
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
    std::string progress;
    if(campaign)progress=tr("Odcinek ","Leg ")+std::to_string(leg+1)+" / 3  ·  "+
      iceberg::route_title(v.district,2+2*leg).get(v.language);
    else if(v.mode==iceberg::Mode::Endless)progress=tr("Odcinek ","Leg ")+std::to_string(v.depth/room_count+1)+
      tr(" · Głębokość "," · Depth ")+std::to_string(v.depth);
    else progress=tr("Pokój ","Room ")+std::to_string(r.current+1)+" / "+std::to_string(r.rooms.size());
    clipped(top+1,5,progress,0xc7d9df);
    if(r.rooms.empty()) {
      auto lines=wrap(tr("Trasa zaczyna się za bramą szkoleniową. Tablica wypraw prowadzi dalej.","The route begins beyond the training gate. The expedition board leads onward."),cols_-10);
      int y=top+4;for(const auto &line:lines)clipped(y++,5,line,0xc7d9df);
    } else {
      const bool wide=cols_>=56;
      const int stride=wide?6:3,node_width=wide?4:2;
      const int left=std::max(3,(cols_-((room_count-1)*stride+node_width))/2),y=top+3;
      for(int n=0;n<room_count&&offset+n<int(r.rooms.size());++n) {
        const int idx=offset+n;
        const bool branch=r.branching&&n==2;
        const bool alt=branch&&(r.choices&(1u<<leg));
        const bool current=r.current==idx&&!alt;
        const bool visited=(r.visited&(1u<<idx))&&!alt;
        std::string number=wide?(idx+1<10?"0":"")+std::to_string(idx+1):std::to_string(n+1);
        const auto label=wide?(current?"<":visited?"[":"(")+number+(current?">":visited?"]":")"):
          (current?"▶":visited?"●":"○")+number;
        clipped(y,left+n*stride,label,current?0xffd68a:visited?0x91d4ce:0x607887);
        if(n+1<room_count&&idx+1<int(r.rooms.size()))clipped(y,left+n*stride+node_width,wide?"──":"─",0x607887);
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
    int row=top+8;
    for(const auto &line:wrap(current,cols_-9))clipped(row++,5,line,0xffd68a);
    if(r.current>=0&&r.current+1<int(r.rooms.size())) {
      const auto next=tr("Plan: ","Planned: ")+r.rooms[size_t(r.current+1)].get(v.language);
      for(const auto &line:wrap(next,cols_-9))clipped(row++,5,line,0xc7d9df);
    }
  }
