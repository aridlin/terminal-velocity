#pragma once
// Authored mechanic sequences. This file is included inside iceworld after Pad.
inline EngineGrid build_heatworks_sequence(bool finale) {
  EngineGrid g(finale?158:166,64);
  g.event_level=false;g.adventure_level=true;g.district=1;
  g.template_id=74+int(finale);
  g.room_role=finale?RoomRole::Finale:RoomRole::Lesson;
  g.room_name=finale?Text{"Dwa obiegi, jeden dom","Two circuits, one home"}:
    Text{"Windy pod ciśnieniem","Lifts under pressure"};
  std::vector<Pad> path=finale?
    std::vector<Pad>{{3,56,16},{25,53,10},{41,50,11},{58,47,11},{75,44,12},{92,41,11},{109,38,11},
      {126,35,16},{110,32,10},{94,29,10},{78,26,11},{62,23,11},{46,20,10},{30,17,12}}:
    std::vector<Pad>{{3,57,16},{25,54,12},{44,51,12,'m'},{63,48,12},{82,45,12,'m'},
      {101,42,12},{120,39,12,'m'},{139,36,18}};
  for(size_t i=0;i<path.size();++i) {
    auto p=path[i];
    if(p.kind=='m')g.platforms.push_back({{double(p.x),double(p.y),double(p.w),.5},0,1.2,3.6,double(i%3)/3});
    else g.add_solid_rect(p.x,p.y,p.w,2);
    g.route_points.push_back({p.x+p.w*.5,double(p.y)});
    if(p.kind!='m')g.checkpoint_points.push_back({p.x+2.,p.y-kPlayerHeight});
    if(i%2)g.sweets.push_back({p.x+p.w*.5,p.y-2.});
    if(i%3==2)g.refills.push_back({p.x-2.,p.y-2.});
  }
  g.spawn_x=5;g.spawn_y=path.front().y-kPlayerHeight;
  auto exit=path.back();g.goal={exit.x+exit.w-3.,exit.y-4.,2,4};
  g.add_hazard_rect(0,62,g.width(),2);
  if(finale) {
    g.goal_requires=3;
    g.interactions.push_back({{62,43,3,4},InteractionKind::Valve,0,{"Otwórz zasilanie","Open supply"}});
    g.interactions.push_back({{131,31,3,4},InteractionKind::Valve,1,{"Otwórz powrót","Open return"}});
    g.gates.push_back({{39,12,1,5},3});
    g.add_solid_rect(81,15,9,1);g.platforms.push_back({{96,19,7,.5},0,2.5,3.8,0});
    g.sweets.push_back({84,13});g.sweets.push_back({88,13});
  } else {
    g.interactions.push_back({{7,53,3,4},InteractionKind::Note,3,{"Przeczytaj tabliczkę","Read sign"}});
    g.interactions.push_back({{65,44,3,4},InteractionKind::Note,4,{"Przeczytaj tabliczkę","Read sign"}});
    g.interactions.push_back({{103,38,3,4},InteractionKind::Note,5,{"Przeczytaj tabliczkę","Read sign"}});
    g.pulses.push_back({{70,52,1.2,3},4.2,.3});
    g.add_solid_rect(58,36,9,1);g.sweets.push_back({62,34});
    g.add_solid_rect(94,30,9,1);g.sweets.push_back({98,28});
    g.refills.push_back({93,34});
  }
  return g;
}

inline EngineGrid build_heatworks_settlement() {
  EngineGrid g(98,58);g.adventure_level=true;g.event_level=false;g.district=1;
  g.template_id=101;g.room_role=RoomRole::Settlement;g.room_name=Districts[1].name;
  g.add_solid_rect(0,52,98,6);
  g.add_solid_rect(21,48,11,2);g.add_solid_rect(38,44,20,8);g.add_solid_rect(65,48,11,2);
  g.add_solid_rect(42,36,13,1);
  g.spawn_x=6;g.spawn_y=52-kPlayerHeight;g.checkpoint_points={{6,g.spawn_y}};
  g.route_points={{6,52},{26,48},{48,36},{70,48},{92,52}};
  g.springs.push_back({17,51.7,1.5,.3});g.springs.push_back({33,51.7,1.5,.3});
  g.goal={90,48,3,4};
  g.interactions.push_back({{11,48,4,4},InteractionKind::Npc,1,{"Porozmawiaj","Talk"}});
  g.interactions.push_back({{47,40,4,4},InteractionKind::Npc,7,{"Porozmawiaj","Talk"}});
  g.interactions.push_back({{82,48,4,4},InteractionKind::Board,1,{"Tablica wypraw","Expedition board"}});
  g.interactions.push_back({{73,44,2,4},InteractionKind::Replay,2,{"Fontanna im. JP II","John Paul II fountain"}});
  return g;
}
