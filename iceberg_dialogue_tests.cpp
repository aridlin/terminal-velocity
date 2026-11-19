#define TERMINAL_VELOCITY_NO_TUI_MAIN
#include "main.cpp"
#include <stdexcept>
void dialogue_check(bool pass,const char *message){if(!pass)throw std::runtime_error(message);std::cout<<"PASS "<<message<<'\n';}
int main(){try {
 iceberg::SessionOptions options;options.no_save=options.mute=true;
 AdventureSession game(options,"/tmp/tv-dialogue-test-unused");game.begin_campaign(true);
 const iceberg::Text line{"Zażółć gęślą jaźń. Pod szkłem leży plan starej sieci. Irena czeka przy oknie, a przez rury przechodzi ciepło.","The old network plan lies under glass. Irena waits by the window while heat travels through the pipes."};
 CellFrame frame;cell_frame_sink=[&](const CellFrame &f){frame=f;};
 for(auto language:{iceberg::Language::Polish,iceberg::Language::English}) {
  game.profile.language=language;game.sync_view();
  for(auto dimensions:{std::pair{76,24},std::pair{38,28},std::pair{60,20}}) {
   game.tick({},0,true,dimensions.first,dimensions.second);
   game.talk({{4,line},{0,{"Dziękuję.","Thank you."}}},100);
   auto &dialogue=game.state.adventure.dialogue;
   dialogue_check(dialogue.visible_characters==0&&dialogue.revealing,"new dialogue begins unrevealed");
   for(int i=0;i<4;++i)game.tick({},.1);
   dialogue_check(dialogue.visible_characters>0&&dialogue.visible_characters<40&&dialogue.page==0,"normal time reveals a partial Unicode line without advancing the page");
   RenderGrid(game.grid,game.state,dimensions.second,dimensions.first,2).draw();auto partial=frame;
   Controls confirm;confirm.confirm=true;game.tick(confirm,1./120);
   dialogue_check(dialogue.page==0&&!dialogue.revealing,"first confirmation instantly reveals the current page");
   RenderGrid(game.grid,game.state,dimensions.second,dimensions.first,2).draw();
   int courier=4+int(std::floor((game.state.player.y-game.state.camera_y)/.8));
   auto layout=iceberg::dialogue_layout(line.get(language),dimensions.first,dimensions.second,courier,dialogue.segment);
   int top=layout.text_y,bottom=top+std::min(layout.text_rows,int(layout.body.size())),left=layout.text_x,right=dimensions.first-3;
   int letters=0;
   for(int y=top;y<bottom;++y)for(int x=left;x<right;++x)if(partial[size_t(y)][size_t(x)].glyph!=" ") {
    ++letters;dialogue_check(partial[size_t(y)][size_t(x)].glyph==frame[size_t(y)][size_t(x)].glyph,"revealed letters keep their final wrapped positions");
   }
   dialogue_check(letters>0,"partially revealed text is present in the actual cell renderer");
   game.tick(confirm,1./120);dialogue_check(dialogue.page==1&&!dialogue.revealing&&dialogue.instant,"one reveal choice makes subsequent lines immediate without skipping them");
   dialogue.instant=false;dialogue.reset_reveal();
   game.tick({},.1);auto visible=dialogue.visible_characters;
   Controls pause;pause.pause=true;game.tick(pause,1./120);game.tick({},.1);
   dialogue_check(dialogue.visible_characters==visible,"pause freezes text and speech cadence");game.tick(pause,1./120);
   visible=dialogue.visible_characters;game.tick({},.1,false);
   dialogue_check(dialogue.visible_characters==visible&&game.state.paused,"focus loss freezes dialogue");game.tick(confirm,1./120,true);
   dialogue_check(dialogue.page==1,"focus resume does not accidentally skip a page");
   for(int i=0;i<100&&dialogue.revealing;++i)game.tick({},.1);
   dialogue_check(!dialogue.revealing&&dialogue.active(),"natural reveal finishes and waits for the player");
   game.tick(confirm,1./120);dialogue_check(!dialogue.active()&&game.profile.quests[0]==1,"conversation action commits only after its last acknowledged page");
  }
 }
 const std::string document=
   "Zażółć gęślą jaźń. Po pierwszej odprawie sprawdź mapę. "
   "Na drugim pomoście podnieś lampę. Zapisz stan sieci pod zegarem. "
   "Na trzecim pomoście odczytaj licznik. Klucz zostaw przy skrzynce. "
   "Po ostatniej odprawie zamknij szafę. Raport przekaż dyżurnemu.";
 game.profile.language=iceberg::Language::Polish;
 game.tick({},0,true,34,20);
 game.talk({{-1,{document.c_str(),document.c_str()}}});
 auto &dialogue=game.state.adventure.dialogue;
 Controls confirm;confirm.confirm=true;
 game.tick(confirm,0,true,34,20);
 game.tick(confirm,0,true,34,20);
 const auto current=iceberg::dialogue_segment(dialogue.lines[0],game.profile.language,34,20,1);
 dialogue_check(dialogue.segment==1&&!dialogue.revealing,"document reaches its second segment before resizing");
 game.tick({},0,true,34,20,1.5);
 dialogue_check(dialogue.segment==1&&!dialogue.revealing,"aspect changes preserve an open document segment");
 game.tick({},0,true,80,25,1.5);
 dialogue_check(dialogue.segment==0&&!dialogue.revealing,"a wider document shows the merged segment immediately");
 game.tick({},0,true,34,20,1.5);
 dialogue_check(dialogue.segment==1&&!dialogue.revealing,"returning to the original dimensions restores the original segment");
 dialogue_check(iceberg::dialogue_segment(dialogue.lines[0],game.profile.language,34,20,dialogue.segment)==current,"document reflow retains the same passage");
 game.talk({{-1,{document.c_str(),document.c_str()}}});
 for(int i=0;i<5;++i)game.tick({},.1,true,34,20,1.5);
 const auto visible=dialogue.visible_characters;
 dialogue_check(dialogue.revealing&&visible>0,"Unicode document is partially revealed");
 game.tick({},0,true,80,25,1.5);
 dialogue_check(dialogue.revealing&&dialogue.visible_characters==visible,"rewrapping retains the revealed Unicode characters");
 game.tick({},0,true,34,20,1.5);
 dialogue_check(dialogue.revealing&&dialogue.visible_characters==visible,"reversing the resize keeps the reveal position");
 game.tick({},0,true,18,18,1.5);
 dialogue_check(!dialogue.revealing&&dialogue.visible_characters<visible,"a smaller segment shows only the revealed characters that fit");
 game.tick({},0,true,34,20,1.5);
 dialogue_check(dialogue.revealing&&dialogue.visible_characters==visible,"widening restores previously revealed characters");
 for(auto language:{iceberg::Language::Polish,iceberg::Language::English}) {
  game.profile.language=language;game.sync_view();
  for(auto dimensions:{std::pair{38,28},std::pair{110,34}}) {
   game.talk({},0,0);
   Controls navigate;navigate.navigate=1;
   Controls pause;pause.pause=true;
   game.tick(navigate,0,true,dimensions.first,dimensions.second);
   game.tick(pause,0,true,dimensions.first,dimensions.second);
   game.tick(navigate,0,true,dimensions.first,dimensions.second);
   RenderGrid(game.grid,game.state,dimensions.second,dimensions.first,2).draw();
   std::string screen;
   for(const auto &row:frame){for(const auto &cell:row)screen+=cell.glyph;screen+='\n';}
   const auto resume=iceberg::tr(language,"E/ENTER wznów","E/ENTER resume");
   const auto pause_panel=iceberg::tr(language,"CHWILA ODDECHU","A MOMENT TO BREATHE");
   dialogue_check(screen.find(resume)!=std::string::npos&&screen.find(pause_panel)==std::string::npos,
                  "a paused reply shows resume instructions without a competing pause panel");
   game.tick(confirm,0,true,dimensions.first,dimensions.second);
   dialogue_check(!game.state.paused&&dialogue.choosing()&&dialogue.reply_option==1,
                  "confirming once resumes without changing or committing the selected reply");
  }
 }
 const auto position=std::pair{game.state.player.x,game.state.player.y};
 for(auto dimensions:{std::pair{38,28},std::pair{60,20},std::pair{110,34}}) {
  const auto [cols,rows]=dimensions;
  game.tick({},0,true,cols,rows,2);
  for(int courier_row:{rows/2-2,rows/2}) {
   game.state.camera_x=position.first-(cols/2)*.4;
   game.state.camera_y=position.second-(courier_row-4)*.8;
   game.talk({},0,0);
   RenderGrid(game.grid,game.state,rows,cols,2).draw();
   int head_pixels=0;
   for(const auto &row:frame)for(int x=cols/2-1;x<=cols/2+4;++x)
    if(row[size_t(x)].rgb==0xe4ad68&&(row[size_t(x)].background==0xe0b394||row[size_t(x)].background==0x51474b))
     ++head_pixels;
   dialogue_check(head_pixels==2,"the reply panel leaves the courier's full head visible");
   dialogue_check(std::pair{game.state.player.x,game.state.player.y}==position,"reply framing does not move the courier in the world");
  }
 }
 for(int district=0;district<iceberg::DistrictCount;++district) {
  for(int option=0;option<2;++option) {
   game.begin_campaign(true);
   game.district=district;
   game.stage=0;
   game.profile.story=0;
   game.load_room();
   game.state.menu=false;
   if(option==0) {
    const auto resident=std::find_if(game.grid.interactions.begin(),game.grid.interactions.end(),
       [](const Interaction &node){return node.kind==InteractionKind::Npc;});
    dialogue_check(resident!=game.grid.interactions.end(),"the district has a briefing resident");
    game.state.player.x=resident->area.x;
    game.state.player.y=resident->area.y;
    game.interact();
   } else game.advance_room();
   for(int i=0;i<30&&dialogue.active()&&!dialogue.choosing();++i)game.dialogue_next();
   dialogue_check(dialogue.choosing(),"both the resident and first exit can open a briefing reply");
   const auto &reply=iceberg::briefing_reply(district,option);
   if(option) {
    Controls navigate;navigate.navigate=1;
    game.tick(navigate,0);
   }
   game.dialogue_next();
   dialogue_check((game.profile.story&~iceberg::BriefingReplyMask)==0&&
                  iceberg::pending_briefing_option(game.profile.story,district)==option&&dialogue.active()&&dialogue.lines.size()==2,
                  "the pending briefing reply leaves progression and saved civic choices untouched");
   dialogue_check(dialogue.lines[0].speaker==iceberg::CourierSpeaker&&dialogue.lines[0].text.en==reply.said.en&&
                  dialogue.lines[1].speaker==reply.answer.speaker&&dialogue.lines[1].text.en==reply.answer.text.en,
                  "the selected answer is spoken by the courier and gets its own reply");
   for(int i=0;i<30&&dialogue.active();++i)game.dialogue_next();
   dialogue_check(!dialogue.active()&&game.profile.story==(uint64_t(1)<<district),
                  "the briefing completes after the final exchange");
  }
 }
 struct ExpeditionScene {int district,room,scene,speaker;double x,floor;};
 for(const auto &scene:std::array{
     ExpeditionScene{4,57,iceberg::IrrigationReplyScene,5,52,50},
     ExpeditionScene{1,13,iceberg::HoistCrewReplyScene,1,12,56},
     ExpeditionScene{2,24,iceberg::EstateTramReplyScene,2,12,54}}) {
  AdventureSession expedition(options,{});
  expedition.begin_campaign(true);
  expedition.profile.unlocked=expedition.run_district=scene.district;
  auto &run=expedition.profile.expeditions[0];
  for(uint32_t seed=1;seed<100;++seed) {
   auto itinerary=iceworld::make_itinerary(seed,scene.district,8,4,iceberg::Mode::Endless);
   const auto room=std::find(itinerary.begin(),itinerary.end(),scene.room);
   if(room==itinerary.end())continue;
   run.active=true;run.seed=run.origin_seed=seed;run.district=scene.district;
   run.room=run.depth=int(room-itinerary.begin());run.itinerary=std::move(itinerary);
   break;
  }
  dialogue_check(run.active,"a normal Expedition itinerary contains the conversation room");
  expedition.begin_run(iceberg::Mode::Endless);
  expedition.state.player.x=scene.x;
  expedition.state.player.y=scene.floor-kPlayerHeight;
  const auto story=expedition.profile.story;
  const auto quests=expedition.profile.quests;
  for(int option=0;option<2;++option) {
   expedition.interact();
   auto &exchange=expedition.state.adventure.dialogue;
   for(int i=0;i<30&&exchange.active()&&!exchange.choosing();++i)expedition.dialogue_next();
   dialogue_check(exchange.choosing()&&exchange.reply_scene==scene.scene&&
                  expedition.profile.story==story,"the Expedition reply opens without a pending campaign choice");
   if(option) {
    Controls navigate;navigate.navigate=1;
    expedition.tick(navigate,0);
   }
   const auto &reply=iceberg::civic_reply(scene.scene,option);
   expedition.dialogue_next();
   dialogue_check(exchange.lines.size()==2&&exchange.lines[0].speaker==iceberg::CourierSpeaker&&
                  exchange.lines[0].text.en==reply.said.en&&exchange.lines[1].speaker==scene.speaker&&
                  exchange.lines[1].text.en==reply.answer.text.en,"each Courier choice receives its matching answer");
   for(int i=0;i<30&&exchange.active();++i)expedition.dialogue_next();
   dialogue_check(!exchange.active()&&expedition.profile.story==story&&expedition.profile.quests==quests,
                  "the Expedition exchange leaves campaign story and quests intact");
  }
 }
 cell_frame_sink={};return 0;
}catch(const std::exception &e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
