#define TERMINAL_VELOCITY_NO_TUI_MAIN
#include "main.cpp"
#include <stdexcept>
void require_inspect(bool yes,const char *what){if(!yes)throw std::runtime_error(what);}
int main(){try {
  iceberg::SessionOptions options;options.no_save=options.mute=true;
  AdventureSession game(options,std::filesystem::temp_directory_path()/"tv-inspect-art-test");
  game.begin_campaign(true);game.state.menu=false;CellFrame frame;
  cell_frame_sink=[&](const CellFrame &f){frame=f;};
  int studies=0,pages=0;
  for(int kind=0;kind<=int(iceberg::InspectionArt::Delivery);++kind) {
    auto art=iceberg::inspection_art(iceberg::InspectionArt(kind),0);
    require_inspect(art.size()==12,"inspection study must be independently drawn at twelve rows");
    for(const auto &line:art)require_inspect(iceberg::cell_length(line)<=28,"inspection study overflows its art column");
    ++studies;
  }
  for(auto language:{iceberg::Language::Polish,iceberg::Language::English})for(auto size:std::vector<std::pair<int,int>>{{110,34},{76,24},{38,28},{34,24},{60,20}}) {
    game.profile.language=language;game.sync_view();
    std::vector<Interaction> objects;
    for(int q=0;q<8;++q)objects.push_back({{5,40,3,4},InteractionKind::Task,q,{"Przedmiot","Object"},q*3});
    for(int npc=0;npc<8;++npc)objects.push_back({{5,40,3,4},InteractionKind::Npc,npc,{"Osoba","Person"}});
    for(int note:{0,1,2,3,4,5,6,7,8,9,20,21,22,30})objects.push_back({{5,40,3,4},InteractionKind::Note,note,{"Tablica","Sign"}});
    objects.push_back({{5,40,3,4},InteractionKind::Note,iceberg::CrownShaftPlate,{"Tabliczka przy szybie","Shaft maintenance plate"}});
    for(auto kind:{InteractionKind::Epilogue,InteractionKind::Evidence,InteractionKind::Valve,InteractionKind::Carry,InteractionKind::Deliver,InteractionKind::Board,InteractionKind::Replay})objects.push_back({{5,40,3,4},kind,0,{"Mechanizm","Mechanism"}});
    for(auto node:objects) {
      game.grid=iceworld::build_settlement(0);game.grid.interactions={node};game.state=new_state(game.grid);game.state.menu=false;
      game.state.player.x=5;game.state.player.y=41;game.sync_view();
      auto original_items=game.profile.task_items;double elapsed=game.state.elapsed;
      Controls c;c.inspect=true;game.tick(c,1./120,true,size.first,size.second);
      require_inspect(game.state.adventure.inspecting,"production inspection did not open");
      auto &v=game.state.adventure;
      auto layout=iceberg::inspection_layout(v.inspect_title,v.inspect_body,size.first,size.second);
      std::string seen;
      for(int page=0;page<layout.pages;++page) {
        RenderGrid(game.grid,game.state,size.second,size.first,2).draw();
        auto art=iceberg::inspection_art(v.inspect_art,v.inspect_detail);
        for(size_t y=0;y<art.size();++y) {
          std::string line;for(int x=0;x<iceberg::cell_length(art[y]);++x)line+=frame[size_t(layout.art_y+y)][size_t(layout.art_x+x)].glyph;
          require_inspect(line==art[y],"inspection illustration clipped or overdrawn");
        }
        for(int y=0;y<layout.text_rows;++y) {
          for(int x=0;x<layout.text_width;++x)seen+=frame[size_t(layout.text_y+y)][size_t(layout.text_x+x)].glyph;
          seen+=' ';
        }
        c={};c.interact=true;game.tick(c,1./120,true,size.first,size.second);++pages;
      }
      auto words=[](const std::string &s){std::istringstream in(s);std::string word,out;while(in>>word)out+=word+" ";return out;};
      require_inspect(words(seen)==words(v.inspect_body),"paginated inspection loses description text");
      require_inspect(!v.inspecting&&game.state.elapsed==elapsed&&game.profile.task_items==original_items,"reading inspection changes the world or does not close");
    }
  }
  for(auto language:{iceberg::Language::Polish,iceberg::Language::English}) {
    game.profile.language=language;
    game.grid=iceworld::build_settlement(0);
    game.grid.interactions={{{5,40,3,4},InteractionKind::Note,0,{"Tablica","Sign"}}};
    game.state=new_state(game.grid);game.state.menu=false;
    game.state.player.x=5;game.state.player.y=41;
    Controls inspect;inspect.inspect=true;
    game.tick(inspect,0,true,34,24);
    auto &v=game.state.adventure;
    require_inspect(v.inspecting,"open the training sign at a compact width");
    const auto narrow=iceberg::inspection_layout(v.inspect_title,v.inspect_body,34,24);
    require_inspect(narrow.pages>1,"reflow fixture must have multiple compact pages");
    Controls next;next.navigate=1;
    game.tick(next,0,true,34,24);
    const auto anchor=v.inspect_offset;
    require_inspect(narrow.page_at(anchor)==1,"select the second page");
    const auto wide=iceberg::inspection_layout(v.inspect_title,v.inspect_body,110,34);
    require_inspect(wide.pages==1,"the wide version must fit on one page");
    game.tick({},0,true,110,34);
    require_inspect(wide.page_at(v.inspect_offset)==0&&v.inspect_offset==anchor,
      "a one-page view keeps the original reading position");
    game.tick({},0,true,34,24);
    RenderGrid(game.grid,game.state,24,34,2).draw();
    const std::string expected=narrow.body[size_t(narrow.text_rows)];
    std::string rendered;
    for(const auto &cell:frame[size_t(narrow.text_y)])rendered+=cell.glyph;
    require_inspect(rendered.find(expected)!=std::string::npos&&narrow.page_at(v.inspect_offset)==1,
      "shrinking restores the second page and the actual text");
    Controls previous;previous.navigate=-1;
    game.tick(previous,0,true,34,24);
    require_inspect(v.inspect_offset==0,"paging deliberately moves the reading position");
    game.tick(inspect,0,true,34,24);
  }
  std::cout<<"PASS inspection reflow keeps the active passage in both languages\n";
  game.grid.interactions={{{5,40,3,4},InteractionKind::Note,0,{"Tablica","Sign"}}};
  game.state.player.x=5;game.state.player.y=41;
  Controls inspect;inspect.inspect=true;
  game.tick(inspect,0);
  require_inspect(game.state.adventure.inspecting,"could not reopen inspection for focus check");
  game.tick({},.1,false);game.tick({},.1,true);
  require_inspect(game.state.adventure.inspecting&&!game.state.paused,
    "refocusing the inspection must keep it open without adding a pause");
  game.tick(inspect,0);
  require_inspect(!game.state.adventure.inspecting&&!game.state.paused,"closing the inspection should return to play");
  std::cout<<"PASS inspection survives a focus round trip without adding a pause\n";
  cell_frame_sink={};std::cout<<"PASS "<<studies<<" detailed studies and "<<pages<<" actual inspection pages, both languages, five screen sizes; no clipping or world mutation\n";
  return 0;
}catch(const std::exception &e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
