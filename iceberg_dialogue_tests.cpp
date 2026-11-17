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
 cell_frame_sink={};return 0;
}catch(const std::exception &e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
