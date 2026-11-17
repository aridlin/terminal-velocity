#pragma once
// Optional heatworks set-piece. Stable scene ID 2, independent of the campaign.
inline EngineGrid build_jubilee() {
  EngineGrid g(196,64);g.adventure_level=true;g.event_level=false;g.district=1;
  g.template_id=303;g.room_role=RoomRole::Interlude;
  g.room_name={"Pompa na miarę pontyfikatu","A pump worthy of a pontificate"};
  const std::vector<Pad> path{{3,55,18},{29,52,15},{53,48,13,'v'},{76,44,18},
    {103,40,13},{125,36,13,'v'},{147,32,17},{127,28,13},{105,24,16},
    {82,20,14},{58,24,14},{35,28,17},{13,32,16}};
  for(size_t i=0;i<path.size();++i) {
    auto p=path[i];
    if(p.kind=='v')g.platforms.push_back({{double(p.x),double(p.y),double(p.w),.5},0,1.8,4.,double(i%3)/3.});
    else g.add_solid_rect(p.x,p.y,p.w,2);
    g.route_points.push_back({p.x+p.w*.5,double(p.y)});
    if(p.kind!='v')g.checkpoint_points.push_back({p.x+2.,p.y-kPlayerHeight});
    if(i%3==2)g.refills.push_back({p.x-1.,p.y-2.5});
    if(i%2)g.sweets.push_back({p.x+p.w*.65,p.y-2.});
  }
  g.spawn_x=5;g.spawn_y=55-kPlayerHeight;g.goal={23,28,3,4};g.goal_requires=7;
  g.add_hazard_rect(0,62,g.width(),2);
  g.interactions.push_back({{8,51,3,4},InteractionKind::Note,30,{"Przeczytaj akt fundacyjny","Read the dedication"}});
  g.interactions.push_back({{34,48,3,4},InteractionKind::Valve,0,{"Odłącz wodotrysk","Disconnect the fountain"}});
  g.interactions.push_back({{152,28,3,4},InteractionKind::Valve,1,{"Zmień obieg pompy","Reroute the pump"}});
  g.interactions.push_back({{109,20,3,4},InteractionKind::Valve,2,{"Otwórz ogrzewanie mieszkań","Open the residential heating"}});
  g.gates.push_back({{26,27,1,5},7});
  // Steam columns require the taught wait / jump / lift rhythm. Resting pads
  // remain clear, and each apparatus has its own checkpoint.
  g.pulses.push_back({{84,41,1.5,3},4.8,.15});
  g.pulses.push_back({{155,29,1.5,3},5.2,.55});
  // A high optional maintenance shelf combines lift launch and upward dash.
  g.add_solid_rect(137,20,7,1);g.refills.push_back({135,25});g.sweets.push_back({140,18});
  return g;
}
