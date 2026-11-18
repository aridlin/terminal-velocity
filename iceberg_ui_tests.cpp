#define TERMINAL_VELOCITY_NO_TUI_MAIN
#include "main.cpp"
#include <stdexcept>
void ui_check(bool pass,const char *message){if(!pass)throw std::runtime_error(message);std::cout<<"PASS "<<message<<'\n';}
std::string ui_text(const CellFrame &frame){std::string result;for(const auto &row:frame){for(const auto &cell:row)result+=cell.glyph;result+='\n';}return result;}
std::string ui_body(const CellFrame &frame){std::string text;for(const auto &row:frame){for(const auto &cell:row)text+=cell.rgb==0xeee7d6?cell.glyph:" ";text+=' ' ;}return text;}
std::string normalize_ui(const std::string &text){std::istringstream stream(text);std::string word,result;while(stream>>word){if(!result.empty())result+=' ';result+=word;}return result;}
int main(){try {
  iceberg::SessionOptions options;options.no_save=options.mute=true;
  AdventureSession game(options,"/tmp/tv-ui-test-unused");game.begin_campaign(true);
  CellFrame frame;cell_frame_sink=[&](const CellFrame &f){frame=f;};
  Controls enter;enter.confirm=true;Controls book;book.journal=true;Controls map;map.map=true;Controls tab;tab.inspect=true;Controls use;use.interact=true;
  for(auto language:{iceberg::Language::Polish,iceberg::Language::English}) {
    game.profile.language=language;game.sync_view();game.state.adventure.map=true;game.state.adventure.selected=1;
    auto x=game.state.player.x;game.tick(enter,1./120);
    ui_check(game.state.adventure.map&&game.grid.district==0&&game.state.player.x==x,"locked travel keeps map open and courier in place");
    for(auto size:{std::pair{110,34},std::pair{76,24},std::pair{38,28},std::pair{60,20},std::pair{34,24}}) {
      RenderGrid(game.grid,game.state,size.second,size.first,2).draw();auto text=ui_text(frame);
      ui_check(text.find(language==iceberg::Language::English?"Closed.":"Zamknięte.")!=std::string::npos,"lock explanation appears in wide and compact cell maps");
      ui_check(text.find(language==iceberg::Language::English?"three routes":"trzy trasy")!=std::string::npos,"Harbor unlock condition names the three required routes");
    }
    game.tick(tab,1./120);
    ui_check(game.state.adventure.map&&game.state.adventure.map_route,"B switches to room route without closing map");
    game.tick(tab,1./120);
    ui_check(game.state.adventure.map_room&&!game.state.adventure.map_route,"B opens the campaign room map from the route");
    for(auto size:{std::pair{34,24},std::pair{60,20},std::pair{110,34}}) {
      RenderGrid(game.grid,game.state,size.second,size.first,2).draw();
      bool board=false,exit=false;
      for(const auto &row:frame)for(const auto &cell:row) {
        board|=cell.glyph=="▤"&&cell.rgb==0xd7e8cd;
        exit|=cell.glyph=="▣"&&cell.rgb==0xd7e8cd;
      }
      ui_check(board&&!exit,"the settlement minimap shows its real route board");
    }
    game.tick(map,1./120);game.tick(map,1./120);
    ui_check(game.state.adventure.map&&game.state.adventure.map_room,"reopening the campaign map restores its room view");
    game.tick(tab,1./120);
    ui_check(!game.state.adventure.map_room&&!game.state.adventure.map_route,"B returns to the campaign district map");
  }
  AdventureSession guidance(options,"/tmp/tv-ui-district-guidance-unused");
  guidance.begin_campaign(true);
  guidance.profile.language=iceberg::Language::English;
  guidance.profile.story=uint64_t(1)<<8;
  guidance.profile.unlocked=1;
  guidance.stage=2;guidance.load_room();
  Controls map_and_travel=map;map_and_travel.navigate=-1;map_and_travel.confirm=true;
  guidance.tick(map_and_travel,0);
  ui_check(guidance.state.adventure.map&&guidance.state.adventure.selected==0&&guidance.profile.visit_district<0,
    "opening the district map cannot navigate or travel in the opening frame");
  RenderGrid(guidance.grid,guidance.state,24,34,2).draw();
  auto route_hint=normalize_ui(ui_body(frame));
  ui_check(route_hint.find("Use this room's exit")!=std::string::npos&&
    ui_text(frame).find("ENTER travel")==std::string::npos,
    "the current campaign room points to its exit and offers no travel action");
  guidance.tick(enter,0);guidance.tick(map,0);
  ui_check(!guidance.state.adventure.map&&guidance.state.adventure.notice_left==0,
    "confirming the current district leaves no travel notice over gameplay");
  guidance.tick(map,0);
  Controls pick_district;pick_district.navigate=-1;guidance.tick(pick_district,0);
  RenderGrid(guidance.grid,guidance.state,24,34,2).draw();
  ui_check(ui_text(frame).find("ENTER travel")!=std::string::npos,
    "the map offers travel when an unlocked destination is selected");
  guidance.tick(enter,0);
  ui_check(guidance.profile.visit_district==1&&!guidance.state.adventure.map,
    "confirmation still travels to the selected settlement");
  guidance.tick(map,0);
  RenderGrid(guidance.grid,guidance.state,24,34,2).draw();
  ui_check(normalize_ui(ui_body(frame)).find("Use the board to return to your route")!=std::string::npos,
    "the visited settlement names its board's return action");
  AdventureSession focus(options,"/tmp/tv-ui-overlay-focus-unused");
  auto lose_and_regain_focus=[&](){focus.tick({},.1,false);focus.tick({},.1);};
  lose_and_regain_focus();
  ui_check(focus.state.menu&&!focus.state.paused,"losing focus in the menu does not create a gameplay pause");
  focus.begin_campaign(true);
  Controls pause_overlay;pause_overlay.pause=true;
  focus.tick(map,0);focus.tick(tab,0);focus.tick(tab,0);
  const double reading_time=focus.state.elapsed;
  lose_and_regain_focus();
  ui_check(focus.state.adventure.map&&focus.state.adventure.map_room&&!focus.state.paused&&focus.state.elapsed==reading_time,
    "focus loss preserves the room map without adding a second pause");
  focus.tick(map,0);focus.tick(book,0);focus.tick(tab,0);
  const int book_tab=focus.state.adventure.journal_tab;
  lose_and_regain_focus();
  ui_check(focus.state.journal&&focus.state.adventure.journal_tab==book_tab&&!focus.state.paused,
    "focus loss preserves the quest book tab without adding a second pause");
  focus.tick(pause_overlay,0);
  ui_check(!focus.state.journal&&!focus.state.paused,"closing the quest book returns to play after refocus");
  focus.tick(pause_overlay,0);focus.tick(map,0);
  lose_and_regain_focus();focus.tick(map,0);
  ui_check(!focus.state.adventure.map&&focus.state.paused,"closing a map after refocus preserves the manual pause underneath");
  focus.tick(enter,0);
  lose_and_regain_focus();
  ui_check(focus.state.paused,"losing focus during active play still pauses");
  focus.tick(enter,0);focus.profile.pause_on_focus_loss=false;
  lose_and_regain_focus();
  ui_check(!focus.state.paused,"disabling focus pause still keeps active play unpaused");
  AdventureSession overlay_exit(options,"/tmp/tv-ui-overlay-exit-unused");
  overlay_exit.begin_campaign(true);
  overlay_exit.stage=2;overlay_exit.load_room();
  overlay_exit.tick(map,0);
  const auto elapsed_before_map_close=overlay_exit.state.elapsed;
  Controls map_and_jump=map;map_and_jump.jump_pressed=map_and_jump.jump_held=true;
  overlay_exit.tick(map_and_jump,1./60);
  ui_check(!overlay_exit.state.adventure.map&&overlay_exit.state.elapsed==elapsed_before_map_close,
    "closing the map consumes the UI frame before a simultaneous jump can reach gameplay");
  overlay_exit.state.won=true;
  Controls pause_and_confirm=pause_overlay;pause_and_confirm.confirm=true;
  overlay_exit.tick(pause_and_confirm,0);
  ui_check(overlay_exit.state.paused&&overlay_exit.state.won&&overlay_exit.room==0,
    "opening pause at the door wins over confirmation in the same frame");
  overlay_exit.tick(enter,1./60);
  ui_check(!overlay_exit.state.paused&&overlay_exit.state.won&&overlay_exit.room==0,
    "confirming the pause resumes at the doorway without using it");
  overlay_exit.tick(enter,1./60);
  ui_check(overlay_exit.room==1,"a separate confirmation uses the doorway after resuming");
  AdventureSession camera_settings(options,"/tmp/tv-ui-camera-settings-unused");
  Controls settings;settings.select_level=5;settings.confirm=true;
  camera_settings.tick(settings,0);
  ui_check(camera_settings.state.adventure.settings&&!camera_settings.profile.smooth_camera,
    "smooth camera is opt-in from the settings menu");
  Controls camera_choice;camera_choice.select_level=7;
  camera_settings.tick(camera_choice,0);
  camera_settings.profile.language=iceberg::Language::English;camera_settings.sync_view();
  RenderGrid(camera_settings.grid,camera_settings.state,24,34,2).draw();
  ui_check(camera_settings.profile.smooth_camera&&ui_text(frame).find("Smooth camera (exp.) on")!=std::string::npos,
    "setting 7 enables smooth camera and shows its status on compact screens");
  camera_settings.begin_campaign(true);
  ui_check(camera_settings.profile.smooth_camera,"starting a new journey retains the camera preference");
  camera_settings.start_tour(0);
  ui_check(camera_settings.profile.smooth_camera,"debug tour uses the selected camera preference");
  camera_settings.end_tour();
  ui_check(camera_settings.profile.smooth_camera,"leaving the debug tour restores the profile camera preference");
  AdventureSession detour(options,"/tmp/tv-ui-room-detour-unused");detour.begin_campaign(true);
  detour.profile.language=iceberg::Language::English;
  detour.profile.hidden_room=0;detour.load_room();
  detour.tick(map,1./120);detour.tick(tab,1./120);
  ui_check(!detour.state.adventure.route.available,"a detour stays absent from the campaign route");
  detour.tick(tab,1./120);
  detour.state.player.x+=8;
  RenderGrid(detour.grid,detour.state,24,34,2).draw();
  int doors=0;
  for(const auto &row:frame)for(const auto &cell:row)
    doors+=cell.glyph=="▣"&&cell.rgb==0xd7e8cd;
  ui_check(doors==1&&ui_text(frame).find("Service corridor")!=std::string::npos,
    "the detour minimap shows only its real return doorway");
  AdventureSession features(options,"/tmp/tv-ui-room-features-unused");features.begin_campaign(true);
  features.profile.unlocked=3;features.run_district=3;
  auto &archive=features.profile.expeditions[0];
  archive.active=true;archive.district=3;archive.seed=archive.origin_seed=75;
  archive.itinerary={38,36,37,39,40,41,42,43};
  features.begin_run(iceberg::Mode::Endless);features.tick(map,1./120);features.tick(tab,1./120);
  auto count_map=[&](const char *glyph,uint32_t rgb) {
    int count=0;
    for(size_t y=4;y+7<frame.size();++y)for(const auto &cell:frame[y])
      count+=cell.glyph==glyph&&cell.rgb==rgb;
    return count;
  };
  AdventureSession valves(options,"/tmp/tv-ui-room-valves-unused");
  valves.begin_campaign(true);
  valves.grid=iceworld::build_heatworks_sequence(true);
  valves.state=new_state(valves.grid);
  valves.profile.language=iceberg::Language::English;
  valves.sync_view();
  // An optional control must never be presented as a step toward the exit.
  valves.grid.interactions.push_back({{10,52,3,4},InteractionKind::Valve,2,{"Zawór serwisowy","Service valve"}});
  auto count_valves=[&](){
    valves.state.adventure.map=valves.state.adventure.map_room=true;
    RenderGrid(valves.grid,valves.state,24,34,2).draw();
    return count_map("⊗",0xf0a277);
  };
  ui_check(count_valves()==2&&count_map("▣",0x7c8492)==1&&ui_text(frame).find("⊗ Required valve")!=std::string::npos,
    "the locked room map identifies both required valves and excludes the optional control");
  for(int i=0;i<2;++i) {
    valves.state.adventure.map=valves.state.adventure.map_room=false;
    const auto &area=valves.grid.interactions[size_t(i)].area;
    valves.state.player.x=area.x;
    valves.state.player.y=area.y;
    valves.interact();
    ui_check(count_valves()==1-i,"using a required valve removes only its own map marker");
  }
  ui_check(count_map("▣",0xd7e8cd)==1&&ui_text(frame).find("⊗ Required valve")==std::string::npos,
    "the fully opened room displays its unlocked exit with no valve legend");
  const auto at_spawn=features.state.player;
  const auto &goal=features.grid.goal;
  for(auto [cols,rows]:{std::pair{34,24},std::pair{60,20},std::pair{110,34}}) {
    features.state.player=at_spawn;
    RenderGrid(features.grid,features.state,rows,cols,2).draw();
    auto count_marker=[&](const char *glyph,uint32_t color) {
      int count=0;
      for(const auto &row:frame)for(const auto &cell:row)
        count+=cell.glyph==glyph&&cell.rgb==color;
      return count;
    };
    ui_check(count_marker("@",0xffd68a)==1&&count_marker("◆",0x9dd4dd)==1,
      "courier and checkpoint remain visible when they share a projected map cell");
    features.state.player.x=goal.x+(goal.w-kPlayerWidth)*.5;
    features.state.player.y=goal.y+(goal.h-kPlayerHeight)*.5;
    RenderGrid(features.grid,features.state,rows,cols,2).draw();
    ui_check(count_marker("@",0xffd68a)==1&&count_marker("▣",0xd7e8cd)==1,
      "courier and exit remain visible when they share a projected map cell");
  }
  features.state.player=at_spawn;
  for(auto language:{iceberg::Language::Polish,iceberg::Language::English}) {
    features.profile.language=language;features.sync_view();
    const bool en=language==iceberg::Language::English;
    for(auto [cols,rows]: {std::pair{34,24},std::pair{60,20},std::pair{110,34}}) {
      RenderGrid(features.grid,features.state,rows,cols,2).draw();
      auto text=ui_text(frame);
      ui_check(count_map("↗",0x9dd39c)==2&&text.find(en?"↗ Boost":"↗ Wybicie")!=std::string::npos,
        "the compact room map indicates the directional postal launchers");
      const int tokens=count_map("●",0xedc278);
      ui_check(tokens>1&&text.find(en?"● Token":"● Żeton")!=std::string::npos,
        "the room map identifies token shelves in both languages");
      features.state.collected.back()=true;
      RenderGrid(features.grid,features.state,rows,cols,2).draw();
      ui_check(count_map("●",0xedc278)==tokens-1,"a collected token is removed from the map");
      features.state.collected.back()=false;
    }
  }
  auto tram_room=verified_room(24,73,2);
  auto tram_view=new_state(tram_room);
  tram_view.adventure.map=tram_view.adventure.map_room=true;
  RenderGrid(tram_room,tram_view,24,34,2).draw();
  ui_check(count_map("▰",0xefcd74)==0&&count_map("○",0xc2accf)==0,
    "ordinary lifts do not appear as scheduled trams");
  add_expedition_trams(tram_room);
  tram_view=new_state(tram_room);
  tram_view.adventure.map=tram_view.adventure.map_room=true;
  for(auto [cols,rows]:{std::pair{34,24},std::pair{60,20},std::pair{110,34}}) {
    RenderGrid(tram_room,tram_view,rows,cols,2).draw();
    ui_check(count_map("▰",0xefcd74)==3&&count_map("○",0xc2accf)==6&&
      ui_text(frame).find("○ Przystanek")!=std::string::npos,
      "the compact Estate room map shows three tram cars and both stops on each track");
  }
  auto tram_markers=[&](const char *glyph,uint32_t color){
    std::vector<std::pair<size_t,size_t>> cells;
    for(size_t y=6;y+7<frame.size();++y)for(size_t x=0;x<frame[y].size();++x)
      if(frame[y][x].glyph==glyph&&frame[y][x].rgb==color)cells.emplace_back(y,x);
    return cells;
  };
  const auto first_cars=tram_markers("▰",0xefcd74),stops=tram_markers("○",0xc2accf);
  tram_view.elapsed=3;
  tram_view.adventure.language=iceberg::Language::English;
  RenderGrid(tram_room,tram_view,34,110,2).draw();
  ui_check(tram_markers("▰",0xefcd74)!=first_cars&&tram_markers("○",0xc2accf)==stops&&
    ui_text(frame).find("○ Stop")!=std::string::npos,
    "tram markers follow the actual cars across their routes");
  auto belt_room=verified_room(44,74,3);
  auto belt_view=new_state(belt_room);
  belt_view.adventure.map=belt_view.adventure.map_room=true;
  RenderGrid(belt_room,belt_view,24,34,2).draw();
  ui_check(count_map("▶",0xa0d6df)==0&&count_map("◀",0xa0d6df)==0,
    "the legacy Archive room map has no powered belt markers");
  add_expedition_copy_belts(belt_room);
  belt_view=new_state(belt_room);
  belt_view.adventure.map=belt_view.adventure.map_room=true;
  for(auto language:{iceberg::Language::Polish,iceberg::Language::English}) {
    belt_view.adventure.language=language;
    const auto legend=language==iceberg::Language::English?"◀▶ Belts":"◀▶ Taśmy";
    for(auto [cols,rows]:{std::pair{34,24},std::pair{60,20},std::pair{110,34}}) {
      RenderGrid(belt_room,belt_view,rows,cols,2).draw();
      ui_check(count_map("▶",0xa0d6df)==2&&count_map("◀",0xa0d6df)==1&&
        ui_text(frame).find(legend)!=std::string::npos,
        "the Archive map marks the two right-moving belts and the left-moving belt");
    }
  }
  auto rack_room=verified_room(47,73,3);
  auto rack_view=new_state(rack_room);
  rack_view.adventure.map=rack_view.adventure.map_room=true;
  RenderGrid(rack_room,rack_view,24,34,2).draw();
  ui_check(count_map("▲",0xe5b276)==0&&count_map("┆",0x718c9d)==0,
    "the original Last Stack has no rack indicators");
  add_expedition_archive_racks(rack_room);
  rack_view=new_state(rack_room);
  rack_view.adventure.map=rack_view.adventure.map_room=true;
  for(auto language:{iceberg::Language::Polish,iceberg::Language::English}) {
    rack_view.adventure.language=language;
    const auto legend=language==iceberg::Language::English?"▲▼ Racks":"▲▼ Regały";
    for(auto [cols,rows]:{std::pair{34,24},std::pair{60,20},std::pair{110,34}}) {
      for(double phase:{0.,1.6,3.2,4.8}) {
        rack_view.elapsed=phase;
        const auto deck=rack_room.platforms.back().at(phase);
        rack_view.player.x=deck.x+(deck.w-kPlayerWidth)*.5;
        rack_view.player.y=deck.y-kPlayerHeight;
        RenderGrid(rack_room,rack_view,rows,cols,2).draw();
        const bool turning=phase==1.6||phase==4.8;
        ui_check(count_map("▲",0xe5b276)==!turning&&count_map("▼",0xe5b276)==!turning&&
          count_map("■",0xe5b276)==(turning?2:0)&&count_map("@",0xffd68a)==1&&
          count_map("┆",0x718c9d)>0&&ui_text(frame).find(legend)!=std::string::npos,
          "rack map directions, travel and localized legend remain visible with a rider");
      }
    }
  }
  AdventureSession winter(options,"/tmp/tv-ui-ice-map-unused");
  winter.begin_campaign(true);winter.profile.unlocked=4;winter.run_district=4;
  auto &garden=winter.profile.expeditions[0];
  garden.active=true;garden.district=4;garden.seed=garden.origin_seed=73;
  garden.itinerary={48,58,49,50,51,52,53,54};
  winter.begin_run(iceberg::Mode::Endless);
  for(int index=0;index<2;++index) {
    ui_check(winter.grid.template_id==garden.itinerary[size_t(index)]&&!winter.grid.ice_strips.empty(),
      "the map test starts in a room with static ice and its original terrain");
    winter.tick(map,0);
    if(!winter.state.adventure.map_room)winter.tick(tab,0);
    for(auto language:{iceberg::Language::Polish,iceberg::Language::English}) {
      winter.profile.language=language;winter.sync_view();
      for(auto [cols,rows]:{std::pair{34,24},std::pair{60,20},std::pair{110,34}}) {
        RenderGrid(winter.grid,winter.state,rows,cols,2).draw();
        const auto label=language==iceberg::Language::English?"━ Ice":"━ Lód";
        ui_check(count_map("━",0xa6e9ea)>0&&ui_text(frame).find(label)!=std::string::npos,
          "the room map draws fixed ice and its localized legend at supported sizes");
      }
    }
    winter.tick(map,0);
    if(index==0)winter.advance_room();
  }
  garden.layout_version=2;winter.load_room();winter.tick(map,0);
  if(!winter.state.adventure.map_room)winter.tick(tab,0);
  RenderGrid(winter.grid,winter.state,24,34,2).draw();
  ui_check(count_map("━",0xa6e9ea)==0&&ui_text(frame).find("━ Ice")==std::string::npos,
    "the legacy dry layout does not show ice glyphs or a legend");
  features.begin_campaign(true);features.stage=1;features.load_room();
  features.tick(map,1./120);features.tick(tab,1./120);features.tick(tab,1./120);
  RenderGrid(features.grid,features.state,24,34,2).draw();
  ui_check(count_map("≈",0x66bfd7)>0&&ui_text(frame).find("≈ Water")!=std::string::npos,
    "the compact Harbor lesson map shows the real ferry waterline");
  AdventureSession harbor_fork(options,"/tmp/tv-ui-room-fork-unused");harbor_fork.begin_campaign(true);
  harbor_fork.stage=2;harbor_fork.room=1;harbor_fork.load_room();
  const auto passage=std::find_if(harbor_fork.grid.interactions.begin(),harbor_fork.grid.interactions.end(),[](const auto &node){
    return node.kind==InteractionKind::RouteChoice;
  });
  ui_check(passage!=harbor_fork.grid.interactions.end(),"the current Harbor room has a real alternate passage");
  harbor_fork.tick(map,1./120);harbor_fork.tick(tab,1./120);harbor_fork.tick(tab,1./120);
  for(auto language:{iceberg::Language::Polish,iceberg::Language::English}) {
    harbor_fork.profile.language=language;harbor_fork.sync_view();
    const bool en=language==iceberg::Language::English;
    for(const auto &door:{harbor_fork.grid.goal,passage->area}) {
      harbor_fork.state.player.x=door.x+(door.w-kPlayerWidth)*.5;
      harbor_fork.state.player.y=door.y+(door.h-kPlayerHeight)*.5;
      for(auto [cols,rows]: {std::pair{34,24},std::pair{60,20},std::pair{110,34}}) {
        RenderGrid(harbor_fork.grid,harbor_fork.state,rows,cols,2).draw();
        const auto text=ui_text(frame);
        int main_x=-1,branch_x=-1,player_marks=0;
        for(int y=4;y<rows-8;++y)for(int x=0;x<cols;++x) {
          const auto &cell=frame[size_t(y)][size_t(x)];
          if(cell.glyph=="▧"&&cell.rgb==0xe8a758)branch_x=x;
          if(cell.glyph=="▣"&&cell.rgb==0xd7e8cd)main_x=x;
          if(cell.glyph=="@"&&cell.rgb==0xffd68a)++player_marks;
        }
        ui_check(branch_x>=0&&main_x>branch_x&&player_marks==1&&
          text.find(en?"▣ Main · ▧ Passage B":"▣ Główne · ▧ Przejście B")!=std::string::npos,
          "the map keeps both Harbor doors and the courier visible even at the fork");
      }
    }
  }
  AdventureSession preview(options,"/tmp/tv-ui-route-preview-unused");preview.begin_campaign(true);
  preview.stage=2;preview.load_room();
  Controls next_leg;next_leg.navigate=1;
  preview.tick(map,1./120);preview.tick(tab,1./120);preview.tick(next_leg,1./120);
  preview.tick(map,1./120);preview.tick(map,1./120);
  ui_check(preview.state.adventure.map_route&&preview.state.adventure.route_page==1,
    "reopening a route map keeps the previewed leg");
  preview.advance_room();preview.tick(map,1./120);
  ui_check(preview.state.adventure.map_route&&preview.state.adventure.route_page==-1,
    "advancing a room returns the route view to the active leg");
  AdventureSession paused(options,"/tmp/tv-ui-paused-overlays-unused");paused.begin_campaign(true);
  paused.profile.language=iceberg::Language::English;
  paused.profile.quests[2]=3;paused.sync_view();
  Controls pause;pause.pause=true;
  paused.tick(pause,1./120);paused.tick(map,1./120);
  const auto overlay_visible=[&](const char *title) {
    for(auto size:{std::pair{110,34},std::pair{38,28},std::pair{60,20}}) {
      RenderGrid(paused.grid,paused.state,size.second,size.first,2).draw();
      const auto text=ui_text(frame);
      if(text.find(title)==std::string::npos||text.find("A MOMENT TO BREATHE")!=std::string::npos)return false;
    }
    return true;
  };
  ui_check(paused.state.paused&&overlay_visible("JOURNEY MAP"),"the map is readable over pause on wide and compact screens");
  paused.state.adventure.selected=1;paused.tick(enter,1./120);
  ui_check(paused.state.paused&&paused.state.adventure.map,"confirm on a locked district preserves pause");
  paused.tick(pause,1./120);
  ui_check(paused.state.paused&&!paused.state.adventure.map,"close the map before resuming the world");
  paused.tick(book,1./120);
  ui_check(paused.state.paused&&overlay_visible("QUEST BOOK"),"the quest book is readable while the world is paused");
  paused.tick(tab,1./120);paused.tick(enter,1./120);
  ui_check(paused.state.paused&&paused.state.adventure.dialogue.active(),"read a completed task record without resuming the world");
  paused.tick(enter,1./120);
  ui_check(paused.state.paused&&!paused.state.adventure.dialogue.revealing,"confirm reveals the record while the world stays paused");
  paused.tick(pause,1./120);
  ui_check(paused.state.paused&&paused.state.journal&&!paused.state.adventure.dialogue.active(),"back closes the record to the paused quest book");
  paused.tick(book,1./120);paused.tick(enter,1./120);
  ui_check(!paused.state.paused&&!paused.state.journal,"confirm resumes play from the bare pause screen");
  paused.state.adventure.save_error=true;
  paused.notice("Underfrost: bring spare seeds to the lower stop and deliver the register copy at the counter.",6);
  auto compact_text=[&](){
    RenderGrid(paused.grid,paused.state,24,34,2).draw();
    std::string contents;
    for(const auto &row:frame) {
      for(size_t x=2;x+2<row.size();++x)contents+=row[x].glyph;
      contents+=' ';
    }
    return normalize_ui(contents);
  };
  const std::string warning="Save error — progress remains in this session.";
  ui_check(compact_text().find(warning)!=std::string::npos,"compact play shows the save failure over other notices");
  paused.state.won=true;
  const auto exit_with_error=compact_text();
  ui_check(exit_with_error.find(warning)!=std::string::npos&&exit_with_error.find("E / ENTER — pass through")!=std::string::npos,
           "compact doorway shows the save warning and keeps the continue prompt");
  paused.state.won=false;
  paused.tick(pause,1./120);
  ui_check(compact_text().find(warning)!=std::string::npos,
           "compact pause carries the save failure");
  paused.tick(pause,1./120);
  paused.state.adventure.save_error=false;
  paused.notice("Leave the seeds by the stop and deliver the register copy to the counter.",6);
  ui_check(compact_text().find("Leave the seeds by the stop and deliver the register copy to the counter.")!=std::string::npos,
           "compact notices can use all three footer rows");
  game.profile.unlocked=1;game.profile.story|=uint64_t(1)<<8;game.sync_view();game.state.adventure.selected=1;
  game.tick(tab,1./120);game.tick(tab,1./120);game.tick(enter,1./120);
  ui_check(game.state.adventure.map_room&&game.grid.district==0,"Confirm cannot travel from the campaign room map");
  Controls switch_and_confirm=tab;switch_and_confirm.confirm=true;game.tick(switch_and_confirm,1./120);
  ui_check(game.state.adventure.map&&game.grid.district==0,"switching to districts and confirming together cannot start travel");
  game.tick(enter,1./120);ui_check(!game.state.adventure.map&&game.grid.district==1&&game.profile.visit_district==1,"unlocked district travel enters the named settlement");
  ui_check(game.state.adventure.notice.find("ARRIVED")!=std::string::npos&&game.state.adventure.door_left>0,"allowed travel has arrival feedback and door animation");
  game.tick(map,1./120);game.state.adventure.selected=0;game.tick(enter,1./120);
  ui_check(game.grid.district==0&&game.profile.visit_district<0,"map returns to preserved route");
  game.profile.quests={1,2,3,0,0,0,1,0};game.profile.quest_counts[0]=1;game.profile.discoveries=0;game.sync_view();
  game.tick(book,1./120);auto entries=iceberg::journal_entries(game.state.adventure);
  ui_check(entries==std::vector<int>({-1,0,1,6}),"active book lists journey and accepted tasks only");
  game.state.adventure.selected=2;game.tick(use,1./120);ui_check(game.state.adventure.quest_focus==1&&game.state.journal,"E selects a concrete task as current focus");
  game.tick(tab,1./120);ui_check(iceberg::journal_entries(game.state.adventure)==std::vector<int>({2}),"completed tab separates finished tasks");
  game.tick(use,1./120);ui_check(game.state.adventure.dialogue.active(),"completed task opens its actual record");
  while(game.state.adventure.dialogue.active())game.dialogue_next();
  ui_check(game.state.journal,"record returns to quest book");
  game.tick(tab,1./120);ui_check(iceberg::journal_entries(game.state.adventure).empty(),"notes do not advertise unknown secrets");
  game.profile.discoveries=2;game.sync_view();ui_check(iceberg::journal_entries(game.state.adventure)==std::vector<int>({101}),"only discovered note is listed");
  for(auto language:{iceberg::Language::Polish,iceberg::Language::English})for(int q=0;q<8;++q) {
    game.profile.language=language;game.profile.quests={};game.profile.quests[size_t(q)]=1;game.sync_view();
    game.state.adventure.journal_tab=0;game.state.adventure.selected=1;game.state.adventure.quest_focus=q;
    for(auto dimensions:{std::pair{34,24},std::pair{60,20},std::pair{110,34}}) {
      RenderGrid(game.grid,game.state,dimensions.second,dimensions.first,2).draw();
      if(normalize_ui(ui_body(frame)).find(normalize_ui(iceberg::quest_next_step(game.state.adventure,q)))==std::string::npos)
        throw std::runtime_error("book clips quest next step q="+std::to_string(q)+" columns="+std::to_string(dimensions.first)+" language="+std::to_string(int(language)));
    }
  }
  ui_check(true,"all accepted-task next steps remain readable at compact and wide sizes in both languages");
  game.state.journal=false;game.profile.quests[4]=0;game.profile.language=iceberg::Language::English;game.sync_view();
  game.tick({},0,true,34,24);game.talk({{0,{"Pierwszy bardzo długi dokument, którego nie można zgubić przy małym oknie. Ten tekst opisuje drogę przez pomosty, schody i kanał do lampy, a później skręt ku wejściu do ciepłowni. Najpierw trzeba przywrócić ciepło mieszkańcom, następnie dostarczyć przesyłkę i dopiero potem wrócić do Heleny. To ostatnie zdanie musi zostać pokazane przed zatwierdzeniem zadania.","A long essential objective that must never disappear in a small window. Follow the boardwalks, stairs and channel to the lamp, then turn toward the heating works entrance. First restore heat to the homes, then deliver the parcel and only then return to Helena. This final instruction must be displayed before the task can be accepted. The next room is reached through the lit door beyond the pump."}}},104);
  game.dialogue_next();game.dialogue_next();ui_check(game.state.adventure.dialogue.active()&&game.profile.quests[4]==0,"fast text does not acknowledge undisplayed essential pages");
  ui_check(game.state.adventure.dialogue.segment>0,"long essential objective reaches a later segment");
  game.tick({},0,true,60,20);
  ui_check(game.state.adventure.dialogue.segment==0&&game.state.adventure.dialogue.instant&&!game.state.adventure.dialogue.revealing,"resize returns to first segment and preserves the chosen pace");
  ui_check(game.profile.quests[4]==0,"resize never acknowledges unseen objective text");
  while(game.state.adventure.dialogue.active())game.dialogue_next();
  ui_check(game.profile.quests[4]==1,"task commits after all pages acknowledged");
  for(double aspect:{1.1,2.,3.4})for(auto dimensions:{std::pair{34,24},std::pair{60,20},std::pair{110,34}}) {
    game.tick({},0,true,dimensions.first,dimensions.second,aspect);game.talk(iceberg::task_record(0));game.dialogue_next();
    while(game.state.adventure.dialogue.active()) {
      const auto &dialogue=game.state.adventure.dialogue;
      int courier=4+int(std::floor((game.state.player.y-game.state.camera_y)/(aspect*.4)));
      int feet=5+int(std::floor((game.state.player.y+kPlayerHeight-.001-game.state.camera_y)/(aspect*.4)));
      auto layout=iceberg::dialogue_layout(dialogue.lines[dialogue.page].text.get(game.profile.language),dimensions.first,dimensions.second,courier,dialogue.segment);
      ui_check(feet-3>=4&&feet-1<dimensions.second-3&&!(feet-1>=layout.top&&feet-3<layout.top+layout.height),"long dialogue preserves courier at narrow and tall terminal cell aspects");
      game.dialogue_next();
    }
  }
  game.grid=iceworld::build_harbor_mission_v4(2);game.state=new_state(game.grid);game.sync_view();
  auto approach=[&](InteractionKind kind,int id){auto found=std::find_if(game.grid.interactions.begin(),game.grid.interactions.end(),[&](const auto &node){return node.kind==kind&&node.id==id;});if(found==game.grid.interactions.end())throw std::runtime_error("missing gauge fixture");game.state.player.x=found->area.x+1;game.state.player.y=found->area.bottom()-kPlayerHeight;};
  approach(InteractionKind::Note,40);game.tick(tab,1./120);
  ui_check(game.state.adventure.inspecting&&(game.grid.switch_flags&16)&&!(game.grid.switch_flags&3),"B reading actual gauges records evidence without operating machinery");
  game.tick(tab,1./120);approach(InteractionKind::Valve,1);game.tick(use,1./120);
  approach(InteractionKind::Valve,0);game.tick(use,1./120);
  ui_check((game.grid.switch_flags&3)==3,"observed gauge evidence allows bypass then pump without redundant E reading");
  game.profile.quests[6]=3;
  for(auto mode:{iceberg::Mode::Endless,iceberg::Mode::Trial}) {
    game.begin_run(mode,true);
    for(int i=0;i<(mode==iceberg::Mode::Endless?9:2);++i)game.advance_room();
    auto x=game.state.player.x;
    game.tick(map,1./120);
    ui_check(game.state.adventure.map&&game.state.adventure.map_route&&!game.state.adventure.map_room,
      "J opens the active expedition route");
    Controls up;up.navigate=-1;
    game.tick(tab,1./120);game.tick(up,1./120);game.tick(enter,1./120);
    const auto &v=game.state.adventure;
    ui_check(v.map&&v.map_route&&v.map_room&&v.route_page==-1&&game.state.player.x==x,
      "B opens the room map while district navigation and confirmation leave the expedition paused in place");
    const int room=mode==iceberg::Mode::Endless?1:2;
    ui_check(v.route.available&&v.route.current==room&&v.route.visited==(1u<<(room+1))-1,"expedition route marks only the rooms reached in this leg");
    for(auto language:{iceberg::Language::Polish,iceberg::Language::English}) {
      game.profile.language=language;game.sync_view();
      for(auto size:{std::pair{34,24},std::pair{60,20},std::pair{110,34}}) {
        RenderGrid(game.grid,game.state,size.second,size.first,2).draw();
        const bool en=language==iceberg::Language::English;
        const auto room_map=ui_text(frame);
        bool courier=false,exit=false;
        for(const auto &row:frame)for(const auto &cell:row) {
          courier|=cell.glyph=="@"&&cell.rgb==0xffd68a;
          exit|=cell.glyph=="▣"&&cell.rgb==0xd7e8cd;
        }
        ui_check(room_map.find(en?"ROOM MAP":"PLAN POKOJU")!=std::string::npos&&courier&&exit&&
          room_map.find("▪")!=std::string::npos,"the room map marks terrain, player and exit at all supported sizes");
        game.tick(tab,1./120);
        auto &route=game.state.adventure.route;
        // Representative long, localized names that exceed a compact map line.
        route.place=iceworld::pattern(40).name;
        route.rooms[size_t(route.current+1)]=iceworld::pattern(45).name;
        RenderGrid(game.grid,game.state,size.second,size.first,2).draw();
        const auto text=ui_text(frame);
        const char *title=mode==iceberg::Mode::Endless?(en?"ENDLESS EXPEDITION":"WYPRAWA BEZ KOŃCA"):(en?"MASTERY TRIAL":"PRÓBA MISTRZOWSKA");
        const char *progress=mode==iceberg::Mode::Endless?(en?"Leg 2 · Depth 9":"Odcinek 2 · Głębokość 9"):(en?"Room 3 / 8":"Pokój 3 / 8");
        std::string details;
        for(const auto &row:frame) {
          for(const auto &cell:row)if(cell.rgb==0xffd68a||cell.rgb==0xc7d9df)details+=cell.glyph;
          details+=' ';
        }
        const auto normalized=normalize_ui(details);
        const auto current=(en?"Now: ":"Teraz: ")+route.place.get(language);
        const auto next=(en?"Planned: ":"Plan: ")+route.rooms[size_t(route.current+1)].get(language);
        ui_check(text.find(title)!=std::string::npos&&text.find(progress)!=std::string::npos&&
          normalized.find(current+" "+next)!=std::string::npos,
          "expedition map shows progress and both complete room names at all supported sizes");
        game.tick(tab,1./120);
      }
    }
    game.tick(map,1./120);ui_check(!v.map,"J closes the expedition map");
    game.tick(map,1./120);
    ui_check(v.map&&v.map_room,"reopening an expedition map remembers the room view");
    game.tick(book,1./120);game.tick(map,1./120);
    ui_check(v.map&&v.map_room&&!game.state.journal,"reopening after the quest book keeps the room view");
    game.tick(map,1./120);game.advance_room();game.tick(map,1./120);
    ui_check(v.map&&v.map_room&&v.route.current==room+1,"the next expedition room keeps the map view and updates the room");
    game.tick(tab,0);
    Controls next_room;next_room.navigate=1;
    Controls previous_room;previous_room.navigate=-1;
    for(int i=0;i<8;++i)game.tick(next_room,0);
    ui_check(v.route_room_selection==7&&v.route.current==room+1,
      "browsing reaches the last planned room without moving the courier");
    RenderGrid(game.grid,game.state,24,34,2).draw();
    ui_check(ui_text(frame).find("Room 8:")!=std::string::npos&&ui_text(frame).find("╰╯")!=std::string::npos,
      "compact expedition timeline names and marks the browsed room");
    if(mode==iceberg::Mode::Trial) {
      for(auto language:{iceberg::Language::Polish,iceberg::Language::English}) {
        game.profile.language=language;game.sync_view();
        RenderGrid(game.grid,game.state,24,34,2).draw();
        const bool en=language==iceberg::Language::English;
        std::string detail;
        for(const auto &row:frame) {
          for(const auto &cell:row)detail+=cell.rgb==0xc7d9df?cell.glyph:" ";
          detail+=' ';
        }
        ui_check(normalize_ui(detail).find(en?"Room 8: Kuba's examination":"Pokój 8: Egzamin Kuby")!=std::string::npos,
          "the trial timeline names the final course in both languages");
      }
      game.profile.language=iceberg::Language::English;game.sync_view();
    }
    for(int i=0;i<8;++i)game.tick(previous_room,0);
    ui_check(v.route_room_selection==0&&v.route.current==room+1,
      "browsing stops at the first room without changing expedition progress");
    game.tick(map,0);game.tick(map,0);
    ui_check(v.route_room_selection==-1&&v.route.current==room+1,
      "reopening the expedition timeline returns the selection to the current room");

    if(mode==iceberg::Mode::Trial) {
      const auto final_name=v.route.rooms.back();
      game.tick(map,0);
      for(int i=game.room;i<iceberg::RoomsPerLeg-1;++i)game.advance_room();
      ui_check(game.grid.template_id==320&&
        game.grid.room_name.get(iceberg::Language::Polish)==final_name.get(iceberg::Language::Polish)&&
        game.grid.room_name.get(iceberg::Language::English)==final_name.get(iceberg::Language::English),
        "the planned final trial room title matches the course on arrival");
    }
  }
  game.begin_campaign(false);game.tick(map,1./120);
  ui_check(game.state.adventure.map&&!game.state.adventure.map_route,"returning to the campaign opens district travel");
  Controls menu;menu.menu_request=true;game.tick(menu,1./120);
  game.profile.expeditions[0].active=true;game.profile.expeditions[0].district=0;
  game.profile.endless_best=17;
  Controls choose;choose.select_level=3;game.tick(choose,1./120);
  for(auto language:{iceberg::Language::Polish,iceberg::Language::English}) {
    game.profile.language=language;game.sync_view();
    for(auto size:{std::pair{34,24},std::pair{60,20}}) {
      RenderGrid(game.grid,game.state,size.second,size.first,2).draw();
      const auto text=ui_text(frame);
      const bool en=language==iceberg::Language::English;
      ui_check(text.find(en?"Resume expedition":"Wznów wyprawę")!=std::string::npos&&
          text.find(en?"J change district":"J zmień dzielnicę")!=std::string::npos&&
          text.find(iceberg::Districts[0].name.get(language))!=std::string::npos&&
          text.find(en?"Farthest expedition: 17":"Najdalsza wyprawa: 17")!=std::string::npos,
        "the compact menu displays an expedition's district control and best depth");
    }
  }
  game.tick(map,1./120);
  ui_check(game.run_district==1&&!game.state.adventure.endless_resume,
    "district cycling leaves the saved expedition available in its own district");
  game.profile.quests[6]=3;game.begin_run(iceberg::Mode::Trial,true);
  game.options.no_save=false;game.saves=iceberg::SaveStore(std::filesystem::path{});
  game.room=iceberg::RoomsPerLeg-1;
  game.run_seconds=1234.5;game.run_deaths=12;game.run_treasures=8;
  game.advance_room();
  ui_check(game.state.menu&&game.state.adventure.selected==3&&game.state.adventure.save_error,
    "a finished trial reselects the trial and flags a failed save");
  for(auto language:{iceberg::Language::Polish,iceberg::Language::English}) {
    game.profile.language=language;game.sync_view();
    const bool en=language==iceberg::Language::English;
    for(auto size:{std::pair{34,24},std::pair{60,20},std::pair{110,34}}) {
      RenderGrid(game.grid,game.state,size.second,size.first,2).draw();
      std::string interior;
      for(const auto &row:frame) {
        for(const auto &cell:row)if(cell.glyph!="│")interior+=cell.glyph;
        interior+=' ';
      }
      const auto text=normalize_ui(interior);
      ui_check(text.find("1234.5s")!=std::string::npos&&
        text.find(en?"12 retries · 8 ◆":"12 powrotów · 8 ◆")!=std::string::npos&&
        text.find("▸ 4")!=std::string::npos,"the trial menu shows all records and keeps retry selected");
    }
  }
  ui_check(game.state.adventure.notice=="TRIAL COMPLETE — save error","completing a trial does not report a failed write as saved");
  game.tick(enter,1./120);
  ui_check(game.run_active&&game.room==0&&game.mode==iceberg::Mode::Trial&&!game.state.menu,
    "confirm starts a fresh trial after completion");
  cell_frame_sink={};return 0;
}catch(const std::exception &error){std::cerr<<"FAIL "<<error.what()<<'\n';return 1;}}
