#define TERMINAL_VELOCITY_NO_TUI_MAIN
#include "main.cpp"
#include <emscripten.h>

namespace {
std::unique_ptr<AdventureSession> web_game;
std::vector<uint32_t> web_cells;
bool web_account_was_paused=false;
EM_JS(void, browser_frame, (const uint32_t *cells,int cols,int rows), {
  if(window.tvFrame)window.tvFrame(HEAPU32.subarray(cells>>>2,(cells>>>2)+cols*rows*4),cols,rows);
});
EM_JS(void, browser_ui, (int flags,int audio_ready,int clips,const char *error), {
  if(window.tvUI)window.tvUI({flags,audioReady:audio_ready,clips,error:UTF8ToString(error)});
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
  int flags=int(s.menu)|(int(s.paused)<<1)|(int(v.dialogue.active())<<2)|(int(v.inspecting)<<3)|
    (int(v.settings)<<4)|(int(v.confirm_new)<<5)|(int(s.journal)<<6)|(int(v.map)<<7)|
    (int(v.can_continue)<<8)|(int(v.language==iceberg::Language::English)<<9)|(int(s.won)<<10);
  browser_ui(flags,web_game->audio.ready(),int(web_game->audio.loaded_clips()),web_game->audio.error().c_str());
}
}
extern "C" {
EMSCRIPTEN_KEEPALIVE void web_start(unsigned seed) {
  iceberg::SessionOptions options;options.seed=seed;options.mute=false;
  web_game=std::make_unique<AdventureSession>(options,"/persist/progress.dat.iceberg");
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
  if(!web_game->audio.ready())web_game->notice(web_game->t("Dźwięk niedostępny w tej przeglądarce.","Audio is unavailable in this browser."),8);
}
EMSCRIPTEN_KEEPALIVE void web_tick(double dt,unsigned held,unsigned pressed,int cols,int rows,int focused) {
  if(!web_game)return;
  auto h=[&](int key){return bool(held&(1u<<key));};auto p=[&](int key){return bool(pressed&(1u<<key));};
  Controls c;c.move=int(h(1))-int(h(0));c.aim_y=int(h(3))-int(h(2));
  c.jump_held=h(4)||h(2);c.jump_pressed=p(4)||p(2);c.dash_pressed=p(5);c.fast_fall=h(3);
  c.interact=p(6);c.inspect=p(7);c.confirm=p(8);c.pause=p(9);c.journal=p(10);c.map=p(11);
  c.menu_request=p(12);c.restart=p(13);c.new_run=p(14);c.language=p(15);c.mute_audio=p(16);
  c.navigate=int(p(3))-int(p(2));
  for(int i=0;i<6;++i)if(p(20+i))c.select_level=i+1;
  web_game->tick(c,std::isfinite(dt)?dt:0,focused!=0,std::clamp(cols,34,160),std::clamp(rows,cols>=56?20:24,60),2);
  if(web_game->legacy_requested) {
    web_game->legacy_requested=false;web_game->running=true;web_game->state.menu=true;
    web_game->notice(web_game->t("Trasy klasyczne są dostępne w wydaniu na komputer.","Classic courses are available in the desktop edition."),6);
  }
  RenderGrid(web_game->grid,web_game->state,std::clamp(rows,cols>=56?20:24,60),std::clamp(cols,34,160),2).draw();send_web_ui();
}
EMSCRIPTEN_KEEPALIVE void web_save() {if(web_game)web_game->persist();}
EMSCRIPTEN_KEEPALIVE int web_import_save() {
  iceberg::Profile imported;
  if(!iceberg::SaveStore::validate_file("/persist/import.iceberg",imported))return 0;
  // Validate before replacing anything. SaveStore preserves the prior profile.
  if(!web_game||!web_game->saves.save(imported))return 0;
  web_game.reset();web_start(imported.seed);return 1;
}
EMSCRIPTEN_KEEPALIVE void web_guest_profile() {
  if(!web_game)return;
  iceberg::Profile fresh;fresh.seed=unsigned(std::chrono::system_clock::now().time_since_epoch().count());
  web_game->saves.save(fresh);web_game.reset();web_start(fresh.seed);
}
EMSCRIPTEN_KEEPALIVE void web_account_pause() {
  if(web_game){web_account_was_paused=web_game->state.paused;web_game->state.paused=!web_game->state.menu;web_game->audio.update(AudioScene::Paused,web_game->music_track(),.05,true);}
}
EMSCRIPTEN_KEEPALIVE void web_account_resume() {
  if(web_game)web_game->state.paused=web_account_was_paused;
}
}
int main(){return 0;}
