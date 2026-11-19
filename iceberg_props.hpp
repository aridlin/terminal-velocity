// Small environmental objects, drawn in the same cells as the architecture.
  void prop_art(double center_x,double feet_y,const std::vector<std::string> &lines,uint32_t color) {
    size_t width=0;for(const auto &line:lines)width=std::max(width,glyphs(line).size());
    world_art(center_x-width*scale_*.5,feet_y-lines.size()*aspect_*scale_,lines,color);
  }
  void token_art(double x,double y,uint32_t color) {
    prop_art(x,y+1.2,{" ▄▄▄ ","▐ ▪ ▌"," ▀▀▀ "},color);
    if(int(state_.elapsed*3+x)%9==0)world_text(x-.15,y-.8,"╱",0xf7dfa4);
  }
  void refill_art(double x,double y,bool empty) {
    prop_art(x,y+1.2,{"╭┴╮",empty?"│ │":"│▥│","╰─╯"},empty?0x426269:0x8ee0dd);
  }
  void valve_art(double x,double y,bool open) {
    prop_art(x,y,{" ╭┼╮ ",open?"═╡╋╞═":"═╡⊗╞═"," ╰┼╯ "},open?0xc7dfc9:iceberg::Districts[size_t(grid_.district)].accent);
  }
  void sign_art(double x,double y,int id,uint32_t color) {
    if(id==iceberg::CrownShaftPlate) {
      prop_art(x,y,{"╔═══╗","║•↑•║","╚═╤═╝","  ╵  "},0xb5cbd8);
      return;
    }
    if(id==iceberg::ArchiveTrolleyNotice) {
      prop_art(x,y,{"╔═══╗","║ ↗ ║","╚═╤═╝","  ╵  "},0xaee1df);
      return;
    }
    if(id==iceberg::ArchiveDryingLetter) {
      prop_art(x,y,{"┌────┐","│╲  ╱│","└─╳──┘","  ╵   "},0xf2d0a4);
      return;
    }
    if(id==iceberg::IrrigationCallBox) {
      prop_art(x,y,{"◀╗ "," ╚╗","  ║"," ═╩"},0xd4bc88);
      return;
    }
    if(id>=10&&id!=30){prop_art(x,y,{"╭──╮","│≋≋│","│≋ │","╰──╯"},color);return;}
    const char *face=id==30?"│JP2│":id==0?"│A D│":id==1?"│ X │":id==2?"│↑ X│":id==3?"│ ⇧ │":id==4?"│♨ !│":"│ ↑ │";
    if(id==2&&grid_.template_id==601)face="│≈ →│";
    prop_art(x,y,{"╭───╮",face,"╰─┬─╯","  │  "," ╱┴╲ "},color);
  }
  void task_art(const Interaction &node,bool done) {
    const auto &d=iceberg::Districts[size_t(grid_.district)];double x=node.area.x+node.area.w*.5,y=node.area.bottom();
    uint32_t color=done?d.stone:d.accent;
    switch(node.id) {
    case 0:prop_art(x,y,{" ╭▥╮ ","╭┴─┴╮","│╲ ╱│","╰───╯"},color);break;
    case 1:valve_art(x,y,done);break;
    case 2:{
      const auto &v=state_.adventure;
      bool warming=done&&v.machinery_kind==1&&v.machinery_left>.35&&std::abs(v.machinery_x-x)<2;
      bool lit=done&&(!warming||int(v.presentation_time*12)%2);
      prop_art(x,y,{"╭─╮",lit?"│○│":"│●│",lit?"│●│":"│○│","╰┬╯"," │ "},warming?0xe4ad68:lit?0xb8dfb7:color);break;
    }
    case 3:prop_art(x,y,{"╔═══╗","║ "+std::to_string(11+node.detail%3)+"║","╚═╤═╝"},color);break;
    case 4:prop_art(x,y,{"┌┄┐ ","│≋└┐","│≋≋│","└──┘"},color);break;
    case 5:prop_art(x,y,{" ╭╮ ","╭┤├╮","│❧ │","╰──╯"},color);break;
    case 7:prop_art(x,y,{"╭──╮","│▦▦│","│▦▦│",done?"│ ●│":"│ ○│","╰──╯"},color);break;
    default:prop_art(x,y,{"╭──╮","│╳ │","╰──╯"},color);break;
    }
  }
