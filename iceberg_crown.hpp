#pragma once
// The Crown uses exposed maintenance cradles and beacon counterweights.
// Every usable deck is real collision geometry shared by all frontends.
inline EngineGrid build_crown_settlement() {
  EngineGrid g(158,66);g.adventure_level=true;g.event_level=false;g.district=5;g.template_id=105;
  g.room_role=RoomRole::Settlement;g.room_name=Districts[5].name;
  g.add_solid_rect(0,51,158,15);g.add_solid_rect(32,40,23,2);g.add_solid_rect(88,34,25,2);
  g.platforms.push_back({{61,44,17,.5},0,3,6,0});g.sweets={{95,32},{103,32}};
  g.spawn_x=6;g.spawn_y=51-kPlayerHeight;g.checkpoint_points={{6,g.spawn_y}};
  g.route_points={{6,51},{146,51}};g.goal={148,47,3,4};
  g.interactions.push_back({{16,47,4,4},InteractionKind::Npc,6,{"Porozmawiaj","Talk"}});
  g.interactions.push_back({{72,47,4,4},InteractionKind::Npc,4,{"Porozmawiaj","Talk"}});
  g.interactions.push_back({{136,47,4,4},InteractionKind::Board,5,{"Droga do latarni","The beacon route"}});
  return g;
}
inline EngineGrid build_crown_sequence(bool finale) {
  EngineGrid g(finale?280:194,86);g.adventure_level=true;g.event_level=false;g.district=5;
  g.template_id=82+int(finale);g.room_role=finale?RoomRole::Finale:RoomRole::Lesson;
  g.room_name=finale?Text{"Latarnia wszystkich dzielnic","Beacon of every district"}:Text{"Podwieszona droga","The suspended route"};
  std::vector<Pad> path=finale?
    std::vector<Pad>{{3,77,23},{32,74,16},{54,70,18,'v'},{78,66,20},{104,62,17},
      {127,58,21},{154,54,17,'h'},{177,50,20},{203,46,17},{226,42,22},
      {254,38,20},{231,34,16},{209,30,16},{183,26,20},{154,22,23}}:
    std::vector<Pad>{{3,65,23},{32,62,17,'h'},{55,59,20},{81,55,17,'v'},
      {104,51,21},{131,47,17},{154,43,25}};
  for(size_t i=0;i<path.size();++i) {
    auto p=path[i];
    if(p.kind=='h'||p.kind=='v')g.platforms.push_back({{double(p.x),double(p.y),double(p.w),.5},p.kind=='h'?2.8:0,p.kind=='v'?1.8:0,5.8,0});
    else {g.add_solid_rect(p.x,p.y,p.w,2);if(i%3==0)g.checkpoint_points.push_back({p.x+2.,p.y-kPlayerHeight});}
    g.route_points.push_back({p.x+p.w*.5,double(p.y)});
    if(i%2)g.sweets.push_back({p.x+p.w*.5,p.y-2.});
    if(i%3==2)g.refills.push_back({p.x-2.,p.y-2.5});
  }
  g.spawn_x=6;g.spawn_y=path.front().y-kPlayerHeight;
  auto last=path.back();g.goal={last.x+last.w-3.,last.y-4.,2,4};g.add_hazard_rect(0,84,g.width(),2);
  if(finale) {
    g.air_fields.push_back({{106,20,142,47},-5,0,5.5,0,2});
    g.goal_requires=7;
    for(auto [index,bit]:{std::pair{0,0},std::pair{5,1},std::pair{14,2}}) {
      auto p=path[size_t(index)];
      const Text labels[]={{"Otwórz dolną przesłonę","Open the lower shutter"},{"Wyrównaj obieg mieszkań","Balance the residential circuit"},{"Zapal latarnię","Light the beacon"}};
      g.interactions.push_back({{p.x+7.,p.y-4.,3,4},InteractionKind::Valve,bit,labels[bit]});
    }
    g.gates.push_back({{174,16,1,6},7});
  } else {
    g.air_fields.push_back({{57,45,49,18},6,0,6.,0,2});
    g.interactions.push_back({{7,61,3,4},InteractionKind::Note,22,{"Notatka latarnika","Beacon keeper's note"}});
    g.goal_requires=3;
    g.interactions.push_back({{12,61,3,4},InteractionKind::Valve,0,{"Zwolnij wózki serwisowe","Release the service cradles"}});
    g.interactions.push_back({{112,47,3,4},InteractionKind::Valve,1,{"Otwórz przesłonę grani","Open the ridge shutter"}});
    g.gates.push_back({{176,37,1,6},3});
  }
  return g;
}
inline EngineGrid build_epilogue() {
  EngineGrid g(204,62);g.adventure_level=true;g.event_level=false;g.district=0;g.template_id=330;
  g.room_role=RoomRole::Settlement;g.room_name={"Następnego ranka","The following morning"};
  g.add_solid_rect(0,49,204,13);g.add_solid_rect(52,39,22,2);g.add_solid_rect(116,35,25,2);
  g.spawn_x=6;g.spawn_y=49-kPlayerHeight;g.checkpoint_points={{6,g.spawn_y}};
  g.route_points={{6,49},{191,49}};g.goal={194,45,3,4};
  const int positions[]={16,38,64,84,108,133,157,178};
  for(int npc=0;npc<8;++npc)g.interactions.push_back({{double(positions[npc]),45,3,4},InteractionKind::Npc,npc,{"Porozmawiaj","Talk"}});
  g.interactions.push_back({{48,45,3,4},InteractionKind::Epilogue,0,{"Posadź nasiona Celiny","Plant Celina's seeds"}});
  g.interactions.push_back({{120,45,3,4},InteractionKind::Epilogue,1,{"Doręcz odpis spisu","Deliver the register copy"}});
  g.interactions.push_back({{190,45,3,4},InteractionKind::Board,6,{"Zakończ obchód","Finish the round"}});
  g.interactions.push_back({{96,45,4,4},InteractionKind::Outfitter,0,{"Sklep portowy","Harbor outfitter"}});
  return g;
}
