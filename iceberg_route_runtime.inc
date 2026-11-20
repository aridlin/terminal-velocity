// Included inside AdventureSession. Branch decisions are campaign state, not
// a drawing convention. A fork replaces exactly one non-quest traversal cell.
  int route_leg() const {return stage<=3?0:stage<=5?1:2;}
  uint32_t next_expedition_seed() const {return iceberg::mix(run_seed+1);}
  std::vector<int> expedition_itinerary(uint32_t seed,std::span<const int> preceding={}) const {
    const auto &e=profile.expeditions[mode==iceberg::Mode::Endless?0:1];
    return iceworld::make_itinerary(seed,district,iceberg::RoomsPerLeg,e.layout_version,mode,preceding);
  }
  std::vector<int> next_expedition_itinerary() const {
    return expedition_itinerary(next_expedition_seed(),profile.expeditions[0].itinerary);
  }
  bool route_forks_enabled() const {
    return mode==iceberg::Mode::Campaign&&profile.layout_version>=4&&district==0;
  }
  bool on_campaign_route() const {
    return mode==iceberg::Mode::Campaign&&profile.hidden_room<0&&profile.visit_district<0&&
      !profile.revisit.active&&!profile.interlude&&(stage==2||stage==4||stage==6);
  }
  bool at_route_fork() const {
    if(!state.won)return false;
    const int nearby=near_interaction();
    return nearby>=0&&grid.interactions[size_t(nearby)].kind==InteractionKind::RouteChoice;
  }
  void restore_route_fork() {
    if(!on_campaign_route()||!route_forks_enabled()||room!=1)return;
    const int leg=route_leg();
    if((profile.story&iceberg::ForkResumeMask)!=iceberg::fork_resume_bits(leg)||
       (profile.route_decisions[size_t(district)]&(1u<<leg)))return;
    const double x=grid.goal.x-.25,y=grid.goal.bottom()-kPlayerHeight;
    const int floor=int(std::lround(grid.goal.bottom()));
    const Rect courier{x,y,kPlayerWidth,kPlayerHeight};
    if(std::abs(grid.goal.bottom()-floor)>.001||!grid.touches_exit(courier)||!ledge_clearance(grid,courier)||
       !grid.solid_at(int(std::floor(x+kPlayerWidth*.5)),floor))return;
    const auto previous=state.player;
    state.player.x=x;state.player.y=y;state.won=true;
    if(!at_route_fork()) {
      state.player=previous;state.won=false;return;
    }
    state.player.grounded=true;
    state.adventure.door_open=1.;
  }
  int route_room_id(int leg,int cell) const {
    if(route_forks_enabled()&&cell==2&&(profile.route_choices[size_t(district)]&(1u<<leg)))
      return iceworld::harbor_alternate(profile.itinerary,leg,profile.seed);
    return profile.itinerary[size_t(leg*iceberg::RoomsPerLeg+cell)];
  }
  void configure_route_room() {
    if(!on_campaign_route()||grid.room_role!=iceberg::RoomRole::Traverse)return;
    const int leg=route_leg(),index=leg*iceberg::RoomsPerLeg+room;
    if(profile.started)profile.route_visited[size_t(district)]|=1u<<index;
    if(route_forks_enabled()&&room==1) {
      const int alternative=iceworld::harbor_alternate(profile.itinerary,leg,profile.seed);
      grid.interactions.push_back({{grid.goal.x-4,grid.goal.y,3,grid.goal.h},
        InteractionKind::RouteChoice,alternative,iceworld::pattern(alternative).name,leg});
    }
  }
  std::pair<int,int> expedition_route_position() const {
    const auto &v=state.adventure;
    const int completed=v.route.completed_legs*iceberg::RoomsPerLeg;
    const int selected=v.route_room_selection<0?completed+v.route.current:v.route_room_selection;
    const int count=v.route.rooms.empty()?0:completed+int(v.route.rooms.size())+
      (mode==iceberg::Mode::Endless?iceberg::RoomsPerLeg:0);
    return {selected,count};
  }
  void sync_route_view() {
    auto &v=state.adventure;auto &r=v.route;r={};
    // A hidden excursion never appears in the route graph or its counters.
    if(mode==iceberg::Mode::Campaign) {
      if(profile.hidden_room>=0||profile.visit_district>=0||profile.revisit.active||grid.template_id==330)return;
      r.available=true;r.branching=route_forks_enabled();r.leg=route_leg();
      r.visited=profile.route_visited[size_t(district)];r.choices=profile.route_choices[size_t(district)];
      r.decisions=profile.route_decisions[size_t(district)];r.place=grid.room_name;
      if(profile.itinerary.size()==24) {
        for(int id:profile.itinerary)r.rooms.push_back(iceworld::pattern(id).name);
        if(r.branching)for(int leg=0;leg<3;++leg)
          r.alternatives[size_t(leg)]=iceworld::pattern(iceworld::harbor_alternate(profile.itinerary,leg,profile.seed)).name;
      }
      if(on_campaign_route())r.current=r.leg*8+room;
    } else {
      const auto &e=profile.expeditions[mode==iceberg::Mode::Endless?0:1];
      r.available=!e.itinerary.empty();r.current=room;r.place=grid.room_name;
      r.visited=(1u<<unsigned(room+1))-1;
      for(int id:e.itinerary)r.rooms.push_back(iceworld::pattern(id).name);
      const bool timeline=v.map&&!v.map_room;
      const bool show_features=mode==iceberg::Mode::Endless&&e.layout_version>=4&&tour_scene<0;
      const int selected=v.route_room_selection<0?-1:v.route_room_selection%iceberg::RoomsPerLeg;
      if(mode==iceberg::Mode::Endless&&run_active) {
        r.completed_legs=int(e.completed_legs.size());
        if(timeline&&selected>=0) {
          const int page=v.route_room_selection/iceberg::RoomsPerLeg;
          r.browsed_leg=page-r.completed_legs;
          const auto browse=[&](const auto &leg) {
            for(int id:leg)r.browsed_rooms.push_back(iceworld::pattern(id).name);
            if(selected<int(leg.size())) {
              const int id=leg[size_t(selected)];
              r.detail_rhythm=iceworld::pattern(id).rhythm;
              if(show_features&&r.browsed_leg>0)r.detail_feature=iceworld::expedition_feature(id).name;
            }
          };
          if(r.browsed_leg<0)browse(e.completed_legs[size_t(page)]);
          else if(r.browsed_leg>0)browse(next_expedition_itinerary());
        }
      }
      if(timeline) {
        const auto room_rhythm=[&](int index)->std::optional<iceberg::Rhythm> {
          const int count=int(e.itinerary.size());
          if(index<0||index>=count||(mode==iceberg::Mode::Trial&&index==count-1))return {};
          return iceworld::pattern(e.itinerary[size_t(index)]).rhythm;
        };
        r.current_rhythm=room_rhythm(room);
        if(show_features) {
          const auto feature=iceworld::expedition_feature(grid.template_id);
          if(feature.present&&feature.present(grid))r.current_feature=feature.name;
        }
        if(r.browsed_leg==0) {
          const int detail=selected>=0&&selected!=room?selected:room+1;
          r.detail_rhythm=room_rhythm(detail);
          if(show_features&&r.detail_rhythm)
            r.detail_feature=iceworld::expedition_feature(e.itinerary[size_t(detail)]).name;
        }
      }
      if(mode==iceberg::Mode::Trial&&!r.rooms.empty())r.rooms.back()=iceworld::MasteryCourseName;
    }
  }
  bool choose_route(const Interaction &node) {
    if(node.kind!=InteractionKind::RouteChoice)return false;
    if(!on_campaign_route()||!route_forks_enabled()||room!=1)return true;
    const int leg=route_leg();
    if(node.id!=iceworld::harbor_alternate(profile.itinerary,leg,profile.seed))return true;
    unlock_achievement(iceberg::Achievement::Detour);
    profile.route_choices[size_t(district)]|=1u<<leg;
    profile.route_decisions[size_t(district)]|=1u<<leg;
    advance_room();
    notice(t("ODGAŁĘZIENIE — ","BRANCH — ")+grid.room_name.get(profile.language),4);
    return true;
  }
