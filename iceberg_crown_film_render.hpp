  void film_art(int top,int left,const std::vector<std::string> &art,uint32_t color,int stretch=1) {
    for(const auto &row:art) {
      int x=left;
      for(const auto &glyph:glyphs(row)) {
        if(glyph!=" ")for(int i=0;i<stretch;++i)paint_text(top,x+i,glyph,color);
        x+=stretch;
      }
      ++top;
    }
  }

  void film_fill(int top,int bottom,uint32_t background) {
    for(int y=top;y<bottom;++y)for(int x=0;x<cols_;++x)
      put(y,x,{" ",Color::Default,false,false,0,0,background});
  }

  void film_skyline(int top,int base,double time,bool daylight) {
    const int pan=int(14.*(1.-std::exp(-time*.11)));
    const uint32_t sky=daylight?0x52647a:0x10283d;
    film_fill(top,base,sky);
    if(daylight) {
      film_art(top+1,cols_*3/4-6-pan/2,{" ▄██▄ ","██████"," ▀██▀ "},0xffdc96,2);
    }
    for(int layer=0;layer<2;++layer) {
      int pitch=layer?12:8;
      int ground=base-(layer?0:2);
      uint32_t wall=layer?0x324960:0x47627d;
      for(int block=-1;block<=cols_/pitch+3;++block) {
        int x=block*pitch-pan*(layer+1)/2;
        int height=2+std::abs(block*17+layer*13)%std::max(3,ground-top-2);
        int roof=ground-height;
        for(int y=roof;y<ground;++y)for(int dx=0;dx<pitch-2;++dx) {
          if(y<top)continue;
          bool lit=y>roof&&(y-roof)%2==1&&dx%3==1&&
            (daylight||((block+dx+y+int(time*.8))%4==0));
          put(y,x+dx,{y==roof?"▄":lit?"▪":" ",Color::Decor,false,false,0,
            lit?0xf3ca83u:0x638099u,wall});
        }
      }
    }
    for(int i=0;i<cols_/3;++i) {
      const int x=(i*41+int(time*3))%cols_;
      const int y=top+(i*11+int(time*.8))%std::max(1,base-top);
      if(buffer_[y][x].background==sky)paint_text(y,x,i%3?"·":"✦",0xc1d7dd);
    }
  }

  void film_quay(int top,int bottom,double time) {
    const int water=bottom-3;
    film_skyline(top,water,time,false);
    film_fill(water,bottom,0x173e53);
    for(int y=water;y<bottom;++y)for(int x=0;x<cols_;++x)
      if((x+y*7+int(time*(2+y-water)))%12<5)paint_text(y,x,"─",y==water?0x82b6bf:0x467c92);
    const int boat=3+int(std::min(time*1.7,cols_*.32));
    film_art(water-4,boat,{"         ╷","     ╭───┴────╮","     │▪ ▪ ▪ ▪ │",
      "╲════╧════════╧═════╱"," ╲█████████████████╱"},0xb7cbd2);
    film_art(water-3,cols_-15,{"  ┌─────┐","  │ ▪ ▪ │","══╧═════╧════","  ╎     ╎"},0xb18d6d);
  }

  void film_stairwell(int top,int bottom,double time) {
    film_fill(top,bottom,0x202a37);
    const int pan=int(5.*(1.-std::exp(-time*.15)));
    for(int y=top+1;y<bottom;++y) {
      const int x=cols_/2+(y-top)/2-pan;
      paint_text(y,x,"┃",0x7995a5);
      paint_text(y,x-16,"│",0x4d6877);
      paint_text(y,x+18,"┃",0x7995a5);
      if((y-top)%4==1)film_art(y,x-15,{"╭───┐ ▥ ╭────────────╮","│ ▪ │   ╰─┐    ┌─────╯","└───┘     ╰────╯"},0x8b9baa);
      if(y%2)paint_text(y,cols_-5,"╎",0x608ba2);
    }
    const int arrow=int(time*4)%std::max(1,bottom-top-1);
    paint_text(top+arrow,cols_-5,"◆",0x9dd9e4);
    film_art(top+1,2,{"╔═════╗","║  11 ║","╚═════╝"},0xe4ad68);
  }

  void film_chamber(int top,int bottom,double time) {
    film_fill(top,bottom,0x253344);
    const int x=cols_/2-16-int(std::min(time*1.3,5.));
    for(int y=top+1;y<bottom;++y) {
      paint_text(y,x-3,"║",0x475d72);
      paint_text(y,x+34,"║",0x475d72);
      if((y-top)%4==0)paint_text(y,x-8,"╞════╡",0x475d72);
    }
    film_art(top+(bottom-top-8)/2,x,{"   ╔════════╤══════════╗",
      "═══╣ ▥ ▥    │    ▥ ▥   ╠═══","   ║        ╵          ║",
      "   ║  ◉    ╔═══╗    ◉  ║","   ║       ║ ▤ ║       ║",
      "   ╚═════╤═╩═╤═╩═╤═════╝","        ╱╵   │   ╵╲ ",
      "             ╵"},0xb7c5d1);
    if(time<2.4&&int(time*12)%4!=0)paint_text(top+(bottom-top-8)/2+6,x+17,"✶",0xffc277);
  }

  void film_hearing(int top,int bottom,double time,int speaker) {
    film_fill(top,bottom,0x3a303b);
    int x=cols_/2;
    paint_text(top,x-12,"╔══════════════════════╗",0x78617c);
    for(int y=top+1;y<bottom-3;++y) {
      paint_text(y,x-12,"║",0x78617c);
      paint_text(y,x+11,"║",0x78617c);
      if((y-top)%3==1)paint_text(y,x-9,"▤ ▤ ▤ ▤ ▤ ▤ ▤ ▤ ▤",0x68506b);
    }
    for(int actor:{1,3,4,6}) {
      const int at=3+(actor==1?0:actor==3?1:actor==4?2:3)*(cols_-10)/3;
      const uint32_t color=actor==speaker?iceberg::Npcs[size_t(actor)].color:0x8e7e89;
      film_art(bottom-6,at,{" ▄▄ "," ▀█ ","▄██▄"},color);
    }
    film_fill(bottom-3,bottom,0x594443);
    for(int i=3;i<cols_-3;i+=7)
      paint_text(bottom-3,i,(i+int(time*2))%4?"▤":"▥",0xc2a489);
  }

  void film_actor(int top,int bottom,int speaker,double time,bool speaking) {
    static const std::array<const char *,8> courier={
      "     ▄████▄     ","   ▄██▀▀▀██▄    ","   ██ ▪▪ ▐██    ","   ▐█   ▵ ▐█    ",
      "    ▀█▄▄▄█▀     ","    ═══╗██▄     ","  ▄█▣══╩████▄   ","  ██ ║ ████ █   "};
    if(speaker<0&&speaker!=iceberg::CourierSpeaker)return;
    const bool player=speaker==iceberg::CourierSpeaker;
    const auto &npc=iceberg::Npcs[size_t(player?6:speaker)];
    const auto &portrait=player?courier:npc.portrait;
    const int stretch=cols_>=88?2:1;
    const int right=cols_-17*stretch;
    const int left=speaker%2==0?right:1;
    const int y=std::max(top,bottom-9)-int(time<.3);
    std::vector<std::string> face(portrait.begin(),portrait.end());
    if(speaking&&int(time*16)%3==0) {
      if(speaker==7)face[2]="   │ │ ▓▓ │ │   ";
      else face[4]="   ▀█▄ ▪▪ ▄█▀   ";
    }
    film_art(y,left,face,npc.color,stretch);
  }

  void draw_crown_film() {
    using iceberg::CrownShot;
    const auto &dialogue=state_.adventure.dialogue;
    const auto &line=dialogue.lines[dialogue.page];
    const auto layout=iceberg::dialogue_layout(line.text.get(state_.adventure.language),cols_,rows_);
    const int first=dialogue.segment*layout.text_rows;
    const int count=std::min(layout.text_rows,int(layout.body.size())-first);
    const int end=dialogue.choosing()?std::max(8,rows_-10):rows_-5-count;
    const int top=1;
    const double time=dialogue.shot_seconds;
    film_fill(0,rows_,0x0b111a);
    switch(line.shot) {
      case CrownShot::Chamber:film_chamber(top,end,time);break;
      case CrownShot::Stairwell:film_stairwell(top,end,time);break;
      case CrownShot::Hearing:film_hearing(top,end,time,line.speaker);break;
      case CrownShot::Quay:film_quay(top,end,time);break;
      case CrownShot::Daybreak: {
        film_skyline(top,end,time,true);
        const int gate=2;
        const int lift=int(std::min(5.,time*2.5));
        film_art(top+1,gate,{"╔══╤════════╤══╗","║  ┃        ┃  ║",
          "║  ┃        ┃  ║","║  ┃        ┃  ║","╨──╨────────╨──╨"},0xf1cf92);
        for(int y=top+2;y<top+6-lift;++y)paint_text(y,gate+4,"▓▓▓▓▓▓▓▓",0xa0aeb9);
        if(lift>=4)for(int x=gate+13;x<cols_;++x)
          paint_text(top+3+(x-gate)/15,x,x%5?"─":"✦",0xffdb8a);
        break;
      }
    }
    if(end-top>=8)
      film_actor(top,end,line.speaker,time,dialogue.revealing);
    // A cut rolls in from black over a few cells while the camera settles.
    const int shade=std::max(0,int((.26-time)*(end-top)*1.9));
    film_fill(0,top+shade,0x0b111a);
    film_fill(end-shade,rows_,0x0b111a);
    if(dialogue.choosing()){draw_dialogue_replies(dialogue,end);return;}
    const bool is_courier=line.speaker==iceberg::CourierSpeaker;
    const auto &npc=iceberg::Npcs[size_t(is_courier?6:std::max(0,line.speaker))];
    if(line.speaker>=0||is_courier)
      paint_text(end+1,3,is_courier?tr("KURIER","COURIER"):npc.name,npc.color,true);
    for(int row=0;row<count;++row)
      paint_text(end+2+row,3,layout.body[size_t(first+row)],0xf6e9d9);
    paint_text(rows_-1,3,state_.paused?tr("E / ENTER wznów","E / ENTER resume"):
      tr("E / ENTER dalej · P pauza","E / ENTER continue · P pause"),0x879bab);
  }
