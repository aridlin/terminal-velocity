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
inline Rect crown_core_area(int hits) {
  auto core=crown_core;
  if(hits==2)core.y-=3.6;
  return core;
}
enum class CrownBeat { Guard, Warning, Strike, Exposed };

inline constexpr double crown_warning_time = .72;
inline constexpr double crown_strike_time = 1.38;
inline constexpr double crown_follow_warning = 1.78;
inline constexpr double crown_follow_strike = 2.32;
inline constexpr double crown_last_warning = 2.69;
inline constexpr double crown_last_strike = 3.23;
inline constexpr double crown_cooldown = .42;

inline double crown_open_time(int hits) { return hits==0?1.85:hits==1?2.63:3.49; }
inline double crown_close_time(int hits) { return crown_open_time(hits)+(hits==0?1.10:hits==1?.88:.64); }

inline CrownBeat crown_beat(const CrownEncounter &boss) {
  const double t=boss.time;
  if(boss.hits>=3)return CrownBeat::Guard;
  if(t>=crown_open_time(boss.hits)&&t<crown_close_time(boss.hits))return CrownBeat::Exposed;
  if((t>=crown_strike_time&&t<1.66)||(boss.hits>0&&t>=crown_follow_strike&&t<2.58)||
     (boss.hits==2&&t>=crown_last_strike&&t<3.48))
    return CrownBeat::Strike;
  if((t>=crown_warning_time&&t<crown_strike_time)||
     (boss.hits>0&&t>=crown_follow_warning&&t<crown_follow_strike)||
     (boss.hits==2&&t>=crown_last_warning&&t<crown_last_strike))return CrownBeat::Warning;
  return CrownBeat::Guard;
}

inline std::array<Rect,2> crown_attack_areas(const CrownEncounter &boss) {
  if(boss.hits==0)return {{{0,46.15,54,1.85},{}}};
  const bool follow=boss.time>=crown_follow_warning;
  const bool final_wave=boss.hits==2&&boss.time>=crown_last_warning;
  const double width=final_wave?6.2:follow?5.2:4.0;
  const Rect beam{boss.target-width*.5,33,width,15};
  const Rect floor{0,46.15,54,1.85};
  if(final_wave||(boss.hits==1&&follow))return {{beam,floor}};
  if(boss.hits==1)return {{beam,{}}};
  const bool right=(boss.cycle%2!=0)!=follow;
  return {{beam,right?Rect{27,46.15,27,1.85}:Rect{0,46.15,27,1.85}}};
}

inline void step_crown_sentinel(GameState &state,double dt,bool dashing) {
  auto &boss=state.crown;
  if(boss.hits>=3)return;
  const auto before=crown_beat(boss);
  boss.time+=dt;
  if(boss.time>=crown_close_time(boss.hits)+crown_cooldown) {
    boss.time=0.;
    ++boss.cycle;
  }
  const auto beat=crown_beat(boss);
  if(beat==CrownBeat::Warning&&before!=beat) {
    const double margin=boss.time>=crown_last_warning?3.1:boss.time>=crown_follow_warning?2.6:2.;
    boss.target=clamp(state.player.x+kPlayerWidth*.5,margin,54.-margin);
  }
  if(beat==CrownBeat::Strike) {
    for(const auto &attack:crown_attack_areas(boss))
      if(attack.w>0.&&state.player.rect().intersects(attack)) {
        ++state.deaths;
        reset_player(state);
        return;
      }
  }
  if(beat==CrownBeat::Exposed&&dashing&&state.player.rect().intersects(crown_core_area(boss.hits))) {
    ++boss.hits;
    boss.time=0.;
    boss.cycle=boss.hits;
    state.player.dash_available=true;
  }
}
