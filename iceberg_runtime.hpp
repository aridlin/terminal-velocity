#pragma once
// Session owns progression and interaction, not a frontend. SDL and TUI feed
// identical Controls and read the same GameState and rendered cell frame.
class AdventureSession {
public:
  iceberg::Profile profile;
  iceberg::SaveStore saves;
  iceberg::SessionOptions options;
  EngineGrid grid;
  GameState state;
  FixedStepper stepper;
  GameAudio audio;
  ghosts::Client network;
  uint32_t network_epoch=0,previous_course=0;
  int network_deaths=0;
  int last_columns=110,last_rows=34;
  double last_aspect=2;
  iceberg::Mode mode = iceberg::Mode::Campaign;
  bool running = true, legacy_requested = false;
  int district = 0, stage = 0, room = 0, depth = 0, run_district = 0;
  uint32_t run_seed = 1;
  double autosave_left = 15, run_seconds = 0;
  int run_deaths = 0, run_treasures = 0;
  bool previous_focus = true;
  bool run_active = false;

  explicit AdventureSession(const iceberg::SessionOptions &o,
      const std::filesystem::path &save_file = iceberg::SaveStore::default_file())
    : saves(save_file), options(o), grid(iceworld::build_settlement(0)), mode(o.mode), run_seed(o.seed) {
    saves.load(profile);
    if(profile.content_version!=iceberg::ContentVersion) {
      for(int d=0;d<iceberg::DistrictCount;++d)if(profile.trials[size_t(d)].complete) {
        if(profile.challenges.size()<128)profile.challenges.push_back({iceberg::mix(0x54495241u+unsigned(d)*7919u),d,profile.content_version,profile.trials[size_t(d)]});
        profile.trials[size_t(d)]={};
      }
    }
    profile.content_version=iceberg::ContentVersion;run_district=std::min(o.district,expedition_unlocked());
    if (!profile.started) profile.seed = o.seed;
    if (o.language_override) profile.language = o.language;
    district = profile.district; stage = profile.stage; room = profile.room;
    run_seed = profile.seed;
    if (mode != iceberg::Mode::Campaign) {
      district = run_district; stage = 2; room = 0; run_seed = o.seed;
    }
    audio.init(audio_directory(), o.mute || (o.remote && !o.remote_audio));
    load_room(true);
    state.menu = true;
    state.adventure.selected=mode==iceberg::Mode::Endless?2:mode==iceberg::Mode::Trial?3:0;
    choose_saved_run_district(state.adventure.selected);
    if(!o.ghost_host.empty()) {
      try {network.connect_to(o.ghost_host,o.ghost_port);}
      catch(const std::exception &e){notice(e.what(),10);}
    }
    if(saves.recovered)notice(t("Przywrócono ostatni zdrowy zapis.","Recovered the last healthy save."),8);
    if(saves.protected_future)notice(t("Ten zapis wymaga nowszej wersji gry. Nie zostanie nadpisany.","This save needs a newer game version. It will not be overwritten."),12);
    sync_view();
  }
  ~AdventureSession() = default;
  std::string t(const char *pl,const char *en) const { return iceberg::tr(profile.language,pl,en); }
  unsigned route_version() const {
    return mode==iceberg::Mode::Campaign?profile.layout_version:profile.expeditions[mode==iceberg::Mode::Endless?0:1].layout_version;
  }
  int music_track() const {
    // The short snow theme belongs to inhabited resting places. Long route
    // pieces keep an expedition from repeating a 25-second musical phrase.
    if(grid.room_role==iceberg::RoomRole::Settlement)return 2;
    if(grid.room_role==iceberg::RoomRole::Interlude)return 1;
    return grid.district==0?0:1;
  }
  void notice(const std::string &text, double seconds=3) {
    state.adventure.notice=text;state.adventure.notice_left=seconds;
  }
  int expedition_unlocked() const {
    int available=profile.unlocked;
    for(int d=0;d<iceberg::AuthoredDistrictCount;++d)if(profile.records[size_t(d)].complete)available=std::max(available,d+1);
    return std::min(iceberg::AuthoredDistrictCount-1,available);
  }
  void choose_saved_run_district(int selected) {
    if(options.district_override||selected<2||selected>3)return;
    const auto &e=profile.expeditions[size_t(selected-2)];
    run_district=e.active?e.district:std::min(run_district,expedition_unlocked());
  }
  void sync_view() {
    auto &v=state.adventure;
    v.language=profile.language;v.mode=mode;v.district=grid.district;v.stage=stage;v.room=room;v.depth=depth;
    v.unlocked=profile.unlocked;v.finished=profile.finished;v.at_content_boundary=profile.at_content_boundary;v.can_continue=profile.started;
    v.endless_resume=profile.expeditions[0].active&&profile.expeditions[0].district==run_district;
    v.trial_resume=profile.expeditions[1].active&&profile.expeditions[1].district==run_district;v.revisit=profile.revisit.active;
    v.hidden_room=profile.hidden_room;v.visit_district=profile.visit_district;v.cosmetics=profile.cosmetics;
    v.run_district=run_district;v.endless_best=profile.endless_best;v.seed=mode==iceberg::Mode::Campaign?profile.seed:run_seed;
    v.outfit=profile.outfit;v.quests=profile.quests;v.quest_counts=profile.quest_counts;v.task_items=profile.task_items;
    v.records=profile.records;v.trials=profile.trials;
    for(int d=0;d<iceberg::DistrictCount;++d) {
      uint32_t wanted=options.seed_override?options.seed:iceberg::mix(0x54495241u+unsigned(d)*7919u);
      const auto &e=profile.expeditions[1];
      unsigned version=e.active&&e.district==d&&e.seed==wanted?e.layout_version:iceberg::ContentVersion;
      if(options.seed_override||version!=iceberg::ContentVersion) {
        v.trials[size_t(d)]={};
        for(const auto &challenge:profile.challenges)if(challenge.district==d&&challenge.seed==wanted&&challenge.version==version)
          v.trials[size_t(d)]=challenge.record;
      }
    }
    v.discoveries=profile.discoveries;
    v.switches=grid.switch_flags;v.room_name=(grid.room_role==iceberg::RoomRole::Settlement?
      iceberg::Districts[size_t(grid.district)].subtitle:grid.room_name).get(profile.language);
    v.music_volume=profile.music_volume;v.effects_volume=profile.effects_volume;v.ambience_volume=profile.ambience_volume;
    state.audio_muted=audio.muted;state.audio_error=audio.error();
  }
  void network_frame(double dt,bool focused,int columns,int rows) {
    if(!network.enabled())return;
    ghosts::Snapshot snapshot;
    snapshot.room=options.ghost_room;
    snapshot.level=ghosts::adventure_level(route_version(),grid.district,int(mode),grid.template_id);
    snapshot.seed=grid.seed?grid.seed:mode==iceberg::Mode::Campaign?profile.seed:run_seed;
    // Optional unlocked geometry is part of course identity too.
    if(grid.district==2&&profile.quests[3]==3)snapshot.seed^=0x72574f46u;
    if(snapshot.level!=previous_course||network_deaths!=state.deaths)++network_epoch;
    previous_course=snapshot.level;network_deaths=state.deaths;
    snapshot.x=state.player.x;snapshot.y=state.player.y;snapshot.epoch=network_epoch;
    bool visible=focused&&!state.menu&&!state.paused&&!state.journal&&!state.adventure.map&&
      !state.adventure.dialogue.active()&&!state.adventure.inspecting&&columns>=34&&rows>=(columns>=56?20:24)&&grid.room_role!=iceberg::RoomRole::Secret;
    snapshot.flags=(state.player.dash_available?1:0)|(visible?2:0)|(state.won?4:0);
    network.tick(snapshot,dt);state.multiplayer=true;state.ghosts=network.visible();
    if(grid.room_role==iceberg::RoomRole::Secret)state.ghosts.clear();
  }
  void persist() {
    if (mode == iceberg::Mode::Campaign && profile.started && profile.hidden_room<0 && profile.visit_district<0 && !profile.revisit.active) {
      profile.district=district;profile.stage=stage;profile.room=room;
      profile.checkpoint=state.checkpoint;profile.switches=grid.switch_flags;
      profile.collected=0;
      for(size_t i=0;i<state.collected.size() && i<64;++i)
        if(state.collected[i])profile.collected|=uint64_t(1)<<i;
      profile.room_seconds=state.elapsed;profile.room_deaths=state.deaths;
    }
    if(mode==iceberg::Mode::Campaign&&profile.revisit.active&&profile.hidden_room<0) {
      auto &r=profile.revisit;r.checkpoint=state.checkpoint;r.switches=grid.switch_flags;
      r.collected=0;for(size_t i=0;i<state.collected.size()&&i<64;++i)if(state.collected[i])r.collected|=uint64_t(1)<<i;
      r.seconds=state.elapsed;r.deaths=state.deaths;
    }
    if(mode!=iceberg::Mode::Campaign && run_active) {
      auto &e=profile.expeditions[mode==iceberg::Mode::Endless?0:1];
      e.active=true;e.seed=run_seed;e.district=district;e.room=room;e.depth=depth;e.checkpoint=state.checkpoint;
      e.collected=0;for(size_t i=0;i<state.collected.size()&&i<64;++i)if(state.collected[i])e.collected|=uint64_t(1)<<i;
      e.switches=grid.switch_flags;e.room_seconds=state.elapsed;e.room_deaths=state.deaths;
      e.seconds=run_seconds;e.deaths=run_deaths;e.treasures=run_treasures;
    }
    if(options.no_save){autosave_left=15;return;}
    state.adventure.save_error=!saves.save(profile);
    autosave_left=15;
  }
  void clear_room_record() {
    profile.checkpoint=0;profile.switches=profile.collected=0;
    profile.room_seconds=0;profile.room_deaths=0;
  }
  void load_room(bool resume=false) {
    if(mode==iceberg::Mode::Campaign && profile.hidden_room>=0) {
      grid=iceworld::build_hidden(profile.hidden_room);
    } else if(mode==iceberg::Mode::Campaign && profile.revisit.active) {
      grid=profile.revisit.scene==2?iceworld::build_jubilee():iceworld::build_authored(profile.revisit.district,profile.revisit.scene==1);
      if(profile.revisit.scene==2&&(profile.story&(uint64_t(1)<<40)))grid.switch_flags=7;
      grid.interactions.insert(grid.interactions.begin(),{{grid.spawn_x-2,grid.spawn_y-1,1,3},InteractionKind::Return,0,{"Wróć do osady","Return to settlement"}});
    } else if(mode==iceberg::Mode::Campaign && profile.visit_district>=0) {
      grid=iceworld::build_settlement(profile.visit_district);
    } else if(mode==iceberg::Mode::Trial&&room==iceberg::RoomsPerLeg-1) {
      grid=iceworld::build_mastery_course(district);grid.seed=run_seed;
    } else if(mode==iceberg::Mode::Campaign && profile.interlude) {
      grid=iceworld::build_harbor_mission(stage==2?0:stage==4?1:2);
    } else if(mode==iceberg::Mode::Campaign && (stage==0 || stage==3 || stage==5)) {
      grid=iceworld::build_settlement(district);
    } else if(mode==iceberg::Mode::Campaign && (stage==1 || stage==7)) {
      grid=iceworld::build_authored(district,stage==7);
    } else {
      if(mode==iceberg::Mode::Campaign) {
        if(profile.itinerary.size()!=size_t(iceberg::RoomsPerLeg*3))
          profile.itinerary=iceworld::make_itinerary(profile.seed,district,iceberg::RoomsPerLeg*3,profile.layout_version);
        int leg=stage==2?0:stage==4?1:2;
        grid=verified_room(profile.itinerary[size_t(leg*iceberg::RoomsPerLeg+room)],
          iceberg::mix(profile.seed ^ unsigned(district*104729+leg*31+room)),district);
      } else {
        auto &e=profile.expeditions[mode==iceberg::Mode::Endless?0:1];
        if(e.itinerary.size()!=iceberg::RoomsPerLeg)e.itinerary=iceworld::make_itinerary(run_seed,district,iceberg::RoomsPerLeg,e.layout_version);
        grid=verified_room(e.itinerary[size_t(room % iceberg::RoomsPerLeg)],
          iceberg::mix(run_seed+unsigned(depth)),district,mode==iceberg::Mode::Trial);
      }
    }
    if(mode==iceberg::Mode::Campaign && profile.quests[1]==3 && district==1 && room==0 && grid.room_role==iceberg::RoomRole::Traverse)
      grid.interactions.push_back({{grid.spawn_x+2,grid.spawn_y-1,3,3},InteractionKind::Shortcut,0,{"Przejście serwisowe","Service passage"}});
    if(profile.quests[3]==3 && grid.district==2 && grid.room_role==iceberg::RoomRole::Settlement) {
      grid.add_solid_rect(55,31,11,1);grid.springs.push_back({50,38.7,1.5,.3});
      grid.sweets.push_back({60,29});grid.sweets.push_back({63,29});
    }
    if(mode==iceberg::Mode::Campaign&&grid.room_role==iceberg::RoomRole::Settlement&&
        (profile.story&(uint64_t(1)<<(grid.district+8)))) {
      int ground=grid.district==1?48:43;
      for(int i=0;i<2;++i)grid.interactions.push_back({{i?grid.district==1?67.:62.:24.,ground-3.,2,3},
        InteractionKind::Replay,i,i?iceberg::Text{"Wróć do stacji pomp","Revisit the pumping station"}:
        iceberg::Text{"Trasa szkoleniowa","Training route"}});
    }
    if(grid.room_role==iceberg::RoomRole::Settlement)for(auto &node:grid.interactions)if(node.kind==InteractionKind::Board)
      node.label=profile.visit_district>=0?iceberg::Text{"Wróć na swoją trasę","Return to your route"}:iceberg::route_title(grid.district,stage);
    auto old_view=state.adventure;
    state=new_state(grid);state.adventure=std::move(old_view);
    state.adventure.dialogue={};state.adventure.map=false;state.adventure.inspecting=false;state.adventure.pickup_left=0;state.adventure.prompt.clear();
    state.ssh=options.remote;
    if(resume && mode==iceberg::Mode::Campaign && profile.hidden_room<0 && profile.visit_district<0 && !profile.revisit.active) {
      grid.switch_flags=profile.switches;
      for(size_t i=0;i<state.collected.size() && i<64;++i)state.collected[i]=(profile.collected>>i)&1;
      if(size_t(profile.checkpoint)<grid.checkpoint_points.size()) {
        state.checkpoint=profile.checkpoint;
        auto [x,y]=grid.checkpoint_points[size_t(state.checkpoint)];
        state.spawn_x=x;state.spawn_y=y;state.player.x=x;state.player.y=y;
      }
      state.elapsed=profile.room_seconds;state.deaths=profile.room_deaths;
    }
    if(resume&&mode==iceberg::Mode::Campaign&&profile.revisit.active&&profile.hidden_room<0) {
      const auto &r=profile.revisit;grid.switch_flags=r.switches;
      for(size_t i=0;i<state.collected.size()&&i<64;++i)state.collected[i]=(r.collected>>i)&1;
      if(size_t(r.checkpoint)<grid.checkpoint_points.size()) {
        state.checkpoint=r.checkpoint;auto [x,y]=grid.checkpoint_points[size_t(r.checkpoint)];
        state.spawn_x=x;state.spawn_y=y;state.player.x=x;state.player.y=y;
      }
      state.elapsed=r.seconds;state.deaths=r.deaths;
    }
    if(resume && mode!=iceberg::Mode::Campaign && run_active) {
      const auto &e=profile.expeditions[mode==iceberg::Mode::Endless?0:1];
      grid.switch_flags=e.switches;
      for(size_t i=0;i<state.collected.size()&&i<64;++i)state.collected[i]=(e.collected>>i)&1;
      if(size_t(e.checkpoint)<grid.checkpoint_points.size()) {
        state.checkpoint=e.checkpoint;auto [x,y]=grid.checkpoint_points[size_t(e.checkpoint)];
        state.spawn_x=x;state.spawn_y=y;state.player.x=x;state.player.y=y;
      }
      state.elapsed=e.room_seconds;state.deaths=e.room_deaths;
    }
    stepper={};sync_view();
    update_camera(state,grid,110,27,2,10);
  }
  void talk(std::vector<iceberg::DialogueLine> lines,int action=0) {
    state.adventure.dialogue={std::move(lines),0,action};stepper={};
    state.player.vx=0;audio.play(AudioCue::Voice);
  }
  void dialogue_next() {
    auto &dialogue=state.adventure.dialogue;
    if(!dialogue.active())return;
    ++dialogue.page;
    if(dialogue.active()){audio.play(AudioCue::Paper);return;}
    int action=dialogue.action;
    dialogue={};
    if(action==1) {
      profile.story|=uint64_t(1)<<district;
    } else if(action==2) {
      complete_district();
    } else if(action>=100 && action<108) {
      int quest=action-100;
      profile.quests[size_t(quest)]=profile.quest_counts[size_t(quest)]>=unsigned(iceberg::Quests[size_t(quest)].required)?2:1;
    } else if(action>=200 && action<208) {
      int quest=action-200;profile.quests[size_t(quest)]=3;
      profile.cosmetics|=1u<<(quest%7+1);
      notice(t("ZADANIE UKOŃCZONE — nagroda w dzienniku","TASK COMPLETE — reward in journal"));
      audio.play(AudioCue::Win);
    }
    persist();sync_view();
  }
  static void merge_record(iceberg::Record &record,double seconds,int deaths,int treasures) {
    if(!record.complete || seconds<record.best_seconds)record.best_seconds=seconds;
    if(record.best_deaths<0 || deaths<record.best_deaths)record.best_deaths=deaths;
    record.best_treasures=std::max(record.best_treasures,treasures);record.complete=true;
  }
  void complete_district() {
    profile.interlude=false;
    merge_record(profile.records[size_t(district)],profile.district_seconds,
      profile.district_deaths,profile.district_treasures);
    profile.story|=uint64_t(1)<<(district+8);
    profile.district_seconds=0;profile.district_deaths=profile.district_treasures=0;
    profile.itinerary.clear();
    if(district+1<iceberg::AuthoredDistrictCount) {
      ++district;profile.unlocked=std::max(profile.unlocked,district);
    } else if(district+1==iceberg::DistrictCount){profile.finished=true;profile.cosmetics|=1u<<7;}
    else profile.at_content_boundary=true;
    stage=0;room=0;clear_room_record();load_room();persist();
  }
  void begin_campaign(bool fresh) {
    persist();
    if(fresh) {
      auto previous=profile;
      auto language=profile.language;
      float music=profile.music_volume,effects=profile.effects_volume,ambience=profile.ambience_volume;
      profile={};profile.language=language;
      profile.seed=options.seed_override||!previous.started?options.seed:iceberg::mix(previous.seed+1);
      profile.records=previous.records;profile.trials=previous.trials;profile.challenges=previous.challenges;
      profile.expeditions=previous.expeditions;profile.endless_best=previous.endless_best;
      profile.cosmetics=previous.cosmetics;profile.outfit=previous.outfit;
      profile.music_volume=music;profile.effects_volume=effects;profile.ambience_volume=ambience;
    }
    run_active=false;
    profile.started=true;mode=iceberg::Mode::Campaign;
    district=profile.district;stage=profile.stage;room=profile.room;
    if(profile.at_content_boundary&&district+1<iceberg::AuthoredDistrictCount) {
      profile.at_content_boundary=false;++district;profile.unlocked=std::max(profile.unlocked,district);
      stage=room=0;profile.itinerary.clear();clear_room_record();
      profile.district_seconds=0;profile.district_deaths=profile.district_treasures=0;
    }
    load_room(true);state.menu=false;persist();
  }
  void begin_run(iceberg::Mode next,bool fresh=false) {
    if(next==iceberg::Mode::Trial&&profile.quests[6]!=3&&!(profile.cosmetics&(1u<<7))&&!profile.finished&&!profile.at_content_boundary) {
      notice(t("Porozmawiaj z Kubą i pokaż trzy skoki ze zrywu.","Talk to Kuba and demonstrate three dash jumps."),6);return;
    }
    persist();mode=next;run_district=std::min(run_district,expedition_unlocked());
    auto &e=profile.expeditions[next==iceberg::Mode::Endless?0:1];
    uint32_t requested_seed=options.seed_override?options.seed:iceberg::mix(0x54495241u+unsigned(run_district)*7919u);
    bool resume=e.active&&!fresh&&e.district==run_district&&
      (next==iceberg::Mode::Trial?e.seed==requested_seed:!options.seed_override||e.origin_seed==options.seed);
    if(resume) {
      district=e.district;room=e.room;depth=e.depth;run_seed=e.seed;
      run_seconds=e.seconds;run_deaths=e.deaths;run_treasures=e.treasures;
    } else {
      district=std::min(expedition_unlocked(),run_district);room=depth=0;
      run_seed=next==iceberg::Mode::Trial?requested_seed:options.seed_override?options.seed:iceberg::mix(run_seed+1);
      run_seconds=0;run_deaths=run_treasures=0;e={};e.origin_seed=run_seed;
    }
    stage=2;run_active=true;load_room(resume);state.menu=false;persist();
    notice(resume?t("WZNOWIONO WYPRAWĘ — ta sama droga","EXPEDITION RESUMED — the same route"):
      next==iceberg::Mode::Endless?t("WYPRAWA BEZ KOŃCA — każda brama to nowa droga","ENDLESS EXPEDITION — every gate is a new route"):
      t("PRÓBA MISTRZOWSKA — osiem sal, jeden rekord","MASTERY TRIAL — eight rooms, one record"));
  }
  void advance_room() {
    audio.play(AudioCue::Door);
    if(mode!=iceberg::Mode::Campaign) {
      ++room;++depth;if(mode==iceberg::Mode::Endless)profile.endless_best=std::max(profile.endless_best,depth);
      if(mode==iceberg::Mode::Trial && room>=iceberg::RoomsPerLeg) {
        if(options.seed_override||route_version()!=iceberg::ContentVersion) {
          auto it=std::find_if(profile.challenges.begin(),profile.challenges.end(),[&](const iceberg::SeedRecord &r){
            return r.seed==run_seed&&r.district==district&&r.version==route_version();});
          if(it==profile.challenges.end()) {
            if(profile.challenges.size()>=128)profile.challenges.erase(profile.challenges.begin());
            profile.challenges.push_back({run_seed,district,route_version(),{}});it=std::prev(profile.challenges.end());
          }
          merge_record(it->record,run_seconds,run_deaths,run_treasures);
        } else merge_record(profile.trials[size_t(district)],run_seconds,run_deaths,run_treasures);
        profile.cosmetics|=(1u<<(district+1))|(1u<<(16+district));
        run_active=false;profile.expeditions[1].active=false;
        persist();state.menu=true;state.won=false;room=0;
        notice(t("PRÓBA UKOŃCZONA — rekord zapisany","TRIAL COMPLETE — record saved"),6);
      } else {
        if(room>=iceberg::RoomsPerLeg){room=0;run_seed=iceberg::mix(run_seed+1);profile.expeditions[0].itinerary.clear();}
        load_room();persist();
      }
      return;
    }
    if(profile.hidden_room>=0) {
      profile.hidden_room=-1;load_room(true);persist();return;
    }
    if(profile.revisit.active) {
      bool jubilee=profile.revisit.scene==2&&state.won&&!(profile.story&(uint64_t(1)<<40));
      if(jubilee){profile.story|=uint64_t(1)<<40;profile.cosmetics|=1u<<3;}
      profile.revisit={};load_room(true);
      if(jubilee){talk(iceberg::jubilee_complete());audio.play(AudioCue::Win);}
      persist();return;
    }
    if(profile.visit_district>=0) {
      profile.visit_district=-1;load_room(true);persist();return;
    }
    if(grid.room_role==iceberg::RoomRole::Interlude) {
      int leg=stage==2?0:stage==4?1:2;
      profile.story|=uint64_t(1)<<(16+leg);profile.interlude=false;
      stage=stage==2?3:stage==4?5:7;room=0;clear_room_record();load_room();
      if(stage==3||stage==5)talk(iceberg::homecoming(district,stage==3?0:1));
      persist();return;
    }
    if(grid.room_role==iceberg::RoomRole::Settlement) {
      if(profile.at_content_boundary) {
        talk({{1,{"Dolne obiegi pracują. Droga wyżej poczeka na następną część podróży. Na dziś uratowaliśmy komuś mieszkanie. To nie jest mało.","The lower circuits are working. The route above awaits the next part of the journey. Today we saved somebody's home. That's no small thing."}},
          {0,{"Możesz wrócić do mieszkańców, dokończyć ich sprawy albo wybrać wyprawę i próbę mistrzowską w menu. Poczta poczeka. Tym razem naprawdę.","You can return to the residents, finish their tasks, or choose an expedition or mastery trial in the menu. The mail can wait. This time, really."}}});return;
      }
      if(profile.finished) {begin_run(iceberg::Mode::Endless);return;}
      if(!(profile.story&(uint64_t(1)<<district))) {
        talk(iceberg::introduction(district),1);return;
      }
      stage=stage==0?1:stage==3?4:6;room=0;
    } else if(stage==1) {stage=2;room=0;}
    else if(stage==7) {talk(iceberg::conclusion(district),2);return;}
    else if(++room>=iceberg::RoomsPerLeg) {
      int leg=stage==2?0:stage==4?1:2;
      if(district==0&&!(profile.story&(uint64_t(1)<<(16+leg)))) {
        room=iceberg::RoomsPerLeg-1;profile.interlude=true;clear_room_record();load_room();
        talk(iceberg::mission_arrival(leg));persist();return;
      }
      room=0;stage=stage==2?3:stage==4?5:7;
    }
    clear_room_record();load_room();
    if(stage==3||stage==5)talk(iceberg::homecoming(district,stage==3?0:1));
    persist();
  }
  int near_interaction() const {
    auto p=state.player.rect();p.x-=1.6;p.y-=1.;p.w+=3.2;p.h+=2.;
    for(size_t i=0;i<grid.interactions.size();++i)
      if(p.intersects(grid.interactions[i].area))return int(i);
    return -1;
  }
  void animate_pickup(const Interaction &node,int kind) {
    auto &v=state.adventure;v.pickup_left=.65;v.pickup_x=node.area.x;
    v.pickup_y=node.area.bottom()-1.3;v.pickup_kind=kind;
    if(kind==0){audio.play(AudioCue::Paper);audio.play(AudioCue::Cloth);}
  }
  void inspect_nearby() {
    auto &v=state.adventure;
    v.inspect_page=0;v.inspect_detail=0;
    using Art=iceberg::InspectionArt;
    auto describe=[&](std::string title,std::string body,const char *icon,Art art) {
      v.inspect_title=std::move(title);v.inspect_body=std::move(body);v.inspect_icon=icon;v.inspect_art=art;
      v.inspecting=true;stepper={};audio.play(AudioCue::Paper);
    };
    int at=near_interaction();
    if(at<0) {
      auto close=[&](double x,double y){return std::hypot(state.player.x+.45-x,state.player.y+.6-y)<2.8;};
      for(size_t i=0;i<grid.sweets.size();++i)if(!state.collected[i]&&close(grid.sweets[i].first,grid.sweets[i].second)) {
        describe(t("Żeton transportowy","Transit token"),t("Mosiężny krążek z pieczęcią poczty. Znajdźki liczą się osobno od czasu i powrotów — możesz szukać ich spokojnie albo zbierać podczas próby.","A brass token stamped by the postal service. Collection records are separate from time and retries — explore for them at your own pace or collect them during a trial."),"◉",Art::Coin);return;
      }
      for(size_t i=0;i<grid.refills.size();++i)if(close(grid.refills[i].first,grid.refills[i].second)) {
        describe(t("Ogniwo zrywu","Dash cell"),t("Szklana osłona chroni naładowany rdzeń. Dotknij go, żeby odzyskać zryw w powietrzu. Puste ogniwo odnowi się po chwili.","A glass casing protects a charged core. Touch it to restore an airborne dash. An empty cell recharges after a moment."),"▥",Art::Refill);return;
      }
      for(auto [x,y]:grid.checkpoint_points)if(close(x,y)) {
        describe(t("Lampa powrotna","Return lamp"),t("Światło zapamiętuje bezpieczne miejsce na trasie. Po upadku wrócisz tutaj. Listy, naprawy i sprawy mieszkańców zostają załatwione.","The light marks a safe place on your route. A fall returns you here. Collected letters, repairs and completed character tasks stay done."),"▣",Art::Lamp);return;
      }
    }
    if(at<0||grid.interactions[size_t(at)].kind==InteractionKind::Secret) {
      notice(t("Podejdź do przedmiotu lub postaci, żeby obejrzeć z bliska.","Approach an object or character to inspect it."));return;
    }
    const auto &node=grid.interactions[size_t(at)];
    v.inspect_title=node.label.get(profile.language);v.inspect_icon="▤";v.inspect_art=Art::Door;v.inspect_detail=node.id;
    switch(node.kind) {
    case InteractionKind::Npc:case InteractionKind::Rescue:
      v.inspect_art=Art::Character;v.inspect_title=iceberg::Npcs[size_t(node.id)].name;
      v.inspect_body=iceberg::Npcs[size_t(node.id)].title.get(profile.language)+t(". E — rozmowa. Zlecenia i odebrane wiadomości znajdziesz w dzienniku.",". E — talk. Tasks and received messages are kept in your journal.");v.inspect_icon="◉";break;
    case InteractionKind::Carry:
      v.inspect_art=Art::Thermometer;v.inspect_title=t("Termometr transportowy","Transit thermometer");v.inspect_icon="▣";
      v.inspect_body=t("Szklana rurka jest zawieszona w skrzynce na dwóch sprężynach. Strzałka na wieku wskazuje górę. Możesz biec i skakać, ale zryw uszkodziłby pomiar. Po odebraniu przesyłki zryw wróci.","A glass tube hangs inside the case on two springs. The lid's arrow points up. Running and jumping are safe, but a dash would spoil the reading. Dashing returns after delivery.");break;
    case InteractionKind::Task:
      static const Art task_art[]={Art::Parcel,Art::Valve,Art::Signal,Art::Address,Art::File,Art::Seeds,Art::Character,Art::Speaker};
      v.inspect_art=task_art[std::clamp(node.id,0,7)];v.inspect_title=iceberg::Quests[size_t(node.id)].title.get(profile.language);
      v.inspect_icon=node.id==1?"⊗":node.id==7?"▤":"▣";
      v.inspect_body=node.id==1?t("Mosiężne koło jest ciepłe. Biała kreska na rurze oznacza położenie otwarte. Przekręć zawór i zgłoś Witkowi przywrócony obieg.","The brass wheel is warm. A white mark on the pipe indicates the open position. Turn the valve and report the restored circuit to Witek."):
        node.id==7?t("Pod siatką ktoś wsunął kartkę z częstotliwością. Głośnik wciąż ma zasilanie; trzeba dostroić odbiornik, nie uderzać w obudowę.","Someone slipped a frequency note beneath the grille. The speaker still has power; its receiver needs tuning, not a hit on the casing."):
        t("Papier spuchł od wilgoci, ale sznurek trzyma. Pod pieczątką widać adres. Zabierz przesyłkę; Helena odnajdzie odbiorcę.","The paper has swollen with moisture, but the string holds. An address is visible beneath the stamp. Take the parcel; Helena can find its recipient.");
      if(node.id==2)v.inspect_body=t("Latarnia sygnałowa tramwaju. Dolne światło wskazuje gotowy odcinek. Przywrócenie zasilania otworzy Bogdanowi drogę między przystankami.","A tram signal. Its lower light marks a ready section. Restoring power gives Bogdan a route between the stops.");
      if(node.id==3)v.inspect_body=t("Emaliowana tabliczka jest starsza niż drzwi. Numer można odczytać pod nową warstwą farby. Grażyna prosiła o porównanie trzech takich adresów.","The enamel plaque is older than the door. Its number shows through a newer coat of paint. Grażyna asked you to compare three of these addresses.");
      if(node.id==4)v.inspect_body=t("Związana teczka pęcznieje od wilgoci. Na brzegu widać ślady po usuniętych kartach. Irena potrzebuje całego pliku, razem z opisem na okładce.","The tied file is swollen with damp. Its edge shows where pages were removed. Irena needs the entire file, including the cover label.");
      if(node.id==5)v.inspect_body=t("Nasiona przechowano w szklanej kapsule. W środku jest jeszcze sucho. Celina znajdzie dla nich miejsce na dole.","Seeds are stored in a glass capsule. The inside is still dry. Celina will find a place for them downstairs.");
      if(profile.task_items[size_t(node.detail/64)]&(uint64_t(1)<<(node.detail%64)))v.inspect_body+=t("\nJuż załatwione.","\nAlready handled.");
      break;
    case InteractionKind::Valve:
      v.inspect_art=Art::Valve;v.inspect_icon="⊗";v.inspect_body=t("Korba steruje pobliską maszyną. Ciemne koło jest zamknięte; jasny środek oznacza otwarty obieg. Stan mechanizmu pozostaje zapisany po powrocie do lampy.","The crank controls the nearby machinery. A dark wheel is closed; a bright centre marks an open circuit. The mechanism stays set after a checkpoint retry.");break;
    case InteractionKind::Note:
      v.inspect_art=node.id==30?Art::Dedication:node.id<10?Art::Sign:Art::File;v.inspect_title=node.id==30?t("Akt fundacyjny","Dedication"):t("Tablica informacyjna","Information sign");
      if(node.id==30){auto lines=iceberg::jubilee_dedication();v.inspect_body=lines[0].text.get(profile.language)+" "+lines[1].text.get(profile.language);}
      else if(node.id<10)v.inspect_body=iceberg::training_sign(node.id).get(profile.language);
      else v.inspect_body=t("Kartka zachowała się za osłoną. Możesz odczytać ją w całości, naciskając E.","The sheet survived behind its cover. Press E to read the complete record.");
      break;
    case InteractionKind::Deliver:
      v.inspect_art=Art::Delivery;v.inspect_body=t("Gniazdo pomiarowe przyjmuje skrzynkę z termometrem. Zatrzaski utrzymują ją pionowo podczas odczytu.","The measuring socket accepts the thermometer case. Its catches keep the case upright during the reading.");break;
    case InteractionKind::Board:
      v.inspect_art=Art::Board;v.inspect_body=iceberg::route_title(district,stage).get(profile.language);break;
    default:v.inspect_body=t("E — użyj. Możesz zamknąć ten widok bez wykonywania działania.","E — use. You can close this view without performing the action.");break;
    }
    v.inspecting=true;stepper={};audio.play(AudioCue::Paper);
  }
  void interact() {
    int at=near_interaction();if(at<0)return;
    const auto node=grid.interactions[size_t(at)];
    if(node.kind==InteractionKind::Npc) {
      if(mode==iceberg::Mode::Campaign && profile.visit_district<0 && stage==0 && !(profile.story&(uint64_t(1)<<district))) {
        talk(iceberg::introduction(district),1);return;
      }
      int quest=-1;
      for(size_t i=0;i<iceberg::Quests.size();++i)if(iceberg::Quests[i].owner==node.id){quest=int(i);break;}
      if(quest>=0 && profile.quests[size_t(quest)]==2)
        talk(iceberg::task_thanks(quest),200+quest);
      else talk(iceberg::small_talk(node.id,quest<0?0:profile.quests[size_t(quest)]),quest>=0&&profile.quests[size_t(quest)]==0?100+quest:0);
    } else if(node.kind==InteractionKind::Rescue) {
      if(!(grid.switch_flags&1)){talk({{2,{"Najpierw wyciąg. Czerwona korba, dwa pomosty wyżej. To ten element, który wygląda, jakby od dawna chciał się urwać.","The winch first. Red crank, two landings above. The bit that looks as though it's wanted to fall off for years."}}});return;}
      if(!(grid.switch_flags&2)){grid.switch_flags|=2;talk(iceberg::rescued_conductor());persist();}
    } else if(node.kind==InteractionKind::Carry) {
      if(!(grid.switch_flags&1)) {
        animate_pickup(node,0);grid.switch_flags|=1;state.player.dash_time_left=state.player.dash_buffer_left=state.player.momentum_left=0;
        state.player.vx=clamp(state.player.vx,-kMaxRunSpeed,kMaxRunSpeed);
        notice(t("TERMOMETR: skacz ostrożnie. Zryw wróci po dostawie.","THERMOMETER: jump carefully. Dashing returns after delivery."),6);audio.play(AudioCue::Pickup);persist();
      }
    } else if(node.kind==InteractionKind::Deliver) {
      if(!(grid.switch_flags&1)){notice(t("Najpierw zabierz skrzynkę przy wejściu.","First collect the box by the entrance."),5);return;}
      if(!(grid.switch_flags&2)){grid.switch_flags|=2;talk(iceberg::parcel_delivered());audio.play(AudioCue::Win);persist();}
    } else if(node.kind==InteractionKind::Replay) {
      audio.play(AudioCue::Door);persist();profile.revisit={};profile.revisit.active=true;
      profile.revisit.district=grid.district;profile.revisit.scene=node.id;
      load_room();if(node.id==2&&!(profile.story&(uint64_t(1)<<40)))talk(iceberg::jubilee_arrival());persist();return;
    } else if(node.kind==InteractionKind::Shortcut) {
      room=std::min(iceberg::RoomsPerLeg-1,room+3);clear_room_record();load_room();persist();
      notice(t("WITEK: Uważaj na głowę. To nie był projekt dla ludzi.","WITEK: Mind your head. This wasn't designed for people."),5);return;
    } else if(node.kind==InteractionKind::Return) {
      advance_room();return;
    } else if(node.kind==InteractionKind::Secret) {
      if(mode!=iceberg::Mode::Campaign)return;
      if(node.id>0 && !(profile.discoveries&(1u<<(node.id-1))))return;
      persist();profile.hidden_room=node.id;load_room();persist();return;
    } else if(node.kind==InteractionKind::Board) {
      advance_room();
    } else if(node.kind==InteractionKind::Valve) {
      if(grid.template_id==303&&node.id>0&&!(grid.switch_flags&(uint64_t(1)<<(node.id-1)))) {
        notice(t("Najpierw odłącz wcześniejszy obieg. Strzałki prowadzą od fontanny do mieszkań.","Disconnect the preceding circuit first. Follow the arrows from fountain to homes."),5);return;
      }
      if(!(grid.switch_flags&(uint64_t(1)<<node.id)))animate_pickup(node,1);
      grid.switch_flags|=uint64_t(1)<<node.id;
      audio.play(AudioCue::Valve);notice(t("OBIEG OTWARTY","CIRCUIT OPEN"));persist();
    } else if(node.kind==InteractionKind::Task) {
      int bit=node.detail;auto mask=uint64_t(1)<<(bit%64);
      if(!(profile.task_items[size_t(bit/64)]&mask)) {
        animate_pickup(node,node.id);
        profile.task_items[size_t(bit/64)]|=mask;
        ++profile.quest_counts[size_t(node.id)];
        if(profile.quests[size_t(node.id)]==1 && profile.quest_counts[size_t(node.id)]>=unsigned(iceberg::Quests[size_t(node.id)].required))profile.quests[size_t(node.id)]=2;
        notice(iceberg::Quests[size_t(node.id)].title.get(profile.language)+"  "+std::to_string(profile.quest_counts[size_t(node.id)])+"/3");
        audio.play(node.id==1||node.id==2?AudioCue::Valve:node.id==7?AudioCue::Voice:AudioCue::Pickup);persist();
      } else notice(t("Już zabrane. Wróć do zleceniodawcy.","Already collected. Return to the task giver."));
    } else if(node.kind==InteractionKind::Note) {
      if(node.id==30){talk(iceberg::jubilee_dedication());return;}
      if(node.id>=10) {
        profile.discoveries|=1u<<(node.id-10);
        talk(iceberg::hidden_testimony(node.id-10));persist();return;
      }
      talk({{6,iceberg::training_sign(node.id)}});
    }
    sync_view();
  }
  void menu(Controls c) {
    auto &v=state.adventure;
    if(v.settings) {
      float *volume=c.select_level==1?&profile.music_volume:
        c.select_level==2?&profile.effects_volume:c.select_level==3?&profile.ambience_volume:nullptr;
      if(volume){*volume=std::round((*volume+.1f)*10)/10;if(*volume>1)*volume=0;persist();}
      if(c.select_level==4) {
        for(int i=0;i<8;++i){profile.outfit=(profile.outfit+1)%8;if(profile.outfit==0||(profile.cosmetics&(1u<<profile.outfit)))break;}
        persist();
      }
      if(c.language){profile.language=profile.language==iceberg::Language::Polish?iceberg::Language::English:iceberg::Language::Polish;persist();}
      if(c.confirm||c.pause){v.settings=false;v.selected=4;}
      return;
    }
    if(v.confirm_new) {
      if(c.confirm){v.confirm_new=false;begin_campaign(true);}
      if(c.pause||c.menu_request)v.confirm_new=false;
      return;
    }
    int old_selected=v.selected;
    if(c.map && (v.selected==2||v.selected==3))run_district=(run_district+1)%(expedition_unlocked()+1);
    if(c.navigate)v.selected=(v.selected+c.navigate+6)%6;
    if(c.select_level>=1&&c.select_level<=6)v.selected=c.select_level-1;
    if(v.selected!=old_selected){choose_saved_run_district(v.selected);audio.play(AudioCue::Menu);}
    if(c.language){profile.language=profile.language==iceberg::Language::Polish?iceberg::Language::English:iceberg::Language::Polish;persist();}
    if((c.restart||c.new_run) && (v.selected==2||v.selected==3)){begin_run(v.selected==2?iceberg::Mode::Endless:iceberg::Mode::Trial,true);return;}
    if(!c.confirm)return;
    int selected=v.selected;
    if(selected==0)begin_campaign(false);
    if(selected==1){if(profile.started)v.confirm_new=true;else begin_campaign(true);}
    if(selected==2)begin_run(iceberg::Mode::Endless);
    if(selected==3)begin_run(iceberg::Mode::Trial);
    if(selected==4){v.settings=!v.settings;}
    if(selected==5){persist();legacy_requested=true;running=false;}
  }
  void tick(Controls c,double dt,bool focused=true,int columns=110,int rows=34,double aspect=2) {
    dt=clamp(dt,0.,.1);
    if(columns!=last_columns||rows!=last_rows||aspect!=last_aspect) {
      update_camera(state,grid,columns,std::max(1,rows-7),aspect,10);
      last_columns=columns;last_rows=rows;last_aspect=aspect;
    }
    struct SendFrame {
      AdventureSession &session;double dt;bool focused;int columns,rows;
      ~SendFrame(){session.network_frame(dt,focused,columns,rows);}
    } send_frame{*this,dt,focused,columns,rows};
    audio.set_mix(profile.music_volume,profile.effects_volume,profile.ambience_volume);
    audio.set_environment(grid.district,grid.room_role==iceberg::RoomRole::Secret);
    if(c.quit){persist();running=false;return;}
    if(c.mute_audio)audio.muted=!audio.muted;
    if(c.language&&!state.menu){profile.language=profile.language==iceberg::Language::Polish?iceberg::Language::English:iceberg::Language::Polish;persist();}
    if(!focused && previous_focus){state.paused=true;stepper={};}
    previous_focus=focused;
    if(!focused)c={};
    auto &v=state.adventure;
    v.notice_left=std::max(0.,v.notice_left-dt);
    if(state.menu) {menu(c);sync_view();audio.update(AudioScene::Menu,2,dt,focused);return;}
    if(c.menu_request){persist();state.menu=true;v.inspecting=false;v.dialogue={};stepper={};sync_view();return;}
    if(v.inspecting) {
      auto layout=iceberg::inspection_layout(v.inspect_title,v.inspect_body,columns,rows);
      v.inspect_page=std::clamp(v.inspect_page,0,layout.pages-1);
      if(c.inspect||c.pause)v.inspecting=false;
      else if(c.confirm||c.interact){if(v.inspect_page+1<layout.pages)++v.inspect_page;else v.inspecting=false;}
      else if(c.navigate)v.inspect_page=std::clamp(v.inspect_page+c.navigate,0,layout.pages-1);
      audio.update(AudioScene::Paused,music_track(),dt,focused);sync_view();return;
    }
    if(c.inspect&&!v.dialogue.active()&&!state.paused&&!state.journal&&!v.map) {
      inspect_nearby();sync_view();if(v.inspecting)return;
    }
    if(v.dialogue.active()) {
      if(c.confirm||c.interact||c.jump_pressed)dialogue_next();
      audio.update(AudioScene::Paused,music_track(),dt,focused);sync_view();return;
    }
    if(c.journal){state.journal=!state.journal;v.map=false;stepper={};}
    if(c.map){v.map=!v.map;state.journal=false;stepper={};}
    if(c.pause){if(state.journal||v.map){state.journal=false;v.map=false;}else state.paused=!state.paused;stepper={};}
    if(state.paused&&c.confirm)state.paused=false;
    if(state.journal||v.map) {
      if(state.journal&&c.interact&&profile.quests[size_t(std::clamp(v.selected,0,7))]==3) {
        talk(iceberg::task_record(v.selected));sync_view();return;
      }
      if(c.navigate){int n=v.map?iceberg::DistrictCount:8;v.selected=(v.selected+(v.map?-c.navigate:c.navigate)+n)%n;}
      if(c.confirm && v.map && mode==iceberg::Mode::Campaign && profile.hidden_room<0 &&
          !profile.revisit.active && v.selected<=profile.unlocked && (v.selected!=district||profile.visit_district>=0) && ((profile.story&(uint64_t(1)<<8))||profile.quests[2]==3||profile.finished||profile.at_content_boundary)) {
        persist();profile.visit_district=v.selected==district?-1:v.selected;load_room(profile.visit_district<0);persist();
      } else if(c.confirm){state.journal=false;v.map=false;}
    }
    bool blocked=state.paused||state.journal||v.map||columns<34||rows<(columns>=56?20:24);
    audio.update(blocked?AudioScene::Paused:AudioScene::Play,music_track(),dt,focused);
    if(blocked){sync_view();return;}
    v.pickup_left=std::max(0.,v.pickup_left-dt);
    if(c.dash_pressed&&grid.dash_locked())notice(t("Przesyłka musi stać pionowo — skocz zamiast zrywu.","Keep the parcel upright — jump instead of dashing."));
    if(c.interact){interact();if(v.dialogue.active()){sync_view();return;}}
    if(state.won) {
      if(c.restart||c.new_run){state.won=false;stepper={};}
      else {if(c.confirm||c.interact)advance_room();sync_view();return;}
    }
    if(c.new_run){c.restart=true;}
    Player before=state.player;
    int old_deaths=state.deaths,old_pickups=state.sweets_count(),old_cp=state.checkpoint;
    double old_elapsed=state.elapsed;
    stepper.advance(state,grid,c,dt);
    feedback_step(state,grid,before,old_deaths,old_pickups,old_cp,false,dt,audio);
    // Spatially restrained machinery cues follow actual motion/timing.
    if(state.elapsed>old_elapsed&&state.deaths==old_deaths) {
      if(state.player.support>=0&&before.support!=state.player.support)audio.play(AudioCue::Lift,.7f);
      if(before.support>=0&&state.player.support<0&&state.player.vy<-10)audio.play(AudioCue::Lift,.45f);
      for(const auto &vent:grid.pulses)if(vent.active(state.elapsed)&&!vent.active(old_elapsed)) {
        double distance=std::hypot(vent.area.x-state.player.x,vent.area.y-state.player.y);
        if(distance<22)audio.play(AudioCue::Steam,float(1-distance/22));
      }
    }
    if(state.sweets_count()>old_pickups&&v.pickup_left<=0) {
      v.pickup_kind=8;v.pickup_left=.65;v.pickup_x=state.player.x+.4;v.pickup_y=state.player.y-.8;
    }
    double active=state.elapsed-old_elapsed;
    int deaths=state.deaths-old_deaths,pickups=state.sweets_count()-old_pickups;
    if(mode==iceberg::Mode::Campaign) {
      if(!profile.revisit.active) {
      profile.campaign_seconds+=active;profile.district_seconds+=active;
      profile.campaign_deaths+=deaths;profile.district_deaths+=deaths;
      profile.district_treasures+=pickups;
      }
    } else {run_seconds+=active;run_deaths+=deaths;run_treasures+=pickups;}
    if(state.player.wall_bounce_left>before.wall_bounce_left)audio.play(AudioCue::Bounce);
    if(state.player.dash_jump_left>before.dash_jump_left && profile.quest_counts[6]<3) {
      ++profile.quest_counts[6];
      if(profile.quest_counts[6]>=3 && profile.quests[6]==1)profile.quests[6]=2;
    }
    int at=near_interaction();
    v.prompt=at>=0&&grid.interactions[size_t(at)].kind!=InteractionKind::Secret?"[E] "+grid.interactions[size_t(at)].label.get(profile.language):"";
    if(at>=0&&grid.interactions[size_t(at)].kind==InteractionKind::Npc)v.prompt+=" — "+std::string(iceberg::Npcs[size_t(grid.interactions[size_t(at)].id)].name);
    autosave_left-=dt;
    if(old_cp!=state.checkpoint||state.won||autosave_left<=0)persist();
    update_camera(state,grid,columns,std::max(1,rows-7),aspect,dt);
    sync_view();
  }
};
