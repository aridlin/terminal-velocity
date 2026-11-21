#define TERMINAL_VELOCITY_NO_TUI_MAIN
#include "../main.cpp"
#include "match.hpp"
// Pipe protocol is local to the gateway. Each outgoing line is slot + hex frame.
int main(int argc,char **argv) {
  const bool coop=argc>1&&std::string(argv[1])=="coop";
  const uint32_t seed=argc>2?uint32_t(std::stoul(argv[2])):1;
  const std::string code=argc>3?argv[3]:"local";
  party::Match match(coop?party::Mode::Coop:party::Mode::Race,seed);
  std::array<Controls,2> held{};
  std::array<int,2> masks{},pressed{};
  unsigned frames=0;
  std::array<int,2> old_deaths{},old_pickups{},old_checkpoint{},old_progress{};
  std::array<uint64_t,2> old_switches{};
  std::array<bool,2> old_grounded{true,true},old_dash{};
  std::string line;
  while(std::getline(std::cin,line)) {
    if(line.size()>128)continue;
    std::istringstream input(line);char op;int slot=0,a=0,b=0,c=0,d=0;input>>op;
    if(op=='C'&&input>>slot>>a&&slot>=0&&slot<2){match.connected[slot]=bool(a);masks[slot]=pressed[slot]=0;continue;}
    if(op=='I') {
      if(!(input>>slot>>a>>b>>c>>d)||slot<0||slot>1)continue;
      pressed[slot]|=a&~masks[slot];masks[slot]=a&0x3ffff;
      auto &p=*match.players[slot];p.last_columns=std::clamp(b,34,160);p.last_rows=std::clamp(c,24,60);
      p.profile.language=d?iceberg::Language::English:iceberg::Language::Polish;
      continue;
    }
    if(op!='T')continue;
    for(int i=0;i<2;++i) {
      const int h=masks[i],p=pressed[i];auto &c=held[i];c={};
      c.move=int(bool(h&2))-int(bool(h&1));c.aim_y=int(bool(h&8))-int(bool(h&4));
      c.jump_held=h&(16|4);c.jump_pressed=p&(16|4);c.dash_pressed=p&32;
      c.fast_fall=h&8;c.interact=p&64;c.inspect=p&128;c.confirm=p&256;
      c.pause=p&512;c.journal=p&1024;c.map=p&2048;c.restart=p&8192;
      c.navigate=int(bool(p&8))-int(bool(p&4));
      pressed[i]=0;
    }
    match.step(held);
    if(++frames%4)continue; // 30 rendered frames, fixed 120 Hz simulation.
    for(int slot=0;slot<2;++slot)if(match.connected[slot]) {
      auto &p=*match.players[slot];
      const int cols=p.last_columns,rows=p.last_rows;
      CellFrame frame;
      cell_frame_sink=[&](const CellFrame &f){frame=f;};
      RenderGrid(p.grid,p.state,rows,cols,2).draw();cell_frame_sink={};
      // Render the other courier as a solid, colored silhouette, including co-op waiting.
      if(match.same_room()&&!p.state.adventure.map&&!p.state.adventure.inspecting&&!p.state.journal) {
        const auto &other=match.players[1-slot]->state.player;
        int x=int(std::floor((other.x-p.state.camera_x)/.4));
        int y=4+int(std::floor((other.y-p.state.camera_y)/.8));
        
        for(int dy=0;dy<3;++dy)for(int dx=0;dx<3;++dx)if(y+dy>=4&&y+dy<rows-3&&x+dx>=0&&x+dx<cols){
          static const std::string pixels[3][3]={{"▄","█","▄"},{"█","█","█"},{"▌"," ","▐"}};
          frame[y+dy][x+dx].glyph=pixels[dy][dx];frame[y+dy][x+dx].rgb=slot?0xffb78e:0x81ddd8;
        }
      }
      auto label=[&](int row,const std::string &text){
        for(auto &cell:frame[row]){cell.glyph=" ";cell.background=0x101923;}
        int col=1;
        for(size_t n=0;n<text.size()&&col<cols-1;) {
          const unsigned char b=text[n];const size_t length=b<128?1:b<224?2:b<240?3:4;
          frame[row][col].glyph=text.substr(n,length);frame[row][col].rgb=0xffd68a;frame[row][col].bold=true;n+=length;++col;
        }
      };
      label(0,"ROOM / POKÓJ "+code+" · P"+std::to_string(slot+1));
      label(1,match.status(slot));
      if(match.started)label(2,p.profile.language==iceberg::Language::English?"Eight rooms across districts · P: personal pause · M: leave":"Osiem pokoi dzielnic · P: osobista pauza · M: wyjdź");
      if(!match.started)label(2,"ENTER: ready · 8 rooms · "+std::string(coop?"Shared switches, pickups and exits":"First courier to finish wins"));
      unsigned cues=0;
      if(p.state.deaths>old_deaths[slot])cues|=1u<<3;
      if(p.state.sweets_count()>old_pickups[slot])cues|=1u<<2;
      if(p.state.checkpoint>old_checkpoint[slot])cues|=1u<<4;
      if(!p.state.player.grounded&&old_grounded[slot]&&p.state.player.vy<0)cues|=1u;
      if(p.state.player.grounded&&!old_grounded[slot])cues|=1u<<5;
      const bool dashing=p.state.player.dash_time_left>0;
      if(dashing&&!old_dash[slot])cues|=1u<<1;
      if(match.progress[slot]!=old_progress[slot])cues|=1u<<6;
      if(frames%48==0&&p.state.player.grounded&&std::abs(p.state.player.vx)>1)cues|=1u<<7;
      if(p.grid.switch_flags&~old_switches[slot])cues|=1u<<8;
      old_switches[slot]=p.grid.switch_flags;
      old_deaths[slot]=p.state.deaths;old_pickups[slot]=p.state.sweets_count();old_checkpoint[slot]=p.state.checkpoint;
      old_grounded[slot]=p.state.player.grounded;old_dash[slot]=dashing;old_progress[slot]=match.progress[slot];
      const unsigned metadata=(cues<<1)|(unsigned(p.grid.district)<<24);
      std::string bytes;
      auto put=[&](uint32_t value){for(int n=0;n<4;++n){bytes+=char(value&255);value>>=8;}};
      put(cols);put(rows);
      bool first_cell=true;
      for(auto &row:frame)for(auto &cell:row) {
        auto &s=cell.glyph;uint32_t cp=s.empty()?32:static_cast<unsigned char>(s[0]);
        if(cp>=128){const int n=cp<224?2:cp<240?3:4;cp&=(1u<<(7-n))-1;for(int j=1;j<n&&j<int(s.size());++j)cp=(cp<<6)|(static_cast<unsigned char>(s[j])&63);}
        put(cp);put(cell_color(cell));put(cell_background(cell));put((cell.bold?1u:0u)|(first_cell?metadata:0));first_cell=false;
      }
      const char *hex="0123456789abcdef";std::cout<<slot<<' ';
      for(unsigned char b:bytes)std::cout<<hex[b>>4]<<hex[b&15];
      std::cout<<'\n';
    }
    std::cout.flush();
  }
}
