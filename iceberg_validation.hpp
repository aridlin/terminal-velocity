#pragma once
// A reproducible ordinary-input pilot used for route verification. It never
// moves the player directly or disables collisions, damage, or gravity.
struct RoutePilot {
  size_t target = 1;
  int previous_deaths = 0;
  double dash_delay = 0;
  bool predictive=false;
  bool launched=false;
  double takeoff_y=0;
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
    if(p.support>=0) {
      auto platform=g.platforms[size_t(p.support)].at(s.elapsed);
      gap=platform.w>=12?(c.move>0?p.x+kPlayerWidth>=platform.right()-.3:p.x<=platform.x+.3):true;
    }
    if(p.grounded&&target<g.route_points.size())
      c.jump_pressed=(gap||rise)&&std::abs(dx)>1;
    if(p.grounded){launched=c.jump_pressed;takeoff_y=p.y+kPlayerHeight;}
    if(!p.grounded&&launched&&takeoff_y-ty>3.6&&p.dash_available&&p.dash_time_left<=0&&p.vy>=-7&&p.vy<0&&
       p.y+kPlayerHeight>ty-.5&&std::abs(dx)>2) {
      c.dash_pressed=true;c.aim_y=-1;
    }
    // Predict whether the leading foot can reach the actual landing edge.
    // Using the centre distance alone fails when a vent forces an early jump.
    double landing_left=tx-2,landing_right=tx+2;
    int landing_y=int(std::floor(ty));
    if(g.solid_at(int(std::floor(tx)),landing_y)) {
      int left=int(std::floor(tx)),right=left;
      while(left>0&&g.solid_at(left-1,landing_y))--left;
      while(right<g.width()&&g.solid_at(right,landing_y))++right;
      landing_left=left;landing_right=right;
    } else for(const auto &platform:g.platforms) {
      auto r=platform.at(s.elapsed);
      if(std::abs(r.y-ty)<1.5&&tx>=r.x-1&&tx<=r.right()+1){landing_left=r.x;landing_right=r.right();break;}
    }
    double drop=ty-p.y-kPlayerHeight;
    double flight=drop>0?(std::sqrt(p.vy*p.vy+2*kGravity*drop)-p.vy)/kGravity+.06:0;
    double edge_distance=dx>=0?landing_left-p.x-kPlayerWidth:p.x-landing_right;
    if(p.grounded&&p.support>=0) {
      const auto &platform=g.platforms[size_t(p.support)];auto r=platform.at(s.elapsed);
      double deck_left=c.move>0?r.right()-p.x-kPlayerWidth:p.x-r.x;
      if(std::abs(platform.dx)>2&&deck_left<1.5&&edge_distance>11.5) {
        c.move=0;c.jump_pressed=c.dash_pressed=false;return c;
      }
    }
    bool needs_range=edge_distance>std::abs(p.vx)*flight-.6;
    if(!p.grounded&&p.dash_available&&p.dash_time_left<=0&&p.vy>=-5&&
       p.vy<8&&p.y+kPlayerHeight<ty-.25&&(predictive?(std::abs(dx)>4&&needs_range):std::abs(dx)>9))
      c.dash_pressed=true;
    if(!p.grounded&&!c.dash_pressed&&p.dash_time_left<=0&&std::abs(p.vx)>14&&p.vx*dx>0&&
       std::abs(dx)<p.vx*p.vx/(2*kAirAccel)+.4)c.move=p.vx>0?-1:1;
    if(p.grounded)for(const auto &pulse:g.pulses) {
      double eta=std::max(.45,std::abs(dx)/kMaxRunSpeed);
      bool target_exposed=tx>=pulse.area.x-2&&tx<=pulse.area.right()+2&&std::abs(ty-pulse.area.bottom())<1;
      bool standing_clear=!p.rect().intersects(pulse.area);
      if(target_exposed&&standing_clear&&(pulse.active(s.elapsed+eta)||pulse.active(s.elapsed+eta+.4))) {
        c.move=0;c.jump_pressed=c.dash_pressed=false;return c;
      }
    }
    if(p.grounded)for(const auto &pulse:g.pulses) {
      bool approaching=(c.move>0&&pulse.area.x>p.x)||(c.move<0&&pulse.area.x<p.x);
      if(approaching&&std::abs(pulse.area.x-p.x)<4&&std::abs(pulse.area.bottom()-p.y-kPlayerHeight)<2&&
         (pulse.active(s.elapsed+.15)||pulse.warning(s.elapsed))) {
        c.jump_pressed=true;
      }
    }
    if(p.grounded)for(const auto &enemy:g.enemies) {
      auto r=enemy.at(s.elapsed);
      if(std::abs(r.y+1-p.y-kPlayerHeight)<2&&std::abs(r.x-p.x)<3)c.jump_pressed=true;
    }
    return c;
  }
};
struct RouteResult {bool complete=false;int deaths=0,ticks=0,policy=0;size_t target=0;double x=0,y=0;};
RouteResult verify_route_once(const EngineGrid &g,int max_ticks,bool predictive) {
  auto state=new_state(g);FixedStepper stepper;RoutePilot pilot;pilot.predictive=predictive;
  int ticks=0;
  for(;ticks<max_ticks&&!state.won&&state.deaths<5;++ticks)
    stepper.advance(state,g,pilot.controls(g,state),1./120);
  return {state.won,state.deaths,ticks,int(predictive),pilot.target,state.player.x,state.player.y};
}

inline RouteResult verify_route(const EngineGrid &g,int max_ticks=120*120) {
  auto first=verify_route_once(g,max_ticks,false);
  if(first.complete)return first;
  return verify_route_once(g,max_ticks,true);
}
// Generation has a hard two-candidate bound. Failure falls back to a separately
// tested authored recovery route; death/resume rebuilds the same candidate.
inline EngineGrid recovery_room_for(const EngineGrid &requested) {
  int id=requested.district*12+7;
  auto fallback=iceworld::build_room(id,requested.seed,requested.district,false,true);
  const auto &layout=iceworld::pattern(id);size_t branch=0;
  for(auto node:requested.interactions)if(node.kind==InteractionKind::Task) {
    auto pad=layout.optional.empty()?layout.path.front():layout.optional[branch++%layout.optional.size()];
    node.area={pad.x+1.,pad.y-2.,2,2};
    fallback.interactions.push_back(node);
  }
  fallback.generation_fallback=2;return fallback;
}
inline EngineGrid verified_room(int id,uint32_t seed,int district,bool expert=false) {
  auto candidate=iceworld::build_room(id,seed,district,expert);
  auto result=verify_route(candidate);
  if(result.complete){candidate.verification_policy=result.policy;return candidate;}
  candidate=iceworld::build_room(id,seed,district,expert,true);
  result=verify_route(candidate);
  if(result.complete){candidate.verification_policy=result.policy;candidate.generation_fallback=1;return candidate;}
  return recovery_room_for(candidate);
}
