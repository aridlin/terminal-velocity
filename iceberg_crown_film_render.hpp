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

  void film_person(int top,int floor,int x,int actor,double time,bool speaking,bool faces_left=false) {
    const auto &npc=iceberg::Npcs[size_t(actor==iceberg::CourierSpeaker?6:actor)];
    const int nod=speaking&&std::fmod(time,1.9)<.2;
    film_art(std::max(top,floor-6)+nod,x,
      {"  ▄██▄  ",faces_left?" ▐▪███▌ ":" ▐███▪▌ ",
       speaking&&int(time*12)%3?"  ▀▄▂▀  ":"  ▀▄▄▀  ",
       " ▄█▣██▄ "," ██████ ","  ██ ██ "},npc.color);
  }

  void film_stairwell(int top,int bottom,double time) {
    film_fill(top,bottom,0x192634);
    const int depth=bottom-top;
    const int pan=int(16.*(1.-std::exp(-time*.09)));
    const int shaft=cols_/2+2-pan/3;
    for(int y=top;y<bottom;++y) {
      const int level=(y-top+pan)%6;
      paint_text(y,shaft-12,"┃",0x627d8b);
      paint_text(y,shaft+12,"┃",0x627d8b);
      if(level==0||level==3) {
        const int step=level==0?shaft-11:shaft+1;
        paint_text(y,step,"═══════════",0x658895);
        paint_text(y,level==0?shaft+14:shaft-21,"╞══════╡",0x5a6d82);
      }
      if(level==2||level==5)paint_text(y,shaft-4+level,"╱╱╱╱╱",0x647887);
    }
    const int water=bottom-1-int(depth*.25*std::min(1.,time/15.));
    film_fill(water,bottom,0x154b63);
    for(int y=water;y<bottom;++y)for(int x=(y*7+int(time*5))%6;x<cols_;x+=6)
      paint_text(y,x,y==water?"≈──":"─",y==water?0x80cfd7:0x418da0);
    film_art(top+1,2,{"┌─────┐","│  11 │","└─────┘"},0xe4ad68);
    const int lamp=shaft+17;
    paint_text(top+2,lamp,"◆",std::fmod(time,.9)<.55?0xe78868:0x775665);
    const int witness_y=std::max(top+4,water-2-int(time/8.));
    film_art(witness_y-3,shaft-20,{" ▄█▄ ","▄███▄"," ▐ ▌ "},0xaec0c4);
    film_art(witness_y-2,shaft+15,{" ▄█▄ ","╶███▄"," ▐ ▌ "},0xd4ad7e);
  }

  void film_chamber(int top,int bottom,double time,bool speaking) {
    film_fill(top,bottom,0x253344);
    const int x=cols_/2-15-int(std::min(time*1.3,5.));
    const int center=std::max(top+2,top+(bottom-top-9)/2);
    for(int y=top+1;y<bottom;++y) {
      paint_text(y,x-3,"║",0x475d72);
      paint_text(y,x+34,"║",0x475d72);
      if((y-top)%4==0)paint_text(y,x-8,"╞════╡",0x475d72);
    }
    film_art(center,x,{"   ╔════════╤══════════╗",
      "═══╣ ▥ ▥    │    ▥ ▥   ╠═══","   ║        ╵          ║",
      "   ║  ◉    ╔═══╗    ◉  ║","   ║       ║ ▤ ║       ║",
      "   ╚═════╤═╩═╤═╩═╤═════╝","        ╱╵   │   ╵╲ ",
      "             ╵"},0xb7c5d1);
    if(time<1.1&&int(time*14)%3)paint_text(center+3,x+6,"◉",0xf18471);
    if(time>.65) {
      const int length=std::min(6,int((time-.65)*5));
      for(int row=0;row<length&&center+5+row<bottom;++row)
        paint_text(center+5+row,x+12,row+1==length?"╰────╯":"│ ≡≡ │",0xf1dcb6);
    }
    if(time<2.4&&int(time*12)%4!=0)paint_text(center+6,x+17,"✶",0xffc277);
    if(speaking)paint_text(center+3,x+21,int(time*13)%3?"◉))":"◉)",0xf3c68e);
    film_person(top,bottom-1,x-10+std::min(8,int(time*4)),iceberg::CourierSpeaker,time,false);
  }

  void film_hearing(int top,int bottom,double time,int speaker,double line_time,bool speaking) {
    film_fill(top,bottom,0x282938);
    const int mid=cols_/2;
    const int window=std::min(40,cols_-8);
    const int left=mid-window/2;
    const int frame_bottom=std::max(top+3,bottom-7);
    film_fill(top+1,frame_bottom,0x36475c);
    for(int y=top+1;y<frame_bottom;++y) {
      paint_text(y,left-1,"┃",0x8c8396);
      paint_text(y,mid,"┃",0x8c8396);
      paint_text(y,left+window,"┃",0x8c8396);
      for(int x=left+2;x<left+window;x+=4) {
        if((x*3+y*5)%11<3)paint_text(y,x,"▪",0xd7aa7a);
      }
    }
    const int floor=bottom-3;
    const int active=speaker>=0&&speaker<7?speaker:4;
    const int entrance=std::min(5,int(line_time*10));
    film_person(top,floor,mid-4+entrance-5,active,line_time,speaking,true);
    film_person(top,floor,mid-cols_/4-4,active==4?3:4,time,false);
    film_person(top,floor,mid+cols_/4-5,active==6?1:6,time,false,true);
    film_fill(floor-1,bottom,0x514444);
    for(int x=1;x<cols_;x+=6)paint_text(floor-1,x,"━━━━━━",0xae8569);
    const int hand=mid+(active%2?5:-7);
    paint_text(floor-2,hand,speaking&&int(line_time*3)%3==0?"╲▤":"▤",0xefcfab);
    for(int x:{4,cols_-11})film_art(bottom-3,x,{" ▄██▄ ","▄████▄","██████"},0x263040);
  }

  void film_lookout(int top,int bottom,double time) {
    film_skyline(top,bottom-2,time,true);
    for(int x=0;x<cols_;++x) {
      paint_text(bottom-2,x,x%6==0?"╨":"═",0x889c9c);
      paint_text(bottom-1,x,x%6==0?"║":" ",0x708490);
    }
    const int gate=cols_/9;
    const int lift=int(5.*std::clamp((time-.65)/iceberg::CrownAirlockOpeningSeconds,0.,1.));
    film_art(top+1,gate,{"╔══╤════════╤══╗","║  ┃        ┃  ║",
      "║  ┃        ┃  ║","║  ┃        ┃  ║","╨──╨────────╨──╨"},0xf1cf92);
    for(int y=top+2;y<top+6-lift;++y)paint_text(y,gate+4,"▓▓▓▓▓▓▓▓",0xa0aeb9);
    if(lift>=4)for(int x=gate+13;x<cols_;++x)
      paint_text(std::min(bottom-3,top+3+(x-gate)/15),x,x%5?"─":"✦",0xffdb8a);
  }

  void film_conversation(int top,int bottom,const iceberg::DialogueLine &line,double time,bool speaking) {
    const int other=line.speaker==iceberg::CourierSpeaker?4:iceberg::CourierSpeaker;
    const int center=cols_/2;
    film_person(top,bottom-2,center-12,line.speaker,time,speaking);
    film_person(top,bottom-2,center+6,other,time,false,true);
  }

  void draw_crown_film() {
    using iceberg::CrownShot;
    const auto &dialogue=state_.adventure.dialogue;
    const auto &line=dialogue.lines[dialogue.page];
    const auto layout=iceberg::dialogue_layout(line.text.get(state_.adventure.language),cols_,rows_);
    const int first=dialogue.segment*layout.text_rows;
    const int count=std::max(0,std::min(layout.text_rows,int(layout.body.size())-first));
    const bool silent=line.speaker==-1&&count==0;
    const int end=dialogue.choosing()?std::max(8,rows_-10):
      silent?rows_-2:std::max(8,rows_-4-count);
    const int top=1;
    const double time=dialogue.shot_seconds;
    film_fill(0,rows_,0x0b111a);
    switch(line.shot) {
      case CrownShot::Chamber:film_chamber(top,end,time,!silent&&dialogue.revealing);break;
      case CrownShot::Stairwell:film_stairwell(top,end,time);break;
      case CrownShot::Hearing:
        film_hearing(top,end,time,line.speaker,dialogue.line_seconds,dialogue.revealing);break;
      case CrownShot::Quay:
        film_quay(top,end,time);
        film_person(top,end-3,3,0,dialogue.line_seconds,dialogue.revealing);
        paint_text(end-5,12,"╭─╿╮",0x90a4b1);
        paint_text(end-4,12,"╰▓▓╯",0x90a4b1);
        if(dialogue.revealing)paint_text(end-5,16,int(time*9)%2?"〉":"〰",0xe9bd81);
        break;
      case CrownShot::Daybreak:
        film_lookout(top,end,time);
        if(silent) {
          const int arm=cols_/9+2;
          film_person(top,end-2,arm-4,iceberg::CourierSpeaker,time,false);
          paint_text(end-6,arm+3,time<.8?"─┐":"╲│",0xd1ccb4);
        } else if(!dialogue.choosing())film_conversation(top,end,line,dialogue.line_seconds,dialogue.revealing);
        break;
    }
    // A cut rolls in from black over a few cells while the camera settles.
    const int shade=std::max(0,int((.26-time)*(end-top)*1.9));
    film_fill(0,top+shade,0x0b111a);
    film_fill(end-shade,rows_,0x0b111a);
    if(dialogue.choosing()){draw_dialogue_replies(dialogue,end);return;}
    const bool is_courier=line.speaker==iceberg::CourierSpeaker;
    const auto &npc=iceberg::Npcs[size_t(is_courier?6:std::max(0,line.speaker))];
    if(line.speaker>=0||is_courier)
      paint_text(end+1,3,is_courier?tr("KURIER","COURIER"):npc.name,npc.color,true);
    for(int row=0;row<count;++row) {
      const auto &subtitle=layout.body[size_t(first+row)];
      paint_text(end+2+row,std::max(3,(cols_-int(glyphs(subtitle).size()))/2),subtitle,0xf6e9d9);
    }
    paint_text(rows_-1,3,state_.paused?tr("E / ENTER wznów","E / ENTER resume"):
      tr("E / ENTER dalej · P pauza","E / ENTER continue · P pause"),0x879bab);
  }
