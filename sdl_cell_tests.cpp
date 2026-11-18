// Exercise the production Unicode rasterizer at actual drawable pixel sizes.
#define main unused_sdl_main_for_tests
#include "sdl_frontend.cpp"
#undef main
#include <cassert>

int main(int argc,char **argv) {
  auto initial=unicode_viewport(1100,680,1100,680,10),highdpi=unicode_viewport(2200,1360,1100,680,10);
  assert(initial.columns==110&&initial.rows==34);
  assert(highdpi.columns==initial.columns&&highdpi.rows==initial.rows&&highdpi.cell_w==initial.cell_w*2);
  auto fullscreen=unicode_viewport(3840,2160,3840,2160,10);
  assert(fullscreen.columns<=160&&fullscreen.rows<=48&&fullscreen.cell_w>=24);
  auto zoomed=unicode_viewport(1500,760,1500,760,20);
  assert(zoomed.columns>=56&&zoomed.rows>=20);
  for(auto v:{initial,highdpi,fullscreen,zoomed})assert(v.cell_h==v.cell_w*2);
  SDL_SetHint(SDL_HINT_RENDER_DRIVER,"software");
  if(SDL_Init(SDL_INIT_VIDEO))return 2;
  SDL_Surface *surface=SDL_CreateRGBSurfaceWithFormat(0,320,180,32,SDL_PIXELFORMAT_ARGB8888);
  ren=SDL_CreateSoftwareRenderer(surface);if(!ren)return 2;
  {
    Font font;assert(font.init("assets/cell-font.ttf","assets/font.ttf"));
    auto pixels=[&](const std::string &glyph,int w,int h){
      color({0,0,0});SDL_RenderClear(ren);font.cell(Cell{glyph,Color::Player,false,false,0,0xffffff},0,0,w,h);
      std::vector<Uint32> data(size_t(w*h));
      SDL_Rect area{0,0,w,h};assert(SDL_RenderReadPixels(ren,&area,SDL_PIXELFORMAT_ARGB8888,data.data(),w*4)==0);
      return data;
    };
    // Full braille and shading must retain density as well as cell aspect.
    // This catches fixed pixel padding and the former 2→5-pixel dot jump.
    for(const auto &glyph:{"⣿","░","▒","▓","═","║","▟"}){
      auto one=pixels(glyph,10,20),two=pixels(glyph,20,40);
      for(int y=0;y<40;++y)for(int x=0;x<20;++x)
        assert(two[size_t(y*20+x)]==one[size_t(y/2*10+x/2)]);
    }
    // Odd widths may not leave a one-pixel crack at quadrant boundaries.
    auto odd=pixels("▟",11,22);
    for(int y=0;y<22;++y)for(int x=0;x<11;++x)
      assert(bool(odd[size_t(y*11+x)]&0xffffff)==(x>=5||y>=11));
    // Actual glyph bounds, allowing normal hinting differences of one pixel.
    auto bounds=[](const auto &data,int w,int h){
      int x0=w,y0=h,x1=-1,y1=-1;
      for(int y=0;y<h;++y)for(int x=0;x<w;++x)if(data[size_t(y*w+x)]&0xffffff){x0=std::min(x0,x);y0=std::min(y0,y);x1=std::max(x1,x);y1=std::max(y1,y);}
      return std::pair{x1-x0+1,y1-y0+1};
    };
    for(const auto &glyph:{"W","M","ą","Ż","○"}){
      auto small=bounds(pixels(glyph,10,20),10,20),large=bounds(pixels(glyph,20,40),20,40);
      assert(std::abs(large.first-small.first*2)<=2);
      assert(std::abs(large.second-small.second*2)<=2);
    }
    color({16,25,35});SDL_RenderClear(ren);
    for(int row=0;row<3;++row){int cw=10+row*5,ch=cw*2,x=0,y=row*55;for(auto glyph:{"W","M","ą","Ż"," ","○","⣿"," ","░","▒","▓"," ","═","║"," ","▟"}){font.cell(Cell{glyph,Color::Player,false,false,0,0xffffff},x,y,cw,ch);x+=cw;}}
    if(argc>1)assert(SDL_SaveBMP(surface,argv[1])==0);
  }
  SDL_DestroyRenderer(ren);ren=nullptr;SDL_FreeSurface(surface);SDL_Quit();
  std::cout<<"PASS production SDL viewport/DPI/zoom bounds, glyph proportions, doubled strokes/dots, odd-cell quadrant coverage\n";
}
