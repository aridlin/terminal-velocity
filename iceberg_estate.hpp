#pragma once
inline constexpr std::array<std::array<int,4>,2> EstateStairwellWalls{{{59,37,2,9},{72,27,2,13}}};
// The residential district introduces horizontal tram momentum, then combines
// it with stairwell reversals and the existing lift/wall techniques.
inline EngineGrid build_estate_settlement() {
  EngineGrid g(128,58);g.adventure_level=true;g.event_level=false;g.district=2;
  g.template_id=102;g.room_role=RoomRole::Settlement;g.room_name=Districts[2].name;
  g.add_solid_rect(0,48,128,10);
  g.add_solid_rect(13,44,7,2);g.add_solid_rect(20,41,13,2);g.add_solid_rect(33,41,5,2);g.add_solid_rect(38,41,30,2);g.add_solid_rect(78,43,14,2);
  g.add_solid_rect(100,34,17,1);
  g.platforms.push_back({{91,38,7,.5},0,2,4,0});
  g.spawn_x=6;g.spawn_y=48-kPlayerHeight;g.checkpoint_points={{6,g.spawn_y}};
  // The public street stays on the ground; balconies are optional exploration.
  g.route_points={{6,48},{121,48}};
  g.goal={120,44,3,4};g.sweets={{105,32},{111,32}};
  g.interactions.push_back({{11,44,4,4},InteractionKind::Npc,2,{"Porozmawiaj","Talk"}});
  // The housing desk is on the public street. Balcony climbing is optional;
  // returning with an address must not require another traversal challenge.
  g.interactions.push_back({{53,44,4,4},InteractionKind::Npc,3,{"Porozmawiaj","Talk"}});
  g.interactions.push_back({{110,44,4,4},InteractionKind::Board,2,{"Rozkład odjazdów","Departure board"}});
  return g;
}
inline EngineGrid build_estate_sequence(bool finale) {
  EngineGrid g(finale?192:166,64);g.adventure_level=true;g.event_level=false;g.district=2;
  g.template_id=76+int(finale);g.room_role=finale?RoomRole::Finale:RoomRole::Lesson;
  g.room_name=finale?Text{"Pierwszy kurs od dwudziestu lat","The first service in twenty years"}:Text{"Proszę odsunąć się od krawędzi","Please stand back from the edge"};
  std::vector<Pad> path=finale?
    std::vector<Pad>{{3,56,16},{27,53,12,'h'},{49,50,14},{71,47,12},{91,44,14},{113,41,12,'h'},
      {135,38,16},{158,35,12},{178,32,10},{160,29,8},{145,26,11},{127,23,12},{108,20,12},{88,17,16}}:
    std::vector<Pad>{{3,55,18},{29,53,12,'h'},{51,50,14},{73,48,12,'h'},{95,45,14},{117,42,12,'v'},{137,38,20}};
  for(size_t i=0;i<path.size();++i) {
    auto p=path[i];
    if(p.kind=='h'||p.kind=='v')g.platforms.push_back({{double(p.x),double(p.y),double(p.w),.5},p.kind=='h'?2.8:0.,p.kind=='v'?1.8:0.,4.4,double(i%3)/3});
    else g.add_solid_rect(p.x,p.y,p.w,2);
    g.route_points.push_back({p.x+p.w*.5,double(p.y)});
    if(p.kind=='p')g.checkpoint_points.push_back({p.x+2.,p.y-kPlayerHeight});
    if(i%2)g.sweets.push_back({p.x+p.w*.5,p.y-2.});
    if(i%3==2)g.refills.push_back({p.x-2.,p.y-2.5});
  }
  g.spawn_x=5;g.spawn_y=path.front().y-kPlayerHeight;auto last=path.back();g.goal={last.x+last.w-3.,last.y-4.,2,4};
  g.add_hazard_rect(0,62,g.width(),2);
  if(finale) {
    g.goal_requires=7;
    g.interactions.push_back({{53,46,3,4},InteractionKind::Valve,0,{"Zasil rozjazd zachodni","Power the west junction"}});
    g.interactions.push_back({{140,34,3,4},InteractionKind::Valve,1,{"Zwolnij hamulec toru","Release the track brake"}});
    g.interactions.push_back({{130,19,3,4},InteractionKind::Valve,2,{"Połącz linię do archiwum","Connect the archive line"}});
    g.gates.push_back({{100,12,1,5},7});
    g.add_solid_rect(157,18,8,1);g.sweets.push_back({160,16});g.refills.push_back({169,22});
  } else {
    g.interactions.push_back({{7,51,3,4},InteractionKind::Note,6,{"Przeczytaj tabliczkę","Read sign"}});
    g.interactions.push_back({{55,46,3,4},InteractionKind::Note,7,{"Przeczytaj tabliczkę","Read sign"}});
    g.interactions.push_back({{98,41,3,4},InteractionKind::Note,3,{"Przeczytaj tabliczkę","Read sign"}});
    g.add_solid_rect(64,35,8,1);g.sweets.push_back({67,33});g.refills.push_back({63,40});
  }
  return g;
}
