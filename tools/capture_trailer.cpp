// Offline recording of the production cell renderer and real 120 Hz inputs.
// Scene setup is explicit; nothing teleports or changes physics during a take.
#define main terminal_velocity_unused_sdl_entry
#include "../sdl_frontend.cpp"
#undef main
#include <cstdio>
int main(int argc,char **argv) {
 if(argc!=4){std::cerr<<"scene seconds audio.f32\n";return 2;}
 const std::string scene=argv[1];int count=int(std::stod(argv[2])*60);
 if(count<1||count>3600)return 2;
 SDL_SetHint(SDL_HINT_RENDER_DRIVER,"software");
 if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER))return 2;
 SDL_Window *window=SDL_CreateWindow("Gameplay recording",0,0,1920,1080,SDL_WINDOW_HIDDEN);
 ren=SDL_CreateRenderer(window,-1,SDL_RENDERER_SOFTWARE);if(!ren)return 2;
 int result=0;
 {
  Font font;if(!font.init("assets/cell-font.ttf","assets/font.ttf"))return 2;
  iceberg::SessionOptions options;options.no_save=options.mute=true;options.seed=73;options.language=iceberg::Language::English;options.language_override=true;
  AdventureSession game(options,"/tmp/tv-trailer-unused-profile");game.begin_campaign(true);
  if(scene.rfind("run:",0)==0) {
   int id=std::stoi(scene.substr(4));const auto &p=iceworld::pattern(id);
   game.grid=verified_room(id,73,iceworld::district_of(p));
  } else if(scene=="tram") {
   game.grid=iceworld::build_estate_sequence(true);game.grid.switch_flags=3;
  } else if(scene=="inspect")game.grid=iceworld::build_room(0,73,0);
  else game.grid=iceworld::build_settlement(0);
  game.district=game.grid.district;game.state=new_state(game.grid);game.state.menu=scene=="menu";
  if(scene=="tram"){game.state.player.x=130;game.state.player.y=23-kPlayerHeight;}
  if(scene=="dialogue"||scene=="inspect") {
   auto kind=scene=="dialogue"?InteractionKind::Npc:InteractionKind::Task;
   auto node=std::find_if(game.grid.interactions.begin(),game.grid.interactions.end(),[&](const auto &n){return n.kind==kind;});
   if(node==game.grid.interactions.end())return 2;
   game.state.player.x=node->area.x;game.state.player.y=node->area.bottom()-kPlayerHeight;
  }
  game.profile.music_volume=0;game.profile.ambience_volume=.65;game.profile.effects_volume=.85;game.sync_view();
  game.audio.init("assets/audio",false,true);
  if(!game.audio.ready()){std::cerr<<game.audio.error()<<'\n';return 2;}
  update_camera(game.state,game.grid,120,26,2,10);
  std::ofstream sound(argv[3],std::ios::binary);if(!sound)return 2;
  constexpr int cw=16,ch=32,oy=12,cols=120,rows=33;
  cell_frame_sink=[&](const CellFrame &frame) {
   color(ink);SDL_RenderClear(ren);
   for(size_t y=0;y<frame.size();++y)for(size_t x=0;x<frame[y].size();) {
    uint32_t rgb=cell_background(frame[y][x]);size_t end=x+1;
    while(end<frame[y].size()&&cell_background(frame[y][end])==rgb)++end;
    box(int(x)*cw,oy+int(y)*ch,int(end-x)*cw,ch,{Uint8(rgb>>16),Uint8(rgb>>8),Uint8(rgb),255});x=end;
   }
   for(size_t y=0;y<frame.size();++y)for(size_t x=0;x<frame[y].size();++x)
    font.cell(frame[y][x],int(x)*cw,oy+int(y)*ch,cw,ch);
  };
  RoutePilot pilot;pilot.predictive=game.grid.verification_policy;
  std::vector<unsigned char> pixels(1920*1080*4);std::vector<float> samples(400*2);
  for(int frame=0;frame<count;++frame) {
   for(int tick=0;tick<2;++tick) {
    Controls c;if(scene.rfind("run:",0)==0)c=pilot.controls(game.grid,game.state);
    if(frame==30&&tick==0) {if(scene=="tram"||scene=="dialogue")c.interact=true;if(scene=="inspect")c.inspect=true;}
    game.tick(c,1./120,true,cols,rows,2);
    if(!game.audio.render(samples.data(),400)){result=2;break;}
    sound.write(reinterpret_cast<const char*>(samples.data()),std::streamsize(samples.size()*sizeof(float)));
   }
   RenderGrid(game.grid,game.state,rows,cols,2).draw();
   if(SDL_RenderReadPixels(ren,nullptr,SDL_PIXELFORMAT_RGBA32,pixels.data(),1920*4)){result=2;break;}
   if(std::fwrite(pixels.data(),1,pixels.size(),stdout)!=pixels.size()){result=2;break;}
   SDL_RenderPresent(ren);
  }
  cell_frame_sink={};
  std::cerr<<"Recorded "<<scene<<" frames="<<count<<" deaths="<<game.state.deaths<<" x="<<game.state.player.x<<" y="<<game.state.player.y<<'\n';
 }
 SDL_DestroyRenderer(ren);SDL_DestroyWindow(window);SDL_Quit();return result;
}
