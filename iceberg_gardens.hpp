#pragma once
inline EngineGrid build_garden_settlement() {
  EngineGrid g(146,62);g.adventure_level=true;g.event_level=false;g.district=4;g.template_id=104;
  g.room_role=RoomRole::Settlement;g.room_name=Districts[4].name;
  g.add_solid_rect(0,50,146,12);g.add_solid_rect(27,39,21,2);g.add_solid_rect(69,33,23,2);g.add_solid_rect(113,39,21,2);
  g.platforms.push_back({{51,43,12,.5},0,5,6.5,0});g.sweets={{74,31},{83,31}};
  g.spawn_x=6;g.spawn_y=50-kPlayerHeight;g.checkpoint_points={{6,g.spawn_y}};g.route_points={{6,50},{135,50}};g.goal={136,46,3,4};
  g.interactions.push_back({{15,46,4,4},InteractionKind::Npc,5,{"Porozmawiaj","Talk"}});
  g.interactions.push_back({{83,46,4,4},InteractionKind::Npc,6,{"Porozmawiaj","Talk"}});
  g.interactions.push_back({{126,46,4,4},InteractionKind::Board,4,{"Trasy ogrodu","Garden routes"}});
  return g;
}
inline EngineGrid build_garden_sequence(bool finale) {
  EngineGrid g(finale?226:176,66);g.adventure_level=true;g.event_level=false;g.district=4;
  g.template_id=80+int(finale);g.room_role=finale?RoomRole::Finale:RoomRole::Lesson;
  g.room_name=finale?Text{"Serce żywego obiegu","Heart of the living circuit"}:Text{"Most, który trzeba podlać","The bridge that needs watering"};
  std::vector<Pad> path=finale?
    std::vector<Pad>{{3,58,20},{29,55,15,'g'},{50,52,21},{77,49,15,'v'},{98,45,20},{124,42,15,'g'},
      {145,39,24},{175,36,16},{197,33,20},{177,30,14},{157,27,14,'g'},{137,24,14},{117,21,20}}:
    std::vector<Pad>{{3,55,21},{30,52,16,'g'},{52,48,18},{76,45,18},{100,42,16,'g'},{122,38,18},{146,35,20}};
  int grown=0;
  for(size_t i=0;i<path.size();++i) {
    auto p=path[i];
    if(p.kind=='g')g.growth.push_back({{double(p.x),double(p.y),double(p.w),2},uint64_t(1)<<grown++});
    else if(p.kind=='v')g.platforms.push_back({{double(p.x),double(p.y),double(p.w),.5},0,1.8,5.2,.2});
    else g.add_solid_rect(p.x,p.y,p.w,2);
    g.route_points.push_back({p.x+p.w*.5,double(p.y)});
    if(p.kind=='p')g.checkpoint_points.push_back({p.x+2.,p.y-kPlayerHeight});
    if(i%2)g.sweets.push_back({p.x+p.w*.5,p.y-2.});
    if(i%3==2)g.refills.push_back({p.x-2.,p.y-2.5});
  }
  g.spawn_x=6;g.spawn_y=path.front().y-kPlayerHeight;auto last=path.back();g.goal={last.x+last.w-3.,last.y-4.,2,4};
  g.add_hazard_rect(0,64,g.width(),2);g.goal_requires=finale?7:3;
  g.interactions.push_back({{12,path.front().y-4.,3,4},InteractionKind::Valve,0,{"Podaj wodę do korzeni","Water the roots"}});
  if(finale) {
    g.interactions.push_back({{104,41,3,4},InteractionKind::Valve,1,{"Otwórz drugi kanał","Open the second channel"}});
    g.interactions.push_back({{204,29,3,4},InteractionKind::Valve,2,{"Połącz powrót z Koroną","Connect the Crown return"}});
    g.enemies.push_back({151,161,38,2.1});
    g.add_solid_rect(153,30,8,1);g.sweets.push_back({156,28});g.refills.push_back({168,32});
    g.gates.push_back({{128,15,1,6},7});
  } else {
    g.interactions.push_back({{7,51,3,4},InteractionKind::Note,20,{"Notatka Celiny","Celina's note"}});
    g.interactions.push_back({{81,41,3,4},InteractionKind::Valve,1,{"Napełnij drugi zbiornik","Fill the second reservoir"}});
    g.interactions.push_back({{58,44,3,4},InteractionKind::Note,21,{"Notatka Celiny","Celina's note"}});
    g.add_solid_rect(55,37,8,1);g.springs.push_back({62,47.7,2,.3});g.sweets.push_back({58,35});g.refills.push_back({69,38});
    g.gates.push_back({{160,29,1,6},3});
  }
  return g;
}
