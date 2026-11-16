#pragma once
// A shared graduation course: momentum jump, wall-bounce shaft and lift launch.
inline EngineGrid build_mastery_course(int district) {
  EngineGrid g(144,64);g.adventure_level=true;g.event_level=false;g.template_id=320;g.district=district;g.room_role=RoomRole::Finale;
  g.room_name={"Egzamin Kuby","Kuba's examination"};
  g.add_solid_rect(3,55,18,2);g.add_solid_rect(40,55,15,2);
  g.add_solid_rect(63,31,2,26);g.add_solid_rect(53,43,10,1);g.add_solid_rect(43,33,13,1);
  g.platforms.push_back({{72,30,18,.5},0,3,3.,.5});g.add_solid_rect(95,20,15,2);g.add_solid_rect(120,16,18,2);
  g.refills={{62,49},{61,37},{94,20}};
  g.sweets={{48,51},{57,39},{48,29},{101,16}};
  g.spawn_x=5;g.spawn_y=55-kPlayerHeight;g.goal={134,12,2,4};g.add_hazard_rect(0,62,144,2);
  g.checkpoint_points={{5,g.spawn_y},{42,55-kPlayerHeight},{54,43-kPlayerHeight},{45,33-kPlayerHeight},{97,20-kPlayerHeight}};
  g.interactions.push_back({{7,51,3,4},InteractionKind::Note,1,{"Notatka Kuby","Kuba's note"}});
  g.interactions.push_back({{44,51,3,4},InteractionKind::Note,2,{"Notatka Kuby","Kuba's note"}});
  g.interactions.push_back({{46,29,3,4},InteractionKind::Note,5,{"Notatka Kuby","Kuba's note"}});
  return g;
}
