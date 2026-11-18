#pragma once

namespace iceworld {
inline EngineGrid build_crown_sentinel() {
  EngineGrid grid(54,62);
  grid.district=5;
  grid.adventure_level=true;
  grid.template_id=iceberg::CrownSentinel;
  grid.room_role=iceberg::RoomRole::Finale;
  grid.room_name={"Strażnik Korony","Crown Sentinel"};
  grid.add_solid_rect(0,48,54,14);
  grid.add_solid_rect(12,44,6,1);
  grid.add_solid_rect(36,44,6,1);
  grid.spawn_x=18;
  grid.spawn_y=48-kPlayerHeight;
  grid.goal={49,44,3,4};
  grid.checkpoint_points={{grid.spawn_x,grid.spawn_y}};
  grid.refills={{21.5,46.6},{32.5,46.6}};
  return grid;
}
}

inline constexpr Rect crown_core{25.75,45.15,2.5,2.55};
inline constexpr double crown_warning_time = .92;
inline constexpr double crown_strike_time = 1.62;
inline constexpr double crown_open_time = 2.10;
inline constexpr double crown_cycle_time = 4.45;

inline std::array<Rect,2> crown_attack_areas(const CrownEncounter &boss) {
  const Rect beam{boss.target-1.35,33,2.7,15};
  if(boss.hits==0)return {{{11,46.15,32,1.85},{}}};
  if(boss.hits==1)return {{beam,{}}};
  return {{beam,boss.cycle%2==0?Rect{9,46.15,18,1.85}:Rect{27,46.15,18,1.85}}};
}

inline void step_crown_sentinel(GameState &state,double dt,bool dashing) {
  auto &boss=state.crown;
  if(boss.hits>=3)return;
  const double before=boss.time;
  boss.time+=dt;
  if(boss.time>=crown_cycle_time) {
    boss.time-=crown_cycle_time;
    ++boss.cycle;
  }
  if(before<crown_warning_time&&boss.time>=crown_warning_time)
    boss.target=clamp(state.player.x+kPlayerWidth*.5,4.,50.);
  if(boss.time>=crown_strike_time&&boss.time<crown_open_time) {
    for(const auto &attack:crown_attack_areas(boss))
      if(attack.w>0.&&state.player.rect().intersects(attack)) {
        ++state.deaths;
        reset_player(state);
        return;
      }
  }
  if(boss.time>=crown_open_time&&dashing&&state.player.rect().intersects(crown_core)) {
    ++boss.hits;
    boss.time=0.;
    ++boss.cycle;
    state.player.dash_available=true;
  }
}
