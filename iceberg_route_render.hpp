// Included inside RenderGrid. Cells represent rooms and real saved graph edges.
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
