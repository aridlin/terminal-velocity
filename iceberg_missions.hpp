#pragma once
// Fixed story culminations for the harbor's three expeditions.
// Included inside iceworld, after Pad and the general room builders.
inline EngineGrid build_harbor_mission(int mission) {
  EngineGrid g(mission==2?236:mission==1?220:164,64);
  g.adventure_level=true;g.event_level=false;g.district=0;
  g.room_role=RoomRole::Interlude;g.template_id=300+mission;
  static const Text names[]={
    {"Ostatni kurs pod wodą","Last service underwater"},
    {"Przesyłka: nie przewracać","Parcel: keep upright"},
    {"Śluza bez właściciela","The unclaimed sluice"}
  };
  g.room_name=names[mission];
  std::vector<Pad> path;
  if(mission==0)path={{3,20,16},{27,24,14},{49,28,14},{71,32,14},{93,36,14},{115,40,14},{137,44,18},
    {116,48,14},{95,52,14},{73,55,16},{51,52,14},{29,48,14},{7,44,17}};
  else if(mission==1)path={{3,52,18},{24,50,13},{40,48,13},{56,46,14},{73,44,14},{90,42,14},
    {107,40,14},{124,38,14},{141,36,14},{158,34,14},{175,32,14},{192,30,18}};
  else path={{3,55,20},{29,53,18,'m'},{56,50,16},{82,47,18,'m'},{114,44,18},{140,41,18,'m'},{172,38,24},{205,35,18}};
  for(size_t i=0;i<path.size();++i) {
    auto p=path[i];
    if(p.kind=='m')g.platforms.push_back({{double(p.x),double(p.y),double(p.w),.5},6,0,5.,double(i%3)/3});
    else g.add_solid_rect(p.x,p.y,p.w,2);
    g.route_points.push_back({p.x+p.w*.5,double(p.y)});
    if(p.kind!='m')g.checkpoint_points.push_back({p.x+2.,p.y-kPlayerHeight});
    if(i%3==1)g.sweets.push_back({p.x+p.w*.5,p.y-2.2});
  }
  g.spawn_x=5;g.spawn_y=path.front().y-kPlayerHeight;
  auto final=path.back();g.goal={final.x+final.w-3.,final.y-4.,2,4};
  g.add_hazard_rect(0,62,g.width(),2);
  if(mission==0) {
    g.interactions.push_back({{98,32,3,4},InteractionKind::Valve,0,{"Uruchom wyciąg","Start the winch"}});
    g.interactions.push_back({{144,40,3,4},InteractionKind::Rescue,2,{"Pomóż Bogdanowi","Help Bogdan"}});
    g.goal_requires=3;g.gates.push_back({{21,39,1,5},3});
    g.add_solid_rect(125,30,9,1);g.refills.push_back({128,34});g.sweets.push_back({129,28});
  } else if(mission==1) {
    g.interactions.push_back({{8,48,3,4},InteractionKind::Carry,0,{"Weź termometr","Take the thermometer"}});
    g.interactions.push_back({{201,26,3,4},InteractionKind::Deliver,1,{"Dostarcz przesyłkę","Deliver the parcel"}});
    g.goal_requires=2;g.dash_lock=1;g.dash_unlock=2;g.gates.push_back({{207,25,1,5},2});
    for(int x:{42,92,143})g.add_solid_rect(x,18,4,3);
  } else {
    g.interactions.push_back({{61,46,3,4},InteractionKind::Valve,0,{"Otwórz śluzę dolną","Open the lower sluice"}});
    g.interactions.push_back({{120,40,3,4},InteractionKind::Valve,1,{"Otwórz śluzę górną","Open the upper sluice"}});
    g.goal_requires=3;g.gates.push_back({{220,30,1,5},3});
    g.add_solid_rect(182,26,11,1);g.refills.push_back({176,30});g.sweets.push_back({187,24});
  }
  return g;
}
