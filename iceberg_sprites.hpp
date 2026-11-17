// Small two-colour cell sprites. Half blocks give six pixel rows in three
// terminal rows, keeping the visible feet and body close to the collision box.
  void pixel_sprite(int row,int col,const std::vector<std::array<uint32_t,4>> &pixels) {
    for(size_t y=0;y+1<pixels.size();y+=2)for(int x=0;x<4;++x) {
      uint32_t top=pixels[y][size_t(x)],bottom=pixels[y+1][size_t(x)];
      if(!top&&!bottom)continue;
      int r=row+int(y/2),c=col+x;
      if(r<4||r>=rows_-3||c<0||c>=cols_)continue;
      uint32_t bg=buffer_[r][c].background?buffer_[r][c].background:iceberg_base_background();
      if(!top)put(r,c,{"▄",Color::Text,false,false,0,bottom,bg});
      else if(!bottom)put(r,c,{"▀",Color::Text,false,false,0,top,bg});
      else put(r,c,{"▀",Color::Text,false,false,0,top,bottom});
    }
  }
  void resident_sprite(int id,int x,int feet) {
    const auto &npc=iceberg::Npcs[size_t(id)];
    if(id==7) {
      pixel_sprite(feet-3,x,{{0,0x75888c,0x75888c,0},{0x75888c,0x172329,0xe4ad68,0x75888c},
        {0x75888c,0xabc0bf,0xabc0bf,0x75888c},{0x75888c,0x27373d,0x27373d,0x75888c},
        {0,0x75888c,0x75888c,0},{0,0x4e6268,0x4e6268,0}});return;
    }
    static const uint32_t hair[]={0x654b46,0xd79743,0x394a69,0x8b8990,0xd2c5a8,0x665544,0x433d49};
    static const uint32_t clothes[]={0x93717c,0x76838a,0x537b93,0x8f829b,0x698c8b,0x769768,0x5ba6ae};
    uint32_t h=hair[id],coat=clothes[id],skin=0xd9ac89,leg=0x55616f,boot=0xa3abb0;
    bool gesture=(state_.adventure.reacting_npc==id&&state_.adventure.npc_reaction_left>0)||int(state_.adventure.presentation_time*.65+id)%5==0;
    pixel_sprite(feet-3,x,{{0,h,h,0},{0,skin,h,0},
      {gesture?skin:0,coat,coat,0},{gesture?0:skin,coat,npc.color,skin},
      {0,leg,leg,0},{boot,0,boot,0}});
  }
  void courier_sprite(int x,int feet,uint32_t coat) {
    const auto &p=state_.player;const auto &v=state_.adventure;
    uint32_t skin=0xe0b394,bag=0x9b7864,boot=0xc1cbd0,leg=0x55616f;
    uint32_t body=p.dash_available&&!grid_.dash_locked()?0xf5cd79:0x83c5ce;
    std::vector<std::array<uint32_t,4>> pixels{{0,coat,coat,0},{0,skin,0x51474b,0},
      {bag,body,body,0},{bag,body,body,skin},{0,leg,leg,0},{boot,0,boot,0}};
    if(!p.grounded){pixels[4]={leg,leg,0,0};pixels[5]={boot,0,0,boot};}
    else if(std::abs(p.vx)>2) {
      if(int(state_.elapsed*14)%2){pixels[4]={0,leg,0,0};pixels[5]={0,boot,boot,0};}
      else {pixels[4]={0,0,leg,0};pixels[5]={boot,boot,0,0};}
    }
    if(v.pickup_left>.4&&p.grounded) {
      pixels={{0,0,0,0},{0,coat,coat,0},{bag,skin,body,0},{bag,body,body,skin},{0,leg,leg,skin},{boot,0,boot,0}};
    }
    if(p.facing<0)for(auto &row:pixels)std::reverse(row.begin(),row.end());
    pixel_sprite(feet-3,x,pixels);
    if(v.cosmetics&0x3f0000u)paint_text(feet-2,x+1,"✦",0xf3ce82);
  }
