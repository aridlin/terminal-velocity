#pragma once
// A reproducible ordinary-input pilot used for route verification. It never
// moves the player directly or disables collisions, damage, or gravity.
struct RoutePilot {
  size_t target = 1;
  int previous_deaths = 0;
  double dash_delay = 0;
  Controls controls(const EngineGrid &g,const GameState &s) {
    Controls c;
    auto &p=s.player;
    if(s.deaths!=previous_deaths) {
      previous_deaths=s.deaths;
      // Find the authored route waypoint immediately ahead of the checkpoint.
      double closest=1e30;size_t at=0;
      for(size_t i=0;i<g.route_points.size();++i) {
        auto [x,y]=g.route_points[i];double d=std::hypot(x-p.x,y-p.y-kPlayerHeight);
        if(d<closest){closest=d;at=i;}
      }
      target=std::min(at+1,g.route_points.size());
    }
    if(g.route_points.empty()){c.move=1;return c;}
    if(target<g.route_points.size()) {
      auto [x,y]=g.route_points[target];
      if(p.grounded&&std::abs(p.y+kPlayerHeight-y)<1.1&&std::abs(p.x+kPlayerWidth*.5-x)<4.5)++target;
    }
    double tx=g.goal.x+.5,ty=g.goal.bottom();
    if(target<g.route_points.size()){tx=g.route_points[target].first;ty=g.route_points[target].second;}
    double dx=tx-(p.x+kPlayerWidth*.5);
    c.move=dx>.4?1:dx<-.4?-1:0;
    c.jump_held=true;
    int foot=int(std::floor(p.y+kPlayerHeight+.1));
    double ahead=p.x+(c.move>0?kPlayerWidth+.25:-.25);
    bool gap=!g.solid_at(int(std::floor(ahead)),foot);
    bool rise=g.solid_at(int(std::floor(ahead)),foot-1);
    if(p.support>=0)gap=true;
    if(p.grounded&&target<g.route_points.size())
      c.jump_pressed=(gap||rise)&&std::abs(dx)>1;
    // Horizontal dash near the apex supplies range while preserving a clear
    // landing above the target platform. No airborne recharge is injected.
    if(!p.grounded&&p.dash_available&&p.dash_time_left<=0&&p.vy>=-5&&
       p.vy<8&&p.y+kPlayerHeight<ty-.25&&std::abs(dx)>9)
      c.dash_pressed=true;
    if(p.grounded)for(const auto &enemy:g.enemies) {
      auto r=enemy.at(s.elapsed);
      if(std::abs(r.y+1-p.y-kPlayerHeight)<2&&std::abs(r.x-p.x)<3)c.jump_pressed=true;
    }
    return c;
  }
};
struct RouteResult {bool complete=false;int deaths=0,ticks=0;size_t target=0;double x=0,y=0;};
RouteResult verify_route(const EngineGrid &g,int max_ticks=120*120) {
  auto state=new_state(g);FixedStepper stepper;RoutePilot pilot;
  int ticks=0;
  for(;ticks<max_ticks&&!state.won&&state.deaths<5;++ticks)
    stepper.advance(state,g,pilot.controls(g,state),1./120);
  return {state.won,state.deaths,ticks,pilot.target,state.player.x,state.player.y};
}

// Generation has a hard two-candidate bound. Failure falls back to a separately
// tested authored recovery route; death/resume rebuilds the same candidate.
inline EngineGrid verified_room(int id,uint32_t seed,int district,bool expert=false) {
  auto candidate=iceworld::build_room(id,seed,district,expert);
  if(verify_route(candidate).complete)return candidate;
  candidate=iceworld::build_room(id,seed,district,expert,true);
  if(verify_route(candidate).complete)return candidate;
  return iceworld::build_room(7,seed,district,false,true);
}
