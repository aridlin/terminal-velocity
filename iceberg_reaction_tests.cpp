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
 game.district=4;game.stage=1;game.grid=iceworld::build_garden_sequence(false);game.state=new_state(game.grid);game.state.menu=false;
 game.state.player.x=12;game.state.player.y=55-kPlayerHeight;game.sync_view();
 require_reaction(!game.grid.solid_at(31,52),"dry root crossing has no invisible collision");
 game.tick(use,1./120);
 require_reaction(game.grid.switch_flags==1&&game.grid.growth[0].amount==0&&game.state.adventure.machinery_kind==5,"irrigation begins one visible root growth reaction");
 for(int i=0;i<5;i++)game.tick({},.1);
 require_reaction(game.grid.solid_at(31,52)&&!game.grid.solid_at(44,52),"only the unfolded root bridge supports the player");
 auto amount=game.grid.growth[0].amount;game.state.paused=true;game.tick({},.1);
 require_reaction(amount==game.grid.growth[0].amount,"pause freezes root growth and its collision edge together");game.state.paused=false;
 game.tick(use,.1);require_reaction(game.grid.growth[0].amount>amount,"repeated irrigation does not reset the growing bridge");
 for(int i=0;i<8;i++)game.tick({},.1);
 require_reaction(game.grid.solid_at(44,52)&&game.grid.growth[0].amount==1,"fully unfolded bridge has complete landing space");
 game.persist();game.load_room(true);
 require_reaction(game.grid.template_id==80&&game.grid.solid_at(44,52)&&game.grid.switch_flags==1,"saved irrigation restores a fully usable bridge");
 game.state.player.x=7;game.state.player.y=55-kPlayerHeight;auto discoveries=game.profile.discoveries;game.tick(use,1./120);
 require_reaction(game.state.adventure.dialogue.active()&&game.profile.discoveries==discoveries,"garden instructions do not reveal or change hidden discoveries");game.state.adventure.dialogue={};
 game.profile.story|=1;game.stage=0;game.district=0;game.load_room();game.state.menu=false;
 game.state.camera_x=44.9;game.state.camera_y=30.;
 game.state.adventure.door_left=0;
 const int storefront_column=35, storefront_top=15;
 for(auto language:{iceberg::Language::Polish,iceberg::Language::English}) {
   game.state.adventure.language=language;
   RenderGrid(game.grid,game.state,34,110,2).draw();
   const auto wall=[&](int row){return frame[storefront_top+row][storefront_column+20].glyph;};
   require_reaction(wall(0)=="╮"&&wall(1)=="│"&&wall(2)=="┤"&&wall(3)=="│"&&wall(4)=="╯",
     "the cafe sign and its windows share a border in both languages");
 }
 game.state.adventure.door_left=.42;
 auto x=game.state.player.x;Controls move;move.move=1;for(int i=0;i<4;i++)game.tick(move,.1);
 require_reaction(game.state.player.x>x&&game.state.adventure.door_left<.03,"door reveal is brief and does not swallow movement");
 game.state.player.x=game.grid.goal.x-.3;
 game.state.player.y=game.grid.goal.bottom()-kPlayerHeight;
 game.state.player.vx=0;
 auto elapsed=game.state.elapsed;
 for(int i=0;i<4;i++)game.tick({},.1);
 require_reaction(game.state.won&&game.state.adventure.door_open==1&&game.state.elapsed>elapsed+.35,
   "an exit opens while room physics keeps running");
 x=game.state.player.x;
 Controls back;back.move=-1;
 for(int i=0;i<6;i++)game.tick(back,.1);
 require_reaction(game.stage==0&&!game.state.won&&game.state.player.x<x-3&&game.state.adventure.door_open==0,
   "walking back clears and closes the exit without advancing");
 for(int i=0;i<20&&game.stage==0;i++)game.tick(move,.1);
 require_reaction(game.stage==1,"holding forward still walks through the opening exit");
 game.stage=2;game.room=2;game.load_room();
 game.state.player.x=game.grid.goal.x-1.4;
 game.state.player.y=game.grid.goal.bottom()-kPlayerHeight;
 const auto deaths=game.state.deaths;
 Controls door_dash;door_dash.move=1;door_dash.dash_pressed=true;
 game.tick(door_dash,.1);
 for(int i=0;i<10;i++)game.tick(move,.1);
 const int wall=int(std::ceil(game.grid.goal.right()))+1;
 require_reaction(game.grid.solid_at(wall,int(game.grid.goal.bottom())-2)&&
   game.state.player.rect().right()<=wall+.001&&game.state.won&&game.state.deaths==deaths,
   "dashing through a later doorway stops at its wall and stays in exit range");
 game.tick(use,.1);
 require_reaction(game.stage==2&&game.room==3,"interacting still advances a later exit");
 game.grid=iceworld::build_harbor_lesson_v4();
 game.state=new_state(game.grid);
 game.state.menu=false;
 game.state.camera_x=86;game.state.camera_y=40;
 RenderGrid(game.grid,game.state,34,110,2).draw();
 require_reaction(frame[16][62].rgb==0x8ed9e5&&frame[16][62].background==0x214b62&&
   frame[18][62].background==0x14394e&&!game.grid.hazard_at(98,50),
   "ferry water has a visible surface and depth beneath the deck");
 const auto deck=game.grid.platforms[0].at(game.state.elapsed);
 game.state.player.x=deck.x+deck.w*.7;game.state.player.y=deck.y-kPlayerHeight;
 for(double camera_y:{39.73,40.,40.39}) {
   game.state.camera_y=camera_y;
   const int foot_row=4+int(std::floor((deck.y-.001-camera_y)/.8));
   const int rider_col=int(std::floor((game.state.player.x-game.state.camera_x)/.4));
   const int midpoint=int(std::floor((deck.x+deck.w*.5-game.state.camera_x)/.4));
   const int surface=4+int(std::ceil((game.grid.water_basins[0].y-camera_y)/.8-.5));
   game.state.player.support=-1;
   RenderGrid(game.grid,game.state,34,110,2).draw();auto unsupported=frame;
   game.state.player.support=0;
   RenderGrid(game.grid,game.state,34,110,2).draw();
   require_reaction(frame[foot_row][rider_col+1].rgb==0xb18b61&&
     frame[foot_row][rider_col+1].background==0x6d513f&&
     unsupported[foot_row][rider_col+1].rgb!=0xb18b61&&
     frame[foot_row-1][rider_col+1].glyph==unsupported[foot_row-1][rider_col+1].glyph&&
     frame[foot_row-1][rider_col+1].rgb==unsupported[foot_row-1][rider_col+1].rgb,
     "the foreground gunwale covers the rider's lower sprite and leaves the torso visible");
   require_reaction(frame[surface][midpoint].rgb==0x477080&&
     frame[surface][midpoint].background==0x214b62&&
     frame[surface+1][midpoint].glyph=="━"&&frame[surface+1][midpoint].background==0x14394e,
     "the hull intersects the waterline and extends below it");
 }
 game.state.player.support=-1;
 game.state.camera_y=40.;
 for(double time:{1.6,5.0}) {
   game.state.elapsed=time;
   RenderGrid(game.grid,game.state,34,110,2).draw();
   const auto sailing=game.grid.platforms[0].at(time);
   const double center=sailing.x+sailing.w*.5;
   int foam=0;
   for(size_t y=4;y+3<frame.size();++y)for(size_t x=0;x<frame[y].size();++x) {
     const auto &cell=frame[y][x];
     if(cell.rgb!=0xb6eef1&&cell.rgb!=0x75bbc8)continue;
     ++foam;
     const double wx=game.state.camera_x+x*.4;
     const bool trailing=time<3.4?wx<center:wx>center;
     if(!trailing||cell.background!=0x214b62)
       throw std::runtime_error("ferry wake must trail the boat on the water surface");
   }
   require_reaction(foam>=2,"ferry leaves visible foam behind it in either direction");
 }
 const auto heatworks=iceworld::build_room(15,73,1);
 const auto &vent=heatworks.pulses.front();
 auto guard=new_state(heatworks);
 guard.camera_x=27.;guard.camera_y=32.;
 for(double phase:{.40,.84,.98,.10}) {
   guard.elapsed=(1.+phase-vent.phase)*vent.period;
   RenderGrid(heatworks,guard,34,110,2).draw();
   int arrows=0,plume=0;
   for(size_t y=4;y<frame.size()-3;++y)for(const auto &cell:frame[y]) {
     arrows+=cell.glyph=="↑"&&cell.rgb==0xf1c66f;
     plume+=cell.rgb==0xffb7a4;
   }
   const int expected_arrows=phase==.84?1:phase==.98?3:0;
   require_reaction(arrows==expected_arrows&&(plume>0)==vent.active(guard.elapsed),
     "vent pressure rises visibly before the damaging steam appears");
 }
 return 0;
}catch(const std::exception &e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
