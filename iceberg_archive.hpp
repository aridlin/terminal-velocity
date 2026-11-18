#pragma once
inline EngineGrid build_archive_settlement() {
  EngineGrid g(134,58);g.adventure_level=true;g.event_level=false;g.district=3;g.template_id=103;
  g.room_role=RoomRole::Settlement;g.room_name=Districts[3].name;
  g.add_solid_rect(0,48,134,10);g.add_solid_rect(22,38,20,2);g.add_solid_rect(58,32,20,2);g.add_solid_rect(94,38,21,2);
  g.platforms.push_back({{45,41,9,.5},0,4,5.6,0});g.sweets={{63,30},{71,30}};
  g.spawn_x=6;g.spawn_y=48-kPlayerHeight;g.checkpoint_points={{6,g.spawn_y}};g.route_points={{6,48},{124,48}};g.goal={125,44,3,4};
  g.interactions.push_back({{13,44,4,4},InteractionKind::Npc,4,{"Porozmawiaj","Talk"}});
  g.interactions.push_back({{70,44,4,4},InteractionKind::Npc,7,{"Posłuchaj","Listen"}});
  g.interactions.push_back({{116,44,4,4},InteractionKind::Board,3,{"Indeks tras","Route index"}});
  return g;
}
inline EngineGrid build_archive_sequence(bool finale) {
  EngineGrid g(finale?196:166,64);g.adventure_level=true;g.event_level=false;g.district=3;
  g.template_id=78+int(finale);g.room_role=finale?RoomRole::Finale:RoomRole::Lesson;
  g.room_name=finale?Text{"Oryginał bez skreśleń","The unaltered original"}:Text{"Magazyn ruchomych półek","The moving-stack repository"};
  std::vector<Pad> path=finale?
    std::vector<Pad>{{3,57,18},{27,54,14,'v'},{47,50,15},{68,47,15},{89,44,14},{109,41,14,'h'},
      {129,38,16},{151,35,15},{172,32,16},{153,29,12},{135,26,12},{116,23,16},{94,20,16}}:
    std::vector<Pad>{{3,54,18},{27,51,14,'h'},{47,48,15},{68,45,14,'v'},{88,41,16},{110,38,15},{131,35,20}};
  for(size_t i=0;i<path.size();++i) {
    auto p=path[i];
    if(p.kind=='h'||p.kind=='v')g.platforms.push_back({{double(p.x),double(p.y),double(p.w),.5},p.kind=='h'?2.4:0.,p.kind=='v'?1.8:0.,4.8,double(i%3)*.27});
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
    g.interactions.push_back({{51,46,3,4},InteractionKind::Valve,0,{"Odblokuj katalog","Release the catalogue"}});
    g.interactions.push_back({{132,34,3,4},InteractionKind::Valve,1,{"Rozsuń szafę planów","Open the map cabinet"}});
    g.gates.push_back({{169,26,1,6},3});
    g.interactions.push_back({{177,28,3,4},InteractionKind::Evidence,0,{"Zabierz oryginalny plan","Take the original plan"}});
    g.gates.push_back({{106,14,1,6},7});
    g.add_solid_rect(69,35,8,1);g.add_solid_rect(84,30,8,1);g.sweets.push_back({88,28});g.refills.push_back({80,37});
  } else {
    g.goal_requires=1;
    g.interactions.push_back({{8,50,3,4},InteractionKind::Note,8,{"Przeczytaj tabliczkę","Read sign"}});
    g.interactions.push_back({{51,44,3,4},InteractionKind::Note,9,{"Przeczytaj tabliczkę","Read sign"}});
    g.interactions.push_back({{92,37,3,4},InteractionKind::Valve,0,{"Zwolnij zasuwę","Release the shutter"}});
    g.gates.push_back({{126,28,1,7},1});
    g.add_solid_rect(53,37,8,1);g.sweets.push_back({56,35});g.refills.push_back({63,40});
  }
  return g;
}
