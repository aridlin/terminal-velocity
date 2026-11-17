// Captures the real shared-cell SDL renderer from an explicit isolated state.
#define main terminal_velocity_unused_sdl_entry
#include "../sdl_frontend.cpp"
#undef main
int main(int argc,char **argv) {
 if(argc!=6){std::cerr<<"scene columns rows pl|en output.bmp\n";return 2;}
 std::string scene=argv[1];int cols=std::stoi(argv[2]),rows=std::stoi(argv[3]),cw=12,ch=24;
 SDL_SetHint(SDL_HINT_RENDER_DRIVER,"software");if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER))return 2;
 SDL_Window *window=SDL_CreateWindow("UI verification",0,0,cols*cw,rows*ch,SDL_WINDOW_HIDDEN);
 ren=SDL_CreateRenderer(window,-1,SDL_RENDERER_SOFTWARE);if(!ren)return 2;
 int result=0;
 {
  Font font;if(!font.init("assets/cell-font.ttf","assets/font.ttf"))return 2;
  iceberg::SessionOptions options;options.no_save=options.mute=true;options.seed=73;
  options.language_override=true;options.language=std::string(argv[4])=="pl"?iceberg::Language::Polish:iceberg::Language::English;
  AdventureSession game(options,std::filesystem::path{});game.begin_campaign(true);
  if(scene=="lesson")game.grid=iceworld::build_harbor_lesson_v4();
  if(scene=="room500"||scene=="sorting"||scene=="pipe")game.grid=iceworld::build_room(scene=="sorting"?503:scene=="pipe"?505:500,73,0);
 if(scene=="ferry")game.grid=iceworld::build_harbor_lesson_v4();
 if(scene=="boiler")game.grid=game.campaign_authored(1,false);
  if(scene=="mission310")game.grid=iceworld::build_harbor_mission_v4(0);
  if(scene=="archive"||scene=="archive:dialogue")game.grid=iceworld::build_archive_settlement();
  if(scene=="archive:finale")game.grid=iceworld::build_archive_sequence(true);
  if(scene=="route") {game.stage=2;game.room=2;game.profile.route_visited[0]=7;game.load_room();}
  else {game.district=game.grid.district;game.state=new_state(game.grid);game.state.menu=false;}
  if(scene=="dialogue"||scene=="archive:dialogue") {
   auto npc=std::find_if(game.grid.interactions.begin(),game.grid.interactions.end(),[](const auto &n){return n.kind==InteractionKind::Npc;});
   if(npc!=game.grid.interactions.end()){game.state.player.x=npc->area.x;game.state.player.y=npc->area.bottom()-kPlayerHeight;}
  }
  if(scene=="dock-worker"){game.state.player.x=86;game.state.player.y=48-kPlayerHeight;}
  if(scene=="storefront"){game.state.player.x=63;game.state.player.y=43-kPlayerHeight;}
  if(scene=="archive:finale"){game.state.player.x=174;game.state.player.y=32-kPlayerHeight;game.grid.switch_flags=3;}
  if(scene=="hoist"||scene=="ferry"||scene=="sorting"||scene=="pipe"||scene=="boiler"){
   auto &p=game.state.player;
   if(scene=="hoist"){p.x=87;p.y=40-kPlayerHeight;}
   if(scene=="ferry"){p.x=110;p.y=47-kPlayerHeight;}
   if(scene=="sorting"){p.x=44;p.y=49-kPlayerHeight;}
   if(scene=="pipe"){p.x=46;p.y=41-kPlayerHeight;}
   if(scene=="boiler"){p.x=59;p.y=41-kPlayerHeight;}
   game.state.elapsed=game.state.adventure.presentation_time=1.3;
  }
  game.sync_view();update_camera(game.state,game.grid,cols,rows-7,2,10);game.tick({},0,true,cols,rows,2);game.state.adventure.door_left=0;
  if(scene=="dialogue"||scene=="archive:dialogue"){game.talk(iceberg::introduction(game.grid.district));game.dialogue_next();}
  if(scene=="quest") {game.profile.quests[0]=1;game.profile.quest_counts[0]=1;game.profile.quest_focus=0;game.sync_view();game.state.journal=true;game.state.adventure.selected=1;}
  if(scene=="map") {game.state.adventure.map=true;game.state.adventure.selected=1;}
  if(scene=="route"){game.state.adventure.map=true;game.state.adventure.map_route=true;}
  cell_frame_sink=[&](const CellFrame &frame) {
   color(ink);SDL_RenderClear(ren);
   for(size_t y=0;y<frame.size();++y)for(size_t x=0;x<frame[y].size();) {
    uint32_t rgb=cell_background(frame[y][x]);size_t end=x+1;while(end<frame[y].size()&&cell_background(frame[y][end])==rgb)++end;
    box(int(x)*cw,int(y)*ch,int(end-x)*cw,ch,{Uint8(rgb>>16),Uint8(rgb>>8),Uint8(rgb),255});x=end;
   }
   for(size_t y=0;y<frame.size();++y)for(size_t x=0;x<frame[y].size();++x)font.cell(frame[y][x],int(x)*cw,int(y)*ch,cw,ch);
  };
  RenderGrid(game.grid,game.state,rows,cols,2).draw();
  auto surface=SDL_CreateRGBSurfaceWithFormat(0,cols*cw,rows*ch,32,SDL_PIXELFORMAT_RGBA32);
  if(!surface||SDL_RenderReadPixels(ren,nullptr,SDL_PIXELFORMAT_RGBA32,surface->pixels,surface->pitch)||SDL_SaveBMP(surface,argv[5]))result=2;
  if(surface)SDL_FreeSurface(surface);
  cell_frame_sink={};
 }
 SDL_DestroyRenderer(ren);SDL_DestroyWindow(window);SDL_Quit();return result;
}
