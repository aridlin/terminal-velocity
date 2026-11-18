// Included inside AdventureSession. Branch decisions are campaign state, not
// a drawing convention. A fork replaces exactly one non-quest traversal cell.
  int route_leg() const {return stage<=3?0:stage<=5?1:2;}
  bool route_forks_enabled() const {
    return mode==iceberg::Mode::Campaign&&profile.layout_version>=4&&district==0;
  }
  bool on_campaign_route() const {
    return mode==iceberg::Mode::Campaign&&profile.hidden_room<0&&profile.visit_district<0&&
      !profile.revisit.active&&!profile.interlude&&(stage==2||stage==4||stage==6);
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
      if(mode==iceberg::Mode::Trial&&!r.rooms.empty())r.rooms.back()=iceworld::MasteryCourseName;
    }
  }
  bool choose_route(const Interaction &node) {
    if(node.kind!=InteractionKind::RouteChoice)return false;
    if(!on_campaign_route()||!route_forks_enabled()||room!=1)return true;
    const int leg=route_leg();
    if(node.id!=iceworld::harbor_alternate(profile.itinerary,leg,profile.seed))return true;
    profile.route_choices[size_t(district)]|=1u<<leg;
    profile.route_decisions[size_t(district)]|=1u<<leg;
    advance_room();
    notice(t("ODGAŁĘZIENIE — ","BRANCH — ")+grid.room_name.get(profile.language),4);
    return true;
  }
