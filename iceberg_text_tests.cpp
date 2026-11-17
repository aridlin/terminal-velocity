#define TERMINAL_VELOCITY_NO_TUI_MAIN
#include "main.cpp"
#include <stdexcept>
std::string normalize_text(const std::string &input){std::istringstream in(input);std::string word,out;while(in>>word){if(!out.empty())out+=' ';out+=word;}return out;}
int main() {
  try {
    iceberg::SessionOptions options;options.mute=true;options.no_save=true;
    AdventureSession game(options,std::filesystem::temp_directory_path()/"tv-text-layout-no-save");game.state.menu=false;
    std::vector<iceberg::DialogueLine> content;
    auto add=[&](auto lines){content.insert(content.end(),lines.begin(),lines.end());};
    for(int d=0;d<6;++d){add(iceberg::introduction(d));add(iceberg::conclusion(d));for(int leg=0;leg<2;++leg)add(iceberg::homecoming(d,leg));}
    for(int q=0;q<8;++q){for(unsigned state=0;state<4;++state)add(iceberg::small_talk(q,state));add(iceberg::task_thanks(q));add(iceberg::task_record(q));}
    for(int s=0;s<3;++s){add(iceberg::hidden_testimony(s));add(iceberg::mission_arrival(s));}
    add(iceberg::jubilee_arrival());add(iceberg::jubilee_dedication());add(iceberg::jubilee_complete());
    add(iceberg::archive_record());
    add(std::vector<iceberg::DialogueLine>{{6,iceberg::crown_sign()}});
    add(iceberg::epilogue_arrival());add(iceberg::journey_end());
    for(int npc=0;npc<8;++npc)add(std::vector<iceberg::DialogueLine>{iceberg::epilogue_resident(npc)});
    for(int action=0;action<2;++action)add(iceberg::epilogue_action(action));
    for(int note:{20,21})add(std::vector<iceberg::DialogueLine>{{5,iceberg::garden_sign(note)}});
    for(int note=0;note<10;++note)add(std::vector<iceberg::DialogueLine>{{6,iceberg::training_sign(note)}});
    add(iceberg::rescued_conductor());add(iceberg::parcel_delivered());
    int pages=0;CellFrame frame;
    cell_frame_sink=[&](const CellFrame &value){frame=value;};
    for(auto language:{iceberg::Language::Polish,iceberg::Language::English}) {
      game.profile.language=language;game.sync_view();
      for(size_t id=0;id<content.size();++id) {
        const auto &line=content[id];game.talk({line});game.dialogue_next();
        RenderGrid(game.grid,game.state,24,76,2).draw();
        std::string shown;
        for(int row=13;row<20;++row){for(int col=24;col<72;++col)shown+=frame[size_t(row)][size_t(col)].glyph;shown+=' ';}
        if(normalize_text(shown)!=normalize_text(line.text.get(language))) {
          std::cerr<<"FAIL clipped dialogue index="<<id<<" language="<<int(language)<<'\n';return 1;
        }
        RenderGrid(game.grid,game.state,28,38,2).draw();shown.clear();
        for(int row=11;row<26;++row){for(int col=2;col<36;++col)shown+=frame[size_t(row)][size_t(col)].glyph;shown+=' ';}
        if(normalize_text(shown)!=normalize_text(line.text.get(language))) {
          std::cerr<<"FAIL compact dialogue index="<<id<<" language="<<int(language)<<'\n';return 1;
        }
        RenderGrid(game.grid,game.state,20,60,2).draw();shown.clear();
        for(int row=11;row<18;++row){for(int col=2;col<58;++col)shown+=frame[size_t(row)][size_t(col)].glyph;shown+=' ';}
        if(normalize_text(shown)!=normalize_text(line.text.get(language))) {
          std::cerr<<"FAIL landscape dialogue index="<<id<<" language="<<int(language)<<'\n';return 1;
        }
        ++pages;
      }
    }
    cell_frame_sink={};std::cout<<"PASS "<<pages<<" authored dialogue pages fully visible in the actual 76x24, compact 38x28 and landscape 60x20 cell renderers, Polish and English\n";
    return 0;
  }catch(const std::exception &e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}
}
