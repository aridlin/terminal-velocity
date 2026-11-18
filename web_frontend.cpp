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
EM_JS(void, browser_ui, (int flags,int audio_ready,int clips,const char *error), {
  if(window.tvUI)window.tvUI({flags,audioReady:audio_ready,clips,error:UTF8ToString(error)});
});
EM_JS(void, browser_save_ready, (), {
  if(window.tvSaveReady)window.tvSaveReady();
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
  const bool district_travel=v.map&&!v.map_route&&!v.map_room&&iceberg::district_travel_available(v,v.selected);
  int flags=int(s.menu)|(int(s.paused)<<1)|(int(v.dialogue.active())<<2)|(int(v.inspecting)<<3)|
    (int(v.settings)<<4)|(int(v.confirm_new)<<5)|(int(s.journal)<<6)|(int(v.map)<<7)|
    (int(v.can_continue)<<8)|(int(v.language==iceberg::Language::English)<<9)|(int(s.won)<<10)|
    (int(v.map&&v.map_route)<<11)|(int(v.pause_on_focus_loss)<<12)|(int(v.mode!=iceberg::Mode::Campaign)<<13)|
    (int(district_control)<<14)|(int(s.audio_muted)<<15)|(int(v.save_error)<<16)|(int(v.map&&v.map_room)<<17)|
    (int(fork_door)<<18)|(int(s.menu&&v.confirm_new&&v.selected!=1)<<19)|
    ((web_slot-1)<<20)|(int(v.tour_scene>=0)<<22)|(int(v.save_picker||v.tour_picker)<<23)|(int(v.smooth_camera)<<24)|(int(district_travel)<<25)|
    (int(!s.menu&&v.dialogue.choosing())<<26);
  browser_ui(flags,web_game->audio.ready(),int(web_game->audio.loaded_clips()),web_game->audio.error().c_str());
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
EMSCRIPTEN_KEEPALIVE void web_tick(double dt,unsigned held,unsigned pressed,int cols,int rows,int focused) {
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
  c.interact=p(6);c.inspect=p(7);c.confirm=p(8);c.pause=p(9);c.journal=p(10);c.map=p(11);
  c.menu_request=p(12);c.restart=p(13);c.new_run=p(14);c.language=p(15);c.mute_audio=p(16);
  c.navigate=int(p(3))-int(p(2));
  for(int i=0;i<7;++i)if(p(20+i))c.select_level=i+1;
  const auto last_save=web_game->save_revision;
  const int previous_slot=web_slot;
  web_game->tick(c,std::isfinite(dt)?dt:0,focused!=0,std::clamp(cols,34,160),std::clamp(rows,cols>=56?20:24,60),2);
  if(web_game->legacy_requested) {
    web_game->legacy_requested=false;web_game->running=true;web_game->state.menu=true;
    web_game->notice(web_game->t("Trasy klasyczne są dostępne w wydaniu na komputer.","Classic courses are available in the desktop edition."),6);
  }
  web_slot=web_game->save_slot;
  RenderGrid(web_game->grid,web_game->state,std::clamp(rows,cols>=56?20:24,60),std::clamp(cols,34,160),2).draw();send_web_ui();
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
EMSCRIPTEN_KEEPALIVE void web_set_language(int language) {
  if(!web_game||(language!=0&&language!=1))return;
  web_game->profile.language=iceberg::Language(language);
  if(web_game->state.adventure.dialogue.active()){web_game->state.adventure.dialogue.segment=0;web_game->state.adventure.dialogue.reset_reveal();web_game->frame_dialogue();}
  web_game->persist();web_game->sync_view();send_web_ui();
}
EMSCRIPTEN_KEEPALIVE int web_import_save() {
  iceberg::Profile imported;
  const auto directory=std::filesystem::path(web_save_base).parent_path();
  if(!iceberg::SaveStore::validate_file(directory/"import.iceberg",imported))return 0;
  // A valid input with a failed replacement is different from an invalid file.
  if(!web_game||web_game->tour_scene>=0||!web_game->saves.save(imported))return -1;
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
  if(web_game){web_account_was_paused=web_game->state.paused;web_game->state.paused=!web_game->state.menu;web_game->audio.update(AudioScene::Paused,web_game->music_track(),.05,true);}
}
EMSCRIPTEN_KEEPALIVE void web_account_resume() {
  if(web_game)web_game->state.paused=web_account_was_paused;
}
}
int main(){return 0;}
