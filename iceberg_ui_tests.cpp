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
    game.tick(tab,1./120);ui_check(game.state.adventure.map&&game.state.adventure.map_route,"B switches to room route without closing map");game.tick(tab,1./120);
  }
  game.profile.unlocked=1;game.profile.story|=uint64_t(1)<<8;game.sync_view();game.state.adventure.selected=1;
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
    ui_check(game.state.adventure.map&&game.state.adventure.map_route,"J opens the active expedition route");
    Controls up;up.navigate=-1;
    game.tick(tab,1./120);game.tick(up,1./120);game.tick(enter,1./120);
    const auto &v=game.state.adventure;
    ui_check(v.map&&v.map_route&&v.route_page==-1&&game.state.player.x==x,"expedition map stays on the current route during district inputs");
    const int room=mode==iceberg::Mode::Endless?1:2;
    ui_check(v.route.available&&v.route.current==room&&v.route.visited==(1u<<(room+1))-1,"expedition route marks only the rooms reached in this leg");
    for(auto language:{iceberg::Language::Polish,iceberg::Language::English}) {
      game.profile.language=language;game.sync_view();
      for(auto size:{std::pair{34,24},std::pair{60,20},std::pair{110,34}}) {
        RenderGrid(game.grid,game.state,size.second,size.first,2).draw();
        const bool en=language==iceberg::Language::English;
        const auto text=ui_text(frame);
        const char *title=mode==iceberg::Mode::Endless?(en?"ENDLESS EXPEDITION":"WYPRAWA BEZ KOŃCA"):(en?"MASTERY TRIAL":"PRÓBA MISTRZOWSKA");
        const char *progress=mode==iceberg::Mode::Endless?(en?"Leg 2 · Depth 9":"Odcinek 2 · Głębokość 9"):(en?"Room 3 / 8":"Pokój 3 / 8");
        ui_check(text.find(title)!=std::string::npos&&text.find(progress)!=std::string::npos&&
          text.find(en?"Now: ":"Teraz: ")!=std::string::npos&&text.find(en?"Planned: ":"Plan: ")!=std::string::npos,
          "expedition map shows mode, progress and room labels at all supported sizes");
      }
    }
    game.tick(map,1./120);ui_check(!v.map,"J closes the expedition map");
  }
  game.begin_campaign(false);game.tick(map,1./120);
  ui_check(game.state.adventure.map&&!game.state.adventure.map_route,"returning to the campaign opens district travel");
  cell_frame_sink={};return 0;
}catch(const std::exception &error){std::cerr<<"FAIL "<<error.what()<<'\n';return 1;}}
