// RenderGrid members. Reactions are cosmetic, bounded and deterministic.
  void draw_machinery_reaction() {
    const auto &v=state_.adventure;if(v.machinery_left<=0)return;
    const auto &d=iceberg::Districts[size_t(grid_.district)];
    double time=v.machinery_duration-v.machinery_left;
    double x=v.machinery_x,y=v.machinery_y;
    if(v.machinery_kind==2) {
      // The restored crownward tram crosses the overhead rail, shedding frost
      // behind its wheels. The route remains visible beneath its silhouette.
      double rail=y-10,car=x-29+time*time*15;
      std::string track;for(int i=0;i<int(100/scale_);++i)track+="═";
      world_text(x-35,rail+aspect_*scale_,track,d.stone);
      world_art(car,rail-3*aspect_*scale_,{"      ╱╲                ","╭─────┴┴────────────────╮","│ ▯ ▯ ▯ ▯  ║  ▯ ▯ ▯ ▯  │","╰──◉───────╨───────◉───╯"},d.light);
      for(int i=0;i<34;++i) {
        double born=.12+i*.055,age=time-born;
        if(age<0||age>1.15)continue;
        double origin=x-29+born*born*15+3+(i%7)*1.1;
        world_text(origin+(i%3-1)*age*1.8,rail+aspect_*scale_+age*age*5.2,i%4?"·":"⠂",i%3?0x938c96:0xc5bec6);
      }
      if(time<1.6)world_text(car+7*scale_,rail-4*aspect_*scale_,int(time*22)%2?"╲✦╱":" ·✧ ",0xf7d693);
    } else {
      int phase=int(time*14)%4;
      const char *wheel[]={"╲│╱","─◉─","╱│╲","╶⊕╴"};
      world_text(x-1,y-1.7,wheel[phase],0xe8d9ac);
      // A pipe wakes up along the local ceiling; no unexplained camera shake.
      for(int i=0;i<12;++i) {
        double age=time-i*.035;if(age<0||age>.8)continue;
        world_text(x-5+i*.9,y-6+age*age*5,i%3?"·":"⠄",0x8d9098);
      }
      if(v.machinery_kind==1&&time<.55)for(int i=0;i<7;++i) {
        double angle=i*1.15+time*2;
        world_text(x+std::cos(angle)*time*7,y-2+std::sin(angle)*time*4+time*time*3,i%2?"╴":"·",0xf7ce85);
      }
    }
  }
  void draw_room_shutter() {
    const auto &v=state_.adventure;
    if(v.door_left<=0||state_.menu||state_.paused||v.inspecting||v.dialogue.active())return;
    double progress=1-std::clamp(v.door_left/.42,0.,1.);
    double eased=progress*progress*(3-2*progress);
    int covered=int(cols_*.5*(1-eased));
    const auto &d=iceberg::Districts[size_t(grid_.district)];
    for(int y=4;y<rows_-3;++y)for(int x=0;x<covered;++x)for(int side=0;side<2;++side) {
      int col=side?cols_-1-x:x;Cell cell;
      cell.glyph=x==covered-1?(side?"╠":"╣"):y%3==0?"─":" ";
      cell.rgb=d.stone;cell.background=0x11151c;put(y,col,std::move(cell));
    }
  }
