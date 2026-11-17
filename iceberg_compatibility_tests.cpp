#define TERMINAL_VELOCITY_NO_TUI_MAIN
#include "main.cpp"
#include <iomanip>
// Crown d75523cf17d3: geometry and 4,608 seeded itineraries from released layout versions.
// Deliberately excludes new IDs and rendering: old checkpoint positions must remain valid.
int main() {
  uint64_t hash=1469598103934665603ull;
  auto add=[&](auto value){std::ostringstream out;out<<std::setprecision(17)<<value<<';';for(unsigned char c:out.str()){hash^=c;hash*=1099511628211ull;}};
  auto rect=[&](Rect r){add(r.x);add(r.y);add(r.w);add(r.h);};
  auto grid=[&](const EngineGrid &g){
    add(g.width());add(g.height());add(g.template_id);add(g.spawn_x);add(g.spawn_y);rect(g.goal);
    for(int y=0;y<g.height();++y)for(int x=0;x<g.width();++x){add(g.solid_at(x,y));add(g.hazard_at(x,y));}
    for(auto p:g.route_points){add(p.first);add(p.second);}for(auto p:g.checkpoint_points){add(p.first);add(p.second);}
    for(auto p:g.sweets){add(p.first);add(p.second);}for(auto p:g.refills){add(p.first);add(p.second);}
    for(auto p:g.platforms){rect(p.base);add(p.dx);add(p.dy);add(p.period);add(p.phase);}
    for(auto node:g.interactions){rect(node.area);add(int(node.kind));add(node.id);add(node.detail);}
    add(g.goal_requires);add(g.dash_lock);add(g.dash_unlock);
    for(auto gate:g.gates){rect(gate.area);add(gate.flag);}
  };
  for(unsigned version:{1u,2u,3u})for(int district=0;district<6;++district)for(unsigned seed=0;seed<256;++seed)
    for(int id:iceworld::make_itinerary(seed,district,24,version))add(id);
  for(const auto &p:iceworld::patterns())if(p.id<500)for(unsigned seed:{1u,73u,911u})for(bool expert:{false,true})
    grid(iceworld::build_room(p.id,seed,iceworld::district_of(p),expert));
  for(int district=0;district<6;++district){grid(iceworld::build_settlement(district));grid(iceworld::build_authored(district,false));grid(iceworld::build_authored(district,true));}
  for(int mission=0;mission<3;++mission)grid(iceworld::build_harbor_mission(mission));
  grid(iceworld::build_epilogue());
  if(hash!=0x615d70b5bdf48a23ull){std::cerr<<"FAIL released layout fingerprint changed: "<<std::hex<<hash<<'\n';return 1;}
  std::cout<<"PASS released v1-v3 itinerary, collision, checkpoint, objective and interaction fingerprint\n";
}
