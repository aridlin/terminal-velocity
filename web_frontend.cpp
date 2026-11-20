#define TERMINAL_VELOCITY_NO_TUI_MAIN
#include "main.cpp"
#include <emscripten.h>

namespace {
std::unique_ptr<AdventureSession> web_game;
std::vector<uint32_t> web_cells;
bool web_account_was_paused=false;
const char *web_save_base="/persist/progress.dat.iceberg";
int web_slot=1;
EM_JS(void, browser_frame, (const uint32_t *cells,int cols,int rows), {
  if(window.tvFrame)window.tvFrame(HEAPU32.subarray(cells>>>2,(cells>>>2)+cols*rows*4),cols,rows);
});
EM_JS(void, browser_ui, (int flags,int route_legs,int travel_district,int tour_picker,int awards_open,int audio_ready,int clips,const char *error), {
  if(window.tvUI)window.tvUI({flags,routeLegs:route_legs,travelDistrict:travel_district,tourPicker:!!tour_picker,awardsOpen:!!awards_open,audioReady:audio_ready,clips,error:UTF8ToString(error)});
});
EM_JS(void, browser_save_ready, (), {
  if(window.tvSaveReady)window.tvSaveReady();
});
EM_JS(void, browser_reply_rows, (int first_row), {
  if(window.tvReplyRows)window.tvReplyRows(first_row);
});
EM_JS(void, browser_tour_rows, (int first,int count,int row,int left,int right), {
  if(window.tvTourRows)window.tvTourRows(first,count,row,left,right);
});
EM_JS(void, browser_expedition_nodes, (int first,int count,int row,int left,int stride,int width), {
  if(window.tvExpeditionNodes)window.tvExpeditionNodes(first,count,row,left,stride,width);
});
uint32_t first_codepoint(const std::string &s) {
  if(s.empty())return 32;
  auto byte=[&](size_t i){return uint32_t(static_cast<unsigned char>(s[i]));};
  uint32_t c=byte(0);int n=c<128?1:c<224?2:c<240?3:4;
  if(s.size()<size_t(n))return 0xfffd;
  if(n>1)c&=(1u<<(7-n))-1;
  for(int i=1;i<n;++i)c=(c<<6)|(byte(size_t(i))&63);
  return c;
}
void send_web_ui() {
  const auto &s=web_game->state;const auto &v=s.adventure;
  const bool district_control=s.menu&&!v.settings&&!v.confirm_new&&!v.save_picker&&!v.tour_picker&&(v.selected==2||v.selected==3);
  const bool fork_door=v.branch_at_door&&!s.menu&&!s.paused&&!s.journal&&!v.map&&!v.inspecting&&!v.dialogue.active();
  const int travel_district=!s.menu&&v.map&&!v.map_route&&!v.map_room&&
    iceberg::district_travel_available(v,v.selected)?v.selected:-1;
  int flags=int(s.menu)|(int(s.paused)<<1)|(int(v.dialogue.active())<<2)|(int(v.inspecting)<<3)|
    (int(v.settings)<<4)|(int(v.confirm_new)<<5)|(int(s.journal)<<6)|(int(v.map)<<7)|
    (int(v.can_continue)<<8)|(int(v.language==iceberg::Language::English)<<9)|(int(s.won)<<10)|
    (int(v.map&&v.map_route)<<11)|(int(v.pause_on_focus_loss)<<12)|(int(v.mode!=iceberg::Mode::Campaign)<<13)|
    (int(district_control)<<14)|(int(s.audio_muted)<<15)|(int(v.save_error)<<16)|(int(v.map&&v.map_room)<<17)|
    (int(fork_door)<<18)|(int(s.menu&&v.confirm_new&&v.selected!=1)<<19)|
    ((web_slot-1)<<20)|(int(v.tour_scene>=0)<<22)|(int(v.save_picker||v.tour_picker)<<23)|(int(v.smooth_camera)<<24)|
    (int(!s.menu&&iceberg::browsing_campaign_leg(v))<<25)|(int(!s.menu&&v.dialogue.choosing())<<26);
  if(v.inspecting) {
    const auto layout=iceberg::inspection_layout(v.inspect_title,v.inspect_body,web_game->last_columns,web_game->last_rows);
    const int page=layout.page_at(v.inspect_offset);
    flags|=(int(page>0)<<27)|(int(page+1<layout.pages)<<28);
  }
  int route_legs=0;
  if(!s.menu&&v.map&&!v.map_room&&v.tour_scene<0&&v.mode!=iceberg::Mode::Campaign) {
    const auto [selected,count]=web_game->expedition_route_position();
    flags|=(int(iceberg::browsing_expedition_room(v))<<25)|
      (int(count>0&&selected>0)<<29)|(int(selected+1<count)<<30);
    if(v.mode==iceberg::Mode::Endless&&count>iceberg::RoomsPerLeg) {
      const int leg=selected/iceberg::RoomsPerLeg;
      route_legs=int(leg>0)|(int((leg+1)*iceberg::RoomsPerLeg<count)<<1);
    }
  }
  browser_ui(flags,route_legs,travel_district,s.menu&&v.tour_picker,s.menu&&v.achievement_menu,web_game->audio.ready(),
    int(web_game->audio.loaded_clips()),web_game->audio.error().c_str());
}
}
extern "C" {
EMSCRIPTEN_KEEPALIVE void web_set_start_slot(int slot) {
  if(!web_game&&slot>=1&&slot<=iceberg::SaveStore::SlotCount)web_slot=slot;
}
EMSCRIPTEN_KEEPALIVE void web_use_temporary_saves() {
  if(!web_game)web_save_base="/session-progress/progress.dat.iceberg";
}
EMSCRIPTEN_KEEPALIVE void web_profile_action_busy(int busy) {
  if(web_game)web_game->profile_action_busy=busy!=0;
}
EMSCRIPTEN_KEEPALIVE int web_saved_language() {
  iceberg::Profile saved;
  iceberg::SaveStore store(iceberg::SaveStore::slot_file(web_save_base,web_slot));
  return store.load(saved)?int(saved.language):-1;
}
EMSCRIPTEN_KEEPALIVE void web_start(unsigned seed,int language,int muted) {
  iceberg::SessionOptions options;options.seed=seed;options.mute=muted!=0;options.save_slot=web_slot;
  if(language==0||language==1){options.language=iceberg::Language(language);options.language_override=true;}
  web_game=std::make_unique<AdventureSession>(options,web_save_base);
  cell_frame_sink=[](const CellFrame &frame) {
    int rows=int(frame.size()),cols=rows?int(frame[0].size()):0;
    web_cells.clear();web_cells.reserve(size_t(rows*cols*4));
    for(const auto &row:frame)for(const auto &cell:row) {
      web_cells.push_back(first_codepoint(cell.glyph));web_cells.push_back(cell_color(cell));
      web_cells.push_back(cell_background(cell));web_cells.push_back(cell.bold?1:0);
    }
    browser_frame(web_cells.data(),cols,rows);
  };
  // Called by the start button's user gesture so the browser can unlock audio.
  web_game->audio.update(AudioScene::Menu,2,0,true);
  EM_ASM({console.info("Terminal Velocity audio: ready="+$0+", loaded clips="+$1);},web_game->audio.ready(),int(web_game->audio.loaded_clips()));
  if(!options.mute&&!web_game->audio.ready())web_game->notice(web_game->t("Dźwięk niedostępny w tej przeglądarce.","Audio is unavailable in this browser."),8);
}
EMSCRIPTEN_KEEPALIVE void web_enter_campaign() {
  if(web_game)web_game->begin_campaign(false);
}
EMSCRIPTEN_KEEPALIVE void web_select_tour_stop(int index) {
  if(!web_game||!web_game->state.menu||!web_game->state.adventure.tour_picker||
     index<0||index>=int(iceberg::DebugTour.size()))return;
  web_game->tick({.select_level=index+1},0.,true,
                 web_game->last_columns,web_game->last_rows,web_game->last_aspect);
}
EMSCRIPTEN_KEEPALIVE void web_select_expedition_room(int index) {
  if(!web_game||web_game->state.menu||web_game->state.journal)return;
  auto &view=web_game->state.adventure;
  if(!view.map||view.map_room||view.inspecting||view.dialogue.active()||
     view.mode==iceberg::Mode::Campaign||view.tour_scene>=0)return;
  const auto [selected,count]=web_game->expedition_route_position();
  if(index<0||index>=count||index/iceberg::RoomsPerLeg!=selected/iceberg::RoomsPerLeg)return;
  view.route_room_selection=index;
  web_game->navigation_repeat_direction=0;
  web_game->sync_view();
}
EMSCRIPTEN_KEEPALIVE void web_tick(double dt,unsigned held,unsigned pressed,int cols,int rows,int focused,unsigned dash_aim) {
  if(!web_game)return;
  auto h=[&](int key){return bool(held&(1u<<key));};auto p=[&](int key){return bool(pressed&(1u<<key));};
  auto direction_pressed=[&](int negative,int positive)->std::optional<int>{
    if(h(negative)&&h(positive))return 0;
    if(p(negative)||p(positive))return int(p(positive))-int(p(negative));
    return std::nullopt;
  };
  Controls c;c.move=int(h(1))-int(h(0));c.aim_y=int(h(3))-int(h(2));
  c.move_pressed=direction_pressed(0,1);c.aim_pressed=direction_pressed(2,3);
  c.jump_held=h(4)||h(2);c.jump_pressed=p(4)||p(2);c.dash_pressed=p(5);c.fast_fall=h(3);
  c.fast_fall_pressed=p(3);
  if(c.dash_pressed&&dash_aim>=1&&dash_aim<=9) {
    const int direction=int(dash_aim)-1;
    c.dash_pressed_direction=std::pair{direction%3-1,direction/3-1};
  }
  c.interact=p(6);c.inspect=p(7);c.confirm=p(8);c.pause=p(9);c.journal=p(10);c.map=p(11);
  c.menu_request=p(12);c.restart=p(13);c.new_run=p(14);c.language=p(15);c.mute_audio=p(16);
  c.navigate=int(p(3))-int(p(2));
  c.browse_leg=int(p(1))-int(p(0));
  c.tour_page=int(p(29))-int(p(28));
  if(web_game->state.menu&&web_game->state.adventure.tour_picker) {
    if(p(19))c.new_run=true;
    if(p(27)){c.browse_leg=1;c.confirm=true;}
  }
  const auto &dialogue=web_game->state.adventure.dialogue;
  if(dialogue.choosing()&&!web_game->state.paused&&!web_game->state.menu&&(p(17)||p(18))) {
    const int choice=p(18)?1:0;
    c.navigate=int(dialogue.reply_option!=choice);
    c.confirm=true;
  }
  for(int i=0;i<7;++i)if(p(20+i))c.select_level=i+1;
  const auto &view=web_game->state.adventure;
  if(p(30)&&web_game->state.menu&&!view.settings&&!view.confirm_new&&!view.tour_picker&&
     !view.save_picker&&!view.achievement_menu) {
    c.select_level=7;c.confirm=true;
  }
  const auto last_save=web_game->save_revision;
  const int previous_slot=web_slot;
  web_game->tick(c,std::isfinite(dt)?dt:0,focused!=0,std::clamp(cols,34,160),std::clamp(rows,cols>=56?20:24,60),2);
  if(web_game->legacy_requested) {
    web_game->legacy_requested=false;web_game->running=true;web_game->state.menu=true;
    web_game->notice(web_game->t("Trasy klasyczne są dostępne w wydaniu na komputer.","Classic courses are available in the desktop edition."),6);
  }
  web_slot=web_game->save_slot;
  RenderGrid renderer(web_game->grid,web_game->state,std::clamp(rows,cols>=56?20:24,60),std::clamp(cols,34,160),2);
  renderer.draw();
  browser_reply_rows(renderer.reply_first_row());
  const auto tour=renderer.tour_rows();
  browser_tour_rows(tour.first_sample,tour.count,tour.row,tour.left,tour.right);
  const auto nodes=renderer.expedition_nodes();
  browser_expedition_nodes(nodes.first_room,nodes.count,nodes.row,nodes.left,nodes.stride,nodes.width);
  send_web_ui();
  if(web_game->save_revision!=last_save||web_slot!=previous_slot)browser_save_ready();
}
EMSCRIPTEN_KEEPALIVE int web_save() {
  if(!web_game||web_game->tour_scene>=0)return 0;
  web_game->persist();
  send_web_ui();
  return !web_game->state.adventure.save_error;
}
EMSCRIPTEN_KEEPALIVE int web_primary_usable_for_account() {
  const auto file=iceberg::SaveStore::slot_file(web_save_base,web_slot);
  std::error_code ec;
  const bool exists=std::filesystem::exists(file,ec);
  iceberg::Profile stored;
  // If preservation failed, a blank in-memory profile must not replace a
  // potentially healthy account save.
  return !ec&&(!exists||iceberg::SaveStore::validate_file(file,stored));
}
EMSCRIPTEN_KEEPALIVE int web_export_recovery() {
  // The preceding save attempt has already snapped the live session into profile.
  return web_game&&web_game->tour_scene<0&&iceberg::SaveStore("/tmp/terminal-velocity-export.iceberg").save(web_game->profile);
}
EMSCRIPTEN_KEEPALIVE unsigned web_import_size_limit() {
  return iceberg::SaveStore::MaxBackupBytes;
}
EMSCRIPTEN_KEEPALIVE int web_export_manual() {
  return web_game&&web_game->tour_scene<0&&
    iceberg::SaveStore::write_backup("/tmp/terminal-velocity-manual.iceberg",web_game->profile);
}
EMSCRIPTEN_KEEPALIVE void web_set_language(int language) {
  if(!web_game||(language!=0&&language!=1))return;
  web_game->profile.language=iceberg::Language(language);
  if(auto &dialogue=web_game->state.adventure.dialogue;dialogue.active()) {
    dialogue.segment=0;
    dialogue.line_seconds=0.;
    dialogue.reset_reveal();
    web_game->frame_dialogue();
  }
  web_game->persist();web_game->sync_view();send_web_ui();
}
EMSCRIPTEN_KEEPALIVE int web_validate_import_save() {
  iceberg::Profile imported;
  return iceberg::SaveStore::read_backup("/tmp/terminal-velocity-import-check.iceberg",imported);
}
EMSCRIPTEN_KEEPALIVE int web_import_save() {
  iceberg::Profile imported;
  const auto directory=std::filesystem::path(web_save_base).parent_path();
  if(!iceberg::SaveStore::read_backup(directory/"import.iceberg",imported))return 0;
  // A valid input with a failed replacement is different from an invalid file.
  if(!web_game||web_game->tour_scene>=0||!web_game->saves.save(imported,true))return -1;
  const bool muted=web_game->audio.muted;
  web_game.reset();web_start(imported.seed,-1,muted);return 1;
}
EMSCRIPTEN_KEEPALIVE void web_guest_profile() {
  if(!web_game||web_slot!=1||web_game->tour_scene>=0)return;
  iceberg::Profile fresh;fresh.language=web_game->profile.language;fresh.seed=unsigned(std::chrono::system_clock::now().time_since_epoch().count());
  const bool muted=web_game->audio.muted;
  web_game->saves.save(fresh);web_game.reset();web_start(fresh.seed,-1,muted);
}
EMSCRIPTEN_KEEPALIVE void web_account_pause() {
  if(!web_game)return;
  web_game->discard_pending_movement();
  web_account_was_paused=web_game->state.paused;
  web_game->state.paused=!web_game->state.menu;
  web_game->audio.update(AudioScene::Paused,web_game->music_track(),.05,true);
}
EMSCRIPTEN_KEEPALIVE void web_account_resume() {
  if(web_game)web_game->state.paused=web_account_was_paused;
}
}
int main(){return 0;}
