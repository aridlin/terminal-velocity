#pragma once
// A shared graduation course: momentum jump, wall-bounce shaft and lift launch.
inline constexpr Text MasteryCourseName{"Egzamin Kuby","Kuba's examination"};

inline EngineGrid build_mastery_course(int district) {
  EngineGrid g(144,64);g.adventure_level=true;g.event_level=false;g.template_id=320;g.district=district;g.room_role=RoomRole::Finale;
  g.room_name=MasteryCourseName;
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

// A retired Harbor inspection route. The buoy lights double as return lamps.
inline EngineGrid build_survey_course() {
  EngineGrid g(134,62);
  g.adventure_level=true;g.district=0;g.template_id=iceberg::BreakwaterSurvey;g.room_role=iceberg::RoomRole::Lesson;
  g.room_name={"Objazd latarni przyboju","Breakwater survey"};
  g.add_solid_rect(0,51,20,11);
  g.add_solid_rect(39,47,12,2);
  g.add_solid_rect(63,50,8,4);
  g.add_solid_rect(68,42,3,8);
  g.add_solid_rect(76,37,12,2);
  g.add_solid_rect(114,34,20,3);
  g.water_basins.push_back({20,54,114,8});
  g.add_hazard_rect(20,55,114,7);
  g.platforms.push_back({{23,51,11,.5},3,3,4.3,0});
  g.platforms.push_back({{53,46,10,.5},2,2,4.8,.25});
  g.platforms.push_back({{94,37,12,.5},3,2,5.,.5});
  g.springs.push_back({66,49.,1.5,.3});
  g.refills={{61,43},{90,33},{109,31}};
  g.spawn_x=6;g.spawn_y=51-kPlayerHeight;
  g.checkpoint_points={{6,g.spawn_y},{42,47-kPlayerHeight},{78,37-kPlayerHeight}};
  g.route_points={{8,51},{45,47},{69,42},{81,37},{120,34}};
  g.interactions.push_back({{119,30,3,4},InteractionKind::Valve,0,{"Oznacz zgaszoną boję","Mark the dark buoy"}});
  g.goal={128,30,3,4};
  g.goal_requires=1;
  return g;
}
