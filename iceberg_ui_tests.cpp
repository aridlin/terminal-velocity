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
