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
  {
    auto at_door=game.state;
    const auto &door=game.grid.goal;
    const double marker_x=door.x+door.w*.5-.5, marker_y=door.y+1.6;
    at_door.player.x=marker_x-.6;
    at_door.player.y=marker_y-.7;
    at_door.player.facing=1;
    at_door.camera_x=marker_x-4.1;
    at_door.camera_y=marker_y-4.1;
    at_door.adventure.door_left=0;
    at_door.won=true;
    for(auto [columns,rows]:{std::pair{34,24},std::pair{110,34}}) {
      at_door.adventure.door_open=.6;
      RenderGrid(game.grid,at_door,rows,columns,2).draw();
      const Cell courier=frame[9][10];
      at_door.adventure.door_open=1.;
      RenderGrid(game.grid,at_door,rows,columns,2).draw();
      ui_check(courier.glyph=="▀"&&frame[9][10].glyph==courier.glyph&&frame[9][10].rgb==courier.rgb,
        "the open doorway marker stays behind the courier");
      at_door.player.x-=2.;
      RenderGrid(game.grid,at_door,rows,columns,2).draw();
      ui_check(frame[9][10].glyph=="▱","the open doorway marker remains visible when the courier moves away");
      at_door.player.x+=2.;
    }
  }
  {
    const auto quay=iceworld::build_harbor_settlement_v4();
    auto crossing=new_state(quay);
    crossing.adventure.door_left=0.;
    crossing.camera_x=54.;
    crossing.camera_y=32.01;
    for(auto language:{iceberg::Language::English,iceberg::Language::Polish}) {
      crossing.adventure.language=language;
      for(int columns:{34,110}) {
        crossing.player.x=66.6;
        crossing.player.y=39.1;
        RenderGrid(quay,crossing,24,columns,2.).draw();
        ui_check(frame.at(12).at(32).glyph=="╮"&&frame.at(12).at(32).rgb==0x5f777b,
          "the coffee facade stays intact next to the courier's drawn head");
        ui_check(frame.at(13).at(32).rgb==0xe4ad68,
          "the courier draws in front of the coffee facade");
        crossing.player.x=64.1;
        crossing.player.y=34.05;
        RenderGrid(quay,crossing,24,columns,2.).draw();
        ui_check(frame.at(6).at(25).rgb==0x596d73,
          "the supply main stays continuous above the courier");
      }
    }
  }
  {
    auto room=game.grid;
    auto state=game.state;
    room.sweets={{8,20},{12,20}};
    state.collected={true,false};
    state.deaths=7;
    state.adventure.mode=iceberg::Mode::Endless;
    state.adventure.depth=12;
    for(auto language:{iceberg::Language::Polish,iceberg::Language::English}) {
      state.adventure.language=language;
      const bool english=language==iceberg::Language::English;
      RenderGrid(room,state,32,56,2).draw();
      const auto text=ui_text(frame);
      ui_check(text.find(english?"Tokens 1/2   Retries 7   Depth 12":
                                "Żetony 1/2   Powroty 7   Głębokość 12")!=std::string::npos,
               "compact gameplay keeps token, retry and expedition depth readouts");
    }
  }
  {
    AdventureSession tour(options,{});
    tour.start_tour(6);
    const auto &v=tour.state.adventure;
    for(auto language:{iceberg::Language::English,iceberg::Language::Polish}) {
      tour.profile.language=language;
      tour.load_tour_scene(6);
      const auto opening=normalize_ui(v.notice);
      for(int columns:{34,110}) {
        auto footer=[&](){
          RenderGrid(tour.grid,tour.state,34,columns,2).draw();
          return normalize_ui(ui_text(CellFrame(frame.end()-3,frame.end())));
        };
        tour.state.player.x=28;
        tour.tick({},0);
        ui_check(footer().find(opening)!=std::string::npos,
          "the conveyor entry instruction appears away from an interaction");
        tour.state.player.x=36;
        tour.tick({},0);
        const auto nearby=footer();
        ui_check(v.entry_notice&&v.notice_left>0&&
          nearby.find(language==iceberg::Language::English?"Talk to Irena":"Porozmawiaj z Ireną")!=std::string::npos&&
          nearby.find(opening)==std::string::npos,
          "a nearby character prompt takes priority over the unexpired entry instruction");
        tour.state.player.x=28;
        tour.tick({},0);
        ui_check(footer().find(opening)!=std::string::npos,
          "walking away restores the still-current entry instruction");
      }
      tour.state.player.x=45;
      tour.state.player.y=47-kPlayerHeight;
      tour.tick({.interact=true},1./120);
      RenderGrid(tour.grid,tour.state,24,34,2).draw();
      ui_check(!v.entry_notice&&ui_text(frame).find(language==iceberg::Language::English?"Middle belt: →":"Środkowa taśma: →")!=std::string::npos,
        "using the relay still shows immediate direction feedback near the switch");
    }
  }
  {
    auto room=game.grid;
    auto state=game.state;
    room.sweets={{8,20},{12,20}};
    state.collected={true,false};
    state.deaths=12;
    for(auto language:{iceberg::Language::English,iceberg::Language::Polish}) {
      state.adventure.language=language;
      const bool en=language==iceberg::Language::English;
      const std::string stats=en?"Tokens 1/2   Retries 12":"Żetony 1/2   Powroty 12";
      state.adventure.room_name=en?"Hanging beds":"Wiszące grządki";
      RenderGrid(room,state,24,34,2).draw();
      const auto short_frame=ui_text(frame);
      const auto short_row=short_frame.find(stats);
      ui_check(short_row!=std::string::npos&&std::count(short_frame.begin(),short_frame.begin()+short_row,'\n')==2,
        "a short room title leaves the narrow token and retry counts on the third row");
      state.adventure.room_name=en?"The channel beneath the greenhouse":"Kanał pod szklarnią i górne tarasy";
      RenderGrid(room,state,24,34,2).draw();
      const auto long_frame=ui_text(frame);
      const auto stats_row=long_frame.find(stats);
      ui_check(stats_row!=std::string::npos&&std::count(long_frame.begin(),long_frame.begin()+stats_row,'\n')==3&&
        frame[3].front().glyph=="─"&&frame[3].back().glyph=="─"&&
        normalize_ui(long_frame).find(state.adventure.room_name)!=std::string::npos,
        "a wrapped room title keeps both lines and the counters fit in the divider");
    }
  }
  {
    auto settings=game.state;
    settings.menu=settings.adventure.settings=true;
    settings.adventure.smooth_camera=true;
    settings.adventure.pause_on_focus_loss=false;
    for(auto language:{iceberg::Language::English,iceberg::Language::Polish}) {
      settings.adventure.language=language;
      const bool english=language==iceberg::Language::English;
      const std::string lang=english?"L Language: ENGLISH":"L Język: POLSKI";
      const std::string back=english?"ENTER return":"ENTER wróć";
      for(auto [columns,rows]:{std::pair{60,20},std::pair{76,20},std::pair{38,28}}) {
        RenderGrid(game.grid,settings,rows,columns,2).draw();
        const auto text=ui_text(frame);
        const auto language_position=text.find(lang),return_position=text.find(back);
        ui_check(language_position!=std::string::npos&&return_position!=std::string::npos&&
                 text.find('\n',language_position)<return_position,
          "compact Settings puts Language and Enter on separate rows");
        ui_check(text.find(english?"7 Smooth camera (exp.) on":"7 Płynna kamera (test) wł.")!=std::string::npos&&
                 text.find(english?"5 Pause on focus loss off":"5 Pauza po utracie fokusu wył.")!=std::string::npos,
          "compact Settings shows the focus and camera values");
      }
    }
  }
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
  Controls pick_district=enter;pick_district.navigate=-1;guidance.tick(pick_district,0);
  ui_check(guidance.state.adventure.map&&guidance.state.adventure.selected==1&&guidance.profile.visit_district<0,
    "navigating and confirming together leaves the newly selected district on the map");
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
  camera_settings.state.paused=true;
  auto &camera_player=camera_settings.state.player;
  camera_player.x=camera_settings.grid.width()*.5;camera_player.y=camera_settings.grid.height()*.5;
  camera_player.vx=kDashSpeed*.7;camera_player.vy=-kDashSpeed*.7;
  camera_settings.tick({},0,true,38,28,2);
  ui_check(std::abs(camera_settings.state.camera_x-(camera_player.x-38*.4*.42))<.001&&
    std::abs(camera_settings.state.camera_y-(camera_player.y-21*.8*.68))<.001,
    "resizing during a dash centers the smooth camera without adding velocity lookahead");
  camera_settings.profile.smooth_camera=false;
  camera_settings.tick({},0,true,39,28,2);
  ui_check(std::abs(camera_settings.state.camera_x-(camera_player.x-39*.4*.42+camera_player.vx*.15))<.001&&
    std::abs(camera_settings.state.camera_y-(camera_player.y-21*.8*.68+camera_player.vy*.065))<.001,
    "resizing retains the standard camera's velocity lookahead");
  camera_settings.state.paused=false;
  for(bool smooth:{false,true}) {
    camera_settings.profile.smooth_camera=smooth;
    camera_settings.state.camera_x=camera_settings.grid.width()*.5;
    camera_settings.state.camera_y=camera_settings.grid.height()*.5;
    camera_player.y=camera_settings.grid.height()+6.;
    const int previous_deaths=camera_settings.state.deaths;
    camera_settings.tick({},1./120,true,39,28,2);
    const auto &view=camera_settings.state;
    const double width=39*.4,height=21*.8;
    const double expected_x=std::clamp(view.spawn_x-width*.42,0.,camera_settings.grid.width()-width);
    const double expected_y=std::clamp(view.spawn_y-height*.68,0.,camera_settings.grid.height()-height);
    ui_check(view.deaths==previous_deaths+1&&camera_player.x==view.spawn_x&&camera_player.y==view.spawn_y&&
      std::abs(view.camera_x-expected_x)<.001&&std::abs(view.camera_y-expected_y)<.001,
      "the camera immediately frames the checkpoint after a fall with either camera setting");
    const bool en=!smooth;
    camera_settings.profile.language=en?iceberg::Language::English:iceberg::Language::Polish;
    camera_settings.sync_view();
    RenderGrid(camera_settings.grid,view,en?24:20,en?34:60,2).draw();
    ui_check(ui_text(frame).find(en?"DIED · BACK AT CHECKPOINT":"ŚMIERĆ · POWRÓT DO LAMPY")!=std::string::npos,
      "the checkpoint banner visibly distinguishes a death in both languages");
  }
  Controls voluntary_retry;voluntary_retry.restart=true;
  camera_settings.tick(voluntary_retry,1./120,true,39,28,2);
  ui_check(camera_settings.state.adventure.death_left==0,"manual Retry does not show a death banner");
  camera_settings.profile.smooth_camera=true;
  camera_settings.start_tour(0);
  ui_check(camera_settings.profile.smooth_camera,"debug tour uses the selected camera preference");
  camera_settings.end_tour();
  ui_check(camera_settings.profile.smooth_camera,"leaving the debug tour restores the profile camera preference");
  AdventureSession detour(options,"/tmp/tv-ui-room-detour-unused");detour.begin_campaign(true);
  detour.profile.language=iceberg::Language::English;
  detour.profile.hidden_room=0;detour.load_room();
  detour.tick(map,1./120);
  ui_check(detour.state.adventure.map_room&&!detour.state.adventure.route.available,
    "the map opens to the current detour without exposing it on the campaign route");
  detour.state.player.x+=8;
  auto count_marker=[&](const char *glyph,uint32_t color){
    int marks=0;
    for(const auto &row:frame)for(const auto &cell:row)
      marks+=cell.glyph==glyph&&cell.rgb==color;
    return marks;
  };
  for(auto language:{iceberg::Language::Polish,iceberg::Language::English}) {
    detour.profile.language=language;detour.sync_view();
    for(auto [cols,rows]:{std::pair{34,24},std::pair{60,20},std::pair{110,34}}) {
      RenderGrid(detour.grid,detour.state,rows,cols,2).draw();
      const auto text=ui_text(frame);
      ui_check(count_marker("↶",0x91d4ce)==2&&count_marker("▣",0xd7e8cd)==0&&
        text.find(language==iceberg::Language::English?"↶ Return":"↶ Powrót")!=std::string::npos,
        "the detour minimap identifies its real return without advertising a finish door");
    }
  }
  detour.profile.hidden_room=2;detour.load_room();
  detour.state.adventure.map=detour.state.adventure.map_room=true;
  for(auto language:{iceberg::Language::Polish,iceberg::Language::English}) {
    detour.profile.language=language;detour.sync_view();
    for(auto [cols,rows]:{std::pair{34,24},std::pair{60,20},std::pair{110,34}}) {
      RenderGrid(detour.grid,detour.state,rows,cols,2).draw();
      const auto text=ui_text(frame);
      ui_check(count_marker("↕",0xe4ad68)==2&&count_marker("↶",0x91d4ce)==2&&
        count_marker("≈",0x66bfd7)>1&&text.find(language==iceberg::Language::English?
          "↕ Service lift":"↕ Winda serwisowa")!=std::string::npos,
        "the reservoir room map shows the pool, service lift and return at each supported size");
    }
  }
  detour.profile.hidden_room=-1;
  detour.profile.revisit.active=true;
  detour.load_room();
  detour.tick(map,0);
  if(!detour.state.adventure.map_room){detour.tick(tab,0);detour.tick(tab,0);}
  RenderGrid(detour.grid,detour.state,24,34,2).draw();
  ui_check(count_marker("↶",0x91d4ce)==2&&count_marker("▣",0xd7e8cd)==1&&
    ui_text(frame).find("▣ Exit")!=std::string::npos,
    "a revisited course distinguishes the early return from the course's exit");
  {
    AdventureSession flume(options,{});
    const auto sample=std::find_if(iceberg::DebugTour.begin(),iceberg::DebugTour.end(),
      [](const auto &stop){return stop.room==58;});
    ui_check(sample!=iceberg::DebugTour.end(),"the Leaf Chute water has a playable tour sample");
    flume.start_tour(int(sample-iceberg::DebugTour.begin()));
    const auto leaf=std::find_if(flume.grid.platforms.begin(),flume.grid.platforms.end(),
      [](const auto &platform){return platform.chute_leaf;});
    ui_check(leaf!=flume.grid.platforms.end(),"the Leaf Chute sample contains its flume platform");
    constexpr double mid_ride=2.8;
    const auto deck=leaf->at(mid_ride);
    flume.state.player.x=deck.x+deck.w*.5;
    flume.state.player.y=deck.y-kPlayerHeight;
    for(const auto &[cols,rows]:{std::pair{34,24},std::pair{60,20},std::pair{110,34}}) {
      update_camera(flume.state,flume.grid,cols,rows-7,2,10);
      const auto water_columns=[&](double time) {
        flume.state.elapsed=time;
        RenderGrid(flume.grid,flume.state,rows,cols,2).draw();
        std::vector<int> foam;
        int surface=0;
        for(size_t y=0;y<frame.size();++y)for(size_t x=0;x<frame[y].size();++x) {
          const auto &cell=frame[y][x];
          if(cell.background!=0x174e63)continue;
          ++surface;
          if(cell.rgb==0xa8e4ed)foam.push_back(int(y)*cols+int(x));
        }
        ui_check(surface>=8&&foam.size()>=2,"a visible blue surface marks the flume at each supported size");
        return foam;
      };
      const auto initial=water_columns(mid_ride);
      ui_check(initial!=water_columns(mid_ride+.25),"flume foam moves while the camera stays fixed");
    }
  }
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
  {
    auto hulls=verified_room(403,iceberg::mix(73),0);
    auto view=new_state(hulls);
    view.adventure.map=view.adventure.map_room=true;
    for(auto language:{iceberg::Language::Polish,iceberg::Language::English}) {
      view.adventure.language=language;
      const auto legend=language==iceberg::Language::English?"░ Hazard":"░ Ryzyko";
      for(auto [cols,rows]:{std::pair{34,24},std::pair{56,20},std::pair{110,34}}) {
        view.player.x=hulls.spawn_x;view.player.y=hulls.spawn_y;
        RenderGrid(hulls,view,rows,cols,2).draw();
        const int width=std::min(cols-8,72),left=(cols-width)/2;
        auto danger_column=[&](int x) {
          return left+x*width/hulls.width();
        };
        auto has_ledge_spike=[&](int x) {
          return std::any_of(frame.begin()+6,frame.end()-6,[&](const auto &line){
            const auto &cell=line[size_t(danger_column(x))];
            return cell.glyph=="░"&&cell.rgb==0xe09386&&
              std::count_if(line.begin(),line.end(),[](const auto &c){return c.rgb==0xe09386;})<width/2;
          });
        };
        ui_check(has_ledge_spike(30)&&has_ledge_spike(80)&&ui_text(frame).find(legend)!=std::string::npos,
          "both Between the hulls spikes appear above the fall hazard and have a localized legend");
        view.player.x=30.-kPlayerWidth*.5;view.player.y=47.-kPlayerHeight*.5;
        RenderGrid(hulls,view,rows,cols,2).draw();
        ui_check(std::any_of(frame.begin()+6,frame.end()-6,[&](const auto &line){
          const auto &cell=line[size_t(danger_column(30))];
          return cell.glyph=="@"&&cell.rgb==0xffd68a&&cell.background==0x4e2e35;
        }),"the courier stays visible when its map marker overlaps a spike");
      }
    }
  }
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
  auto map_markers=[&](const char *glyph,uint32_t color){
    std::vector<std::pair<size_t,size_t>> cells;
    for(size_t y=6;y+7<frame.size();++y)for(size_t x=0;x<frame[y].size();++x)
      if(frame[y][x].glyph==glyph&&frame[y][x].rgb==color)cells.emplace_back(y,x);
    return cells;
  };
  const auto first_cars=map_markers("▰",0xefcd74),stops=map_markers("○",0xc2accf);
  tram_view.elapsed=3;
  tram_view.adventure.language=iceberg::Language::English;
  RenderGrid(tram_room,tram_view,34,110,2).draw();
  ui_check(map_markers("▰",0xefcd74)!=first_cars&&map_markers("○",0xc2accf)==stops&&
    ui_text(frame).find("○ Stop")!=std::string::npos,
    "tram markers follow the actual cars across their routes");
  auto shutter_room=verified_room(69,73,5);
  auto shutter_view=new_state(shutter_room);
  shutter_view.adventure.map=shutter_view.adventure.map_room=true;
  RenderGrid(shutter_room,shutter_view,24,34,2).draw();
  ui_check(map_markers("↕",0xe4ad68).empty()&&ui_text(frame).find("Winda serwisowa")==std::string::npos,
    "the original counterweights room has no service lift marker");
  add_shutter_service_lift(shutter_room);
  shutter_view=new_state(shutter_room);
  shutter_view.adventure.map=shutter_view.adventure.map_room=true;
  for(auto language:{iceberg::Language::Polish,iceberg::Language::English}) {
    shutter_view.adventure.language=language;
    const auto legend=language==iceberg::Language::English?"↕ Service lift":"↕ Winda serwisowa";
    for(auto [cols,rows]:{std::pair{34,24},std::pair{60,20},std::pair{110,34}}) {
      shutter_view.elapsed=0.;
      RenderGrid(shutter_room,shutter_view,rows,cols,2).draw();
      const auto bottom=map_markers("↕",0xe4ad68);
      ui_check(bottom.size()==1&&count_map("┆",0x718c9d)>1&&ui_text(frame).find(legend)!=std::string::npos,
        "the room map marks the Crown service lift and its shaft");
      shutter_view.elapsed=3.6;
      RenderGrid(shutter_room,shutter_view,rows,cols,2).draw();
      const auto top=map_markers("↕",0xe4ad68);
      ui_check(top.size()==1&&bottom[0].second==top[0].second&&bottom[0].first>top[0].first,
        "the service lift marker follows the deck vertically");
    }
  }
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
    const auto legend=language==iceberg::Language::English?"◀ ▶ Belts":"◀ ▶ Taśmy";
    for(auto [cols,rows]:{std::pair{34,24},std::pair{60,20},std::pair{110,34}}) {
      RenderGrid(belt_room,belt_view,rows,cols,2).draw();
      ui_check(count_map("▶",0xa0d6df)==2&&count_map("◀",0xa0d6df)==1&&
        ui_text(frame).find(legend)!=std::string::npos,
        "the Archive map marks the two right-moving belts and the left-moving belt");
      const auto relay_label=language==iceberg::Language::English?"⇄ Belt relay":"⇄ Przekaźnik";
      ui_check(count_map("⇄",0xa0d6df)==1&&ui_text(frame).find(relay_label)!=std::string::npos,
        "the relay appears beside the reversible belt on the localized map");
      belt_room.switch_flags=1;
      RenderGrid(belt_room,belt_view,rows,cols,2).draw();
      ui_check(count_map("▶",0xa0d6df)==3&&count_map("◀",0xa0d6df)==0,
        "the room map shows all three Archive belts running right after reversal");
      belt_room.switch_flags=0;
    }
  }
  for(const auto &[room_id,district]:{std::pair{39,3},std::pair{52,4}}) {
    auto room=verified_room(room_id,73,district);
    const bool drawers=district==3;
    if(drawers)add_expedition_drawers(room);
    else add_expedition_channel_bridge(room);
    auto view=new_state(room);
    view.adventure.map=view.adventure.map_room=true;
    for(auto language:{iceberg::Language::Polish,iceberg::Language::English}) {
      view.adventure.language=language;
      const bool english=language==iceberg::Language::English;
      const char *control=drawers?(english?"⊗ Extend drawers":"⊗ Wysuń szuflady"):
        (english?"⊗ Water vines":"⊗ Podlej pnącza");
      const char *surface=drawers?(english?"═ Drawers":"═ Szuflady"):(english?"═ Vines":"═ Pnącza");
      for(auto [cols,rows]:{std::pair{34,24},std::pair{60,20},std::pair{110,34}}) {
        room.switch_flags=0;
        RenderGrid(room,view,rows,cols,2).draw();
        ui_check(count_map("⊗",0xabc9e4)==1&&count_map("═",0xabc9e4)==0&&
          ui_text(frame).find(control)!=std::string::npos,
          "the room map names the local mechanism and marks its inactive control");
        room.switch_flags=1;
        for(auto &bridge:room.growth)bridge.amount=0;
        RenderGrid(room,view,rows,cols,2).draw();
        ui_check(count_map("⊗",0xabc9e4)==0&&count_map("═",0xabc9e4)==0,
          "the opened control disappears before any foothold extends");
        for(auto &bridge:room.growth)bridge.amount=.5;
        RenderGrid(room,view,rows,cols,2).draw();
        const int partial_cells=count_map("═",0xabc9e4);
        for(auto &bridge:room.growth)bridge.amount=1;
        RenderGrid(room,view,rows,cols,2).draw();
        ui_check(partial_cells>0&&count_map("═",0xabc9e4)>partial_cells&&
          ui_text(frame).find(surface)!=std::string::npos,
          "the room map names the extended mechanism and draws only its usable surface");
      }
    }
    if(!drawers) {
      view.adventure.map=false;
      auto &vine=room.growth.front();
      view.camera_y=vine.area.y-8.2;
      for(auto [amount,glyph,color]:{std::tuple{.5,"❧",0xa6c780u},std::tuple{1.,"✿",0xe5d48fu}}) {
        vine.amount=amount;
        const double tip=vine.area.x+vine.extended_width(room.switch_flags);
        view.camera_x=tip-5.;
        RenderGrid(room,view,24,34,2).draw();
        ui_check(frame[12][11].glyph==glyph&&frame[12][11].rgb==color,
          "the vine readiness cue stays at the walkable edge in the compact view");
      }
    }
  }
  auto examination=iceworld::build_mastery_course(0);
  auto refill_view=new_state(examination);
  refill_view.adventure.map=refill_view.adventure.map_room=true;
  for(auto language:{iceberg::Language::Polish,iceberg::Language::English}) {
    refill_view.adventure.language=language;
    const bool en=language==iceberg::Language::English;
    for(auto [cols,rows]:{std::pair{34,24},std::pair{60,20},std::pair{110,34}}) {
      RenderGrid(examination,refill_view,rows,cols,2).draw();
      ui_check(count_map("▥",0x8ee0dd)==3&&count_map("▯",0x7895a0)==0&&
        ui_text(frame).find(en?"▥ Dash refill":"▥ Zapas zrywu")!=std::string::npos,
        "the Mastery room map identifies each refill in both languages");
      refill_view.refill_cooldown[0]=2;
      RenderGrid(examination,refill_view,rows,cols,2).draw();
      ui_check(count_map("▥",0x8ee0dd)==2&&count_map("▯",0x7895a0)==1&&
        ui_text(frame).find(en?"▯ Recharging":"▯ Ładuje się")!=std::string::npos,
        "the room map distinguishes a spent refill at compact widths");
      refill_view.refill_cooldown[0]=0;
    }
  }
  examination.dash_lock=1;examination.switch_flags=1;
  RenderGrid(examination,refill_view,24,34,2).draw();
  ui_check(count_map("▥",0x8ee0dd)==0,"the map omits dash refills while dashing is locked");
  const auto pressure_lifts=iceworld::build_heatworks_sequence(false);
  const auto &vent=pressure_lifts.pulses.front();
  auto vent_view=new_state(pressure_lifts);
  vent_view.adventure.map=vent_view.adventure.map_room=true;
  for(auto language:{iceberg::Language::Polish,iceberg::Language::English}) {
    vent_view.adventure.language=language;
    const bool en=language==iceberg::Language::English;
    for(auto [cols,rows]:{std::pair{34,24},std::pair{60,20},std::pair{110,34}}) {
      for(auto [phase,glyph,color]:{std::tuple{.40,"╧",0x987b6cu},
          std::tuple{.90,"↑",0xf1c66fu},std::tuple{.10,"♨",0xeb6a74u}}) {
        vent_view.elapsed=(1.+phase-vent.phase)*vent.period;
        RenderGrid(pressure_lifts,vent_view,rows,cols,2).draw();
        const auto text=ui_text(frame);
        ui_check(count_map(glyph,color)==1&&
          text.find(en?"╧ Vent · ↑ Warning · ♨ Active":"╧ Dysza · ↑ Uwaga · ♨ Aktywna")!=std::string::npos,
          "the room map shows the current steam vent phase and fits its full legend");
        auto exposed=vent_view;
        exposed.player.x=vent.area.x+(vent.area.w-kPlayerWidth)*.5;
        exposed.player.y=vent.area.y+(vent.area.h-kPlayerHeight)*.5;
        RenderGrid(pressure_lifts,exposed,rows,cols,2).draw();
        const auto bg=phase==.90?0x4a3d2bu:phase==.10?0x59303au:0u;
        bool courier=false;
        for(const auto &line:frame)for(const auto &cell:line)
          if(cell.glyph=="@"&&cell.rgb==0xffd68a)
            courier=bg?cell.background==bg:cell.background!=0x4a3d2bu&&cell.background!=0x59303au;
        ui_check(courier,"the steam footprint follows the vent phase under the courier marker");
      }
    }
  }
  vent_view.adventure.map=false;
  vent_view.camera_y=vent.area.y-3.3;
  vent_view.elapsed=(1.+.90-vent.phase)*vent.period;
  for(auto [cols,rows]:{std::pair{34,24},std::pair{60,20},std::pair{110,34}}) {
    for(int edge:{0,cols-1}) {
      vent_view.camera_x=edge==0?vent.area.right()-.2:vent.area.x-.4*edge-.05;
      RenderGrid(pressure_lifts,vent_view,rows,cols,2).draw();
      ui_check(std::any_of(frame.begin()+4,frame.end()-3,[&](const auto &row){
        const auto &cell=row[size_t(edge)];
        return cell.glyph=="▴"&&cell.rgb==0xf1c66f;
      }),"steam buildup remains visible at either clipped viewport edge");
    }
  }
  auto seam_room=verified_room(61,73,5);
  add_crown_seam_shortcut(seam_room);
  auto seam_view=new_state(seam_room);
  seam_view.adventure.map=seam_view.adventure.map_room=true;
  for(auto language:{iceberg::Language::Polish,iceberg::Language::English}) {
    seam_view.adventure.language=language;
    for(auto [cols,rows]:{std::pair{34,24},std::pair{60,20},std::pair{110,34}}) {
      RenderGrid(seam_room,seam_view,rows,cols,2).draw();
      ui_check(count_marker("↑",0xc4deeb)==2&&ui_text(frame).find(language==iceberg::Language::English?
        "↑ Wall-kick shaft":"↑ Wąski szyb")!=std::string::npos,
        "the room map marks the Crown shaft and fits its localized legend");
    }
  }
  {
    auto gutter=verified_room(33,73,2);
    add_estate_gutters(gutter);
    auto view=new_state(gutter);
    view.adventure.map=view.adventure.map_room=true;
    for(auto language:{iceberg::Language::Polish,iceberg::Language::English}) {
      view.adventure.language=language;
      for(auto [cols,rows]:{std::pair{34,24},std::pair{60,20},std::pair{110,34}}) {
        RenderGrid(gutter,view,rows,cols,2).draw();
        ui_check(count_marker("↑",0xc4deeb)==2&&count_map("@",0xffd68a)==1&&
          ui_text(frame).find(language==iceberg::Language::English?"↑ Gutter climb":"↑ Rynny")!=std::string::npos,
          "the Estate room map marks the optional gutter climb and retains the courier");
      }
    }
  }
  auto pots=verified_room(51,73,4);
  add_expedition_fragile_pots(pots);
  const size_t first_pot=pots.platforms.size()-2;
  const auto pots_view=new_state(pots);
  for(auto language:{iceberg::Language::Polish,iceberg::Language::English}) {
    const bool en=language==iceberg::Language::English;
    for(auto [cols,rows]:{std::pair{34,24},std::pair{60,20},std::pair{110,34}}) {
      auto view=pots_view;
      view.adventure.map=view.adventure.map_room=true;
      view.adventure.language=language;
      RenderGrid(pots,view,rows,cols,2).draw();
      const int intact=count_map("┅",0xd5aa80);
      ui_check(intact>1&&count_map("╍",0xf1c66f)==0&&
        ui_text(frame).find(en?"┅ Fragile · ╍ Cracking":"┅ Kruche · ╍ Pękają")!=std::string::npos,
        "the map identifies fragile planters before they crack");
      view.platform_armed_at[first_pot]=1.;
      view.elapsed=1.2;
      RenderGrid(pots,view,rows,cols,2).draw();
      const int cracking=count_map("╍",0xf1c66f);
      ui_check(cracking>0&&count_map("┅",0xd5aa80)+cracking==intact,
        "only the armed planter is shown cracking");
      view.elapsed=1.8;
      RenderGrid(pots,view,rows,cols,2).draw();
      ui_check(count_map("╍",0xf1c66f)==0&&count_map("┅",0xd5aa80)==intact-cracking,
        "the broken planter leaves the map while its neighbor remains");
      view.elapsed=4.;
      RenderGrid(pots,view,rows,cols,2).draw();
      ui_check(count_map("┅",0xd5aa80)==intact,"the restored planter returns as intact on the map");
    }
  }
  auto ribs=verified_room(63,73,5);
  add_fractured_ribs(ribs);
  for(const auto *room:{&pots,&ribs}) {
    const size_t first=room->platforms.size()-2;
    const auto &platform=room->platforms[first];
    for(auto language:{iceberg::Language::Polish,iceberg::Language::English}) {
      const bool en=language==iceberg::Language::English;
      const auto cue=room->district==5?(en?"Beam buckling · Jump!":"Belka się ugina · Skacz!"):
        (en?"Planter cracking · Jump!":"Donica pęka · Skacz!");
      for(auto [cols,rows]:{std::pair{34,24},std::pair{110,34}}) {
        auto view=new_state(*room);
        view.adventure.language=language;
        view.adventure.notice="Older notice";
        view.adventure.notice_left=8.;
        view.elapsed=1.1;
        view.player.x=platform.base.x+1.;
        view.player.y=platform.base.y-kPlayerHeight;
        view.player.support=int(first);
        view.player.grounded=true;
        view.platform_armed_at[first]=1.;
        RenderGrid(*room,view,rows,cols,2).draw();
        ui_check(ui_text(frame).find(cue)!=std::string::npos&&
                 ui_text(frame).find("Older notice")==std::string::npos,
          "the supported fragile platform gives a readable Jump cue over a timed notice");
        view.player.support=-1;
        view.player.grounded=false;
        RenderGrid(*room,view,rows,cols,2).draw();
        ui_check(ui_text(frame).find(cue)==std::string::npos,
          "the Jump cue clears when the courier leaves the fragile platform");
      }
    }
  }
  auto reservoir=verified_room(55,73,4);
  auto reservoir_view=new_state(reservoir);
  reservoir_view.adventure.map=reservoir_view.adventure.map_room=true;
  auto count_background=[&](uint32_t color,const char *glyph=nullptr){
    int cells=0;
    for(size_t y=6;y+7<frame.size();++y)for(const auto &cell:frame[y])
      cells+=cell.background==color&&(!glyph||cell.glyph==glyph);
    return cells;
  };
  RenderGrid(reservoir,reservoir_view,24,34,2).draw();
  ui_check(count_background(0x14394e)==0&&count_background(0x214b62)==0,
    "the dry reservoir has no water tint on the room map");
  add_reservoir_ice(reservoir);
  reservoir_view=new_state(reservoir);
  reservoir_view.adventure.map=reservoir_view.adventure.map_room=true;
  for(auto [cols,rows]:{std::pair{34,24},std::pair{38,28},std::pair{60,20},std::pair{110,34}}) {
    RenderGrid(reservoir,reservoir_view,rows,cols,2).draw();
    const int water=count_background(0x14394e);
    ui_check(water>0&&count_background(0x214b62,"┅")>0,
      "the reservoir map shows the blue basin below intact white ice");
    auto cracking_ice=reservoir_view;
    cracking_ice.elapsed=1.2;
    cracking_ice.platform_armed_at[0]=1.0;
    RenderGrid(reservoir,cracking_ice,rows,cols,2).draw();
    ui_check(count_background(0x214b62,"╍")>0&&count_background(0x214b62,"┅")>0,
      "the first cracking floe is distinguishable from intact ice over the same basin");
    const auto &floe=reservoir.platforms[0].base;
    cracking_ice.player.x=floe.x+floe.w*.5-kPlayerWidth*.5;
    cracking_ice.player.y=floe.y-kPlayerHeight;
    RenderGrid(reservoir,cracking_ice,rows,cols,2).draw();
    ui_check(count_map("╍",0xf1c66f)>0&&count_map("@",0xffd68a)==1,
      "the courier marker cannot hide the cracking floe warning");
    auto gap=reservoir_view;
    gap.elapsed=1.8;
    gap.platform_armed_at[0]=1.0;
    RenderGrid(reservoir,gap,rows,cols,2).draw();
    ui_check(count_background(0x14394e)==water&&count_map("×",0x85c0cd)>0&&
             count_background(0x214b62,"┅")>0&&ui_text(frame).find("× Przerwa")!=std::string::npos,
      "the broken floe is marked as a gap while adjacent ice stays on the map");
    gap.elapsed=4.;
    RenderGrid(reservoir,gap,rows,cols,2).draw();
    ui_check(count_map("×",0x85c0cd)==0&&count_background(0x214b62,"┅")>0,
      "the gap disappears when the floe recovers");
  }
  AdventureSession ribs_session(options,{});
  const auto ribs_sample=std::find_if(iceberg::DebugTour.begin(),iceberg::DebugTour.end(),
    [](const auto &stop){return stop.room==63;});
  ribs_session.start_tour(int(ribs_sample-iceberg::DebugTour.begin()));
  auto rib_view=ribs_session.state;
  rib_view.adventure.map=rib_view.adventure.map_room=true;
  rib_view.elapsed=1.8;
  rib_view.platform_armed_at[0]=1.;
  for(auto [cols,rows]:{std::pair{34,24},std::pair{60,20},std::pair{110,34}}) {
    for(auto language:{iceberg::Language::Polish,iceberg::Language::English}) {
      rib_view.adventure.language=language;
      const bool en=language==iceberg::Language::English;
      RenderGrid(ribs_session.grid,rib_view,rows,cols,2).draw();
      ui_check(count_map("×",0x85c0cd)==1&&count_map("@",0xffd68a)==1&&
        ui_text(frame).find(en?"× Gap":"× Przerwa")!=std::string::npos,
        "the Crown map shows a broken rib and preserves the courier at compact and wide sizes");
      auto recovered=rib_view;
      recovered.elapsed=4.;
      RenderGrid(ribs_session.grid,recovered,rows,cols,2).draw();
      ui_check(count_map("×",0x85c0cd)==0,
        "a reseated Crown rib removes the map gap at the existing recovery time");
    }
  }
  {
    AdventureSession gust(options,{});
    const auto tour=std::find_if(iceberg::DebugTour.begin(),iceberg::DebugTour.end(),
      [](const auto &stop){return stop.room==68;});
    gust.start_tour(int(tour-iceberg::DebugTour.begin()));
    auto view=gust.state;
    view.adventure.map=view.adventure.map_room=true;
    for(auto [cols,rows]:{std::pair{34,24},std::pair{56,20},std::pair{110,34}}) {
      for(auto language:{iceberg::Language::Polish,iceberg::Language::English}) {
        view.adventure.language=language;
        const bool en=language==iceberg::Language::English;
        for(const auto &[time,glyph]:{std::pair{0.,"○"},std::pair{1.8,"→"},std::pair{5.4,"←"}}) {
          view.elapsed=time;
          const bool lull=time==0.;
          const uint32_t tint=lull?0x222d40:0x2c3c54,marker=lull?0x8c9bb1:0xb4d7e4;
          const auto label=std::string(glyph)+(lull?(en?" Lull":" Cisza"):(en?" Wind":" Wiatr"));
          RenderGrid(gust.grid,view,rows,cols,2).draw();
          ui_check(count_background(tint)>8&&count_map(glyph,marker)==1&&count_map("@",0xffd68a)==1&&
            ui_text(frame).find(label)!=std::string::npos,
            "Ridge Sensors map shows the same lane through a lull, east gust and west gust");
        }
      }
    }

    auto sheltered=verified_room(70,73,5);
    strengthen_expedition_wind(sheltered);
    add_overhang_windbreak(sheltered);
    view=new_state(sheltered);
    view.adventure.map=view.adventure.map_room=true;
    view.elapsed=1.5;
    RenderGrid(sheltered,view,34,110,2).draw();
    int last_gust_row=-1,last_terrain_row=-1;
    for(size_t y=4;y+7<frame.size();++y)for(const auto &cell:frame[y]) {
      if(cell.background==0x2c3c54)last_gust_row=int(y);
      if(cell.glyph=="▪"&&cell.rgb==0x8ba2ae)last_terrain_row=int(y);
    }
    ui_check(last_gust_row>=0&&last_terrain_row>last_gust_row+1,
      "the overhang room-map tint leaves the sheltered terrain below the gust unshaded");

    auto shutters=iceworld::build_crown_sequence(false);
    view=new_state(shutters);
    view.adventure.map=view.adventure.map_room=true;
    view.elapsed=1.5;
    RenderGrid(shutters,view,24,34,2).draw();
    ui_check(count_background(0x2c3c54)>0,"the active Crown campaign lane is marked");
    shutters.switch_flags|=2;
    RenderGrid(shutters,view,24,34,2).draw();
    ui_check(count_background(0x2c3c54)==0&&count_background(0x222d40)==0,
      "disabling the campaign gust removes its room-map footprint");
  }
  for(const auto &[room,sign,letter,tube]:{std::array{45,2,2,0},std::array{57,0,0,2},
                                         std::array{61,2,0,0},std::array{49,0,0,0}}) {
    AdventureSession field(options,{});
    const auto sample=std::find_if(iceberg::DebugTour.begin(),iceberg::DebugTour.end(),
      [room](const auto &stop){return stop.room==room;});
    field.start_tour(int(sample-iceberg::DebugTour.begin()));
    field.state.adventure.map=field.state.adventure.map_room=true;
    for(auto language:{iceberg::Language::Polish,iceberg::Language::English}) {
      field.state.adventure.language=language;
      const bool en=language==iceberg::Language::English;
      for(auto [cols,rows]:{std::pair{34,24},std::pair{60,20},std::pair{110,34}}) {
        RenderGrid(field.grid,field.state,rows,cols,2).draw();
        const auto text=ui_text(frame);
        ui_check(count_marker("▤",0xe7d8b4)==sign&&count_marker("✉",0xf2d0a4)==letter&&
          count_marker("◖",0xd4bc88)==tube&&count_map("@",0xffd68a)==1&&
          (text.find(en?"▤ Route sign":"▤ Tabliczka")!=std::string::npos)==bool(sign)&&
          (text.find(en?"✉ Letter":"✉ List")!=std::string::npos)==bool(letter)&&
          (text.find(en?"◖ Speaking tube":"◖ Rura głosowa")!=std::string::npos)==bool(tube),
          "field landmarks and legends coexist with the courier on compact and wide room maps");
      }
    }
  }
  {
    AdventureSession lifts(options,{});
    lifts.start_tour(20);
    ui_check(lifts.grid.template_id==13&&lifts.grid.platforms.size()==4&&
      std::all_of(lifts.grid.platforms.begin(),lifts.grid.platforms.end(),
        [](const auto &platform){return platform.cargo_hoist&&platform.dx==0;}),
      "the Heatworks sample has four vertical cargo hoists");
    const auto &hoist=lifts.grid.platforms.front();
    ui_check(hoist.at(0).y==hoist.at(.4).y&&hoist.at(2.6).y==hoist.at(3.).y&&
      hoist.at(0).y-hoist.at(2.6).y==5.,"the cargo hoist waits at both ends of its five-unit travel");
    for(auto [cols,rows]:{std::pair{34,24},std::pair{60,20},std::pair{110,34}}) {
      lifts.state.elapsed=0;
      lifts.tick({},0,true,cols,rows);
      RenderGrid(lifts.grid,lifts.state,rows,cols,2).draw();
      int courier_cap=-1,mechanic_cap=cols;
      for(int y=4;y<rows-3;++y)for(int x=0;x<cols;++x) {
        const auto &cell=frame[size_t(y)][size_t(x)];
        if(cell.glyph!="▀")continue;
        if(cell.rgb==0xe4ad68)courier_cap=std::max(courier_cap,x);
        if(cell.rgb==0xd79743)mechanic_cap=std::min(mechanic_cap,x);
      }
      ui_check(courier_cap>=0&&mechanic_cap<cols&&courier_cap+3<=mechanic_cap,
        "entering Salvaged lifts leaves the Courier and Witek visibly apart");
      const auto lower_signal=[&]{
        for(int x=mechanic_cap+4;x<cols;++x)for(int y=4;y<rows-3;++y) {
          const auto &cell=frame[size_t(y)][size_t(x)];
          if((cell.glyph=="●"&&cell.rgb==0xb5d599)||(cell.glyph=="○"&&cell.rgb==0xb58952))return cell.rgb;
        }
        return uint32_t(0);
      };
      ui_check(lower_signal()==0xb5d599,"the lower relay shows green when the deck can be boarded");
      lifts.state.elapsed=.8;
      RenderGrid(lifts.grid,lifts.state,rows,cols,2).draw();
      ui_check(lower_signal()==0xb58952,"the lower relay stays amber while the deck rises");
      lifts.state.elapsed=2.6;
      RenderGrid(lifts.grid,lifts.state,rows,cols,2).draw();
      ui_check(lower_signal()==0xb58952,"the lower relay stays amber while the deck waits upstairs");
      lifts.state.elapsed=hoist.period+.1;
      RenderGrid(lifts.grid,lifts.state,rows,cols,2).draw();
      ui_check(lower_signal()==0xb5d599,"the lower relay turns green when the deck returns");
    }
    lifts.state.elapsed=0;
    lifts.interact();
    ui_check(lifts.state.adventure.dialogue.reply_scene==iceberg::HoistCrewReplyScene,
      "Witek remains within talking distance when the sample opens");
    lifts.state.adventure.dialogue={};
    lifts.state.player.x=hoist.base.x+2;
    lifts.state.player.y=hoist.at(0).y-kPlayerHeight;
    for(int tick=0;tick<26;++tick)lifts.tick({},.1);
    ui_check(lifts.state.player.support==0&&lifts.state.deaths==0&&
      std::abs(lifts.state.player.rect().bottom()-hoist.at(2.6).y)<.02,
      "a standing courier rides the cargo hoist to its upper landing");
    lifts.state.adventure.map=lifts.state.adventure.map_room=true;
    for(auto language:{iceberg::Language::Polish,iceberg::Language::English}) {
      lifts.state.adventure.language=language;
      for(auto [cols,rows]:{std::pair{34,24},std::pair{60,20},std::pair{110,34}}) {
        RenderGrid(lifts.grid,lifts.state,rows,cols,2).draw();
        ui_check(count_marker("↕",0xe4ad68)==5&&count_map("@",0xffd68a)==1&&
          ui_text(frame).find(language==iceberg::Language::English?"↕ Cargo hoist":"↕ Winda towarowa")!=std::string::npos,
          "the room map shows all four hoists and their localized legend");
      }
    }
  }
  {
    auto room=verified_room(421,73,1);
    add_heatworks_twin_pistons(room);
    auto view=new_state(room);
    view.adventure.map=view.adventure.map_room=true;
    const auto &first=room.platforms[3];
    view.player.x=first.base.x+first.base.w/2-kPlayerWidth/2;
    view.player.y=first.at(0).y-kPlayerHeight;
    for(auto language:{iceberg::Language::Polish,iceberg::Language::English}) {
      view.adventure.language=language;
      for(auto [cols,rows]:{std::pair{34,24},std::pair{60,20},std::pair{110,34}}) {
        view.elapsed=.4;
        RenderGrid(room,view,rows,cols,2).draw();
        ui_check(count_map("↕",0xb5d599)==2&&count_map("○",0xc2accf)==2&&count_map("@",0xffd68a)==1&&
          ui_text(frame).find(language==iceberg::Language::English?"↕ Pressure piston · ○ Dock":"↕ Tłok · ○ Postój")!=std::string::npos,
          "Twin Pistons map shows both docked decks, both vacant stops, and an overlapping courier");
        view.elapsed=1.5;
        RenderGrid(room,view,rows,cols,2).draw();
        ui_check(count_map("↕",0xe4ad68)==2&&count_map("↕",0xb5d599)==0&&count_map("○",0xc2accf)==4,
          "Twin Pistons map marks all four stops while both decks travel");
      }
    }
  }
  {
    AdventureSession wheel(options,{});
    const auto sample=std::find_if(iceberg::DebugTour.begin(),iceberg::DebugTour.end(),
      [](const auto &stop){return stop.room==57;});
    wheel.start_tour(int(sample-iceberg::DebugTour.begin()));
    int scoops=std::count_if(wheel.grid.platforms.begin(),wheel.grid.platforms.end(),
      [](const auto &deck){return deck.irrigation_scoop;});
    ui_check(scoops==4,"the irrigation wheel has four quarter-spaced scoops");
    Controls approach;approach.move=-1;approach.jump_held=true;approach.dash_pressed=true;
    wheel.tick(approach,.1);
    approach.dash_pressed=false;
    for(int i=0;i<3;++i)wheel.tick(approach,.1);
    approach.move=0;wheel.tick(approach,.1);
    const int support=wheel.state.player.support;
    ui_check(support>=0&&wheel.grid.platforms[size_t(support)].irrigation_scoop,
      "dashing off the terrace reaches an orbiting scoop");
    const double low=wheel.state.player.rect().bottom();
    for(int i=0;i<21;++i)wheel.tick({},.1);
    ui_check(wheel.state.player.support==support&&wheel.state.deaths==0&&
             wheel.state.player.rect().bottom()<low-10,
      "the wheel carries the courier more than ten units above the boarding point");
    wheel.state.adventure.map=wheel.state.adventure.map_room=true;
    for(auto language:{iceberg::Language::Polish,iceberg::Language::English}) {
      wheel.state.adventure.language=language;
      for(auto [cols,rows]:{std::pair{34,24},std::pair{60,20},std::pair{110,34}}) {
        RenderGrid(wheel.grid,wheel.state,rows,cols,2).draw();
        ui_check(count_marker("⊕",0xc5a36d)==2&&count_map("@",0xffd68a)==1&&
          ui_text(frame).find(language==iceberg::Language::English?"⊕ Irrigation wheel":"⊕ Koło nawadniające")!=std::string::npos,
          "the irrigation map shows the axle and the rider with a localized legend");
      }
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
  for(unsigned layout:{3,4}) {
    AdventureSession archive(options,{});
    archive.begin_campaign(true);
    archive.profile.unlocked=archive.run_district=3;
    auto &expedition=archive.profile.expeditions[0];
    expedition.active=true;
    expedition.district=3;
    expedition.seed=expedition.origin_seed=73;
    expedition.layout_version=layout;
    expedition.itinerary={45,36,37,38,39,40,41,42};
    archive.begin_run(iceberg::Mode::Endless);
    const bool installed=std::any_of(archive.grid.platforms.begin(),archive.grid.platforms.end(),
      [](const auto &platform){return platform.archive_trolley;});
    ui_check(archive.grid.template_id==45&&installed==(layout>=4),
      "Endless applies the trolley only to current Archive itineraries");
  }
  auto envelope_room=verified_room(45,73,3);
  ui_check(std::none_of(envelope_room.platforms.begin(),envelope_room.platforms.end(),
    [](const auto &platform){return platform.archive_trolley;}),
    "the reference Envelope Ceiling route keeps its original platforms");
  add_expedition_archive_trolley(envelope_room);
  ui_check(envelope_room.platforms.size()==1&&envelope_room.platforms.front().archive_trolley,
    "the current Envelope Ceiling expedition has a single diagonal trolley");
  auto wet_ceiling=new_state(envelope_room);
  wet_ceiling.camera_x=56;
  wet_ceiling.camera_y=22;
  wet_ceiling.adventure.presentation_time=.65;
  RenderGrid(envelope_room,wet_ceiling,34,110,2).draw();
  auto drip_row=[&]() {
    for(int y=0;y<int(frame.size());++y)for(const auto &cell:frame[size_t(y)])
      if(cell.glyph=="╵"&&cell.rgb==0xa0c4d0)return y;
    return -1;
  };
  const int first_drip=drip_row();
  wet_ceiling.adventure.presentation_time=1.04;
  RenderGrid(envelope_room,wet_ceiling,34,110,2).draw();
  ui_check(first_drip>0&&drip_row()>first_drip,"the canopy drain visibly falls toward the walkway");
  const auto &trolley=envelope_room.platforms.front();
  auto trolley_view=new_state(envelope_room);
  const auto start_deck=trolley.at(0);
  const auto upper_deck=trolley.at(3.6);
  const auto same_deck=[&](const Rect &expected,double time) {
    const auto actual=trolley.at(time);
    return actual.x==expected.x&&actual.y==expected.y;
  };
  ui_check(same_deck(start_deck,.7)&&same_deck(upper_deck,4.3)&&
    !same_deck(start_deck,1.2)&&!same_deck(upper_deck,4.8),
    "the trolley waits briefly at both stops before resuming travel");

  auto trolley_lamps=trolley_view;
  trolley_lamps.camera_x=69.9;
  trolley_lamps.camera_y=17;
  for(const auto &[time,lit_column]:{std::pair{0.,36},std::pair{1.8,-1},std::pair{3.6,86},std::pair{5.4,-1}}) {
    trolley_lamps.elapsed=time;
    RenderGrid(envelope_room,trolley_lamps,42,110,2).draw();
    ui_check(count_marker("●",0xb5dba0)==int(lit_column>=0)&&
      (lit_column<0||std::any_of(frame.begin(),frame.end(),[&](const auto &line){
        return line[size_t(lit_column)].glyph=="●"&&line[size_t(lit_column)].rgb==0xb5dba0;
      })),"only the docked Archive trolley landing lights its lamp");
  }

  trolley_view.player.x=start_deck.x+(start_deck.w-kPlayerWidth)*.5;
  trolley_view.player.y=start_deck.y-kPlayerHeight;
  trolley_view.player.grounded=true;
  trolley_view.player.support=0;
  for(int tick=0;tick<432;++tick)world_step(trolley_view,envelope_room,{},1./120);
  ui_check(trolley_view.player.support==0&&trolley_view.deaths==0&&
           trolley_view.player.x>start_deck.x+15&&trolley_view.player.y<start_deck.y-15,
    "the trolley carries a rider across and up to the envelope shelf");
  trolley_view.adventure.map=trolley_view.adventure.map_room=true;
  for(auto language:{iceberg::Language::Polish,iceberg::Language::English}) {
    trolley_view.adventure.language=language;
    const auto label=language==iceberg::Language::English?"▱ Trolley":"▱ Wózek";
    for(auto [cols,rows]:{std::pair{34,24},std::pair{60,20},std::pair{110,34}}) {
      for(double phase:{0.,1.8,3.6,5.4}) {
        trolley_view.elapsed=phase;
        RenderGrid(envelope_room,trolley_view,rows,cols,2).draw();
        const int docked=phase==0.||phase==3.6?1:0;
        ui_check(count_map("▱",0xaee1df)>=1&&count_map("@",0xffd68a)==1&&
          count_map("○",0xc2accf)>=2-docked&&count_map("●",0xb5dba0)==docked&&
          count_map("┄",0x6f9eaa)>0&&
          ui_text(frame).find(label)!=std::string::npos&&ui_text(frame).find(language==iceberg::Language::English?
            "○ Stop":"○ Przystanek")!=std::string::npos,
          "the trolley map shows the rider, both stops, and its localized legend at every phase");
      }
    }
  }
  {
    AdventureSession tour(options,{});
    const auto stop=std::find_if(iceberg::DebugTour.begin(),iceberg::DebugTour.end(),
      [](const auto &entry){return entry.room==45;});
    const int sample=int(stop-iceberg::DebugTour.begin());
    tour.start_tour(sample);
    const auto initial=tour.grid.platforms.front().at(0);
    const double landing_x=tour.state.spawn_x,landing_y=tour.state.spawn_y;
    tour.state.elapsed=3.6;
    tour.state.player.x=100; tour.state.player.y=24;
    tour.state.collected.back()=true;
    tour.state.checkpoint=1;
    tour.state.deaths=2;
    Controls pause;pause.pause=true;tour.tick(pause,0);
    Controls retry;retry.restart=true;retry.navigate=1;retry.confirm=true;
    tour.tick(retry,0);
    const auto &restarted=tour.state;
    const auto deck=tour.grid.platforms.front().at(restarted.elapsed);
    ui_check(!restarted.paused&&tour.tour_scene==sample&&restarted.deaths==3&&restarted.elapsed==0&&
      restarted.player.x==landing_x&&restarted.player.y==landing_y&&
      deck.x==initial.x&&deck.y==initial.y,
      "Retry from Pause restarts the same Debug Tour sample and machinery immediately");
    ui_check(restarted.sweets_count()==0&&restarted.checkpoint==0&&restarted.player.support<0,
      "Retry restores the sample's original pickups and landing");
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
  {
    const auto lesson=iceworld::build_harbor_lesson_v4();
    auto ferry=new_state(lesson);
    ferry.adventure.door_left=0.;
    ferry.camera_y=34.7;
    for(auto [size,rows]:{std::pair{34,24},std::pair{110,34}}) {
      for(int side:{-1,1}) {
        const double x=side<0?89.8:122.2;
        ferry.camera_x=x-5.;
        for(int phase:{-1,0,1}) {
          ferry.elapsed=phase==-1?0.:phase==0?1.7:3.4;
          RenderGrid(lesson,ferry,rows,size,2.).draw();
          const int lamp_row=side<0?17:14;
          const auto &lamp=frame.at(lamp_row).at(12);
          const auto &foot=frame.at(lamp_row+3).at(12);
          ui_check(lamp.glyph==(phase==side?(side<0?"→":"←"):"○")&&
                   lamp.rgb==(phase==side?0xb0e4a8:0xb18b61)&&foot.glyph=="╨",
            "the lesson ferry lights only the near landing and keeps its post mounted");
        }
      }
    }
  }
  {
    const auto lesson=iceworld::build_harbor_lesson_v4();
    auto ferry=new_state(lesson);
    ferry.adventure.map=ferry.adventure.map_room=true;
    ferry.player.x=89.8;ferry.player.y=49.-kPlayerHeight;
    for(auto language:{iceberg::Language::English,iceberg::Language::Polish}) {
      ferry.adventure.language=language;
      for(auto [cols,rows]:{std::pair{34,24},std::pair{56,20},std::pair{110,34}}) {
        for(int phase:{-1,0,1}) {
          ferry.elapsed=(phase+1)*1.7;
          RenderGrid(lesson,ferry,rows,cols,2.).draw();
          const auto label=language==iceberg::Language::English?"→/← Jump · ○ Wait":"→/← Skacz · ○ Czekaj";
          ui_check(count_map("@",0xffd68a)==1&&count_map("▱",0xe4ad68)==1&&
            count_map("○",0xb18b61)==(phase==0?2:1)&&count_map("→",0xb0e4a8)==(phase==-1?1:0)&&
            count_map("←",0xb0e4a8)==(phase==1?1:0)&&ui_text(frame).find(label)!=std::string::npos,
            "the lesson map shows the correct landing signals without covering the courier or ferry");
        }
      }
    }
  }
  for(const int id:{601,504,50}) {
    auto grid=id==601?iceworld::build_harbor_lesson_v4():iceworld::build_room(id,73,id==50?4:0);
    auto state=new_state(grid);
    state.adventure.map=state.adventure.map_room=true;
    const bool leaf=id==50;
    const auto ferry_glyph=leaf?"◒":"▱";
    const uint32_t ferry_rgb=leaf?0xb6d778:0xe4ad68;
    ui_check(grid.water_basins.size()==size_t(id==601?1:3),"each ferry crossing has its own water basin");
    auto boat_positions=[&](){
      std::vector<int> positions;
      for(size_t row=4;row+7<frame.size();++row)
        for(size_t col=0;col<frame[row].size();++col)
          if(frame[row][col].glyph==ferry_glyph&&frame[row][col].rgb==ferry_rgb)positions.push_back(int(col));
      return positions;
    };
    for(auto [cols,rows]:{std::pair{34,24},std::pair{60,20},std::pair{110,34}}) {
      state.elapsed=0;
      state.adventure.language=iceberg::Language::English;
      RenderGrid(grid,state,rows,cols,2).draw();
      const auto before=boat_positions();
      ui_check(before.size()==size_t(id==601?1:3)&&count_map("┄",0x6f9eaa)>0&&
        ui_text(frame).find(leaf?"◒ Leaf · ┄ Route":"▱ Ferry · ┄ Route")!=std::string::npos&&
        count_map("≈",0x66bfd7)>0,
        "the compact and wide maps distinguish ferries from fixed decks and show their route");
      state.elapsed=grid.platforms.front().period*.5;
      state.adventure.language=iceberg::Language::Polish;
      RenderGrid(grid,state,rows,cols,2).draw();
      const auto after=boat_positions();
      ui_check(before.size()==after.size()&&before!=after&&ui_text(frame).find(leaf?"◒ Liść · ┄ Trasa":"▱ Prom · ┄ Trasa")!=std::string::npos,
        "the translated ferry markers follow the crossing phase");
    }
  }
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
  {
    auto archive_grid=iceworld::build_room(36,73,3);
    auto archive_view=new_state(archive_grid);
    auto &v=archive_view.adventure;
    v.district=3;v.map=v.map_route=v.route.available=true;
    v.route.current=0;v.route.place=archive_grid.room_name;
    for(int id:iceworld::make_itinerary(73,3))v.route.rooms.push_back(iceworld::pattern(id).name);
    auto harbor_view=harbor_fork.state;
    auto &harbor=harbor_view.adventure;
    harbor.map_room=false;harbor.map_route=true;
    harbor.route.alternatives[0]=iceworld::pattern(511).name;
    const auto route_text=[&] {
      std::string text;
      for(const auto &line:frame) {
        for(size_t x=1;x+1<line.size();++x)text+=line[x].glyph;
        text+=' ';
      }
      return normalize_ui(text);
    };
    for(auto language:{iceberg::Language::English,iceberg::Language::Polish}) {
      v.language=harbor.language=language;
      const bool en=language==iceberg::Language::English;
      for(auto [cols,rows]:{std::pair{34,24},std::pair{38,28},std::pair{56,20},std::pair{110,34}}) {
        RenderGrid(archive_grid,archive_view,rows,cols,2).draw();
        const auto archive_map=route_text();
        const auto room_name=archive_grid.room_name.get(language);
        ui_check(archive_map.find(std::string(en?"Leg 1 / 3 · ":"Odcinek 1 / 3 · ")+room_name)!=std::string::npos&&
          archive_map.find(std::string(en?"Now: ":"Teraz: ")+room_name)!=std::string::npos,
          "the campaign map keeps the full leg and current room names at every supported size");
        RenderGrid(harbor_fork.grid,harbor_view,rows,cols,2).draw();
        const auto harbor_map=route_text();
        ui_check(harbor_map.find("B: "+harbor.route.alternatives[0].get(language))!=std::string::npos&&
          harbor_map.find(en?"visited":"odwiedzone")!=std::string::npos,
          "the route map wraps the full Harbor alternate name above its legend");
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
  {
    auto &courier=paused.state.player;
    courier.x+=1.;
    paused.tick(pause,0);
    Controls retry;retry.restart=true;retry.move=1;retry.dash_pressed=retry.jump_pressed=true;
    paused.tick(retry,1./120,true,33,23);
    ui_check(paused.state.paused&&paused.state.deaths==0,"Retry leaves Pause in place when the room cannot fit");
    paused.tick(retry,0);
    ui_check(!paused.state.paused&&paused.state.deaths==0,"Retry leaves Pause even before the next physics tick");
    paused.tick({},1./120);
    ui_check(paused.state.deaths==1&&courier.x==paused.state.spawn_x&&courier.y==paused.state.spawn_y&&
             courier.dash_time_left==0.&&courier.jump_buffer_left==0.,
             "Retry returns to the checkpoint once without passing buffered movement");
    paused.tick({},1./120);
    ui_check(paused.state.deaths==1,"Retry from Pause is consumed exactly once");
  }
  for(Controls overlay:{pause,map,book}) {
    paused.state.player.landing_dash_left=.08;
    Controls dash;dash.dash_pressed=true;dash.aim_y=-1;
    paused.tick(dash,1./120);
    ui_check(paused.state.player.dash_buffer_left>0.,"a dash is queued while the charge recovers");
    paused.state.player.jump_buffer_left=.10;
    paused.tick(overlay,1./120);
    ui_check(paused.state.player.dash_buffer_left==0.&&paused.state.player.jump_buffer_left==0.,
             "opening an overlay drops unspent movement");
    paused.tick(overlay,1./120);
    for(int i=0;i<16;++i)paused.tick({},1./120);
    ui_check(paused.state.player.dash_time_left==0.,"an old dash cannot fire on returning to play");
  }
  for(bool focus_pause:{false,true}) {
    paused.profile.pause_on_focus_loss=focus_pause;
    paused.state.player.landing_dash_left=.08;
    Controls dash;dash.dash_pressed=true;dash.aim_y=-1;
    paused.tick(dash,1./120);
    paused.tick({},0.,false);
    ui_check(paused.state.paused==focus_pause&&paused.state.player.dash_buffer_left==0.,
             "focus loss drops queued movement while respecting the pause setting");
    paused.tick(enter,1./120,true);
  }
  paused.state.player.dash_time_left=.1;
  paused.tick(pause,1./120);
  ui_check(paused.state.player.dash_time_left==.1,"pause keeps the remaining time of a dash already in motion");
  paused.tick(pause,1./120);
  paused.state.player.dash_time_left=0.;
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
  for(bool paused:{false,true}) {
    game.state.paused=paused;
    game.tick(use,1./120);
    ui_check(game.state.adventure.dialogue.active(),"the completed record can be reopened");
    Controls map_and_use=map;map_and_use.interact=true;
    game.tick(map_and_use,1./120);
    ui_check(game.state.adventure.map&&!game.state.journal&&!game.state.adventure.dialogue.active()&&
             game.state.paused==paused&&game.profile.visit_district<0,
             "Map opens directly from a journal record without travelling or resuming");
    game.tick(book,1./120);
    game.tick(use,1./120);
    ui_check(game.state.journal&&game.state.adventure.dialogue.active(),"the journal keeps its completed tab");
    Controls book_and_use=book;book_and_use.interact=true;
    game.tick(book_and_use,1./120);
    ui_check(!game.state.journal&&!game.state.adventure.map&&!game.state.adventure.dialogue.active()&&
             game.state.paused==paused&&game.profile.quest_focus==1,
             "Quests closes the journal and record without changing focus or Pause");
    game.tick(book,1./120);
  }
  game.state.paused=false;
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
  AdventureSession tour_repeat(options,"/tmp/tv-ui-tour-repeat-unused");
  tour_repeat.begin_campaign(true);
  tour_repeat.state.menu=tour_repeat.state.adventure.tour_picker=true;
  auto &picker=tour_repeat.state.adventure;
  const int last=int(iceberg::DebugTour.size())-1;
  Controls down;down.navigate=down.aim_y=1;
  tour_repeat.tick(down,0);
  down.navigate=0;
  for(int i=0;i<3;++i)tour_repeat.tick(down,.1);
  ui_check(picker.picker_choice==1,"the Tour picker waits before repeating held Down");
  tour_repeat.tick(down,.1);
  tour_repeat.tick({},0);
  for(int i=0;i<5;++i)tour_repeat.tick(down,.1);
  ui_check(picker.picker_choice==2,"the Tour picker requires another press after release");
  down.navigate=1;
  tour_repeat.tick(down,0);
  down.navigate=0;
  for(int i=0;i<40;++i)tour_repeat.tick(down,.1);
  ui_check(picker.picker_choice==last,"held Down reaches the end of the Tour picker and stops there");
  down.navigate=1;
  tour_repeat.tick(down,0);
  ui_check(picker.picker_choice==0,"a new Down press still wraps to the first Tour sample");
  Controls page_down;page_down.tour_page=1;
  Controls page_up;page_up.tour_page=-1;
  tour_repeat.tick(page_down,0,true,34,24);
  ui_check(picker.picker_choice==12,"Page Down moves by the compact Tour list height without starting a sample");
  tour_repeat.tick(page_down,0,true,34,24);
  tour_repeat.tick(page_down,0,true,34,24);
  ui_check(picker.picker_choice==last,"paging forward clamps at the final stop");
  for(int i=0;i<4;++i)tour_repeat.tick(page_up,0,true,34,24);
  ui_check(picker.picker_choice==0,"paging backward clamps at the first stop");
  tour_repeat.tick(page_down,0,true,110,34);
  ui_check(picker.picker_choice==22,"the wide Tour picker pages by its own visible row count");
  {
    const int previous=picker.picker_choice;
    const auto channel=std::find_if(iceberg::DebugTour.begin(),iceberg::DebugTour.end(),
      [](const auto &stop){return stop.room==52;});
    picker.picker_choice=int(channel-iceberg::DebugTour.begin());
    picker.language=iceberg::Language::English;
    RenderGrid(tour_repeat.grid,tour_repeat.state,24,34,2).draw();
    const auto narrow=ui_text(frame);
    std::string interior;
    for(const auto &row:frame) {
      for(size_t x=1;x+1<row.size();++x)interior+=row[x].glyph;
      interior+=' ';
    }
    const auto preview=normalize_ui(interior);
    ui_check(narrow.find("The channel beneath the green…")!=std::string::npos&&
             preview.find("The channel beneath the greenhouse")!=std::string::npos,
      "the compact Tour marks the abbreviated list title and wraps the full title in its preview");
    ui_check(preview.find(channel->change.en)!=std::string::npos,
      "the full Tour title leaves the sample instructions readable");
    RenderGrid(tour_repeat.grid,tour_repeat.state,28,38,2).draw();
    ui_check(ui_text(frame).find("The channel beneath the greenhouse")!=std::string::npos&&
             ui_text(frame).find(iceberg::Districts[size_t(channel->district)].name.get(picker.language))!=std::string::npos,
      "a wider Tour keeps its full list title and district");
    picker.picker_choice=previous;
  }
  Controls back;back.pause=true;
  tour_repeat.tick(back,0);
  down.navigate=0;
  const int slot=picker.picker_choice;
  tour_repeat.tick(page_up,0);
  for(int i=0;i<8;++i)tour_repeat.tick(down,.1);
  ui_check(picker.save_picker&&picker.picker_choice==slot,"leaving the Tour picker cancels its held direction");

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
        const auto &run=game.profile.expeditions[mode==iceberg::Mode::Endless?0:1];
        ui_check(route.detail_rhythm==iceworld::pattern(run.itinerary[size_t(route.current+1)]).rhythm&&
          normalized.find(current+" "+next+" · ")!=std::string::npos,
          "the expedition plan shows the next authored traversal type beside the full title");
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
    const int leg_start=mode==iceberg::Mode::Endless?iceberg::RoomsPerLeg:0;
    for(int i=v.route.current;i<7;++i)game.tick(next_room,0);
    ui_check(v.route_room_selection==leg_start+7&&v.route.current==room+1,
      "browsing reaches the last planned room without moving the courier");
    RenderGrid(game.grid,game.state,24,34,2).draw();
    ui_check(ui_text(frame).find("Room 8:")!=std::string::npos&&ui_text(frame).find("╰╯")!=std::string::npos,
      "compact expedition timeline names and marks the browsed room");
    const auto ending=[&](bool en) {
      return mode==iceberg::Mode::Endless
        ?(en?"Exit starts leg 3":"Wyjście do odcinka 3")
        :(en?"Exit completes the trial":"Wyjście kończy próbę");
    };
    for(auto language:{iceberg::Language::Polish,iceberg::Language::English}) {
      game.profile.language=language;game.sync_view();
      for(auto size:{std::pair{34,24},std::pair{60,20},std::pair{110,34}}) {
        RenderGrid(game.grid,game.state,size.second,size.first,2).draw();
        ui_check(normalize_ui(ui_text(frame)).find(ending(language==iceberg::Language::English))!=std::string::npos,
          "browsing to room eight previews the exit before the courier arrives");
      }
    }
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
    for(int i=0;i<leg_start+8;++i)game.tick(previous_room,0);
    ui_check(v.route_room_selection==0&&v.route.current==room+1,
      "browsing stops at the first room without changing expedition progress");
    game.tick(map,0);game.tick(map,0);
    ui_check(v.route_room_selection==-1&&v.route.current==room+1,
      "reopening the expedition timeline returns the selection to the current room");

    Controls held_next;held_next.navigate=held_next.aim_y=1;
    game.tick(held_next,0);
    const int first_selection=v.route_room_selection;
    held_next.navigate=0;
    for(int i=0;i<3;++i)game.tick(held_next,.1);
    ui_check(first_selection==leg_start+room+2&&v.route_room_selection==first_selection,
      "a held down arrow moves once before the repeat delay");
    game.tick(held_next,.1);
    ui_check(v.route_room_selection==first_selection+1,
      "a held down arrow repeats in the expedition timeline");
    const int last_selection=v.route_room_selection;
    game.tick({},0);
    for(int i=0;i<5;++i)game.tick(held_next,.1);
    ui_check(v.route_room_selection==last_selection,
      "a released arrow needs a fresh navigation press before it repeats");
    held_next.navigate=1;
    game.tick(held_next,0);
    const int selection_before_tab=v.route_room_selection;
    held_next.navigate=0;
    game.tick(tab,0);game.tick(tab,0);
    for(int i=0;i<5;++i)game.tick(held_next,.1);
    ui_check(v.route_room_selection==selection_before_tab,
      "changing map tabs stops holding the old arrow");

    const auto final_name=v.route.rooms.back();
    game.tick(map,0);
    for(int i=game.room;i<iceberg::RoomsPerLeg-1;++i)game.advance_room();
    if(mode==iceberg::Mode::Trial) {
      ui_check(game.grid.template_id==320&&
        game.grid.room_name.get(iceberg::Language::Polish)==final_name.get(iceberg::Language::Polish)&&
        game.grid.room_name.get(iceberg::Language::English)==final_name.get(iceberg::Language::English),
        "the planned final trial room title matches the course on arrival");
    }
    game.tick(map,0);
    for(auto language:{iceberg::Language::Polish,iceberg::Language::English}) {
      game.profile.language=language;game.sync_view();
      const auto destination=ending(language==iceberg::Language::English);
      for(auto size:{std::pair{34,24},std::pair{60,20},std::pair{110,34}}) {
        RenderGrid(game.grid,game.state,size.second,size.first,2).draw();
        ui_check(normalize_ui(ui_text(frame)).find(destination)!=std::string::npos,
          "the final room on the timeline shows where its exit leads");
      }
    }
    std::vector<iceberg::Text> next_leg_preview;
    if(mode==iceberg::Mode::Endless) {
      game.tick(next_room,0);
      next_leg_preview=v.route.browsed_rooms;
      RenderGrid(game.grid,game.state,24,34,2).draw();
      ui_check(v.route_room_selection==leg_start+8&&v.route.current==7&&next_leg_preview.size()==8&&
        ui_text(frame).find("Next leg 3")!=std::string::npos&&ui_text(frame).find("Room 1:")!=std::string::npos,
        "browsing past the last room previews the first room of the next leg");
      game.profile.language=iceberg::Language::Polish;game.sync_view();
      RenderGrid(game.grid,game.state,24,34,2).draw();
      ui_check(ui_text(frame).find("Następny odcinek 3")!=std::string::npos,
        "the next route page has a localized leg heading");
      game.tick(previous_room,0);
      ui_check(v.route_room_selection==leg_start+7,"browsing back returns to room eight of the current leg");
      game.profile.language=iceberg::Language::English;game.sync_view();
    }
    const int resume_option=mode==iceberg::Mode::Endless?2:3;
    const auto saved_seed=game.run_seed;
    const int saved_depth=game.depth,saved_district=game.district;
    Controls open_menu;open_menu.menu_request=true;
    game.tick(open_menu,0);
    ui_check(game.state.menu&&v.selected==resume_option&&v.run_district==saved_district,
      "opening the menu over the map selects the current run and its district");
    game.tick(enter,0);
    ui_check(!game.state.menu&&!v.map&&game.mode==mode&&game.run_seed==saved_seed&&game.depth==saved_depth,
      "confirming the preselected run resumes the same route and room");
    if(mode==iceberg::Mode::Endless) {
      const auto completed=v.route.rooms;
      game.advance_room();game.tick(map,0);
      RenderGrid(game.grid,game.state,24,34,2).draw();
      ui_check(v.route.current==0&&v.route.visited==1&&
        ui_text(frame).find("Leg 3 · Depth 16")!=std::string::npos,
        "the last exit leads to the promised leg with fresh visit markers");
      ui_check(v.route.rooms.size()==next_leg_preview.size()&&
        std::equal(v.route.rooms.begin(),v.route.rooms.end(),next_leg_preview.begin(),[](const auto &a,const auto &b){
          return std::string_view(a.en)==b.en&&std::string_view(a.pl)==b.pl;
        }),"the generated leg matches every previewed room in order");
      game.tick(previous_room,0);
      RenderGrid(game.grid,game.state,24,34,2).draw();
      ui_check(v.route.browsed_leg==-1&&v.route.current==0&&ui_text(frame).find("Completed leg 2")!=std::string::npos&&
        ui_text(frame).find("Room 8:")!=std::string::npos,
        "browsing back from a new leg opens the completed leg");
      ui_check(v.route.detail_rhythm==
          iceworld::pattern(game.profile.expeditions[0].completed_legs.back().back()).rhythm,
        "a selected historical room shows the type from the stored itinerary");
      ui_check(v.route.browsed_rooms.size()==completed.size()&&
        std::equal(v.route.browsed_rooms.begin(),v.route.browsed_rooms.end(),completed.begin(),[](const auto &a,const auto &b){
          return std::string_view(a.en)==b.en&&std::string_view(a.pl)==b.pl;
        }),"history lists the actual completed itinerary in order");
      game.tick(next_room,0);
      ui_check(v.route.browsed_leg==0&&v.route.current==0,"browsing forward returns to the current leg");
      game.tick(previous_room,0);
      for(int leg_room=0;leg_room<2*iceberg::RoomsPerLeg;++leg_room)game.tick(previous_room,0);
      ui_check(v.route_room_selection==0&&v.route.browsed_leg==-2,"history stops at the first recorded room");
      game.tick(next_room,0);
      Controls next_leg;next_leg.browse_leg=1;
      Controls previous_leg;previous_leg.browse_leg=-1;
      game.tick(previous_leg,0);
      ui_check(v.route_room_selection==1,"previous leg leaves the first recorded leg selected");
      game.tick(next_leg,0);
      ui_check(v.route_room_selection==9&&v.route.browsed_leg==-1,"a leg step keeps the selected room number in history");
      game.tick(next_leg,0);
      ui_check(v.route_room_selection==17&&v.route.browsed_leg==0,"a leg step returns to the active route");
      game.tick(next_leg,0);game.tick(next_leg,0);
      ui_check(v.route_room_selection==25&&v.route.browsed_leg==1&&v.route.current==0&&game.depth==16,
        "future preview clamps leg navigation without moving the courier");
      ui_check(v.route.detail_rhythm==
          iceworld::pattern(game.expedition_itinerary(game.next_expedition_seed())[1]).rhythm,
        "a selected future room shows its type from the next seeded itinerary");
      const auto route_details=[&]{
        std::string details;
        for(const auto &row:frame) {
          for(const auto &cell:row)if(cell.rgb==0xffd68a||cell.rgb==0xc7d9df)details+=cell.glyph;
          details+=' ';
        }
        return normalize_ui(details);
      };
      for(auto language:{iceberg::Language::Polish,iceberg::Language::English}) {
        const bool en=language==iceberg::Language::English;
        game.profile.language=language;game.sync_view();
        const auto location=std::string(en?"Now: leg 3, room 1":"Teraz: odc. 3, pok. 1");
        const auto selected=std::string(en?"Room 2: ":"Pokój 2: ");
        for(auto dimensions:{std::pair{34,24},std::pair{56,20},std::pair{110,34}}) {
          RenderGrid(game.grid,game.state,dimensions.second,dimensions.first,2).draw();
          const auto text=route_details();
          ui_check(text.find(location+" "+v.route.place.get(language)+" "+selected+
                           v.route.browsed_rooms[1].get(language))!=std::string::npos,
                   "future preview distinguishes the physical and selected locations at each viewport size");
        }
        game.tick(previous_leg,0);game.tick(previous_leg,0);
        RenderGrid(game.grid,game.state,24,34,2).draw();
        ui_check(ui_text(frame).find(en?"Completed leg 2":"Ukończony odcinek 2")!=std::string::npos&&
                 route_details().find(location+" "+v.route.place.get(language)+" "+selected+
                           v.route.browsed_rooms[1].get(language))!=std::string::npos,
                 "history displays the courier's active leg and room separately from the historical selection");
        game.tick(next_leg,0);game.tick(next_leg,0);
      }
      game.tick(previous_leg,0);game.tick(previous_room,0);
      ui_check(v.route_room_selection==16&&v.route.current==0,"room arrows still move one room after a leg step");
      const auto courier=game.state.player.rect();
      const double elapsed=game.state.elapsed;
      const auto saved_rooms=v.route.rooms;
      game.tick(previous_leg,0);game.tick(previous_leg,0);
      ui_check(iceberg::browsing_expedition_room(v)&&v.route.browsed_leg==-2,"the return hint is active in the oldest recorded leg");
      RenderGrid(game.grid,game.state,24,34,2).draw();
      ui_check(ui_text(frame).find("ENTER current room")!=std::string::npos,"the timeline shows the current-room shortcut");
      game.tick(enter,.1);
      ui_check(v.map&&!v.map_room&&v.route_room_selection==-1&&v.route.browsed_leg==0&&
        game.state.player.x==courier.x&&game.state.player.y==courier.y&&game.state.elapsed==elapsed&&
        game.depth==16&&v.route.rooms.size()==saved_rooms.size(),
        "Enter returns to the active room from history while the map keeps the courier paused");
      game.tick(next_leg,0);
      ui_check(v.route.browsed_leg==1,"the next-leg preview still opens after returning to the active room");
      game.tick(enter,.1);
      ui_check(v.map&&!iceberg::browsing_expedition_room(v)&&v.route.browsed_leg==0&&game.state.elapsed==elapsed,
        "Enter also returns from the future preview without taking an exit");
      game.tick(open_menu,0);game.tick(enter,0);game.tick(map,0);
      game.tick(previous_room,0);
      ui_check(v.route.browsed_leg==-1,"resuming the same Expedition from the menu retains its session history");
      game.tick(map,0);
    }
  }
  {
    AdventureSession route_book(options,{});
    route_book.begin_campaign(true);
    route_book.profile.language=iceberg::Language::English;
    route_book.profile.unlocked=4;route_book.run_district=4;
    route_book.begin_run(iceberg::Mode::Endless,true);
    route_book.room=route_book.depth=6;
    route_book.load_room();
    const auto &view=route_book.state.adventure;
    const auto destination=view.route.rooms.back().get(view.language);
    const auto goal=[&] {
      const auto &door=route_book.grid.goal;
      route_book.state.player.x=door.x;
      route_book.state.player.y=door.bottom()-kPlayerHeight;
      route_book.tick({},1./120);
      route_book.tick(use,1./120);
    };
    ui_check(iceberg::journey_next_step(view)=="The lit door leads to room 8: "+destination+".",
      "the penultimate Expedition room names the last room of the same leg");
    goal();
    ui_check(route_book.room==7&&view.room_name==destination&&
      iceberg::journey_next_step(view)=="The lit door starts leg 2.",
      "the named door reaches that room and the final door announces the next leg");
    goal();
    ui_check(route_book.room==0&&route_book.depth==8&&
      iceberg::journey_next_step(view).starts_with("The lit door leads to room 2: "),
      "the final Expedition door starts the next leg and its journal resets the room number");
    auto trial=view;
    trial.mode=iceberg::Mode::Trial;trial.route.current=7;
    ui_check(iceberg::journey_next_step(trial)=="The lit door completes the Mastery Trial.",
      "the last Mastery Trial room describes completion");
    route_book.start_tour(17);
    route_book.tick({},.0);
    route_book.state.adventure.notice_left=0.;
    ui_check(iceberg::journey_next_step(view)==iceberg::DebugTour[17].change.get(view.language),
      "the Debug Tour journal keeps a stop's instructions after the entry notice expires");
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
