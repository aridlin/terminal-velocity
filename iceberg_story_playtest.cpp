#define TERMINAL_VELOCITY_NO_TUI_MAIN
#include "main.cpp"
#include "iceberg_search.hpp"
#include <stdexcept>
struct StoryReview {
  AdventureSession &game;
  int pages=0,words=0,searches=0;
  double reading_estimate=0;
  void read_dialogue() {
    while(game.state.adventure.dialogue.active()) {
      if(game.state.adventure.dialogue.revealing)game.dialogue_next();
      auto text=game.state.adventure.dialogue.lines[game.state.adventure.dialogue.page].text.get(game.profile.language);
      std::istringstream input(text);std::string word;int n=0;while(input>>word)++n;
      words+=n;++pages;reading_estimate+=n/3.+.6;
      Controls c;c.confirm=true;game.tick(c,1./120);
    }
  }
  void interact(){Controls c;c.interact=true;game.tick(c,1./120);read_dialogue();}
  void walk(Rect target) {
    ++searches;auto path=find_motion_path(game.grid,game.state,target);
    if(!path.reached)throw std::runtime_error("No ordinary-input interaction path, template="+std::to_string(game.grid.template_id)+" target="+std::to_string(target.x)+","+std::to_string(target.y)+" from="+std::to_string(game.state.player.x)+","+std::to_string(game.state.player.y)+" nearest="+std::to_string(path.nearest));
    for(auto c:path.actions)game.tick(c,.1);
    if(!game.state.player.rect().intersects(target))throw std::runtime_error("Input path replay diverged");
  }
  static Rect reach(Rect a){a.x-=1.5;a.y-=.9;a.w+=3;a.h+=1.8;return a;}
  void residents() {
    std::vector<Interaction> npcs;
    for(auto node:game.grid.interactions)if(node.kind==InteractionKind::Npc)npcs.push_back(node);
    for(auto node:npcs) {
      walk(reach(node.area));interact();
      // The first arrival conversation precedes the personal task offer.
      int quest=0;for(int q=0;q<8;++q)if(iceberg::Quests[size_t(q)].owner==node.id)quest=q;
      if(game.profile.quests[size_t(quest)]==0)interact();
    }
  }
};
int main() {
  try {
    iceberg::SessionOptions options;options.no_save=true;options.mute=true;options.seed=73;
    AdventureSession game(options,std::filesystem::temp_directory_path()/"tv-story-review-no-save");
    game.begin_campaign(true);StoryReview review{game};RoutePilot pilot;
    std::string last;int rooms=0,jump_phase=0;
    for(int tick=0;tick<120*60*60&&!game.profile.finished&&!game.profile.at_content_boundary;++tick) {
      review.read_dialogue();
      std::string identity=std::to_string(game.district)+":"+std::to_string(game.stage)+":"+std::to_string(game.room)+":"+std::to_string(game.profile.interlude);
      if(identity!=last){last=identity;pilot={};pilot.predictive=game.grid.verification_policy;++rooms;}
      if(game.state.won){Controls c;c.confirm=true;game.tick(c,1./120);continue;}
      if(game.grid.room_role==iceberg::RoomRole::Settlement) {
        review.residents();
        Rect board{};for(auto node:game.grid.interactions)if(node.kind==InteractionKind::Board)board=StoryReview::reach(node.area);
        review.walk(board);review.interact();continue;
      }
      bool task=false;
      for(auto node:game.grid.interactions)if(node.kind==InteractionKind::Task&&
        !(game.profile.task_items[size_t(node.detail/64)]&(uint64_t(1)<<(node.detail%64)))) {
        review.walk(StoryReview::reach(node.area));review.interact();review.walk(game.grid.goal);task=true;break;
      }
      if(task)continue;
      Controls c=pilot.controls(game.grid,game.state);
      // Exercise the taught technique on actual safe route landings.
      if(game.profile.quest_counts[6]<3) {
        auto &p=game.state.player;
        int direction=c.move?c.move:1;
        int foot=int(std::floor(p.y+kPlayerHeight+.1));
        if(jump_phase) {c.move=jump_phase;c.jump_pressed=c.jump_held=true;c.dash_pressed=false;jump_phase=0;}
        else if(p.grounded&&p.dash_available&&p.dash_time_left<=0&&
           game.grid.solid_at(int(std::floor(p.x+direction*1.8)),foot)) {
          c.move=direction;c.dash_pressed=true;c.jump_pressed=false;jump_phase=direction;
        }
      }
      int at=game.near_interaction();
      if(at>=0&&(game.grid.interactions[size_t(at)].kind==InteractionKind::Valve||game.grid.interactions[size_t(at)].kind==InteractionKind::Rescue||game.grid.interactions[size_t(at)].kind==InteractionKind::Carry||game.grid.interactions[size_t(at)].kind==InteractionKind::Deliver||game.grid.interactions[size_t(at)].kind==InteractionKind::Evidence))c.interact=true;
      game.tick(c,1./120);
      if(game.state.deaths>15)throw std::runtime_error("Repeated failure in "+identity);
    }
    if(!game.profile.at_content_boundary&&!game.profile.finished)throw std::runtime_error("Story review timed out");
    review.residents();
    for(int quest:{0,1,2,3,4,6,7})if(game.profile.quests[size_t(quest)]!=3)
      throw std::runtime_error("Task unfinished: "+std::to_string(quest));
    std::cout<<"PASS continuous journey with all seven available tasks, no room teleporting\n"
      <<"rooms="<<rooms<<" active_seconds="<<game.profile.campaign_seconds<<" deaths="<<game.profile.campaign_deaths
      <<" dialogue_pages="<<review.pages<<" dialogue_words="<<review.words
      <<" reading_estimate_180wpm_seconds="<<review.reading_estimate
      <<" combined_estimate_seconds="<<game.profile.campaign_seconds+review.reading_estimate<<'\n';
    std::cout<<"Reading time is an estimate, not measured human first-play duration.\n";
    return 0;
  }catch(const std::exception &e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}
}
