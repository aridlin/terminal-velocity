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
    for(int mission=0;mission<3;++mission)add(iceworld::harbor_mission_arrival(mission));
    {
      AdventureSession gauge(options,std::filesystem::path{});gauge.grid=iceworld::build_harbor_mission_v4(2);gauge.state=new_state(gauge.grid);
      auto note=std::find_if(gauge.grid.interactions.begin(),gauge.grid.interactions.end(),[](const auto &node){return node.kind==InteractionKind::Note&&node.id==40;});
      gauge.harbor_interact(*note);add(gauge.state.adventure.dialogue.lines);
    }
    int pages=0;CellFrame frame;
    cell_frame_sink=[&](const CellFrame &value){frame=value;};
    for(auto language:{iceberg::Language::Polish,iceberg::Language::English}) {
      game.profile.language=language;game.sync_view();
      for(size_t id=0;id<content.size();++id) {
        for(auto dimensions:{std::pair{110,34},std::pair{76,24},std::pair{38,28},std::pair{60,20},std::pair{34,24}}) {
          const auto &line=content[id];game.tick({},0,true,dimensions.first,dimensions.second,2);
          game.talk({line});game.dialogue_next();std::string shown;
          while(game.state.adventure.dialogue.active()) {
            RenderGrid(game.grid,game.state,dimensions.second,dimensions.first,2).draw();
            auto &dialogue=game.state.adventure.dialogue;
            int courier=4+int(std::floor((game.state.player.y-game.state.camera_y)/.8));
            auto layout=iceberg::dialogue_layout(line.text.get(language),dimensions.first,dimensions.second,courier,dialogue.segment);
            int first=dialogue.segment*layout.text_rows;
            for(int row=0;row<std::min(layout.text_rows,int(layout.body.size())-first);++row) {
              for(int col=layout.text_x;col<dimensions.first-3;++col)shown+=frame[size_t(layout.text_y+row)][size_t(col)].glyph;
              shown+=' ';
            }
            if(layout.top+layout.height>dimensions.second-3||layout.top<4)throw std::runtime_error("dialogue leaves world viewport");
            int feet=5+int(std::floor((game.state.player.y+kPlayerHeight-.001-game.state.camera_y)/.8));
            if(feet-1>=layout.top&&feet-3<layout.top+layout.height)throw std::runtime_error("dialogue covers courier");
            if(feet-3<4||feet-1>=dimensions.second-3)throw std::runtime_error("dialogue reframes courier outside viewport");
            game.dialogue_next();
          }
          if(normalize_text(shown)!=normalize_text(line.text.get(language))) {
            std::cerr<<"FAIL clipped dialogue index="<<id<<" language="<<int(language)<<" size="<<dimensions.first<<"x"<<dimensions.second<<'\n';return 1;
          }
          ++pages;
        }
      }
    }
    cell_frame_sink={};std::cout<<"PASS "<<pages<<" authored dialogue layouts fully readable with courier visible in 110x34, 76x24, 38x28, 60x20 and 34x24 cells, Polish and English\n";
    return 0;
  }catch(const std::exception &e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}
}
