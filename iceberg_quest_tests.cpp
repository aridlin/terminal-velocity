#define TERMINAL_VELOCITY_NO_TUI_MAIN
#include "main.cpp"
#include "iceberg_search.hpp"
#include <stdexcept>
void require_quest(bool okay,const char *message){if(!okay)throw std::runtime_error(message);std::cout<<"PASS "<<message<<'\n';}
Rect reach_area(Rect goal){goal.x-=1.5;goal.y-=.9;goal.w+=3;goal.h+=1.8;return goal;}
void install_test_room(AdventureSession &game,EngineGrid grid,int stage=2) {
  game.grid=std::move(grid);game.district=game.grid.district;game.stage=stage;game.room=0;
  game.state=new_state(game.grid);game.stepper={};game.sync_view();
}
void walk_to(AdventureSession &game,Rect goal) {
  auto path=find_motion_path(game.grid,game.state,goal);
  require_quest(path.reached,"ordinary-input path found");
  int deaths=game.state.deaths;
  for(auto c:path.actions)game.tick(c,.1);
  require_quest(game.state.deaths==deaths&&game.state.player.rect().intersects(goal),"path replays through production session without teleporting or damage");
}
void interact(AdventureSession &game){Controls c;c.interact=true;game.tick(c,1./120);}
void finish_dialogue(AdventureSession &game){for(int i=0;i<20&&game.state.adventure.dialogue.active();++i){Controls c;c.confirm=true;game.tick(c,1./120);}}
int main() {
  try {
    iceberg::SessionOptions options;options.no_save=true;options.mute=true;options.seed=73;
    AdventureSession game(options,std::filesystem::temp_directory_path()/"tv-quest-nosave");game.begin_campaign(true);
    {
      AdventureSession inspected(options,std::filesystem::temp_directory_path()/"tv-inspect-nosave");
      inspected.begin_campaign(true);install_test_room(inspected,iceworld::build_room(0,73,0));
      auto object=inspected.grid.interactions.front();
      inspected.state.player.x=object.area.x;inspected.state.player.y=object.area.y;
      auto original_items=inspected.profile.task_items;double elapsed=inspected.state.elapsed;
      Controls examine;examine.inspect=true;inspected.tick(examine,.1);
      require_quest(inspected.state.adventure.inspecting&&inspected.profile.task_items==original_items,
        "inspection opens without collecting or modifying a task");
      Controls move;move.move=1;move.jump_pressed=true;inspected.tick(move,.1);
      require_quest(inspected.state.elapsed==elapsed,"inspection freezes simulation and movement input");
      inspected.tick(examine,.1);require_quest(!inspected.state.adventure.inspecting,"inspection closes without using the object");
      interact(inspected);require_quest(inspected.state.adventure.pickup_left>0&&inspected.profile.task_items!=original_items,
        "parcel collection starts a visible lift animation and persists exactly once");
      auto items=inspected.profile.task_items;interact(inspected);
      require_quest(inspected.profile.task_items==items,"repeated interaction cannot duplicate a parcel");
      Controls pause;pause.pause=true;inspected.tick(pause,.1);double animation=inspected.state.adventure.pickup_left;
      inspected.tick({},.1);require_quest(inspected.state.adventure.pickup_left==animation,"pickup animation respects pause");
      inspected.tick(pause,.1);for(int i=0;i<8;++i)inspected.tick({},.1);
      require_quest(inspected.state.adventure.pickup_left==0,"pickup animation ends without locking controls");
      install_test_room(inspected,iceworld::build_settlement(0));
      auto hidden=std::find_if(inspected.grid.interactions.begin(),inspected.grid.interactions.end(),[](const Interaction &n){return n.kind==InteractionKind::Secret;});
      inspected.state.player.x=hidden->area.x;inspected.state.player.y=hidden->area.y;
      inspected.tick(examine,.1);require_quest(!inspected.state.adventure.inspecting&&inspected.profile.hidden_room<0,
        "inspection does not reveal or enter hidden entrances");
    }
    game.profile.story=63;
    for(int q:{0,1,2,3,4,7})game.profile.quests[size_t(q)]=1;
    for(int id:{0,4,8,12,16,20,13,17,21,24,28,32,26,30,34,36,40,44}) {
      install_test_room(game,verified_room(id,73,id/12));
      Interaction target{};bool found=false;
      for(auto node:game.grid.interactions)if(node.kind==InteractionKind::Task){target=node;found=true;}
      require_quest(found,"quest location reserved in assembled room");
      walk_to(game,reach_area(target.area));interact(game);
      unsigned count=game.profile.quest_counts[size_t(target.id)];interact(game);
      require_quest(game.profile.quest_counts[size_t(target.id)]==count,"task interaction cannot duplicate a collected objective");
      walk_to(game,game.grid.goal);
      require_quest(game.state.won,"optional branch has a playable return to the main exit");
    }
    for(int quest:{0,1,2,3,4,7}) {
      require_quest(game.profile.quests[size_t(quest)]==2,"all three unique objectives make the task ready");
      install_test_room(game,iceworld::build_settlement(iceberg::Quests[size_t(quest)].district),3);
      Rect goal{};for(auto node:game.grid.interactions)
        if(node.kind==InteractionKind::Npc&&node.id==iceberg::Quests[size_t(quest)].owner)goal=reach_area(node.area);
      walk_to(game,goal);interact(game);finish_dialogue(game);
      require_quest(game.profile.quests[size_t(quest)]==3&&(game.profile.cosmetics&(1u<<(quest%7+1))),"character turn-in grants its persistent reward");
    }
    {
      AdventureSession recovered(options,std::filesystem::temp_directory_path()/"tv-recovery-quest-nosave");
      recovered.begin_campaign(true);recovered.profile.story=63;
      for(int q:{0,1,2,3,4,7})recovered.profile.quests[size_t(q)]=1;
      for(int id:{0,4,8,12,16,20,13,17,21,24,28,32,26,30,34,36,40,44}) {
        auto original=iceworld::build_room(id,73,id/12);
        install_test_room(recovered,recovery_room_for(original));
        auto task=std::find_if(recovered.grid.interactions.begin(),recovered.grid.interactions.end(),
          [](const Interaction &node){return node.kind==InteractionKind::Task;});
        require_quest(task!=recovered.grid.interactions.end()&&task->id==original.interactions[0].id&&task->detail==original.interactions[0].detail,
          "generic recovery preserves the reserved task identity");
        walk_to(recovered,reach_area(task->area));interact(recovered);walk_to(recovered,recovered.grid.goal);
        require_quest(recovered.state.won,"recovery task ledge has a playable exit");
      }
      for(int q:{0,1,2,3,4,7})require_quest(recovered.profile.quests[size_t(q)]==2,
        "character task remains completable even if all its rooms use recovery routes");
    }
    EngineGrid practice(200,60);practice.adventure_level=true;practice.event_level=false;
    practice.add_solid_rect(0,50,200,10);practice.spawn_x=4;practice.spawn_y=50-kPlayerHeight;practice.goal={190,45,2,5};
    install_test_room(game,std::move(practice));game.profile.quests[6]=1;game.profile.quest_counts[6]=0;
    game.tick({},1./120);
    for(int attempt=0;attempt<3;++attempt) {
      Controls c;c.move=1;c.dash_pressed=true;game.tick(c,1./120);
      c.dash_pressed=false;c.jump_pressed=c.jump_held=true;game.tick(c,1./120);
      c.jump_pressed=false;for(int tick=0;tick<160&&!game.state.player.grounded;++tick)game.tick(c,1./120);
    }
    require_quest(game.profile.quest_counts[6]==3&&game.profile.quests[6]==2,"Kuba counts three real momentum jumps");
    install_test_room(game,iceworld::build_settlement(0),3);
    Rect kuba{};for(auto node:game.grid.interactions)if(node.kind==InteractionKind::Npc&&node.id==6)kuba=reach_area(node.area);
    walk_to(game,kuba);interact(game);finish_dialogue(game);
    require_quest(game.profile.quests[6]==3,"mastery task can be turned in on the upper settlement street");
    game.profile.story|=(uint64_t(1)<<8)|(uint64_t(1)<<9);
    for(int district=0;district<2;++district)for(int scene=0;scene<2;++scene) {
      game.district=district;game.stage=3;game.room=0;game.profile.revisit={};
      game.load_room();game.persist();
      Rect door{};bool found=false;
      for(auto node:game.grid.interactions)if(node.kind==InteractionKind::Replay&&node.id==scene){door=reach_area(node.area);found=true;}
      require_quest(found,"completed district exposes an authored route revisit");
      walk_to(game,door);interact(game);
      require_quest(game.profile.revisit.active&&game.grid.room_role==(scene?iceberg::RoomRole::Finale:iceberg::RoomRole::Lesson),"revisit door enters the chosen authored scene");
      for(auto node:game.grid.interactions)if(node.kind==InteractionKind::Return)door=reach_area(node.area);
      walk_to(game,door);interact(game);
      require_quest(!game.profile.revisit.active&&game.grid.room_role==iceberg::RoomRole::Settlement,"return door safely leaves the revisited scene");
    }
    for(int id=0;id<3;++id) {
      game.profile.hidden_room=id;install_test_room(game,iceworld::build_hidden(id));
      Rect note{},door{};
      for(auto node:game.grid.interactions){if(node.kind==InteractionKind::Note)note=reach_area(node.area);if(node.kind==InteractionKind::Return)door=reach_area(node.area);}
      walk_to(game,note);interact(game);finish_dialogue(game);
      require_quest(game.profile.discoveries&(1u<<id),"hidden testimony persists independently of campaign completion");
      walk_to(game,door);
      // The unit fixture has no underlying campaign route; return physics is
      // verified here and the campaign checkpoint restoration has its own test.
      game.profile.hidden_room=-1;
    }
    game.state.journal=true;game.state.adventure.selected=0;game.profile.quests[0]=3;
    double journal_x=game.state.player.x;interact(game);
    require_quest(game.state.adventure.dialogue.lines.size()==3,"completed mail task unlocks actual letters through the journal controls");
    finish_dialogue(game);
    require_quest(game.state.journal&&game.state.player.x==journal_x,"reading the reward returns to the journal without moving the courier");
    return 0;
  }catch(const std::exception &e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}
}
