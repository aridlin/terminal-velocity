#define TERMINAL_VELOCITY_NO_TUI_MAIN
#include "main.cpp"
#include <stdexcept>
void require_reaction(bool pass,const char *text){if(!pass)throw std::runtime_error(text);std::cout<<"PASS "<<text<<'\n';}
int main(){try{
 iceberg::SessionOptions options;options.no_save=options.mute=true;
 AdventureSession game(options,"/tmp/tv-reaction-test-unused");game.begin_campaign(true);
 game.grid=iceworld::build_estate_sequence(true);game.grid.switch_flags=3;game.state=new_state(game.grid);game.state.menu=false;game.state.player.x=130;game.state.player.y=20;game.sync_view();
 Controls use;use.interact=true;game.tick(use,1./120);
 require_reaction(game.grid.switch_flags==7&&game.state.adventure.machinery_kind==2&&game.state.adventure.machinery_left>2.7,"last tram junction starts one departure reaction");
 auto left=game.state.adventure.machinery_left;game.tick(use,.1);
 require_reaction(game.state.adventure.machinery_left<left,"repeated use does not restart the machinery");
 CellFrame frame;cell_frame_sink=[&](const CellFrame &f){frame=f;};
 RenderGrid(game.grid,game.state,34,110,2).draw();auto before=frame;
 for(int i=0;i<8;i++)game.tick({},.1);
 RenderGrid(game.grid,game.state,34,110,2).draw();int changed=0;
 for(size_t y=4;y<frame.size()-3;y++)for(size_t x=0;x<frame[y].size();x++)changed+=frame[y][x].glyph!=before[y][x].glyph;
 require_reaction(changed>25,"tram, particles and gate visibly change in the shared cell renderer");
 game.state.paused=true;left=game.state.adventure.machinery_left;game.tick({},.1);
 require_reaction(left==game.state.adventure.machinery_left,"pause freezes the physical reaction");game.state.paused=false;
 for(int i=0;i<32;i++)game.tick({},.1);
 require_reaction(game.state.adventure.machinery_left==0,"departure particles finish without lingering effects");
 game.grid=iceworld::build_harbor_mission(0);game.grid.switch_flags=1;game.state=new_state(game.grid);game.state.menu=false;
 game.state.player.x=144;game.state.player.y=44-kPlayerHeight;game.sync_view();game.tick(use,1./120);
 require_reaction(game.grid.switch_flags==3&&game.state.adventure.machinery_kind==4&&game.state.adventure.machinery_left>2.7,"rescue tram departs from its parked position instead of disappearing");
 left=game.state.adventure.machinery_left;game.tick({},.1);
 require_reaction(game.state.adventure.dialogue.active()&&game.state.adventure.machinery_left<left,"tram departure continues naturally behind the rescue conversation");
 game.district=3;game.stage=7;game.grid=iceworld::build_archive_sequence(true);game.grid.switch_flags=1;game.state=new_state(game.grid);game.state.menu=false;
 game.state.player.x=132;game.state.player.y=38-kPlayerHeight;game.sync_view();game.tick(use,1./120);
 require_reaction(game.grid.switch_flags==3&&game.state.adventure.machinery_kind==3,"archive cabinet opens with a paper and machinery reaction");
 game.state.player.x=177;game.state.player.y=32-kPlayerHeight;game.tick(use,1./120);
 require_reaction((game.grid.switch_flags&4)&&game.state.adventure.pickup_left>0&&game.state.adventure.dialogue.active(),"original plan has a collection animation and its authored discovery");
 left=game.state.adventure.pickup_left;game.tick({},.1);
 require_reaction(game.state.adventure.pickup_left<left,"plan collection continues while the discovery dialogue is read");
 game.persist();game.state.adventure.dialogue={};game.load_room(true);
 require_reaction(game.grid.template_id==79&&game.grid.switch_flags==7&&(game.profile.story&(uint64_t(1)<<41)),"archive resume retains the open cabinet and collected original plan");
 game.state.player.x=177;game.state.player.y=32-kPlayerHeight;
 require_reaction(game.near_interaction()<0,"collected evidence cannot be picked up twice after resume");
 game.profile.story|=1;game.stage=0;game.district=0;game.load_room();game.state.menu=false;
 auto x=game.state.player.x;Controls move;move.move=1;for(int i=0;i<4;i++)game.tick(move,.1);
 require_reaction(game.state.player.x>x&&game.state.adventure.door_left<.03,"door reveal is brief and does not swallow movement");
 game.state.won=true;game.state.adventure.door_open=0;for(int i=0;i<4;i++)game.tick({},.1);
 require_reaction(game.state.adventure.door_open==1,"exit doors slide open while the player can choose to return");
 return 0;
}catch(const std::exception &e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
