#pragma once
// Session owns progression and interaction, not a frontend. SDL and TUI feed
// identical Controls and read the same GameState and rendered cell frame.
class AdventureSession {
public:
  iceberg::Profile profile;
  iceberg::SaveStore saves;
  iceberg::SessionOptions options;
  std::filesystem::path save_base;
  int save_slot=1;
  bool profile_action_busy=false;
  std::optional<iceberg::Profile> before_tour;
  int tour_scene=-1;
  EngineGrid grid;
  GameState state;
  FixedStepper stepper;
  GameAudio audio;
  ghosts::Client network;
  uint32_t network_epoch=0,previous_course=0;
  iceberg::Random dock_chatter;
  int last_dock_line=-1;
  int network_deaths=0;
  std::vector<iceberg::Achievement> achievement_queue;
  int last_columns=110,last_rows=34;
  double last_aspect=2;
  iceberg::Mode mode = iceberg::Mode::Campaign;
  bool running = true, legacy_requested = false;
  int district = 0, stage = 0, room = 0, depth = 0, run_district = 0;
  uint32_t run_seed = 1;
  double autosave_left = 15, run_seconds = 0;
  uint32_t save_revision = 0, room_revision = 0;
  int run_deaths = 0, run_treasures = 0;
  int navigation_repeat_direction = 0;
  double navigation_repeat_left = 0;
  bool previous_focus = true;
  bool run_active = false;

  explicit AdventureSession(const iceberg::SessionOptions &o,
      const std::filesystem::path &save_file = iceberg::SaveStore::default_file())
    : saves(iceberg::SaveStore::slot_file(save_file,o.save_slot)), options(o), save_base(save_file),
      save_slot(o.save_slot), grid(iceworld::build_settlement(0)), dock_chatter{o.seed^0xa75d834bu},
      mode(o.mode), run_seed(o.seed) {
    saves.load(profile);
    if(o.language_override)profile.language=o.language;
    audio.init(audio_directory(), o.mute || (o.remote && !o.remote_audio));
    reset_profile_menu();
    if(!o.ghost_host.empty()) {
      try {network.connect_to(o.ghost_host,o.ghost_port);}
      catch(const std::exception &e){notice(e.what(),10);}
    }
  }
  void reset_profile_menu() {
    achievement_queue.clear();
    mode=options.mode;run_active=false;run_seconds=0;run_deaths=run_treasures=depth=0;
    state={};
    if(profile.content_version!=iceberg::ContentVersion) {
      for(int d=0;d<iceberg::DistrictCount;++d)if(profile.trials[size_t(d)].complete) {
        if(profile.challenges.size()<128)profile.challenges.push_back({default_trial_seed(d),d,profile.content_version,profile.trials[size_t(d)]});
        profile.trials[size_t(d)]={};
      }
    }
    profile.content_version=iceberg::ContentVersion;run_district=std::min(options.district,expedition_unlocked());
    if (!profile.started) profile.seed = options.seed;
    district = profile.district; stage = profile.stage; room = profile.room;
    run_seed = profile.seed;
    if (mode != iceberg::Mode::Campaign) {
      district = run_district; stage = 2; room = 0; run_seed = options.seed;
    }
    load_room(true);
    state.menu = true;
    select_resume_option();
    if(saves.recovered)notice(t("Przywrócono ostatni zdrowy zapis.","Recovered the last healthy save."),8);
    if(saves.protected_future)notice(t("Ten zapis wymaga nowszej wersji gry. Nie zostanie nadpisany.","This save needs a newer game version. It will not be overwritten."),12);
    sync_view();
  }
  ~AdventureSession() = default;
  std::string t(const char *pl,const char *en) const { return iceberg::tr(profile.language,pl,en); }
  void discard_pending_movement() {
    stepper={};
    state.player.jump_buffer_left=state.player.dash_buffer_left=0.;
  }
  unsigned route_version() const {
    return mode==iceberg::Mode::Campaign?profile.layout_version:profile.expeditions[mode==iceberg::Mode::Endless?0:1].layout_version;
  }
  int music_track() const {
    if(grid.template_id==iceberg::CrownSentinel)return 3;
    // The short snow theme belongs to inhabited resting places. Long route
    // pieces keep an expedition from repeating a 25-second musical phrase.
    if(grid.room_role==iceberg::RoomRole::Settlement)return 2;
    if(grid.room_role==iceberg::RoomRole::Interlude)return 1;
    return grid.district==0?0:1;
  }
  void notice(const std::string &text, double seconds=3, bool entry_notice=false) {
    state.adventure.notice=text;state.adventure.notice_left=seconds;
    state.adventure.entry_notice=entry_notice;
  }
  int expedition_unlocked() const {
    int available=profile.unlocked;
    for(int d=0;d<iceberg::AuthoredDistrictCount;++d)if(profile.records[size_t(d)].complete)available=std::max(available,d+1);
    return std::min(iceberg::AuthoredDistrictCount-1,available);
  }
  static uint32_t default_trial_seed(int area) {
    return iceberg::mix(0x54495241u+unsigned(area)*7919u);
  }
  bool compatible_seed(iceberg::Mode target,const iceberg::Expedition &saved) const {
    return !options.seed_override||(target==iceberg::Mode::Trial?saved.seed:saved.origin_seed)==options.seed;
  }
  bool can_resume_run(iceberg::Mode target,int area) const {
    const auto &saved=profile.expeditions[target==iceberg::Mode::Endless?0:1];
    return saved.active&&saved.district==area&&compatible_seed(target,saved);
  }
  bool separate_trial_record(int area,uint32_t seed,unsigned version) const {
    return options.seed_override||seed!=default_trial_seed(area)||version!=iceberg::ContentVersion;
  }
  const iceberg::Record *seeded_trial_record(int area,uint32_t seed,unsigned version) const {
    const auto record=std::find_if(profile.challenges.begin(),profile.challenges.end(),[&](const auto &entry){
      return entry.district==area&&entry.seed==seed&&entry.version==version;
    });
    return record==profile.challenges.end()?nullptr:&record->record;
  }
  bool can_retry_trial(int area) const {
    const auto &saved=profile.expeditions[1];
    if(area<0||area>=iceberg::AuthoredDistrictCount||saved.active||saved.district!=area||
       saved.room!=iceberg::RoomsPerLeg-1||
       saved.depth!=saved.room||saved.itinerary.size()!=iceberg::RoomsPerLeg||
       !compatible_seed(iceberg::Mode::Trial,saved))return false;
    if(!separate_trial_record(area,saved.seed,saved.layout_version)&&profile.trials[size_t(area)].complete)
      return true;
    const auto *record=seeded_trial_record(area,saved.seed,saved.layout_version);
    return record&&record->complete;
  }
  int selected_run_area(iceberg::Mode target,bool fresh) const {
    if(!fresh&&(can_resume_run(target,run_district)||
       (target==iceberg::Mode::Trial&&can_retry_trial(run_district))))return run_district;
    return std::min(run_district,expedition_unlocked());
  }
  void choose_saved_run_district(int selected) {
    if(options.district_override||selected<2||selected>3)return;
    const auto &e=profile.expeditions[size_t(selected-2)];
    run_district=e.active||(selected==3&&can_retry_trial(e.district))?
      e.district:std::min(run_district,expedition_unlocked());
  }
  void select_resume_option() {
    auto &selected=state.adventure.selected;
    selected=mode==iceberg::Mode::Endless?2:mode==iceberg::Mode::Trial?3:0;
    choose_saved_run_district(selected);
  }
#include "iceberg_route_runtime.inc"
  void sync_view() {
    auto &v=state.adventure;
    v.save_slot=save_slot;v.tour_scene=tour_scene;
    v.language=profile.language;v.mode=mode;v.district=grid.district;v.stage=stage;v.room=room;v.depth=depth;
    v.story=profile.story;v.journey_district=district;v.can_travel=(profile.story&(uint64_t(1)<<8))||profile.quests[2]==3||profile.finished||profile.at_content_boundary;
    v.unlocked=profile.unlocked;v.finished=profile.finished;v.at_content_boundary=profile.at_content_boundary;v.can_continue=profile.started;
    v.endless_resume=can_resume_run(iceberg::Mode::Endless,run_district);
    v.trial_resume=can_resume_run(iceberg::Mode::Trial,run_district);
    v.trial_replay=can_retry_trial(run_district);
    v.revisit=profile.revisit.active;
    v.hidden_room=profile.hidden_room;v.visit_district=profile.visit_district;v.cosmetics=profile.cosmetics;
    v.run_district=run_district;v.endless_best=profile.endless_best;v.seed=mode==iceberg::Mode::Campaign?profile.seed:run_seed;
    v.quest_focus=profile.quest_focus;
    v.outfit=profile.outfit;v.quests=profile.quests;v.quest_counts=profile.quest_counts;v.task_items=profile.task_items;
    v.records=profile.records;v.trials=profile.trials;
    const auto &e=profile.expeditions[1];
    for(int d=0;d<iceberg::DistrictCount;++d) {
      const bool saved=can_resume_run(iceberg::Mode::Trial,d)||can_retry_trial(d);
      const uint32_t wanted=saved?e.seed:options.seed_override?options.seed:default_trial_seed(d);
      const unsigned version=saved?e.layout_version:iceberg::ContentVersion;
      if(separate_trial_record(d,wanted,version)) {
        const auto *record=seeded_trial_record(d,wanted,version);
        v.trials[size_t(d)]=record?*record:iceberg::Record{};
      }
    }
    if(grid.template_id==330)for(auto &node:grid.interactions)if(node.kind==InteractionKind::Board)
      node.label=profile.finished?iceberg::Text{"Wyrusz na wyprawę","Start an expedition"}:iceberg::Text{"Zakończ obchód","Finish the round"};
    v.awards=iceberg::achievement_progress(profile);
    v.discoveries=profile.discoveries|uint32_t(v.awards[0]);
    v.switches=grid.switch_flags;v.room_name=(grid.room_role==iceberg::RoomRole::Settlement&&grid.template_id!=330?
      iceberg::Districts[size_t(grid.district)].subtitle:grid.room_name).get(profile.language);
    v.music_volume=profile.music_volume;v.effects_volume=profile.effects_volume;v.ambience_volume=profile.ambience_volume;
    v.pause_on_focus_loss=profile.pause_on_focus_loss;
    v.smooth_camera=profile.smooth_camera;
    state.audio_muted=audio.muted;state.audio_error=audio.error();
    if(grid.template_id==iceberg::CrownSentinel) {
      const int hits=state.crown.hits;
      if(hits<2)v.current_objective=t(
        "Gdy właz jest zamknięty, omiń oznaczone uderzenia. Po otwarciu zrób zryw przez rdzeń.",
        "While the hatch is closed, dodge the marked strikes. When it opens, dash through the core.");
      else if(hits==2)v.current_objective=t(
        "Ostatnie odsłonięcie jest wysoko. Skocz, a potem zrób zryw przez odsłonięty rdzeń.",
        "The last opening is high. Jump, then dash through the exposed core.");
      else if(tour_scene>=0)v.current_objective=t(
        "Strażnik pokonany. M wraca do menu.","The Sentinel is down. Press M to return to the menu.");
      else v.current_objective=t("Wysłuchaj zeznań przy latarni.","Hear the testimony at the beacon.");
    } else v.current_objective=iceworld::harbor_objective(grid.template_id,grid.switch_flags).get(profile.language);
    v.branch_at_door=at_route_fork();
    sync_route_view();
  }
  void network_frame(double dt,bool focused,int columns,int rows) {
    if(tour_scene>=0||!network.enabled())return;
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
    if(tour_scene>=0){autosave_left=15;return;}
    if (mode == iceberg::Mode::Campaign && profile.started && profile.hidden_room<0 && profile.visit_district<0 && !profile.revisit.active) {
      profile.district=district;profile.stage=stage;profile.room=room;
      profile.story&=~iceberg::ForkResumeMask;
      if(at_route_fork())profile.story|=iceberg::fork_resume_bits(route_leg());
      profile.checkpoint=state.checkpoint;
      if(grid.template_id==iceberg::CrownSentinel) {
        // Keep the film checkpoint when the main menu has cleared the dialogue.
        uint64_t checkpoint=profile.switches>>iceberg::CrownFilmShift;
        const auto &film=state.adventure.dialogue;
        if(state.crown.hits<3||(profile.story&iceberg::reply_recorded(6)))checkpoint=0;
        else if(film.cinematic&&film.active()&&film.reply_scene==6)
          checkpoint=film.choosing()?iceberg::CrownReplyCheckpoint:uint64_t(film.lines[film.page].shot);
        profile.switches=uint64_t(state.crown.hits)|(checkpoint<<iceberg::CrownFilmShift);
      } else profile.switches=grid.switch_flags;
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
    if(!(profile.cosmetics&iceberg::MasteryTransitPass)&&
       iceberg::mastery_token_progress(profile)==iceberg::MasteryTokenFare) {
      profile.cosmetics|=iceberg::MasteryTransitPass;
      if(!trial_unlocked_without_pass()) {
        notice(t("Zebrano żetony na wstęp do próby mistrzowskiej.","Transit tokens paid your entry to the Mastery Trial."),6);
        audio.play(AudioCue::Win);
      }
    }
    if(!(profile.cosmetics&iceberg::SurveyTransitPass)&&iceberg::survey_pass(profile)) {
      profile.cosmetics|=iceberg::SurveyTransitPass;
      notice(t("Żetony otworzyły objazd latarni przyboju w Dolnej Przystani.",
               "Transit tokens opened the Breakwater Survey in Lower Harbor."),6);
      audio.play(AudioCue::Win);
    }
    if(options.no_save){autosave_left=15;return;}
    state.adventure.save_error=!saves.save(profile);
    if(!state.adventure.save_error)++save_revision;
    autosave_left=15;
  }
  void clear_room_record() {
    profile.checkpoint=0;profile.switches=profile.collected=0;
    profile.room_seconds=0;profile.room_deaths=0;
  }
  EngineGrid campaign_settlement(int area) const {
    return area==0&&profile.layout_version>=4?iceworld::build_harbor_settlement_v4():iceworld::build_settlement(area);
  }
  EngineGrid campaign_authored(int area,bool finale) const {
    if(area==0&&!finale&&profile.layout_version>=4)return iceworld::build_harbor_lesson_v4();
    auto course=iceworld::build_authored(area,finale);
    if(area==1&&!finale&&profile.layout_version>=4) {
      // Relief valves under the high service galleries pulse out of phase.
      // Ride a plume, then steer sideways before the steam thins.
      course.air_fields.push_back({{56,33,3.5,19},0,-170,4.8,0});
      course.air_fields.push_back({{91,26,3.5,20},0,-170,4.8,.5});
    }
    return course;
  }
  static void apply_heatworks_mechanics(EngineGrid &room) {
    if(room.district!=1)return;
    add_expedition_steam(room);
    add_heatworks_cargo_hoists(room);
    add_heatworks_twin_pistons(room);
    add_heatworks_pressure_bypass(room);
    add_heatworks_turbine(room);
    add_heatworks_return_fan(room);
    add_heatworks_service_gallery(room);
    add_heatworks_pressure_hazards(room);
    add_heatworks_boiler_wall_fan(room);
    add_heatworks_service_bypass(room);
  }
  static void apply_estate_mechanics(EngineGrid &room) {
    if(room.district!=2)return;
    add_expedition_trams(room);
    add_expedition_stairwells(room);
    add_estate_clotheslines(room);
    add_estate_gutters(room);
    add_estate_awnings(room);
    add_estate_balconies(room);
    add_estate_masonry(room);
  }
  static void apply_harbor_mechanics(EngineGrid &room) {
    add_harbor_cargo_sling(room);
    add_harbor_tidal_surge(room);
  }
  static void apply_archive_mechanics(EngineGrid &room,bool expedition=false) {
    if(room.district!=3)return;
    if(expedition)add_expedition_drawers(room);
    add_archive_glass_catwalk(room);
    add_archive_stack_shuttles(room);
    add_expedition_post_launchers(room);
    add_expedition_copy_belts(room);
    add_expedition_archive_trolley(room);
    add_expedition_archive_racks(room);
    add_archive_reference_carriers(room);
    add_archive_scan_rails(room);
  }
  static void apply_crown_mechanics(EngineGrid &room) {
    if(room.district!=5)return;
    strengthen_expedition_wind(room);
    add_fractured_ribs(room);
    add_overhang_windbreak(room);
    add_crown_seam_shortcut(room);
    add_ridge_crosswind(room);
    add_shutter_service_lift(room);
    add_crown_exposed_ridge(room);
  }
  static void apply_expedition_mechanics(EngineGrid &room) {
    apply_harbor_mechanics(room);
    apply_heatworks_mechanics(room);
    apply_estate_mechanics(room);
    apply_archive_mechanics(room,true);
    add_expedition_ice(room);
    add_leaf_chute_flume(room);
    add_reservoir_ice(room);
    add_expedition_root_pod(room);
    add_expedition_irrigation_wheel(room);
    add_expedition_channel_bridge(room);
    add_expedition_fragile_pots(room);
    add_garden_hanging_planter(room);
    add_garden_seed_tray(room);
    add_garden_vines_and_traps(room);
    apply_crown_mechanics(room);
  }
  void refresh_slots() {
    auto &v=state.adventure;
    for(int i=1;i<=iceberg::SaveStore::SlotCount;++i) {
      const auto file=iceberg::SaveStore::slot_file(save_base,i);
      iceberg::Profile candidate;
      iceberg::SaveStore slot(file);
      std::error_code error;
      if(slot.load(candidate))v.slot_status[size_t(i-1)]=candidate.started||candidate.expeditions[0].active||candidate.expeditions[1].active?1:0;
      else v.slot_status[size_t(i-1)]=std::filesystem::exists(file,error)||std::filesystem::exists(file.string()+".bak",error)?2:0;
    }
  }
  void switch_slot(int slot) {
    if(slot==save_slot){state.adventure.save_picker=false;return;}
    if(profile_action_busy) {
      notice(t("Zakończ obsługę konta, by zmienić zapis.","Finish the account action before switching slots."),7);
      return;
    }
    if(!saves.protected_future&&state.adventure.slot_status[size_t(save_slot-1)]!=2) {
      persist();
      if(state.adventure.save_error)return;
    }
    const auto previous_language=profile.language;
    save_slot=slot;
    saves=iceberg::SaveStore(iceberg::SaveStore::slot_file(save_base,slot));
    profile={};
    if(!saves.load(profile))profile.language=previous_language;
    reset_profile_menu();
  }
  void load_tour_scene(int index,int retries=0) {
    ++room_revision;
    tour_scene=index;
    const auto &stop=iceberg::DebugTour[size_t(index)];
    mode=iceberg::Mode::Endless;district=stop.district;stage=2;room=depth=0;run_seed=73;run_active=false;
    profile.task_items[0]=0;
    if(stop.source==iceberg::TourStopSource::Settlement||stop.source==iceberg::TourStopSource::ArchiveReader) {
      if(stop.source==iceberg::TourStopSource::ArchiveReader)remember_curio(iceberg::Curio::NightPlateRecorded);
      else if(stop.room==103)remember_curio(iceberg::Curio::BasementCall);
      profile.task_items[1]=stop.room==102?iceberg::side_flag(iceberg::SideFlag::Roster):0;
      grid=iceworld::build_settlement(district);
      place_side_stories();
    } else if(stop.source==iceberg::TourStopSource::NightPlatform) {
      profile.task_items[1]=0;
      grid=iceworld::build_hidden(iceberg::NightPlatform);
    } else if(stop.source==iceberg::TourStopSource::Basement||stop.source==iceberg::TourStopSource::NightWindow) {
      profile.task_items[1]=0;
      grid=iceworld::build_hidden(iceberg::UnnumberedFloor);
      if(stop.source==iceberg::TourStopSource::NightWindow) {
        remember_curio(iceberg::Curio::ArchiveCall);
        grid.switch_flags|=iceberg::BasementCircuitCut;
      }
    } else {
      grid=stop.room==iceberg::CrownSentinel?iceworld::build_crown_sentinel():verified_room(stop.room,run_seed,district);
      apply_expedition_mechanics(grid);
    }
    grid.protect_exit();
    const Rect entry{stop.entry_x,stop.entry_floor-kPlayerHeight,kPlayerWidth,kPlayerHeight};
    const auto solids=grid.solid_rects_near(entry), hazards=grid.hazard_rects_near(entry);
    const auto overlaps_entry=[&](const Rect &tile){return tile.intersects(entry);};
    if(grid.template_id==stop.room&&grid.solid_at(int(entry.left()),int(stop.entry_floor))&&
       grid.solid_at(int(std::nextafter(entry.right(),entry.left())),int(stop.entry_floor))&&
       std::none_of(solids.begin(),solids.end(),overlaps_entry)&&
       std::none_of(hazards.begin(),hazards.end(),overlaps_entry)) {
      grid.spawn_x=entry.x;grid.spawn_y=entry.y;
      if(!grid.checkpoint_points.empty())grid.checkpoint_points.front()={entry.x,entry.y};
    }
    state=new_state(grid);state.ssh=options.remote;state.deaths=retries;stepper={};
    update_camera(state,grid,last_columns,std::max(1,last_rows-7),last_aspect,10,profile.smooth_camera);
    if(stop.room!=iceberg::CrownSentinel)notice(stop.change.get(profile.language),8,true);
    sync_view();
  }
  void start_tour(int index) {
    if(profile_action_busy) {
      notice(t("Zakończ obsługę konta przed przeglądem.","Finish the account action before starting the tour."),7);
      return;
    }
    before_tour=profile;

    const auto language=profile.language;
    const auto music=profile.music_volume,effects=profile.effects_volume,ambience=profile.ambience_volume;
    const bool focus_pause=profile.pause_on_focus_loss,smooth_camera=profile.smooth_camera;
    profile={};profile.language=language;profile.music_volume=music;
    profile.effects_volume=effects;profile.ambience_volume=ambience;profile.pause_on_focus_loss=focus_pause;profile.smooth_camera=smooth_camera;
    load_tour_scene(index);
  }
  void end_tour() {
    const int last_scene=tour_scene;
    profile=std::move(*before_tour);before_tour.reset();tour_scene=-1;
    reset_profile_menu();
    state.adventure.tour_picker=true;state.adventure.picker_choice=last_scene;
  }
  int harbor_mission_leg() const {return stage==2?0:stage==4?1:2;}
  void load_room(bool resume=false) {
    ++room_revision;
    if(mode==iceberg::Mode::Campaign && profile.hidden_room>=0) {
      grid=iceworld::build_hidden(profile.hidden_room);
      if(profile.hidden_room==iceberg::UnnumberedFloor&&has_side_flag(iceberg::SideFlag::BasementPowerCut))
        grid.switch_flags|=iceberg::BasementCircuitCut;
    } else if(mode==iceberg::Mode::Campaign && profile.revisit.active) {
      if(profile.revisit.scene==iceberg::SurveyReplay)grid=iceworld::build_survey_course();
      else if(profile.revisit.scene==2)grid=iceworld::build_jubilee();
      else grid=campaign_authored(profile.revisit.district,profile.revisit.scene==1);
      if(profile.revisit.scene==2&&(profile.story&(uint64_t(1)<<40)))grid.switch_flags=7;
      grid.interactions.insert(grid.interactions.begin(),{{grid.spawn_x-2,grid.spawn_y-1,1,3},InteractionKind::Return,0,{"Wróć do osady","Return to settlement"}});
    } else if(mode==iceberg::Mode::Campaign && profile.visit_district>=0) {
      grid=campaign_settlement(profile.visit_district);
    } else if(mode==iceberg::Mode::Campaign&&(profile.story&(uint64_t(1)<<42))) {
      grid=iceworld::build_epilogue();
    } else if(mode==iceberg::Mode::Trial&&room==iceberg::RoomsPerLeg-1) {
      grid=iceworld::build_mastery_course(district);grid.seed=run_seed;
    } else if(mode==iceberg::Mode::Campaign && profile.interlude) {
      const int leg=harbor_mission_leg();
      grid=profile.layout_version>=4?iceworld::build_harbor_mission_v4(leg):iceworld::build_harbor_mission(leg);
    } else if(mode==iceberg::Mode::Campaign && (stage==0 || stage==3 || stage==5)) {
      grid=campaign_settlement(district);
    } else if(mode==iceberg::Mode::Campaign&&district==5&&stage==7&&(profile.story&iceberg::CrownArenaReached)) {
      grid=iceworld::build_crown_sentinel();
    } else if(mode==iceberg::Mode::Campaign && (stage==1 || stage==7)) {
      grid=campaign_authored(district,stage==7);
    } else {
      if(mode==iceberg::Mode::Campaign) {
        if(profile.itinerary.size()!=size_t(iceberg::RoomsPerLeg*3))
          profile.itinerary=iceworld::make_itinerary(profile.seed,district,iceberg::RoomsPerLeg*3,profile.layout_version);
        int leg=stage==2?0:stage==4?1:2;
        grid=verified_room(route_room_id(leg,room),
          iceberg::mix(profile.seed ^ unsigned(district*104729+leg*31+room)),district);
        if(profile.layout_version>=4) {
          apply_harbor_mechanics(grid);
          apply_heatworks_mechanics(grid);
          apply_estate_mechanics(grid);
          apply_archive_mechanics(grid);
          apply_crown_mechanics(grid);
          add_garden_vines_and_traps(grid);
        }
      } else {
        auto &e=profile.expeditions[mode==iceberg::Mode::Endless?0:1];
        if(e.itinerary.size()!=iceberg::RoomsPerLeg)
          e.itinerary=expedition_itinerary(run_seed);
        grid=verified_room(e.itinerary[size_t(room % iceberg::RoomsPerLeg)],
          iceberg::mix(run_seed+unsigned(depth)),district,mode==iceberg::Mode::Trial);
        if(mode==iceberg::Mode::Endless&&e.layout_version>=4) {
          apply_expedition_mechanics(grid);
        }
      }
    }
    if(mode==iceberg::Mode::Campaign)place_side_stories();
    if(mode==iceberg::Mode::Campaign && profile.quests[1]==3 && district==1 && room==0 && grid.room_role==iceberg::RoomRole::Traverse)
      grid.interactions.push_back({{grid.spawn_x+2,grid.spawn_y-1,3,3},InteractionKind::Shortcut,0,{"Przejście serwisowe","Service passage"}});
    if(mode==iceberg::Mode::Campaign&&profile.quests[2]==3&&district==2&&room==0&&grid.room_role==iceberg::RoomRole::Traverse)
      grid.interactions.push_back({{grid.spawn_x+2,grid.spawn_y-1,3,3},InteractionKind::Shortcut,2,{"Tramwaj pospieszny","Express tram"}});
    if(profile.quests[3]==3 && grid.district==2 && grid.room_role==iceberg::RoomRole::Settlement) {
      grid.add_solid_rect(55,31,11,1);grid.springs.push_back({50,40.7,1.5,.3});
      grid.sweets.push_back({60,29});grid.sweets.push_back({63,29});
    }
    if(mode==iceberg::Mode::Campaign&&grid.room_role==iceberg::RoomRole::Settlement&&grid.template_id!=330&&
        (profile.story&(uint64_t(1)<<(grid.district+8)))) {
      int ground=grid.district==1?48:grid.district==4?50:grid.district==5?51:43;
      for(int i=0;i<2;++i)grid.interactions.push_back({{i?grid.district==1?67.:grid.district==2?82.:62.:24.,ground-3.,2,3},
        InteractionKind::Replay,i,i?iceberg::Text{"Wróć do stacji pomp","Revisit the pumping station"}:
        iceberg::Text{"Trasa szkoleniowa","Training route"}});
    }
    if(grid.room_role==iceberg::RoomRole::Settlement)for(auto &node:grid.interactions)if(node.kind==InteractionKind::Board)
      node.label=profile.visit_district>=0?iceberg::Text{"Wróć na swoją trasę","Return to your route"}:iceberg::route_title(grid.district,stage);
    if(mode==iceberg::Mode::Campaign&&grid.room_role==iceberg::RoomRole::Settlement&&grid.district==0) {
      const double x=grid.template_id==600?101.:grid.template_id==330?57.:73.;
      const double floor=grid.template_id==330?49.:48.;
      grid.interactions.push_back({{x,floor-4.,3,4},InteractionKind::Replay,iceberg::SurveyReplay,
        {"Objazd latarni przyboju","Breakwater survey"}});
    }
    if(grid.template_id==100||grid.template_id==600||grid.template_id==330) {
      const Rect stall=grid.template_id==600?Rect{72,39,4,4}:
        grid.template_id==100?Rect{72,44,3,4}:Rect{96,45,4,4};
      grid.interactions.push_back({stall,InteractionKind::Outfitter,0,{"Sklep portowy","Harbor outfitter"}});
    }
    grid.protect_exit();
    configure_route_room();
    auto old_view=state.adventure;
    state=new_state(grid);state.adventure=std::move(old_view);
    if(state.adventure.mode!=mode) {
      state.adventure.map_route=false;
      state.adventure.map_room=false;
    }
    state.adventure.route_page=-1;
    state.adventure.dialogue={};state.adventure.map=false;state.adventure.inspecting=false;state.adventure.pickup_left=0;state.adventure.prompt.clear();
    state.adventure.door_left=.42;state.adventure.door_open=0;state.adventure.machinery_left=0;
    state.adventure.lamp_left=0;state.adventure.npc_reaction_left=0;
    state.adventure.water_splash_left=0;state.adventure.garden_snail_left=0;
    state.ssh=options.remote;
    if(resume && mode==iceberg::Mode::Campaign && profile.hidden_room<0 && profile.visit_district<0 && !profile.revisit.active) {
      if(grid.template_id==iceberg::CrownSentinel) {
        if(profile.switches<3)state.crown.hits=state.crown.cycle=int(profile.switches);
      } else grid.switch_flags=profile.switches;
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
    if(resume)restore_route_fork();
    stepper={};sync_view();
    update_camera(state,grid,last_columns,std::max(1,last_rows-7),last_aspect,10,profile.smooth_camera);
    if(grid.template_id==iceberg::CrownSentinel&&(profile.story&iceberg::CrownSentinelDefeated)) {
      state.crown.hits=3;
      if(profile.story&iceberg::reply_recorded(6)) {
        state.crown.airlock_open=true;
        reply_dialogue(6,(profile.story&iceberg::reply_value(6))!=0,2);
      } else show_crown_film(2,profile.switches>>iceberg::CrownFilmShift);
    } else if(resume&&mode==iceberg::Mode::Campaign&&grid.room_role==iceberg::RoomRole::Interlude&&
              grid.district==0&&profile.interlude&&!grid.switch_flags) {
      const int leg=harbor_mission_leg();
      if((profile.story&iceberg::ArrivalMask)==iceberg::arrival_bits(iceberg::FirstHarborMissionArrival+leg))
        restore_harbor_arrival(leg);
    } else if(resume&&mode==iceberg::Mode::Campaign&&grid.room_role==iceberg::RoomRole::Settlement&&profile.visit_district<0) {
      const auto arrival=profile.story&iceberg::ArrivalMask;
      if(grid.template_id==330) {
        if(arrival==iceberg::arrival_bits(iceberg::EpilogueArrival)&&!profile.finished&&!(grid.switch_flags&3))
          restore_arrival_exchange(iceberg::EpilogueReplyScene,iceberg::epilogue_arrival());
      } else if(stage==5&&arrival==iceberg::arrival_bits(district+1)) {
        talk_homecoming(1);
      } else if(stage==3&&(profile.story&iceberg::HomecomingReplyPending)) {
        if(profile.story&iceberg::reply_recorded(district))
          reply_dialogue(district,(profile.story&iceberg::reply_value(district))!=0,FinishHomecomingReply);
        else talk_homecoming(0);
      } else if(!(profile.story&(uint64_t(1)<<district))) {
        const int option=iceberg::pending_briefing_option(profile.story,district);
        const auto opening=profile.story&iceberg::BriefingOpeningMask;
        if(option>=0)reply_dialogue(iceberg::BriefingReplyBase+district,option,1);
        else if(opening==iceberg::briefing_opening_bits(district,true))talk_briefing(true);
        else if(opening==iceberg::briefing_opening_bits(district,false))talk_briefing();
      }
    }
  }
  static constexpr int FinishHomecomingReply=4;
  static constexpr int FinishArrival=5;
  static constexpr int FinishBasementTape=6;
  void talk(std::vector<iceberg::DialogueLine> lines,int action=0,int reply_scene=-1) {
    if(reply_scene>=0)lines.push_back({iceberg::ReplyPrompt,{"Odpowiedz","Reply"}});
    state.adventure.dialogue={std::move(lines),0,action};discard_pending_movement();
    state.adventure.dialogue.reply_scene=reply_scene;
    state.player.vx=0;frame_dialogue();
  }
  void show_crown_film(int action,uint64_t checkpoint=0) {
    talk(iceberg::crown_cutscene(profile.story),action,6);
    auto &film=state.adventure.dialogue;
    film.cinematic=true;
    film.lines.back().shot=iceberg::CrownShot::Daybreak;
    if(checkpoint==iceberg::CrownReplyCheckpoint) {
      film.page=film.lines.size()-1;
      film.shot_seconds=iceberg::CrownAirlockOpeningSeconds;
    } else if(checkpoint>0&&checkpoint<iceberg::CrownReplyCheckpoint) {
      const auto shot=std::find_if(film.lines.cbegin(),film.lines.cend(),[checkpoint](const auto &line){
        return uint64_t(line.shot)==checkpoint;
      });
      if(shot!=film.lines.cend())film.page=std::distance(film.lines.cbegin(),shot);
    }
    state.crown.airlock_open=film.lines[film.page].shot==iceberg::CrownShot::Daybreak;
  }
  void talk_arrival(std::vector<iceberg::DialogueLine> lines,int scene) {
    start_arrival(scene);
    talk(std::move(lines),FinishArrival);
  }
  void start_arrival(int arrival) {
    profile.story=(profile.story&~(iceberg::ArrivalMask|iceberg::ArrivalExchangeMask))|
      iceberg::arrival_bits(arrival);
  }
  void talk_harbor_arrival(int leg) {
    start_arrival(iceberg::FirstHarborMissionArrival+leg);
    restore_harbor_arrival(leg);
  }
  void restore_harbor_arrival(int leg) {
    restore_arrival_exchange(iceberg::HarborMissionReplyBase+leg,
      profile.layout_version>=4?iceworld::harbor_mission_arrival(leg):iceberg::mission_arrival(leg));
  }
  void restore_arrival_exchange(int scene,std::vector<iceberg::DialogueLine> lines) {
    using iceberg::ArrivalExchange;
    const auto exchange=ArrivalExchange((profile.story&iceberg::ArrivalExchangeMask)>>iceberg::ArrivalExchangeShift);
    if(exchange==ArrivalExchange::FirstReply||exchange==ArrivalExchange::SecondReply) {
      reply_dialogue(scene,exchange==ArrivalExchange::SecondReply,FinishArrival);
    } else {
      if(exchange==ArrivalExchange::Prompt)lines.clear();
      talk(std::move(lines),FinishArrival,scene);
    }
  }
  void set_arrival_exchange(iceberg::ArrivalExchange exchange) {
    profile.story=(profile.story&~iceberg::ArrivalExchangeMask)|iceberg::arrival_exchange_bits(exchange);
  }
  void talk_homecoming(int leg) {
    if(leg==0) {
      profile.story|=iceberg::HomecomingReplyPending;
      talk(iceberg::homecoming(district,leg,profile.story),FinishHomecomingReply,district);
    } else {
      talk_arrival(iceberg::homecoming(district,leg,profile.story),district+1);
    }
  }
  void remember_briefing(bool at_prompt) {
    if(tour_scene>=0)return;
    const auto next=(profile.story&~(iceberg::BriefingOpeningMask|iceberg::BriefingReplyMask))|
      iceberg::briefing_opening_bits(district,at_prompt);
    if(next==profile.story)return;
    profile.story=next;
    persist();
  }
  void talk_briefing(bool at_prompt=false) {
    const int scene=iceberg::BriefingReplyBase+district;
    if(at_prompt)talk({},1,scene);
    else talk(iceberg::introduction(district),1,scene);
    remember_briefing(at_prompt);
  }
  void reply_dialogue(int scene,int option,int action) {
    const auto &reply=iceberg::civic_reply(scene,option);
    const double shot_seconds=state.adventure.dialogue.shot_seconds;
    talk({{iceberg::CourierSpeaker,reply.said},reply.answer},action);
    if(scene==6&&grid.template_id==iceberg::CrownSentinel) {
      auto &film=state.adventure.dialogue;
      film.cinematic=true;
      film.shot_seconds=std::max(shot_seconds,iceberg::CrownAirlockOpeningSeconds);
      for(auto &line:film.lines)line.shot=iceberg::CrownShot::Daybreak;
      film.lines[0].voice=20+option*2;
      film.lines[1].voice=21+option*2;
    }
  }
  void frame_dialogue() {
    const auto &dialogue=state.adventure.dialogue;if(!dialogue.active())return;
    if(!state.crown.airlock_open&&dialogue.lines[dialogue.page].cue==iceberg::DialogueCue::CrownAirlock) {
      state.crown.airlock_open=true;
      audio.play(AudioCue::Valve);
    }
    if(dialogue.cinematic)return;
    int courier=4+int(std::floor((state.player.y-state.camera_y)/(last_aspect*.4)));
    auto box=dialogue.choosing()?iceberg::reply_layout(last_rows,courier):
      iceberg::dialogue_layout(dialogue.lines[dialogue.page].text.get(profile.language),last_columns,last_rows,courier,dialogue.segment);
    int feet=5+int(std::floor((state.player.y+kPlayerHeight-.001-state.camera_y)/(last_aspect*.4)));
    // Reserve the courier and a nearby four-row resident at the same floor.
    if(feet-1>=box.top&&feet-4<box.top+box.height) {
      int wanted_feet=box.top==4?box.top+box.height+4:box.top-1;
      state.camera_y-=(wanted_feet-feet)*last_aspect*.4;
    }
  }
  void reflow_dialogue(int columns,int rows) {
    auto &dialogue=state.adventure.dialogue;
    if(!dialogue.active())return;
    auto text=dialogue.lines[dialogue.page].text.get(profile.language);
    auto previous=iceberg::dialogue_layout(text,last_columns,last_rows);
    auto next=iceberg::dialogue_layout(text,columns,rows);
    if(previous.text_rows==next.text_rows&&previous.body==next.body)return;
    if(dialogue.cinematic) {
      const auto &line=dialogue.lines[dialogue.page];
      const double duration=line.voice>=0?iceberg::CrownVoiceSeconds[line.voice][profile.language==iceberg::Language::English]+.55:1.;
      dialogue.segment=std::min(next.pages-1,int(dialogue.line_seconds*next.pages/duration));
      return;
    }

    // Count non-space characters so a new line break does not move the reading position.
    size_t previous_start=0;
    for(int i=0;i<dialogue.segment*previous.text_rows;++i)
      previous_start+=iceberg::dialogue_character_count(previous.body[size_t(i)]);
    if(!dialogue.reflow_anchor)dialogue.reflow_anchor=previous_start;

    size_t start=0;
    std::string segment;
    for(int i=0;i<next.pages;++i) {
      segment=iceberg::dialogue_segment(next,i);
      auto end=start+iceberg::dialogue_character_count(segment);
      if(*dialogue.reflow_anchor<end||i+1==next.pages){dialogue.segment=i;break;}
      start=end;
    }
    if(dialogue.instant){dialogue.reveal_all();return;}

    // A narrower segment can temporarily hide characters that were already visible.
    dialogue.reflow_revealed=std::max(dialogue.reflow_revealed,previous_start+dialogue.visible_characters);
    size_t shown=dialogue.reflow_revealed-start;
    dialogue.visible_characters=dialogue.byte_cursor=0;
    while(dialogue.byte_cursor<segment.size()&&dialogue.visible_characters<shown) {
      unsigned char ch=segment[dialogue.byte_cursor++];
      if(ch!=' '&&(ch&0xc0)!=0x80)++dialogue.visible_characters;
    }
    while(dialogue.byte_cursor<segment.size()&&
          (static_cast<unsigned char>(segment[dialogue.byte_cursor])&0xc0)==0x80)++dialogue.byte_cursor;
    dialogue.revealing=dialogue.byte_cursor<segment.size();
  }
  void tick_dialogue(double dt) {
    auto &dialogue=state.adventure.dialogue;
    if(!dialogue.active()||!dialogue.revealing)return;
    const auto &line=dialogue.lines[dialogue.page];auto text=iceberg::dialogue_segment(line,profile.language,last_columns,last_rows,dialogue.segment);
    dialogue.character_delay-=dt;dialogue.speech_delay=std::max(0.,dialogue.speech_delay-dt);
    bool voiced=false;
    static const double rates[]={64,56,59,68,55,60,72,52};
    while(dialogue.character_delay<=0&&dialogue.byte_cursor<text.size()) {
      unsigned char ch=text[dialogue.byte_cursor];
      size_t bytes=ch<128?1:ch<224?2:ch<240?3:4;
      dialogue.byte_cursor+=std::min(bytes,text.size()-dialogue.byte_cursor);
      if(ch==' '||ch=='\n'||ch=='\t'||ch=='\r')continue;
      ++dialogue.visible_characters;
      double delay=1./rates[size_t(std::clamp(line.speaker,0,7))];
      if(ch=='.'||ch=='!'||ch=='?')delay+=.12;
      else if(ch==','||ch==';'||ch==':')delay+=.055;
      dialogue.character_delay+=delay;
      bool letter=ch>=128||(ch>='A'&&ch<='Z')||(ch>='a'&&ch<='z')||(ch>='0'&&ch<='9');
      if(letter&&dialogue.speech_delay<=0&&!voiced) {
        if(line.speaker>=0)audio.play(AudioCue(int(AudioCue::Talk0)+std::clamp(line.speaker,0,7)),.86f+float(dialogue.visible_characters%3)*.06f);
        else audio.play(AudioCue::Paper,.12f);
        dialogue.speech_delay=line.speaker<0?.22:.075;voiced=true;
      }
    }
    if(dialogue.byte_cursor>=text.size())dialogue.revealing=false;
  }
  void tick_crown_film(double dt) {
    auto &film=state.adventure.dialogue;
    const auto &line=film.lines[film.page];
    film.shot_seconds+=dt;
    film.line_seconds+=dt;
    const auto layout=iceberg::dialogue_layout(line.text.get(profile.language),last_columns,last_rows);
    const auto speaker_time=line.voice>=0?iceberg::CrownVoiceSeconds[line.voice][profile.language==iceberg::Language::English]:0.;
    film.revealing=line.speaker!=-1&&film.line_seconds<speaker_time;
    const double duration=line.speaker==-1?speaker_time:speaker_time+.55;
    const int segment=std::min(layout.pages-1,int(film.line_seconds*layout.pages/std::max(1.,duration)));
    film.segment=std::max(film.segment,segment);
    if(film.line_seconds>=duration)dialogue_next();
  }
  void unlock_achievement(iceberg::Achievement id) {
    const auto bit=iceberg::achievement_bit(id);
    if(tour_scene>=0||iceberg::has_award(iceberg::stored_award_progress(profile),id))return;
    if(iceberg::achievement_word(id))profile.task_items[1]|=bit;
    else {
      profile.discoveries|=uint32_t(bit);
      profile.task_items[0]|=bit&0xffffffff00000000ull;
    }
    if(!iceberg::has_award(state.adventure.awards,id))achievement_queue.push_back(id);
  }
  void advance_achievement_toast(double dt) {
    auto &v=state.adventure;
    if(dt<=0.||tour_scene>=0||v.dialogue.active())return;
    v.achievement_left=std::max(0.,v.achievement_left-dt);
    if(v.achievement_left>0.)return;
    if(achievement_queue.empty()){v.achievement_toast=-1;return;}
    v.achievement_toast=int(achievement_queue.front());
    achievement_queue.erase(achievement_queue.begin());
    v.achievement_left=iceberg::AchievementToastTime;
    audio.play(AudioCue::Achievement);
  }
  void offer_outfitter_item(int index) {
    talk({{iceberg::ReplyPrompt,{}}});
    auto &dialogue=state.adventure.dialogue;
    dialogue.outfitter_item=index;
    dialogue.outfitter_tokens=iceberg::outfitter_balance(profile);
    dialogue.reply_option=1;
  }
  void dialogue_next() {
    auto &dialogue=state.adventure.dialogue;
    if(!dialogue.active())return;
    if(dialogue.choosing()) {
      if(dialogue.outfitter_item>=0) {
        const int index=dialogue.outfitter_item;
        const auto &item=iceberg::OutfitterItems[size_t(index)];
        if(dialogue.reply_option==1) {
          if(index+1<int(iceberg::OutfitterItems.size()))offer_outfitter_item(index+1);
          else dialogue={};
        } else if(!(profile.cosmetics&item.flag)&&iceberg::outfitter_balance(profile)>=item.price) {
          iceberg::remember_outfitter_tokens(profile);
          profile.cosmetics|=item.flag;
          unlock_achievement(iceberg::Achievement::Gear);
          constexpr uint32_t kit=iceberg::HarborScarf|iceberg::DockBoots|iceberg::SignalSatchel;
          if((profile.cosmetics&kit)==kit)unlock_achievement(iceberg::Achievement::AllGear);
          persist();audio.play(AudioCue::Cloth);
          offer_outfitter_item(index);
        }
        return;
      }
      const int scene=dialogue.reply_scene, option=dialogue.reply_option;
      const int action=dialogue.action;
      if(scene==iceberg::HeatworksMugReplyScene) {
        unlock_achievement(iceberg::Achievement::MugReturned);
        remember_side_flag(iceberg::SideFlag::HeatworksMugReturned);
        audio.play(AudioCue::Stone1,.55f);
        persist();
      } else if(scene==iceberg::EstateRosterReplyScene) {
        unlock_achievement(iceberg::Achievement::RosterChoice);
        remember_side_flag(option==0?iceberg::SideFlag::RosterPosted:iceberg::SideFlag::RosterSealed);
        persist();
      } else if(tour_scene<0&&!iceberg::expedition_reply_scene(scene)) {
        if(scene>=iceberg::HarborMissionReplyBase) {
          set_arrival_exchange(option?iceberg::ArrivalExchange::SecondReply:iceberg::ArrivalExchange::FirstReply);
        } else if(scene<iceberg::BriefingReplyBase) {
          profile.story|=iceberg::reply_recorded(scene);
          if(option)profile.story|=iceberg::reply_value(scene);
          else profile.story&=~iceberg::reply_value(scene);
        } else {
          profile.story=(profile.story&~(iceberg::BriefingReplyMask|iceberg::BriefingOpeningMask))|
            iceberg::briefing_reply_bits(scene-iceberg::BriefingReplyBase,option);
        }
        persist();
      }
      reply_dialogue(scene,option,action);
      return;
    }
    if(dialogue.revealing&&!dialogue.cinematic){
      dialogue.reveal_all();
      dialogue.instant=true;
      return;
    }
    const auto shot=dialogue.lines[dialogue.page].shot;
    auto layout=iceberg::dialogue_layout(dialogue.lines[dialogue.page].text.get(profile.language),last_columns,last_rows,1000,dialogue.segment);
    if(!dialogue.cinematic&&dialogue.segment+1<layout.pages)++dialogue.segment;
    else {++dialogue.page;dialogue.segment=0;}
    if(dialogue.active()){
      dialogue.line_seconds=0.;
      if(dialogue.cinematic)dialogue.instant=false;
      if(shot!=dialogue.lines[dialogue.page].shot)dialogue.shot_seconds=0.;
      dialogue.reset_reveal();if(dialogue.instant)dialogue.reveal_all();
      if(dialogue.choosing()&&dialogue.reply_scene!=iceberg::EstateRosterReplyScene&&
         dialogue.reply_scene!=iceberg::HeatworksMugReplyScene&&
         !iceberg::expedition_reply_scene(dialogue.reply_scene)) {
        if(dialogue.reply_scene>=iceberg::HarborMissionReplyBase) {
          if(tour_scene<0) {
            set_arrival_exchange(iceberg::ArrivalExchange::Prompt);
            persist();
          }
        } else if(dialogue.reply_scene>=iceberg::BriefingReplyBase)remember_briefing(true);
      }
      frame_dialogue();
      if(dialogue.cinematic&&dialogue.reply_scene==6&&tour_scene<0&&
         (dialogue.choosing()||shot!=dialogue.lines[dialogue.page].shot))persist();
      if(dialogue.action==FinishBasementTape&&(dialogue.page==3||dialogue.page==4))
        audio.play(dialogue.page==3?AudioCue::Spark:AudioCue::Metal1,.43f);
      else audio.play(AudioCue::Paper,.4f);
      return;
    }
    int action=dialogue.action;
    dialogue={};
    if(action==1) {
      profile.story&=~(iceberg::BriefingReplyMask|iceberg::BriefingOpeningMask);
      profile.story|=uint64_t(1)<<district;
    } else if(action==2) {
      complete_district();
    } else if(action==3) {
      profile.finished=true;profile.cosmetics|=1u<<7;audio.play(AudioCue::Win);
      unlock_achievement(iceberg::Achievement::Journey);
    } else if(action==FinishHomecomingReply) {
      profile.story&=~iceberg::HomecomingReplyPending;
    } else if(action==FinishArrival) {
      profile.story&=~(iceberg::ArrivalMask|iceberg::ArrivalExchangeMask);
    } else if(action==FinishBasementTape) {
      unlock_achievement(iceberg::Achievement::WitnessTape);
    } else if(action>=100 && action<108) {
      int quest=action-100;
      profile.quests[size_t(quest)]=profile.quest_counts[size_t(quest)]>=unsigned(iceberg::Quests[size_t(quest)].required)?2:1;
    } else if(action>=200 && action<208) {
      int quest=action-200;profile.quests[size_t(quest)]=3;if(profile.quest_focus==quest)profile.quest_focus=state.adventure.quest_focus=-1;
      profile.cosmetics|=1u<<(quest==5?5:quest%7+1);
      notice(t("ZADANIE UKOŃCZONE — wpis w księdze zadań","TASK COMPLETE — entry in the quest book"));
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
    if(district==0)unlock_achievement(iceberg::Achievement::FirstDistrict);
    if(profile.district_deaths==0)unlock_achievement(iceberg::Achievement::CleanDistrict);
    if(profile.district_deaths>=20)unlock_achievement(iceberg::Achievement::RoughDistrict);
    if(district==0)unlock_achievement(iceberg::Achievement::NextShift);
    if(district==4)unlock_achievement(iceberg::Achievement::CrownReached);
    merge_record(profile.records[size_t(district)],profile.district_seconds,
      profile.district_deaths,profile.district_treasures);
    if(std::all_of(profile.records.begin(),profile.records.end(),
        [](const auto &record){return record.complete&&record.best_deaths==0;}))
      unlock_achievement(iceberg::Achievement::CleanJourney);
    profile.story|=uint64_t(1)<<(district+8);
    profile.district_seconds=0;profile.district_deaths=profile.district_treasures=0;
    profile.itinerary.clear();
    if(district+1<iceberg::AuthoredDistrictCount) {
      ++district;profile.unlocked=std::max(profile.unlocked,district);
    } else if(district+1==iceberg::DistrictCount){profile.story|=uint64_t(1)<<42;}
    else profile.at_content_boundary=true;
    stage=0;room=0;clear_room_record();load_room();
    if(profile.story&(uint64_t(1)<<42)) {
      start_arrival(iceberg::EpilogueArrival);
      restore_arrival_exchange(iceberg::EpilogueReplyScene,iceberg::epilogue_arrival());
    }
    persist();
  }
  void begin_campaign(bool fresh) {
    persist();
    if(fresh) {
      iceberg::remember_outfitter_tokens(profile);
      auto previous=profile;
      auto language=profile.language;
      float music=profile.music_volume,effects=profile.effects_volume,ambience=profile.ambience_volume;
      profile={};profile.language=language;
      profile.seed=options.seed_override||!previous.started?options.seed:iceberg::mix(previous.seed+1);
      profile.records=previous.records;profile.trials=previous.trials;profile.challenges=previous.challenges;
      profile.expeditions=previous.expeditions;profile.endless_best=previous.endless_best;
      profile.cosmetics=previous.cosmetics;profile.outfit=previous.outfit;
      const auto awards=iceberg::achievement_progress(previous);
      profile.discoveries=uint32_t(awards[0]);
      profile.task_items[0]=awards[0]&0xffffffff00000000ull;
      profile.task_items[1]=awards[1];
      profile.music_volume=music;profile.effects_volume=effects;profile.ambience_volume=ambience;
      profile.pause_on_focus_loss=previous.pause_on_focus_loss;
      profile.smooth_camera=previous.smooth_camera;
      state.adventure.map_route=state.adventure.map_room=false;
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
  bool trial_unlocked_without_pass() const {
    return profile.quests[6]==3||(profile.cosmetics&(1u<<7))||profile.finished||profile.at_content_boundary;
  }
  bool trial_unlocked() const {
    return trial_unlocked_without_pass()||(profile.cosmetics&iceberg::MasteryTransitPass)||
      iceberg::mastery_token_progress(profile)==iceberg::MasteryTokenFare;
  }
  void request_run(iceberg::Mode next,bool fresh=false) {
    const bool trial=next==iceberg::Mode::Trial;
    const int area=selected_run_area(next,fresh);
    if(profile.expeditions[trial?1:0].active&&(fresh||!can_resume_run(next,area))&&
       (!trial||trial_unlocked())) {
      state.adventure.selected=trial?3:2;
      if(state.menu)state.adventure.confirm_new=true;
      else {persist();state.menu=true;discard_pending_movement();}
      return;
    }
    begin_run(next,fresh);
  }
  void begin_run(iceberg::Mode next,bool fresh=false) {
    const bool trial=next==iceberg::Mode::Trial;
    const int area=selected_run_area(next,fresh);
    auto &e=profile.expeditions[trial?1:0];
    const bool resume=!fresh&&can_resume_run(next,area);
    const bool replay=trial&&!fresh&&can_retry_trial(area);
    if(trial&&!resume&&!replay&&!trial_unlocked()) {
      const auto tokens=std::to_string(iceberg::mastery_token_progress(profile))+"/"+
        std::to_string(iceberg::MasteryTokenFare);
      notice(t("Wstęp: pokaż Kubie trzy skoki ze zrywu albo zbierz żetony (",
               "Entry: show Kuba three dash jumps or collect transit tokens (")+tokens+").",8);
      return;
    }
    persist();mode=next;run_district=area;
    if(resume) {
      district=e.district;room=e.room;depth=e.depth;run_seed=e.seed;
      run_seconds=e.seconds;run_deaths=e.deaths;run_treasures=e.treasures;
    } else {
      district=area;room=depth=0;
      run_seed=replay?e.seed:options.seed_override?options.seed:trial?default_trial_seed(area):iceberg::mix(run_seed+1);
      iceberg::Expedition restarted;
      restarted.origin_seed=run_seed;
      if(replay) {
        restarted.layout_version=e.layout_version;
        restarted.itinerary=std::move(e.itinerary);
      }
      if(!trial)iceberg::remember_outfitter_tokens(profile);
      run_seconds=0;run_deaths=run_treasures=0;e=std::move(restarted);
    }
    stage=2;run_active=true;load_room(resume);state.menu=false;persist();
    notice(resume?t("WZNOWIONO WYPRAWĘ — ta sama droga","EXPEDITION RESUMED — the same route"):
      next==iceberg::Mode::Endless?t("WYPRAWA BEZ KOŃCA — osiem pokoi w odcinku","ENDLESS EXPEDITION — eight rooms per leg"):
      t("PRÓBA MISTRZOWSKA — osiem sal, jeden rekord","MASTERY TRIAL — eight rooms, one record"),3,true);
  }
  void advance_room() {
    if(tour_scene>=0) {
      const int next=tour_scene+1;
      if(next<int(iceberg::DebugTour.size()))load_tour_scene(next);
      else end_tour();
      return;
    }
    if(on_campaign_route()&&route_forks_enabled()&&room==1)
      profile.route_decisions[size_t(district)]|=1u<<route_leg();
    audio.play(AudioCue::Door);
    if(mode==iceberg::Mode::Campaign&&district==0&&stage==0&&profile.hidden_room<0&&profile.visit_district<0&&!profile.revisit.active)
      unlock_achievement(iceberg::Achievement::FirstDoor);
    if(state.deaths>=5&&grid.room_role==iceberg::RoomRole::Traverse)
      unlock_achievement(iceberg::Achievement::MadeItAnyway);
    if(state.won&&state.deaths>=5&&grid.room_role==iceberg::RoomRole::Traverse&&
       grid.sweets.size()>=3&&state.sweets_count()==0)
      unlock_achievement(iceberg::Achievement::EmptyDelivery);
    if(state.won&&grid.room_role==iceberg::RoomRole::Traverse&&state.deaths==0&&
       grid.sweets.size()>=3&&size_t(state.sweets_count())==grid.sweets.size())
      unlock_achievement(iceberg::Achievement::FullRoom);
    if(mode!=iceberg::Mode::Campaign) {
      if(mode==iceberg::Mode::Endless&&depth==iceberg::RoomsPerLeg-1) {
        unlock_achievement(iceberg::Achievement::TrailLeg);
        if(run_deaths==0)unlock_achievement(iceberg::Achievement::CleanLeg);
      }
      ++room;++depth;if(mode==iceberg::Mode::Endless)profile.endless_best=std::max(profile.endless_best,depth);
      if(mode==iceberg::Mode::Trial && room>=iceberg::RoomsPerLeg) {
        unlock_achievement(iceberg::Achievement::Mastery);
        if(run_deaths==0) {
          unlock_achievement(iceberg::Achievement::CleanMastery);
          if(run_treasures==0)unlock_achievement(iceberg::Achievement::QuietLeg);
        }
        if(run_deaths>=25)unlock_achievement(iceberg::Achievement::HardLesson);
        if(separate_trial_record(district,run_seed,route_version())) {
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
        state.adventure.selected=3;
        notice(options.no_save?t("PRÓBA UKOŃCZONA","TRIAL COMPLETE"):
          state.adventure.save_error?t("PRÓBA UKOŃCZONA — błąd zapisu","TRIAL COMPLETE — save error"):
          t("PRÓBA UKOŃCZONA — zapisano","TRIAL COMPLETE — record saved"),6);
      } else {
        if(room>=iceberg::RoomsPerLeg) {
          auto &expedition=profile.expeditions[0];
          auto &itinerary=expedition.itinerary;
          if(itinerary.size()==iceberg::RoomsPerLeg) {
            std::array<int,iceberg::RoomsPerLeg> leg;
            std::copy(itinerary.begin(),itinerary.end(),leg.begin());
            expedition.completed_legs.push_back(leg);
          } else expedition.completed_legs.clear();
          room=0;run_seed=next_expedition_seed();itinerary.clear();
        }
        load_room();persist();
      }
      return;
    }
    if(profile.hidden_room>=0) {
      const int secret=profile.hidden_room;
      if(secret==iceberg::UnnumberedFloor&&has_side_flag(iceberg::SideFlag::Roster))
        unlock_achievement(iceberg::Achievement::BelowTheRoster);
      profile.hidden_room=-1;load_room(true);
      const auto entrance=std::find_if(grid.interactions.begin(),grid.interactions.end(),[secret](const auto &node){
        return node.kind==InteractionKind::Secret&&node.id==secret;
      });
      if(entrance!=grid.interactions.end()) {
        auto &courier=state.player;
        const auto door=entrance->at(grid.platforms,state.elapsed);
        courier.x=door.x+(door.w-kPlayerWidth)*.5;
        courier.y=door.bottom()-kPlayerHeight;
        courier.grounded=true;
        courier.support=entrance->platform;
        update_camera(state,grid,last_columns,std::max(1,last_rows-7),last_aspect,10,profile.smooth_camera);
      }
      persist();return;
    }
    if(profile.revisit.active) {
      const int scene=profile.revisit.scene;
      bool jubilee=profile.revisit.scene==2&&state.won&&!(profile.story&(uint64_t(1)<<40));
      const bool surveyed=scene==iceberg::SurveyReplay&&state.won&&!(profile.cosmetics&iceberg::SurveyCourseComplete);
      if(scene==iceberg::SurveyReplay&&state.won&&state.deaths==0)
        unlock_achievement(iceberg::Achievement::CleanSurvey);
      if(surveyed) {
        profile.cosmetics|=iceberg::SurveyCourseComplete;
        unlock_achievement(iceberg::Achievement::Survey);
      }
      if(jubilee){profile.story|=uint64_t(1)<<40;profile.cosmetics|=1u<<3;}
      profile.revisit={};load_room(true);
      const auto entrance=std::find_if(grid.interactions.begin(),grid.interactions.end(),[scene](const auto &node){
        return node.kind==InteractionKind::Replay&&node.id==scene;
      });
      if(entrance!=grid.interactions.end()) {
        state.player.x=entrance->area.x;
        state.player.y=entrance->area.bottom()-kPlayerHeight;
        update_camera(state,grid,last_columns,last_rows-7,last_aspect,10,profile.smooth_camera);
      }
      if(jubilee){talk(iceberg::jubilee_complete());audio.play(AudioCue::Win);}
      if(surveyed){
        talk({{0,{"Z brzegu nie widać, że trzecia boja zgasła. Dobrze, że masz ją zaznaczoną na mapie. Popłynę tam rano.",
                  "From shore you can't tell the third buoy has gone dark. Good thing you marked it on the chart. I'll sail out in the morning."}}});
        audio.play(AudioCue::Win);
      }
      persist();return;
    }
    if(profile.visit_district>=0) {
      profile.visit_district=-1;load_room(true);persist();return;
    }
    if(grid.room_role==iceberg::RoomRole::Interlude) {
      int leg=stage==2?0:stage==4?1:2;
      profile.story|=uint64_t(1)<<(16+leg);profile.interlude=false;
      stage=stage==2?3:stage==4?5:7;room=0;clear_room_record();load_room();
      if(stage==3||stage==5)talk_homecoming(stage==3?0:1);
      persist();return;
    }
    if(grid.room_role==iceberg::RoomRole::Settlement) {
      if(profile.at_content_boundary) {
        talk({{1,{"Dolne obiegi pracują. Droga wyżej poczeka na następną część podróży. Na dziś uratowaliśmy komuś mieszkanie. To nie jest mało.","The lower circuits are working. The route above awaits the next part of the journey. Today we saved somebody's home. That's no small thing."}},
          {0,{"Możesz wrócić do mieszkańców, dokończyć ich sprawy albo wybrać wyprawę i próbę mistrzowską w menu. Poczta poczeka. Tym razem naprawdę.","You can return to the residents, finish their tasks, or choose an expedition or mastery trial in the menu. The mail can wait. This time, really."}}});return;
      }
      if(grid.template_id==330) {
        if(profile.finished) {
          choose_saved_run_district(2);
          request_run(iceberg::Mode::Endless);
          return;
        }
        if((grid.switch_flags&3)!=3) {
          notice(t("Zostaw nasiona przy przystanku i doręcz odpis spisu do okienka.","Leave the seeds by the stop and deliver the register copy to the counter."),6);return;
        }
        talk(iceberg::journey_end(),3);return;
      }
      if(!(profile.story&(uint64_t(1)<<district))) {
        talk_briefing();return;
      }
      stage=stage==0?1:stage==3?4:6;room=0;
    } else if(stage==1) {stage=2;room=0;}
    else if(stage==7) {
      if(district==5&&grid.template_id!=iceberg::CrownSentinel) {
        profile.story|=iceberg::CrownArenaReached;
        clear_room_record();load_room();persist();
      } else if(district!=5)talk(iceberg::conclusion(district),2);
      return;
    }
    else if(++room>=iceberg::RoomsPerLeg) {
      int leg=stage==2?0:stage==4?1:2;
      if(district==0&&!(profile.story&(uint64_t(1)<<(16+leg)))) {
        room=iceberg::RoomsPerLeg-1;profile.interlude=true;clear_room_record();load_room();
        talk_harbor_arrival(leg);persist();return;
      }
      room=0;stage=stage==2?3:stage==4?5:7;
    }
    clear_room_record();load_room();
    if(stage==3||stage==5)talk_homecoming(stage==3?0:1);
    persist();
  }
  bool at_locked_survey_exit() const {
    return grid.template_id==iceberg::BreakwaterSurvey&&(grid.switch_flags&1)==0&&
      grid.touches_exit(state.player.rect());
  }
#include "iceberg_side_stories.inc"
  int near_interaction() const {
    auto p=state.player.rect();p.x-=1.6;p.y-=1.;p.w+=3.2;p.h+=2.;
    for(size_t i=0;i<grid.interactions.size();++i)
      if(side_story_available(grid.interactions[i])&&
         !(grid.interactions[i].kind==InteractionKind::Evidence&&(grid.switch_flags&4))&&
         !(grid.template_id==310&&grid.interactions[i].kind==InteractionKind::Rescue&&(grid.switch_flags&8))&&
         p.intersects(grid.interactions[i].at(grid.platforms,state.elapsed)))return int(i);
    return -1;
  }
  void animate_pickup(const Interaction &node,int kind) {
    auto &v=state.adventure;v.pickup_left=.65;v.pickup_x=node.area.x;
    v.pickup_y=node.area.bottom()-1.3;v.pickup_kind=kind;
    if(kind==0){audio.play(AudioCue::Paper);audio.play(AudioCue::Cloth);}
  }
  void update_water_splash(const Player &before,int old_deaths,double dt) {
    auto &view=state.adventure;
    if(state.deaths!=old_deaths){view.water_splash_left=0;return;}
    view.water_splash_left=std::max(0.,view.water_splash_left-dt);
    const double from=before.rect().bottom(),to=state.player.rect().bottom();
    if(to<=from)return;
    for(const auto &basin:grid.water_basins) {
      if(from>basin.y||to<=basin.y)continue;
      const double contact=(basin.y-from)/(to-from);
      const double x=std::lerp(before.x,state.player.x,contact)+kPlayerWidth*.5;
      if(x<basin.x||x>=basin.right())continue;
      view.water_splash_x=x;view.water_splash_y=basin.y;
      view.water_splash_left=iceberg::View::water_splash_duration;
      if(grid.district==0)unlock_achievement(iceberg::Achievement::SaltBath);
      break;
    }
  }
  void activate_machinery(const Interaction &node,bool tram=false,bool steam_supply=false) {
    auto &v=state.adventure;v.machinery_duration=tram?2.8:grid.district==5?2.4:grid.district==4?1.6:1.05;
    v.machinery_left=v.machinery_duration;v.machinery_kind=tram?2:grid.district==4?5:grid.district==3?3:node.kind==InteractionKind::Task&&node.id==2?1:0;
    v.machinery_x=node.area.x+node.area.w*.5;v.machinery_y=node.area.bottom();
    v.machinery_changed=node.kind==InteractionKind::Valve?uint64_t(1)<<node.id:0;
    if(steam_supply)audio.play(AudioCue::Steam,.8f);
    else {
      audio.play(AudioCue::Motor,tram?1.f:.6f);audio.play(AudioCue::Rumble,tram?1.f:.45f);
    }
    if(tram||v.machinery_kind==1)audio.play(AudioCue::Spark,.65f);
  }
  void inspect_nearby() {
    auto &v=state.adventure;
    v.inspect_offset=0;v.inspect_detail=0;
    using Art=iceberg::InspectionArt;
    auto describe=[&](std::string title,std::string body,const char *icon,Art art) {
      v.inspect_title=std::move(title);v.inspect_body=std::move(body);v.inspect_icon=icon;v.inspect_art=art;
      v.inspecting=true;discard_pending_movement();audio.play(AudioCue::Paper);
    };
    if(at_locked_survey_exit()) {
      describe(t("Powrót do przystani","Return to harbor"),
        t("Drzwi otworzą się, gdy oznaczysz trzecią boję na mapie po lewej.",
          "The return door opens once buoy 3 is marked on the chart to your left."),
        "▣",Art::Door);
      return;
    }
    int at=near_interaction();
    if(at<0) {
      auto close=[&](double x,double y){return std::hypot(state.player.x+.45-x,state.player.y+.6-y)<2.8;};
      for(size_t i=0;i<grid.sweets.size();++i)if(!state.collected[i]&&close(grid.sweets[i].first,grid.sweets[i].second)) {
        const auto tokens=std::to_string(iceberg::mastery_token_progress(profile))+"/"+
          std::to_string(iceberg::MasteryTokenFare);
        const auto body=trial_unlocked()?
          t("Pocztowy żeton przywraca zryw. Wstęp do próby mistrzowskiej jest już otwarty.",
            "A postal token restores your dash. The Mastery Trial is already open."):
          t("Żeton przywraca zryw. Zbierz komplet w podróży lub podczas jednej wyprawy, by opłacić stały wstęp do próby mistrzowskiej. Stan: ",
            "A token restores your dash. Collect a set in the campaign or in one expedition to pay for permanent access to the Mastery Trial. Collected: ")+tokens+".";
        const auto survey=t(" Objazd latarni przyboju w Dolnej Przystani: "," Breakwater Survey in Lower Harbor: ")+
          std::to_string(iceberg::survey_pass(profile)?iceberg::SurveyTokenFare:
            iceberg::transit_token_progress(profile,iceberg::SurveyTokenFare))+"/"+std::to_string(iceberg::SurveyTokenFare)+".";
        const auto shop=t(" Żetony wydasz w sklepie w Dolnej Przystani.",
          " Spend tokens at the Lower Harbor outfitter.");
        describe(t("Żeton transportowy","Transit token"),body+survey+shop,"◉",Art::Coin);return;
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
    if(node.kind==InteractionKind::SideStory) {
      using iceberg::SideStory;
      using iceberg::SideFlag;
      if(node.id==int(SideStory::ArchiveLine)) {
        describe(t("Linia 11","Line 11"),t("Wtyk jest wyjęty. Pod szybką tkwi plakietka: «Klatka 11».",
          "The plug is unplugged. A tag under the glass reads ‘Stairwell 11’."),"╪",Art::Speaker);return;
      }
      if(node.id==int(SideStory::CarbonReader)||node.id==int(SideStory::CarbonDial)) {
        const bool printed=has_side_flag(SideFlag::ArchiveCarbonPrinted);
        const auto instructions=t("Ustaw godzinę, dodatkowe obciążenie i ostemplowaną stronę według wydruku wagi. Potem użyj czytnika.",
          "Set the time, the extra load, and the stamped side from the scale receipt. Then use the reader.");
        const auto receipt=t("Twój wydruk · 00:07 · OBCIĄŻENIE 2 · PO TEJ STRONIE.",
          "Your receipt · 00:07 · LOAD 2 · THIS SIDE.");
        auto body=printed?iceberg::CarbonReaderRecord.get(profile.language):instructions;
        if(!printed&&has_curio(iceberg::Curio::NightPlateRecorded))body+=" "+receipt;
        describe(printed?t("Kopia spod taśmy","Carbon copy"):t("Czytnik kalek","Carbon reader"),body,"▤",Art::File);return;
      }
      if(node.id==int(SideStory::NightWeighbridge)) {
        const auto body=has_curio(iceberg::Curio::NightPlateRecorded)?
          t("00:07 · OBCIĄŻENIE 1: KURIER. OBCIĄŻENIE 2: OCZEKUJE. W rubryce «gdzie» ktoś wcisnął pieczątkę: «po tej stronie».",
            "00:07 · LOAD 1: COURIER. LOAD 2: WAITING. Someone stamped the location field ‘THIS SIDE’."):
          t("Instrukcja: stań nieruchomo między żółtymi kreskami, aż wskazówki się zatrzymają. Do pomiaru bez zasilania służy zapasowa sprężyna.",
            "Stand still between the yellow lines until the needles settle. A backup spring takes measurements with the power off.");
        describe(t("Waga peronowa","Platform scale"),body,"◴",Art::File);return;
      }
      if(node.id==int(SideStory::NightWindow)) {
        describe(t("Szyba obserwacyjna","Observation window"),
          grid.switch_flags&iceberg::BasementCircuitCut?
            t("Po drugiej stronie szyby słabo widać peron. Na tablicy mignął numer 11.",
              "A platform is faintly visible through the window. The board briefly showed 11."):
            t("Jasne lampy odbijają się w szkle. Za nim nie widać niczego.",
              "The bright lamps reflect in the glass. Nothing beyond it is visible."),
          "▥",Art::Speaker);return;
      }
      if(node.id==int(SideStory::HeatworksMug)) {
        describe(t("Kubek na rurze","Mug on the pipe"),HeatworksMugNote.get(profile.language),"♨",Art::File);return;
      }
      if(node.id==int(SideStory::HeatworksMugBench)) {
        const bool returned=has_side_flag(SideFlag::HeatworksMugReturned);
        describe(t("Stół Witka","Witek’s bench"),returned?HeatworksMugRota.get(profile.language):
          t("E — oddaj Witkowi kubek.","Press E to give Witek his mug."),"♨",Art::File);return;
      }
      if(node.id==int(SideStory::GardensPot)&&has_side_flag(SideFlag::GardenSnailFound)) {
        describe(t("Doniczka","Pot"),garden_snail_pot_text().get(profile.language),"♧",Art::Seeds);return;
      }
      if(node.id==int(SideStory::GardenSnailTap)) {
        const auto body=has_side_flag(SideFlag::GardenSnailWatered)?
          t("Ziemia jest mokra aż do liści po prawej.","The soil is damp all the way to the leaves on the right."):
          t("Kranik jest połączony z rynną przy odwróconej doniczce.","The tap feeds the gutter beside the overturned pot.");
        describe(t("Kranik ogrodowy","Garden tap"),body,"⊗",Art::Valve);return;
      }
      if(node.id==int(SideStory::GardenSnailShelter)) {
        describe(t("Pod funkią","Under the hosta"),GardenSnailTag.get(profile.language),"♧",Art::Seeds);return;
      }
      if(node.id==int(iceberg::SideStory::BasementBreaker)) {
        describe(t("Odłącznik piwnicy","Basement breaker"),
          t("Ten odłącznik zasila lampy za pomostem i zwarty przewód. E — przełącz zasilanie.",
            "This breaker feeds the lamps beyond the landing and the shorted cable. Press E to flip it."),
          "⊗",Art::Valve);
        return;
      }
      if(node.id==int(iceberg::SideStory::RosterDesk)) {
        describe(roster_desk_title().get(profile.language),roster_desk_body().get(profile.language),"▤",Art::File);
        return;
      }
      if(const auto document=side_document(iceberg::SideStory(node.id))) {
        auto body=document->body.get(profile.language);
        if(!has_side_flag(document->flag))
          body+=t(" Zamknij podgląd i naciśnij E, aby zrobić odpis.",
                  "Close inspection and press E to make a copy.");
        describe(document->title.get(profile.language),std::move(body),"▤",Art::File);
        return;
      }
    }
    v.inspect_title=node.label.get(profile.language);v.inspect_icon="▤";v.inspect_art=Art::Door;v.inspect_detail=node.id;
    switch(node.kind) {
    case InteractionKind::Npc:case InteractionKind::Rescue:
      v.inspect_art=Art::Character;v.inspect_title=iceberg::Npcs[size_t(node.id)].name;
      v.inspect_body=grid.template_id==13&&node.id==1?
        t("Witek pilnuje przekaźników. Zielona lampka oznacza, że winda stoi na tym poziomie. E — porozmawiaj.",
          "Witek watches the relays. A green light on his panel means the lift is at this landing. Press E to talk."):
        iceberg::Npcs[size_t(node.id)].title.get(profile.language)+t(". E — rozmowa. Zlecenia i odebrane wiadomości znajdziesz w dzienniku.",". E — talk. Tasks and received messages are kept in your journal.");
      if(grid.template_id==24&&node.id==2)
        v.inspect_body=t("Bogdan liczy nawroty wagonu. Złota lampka wskazuje, po której stronie tramwaj stoi. E — porozmawiaj.",
          "Bogdan counts the tram's trips. The gold lamp marks the side where the tram is stopped. Press E to talk.");
      if(grid.template_id==44&&node.id==4)
        v.inspect_body=t("Irena porównuje spis lokatorów z odpisem. Obok jest przekaźnik taśmy. E — porozmawiaj.",
          "Irena is checking the tenant register against a carbon copy. The belt relay is nearby. Press E to talk.");
      if(grid.template_id==iceberg::ArchiveGlassRoom&&node.id==4)
        v.inspect_body=t("Irena przytrzymuje otwarty spis pustą kartką. Na widocznej stronie jest data powodzi. E — porozmawiaj.",
          "Irena holds the register open with a blank sheet. The visible page is dated the night of the flood. Press E to talk.");
      v.inspect_icon="◉";break;
    case InteractionKind::DockWorker:
      v.inspect_title=t("Doker","Dockworker");
      v.inspect_body=t("Sortuje skrzynki. E — porozmawiaj.","Sorting crates. E — talk.");break;
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
    case InteractionKind::SurveyChart:
      v.inspect_title=t("Mapa portu","Harbor chart");v.inspect_art=Art::Chart;v.inspect_detail=bool(grid.switch_flags&1);
      v.inspect_body=grid.switch_flags&1?
        t("Pozycja trzeciej boi jest już zaznaczona. Drzwi na końcu pomostu prowadzą z powrotem do przystani.",
          "The third buoy is already marked. The door at the end of the platform leads back to the harbor."):
        t("Trzecia boja zgasła. Zamknij oględziny, a potem naciśnij E przy mapie, żeby oznaczyć boję i otworzyć drogę powrotną.",
          "Buoy 3 is dark. Close this panel, then press E at the chart to mark it and open the way back.");
      break;
    case InteractionKind::Valve:
      v.inspect_art=Art::Valve;v.inspect_icon="⊗";
      if(grid.belt_relay(uint64_t(1)<<node.id)) {
        v.inspect_title=t("Przekaźnik taśmy","Conveyor relay");
        v.inspect_body=t("Naciśnij E, żeby zmienić kierunek środkowej taśmy. Biegnij z taśmą i odbij się z jej końca.",
          "Press E to reverse the middle belt. Run with the belt and jump off its end.");
        break;
      }
      if(grid.district==1&&grid.template_id==20) {
        v.inspect_body=t("Otwórz dopływ i wskocz w strumień pary. Nad górną półką skręć w lewo.",
          "Open the supply and jump into the steam. Steer left above the upper shelf.");
        break;
      }
      if(grid.district==3&&!grid.growth.empty()) {
        v.inspect_body=t("Pociągnij za uchwyt, aby wysunąć dwie szuflady. Po nich wskoczysz na górne półki.",
          "Pull the handle to slide out two drawers. Use them as steps to reach the upper shelves.");
        break;
      }
      v.inspect_body=t("Korba steruje pobliską maszyną. Ciemne koło jest zamknięte; jasny środek oznacza otwarty obieg. Stan mechanizmu pozostaje zapisany po powrocie do lampy.","The crank controls the nearby machinery. A dark wheel is closed; a bright centre marks an open circuit. The mechanism stays set after a checkpoint retry.");break;
    case InteractionKind::Note:
      v.inspect_art=node.id==30?Art::Dedication:(node.id<10||node.id==20||node.id==21)?Art::Sign:Art::File;v.inspect_title=node.id==30?t("Akt fundacyjny","Dedication"):t("Tablica informacyjna","Information sign");
      if(node.id==40&&grid.template_id==312) {
        v.inspect_title=t("Manometry","Pressure gauges");v.inspect_art=Art::Thermometer;
        v.inspect_body=t("Dopływ: 16. Powrót mieszkań: 8. Najpierw otwórz obejście, potem uruchom pompę. Ciepłą wodę skieruj do znaku domu; zrzut z falą prowadzi do morza. Odczyt zapisany.",
          "Inlet: 16. Homes' return: 8. Open the bypass before starting the pump. Send warm water to the house symbol; the wave outlet drains into the sea. Reading recorded.");
        grid.switch_flags|=16;persist();
      }
      else if(node.id==30){auto lines=iceberg::jubilee_dedication();v.inspect_body=lines[0].text.get(profile.language)+" "+lines[1].text.get(profile.language);}
      else if(node.id==20||node.id==21)v.inspect_body=iceberg::garden_sign(node.id).get(profile.language);
      else if(node.id==22)v.inspect_body=iceberg::crown_sign().get(profile.language);
      else if(node.id==iceberg::CrownShaftPlate) {
        v.inspect_title=node.label.get(profile.language);
        v.inspect_art=Art::Sign;
        v.inspect_body=iceberg::CrownShaftDirections.get(profile.language);
      }
      else if(node.id==iceberg::IrrigationCallBox) {
        v.inspect_title=node.label.get(profile.language);
        v.inspect_art=Art::Speaker;
        v.inspect_body=iceberg::IrrigationCallBoxDetail.get(profile.language);
      }
      else if(node.id==iceberg::ArchiveTrolleyNotice||node.id==iceberg::ArchiveDryingLetter) {
        const bool notice=node.id==iceberg::ArchiveTrolleyNotice;
        v.inspect_title=node.label.get(profile.language);
        v.inspect_art=notice?Art::Sign:Art::Parcel;
        v.inspect_body=(notice?iceberg::ArchiveTrolleyDirections:iceberg::ArchiveDryingLetterDetail).get(profile.language);
      }
      else if(node.id<10)v.inspect_body=iceberg::training_sign(node.id,grid.template_id).get(profile.language);
      else v.inspect_body=t("Kartka zachowała się za osłoną. Możesz odczytać ją w całości, naciskając E.","The sheet survived behind its cover. Press E to read the complete record.");
      break;
    case InteractionKind::Epilogue:
      v.inspect_art=node.id==0?Art::Seeds:Art::File;
      v.inspect_body=node.id==0?t("Ziemia w skrzynce jest wilgotna. Ciepła rura biegnie tuż pod dnem. E — posadź nasiona.","The soil in the box is damp. A warm pipe runs just beneath its base. E — plant the seeds."):t("Nowy spis leży przy publicznym okienku. Jest w nim miejsce na dopiski. E — doręcz odpis.","The new register lies at the public counter. It has room for additions. E — deliver the copy.");break;
    case InteractionKind::Evidence:
      v.inspect_art=Art::File;v.inspect_body=t("Stary plan sieci, oprawiony między dwie płyty szkła. Przy pieczęci pozostała data sprzed przebudowy. Irena prosiła o cały arkusz.","An old network plan held between two glass plates. The date beside its seal predates the reconstruction. Irena asked for the whole sheet.");break;
    case InteractionKind::Deliver:
      v.inspect_art=Art::Delivery;v.inspect_body=t("Gniazdo pomiarowe przyjmuje skrzynkę z termometrem. Zatrzaski utrzymują ją pionowo podczas odczytu.","The measuring socket accepts the thermometer case. Its catches keep the case upright during the reading.");break;
    case InteractionKind::RouteChoice:
      v.inspect_art=Art::Door;v.inspect_body=t("Przejście B prowadzi przez: ","Passage B leads through: ")+node.label.get(profile.language)+". "+
        t("Obie drogi łączą się w następnym wspólnym pokoju. E wybiera B; zwykłe oświetlone drzwi prowadzą trasą główną.","Both paths rejoin at the next shared room. E chooses B; the usual lit doorway takes the main path.");break;
    case InteractionKind::Replay:
      if(node.id==iceberg::SurveyReplay) {
        const auto count=iceberg::survey_pass(profile)?iceberg::SurveyTokenFare:
          iceberg::transit_token_progress(profile,iceberg::SurveyTokenFare);
        v.inspect_art=Art::Coin;
        v.inspect_body=t("Na końcu falochronu zgasła boja. Trasa prowadzi przez trzy kołyszące się pomosty. Przepustka: ",
          "A buoy has gone dark at the end of the breakwater. Three moving platforms lead out to it. Survey pass: ")+
          std::to_string(count)+"/"+std::to_string(iceberg::SurveyTokenFare)+t(" żetony."," tokens.");
      }
      break;
    case InteractionKind::Outfitter:
      v.inspect_art=Art::Coin;v.inspect_body=t("Kup wyposażenie za żetony. Zebrane żetony nadal liczą się do obu wejściówek.",
        "Buy gear with tokens. Spending them leaves your progress toward both passes intact.");break;
    case InteractionKind::Board:
      v.inspect_art=Art::Board;v.inspect_body=grid.template_id==330?node.label.get(profile.language):iceberg::route_title(district,stage).get(profile.language);break;
    default:v.inspect_body=t("E — użyj. Możesz zamknąć ten widok bez wykonywania działania.","E — use. You can close this view without performing the action.");break;
    }
    v.inspecting=true;discard_pending_movement();audio.play(AudioCue::Paper);
  }
#include "iceberg_harbor_runtime.inc"
  void interact() {
    int at=near_interaction();if(at<0)return;
    const auto node=grid.interactions[size_t(at)];
    if(choose_route(node)||harbor_interact(node)||side_story_interact(node))return;
    if(node.kind==InteractionKind::Outfitter) {offer_outfitter_item(0);return;}
    if(node.kind==InteractionKind::Epilogue) {
      if(grid.template_id!=330)return;
      auto bit=uint64_t(1)<<node.id;
      if(grid.switch_flags&bit){notice(t("Już załatwione.","Already handled."));return;}
      grid.switch_flags|=bit;animate_pickup(node,node.id==0?5:4);
      talk(iceberg::epilogue_action(node.id));audio.play(AudioCue::Cloth);persist();return;
    } else if(node.kind==InteractionKind::Npc) {
      state.adventure.reacting_npc=node.id;state.adventure.npc_reaction_left=1.2;
      if(grid.template_id==13&&node.id==1) {
        talk({iceberg::HoistCrewOpening},0,iceberg::HoistCrewReplyScene);
        return;
      }
      if(grid.template_id==24&&node.id==2) {
        talk({iceberg::EstateTramOpening},0,iceberg::EstateTramReplyScene);
        return;
      }
      if(grid.template_id==44&&node.id==4) {
        talk({iceberg::ArchiveCopiesOpening},0,iceberg::ArchiveCopiesReplyScene);
        return;
      }
      if(grid.template_id==iceberg::ArchiveGlassRoom&&node.id==4) {
        talk({iceberg::ArchiveShelterOpening},0,iceberg::ArchiveShelterReplyScene);
        return;
      }
      if(grid.template_id==330) {
        if(node.id==0&&(profile.story&iceberg::reply_recorded(6))) {
          talk({{0,(profile.story&iceberg::reply_value(6))!=0?
            iceberg::Text{"Przy decyzjach wiszą podpisy. Koperta z głosami została zamknięta. Pierwsza osoba już zapytała, czy może odsłuchać własną odpowiedź.","The signed orders are up. The envelope with the voices is still sealed. The first person has already asked if she can hear her own answer."}:
            iceberg::Text{"Jedno zeznanie ma już podpis, dwa wciąż czekają. Najdłuższa kolejka nie jest do okienka, tylko do krzesła po tej stronie.","One testimony is signed, and two are still waiting. The longest queue isn't for the counter. It's for the chair on this side."}}});
        } else talk({iceberg::epilogue_resident(node.id)});
        return;
      }
      if(mode==iceberg::Mode::Campaign && profile.visit_district<0 && stage==0 && !(profile.story&(uint64_t(1)<<district))) {
        talk_briefing();return;
      }
      int quest=-1;
      for(size_t i=0;i<iceberg::Quests.size();++i)if(iceberg::Quests[i].owner==node.id){quest=int(i);break;}
      if(quest>=0 && profile.quests[size_t(quest)]==2)
        talk(iceberg::task_thanks(quest),200+quest);
      else talk(iceberg::small_talk(node.id,quest<0?0:profile.quests[size_t(quest)]),quest>=0&&profile.quests[size_t(quest)]==0?100+quest:0);
    } else if(node.kind==InteractionKind::Rescue) {
      if(!(grid.switch_flags&1)){talk({{2,{"Najpierw wyciąg. Czerwona korba, dwa pomosty wyżej. To ten element, który wygląda, jakby od dawna chciał się urwać.","The winch first. Red crank, two landings above. The bit that looks as though it's wanted to fall off for years."}}});return;}
      if(!(grid.switch_flags&2)){grid.switch_flags|=2;activate_machinery(node,true);state.adventure.machinery_kind=4;state.adventure.machinery_x=166;talk(iceberg::rescued_conductor());persist();}
    } else if(node.kind==InteractionKind::Carry) {
      if(!(grid.switch_flags&1)) {
        animate_pickup(node,0);grid.switch_flags|=1;state.player.dash_time_left=state.player.dash_buffer_left=state.player.momentum_left=0;
        state.player.vx=clamp(state.player.vx,-kMaxRunSpeed,kMaxRunSpeed);
        notice(t("TERMOMETR: skacz ostrożnie. Zryw wróci po dostawie.","THERMOMETER: jump carefully. Dashing returns after delivery."),6);audio.play(AudioCue::Pickup);persist();
      }
    } else if(node.kind==InteractionKind::Deliver) {
      if(!(grid.switch_flags&1)){notice(t("Najpierw zabierz skrzynkę przy wejściu.","First collect the box by the entrance."),5);return;}
      if(!(grid.switch_flags&2)){animate_pickup(node,9);grid.switch_flags|=2;talk(iceberg::parcel_delivered());audio.play(AudioCue::Cloth);audio.play(AudioCue::Win);persist();}
    } else if(node.kind==InteractionKind::Evidence) {
      if((grid.switch_flags&3)!=3){notice(t("Najpierw otwórz szafę planów.","Open the map cabinet first."));return;}
      if(!(grid.switch_flags&4)){animate_pickup(node,4);grid.switch_flags|=4;profile.story|=uint64_t(1)<<41;talk(iceberg::archive_record());audio.play(AudioCue::Paper);persist();}
    } else if(node.kind==InteractionKind::Replay) {
      if(node.id==iceberg::SurveyReplay&&!iceberg::survey_pass(profile)) {
        const auto count=iceberg::transit_token_progress(profile,iceberg::SurveyTokenFare);
        notice(t("Przepustka na objazd: ","Survey pass: ")+std::to_string(count)+"/"+
          std::to_string(iceberg::SurveyTokenFare)+t(" żetony"," tokens"),5);
        return;
      }
      audio.play(AudioCue::Door);persist();profile.revisit={};profile.revisit.active=true;
      profile.revisit.district=grid.district;profile.revisit.scene=node.id;
      load_room();if(node.id==2&&!(profile.story&(uint64_t(1)<<40)))talk(iceberg::jubilee_arrival());persist();return;
    } else if(node.kind==InteractionKind::Shortcut) {
      room=std::min(iceberg::RoomsPerLeg-1,room+3);clear_room_record();load_room();persist();
      notice(node.id==2?t("BOGDAN: Trzy przystanki bez zatrzymania. Bilet proszę zachować.","BOGDAN: Three stops without stopping. Keep your ticket."):t("WITEK: Uważaj na głowę. To nie był projekt dla ludzi.","WITEK: Mind your head. This wasn't designed for people."),5);return;
    } else if(node.kind==InteractionKind::Return) {
      advance_room();return;
    } else if(node.kind==InteractionKind::Secret) {
      if(mode!=iceberg::Mode::Campaign)return;
      if(node.id==iceberg::UnnumberedFloor) {
        using iceberg::SideFlag;
        if(!has_side_flag(SideFlag::EstatePlate)||(!has_side_flag(SideFlag::HarborStub)&&!has_side_flag(SideFlag::ArchiveCopy))) {
          notice(t("Zamek bez klucza. Na blaszce widać dwie nakładające się linie.", "No keyhole. Two lines overlap on the plate."),5);
          return;
        }
        unlock_achievement(iceberg::Achievement::LostFloor);
      } else if(node.id==iceberg::NightPlatform) {
        if(!has_side_flag(iceberg::SideFlag::ArchiveCarbonPrinted))return;
        unlock_achievement(iceberg::Achievement::PlatformFound);
        audio.play(AudioCue::Door,.65f);
      } else if(node.id>0&&!(profile.discoveries&(1u<<(node.id-1))))return;
      persist();profile.hidden_room=node.id;load_room();persist();return;
    } else if(node.kind==InteractionKind::Board) {
      advance_room();
    } else if(node.kind==InteractionKind::SurveyChart) {
      if(!(grid.switch_flags&1)) {
        grid.switch_flags|=1;
        animate_pickup(node,0);
        notice(t("Pozycja boi zaznaczona na mapie.","Buoy position marked on the chart."));
        persist();
      }
    } else if(node.kind==InteractionKind::Valve) {
      const auto flag=uint64_t(1)<<node.id;
      if(const auto *belt=grid.belt_relay(flag)) {
        grid.switch_flags^=flag;
        const bool right=belt->speed_at(grid.switch_flags)>0;
        unlock_achievement(iceberg::Achievement::BeltReverse);
        notice(right?t("Środkowa taśma: →","Middle belt: →"):t("Środkowa taśma: ←","Middle belt: ←"));
        audio.play(AudioCue::Valve);
        persist();
        return;
      }
      if(grid.template_id==303&&node.id>0&&!(grid.switch_flags&(flag>>1))) {
        notice(t("Najpierw odłącz wcześniejszy obieg. Strzałki prowadzą od fontanny do mieszkań.","Disconnect the preceding circuit first. Follow the arrows from fountain to homes."),5);return;
      }
      if(grid.switch_flags&flag){notice(t("Obieg już pracuje.","The circuit is already running."));return;}
      animate_pickup(node,1);
      grid.switch_flags|=flag;
      for(auto &bridge:grid.growth)if(bridge.flag&flag)bridge.amount=0;
      const auto opened_supply=std::any_of(grid.air_fields.begin(),grid.air_fields.end(),[&](const auto &field){
        return (field.enabled_by&flag)&&field.active(grid.switch_flags)&&!field.active(grid.switch_flags&~flag);
      });
      activate_machinery(node,grid.template_id==77&&grid.switch_flags==7,opened_supply);
      audio.play(AudioCue::Valve);
      notice(grid.district==3
        ?grid.growth.empty()?t("ZASUWA OTWARTA","SHUTTER RELEASED"):t("SZUFLADY WYSUNIĘTE","DRAWERS RELEASED")
        :t("OBIEG OTWARTY","CIRCUIT OPEN"));
      persist();
    } else if(node.kind==InteractionKind::Task) {
      int bit=node.detail;auto mask=uint64_t(1)<<(bit%64);
      if(!(profile.task_items[size_t(bit/64)]&mask)) {
        animate_pickup(node,node.id);
        if(node.id==1||node.id==2||node.id==7)activate_machinery(node);
        profile.task_items[size_t(bit/64)]|=mask;
        ++profile.quest_counts[size_t(node.id)];
        if(profile.quests[size_t(node.id)]==1 && profile.quest_counts[size_t(node.id)]>=unsigned(iceberg::Quests[size_t(node.id)].required))profile.quests[size_t(node.id)]=2;
        notice(iceberg::Quests[size_t(node.id)].title.get(profile.language)+"  "+std::to_string(profile.quest_counts[size_t(node.id)])+"/3");
        audio.play(node.id==1||node.id==2?AudioCue::Valve:node.id==7?AudioCue::Voice:AudioCue::Pickup);persist();
      } else notice(t("Już zabrane. Wróć do zleceniodawcy.","Already collected. Return to the task giver."));
    } else if(node.kind==InteractionKind::Note) {
      if(node.id==30){talk(iceberg::jubilee_dedication());return;}
      if(node.id==20||node.id==21){talk({{5,iceberg::garden_sign(node.id)}});return;}
      if(node.id==22){talk({{6,iceberg::crown_sign()}});return;}
      if(node.id==iceberg::CrownShaftPlate){talk(iceberg::crown_shaft_plate());return;}
      if(node.id==iceberg::ArchiveTrolleyNotice){talk(iceberg::archive_trolley_notice());return;}
      if(node.id==iceberg::ArchiveDryingLetter){talk(iceberg::archive_drying_letter());return;}
      if(node.id==iceberg::IrrigationCallBox){talk(iceberg::irrigation_call(),0,iceberg::IrrigationReplyScene);return;}
      if(node.id>=10&&node.id<=12) {
        profile.discoveries|=1u<<(node.id-10);
        unlock_achievement(iceberg::Achievement::FirstTestimony);
        if((profile.discoveries&7)==7)unlock_achievement(iceberg::Achievement::ThreeTestimonies);
        talk(iceberg::hidden_testimony(node.id-10));persist();return;
      }
      talk({{6,iceberg::training_sign(node.id,grid.template_id)}});
    }
    sync_view();
  }
  int repeated_navigation(const Controls &c,double dt) {
    if(c.navigate) {
      // Basic terminal events have no held state and keep their host repeat.
      navigation_repeat_direction=c.navigate==c.aim_y?c.navigate:0;
      navigation_repeat_left=.35;
      return c.navigate;
    }
    if(navigation_repeat_direction&&(navigation_repeat_left-=dt)<=0) {
      navigation_repeat_left=.1;
      return navigation_repeat_direction;
    }
    return 0;
  }
  void menu(Controls c,double dt) {
    auto &v=state.adventure;
    if(v.save_picker||v.tour_picker) {
      const int count=v.save_picker?iceberg::SaveStore::SlotCount+1:int(iceberg::DebugTour.size());
      if(c.pause||c.menu_request) {
        if(v.tour_picker){v.tour_picker=false;v.save_picker=true;v.picker_choice=iceberg::SaveStore::SlotCount;refresh_slots();}
        else {v.save_picker=false;v.settings=true;}
        return;
      }
      if(v.tour_picker&&c.new_run) {
        start_tour(int(iceberg::DebugTourLatest));
        return;
      }
      const int direction=v.tour_picker?repeated_navigation(c,dt):c.navigate;
      if(direction) {
        const int next=v.picker_choice+direction;
        v.picker_choice=c.navigate?(next+count)%count:std::clamp(next,0,count-1);
      }
      if(c.select_level>=1&&c.select_level<=count)v.picker_choice=c.select_level-1;
      if(v.tour_picker&&c.tour_page)
        v.picker_choice=std::clamp(v.picker_choice+c.tour_page*iceberg::tour_page_size(last_rows),0,count-1);
      if(v.tour_picker&&c.browse_leg)v.picker_choice=c.browse_leg<0?0:count-1;
      if(c.confirm) {
        if(v.tour_picker)start_tour(v.picker_choice);
        else if(v.picker_choice<iceberg::SaveStore::SlotCount)switch_slot(v.picker_choice+1);
        else {v.save_picker=false;v.tour_picker=true;v.picker_choice=0;}
      }
      return;
    }
    if(v.achievement_menu) {
      if(c.pause||c.menu_request||c.confirm){v.achievement_menu=false;return;}
      const int groups=int(iceberg::AwardGroups.size()),group_step=c.inspect?1:c.browse_leg;
      if(group_step) {
        v.achievement_group=iceberg::AwardGroup((int(v.achievement_group)+group_step+groups)%groups);
        v.achievement_selection=0;
        audio.play(AudioCue::Menu,.5f);
      }
      const int count=int(iceberg::award_entries(v.achievement_group).size());
      const int page=c.tour_page*std::max(1,last_rows-13);
      v.achievement_selection=std::clamp(v.achievement_selection+repeated_navigation(c,dt)+page,0,count-1);
      return;
    }
    if(v.settings) {
      if(c.select_level==6){v.settings=false;v.save_picker=true;v.picker_choice=save_slot-1;refresh_slots();return;}
      float *volume=c.select_level==1?&profile.music_volume:
        c.select_level==2?&profile.effects_volume:c.select_level==3?&profile.ambience_volume:nullptr;
      if(volume){*volume=std::round((*volume+.1f)*10)/10;if(*volume>1)*volume=0;persist();}
      if(c.select_level==4) {
        for(int i=0;i<8;++i){profile.outfit=(profile.outfit+1)%8;if(profile.outfit==0||(profile.cosmetics&(1u<<profile.outfit)))break;}
        persist();
      }
      if(c.select_level==5){profile.pause_on_focus_loss=!profile.pause_on_focus_loss;persist();}
      if(c.select_level==7){profile.smooth_camera=!profile.smooth_camera;persist();}
      if(c.language){profile.language=profile.language==iceberg::Language::Polish?iceberg::Language::English:iceberg::Language::Polish;persist();}
      if(c.confirm||c.pause){v.settings=false;v.selected=4;}
      return;
    }
    if(v.confirm_new) {
      if(c.pause||c.menu_request)v.confirm_new=false;
      else if(v.selected==1?c.confirm:c.interact) {
        // A held Enter can repeat in a basic terminal. Use a separate key to replace a run.
        v.confirm_new=false;
        if(v.selected==1)begin_campaign(true);
        else begin_run(v.selected==2?iceberg::Mode::Endless:iceberg::Mode::Trial,true);
      }
      return;
    }
    int old_selected=v.selected;
    if(c.map && (v.selected==2||v.selected==3))run_district=(run_district+1)%(expedition_unlocked()+1);
    if(c.navigate)v.selected=(v.selected+c.navigate+7)%7;
    if(c.select_level>=1&&c.select_level<=7)v.selected=c.select_level-1;
    if(v.selected!=old_selected){choose_saved_run_district(v.selected);audio.play(AudioCue::Menu);}
    if(c.language){profile.language=profile.language==iceberg::Language::Polish?iceberg::Language::English:iceberg::Language::Polish;persist();}
    if((c.restart||c.new_run) && (v.selected==2||v.selected==3)){request_run(v.selected==2?iceberg::Mode::Endless:iceberg::Mode::Trial,true);return;}
    if(!c.confirm)return;
    int selected=v.selected;
    if(selected==0)begin_campaign(false);
    if(selected==1){if(profile.started)v.confirm_new=true;else begin_campaign(true);}
    if(selected==2||selected==3)request_run(selected==2?iceberg::Mode::Endless:iceberg::Mode::Trial);
    if(selected==4){v.settings=!v.settings;}
    if(selected==5){persist();legacy_requested=true;running=false;}
    if(selected==6){v.achievement_menu=true;v.achievement_group=iceberg::AwardGroup::All;v.achievement_selection=0;}
  }
  void tick(Controls c,double dt,bool focused=true,int columns=110,int rows=34,double aspect=2) {
    dt=clamp(dt,0.,.1);
    if(columns!=last_columns||rows!=last_rows||aspect!=last_aspect) {
      if(columns!=last_columns||rows!=last_rows)reflow_dialogue(columns,rows);
      update_camera(state,grid,columns,std::max(1,rows-7),aspect,10,profile.smooth_camera);
      last_columns=columns;last_rows=rows;last_aspect=aspect;
      frame_dialogue();
    }
    struct SendFrame {
      AdventureSession &session;double dt;bool focused;int columns,rows;
      ~SendFrame(){session.network_frame(dt,focused,columns,rows);}
    } send_frame{*this,dt,focused,columns,rows};
    audio.set_mix(profile.music_volume,profile.effects_volume,profile.ambience_volume);
    audio.set_environment(grid.district,grid.room_role==iceberg::RoomRole::Secret);
    const bool climax=(grid.template_id==83||grid.template_id==iceberg::CrownSentinel)&&!state.menu;
    int shutters=int(bool(grid.switch_flags&1))+int(bool(grid.switch_flags&2))+int(bool(grid.switch_flags&4));
    audio.set_climax(climax,.28f+float(shutters)*.24f);
    if(c.quit){persist();running=false;return;}
    const auto &film=state.adventure.dialogue;
    if(!film.cinematic||!film.active()||c.menu_request)
      audio.film_voice(-1,false,0.,false);
    if(c.mute_audio)audio.muted=!audio.muted;
    if(c.language&&!state.menu){profile.language=profile.language==iceberg::Language::Polish?iceberg::Language::English:iceberg::Language::Polish;if(state.adventure.dialogue.active()){state.adventure.dialogue.segment=0;state.adventure.dialogue.line_seconds=0;state.adventure.dialogue.reset_reveal();frame_dialogue();}persist();}
    auto &v=state.adventure;
    if((!focused||c.focus_lost)&&previous_focus){
      // Maps, the quest book and inspections already stop the world.
      if(profile.pause_on_focus_loss&&!state.menu&&!state.journal&&!v.map&&!v.inspecting)
        state.paused=true;
      discard_pending_movement();
    }
    previous_focus=focused;
    const bool tour_picker=state.menu&&v.tour_picker;
    const bool expedition_timeline=!state.menu&&v.map&&!v.map_room&&mode!=iceberg::Mode::Campaign;
    if(!focused||(!tour_picker&&!expedition_timeline&&!(state.menu&&v.achievement_menu))||c.map||c.journal||c.pause||c.menu_request||
       c.inspect||(tour_picker&&(c.confirm||c.select_level||c.browse_leg||c.tour_page||c.new_run))||
       c.aim_y!=navigation_repeat_direction)navigation_repeat_direction=0;
    if(!focused)c={};
    v.notice_left=std::max(0.,v.notice_left-dt);
    if(!state.paused&&!state.menu&&!v.inspecting&&!state.journal&&!v.map) {
      v.presentation_time+=dt;v.npc_reaction_left=std::max(0.,v.npc_reaction_left-dt);
      v.death_left=std::max(0.,v.death_left-dt);
      v.machinery_left=std::max(0.,v.machinery_left-dt);v.lamp_left=std::max(0.,v.lamp_left-dt);
      if(v.dialogue.active())v.pickup_left=std::max(0.,v.pickup_left-dt);
    }
    if(state.menu) {menu(c,dt);sync_view();audio.update(AudioScene::Menu,2,dt,focused);return;}
    if(c.menu_request&&tour_scene>=0){end_tour();return;}
    if(c.menu_request){
      persist();state.menu=true;v.inspecting=false;v.dialogue={};discard_pending_movement();
      select_resume_option();sync_view();return;
    }
    if(v.inspecting) {
      auto layout=iceberg::inspection_layout(v.inspect_title,v.inspect_body,columns,rows);
      const int page=layout.page_at(v.inspect_offset);
      const bool advance=c.confirm||c.interact;
      if(c.inspect||c.pause||(advance&&page+1==layout.pages))v.inspecting=false;
      else {
        const int selected=std::clamp(page+(advance?1:c.navigate),0,layout.pages-1);
        if(selected!=page)v.inspect_offset=layout.page_starts[size_t(selected)];
      }
      audio.update(AudioScene::Paused,music_track(),dt,focused);sync_view();return;
    }
    const bool toggle_overlay=c.pause||c.map||c.journal;
    if(c.inspect&&!toggle_overlay&&!v.dialogue.active()&&!state.paused&&!state.journal&&!v.map) {
      inspect_nearby();sync_view();if(v.inspecting)return;
    }
    if(v.dialogue.active()&&state.journal&&!c.pause&&(c.map||c.journal))
      v.dialogue={}; // Let the requested overlay consume the input.
    if(v.dialogue.active()) {
      if(v.dialogue.outfitter_item>=0&&c.pause) {
        v.dialogue={};discard_pending_movement();sync_view();return;
      }
      const bool reading_journal=state.journal;
      if(!reading_journal&&state.paused&&(c.confirm||c.interact)){
        state.paused=false;audio.update(AudioScene::Paused,music_track(),dt,focused);return;
      }
      if(c.pause) {
        if(reading_journal)v.dialogue={};
        else state.paused=!state.paused;
      }
      if(v.dialogue.active()&&(!state.paused||reading_journal)) {
        if(v.dialogue.choosing()) {
          if(v.dialogue.cinematic&&(focused||!profile.pause_on_focus_loss))v.dialogue.shot_seconds+=dt;
          if(c.navigate)v.dialogue.reply_option=(v.dialogue.reply_option+1)%2;
          if(c.confirm||c.interact)dialogue_next();
        } else if(c.confirm||c.interact||(c.jump_pressed&&!v.dialogue.cinematic))dialogue_next();
        else if(focused||!profile.pause_on_focus_loss) {
          if(v.dialogue.cinematic)tick_crown_film(dt);
          else tick_dialogue(dt);
        }
      }
      if(v.dialogue.cinematic&&v.dialogue.active()&&v.dialogue.lines[v.dialogue.page].speaker!=-1)
        audio.film_voice(v.dialogue.lines[v.dialogue.page].voice,profile.language==iceberg::Language::Polish,
          v.dialogue.line_seconds,!state.paused&&focused);
      else audio.film_voice(-1,false,0.,false);
      audio.update(v.dialogue.cinematic&&!state.paused?AudioScene::Play:AudioScene::Paused,
                   v.dialogue.cinematic?2:music_track(),dt,focused);sync_view();return;
    }
    const bool was_in_overlay=state.paused||state.journal||v.map;
    if(toggle_overlay) {
      if(c.pause) {
        if(state.journal||v.map){state.journal=false;v.map=false;}
        else state.paused=!state.paused;
      } else if(c.map) {
        v.map=!v.map;state.journal=false;v.selected=grid.district;
        if(v.map) {
          v.map_room=true;
          v.map_route=mode!=iceberg::Mode::Campaign&&tour_scene<0;
          v.route_room_selection=-1;
        }
      } else {
        state.journal=!state.journal;v.map=false;v.selected=0;
        const auto entries=iceberg::journal_entries(v);
        for(size_t i=0;i<entries.size();++i)if(entries[i]==v.quest_focus)v.selected=int(i);
      }
      // Inputs captured before the overlay changed belong to the old view.
      discard_pending_movement();
      audio.update(state.paused||state.journal||v.map?AudioScene::Paused:AudioScene::Play,music_track(),dt,focused);
      sync_view();
      return;
    }
    const bool view_too_small=columns<34||rows<(columns>=56?20:24);
    const bool retry_from_pause=!view_too_small&&state.paused&&!state.journal&&!v.map&&(c.restart||c.new_run);
    if(state.paused&&!state.journal&&!v.map) {
      if(retry_from_pause) {
        state.paused=false;
        // Retry uses the normal checkpoint path without passing Pause input to the world.
        c={};
        c.restart=true;
      } else if(c.confirm)state.paused=false;
      else if(tour_scene>=0&&c.navigate) {
        const int count=int(iceberg::DebugTour.size());
        load_tour_scene((tour_scene+c.navigate+count)%count);
        state.paused=true;sync_view();
        return;
      }
    }
    if(state.journal) {
      if(c.inspect){v.journal_tab=(v.journal_tab+1)%4;v.selected=0;v.achievement_selection=0;}
      if(v.journal_tab==3&&c.navigate)
        v.achievement_selection=std::clamp(v.achievement_selection+c.navigate,0,int(iceberg::Achievements.size())-1);
      auto entries=iceberg::journal_entries(v);
      if(!entries.empty()) {
        int count=int(entries.size());v.selected=std::clamp(v.selected,0,count-1);
        if(c.navigate)v.selected=(v.selected+c.navigate+count)%count;
        int selected=entries[size_t(v.selected)];
        if(c.interact||c.confirm) {
          if(selected>=100){talk(selected==200?iceberg::archive_record():iceberg::hidden_testimony(selected-100));sync_view();return;}
          if(selected>=0&&profile.quests[size_t(selected)]==3){talk(iceberg::task_record(selected));sync_view();return;}
          profile.quest_focus=v.quest_focus=selected;persist();
          notice(selected<0?t("Wybrano główną podróż", "Journey selected"):iceberg::Quests[size_t(selected)].title.get(profile.language));
        }
      }
    }
    if(v.map&&c.inspect&&tour_scene<0) {
      if(mode==iceberg::Mode::Campaign) {
        const bool next_room=v.map_route;
        v.map_route=!v.map_route&&!v.map_room;
        v.map_room=next_room;
      } else v.map_room=!v.map_room;
    }
    if(v.map&&!v.map_room&&!c.inspect&&mode!=iceberg::Mode::Campaign&&!v.route.rooms.empty()) {
      const auto [selected,count]=expedition_route_position();
      if(c.confirm) {
        v.route_room_selection=-1;
        navigation_repeat_direction=0;
      } else if(c.browse_leg&&mode==iceberg::Mode::Endless) {
        const int current_leg=selected/iceberg::RoomsPerLeg;
        const int last_leg=(count-1)/iceberg::RoomsPerLeg;
        const int target=std::clamp(current_leg+c.browse_leg,0,last_leg);
        if(target!=current_leg)
          v.route_room_selection=std::min(target*iceberg::RoomsPerLeg+selected%iceberg::RoomsPerLeg,count-1);
        navigation_repeat_direction=0;
      } else if(const int direction=repeated_navigation(c,dt)) {
        v.route_room_selection=std::clamp(selected+direction,0,count-1);
      }
    }
    if(v.map&&mode==iceberg::Mode::Campaign&&!v.map_room&&!c.inspect) {
      if(c.navigate&&!v.map_route)v.selected=(v.selected-c.navigate+iceberg::DistrictCount)%iceberg::DistrictCount;
      if(v.map_route&&v.route.available) {
        if(c.confirm)v.route_page=-1;
        else if(c.navigate)v.route_page=std::clamp((v.route_page<0?v.route.leg:v.route_page)+c.navigate,0,2);
      }
      // A selection change needs a fresh confirmation for the newly highlighted district.
      if(c.confirm&&!c.navigate&&!v.map_route&&iceberg::district_travel_available(v,v.selected)) {
        int destination=v.selected;persist();profile.visit_district=destination==district?-1:destination;
        load_room(profile.visit_district<0);persist();audio.play(AudioCue::Door);
        notice((profile.visit_district<0?t("POWRÓT NA TRASĘ — ","BACK ON YOUR ROUTE — "):t("PRZYJAZD — ","ARRIVED — "))+iceberg::Districts[size_t(destination)].name.get(profile.language),6);
      }
    }
    const bool blocked=state.paused||state.journal||v.map||view_too_small;
    audio.update(blocked?AudioScene::Paused:AudioScene::Play,music_track(),dt,focused);
    // Closing an overlay consumes this input frame. Its confirm or direction
    // may also be a doorway interaction or a movement action in the world.
    if(blocked||(was_in_overlay&&!retry_from_pause)){sync_view();return;}
    v.pickup_left=std::max(0.,v.pickup_left-dt);
    v.door_left=std::max(0.,v.door_left-dt);
    const bool at_door=state.won;
    const bool at_fork=v.branch_at_door;
    const bool first_exit=mode==iceberg::Mode::Campaign&&district==0&&stage<=1&&
      profile.hidden_room<0&&profile.visit_district<0&&!profile.revisit.active;
    if(at_door)v.door_open=std::min(1.,v.door_open+dt*4.5);
    c.restart=c.restart||c.new_run;
    const bool retrying=c.restart||stepper.pending.restart;
    if(tour_scene>=0&&retrying) {
      load_tour_scene(tour_scene,state.deaths+1);
      audio.play(AudioCue::Death);
      return;
    }
    const auto discoveries_at_start=profile.discoveries;
    const auto awards_at_start=iceberg::stored_award_progress(profile);
    const auto switches_at_start=grid.switch_flags;
    // A retry queued between physics steps still takes priority at a doorway.
    if(!retrying) {
      if(c.dash_pressed&&grid.dash_locked())notice(t("Przesyłka musi stać pionowo — skocz zamiast zrywu.","Keep the parcel upright — jump instead of dashing."));
      if(c.interact) {
        const auto previous_room=room_revision;
        interact();
        // A room-changing interaction consumes this frame's movement.
        if(room_revision!=previous_room||v.dialogue.active()||v.inspecting||state.menu){sync_view();return;}
      }
      if(state.won&&(c.confirm||c.interact||(first_exit&&c.move>0&&v.door_open>=1.))) {
        advance_room();
        sync_view();return;
      }
    }
    // Only start or age the toast on a frame that reaches gameplay.
    if(focused)advance_achievement_toast(dt);
    v.garden_snail_left=std::max(0.,v.garden_snail_left-dt);
    v.pressure_test_left=std::max(0.,v.pressure_test_left-dt);
    Player before=state.player;
    int old_deaths=state.deaths,old_pickups=state.sweets_count(),old_cp=state.checkpoint;
    int old_crown_hits=state.crown.hits;
    const auto old_crown_beat=crown_beat(state.crown);
    double old_elapsed=state.elapsed;
    // A newly opened bridge starts extending on the next playable frame.
    const auto active_bridges=switches_at_start&grid.switch_flags;
    for(auto &bridge:grid.growth)
      if((active_bridges&bridge.flag)==bridge.flag)
        bridge.amount=std::min(1.,bridge.amount+dt/1.15);
    stepper.advance(state,grid,c,dt);
    update_water_splash(before,old_deaths,dt);
    if(!state.won)v.door_open=std::max(0.,v.door_open-dt*4.5);
    feedback_step(state,grid,before,old_deaths,old_pickups,old_cp,false,old_elapsed,dt,audio);
    harbor_step(before,old_deaths);
    night_line_step(before,old_deaths,dt);
    night_platform_step(state.elapsed-old_elapsed);
    if(state.checkpoint!=old_cp)v.lamp_left=.8;
    // Spatially restrained machinery cues follow actual motion/timing.
    if(state.elapsed>old_elapsed&&state.deaths==old_deaths) {
      if(state.player.support>=0&&before.support!=state.player.support&&
         !grid.platforms[size_t(state.player.support)].fragile)audio.play(AudioCue::Lift,.7f);
      if(before.support>=0&&state.player.support<0&&state.player.vy<-10&&
         !grid.platforms[size_t(before.support)].fragile)audio.play(AudioCue::Lift,.45f);
      double piston_dock_gain=0.;
      const auto courier=state.player.rect();
      for(size_t i=0;i<grid.platforms.size();++i) {
        const auto &deck=grid.platforms[i];
        if(deck.pressure_piston||deck.seed_tray||deck.chute_leaf) {
          const int stop=deck.stop(state.elapsed);
          if(stop!=0&&stop!=deck.stop(old_elapsed)) {
            const auto platform=deck.at(state.elapsed);
            const double distance=std::hypot(platform.x+platform.w*.5-courier.x-courier.w*.5,
                                             platform.y-courier.bottom());
            const double gain=1.-distance/12.;
            if(deck.pressure_piston)piston_dock_gain=std::max(piston_dock_gain,gain);
            else if(gain>0.)audio.play(deck.chute_leaf?AudioCue::Cloth:AudioCue::Wood1,float(.7*gain));
          }
        }
        if(deck.swing_length>0.&&state.player.support==int(i)&&
           deck.stop(old_elapsed)!=1&&deck.stop(state.elapsed)==1)audio.play(AudioCue::Wood2,.7f);
        if(deck.fragile&&deck.available(state,i,old_elapsed)&&!deck.available(state,i,state.elapsed)&&
           std::hypot(deck.base.x-state.player.x,deck.base.y-state.player.y)<16)
          audio.play(AudioCue::Stone2,.65f);
      }
      // Both pistons dock together. The nearest one determines the cue.
      if(piston_dock_gain>0.)audio.play(AudioCue::Metal2,float(.7*piston_dock_gain));
      double valve_warning=0.;
      for(const auto &vent:grid.pulses) {
        if(!vent.powered(grid.switch_flags))continue;
        if(vent.active(state.elapsed)&&!vent.active(old_elapsed)) {
          const double distance=std::hypot(vent.area.x-state.player.x,vent.area.y-state.player.y);
          const auto cue=vent.kind==PulseKind::Flytrap?AudioCue::Wood2:
            vent.kind==PulseKind::Masonry?AudioCue::Stone2:
            (vent.kind==PulseKind::Scanner||vent.kind==PulseKind::Arc)?AudioCue::Spark:
            vent.kind==PulseKind::Surge?AudioCue::Rumble:AudioCue::Steam;
          if(distance<22)audio.play(cue,float(1-distance/22));
        } else if(vent.warning(state.elapsed)&&!vent.warning(old_elapsed)) {
          const double distance=std::hypot(vent.area.x+vent.area.w*.5-courier.x-courier.w*.5,
                                          vent.area.bottom()-courier.bottom());
          if(vent.kind==PulseKind::Flytrap) {
            if(distance<15.)audio.play(AudioCue::Cloth,float(.65*(1.-distance/15.)));
          } else if(vent.kind==PulseKind::Masonry) {
            if(distance<15.)audio.play(AudioCue::Stone1,float(.45*(1.-distance/15.)));
          } else if(vent.kind==PulseKind::Scanner||vent.kind==PulseKind::Arc) {
            if(distance<15.)audio.play(AudioCue::Metal1,float(.5*(1.-distance/15.)));
          } else if(vent.kind==PulseKind::Surge) {
            if(distance<15.)audio.play(AudioCue::Cloth,float(.4*(1.-distance/15.)));
          } else valve_warning=std::max(valve_warning,1.-distance/15.);
        }
      }
      // A nearby valve clicks once as pressure builds, before any damaging steam.
      if(valve_warning>0.)audio.play(AudioCue::Valve,float(.65*valve_warning));
    }
    if(state.sweets_count()>old_pickups&&v.pickup_left<=0) {
      v.pickup_kind=8;v.pickup_left=.65;v.pickup_x=state.player.x+.4;v.pickup_y=state.player.y-.8;
    }
    double active=state.elapsed-old_elapsed;
    int deaths=state.deaths-old_deaths,pickups=state.sweets_count()-old_pickups;
    if(deaths>0)v.death_left=retrying?0.:1.6;
    if(mode==iceberg::Mode::Campaign) {
      if(!profile.revisit.active) {
        profile.campaign_seconds+=active;
        profile.campaign_deaths+=deaths;
        if(profile.hidden_room<0) {
          profile.district_seconds+=active;
          profile.district_deaths+=deaths;
          profile.district_treasures+=pickups;
        }
      }
    } else {run_seconds+=active;run_deaths+=deaths;run_treasures+=pickups;}
    if(state.player.wall_bounce_left>before.wall_bounce_left) {
      unlock_achievement(iceberg::Achievement::WallBounce);audio.play(AudioCue::Bounce);
    }
    if(deaths>0&&!retrying) {
      unlock_achievement(iceberg::Achievement::FirstFall);
      if(state.deaths>=5)unlock_achievement(iceberg::Achievement::RoughRoom);
    }
    if(deaths>0&&!retrying&&state.deaths>=10)unlock_achievement(iceberg::Achievement::LongWayBack);
    if(deaths==0&&before.ledge_climb_left>0.&&!state.player.ledge&&state.player.grounded)
      unlock_achievement(iceberg::Achievement::LedgeClimb);
    if(deaths==0&&state.last_spring>=0&&state.last_spring_time>old_elapsed&&
       grid.springs[size_t(state.last_spring)].awning)
      unlock_achievement(iceberg::Achievement::MasonrySpring);
    if(before.vine<0&&state.player.vine>=0)unlock_achievement(iceberg::Achievement::VineRide);
    if(state.player.support>=0&&state.player.support!=before.support) {
      const auto &deck=grid.platforms[size_t(state.player.support)];
      if(deck.estate_balcony)unlock_achievement(iceberg::Achievement::BalconyRide);
      if(grid.template_id==504||(grid.template_id==601&&state.player.support==0))
        unlock_achievement(iceberg::Achievement::FerryRide);
    }
    const bool mastery_progress=state.player.dash_jump_left>before.dash_jump_left&&profile.quest_counts[6]<3;
    if(deaths==0&&state.airborne_refill_time>old_elapsed)
      unlock_achievement(iceberg::Achievement::AirRefill);
    if(pickups>0) {
      unlock_achievement(iceberg::Achievement::FirstToken);
      if(iceberg::outfitter_earned(profile)>=100)unlock_achievement(iceberg::Achievement::HundredTokens);
    }
    if(mastery_progress) {
      unlock_achievement(iceberg::Achievement::DashJump);
      ++profile.quest_counts[6];
      if(profile.quest_counts[6]>=3 && profile.quests[6]==1)profile.quests[6]=2;
    }
    int at=near_interaction();
    v.prompt=at>=0&&(grid.interactions[size_t(at)].kind!=InteractionKind::Secret||
      grid.interactions[size_t(at)].id>=iceberg::UnnumberedFloor)?"[E] "+grid.interactions[size_t(at)].label.get(profile.language):"";
    if(at>=0&&grid.interactions[size_t(at)].kind==InteractionKind::RouteChoice)v.prompt="[E] B: "+grid.interactions[size_t(at)].label.get(profile.language);
    if(at>=0&&grid.interactions[size_t(at)].kind==InteractionKind::SideStory&&
       grid.interactions[size_t(at)].id==int(iceberg::SideStory::RosterDesk)) {
      if(has_side_flag(iceberg::SideFlag::RosterPosted))v.prompt=t("[E] Przeczytaj sprostowanie","[E] Read the correction");
      if(has_side_flag(iceberg::SideFlag::RosterSealed))v.prompt=t("[E] Obejrzyj kopertę","[E] Look at the envelope");
    }
    if(at>=0&&grid.interactions[size_t(at)].kind==InteractionKind::SideStory) {
      using iceberg::SideStory;
      using iceberg::SideFlag;
      const auto prop=SideStory(grid.interactions[size_t(at)].id);
      if(prop==SideStory::HeatworksMugBench&&has_side_flag(SideFlag::HeatworksMugReturned))
        v.prompt=t("[E] Przeczytaj rozpiskę","[E] Read the shift rota");
      if(prop==SideStory::GardensPot&&has_side_flag(SideFlag::GardenSnailFound))
        v.prompt=t("[E] Obejrzyj doniczkę","[E] Inspect the pot");
      if(prop==SideStory::GardenSnailTap&&has_side_flag(SideFlag::GardenSnailWatered))
        v.prompt=t("[E] Obejrzyj kranik","[E] Inspect the garden tap");
    }
    if(at>=0&&grid.interactions[size_t(at)].kind==InteractionKind::Npc)v.prompt+=" — "+std::string(iceberg::Npcs[size_t(grid.interactions[size_t(at)].id)].name);
    if(at<0&&state.player.support>=0&&grid.platforms[size_t(state.player.support)].clothesline)
      v.prompt=t("↓ / S — zeskocz z liny","↓ / S — drop through the line");
    if(v.prompt.empty()&&grid.template_id==55&&state.player.grounded) {
      const Rect feet=state.player.rect();
      for(const auto &platform:grid.platforms) {
        if(!platform.icy||!platform.fragile)continue;
        const auto &ice=platform.base;
        if(feet.bottom()<=ice.top()&&ice.top()-feet.bottom()<=4.&&
           feet.right()+2.5>ice.left()&&feet.left()-2.5<ice.right()) {
          v.prompt=t("Cienki lód · Nie zatrzymuj się","Thin ice · Keep moving");
          break;
        }
      }
    }
    if(at_locked_survey_exit())
      v.prompt=t("Oznacz trzecią boję na mapie.","Mark buoy 3 at the chart.");
    if(grid.template_id==iceberg::CrownSentinel&&state.crown.hits<3) {
      const auto beat=crown_beat(state.crown);
      const std::string shield=std::to_string(3-state.crown.hits)+"/3 · ";
      v.prompt=shield+(beat==CrownBeat::Exposed?(state.crown.hits==2?
        t("Skocz i zrób zryw w rdzeń","Jump and dash through the core"):
        t("Zryw w odsłonięty rdzeń","Dash through the core")):
        beat==CrownBeat::Warning||beat==CrownBeat::Strike?t("Unikaj stref uderzenia","Dodge the marked zones"):
        t("Rdzeń osłonięty","Core shielded"));
      if(old_crown_beat!=beat) {
        if(beat==CrownBeat::Warning)audio.play(AudioCue::Motor,.55f);
        if(beat==CrownBeat::Strike)audio.play(AudioCue::Spark);
        if(beat==CrownBeat::Exposed)audio.play(AudioCue::Valve,.6f);
      }
    }
    autosave_left-=dt;
    const bool crown_advanced=state.crown.hits>old_crown_hits;
    const bool crown_victory=crown_advanced&&state.crown.hits==3;
    if(crown_victory&&tour_scene<0) {
      unlock_achievement(iceberg::Achievement::CrownDown);
      if(state.deaths>=5)unlock_achievement(iceberg::Achievement::CrownRecovery);
      profile.story|=iceberg::CrownSentinelDefeated;
      if(state.deaths==0)unlock_achievement(iceberg::Achievement::CrownClean);
    }
    if(crown_advanced||old_cp!=state.checkpoint||(state.won&&!at_door)||at_route_fork()!=at_fork||
       deaths>0||pickups>0||mastery_progress||profile.discoveries!=discoveries_at_start||
       iceberg::stored_award_progress(profile)!=awards_at_start||autosave_left<=0)persist();
    update_camera(state,grid,columns,std::max(1,rows-7),aspect,
      deaths>0?10.:dt,profile.smooth_camera);
    if(crown_advanced) {
      audio.play(AudioCue::Metal1);
      if(crown_victory) {
        audio.play(AudioCue::Win);
        show_crown_film(tour_scene>=0?0:2);
      }
    }
    sync_view();
  }
};
