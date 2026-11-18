#pragma once
// Deliberately absent from the public journal, route map, objectives, and counters.
// Entry fixtures are authored locations, independent of expedition randomness.
namespace iceworld {
inline EngineGrid build_hidden(int id) {
  EngineGrid g(78,58);
  g.adventure_level=true;g.event_level=false;g.room_role=iceberg::RoomRole::Secret;
  g.template_id=200+id;g.district=0;
  g.room_name=id==0?iceberg::Text{"Korytarz techniczny","Service corridor"}:
    id==1?iceberg::Text{"Linia wewnętrzna","Internal line"}:iceberg::Text{"Zbiornik wyrównawczy","Equalization reservoir"};
  g.add_solid_rect(0,48,78,10);g.add_solid_rect(0,20,2,28);g.add_solid_rect(76,20,2,28);
  g.spawn_x=6;g.spawn_y=48-kPlayerHeight;g.checkpoint_points={{6,g.spawn_y}};
  g.goal={-20,-20,1,1};
  g.interactions.push_back({{4,44,4,4},InteractionKind::Return,0,{"Wróć","Return"}});
  if(id==0) {
    for(int x:{22,34,46,58})g.add_solid_rect(x,38,8,1);
    g.interactions.push_back({{64,44,4,4},InteractionKind::Note,10,{"Podnieś kartkę","Pick up paper"}});
  } else if(id==1) {
    g.interactions.push_back({{48,44,4,4},InteractionKind::Note,11,{"Naciśnij przycisk","Press button"}});
  } else {
    g.add_solid_rect(32,44,32,4);
    g.interactions.push_back({{56,40,5,4},InteractionKind::Note,12,{"Otwórz protokół","Open the log"}});
    g.springs.push_back({28,47.7,1.5,.3});
  }
  return g;
}
}
