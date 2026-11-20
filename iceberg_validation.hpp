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
    // A launch can legitimately skip a landing on a folding stairwell. Once
    // standing on a later landing, continue from there instead of steering
    // back into the underside of the platform that was passed in the air.
    if(g.district>=2&&p.grounded&&target>1&&target<=g.route_points.size()&&p.y+kPlayerHeight>g.route_points[target-1].second+3) {
      for(size_t i=0;i+1<target;++i)if(std::abs(p.y+kPlayerHeight-g.route_points[i].second)<1.1&&std::abs(p.x+kPlayerWidth*.5-g.route_points[i].first)<4.5){target=i+1;break;}
    }
    if(g.district>=2&&p.grounded)for(size_t i=target+1;i<g.route_points.size();++i) {
      auto [x,y]=g.route_points[i];
      if(std::abs(p.y+kPlayerHeight-y)<1.1&&std::abs(p.x+kPlayerWidth*.5-x)<4.5)target=i+1;
    }
    if(target<g.route_points.size()) {
      auto [x,y]=g.route_points[target];
      if(p.grounded&&std::abs(p.y+kPlayerHeight-y)<1.1&&std::abs(p.x+kPlayerWidth*.5-x)<4.5)++target;
    }
    double tx=g.goal.x+.5,ty=g.goal.bottom();
    if(target<g.route_points.size()){tx=g.route_points[target].first;ty=g.route_points[target].second;}
    // Aim at the platform's current height, including the tall counterweights.
    // A nominal three-unit climb may actually be five units at this phase.
    if(g.district>=2)for(const auto &platform:g.platforms)if(std::abs(tx-platform.base.x-platform.base.w*.5)<.1&&std::abs(ty-platform.base.y)<.1) {
      auto at=platform.at(s.elapsed);tx=at.x+at.w*.5;ty=at.y;break;
    }
    double dx=tx-(p.x+kPlayerWidth*.5);
    c.move=dx>.4?1:dx<-.4?-1:0;
    c.jump_held=true;
    if(p.ledge) {
      if(c.move==p.ledge->side&&ty<=p.ledge->row+1.)
        c.jump_pressed=p.ledge_climb_left<=0.;
      else c.move=0;
      return c;
    }
    int foot=int(std::floor(p.y+kPlayerHeight+.1));
    double ahead=p.x+(c.move>0?kPlayerWidth+.25:-.25);
    bool gap=!g.solid_at(int(std::floor(ahead)),foot);
    bool rise=g.solid_at(int(std::floor(ahead)),foot-1);
    if(p.support>=0) {
      auto platform=g.platforms[size_t(p.support)].at(s.elapsed);
      gap=platform.w>=(g.district>=2?9:12)?(c.move>0?p.x+kPlayerWidth>=platform.right()-.3:p.x<=platform.x+.3):true;
    }
    if(p.grounded&&target<g.route_points.size())
      c.jump_pressed=(gap||rise)&&std::abs(dx)>1;
    // Descending passages are entered by dropping from the edge. A full jump
    // can put the courier on a roof instead of inside the passage.
    if((g.template_id>=400||(g.template_id>=24&&g.template_id<72))&&p.grounded&&ty>p.y+kPlayerHeight+2&&p.dash_available)c.jump_pressed=false;
    if(p.grounded&&c.move) {
      int danger_x=int(std::floor(p.x+(c.move>0?kPlayerWidth+1.7:-1.7)));
      if(g.hazard_at(danger_x,foot-1)||g.hazard_at(danger_x,foot-2))c.jump_pressed=true;
    }
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
  // Room selection must stay stable for saved seeds as movement features evolve.
  state.reference_movement=true;
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
  int id=requested.district==2?29:requested.district*12+7;
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

inline void add_crown_seam_shortcut(EngineGrid &room) {
  if(room.district!=5||room.template_id!=61)return;
  auto shortcut=room;
  // Leave the zigzag ridge to the west open. The existing side landing
  // at y=45 leads into this narrow climb and the ridge at y=21.
  constexpr Rect seam{82,25,7,16};
  shortcut.add_solid_rect(80,25,2,16);
  shortcut.add_solid_rect(89,25,2,17);
  shortcut.crown_seam=seam;
  shortcut.interactions.push_back({{86,42,2,3},InteractionKind::Note,iceberg::CrownShaftPlate,
    {"Tabliczka przy szybie","Shaft maintenance plate"}});
  const auto route=verify_route(shortcut);
  if(route.complete&&route.deaths==0) {
    shortcut.verification_policy=route.policy;
    room=std::move(shortcut);
  }
}

inline void add_ridge_crosswind(EngineGrid &room) {
  if(room.district!=5||room.template_id!=68)return;
  // The upper sensor ledges sit in the exposed strip. The lower landings
  // stay below it and keep their original terrain and checkpoints.
  room.air_fields.push_back({{73.,21.,44.,17.},52.,0.,7.2,0.,0,true});
  auto mast=room;
  for(const auto &step:iceworld::RidgeMastSteps) {
    mast.add_solid_rect(step.x,step.y,step.w,1);
  }
  const auto mast_route=verify_route(mast);
  if(mast_route.complete&&mast_route.deaths==0) {
    mast.verification_policy=mast_route.policy;
    room=std::move(mast);
    return;
  }
  const auto route=verify_route(room);
  if(route.complete)room.verification_policy=route.policy;
  else room.air_fields.pop_back();
}

inline void strengthen_expedition_wind(EngineGrid &room) {
  if(room.district!=5||room.air_fields.size()!=1)return;
  auto &field=room.air_fields.front();
  const double original=field.ax;
  field.ax=std::copysign(44.,original);
  const auto route=verify_route(room);
  if(route.complete)room.verification_policy=route.policy;
  else field.ax=original;
}

inline void add_overhang_windbreak(EngineGrid &room) {
  if(room.district!=5||room.template_id!=70||room.air_fields.size()!=1)return;
  auto sheltered=room;
  constexpr Rect roof{65,34,50,2};
  sheltered.add_solid_rect(int(roof.x),int(roof.y),int(roof.w),int(roof.h));
  sheltered.windbreak_roof=roof;
  auto &wind=sheltered.air_fields.front();
  wind.area.h=roof.top()-wind.area.top();
  const auto route=verify_route(sheltered);
  if(route.complete&&route.deaths==0) {
    sheltered.verification_policy=route.policy;
    room=std::move(sheltered);
  }
}

inline void add_fractured_ribs(EngineGrid &room) {
  if(room.district!=5||room.template_id!=63)return;
  const size_t previous=room.platforms.size();
  room.platforms.push_back({.base={74,41,4.5,.5},.fragile=true});
  room.platforms.push_back({.base={75,37,4.5,.5},.fragile=true});
  const auto route=verify_route(room);
  if(route.complete&&route.deaths==0)room.verification_policy=route.policy;
  else room.platforms.resize(previous);
}

inline void add_shutter_service_lift(EngineGrid &room) {
  if(room.district!=5||room.template_id!=69)return;
  // The narrow lift travels between the lower approach and the upper
  // service shelves. Its shaft sits in the gap between the solid ledges.
  room.platforms.push_back({.base={68.5,40.5,3.5,.5},.dy=9.,.period=7.2,.phase=.25,.service_lift=true});
  const auto route=verify_route(room);
  if(route.complete&&route.deaths==0)room.verification_policy=route.policy;
  else room.platforms.pop_back();
}

inline void add_expedition_ice(EngineGrid &room) {
  if(room.district!=4)return;
  if(room.template_id==48)room.ice_strips={{8,56,12,1},{48,50,9,1},{68,47,12,1},{108,47,9,1}};
  else if(room.template_id==58)room.ice_strips={{8,30,9,1},{64,42,10,1},{104,50,9,1}};
  else return;
  const auto route=verify_route(room);
  if(route.complete)room.verification_policy=route.policy;
  else room.ice_strips.clear();
}

inline void add_leaf_chute_flume(EngineGrid &room) {
  if(room.district!=4||room.template_id!=58)return;
  // An optional ride above the original stepping beds.
  room.platforms.push_back({.base={73,40.2,6,.5},.dx=18.,.dy=3.,.period=8.4,.chute_leaf=true});
  const auto route=verify_route(room);
  if(route.complete&&route.deaths==0)room.verification_policy=route.policy;
  else room.platforms.pop_back();
}

inline void add_reservoir_ice(EngineGrid &room) {
  if(room.district!=4||room.template_id!=55)return;
  const size_t existing=room.platforms.size();
  for(int x=76;x<106;x+=6)
    room.platforms.push_back({.base={double(x),48,6,.5},.fragile=true,.icy=true});
  room.water_basins.push_back({76,48,30,14});
  room.sweets.push_back({98.5,46.7});
  const auto route=verify_route(room);
  if(route.complete)room.verification_policy=route.policy;
  else {
    room.platforms.resize(existing);
    room.water_basins.pop_back();
    room.sweets.pop_back();
  }
}

inline void add_expedition_root_pod(EngineGrid &room) {
  if(room.district!=4||room.template_id!=49)return;
  // The outer end of the turnaround lets riders reach the side shelves.
  room.springs.push_back({71.5,46.5,2.5,1.5});
  const auto route=verify_route(room);
  if(route.complete)room.verification_policy=route.policy;
  else room.springs.pop_back();
}

inline void add_expedition_irrigation_wheel(EngineGrid &room) {
  if(room.district!=4||room.template_id!=57)return;
  const size_t original=room.platforms.size();
  // The scoops pass above the first bucket lift and beside the seed shelf.
  for(double phase:{0.,.25,.5,.75})
    room.platforms.push_back({.base={42,44,4,.5},.dx=5.,.dy=7.6,.period=7.2,
      .phase=phase,.irrigation_scoop=true});
  const auto route=verify_route(room);
  if(!route.complete||route.deaths!=0) {
    room.platforms.resize(original);
    return;
  }
  room.verification_policy=route.policy;
  room.interactions.push_back({{51,47,2,3},InteractionKind::Note,iceberg::IrrigationCallBox,
    {"Rura głosowa","Speaking tube"}});
}

inline void add_expedition_channel_bridge(EngineGrid &room) {
  if(room.district!=4||room.template_id!=52)return;
  // Extend from the overlook over the lower landing. The old route stays
  // open under the trellis, and the bridge ends above the next platform.
  room.interactions.push_back({{80,38,3,3},InteractionKind::Valve,0,
    {"Podlej pnącza","Water the vines"}});
  room.growth.push_back({{86,41,27,1},1});
  room.sweets.push_back({101.5,39.5});
}

inline void add_garden_hanging_planter(EngineGrid &room) {
  if(room.district!=4||room.template_id!=54)return;
  // Board from the west bed and swing upward to the edge of the seed shelves.
  room.platforms.push_back({.base={61.5,36.,4.,.5},.dx=3.,.period=7.2,
    .swing_pivot=55.667,.swing_length=13.034});
  const auto route=verify_route(room);
  if(route.complete&&route.deaths==0)room.verification_policy=route.policy;
  else room.platforms.pop_back();
}

inline void add_garden_seed_tray(EngineGrid &room) {
  if(room.district!=4||room.template_id!=iceberg::GardenSeedBank)return;
  // The tray stays beside the shelves and leaves both lower landings clear.
  room.platforms.push_back({.base={60,41.5,7,.5},.dy=5.5,.period=8.,.phase=.5,.seed_tray=true});
  const auto route=verify_route(room);
  if(route.complete&&route.deaths==0)room.verification_policy=route.policy;
  else room.platforms.pop_back();
}

inline void add_expedition_fragile_pots(EngineGrid &room) {
  if(room.district!=4||room.template_id!=51)return;
  const size_t original=room.platforms.size();
  room.platforms.push_back({.base={57,39,5,.5},.fragile=true});
  room.platforms.push_back({.base={67,34,5,.5},.fragile=true});
  const auto route=verify_route(room);
  if(route.complete)room.verification_policy=route.policy;
  else room.platforms.resize(original);
}

inline void add_expedition_post_launchers(EngineGrid &room) {
  if(room.district!=3||room.template_id!=38)return;
  const size_t original=room.springs.size();
  room.springs.emplace_back(55.8,37.65,1.6,.35,28.,-40.);
  room.springs.emplace_back(72.,31.65,1.6,.35,28.,-34.);
  const auto route=verify_route(room);
  if(route.complete)room.verification_policy=route.policy;
  else room.springs.erase(room.springs.begin()+original,room.springs.end());
}

inline void add_archive_reference_carriers(EngineGrid &room) {
  if(room.district!=3)return;
  struct Carrier { int room;double x,upper,lower,w; };
  static constexpr Carrier plans[]{
    {37,66.,42.3,48.4,8.},{40,44.,30.6,37.,6.5},{42,65.,43.0,49.3,9.},
    {43,52.9,35.5,43.,6.4},{46,67.2,36.,47.,6.4}
  };
  for(const auto &plan:plans)if(plan.room==room.template_id) {
    room.platforms.push_back({.base={plan.x,(plan.upper+plan.lower)*.5,plan.w,.5},
      .dy=(plan.lower-plan.upper)*.5,.period=6.4,.phase=.25,.archive_rack=true});
    const auto route=verify_route(room);
    if(route.complete&&route.deaths==0)room.verification_policy=route.policy;
    else room.platforms.pop_back();
    break;
  }
}

inline void add_archive_scan_rails(EngineGrid &room) {
  if(room.district!=3||room.room_role!=iceberg::RoomRole::Traverse)return;
  const auto &layout=iceworld::pattern(room.template_id);
  if(iceworld::district_of(layout)!=3||layout.optional.size()<3)return;
  const auto &desk=layout.optional.back();
  // The scanner sweeps the outer edge of the optional desk.
  room.pulses.push_back({{desk.x+1.,desk.y-2.7,desk.w-1.7,2.7},
    5.1+(room.template_id%3)*.4,.31+(room.template_id%4)*.12,PulseKind::Scanner});
  const auto route=verify_route(room);
  if(route.complete&&route.deaths==0)room.verification_policy=route.policy;
  else room.pulses.pop_back();
}

inline void add_archive_glass_catwalk(EngineGrid &room) {
  if(room.template_id!=iceberg::ArchiveGlassRoom)return;
  const auto count=room.platforms.size();
  // Two panes climb out of the reading aisle. The rest join the existing shelves.
  for(const Rect pane:std::array<Rect,4>{{{53.5,43,5,.5},{54,39.5,5,.5},{56,33.5,6,.5},{71,29.25,7,.5}}})
    room.platforms.push_back({.base=pane,.fragile=true});
  const auto route=verify_route(room);
  if(!route.complete||route.deaths!=0) {
    room.platforms.resize(count);
    return;
  }
  room.verification_policy=route.policy;
  room.interactions.push_back({{85.5,24,2,3},InteractionKind::Npc,4,{"Porozmawiaj","Talk"}});
}

inline void add_expedition_drawers(EngineGrid &room) {
  if(room.district!=3||room.template_id!=39)return;
  room.interactions.push_back({{57,41,3,3},InteractionKind::Valve,0,
    {"Wysuń szuflady","Extend the drawers"}});
  room.growth.push_back({{59,40,12,1},1});
  room.growth.push_back({{65,35,12,1},1});
}

inline void add_archive_stack_shuttles(EngineGrid &room) {
  if(room.district!=3||room.template_id!=41||room.platforms.size()!=3)return;
  auto original=room.platforms;
  for(size_t i=0;i<room.platforms.size();++i) {
    auto &shuttle=room.platforms[i];
    shuttle.dx=5.4;
    shuttle.period=5.6;
    shuttle.phase=i*.16;
    shuttle.tram=true;
  }
  const auto route=verify_route(room);
  if(route.complete&&route.deaths==0)room.verification_policy=route.policy;
  else room.platforms=std::move(original);
}

inline void add_expedition_copy_belts(EngineGrid &room) {
  if(room.district!=3||room.template_id!=44)return;
  // Leave ordinary footing at both ends of each landing.
  room.sorting_belts={{29,50,8,5},{47,47,9,-4},{67,44,8,6}};
  const auto route=verify_route(room);
  if(!route.complete) {
    room.sorting_belts.clear();
    return;
  }
  room.verification_policy=route.policy;
  // The relay is on the fixed footing before the middle belt.
  auto &belt=room.sorting_belts[1];
  constexpr uint64_t reverse=1;
  const auto original_flags=room.switch_flags;
  belt.reverse_flag=reverse;
  room.switch_flags|=reverse;
  const auto reversed=verify_route(room);
  room.switch_flags=original_flags;
  if(reversed.complete&&reversed.deaths==0) {
    room.interactions.push_back({{45,44,2,3},InteractionKind::Valve,0,
      {"Odwróć środkową taśmę","Reverse the middle belt"}});
    room.interactions.push_back({{36.75,47.,1.5,3.},InteractionKind::Npc,4,
      {"Porozmawiaj z Ireną","Talk to Irena"}});
  } else belt.reverse_flag=0;
}

inline void add_expedition_archive_trolley(EngineGrid &room) {
  if(room.district!=3||room.template_id!=45)return;
  // Board from the main walkway and step off at the upper envelope shelf.
  room.platforms.push_back({.base={92,34.4,5,.5},.dx=8.5,.dy=-8,.period=7.2,
    .archive_trolley=true});
  const auto route=verify_route(room);
  if(!route.complete||route.deaths!=0) {
    room.platforms.pop_back();
    return;
  }
  room.verification_policy=route.policy;
  room.interactions.push_back({{84,43,2,3},InteractionKind::Note,iceberg::ArchiveTrolleyNotice,
    {"Tabliczka suszarni","Drying-room notice"}});
  room.interactions.push_back({{96,20,2,3},InteractionKind::Note,iceberg::ArchiveDryingLetter,
    {"List z suszarni","Letter on the drying shelf"}});
}

inline void add_expedition_archive_racks(EngineGrid &room) {
  if(room.district!=3||room.template_id!=47)return;
  const size_t original=room.platforms.size();
  // The two racks share a cable. Each is at its highest when the other
  // reaches the lower winding drum.
  room.platforms.push_back({.base={58,37,6,.5},.dy=7,.period=6.4,.archive_rack=true});
  room.platforms.push_back({.base={74,37,6,.5},.dy=7,.period=6.4,.phase=.5,.archive_rack=true});
  room.sweets.push_back({76.5,24.});
  const auto route=verify_route(room);
  if(route.complete&&route.deaths==0)room.verification_policy=route.policy;
  else {
    room.platforms.resize(original);
    room.sweets.pop_back();
  }
}

inline void add_expedition_stairwells(EngineGrid &room) {
  if(room.district!=2||room.template_id!=25)return;
  auto original=room;
  for(auto [x,y,w,h]:iceworld::EstateStairwellWalls)room.add_solid_rect(x,y,w,h);
  const auto route=verify_route(room);
  if(route.complete)room.verification_policy=route.policy;
  else room=std::move(original);
}

inline void add_estate_gutters(EngineGrid &room) {
  if(room.district!=2||room.template_id!=33)return;
  auto climb=room;
  // The downspouts end above the street route, below the token landing.
  for(auto [x,y,w,h]:iceworld::EstateGutterWalls)climb.add_solid_rect(x,y,w,h);
  const auto route=verify_route(climb);
  if(route.complete&&route.deaths==0) {
    climb.verification_policy=route.policy;
    room=std::move(climb);
  }
}

inline void add_estate_clotheslines(EngineGrid &room) {
  if(room.district!=2||room.template_id!=26)return;
  const size_t original=room.platforms.size();
  // The upper lines bridge the small balconies without changing the street route.
  room.platforms.push_back({.base={32,39.5,21,.5},.dy=.45,.period=3.8,.clothesline=true});
  // Keep the rider below the awning at y=26 and their feet above the balcony at y=29.
  room.platforms.push_back({.base={66,28.3,17,.5},.dy=.4,.period=4.3,.phase=.5,.clothesline=true});
  const auto route=verify_route(room);
  if(route.complete&&route.deaths==0)room.verification_policy=route.policy;
  else room.platforms.resize(original);
}

inline void add_estate_balconies(EngineGrid &room) {
  if(room.district!=2)return;
  struct Balcony { int room; double x,upper,lower,w; };
  static constexpr Balcony plans[]{
    {27,37.3,30.,39.,3.5},{28,104.25,31.,37.,6.5},{29,52.6,36.,43.,5.},
    {30,65.2,41.,47.,5.6},{32,95.2,36.,43.,6.},{34,114.,27.,34.,6.3}
  };
  for(const auto &plan:plans)if(plan.room==room.template_id) {
    room.platforms.push_back({.base={plan.x,(plan.upper+plan.lower)*.5,plan.w,.5},
      .dy=(plan.lower-plan.upper)*.5,.period=5.8,.phase=.5,.estate_balcony=true});
    const auto route=verify_route(room);
    if(route.complete&&route.deaths==0)room.verification_policy=route.policy;
    else room.platforms.pop_back();
    break;
  }
}

inline void add_estate_masonry(EngineGrid &room) {
  if(room.district!=2||room.room_role!=iceberg::RoomRole::Traverse)return;
  const auto &layout=iceworld::pattern(room.template_id);
  if(iceworld::district_of(layout)!=2||layout.optional.empty())return;
  const auto &bed=layout.optional.back();
  // Leave the token at the center of the upper balcony clear. The loose
  // cornice drops onto the far end just beyond it.
  const double floor=bed.y;
  room.pulses.push_back({{bed.x+bed.w-1.85,floor-5.,1.5,5.},
    4.9+(room.template_id%3)*.3,.33+(room.template_id%4)*.12,PulseKind::Masonry});
  const auto route=verify_route(room);
  if(route.complete&&route.deaths==0)room.verification_policy=route.policy;
  else room.pulses.pop_back();
}

inline void add_estate_awnings(EngineGrid &room) {
  if(room.district!=2||room.template_id!=35)return;
  // Both canopies sit in the gaps before the upper token shelves.
  const auto original=room.springs.size();
  room.springs.emplace_back(46.,45.35,6.,.55,20.,-42.,true);
  room.springs.emplace_back(65.3,34.35,4.,.55,17.,-38.,true);
  const auto route=verify_route(room);
  if(route.complete&&route.deaths==0)room.verification_policy=route.policy;
  else room.springs.erase(room.springs.begin()+original,room.springs.end());
}

inline void add_expedition_trams(EngineGrid &room) {
  if(room.district!=2||room.template_id!=24||room.platforms.size()!=3)return;
  auto original=room.platforms;
  for(size_t i=0;i<room.platforms.size();++i) {
    auto &tram=room.platforms[i];
    tram.dx=7.5;tram.period=6.;tram.phase=i*.17;tram.tram=true;
  }
  const auto route=verify_route(room);
  if(!route.complete) {
    room.platforms=std::move(original);
    return;
  }
  room.verification_policy=route.policy;
  room.interactions.push_back({{13,51,2,3},InteractionKind::Npc,2,{"Porozmawiaj","Talk"}});
}

inline void add_heatworks_cargo_hoists(EngineGrid &room) {
  if(room.district!=1||room.template_id!=13||room.platforms.size()!=4)return;
  auto original=room.platforms;
  for(size_t i=0;i<room.platforms.size();++i) {
    auto &hoist=room.platforms[i];
    hoist.dx=0;
    hoist.dy=2.5;
    hoist.period=5.2;
    hoist.phase=i*.25+.5;
    hoist.cargo_hoist=true;
  }
  const auto route=verify_route(room);
  if(!route.complete||route.deaths!=0) {
    room.platforms=std::move(original);
    return;
  }
  room.verification_policy=route.policy;
  room.interactions.push_back({{11,53,2,3},InteractionKind::Npc,1,{"Porozmawiaj","Talk"}});
}

inline void add_heatworks_twin_pistons(EngineGrid &room) {
  if(room.district!=1||room.template_id!=421)return;
  const size_t original=room.platforms.size();
  // These decks fit between the existing token shelves. Both swing lifts
  // and fixed landings on the original through-route remain in place.
  room.platforms.push_back({.base={62,40.25,4,.5},.dy=6.75,.period=6.4,
    .phase=.5,.pressure_piston=true});
  room.platforms.push_back({.base={76,33.25,5,.5},.dy=5.25,.period=6.4,
    .pressure_piston=true});
  const auto route=verify_route(room);
  if(!route.complete||route.deaths!=0) {
    room.platforms.resize(original);
    return;
  }
  room.verification_policy=route.policy;
}

inline void add_heatworks_pressure_bypass(EngineGrid &room) {
  if(room.district!=1||room.template_id!=20)return;
  auto bypass=room;
  bypass.air_fields.push_back({.area={58.5,27.,6.4,15.},.ax=12.,.ay=-150.,.enabled_by=1});
  bypass.switch_flags|=1;
  const auto route=verify_route(bypass);
  if(!route.complete||route.deaths!=0)return;
  // The generated route was verified before adding the bypass. Keep its
  // policy for the unpowered room and leave the valve closed on first entry.
  room.air_fields=std::move(bypass.air_fields);
  room.interactions.push_back({{52,40,4,3},InteractionKind::Valve,0,
    {"Otwórz dopływ pary","Open steam supply"}});
}

inline void add_heatworks_turbine(EngineGrid &room) {
  if(room.district!=1||room.template_id!=23)return;
  // The draft crosses the upper maintenance gap and ends over the receiving
  // ledge. The regular catwalks are well below the airstream.
  room.air_fields.push_back({.area={63.,22.,15.,10.},.ax=120.,.ay=-14.,.period=5.2,.turbine=true});
  auto access=room;
  for(const auto &step:iceworld::TurbineAccessSteps)
    access.add_solid_rect(step.x,step.y,step.w,1);
  const auto stepped_route=verify_route(access);
  if(stepped_route.complete&&stepped_route.deaths==0) {
    access.verification_policy=stepped_route.policy;
    room=std::move(access);
    return;
  }
  const auto route=verify_route(room);
  if(route.complete)room.verification_policy=route.policy;
  else room.air_fields.pop_back();
}

inline void add_heatworks_pressure_hazards(EngineGrid &room) {
  if(room.district!=1)return;
  struct Outlet { int room;double x,floor,period,phase; };
  // The service lanes have room to jump. The warehouse and cool-down
  // routes keep their warning outlets on the higher maintenance shelves.
  static constexpr Outlet outlets[]{
    {12,73.,42.,4.5,.21},{17,73.8,43.,4.8,.58},{19,88.,33.,5.3,.37},
    {420,93.3,39.,4.7,.26},{425,63.3,33.,5.2,.41},{425,95.8,31.,5.2,.85}
  };
  const auto original=room.pulses.size();
  for(const auto &outlet:outlets)if(outlet.room==room.template_id)
    room.pulses.push_back({{outlet.x,outlet.floor-2.5,1.5,2.5},outlet.period,outlet.phase});
  if(room.pulses.size()==original)return;
  const auto route=verify_route(room);
  if(route.complete&&route.deaths==0)room.verification_policy=route.policy;
  else room.pulses.resize(original);
}

inline void add_heatworks_boiler_wall_fan(EngineGrid &room) {
  if(room.district!=1||room.template_id!=14)return;
  auto fan_room=room;
  fan_room.add_solid_rect(75,45,3,1);
  fan_room.air_fields.push_back({.area={75.,30.,2.8,15.},.ay=-165.,.period=5.,.phase=.46,.turbine=true});
  const auto route=verify_route(fan_room);
  if(route.complete&&route.deaths==0) {
    fan_room.verification_policy=route.policy;
    room=std::move(fan_room);
  }
}

inline void add_heatworks_service_bypass(EngineGrid &room) {
  if(room.district!=1||room.template_id!=16)return;
  auto bypass=room;
  bypass.air_fields.push_back({.area={107.,23.,2.9,17.},.ay=-162.,.enabled_by=1});
  bypass.switch_flags|=1;
  const auto route=verify_route(bypass);
  if(!route.complete||route.deaths!=0)return;
  room.air_fields=std::move(bypass.air_fields);
  room.interactions.push_back({{102.,37.,3,3},InteractionKind::Valve,0,
    {"Uruchom boczny obieg","Open the bypass feed"}});
}

inline void add_heatworks_service_gallery(EngineGrid &room) {
  if(room.district!=1||room.template_id!=424)return;
  const auto original=room.pulses.size();
  // The pressure rise is visible from the approach to each catwalk.
  room.pulses.push_back({{91.,48.,1.7,3.},5.1,.33});
  room.pulses.push_back({{129.,40.,1.7,3.},4.6,.71});
  const auto route=verify_route(room);
  if(route.complete&&route.deaths==0)room.verification_policy=route.policy;
  else room.pulses.resize(original);
}

inline void add_heatworks_return_fan(EngineGrid &room) {
  if(room.district!=1||room.template_id!=422)return;
  auto fan_room=room;
  // Install the fan in an extension of the turnback landing. The draft
  // rises to the right of the first shelf, which it would otherwise hit.
  fan_room.add_solid_rect(114,39,5,1);
  fan_room.air_fields.push_back({.area={116.75,19.,2.7,20.},.ay=-177.,.period=5.4,.phase=.25,.turbine=true});
  const auto route=verify_route(fan_room);
  if(route.complete&&route.deaths==0) {
    fan_room.verification_policy=route.policy;
    room=std::move(fan_room);
  }
}

inline void add_expedition_steam(EngineGrid &room) {
  if(room.district!=1||(room.template_id!=18&&room.template_id!=22))return;
  if(room.template_id==18)room.air_fields.push_back({{59.2,29.,2.6,19.},0.,-165.,4.8,0.});
  else room.air_fields.push_back({{75.2,31.,2.6,14.},0.,-165.,4.8,.5});

  // Preserve the already selected room and its checkpoint geometry.
  auto route=verify_route(room);
  if(route.complete)room.verification_policy=route.policy;
  else room.air_fields.pop_back();
}

inline void add_garden_vines_and_traps(EngineGrid &room) {
  if(room.district!=4||room.room_role!=iceberg::RoomRole::Traverse)return;
  const auto &layout=iceworld::pattern(room.template_id);
  if(iceworld::district_of(layout)!=4||layout.optional.size()<3)return;
  const auto &takeoff=layout.optional[0],&landing=layout.optional[1],&bed=layout.optional[2];
  const double left=takeoff.x+takeoff.w-.85;
  const double right=landing.x-.85;
  const double length=(right-left)*.5/std::sin(.86);
  const double angle_y=length*std::cos(.86);
  const SwingVine vine{(left+right)*.5,takeoff.y-4.2-angle_y,length,-1.57};
  // Sample the swept body, including the sag at mid-span. Keep vines out
  // of existing trellises and off the ground on every room variant.
  for(int sample=0;sample<=64;++sample)
    if(!ledge_clearance(room,vine.rider(sample/64.*(2.*std::acos(-1.)/1.85))))return;
  const PulseHazard trap{{bed.x+bed.w-1.9,bed.y-2.35,1.8,2.35},
    4.4+(room.template_id%3)*.35,.42+(room.template_id%4)*.12,PulseKind::Flytrap};
  auto decorated=room;
  decorated.vines.push_back(vine);
  decorated.pulses.push_back(trap);
  const auto route=verify_route(decorated);
  if(route.complete&&route.deaths==0) {
    decorated.verification_policy=route.policy;
    room=std::move(decorated);
  }
}
