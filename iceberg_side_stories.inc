  bool has_side_flag(iceberg::SideFlag flag) const {
    return (profile.task_items[1]&iceberg::side_flag(flag))!=0;
  }
  void remember_side_flag(iceberg::SideFlag flag) {
    profile.task_items[1]|=iceberg::side_flag(flag);
  }
  bool has_curio(iceberg::Curio curio) const {
    return (profile.task_items[0]&iceberg::curio_bit(curio))!=0;
  }
  void remember_curio(iceberg::Curio curio) {
    profile.task_items[0]|=iceberg::curio_bit(curio);
  }
  void place_side_stories() {
    using iceberg::SideStory;
    const auto add=[&](Rect area,SideStory story,iceberg::Text label) {
      grid.interactions.push_back({area,InteractionKind::SideStory,int(story),label});
    };
    if(grid.template_id==100||grid.template_id==600) {
      add({2,44,2,4},SideStory::HarborStub,{"Bloczek pocztowy","Postal counterfoil"});
      add({grid.template_id==600?54.:49.,35,3,4},SideStory::HarborTicket,{"Otwórz okienko zwrotów","Open the returns hatch"});
    } else if(grid.template_id==101) {
      add({29,44,3,4},SideStory::HeatworksMug,{"Zabierz kubek z rury","Take the mug from the pipe"});
      add({2,48,2,4},SideStory::HeatworksMugBench,{"Oddaj Witkowi kubek","Return Witek’s mug"});
    } else if(grid.template_id==102) {
      add({34,44,4,4},SideStory::EstatePlate,{"Blaszka pod schodami","Plate under the stairs"});
      add({104,30,4,4},SideStory::Casting,{"Podnieś odlew","Pick up the casting"});
      add({113,30,3,4},SideStory::EstateBirdhouse,{"Obejrzyj karmnik","Inspect the birdhouse"});
      if(has_side_flag(iceberg::SideFlag::Roster))
        add({60,44,4,4},SideStory::RosterDesk,{"Pokaż Grażynie listę","Show Grażyna the roster"});
      grid.interactions.push_back({{67,44,3,4},InteractionKind::Secret,iceberg::UnnumberedFloor,
        {"Drzwi pod schodami","Door under the stairs"}});
    } else if(grid.template_id==103) {
      add({33,44,4,4},SideStory::ArchiveCopy,{"Odpis z teczki 11","Copy from folder 11"});
      add({84,44,4,4},SideStory::EmergencyBell,{"Dzwonek alarmowy","Emergency bell"});
      add({103,44,3,4},SideStory::ArchiveLine,{"Podnieś słuchawkę linii 11","Pick up Line 11"});
    } else if(grid.template_id==104) {
      add({94,46,4,4},SideStory::GardenSnailTap,{"Podlej ziemię przy doniczce","Water the soil by the pot"});
      add({117,46,3,4},SideStory::GardenSnailShelter,{"Przeczytaj tabliczkę","Read the shelter tag"});
      add({104,46,4,4},SideStory::GardensPot,{"Podnieś odwróconą doniczkę","Lift the overturned pot"});
      add({124,35,3,4},SideStory::GardenButton,{"Podnieś guzik","Pick up the button"});
    } else if(grid.template_id==105) {
      add({97,30,4,4},SideStory::CrownPunch,{"Sprawdź kasownik","Test the ticket punch"});
    }
  }
  struct SideDocument {
    iceberg::SideFlag flag;
    iceberg::Text title, body;
  };
  static std::optional<SideDocument> side_document(iceberg::SideStory story) {
    using iceberg::SideFlag;
    using iceberg::SideStory;
    switch(story) {
      case SideStory::HarborStub:
        return SideDocument{SideFlag::HarborStub,{"Bloczek pocztowy","Postal counterfoil"},
          {"ZWROT 11 — dozorczyni: najpierw pociągnąć w dół, potem w lewo. Nie doręczać po 00:07.",
           "RETURN 11 — caretaker: pull down, then left. Do not deliver after 00:07."}};
      case SideStory::EstatePlate:
        return SideDocument{SideFlag::EstatePlate,{"Blaszka pod schodami","Plate under the stairs"},
          {"Pod numerem 11: rysa w dół, druga w lewo. Zawiasy są na ścianie obok.",
           "Under number 11: a scratch down, a second to the left. The hinges are on the wall beside it."}};
      case SideStory::ArchiveCopy:
        return SideDocument{SideFlag::ArchiveCopy,{"Odpis z teczki 11","Copy from folder 11"},
          {"TEKA 11 — zatarte polecenie: w dół, potem w lewo. Zlecenie wpłynęło po zakończeniu akcji.",
           "FOLDER 11 — erased instruction: down, then left. Order received after the operation ended."}};
      case SideStory::Roster:
        return SideDocument{SideFlag::Roster,{"Lista lokatorów","Tenant roster"},
          {"TRZYNAŚCIE NAZWISK. Czternasty wiersz jest wgnieciony od ołówka: «Zosia, zastępcza kołyska w pralni».",
           "THIRTEEN NAMES. Pencil marks dent the fourteenth line: ‘Zosia, spare cot in the laundry.’"}};
      default:return std::nullopt;
    }
  }
  iceberg::Text roster_desk_title() const {
    using iceberg::SideFlag;
    if(has_side_flag(SideFlag::RosterPosted))return {"Sprostowanie","Correction notice"};
    if(has_side_flag(SideFlag::RosterSealed))return {"Zapieczętowany odpis","Sealed copy"};
    return {"Lista lokatorów","Tenant roster"};
  }
  iceberg::Text roster_desk_body() const {
    using iceberg::SideFlag;
    if(has_side_flag(SideFlag::RosterPosted))
      return {"KLATKA 11 · Obecnych: 14. Drzwi zamknięto przed zakończeniem obchodu. Podpisano: Grażyna.",
              "STAIRWELL 11 · Present: 14. The door was shut before the count finished. Signed: Grażyna."};
    if(has_side_flag(SideFlag::RosterSealed))
      return {"Koperta jest zapieczętowana. Na wierzchu: «Do rąk własnych. Chcesz porozmawiać? Będę przy biurku. Grażyna».",
              "The envelope is sealed. On the front: ‘For you alone. If you want to talk, I'll be by my desk. Grażyna.’"};
    return {"E — pokaż Grażynie odpis znaleziony pod schodami.",
            "E — show Grażyna the copy you found below the stairs."};
  }
  bool side_story_available(const Interaction &node) const {
    if(node.kind!=InteractionKind::SideStory)return true;
    using iceberg::SideStory;
    using iceberg::SideFlag;
    switch(SideStory(node.id)) {
      case SideStory::Casting:return !has_side_flag(SideFlag::Casting);
      case SideStory::HeatworksMug:return !has_side_flag(SideFlag::HeatworksMugTaken);
      case SideStory::HeatworksMugBench:return has_side_flag(SideFlag::HeatworksMugTaken);
      case SideStory::GardenSnailShelter:return has_side_flag(SideFlag::GardenSnailWatered);
      case SideStory::GardenButton:return !has_curio(iceberg::Curio::GardenButton);
      default:return true;
    }
  }
  iceberg::Text garden_snail_pot_text() const {
    using iceberg::SideFlag;
    if(has_side_flag(SideFlag::GardenSnailWatered))
      return {"Doniczka jest pusta. Mokry ślad prowadzi w prawo, pod liście.",
              "The pot is empty. A damp trail leads right, under the leaves."};
    return {"Pod doniczką ślimak i tabliczka: «Najemca tymczasowy. Proszę podlewać obok».",
            "Under the pot, a snail and a sign: ‘Temporary tenant. Please water around it.’"};
  }
  static constexpr iceberg::Text GardenSnailTag{
    "Dziadek mówił, że ślimaki wracają tam, gdzie ostatnio znalazły wodę. Posadziłam tę funkię, żeby ten miał dokąd wrócić. — Celina",
    "Granddad said snails go back to the last place they found water. I planted this hosta so this one would have somewhere to come back to. — Celina"};
  static constexpr iceberg::Text HeatworksMugNote{
    "Zimna herbata. Na karteczce: «Zostaw ciepły obieg do końca nocnej zmiany. Witek nie śpi od wtorku».",
    "Cold tea. The note reads: ‘Leave the hot circuit on through the night shift. Witek hasn't slept since Tuesday.’"};
  static constexpr iceberg::Text HeatworksMugRota{
    "Przy parującym kubku leży rozpiska: «6:00 — Ania przejmuje obieg. Witek idzie do domu». Poniżej dwa podpisy.",
    "Beside the steaming mug is a rota: ‘6:00 — Ania takes the circuit. Witek goes home.’ Two signatures underneath."};
  void night_line_step(const Player &before,int old_deaths,double dt) {
    auto &v=state.adventure;
    if(grid.template_id==103&&has_curio(iceberg::Curio::BasementCall)&&
       !has_curio(iceberg::Curio::ArchiveCall)&&std::abs(state.player.x-104.)<12.&&
       int((state.elapsed+.75)/4.)>int((state.elapsed-dt+.75)/4.))
      audio.play(AudioCue::Metal1,.42f);
    if(grid.template_id!=200+iceberg::UnnumberedFloor)return;
    if(state.deaths!=old_deaths){v.night_beat=0;v.night_left=0.;return;}
    v.night_left=std::max(0.,v.night_left-dt);
    const double x=state.player.x;
    if(profile.discoveries&iceberg::achievement_bit(iceberg::Achievement::WitnessTape)) {
      if(before.x<98&&x>=98)audio.play(AudioCue::Step1,.48f);
      if(before.x<105&&x>=105)audio.play(AudioCue::Step2,.44f);
    }
    if(!(grid.switch_flags&iceberg::BasementCircuitCut))return;
    if(v.night_beat==1&&before.x>112&&x<=112) {
      v.night_beat=2;v.night_left=1.5;
      audio.play(AudioCue::Metal2,.41f);
    } else if(v.night_beat==2&&before.x>96&&x<=96) {
      v.night_beat=3;v.night_left=1.3;
      audio.play(AudioCue::Step2,.51f);
    }
  }
  bool side_story_interact(const Interaction &node) {
    using iceberg::Achievement;
    using iceberg::Curio;
    using iceberg::SideFlag;
    using iceberg::SideStory;
    if(node.kind==InteractionKind::Npc&&grid.template_id==102&&node.id==3&&
       has_side_flag(SideFlag::Casting)&&!has_side_flag(SideFlag::CastingReturned)) {
      remember_side_flag(SideFlag::CastingReturned);
      unlock_achievement(Achievement::SiliconeReturn);
      talk({{3,{"Tak, to nasz silikonowy odbojnik. Nie. Nie wiem, kto zatwierdził ten kształt.",
                  "Yes, that's our silicone doorstop. No, I don't know who approved that shape."}},
            {3,{"Wpiszę «część gumowa». Jemu też oszczędzimy formularza o zmianę nazwiska.",
                  "I'll put down ‘rubber fitting’. One of us should be spared a name-change form."}}});
      persist();
      return true;
    }
    if(node.kind!=InteractionKind::SideStory)return false;
    const auto story=SideStory(node.id);
    if(const auto document=side_document(story)) {
      if(!has_side_flag(document->flag)) {
        remember_side_flag(document->flag);
        if(has_side_flag(SideFlag::HarborStub)&&has_side_flag(SideFlag::EstatePlate)&&has_side_flag(SideFlag::ArchiveCopy))
          unlock_achievement(Achievement::PaperTrail);
        persist();
      }
      inspect_nearby();
      return true;
    }
    switch(story) {
      case SideStory::BasementBreaker: {
        grid.switch_flags^=iceberg::BasementCircuitCut;
        const bool cut=grid.switch_flags&iceberg::BasementCircuitCut;
        if(cut) {
          unlock_achievement(Achievement::PowerCut);
          remember_side_flag(SideFlag::BasementPowerCut);
        }
        else profile.task_items[1]&=~iceberg::side_flag(SideFlag::BasementPowerCut);
        audio.play(AudioCue::Metal1);
        notice(cut?t("Zasilanie odcięte. Zostały lampy awaryjne.","Power cut. Emergency lamps only."):
          t("Zasilanie wróciło. Uwaga na przewód.","Power restored. Watch the cable."),3);
        persist();
        break;
      }
      case SideStory::RosterDesk:
        if(has_side_flag(SideFlag::RosterPosted)||has_side_flag(SideFlag::RosterSealed))inspect_nearby();
        else talk({{3,{"Poznaję ten ślad po ołówku. Pilnowałam Zosi w pralni, kiedy jej mama poszła po wodę.",
                       "I know that pencil mark. I watched Zosia in the laundry while her mother went for water."}},
                   {3,{"Miałam klucz. Kiedy dyżurny powiedział, że wszyscy wyszli, przekręciłam go. Rano nikt nie chciał dopisać dziecka bez numeru mieszkania.",
                       "I had the key. When the dispatcher said everyone was out, I turned it. In the morning nobody would add a child without a flat number."}},
                   {3,{"Zosia przeszła z matką do piekarni przez pralnię. W papierach nigdy nie weszła do budynku. Co robimy z odpisem?",
                       "Zosia and her mother got to the bakery through the laundry. On paper she never entered the building. What should we do with the copy?"}}},
                  0,iceberg::EstateRosterReplyScene);
        break;
      case SideStory::Casting:
        remember_side_flag(SideFlag::Casting);
        animate_pickup(node,0);
        notice(t("Miękki, cielisty odlew. Na spodzie ktoś napisał GRAŻYNA.",
          "A soft, flesh-coloured casting. Somebody wrote GRAŻYNA underneath."),6);
        persist();
        break;
      case SideStory::Intercom:
        remember_curio(Curio::BasementCall);
        persist();
        audio.play(AudioCue::Spark,.45f);
        talk({{-1,{"Trzy uderzenia od środka. Za trzecim razem słuchawka powtarza pierwsze dwa.",
                    "Three knocks from inside. On the third, the receiver repeats the first two."}},
              {-1,{"Na zawieszce: «Jeśli słyszysz swój głos, odłóż słuchawkę. Prawdziwy dyżurny nie zna twojego imienia».",
                    "A tag reads: ‘If you hear your own voice, hang up. The real dispatcher doesn't know your name.’"}}});
        break;
      case SideStory::Tape:
        audio.play(AudioCue::Rumble,.35f);
        talk({{-1,{"TAŚMA 11 · 00:07. Wentylator zwalnia. Dwa głosy siedzą tuż przy mikrofonie.",
                    "TAPE 11 · 00:07. The fan slows. Two voices are close to the microphone."}},
              {-1,{"DOZORCZYNI: Czternaście. Doliczyłam niemowlę. Proszę wpisać czternaście.",
                    "CARETAKER: Fourteen. I counted the baby. Please write down fourteen."}},
              {-1,{"DYŻURNY: Mam trzynaście formularzy. Zaraz przyniosę czternasty. Jeszcze nie zamykaj.",
                    "DISPATCHER: I have thirteen forms. I'll bring a fourteenth. Don't shut it yet."}},
              {-1,{"GŁOS Z GÓRY: Macie trzynaście. Grzejcie trzynaście. Drzwi o 00:07 same podadzą liczbę.",
                    "VOICE FROM ABOVE: You have thirteen. Heat thirteen. At 00:07 the door will give you the number."}},
              {-1,{"Wentylator staje. Od drugiej strony ktoś dyktuje twoje kroki. Liczy o jeden do przodu.",
                    "The fan stops. Someone on the other side calls out your footsteps. They're one step ahead."}}},FinishBasementTape);
        break;
      case SideStory::EmergencyBell:
        audio.play(AudioCue::Metal2,.8f);
        if(has_side_flag(SideFlag::BellRung)) {
          unlock_achievement(Achievement::RingAgain);
          notice(t("GŁOŚNIK: Drugie wezwanie oznacza, że pierwsze już zostało przyjęte.", 
            "SPEAKER: A second call means your first has already been received."),6);
        } else {
          remember_side_flag(SideFlag::BellRung);
          notice(t("GŁOŚNIK: Dzwonek działa. Prosimy nie sprawdzać.", "SPEAKER: The bell works. Please refrain from checking."),6);
        }
        persist();
        break;
      case SideStory::HeatworksMug:
        remember_side_flag(SideFlag::HeatworksMugTaken);
        animate_pickup(node,0);
        notice(t("Kubek Witka w torbie. Jego stół jest przy wejściu.",
          "Witek’s mug is in your bag. His bench is by the entrance."),4);
        persist();
        break;
      case SideStory::HeatworksMugBench:
        if(has_side_flag(SideFlag::HeatworksMugReturned))inspect_nearby();
        else talk({{1,{"Wiedziałem, że został na rurze. Od wtorku podgrzewam tę samą herbatę. Przed chwilą Ania zabrała mi klucz od kotłowni.",
                       "I knew I'd left it on the pipe. I've been reheating the same tea since Tuesday. Ania just took the boiler-room key off me."}},
                   {1,{"Wrócę po odpoczynku. Kubek zostawię jej na zmianę. Kolejki do jednego kubka jeszcze nam nie wpisali do rozpiski.",
                       "I'll come back after I've slept. She can have the mug for her shift. They haven't put a queue for it on the rota yet."}}},
                  0,iceberg::HeatworksMugReplyScene);
        break;
      case SideStory::GardensPot:
        if(!has_side_flag(SideFlag::GardenSnailFound)) {
          remember_side_flag(SideFlag::GardenSnailFound);persist();
          audio.play(AudioCue::Stone1,.55f);
        }
        inspect_nearby();
        break;
      case SideStory::GardenSnailTap:
        if(!has_side_flag(SideFlag::GardenSnailFound)||has_side_flag(SideFlag::GardenSnailWatered)) {
          inspect_nearby();
        } else {
          remember_side_flag(SideFlag::GardenSnailWatered);
          state.adventure.garden_snail_left=iceberg::GardenSnailWalkTime;
          audio.play(AudioCue::Valve,.45f);
          notice(t("Woda spływa do ziemi. Ślimak wysuwa czułki.","Water trickles into the soil. The snail extends its feelers."),2.5);
          persist();
        }
        break;
      case SideStory::GardenSnailShelter:
        if(state.adventure.garden_snail_left<=0.) {
          unlock_achievement(Achievement::SnailSafe);
          persist();
        }
        inspect_nearby();
        break;
      case SideStory::ArchiveLine:
        if(!has_curio(Curio::BasementCall)){inspect_nearby();break;}
        unlock_achievement(Achievement::NightLine);
        remember_curio(Curio::ArchiveCall);
        audio.play(AudioCue::Metal2,.42f);
        talk({{-1,{"LINIA 11. Bilet jednorazowy, peron dolny. Nadal mamy jednego pasażera.",
                    "LINE 11. Single ticket, lower platform. We still have one passenger."}},
              {-1,{"Głos jest twój. Z głośnika dobiega jeszcze odgłos odkładanej słuchawki, choć przewód leży na stole.",
                    "It is your voice. Then comes the sound of a receiver settling in its cradle, though the cord lies loose on the desk."}},
              {-1,{"Na odwrocie plakietki: «Potwierdź wzrokowo. Zgaś światło nad szybą».",
                    "On the back of the tag: ‘Confirm by sight. Switch off the light over the window.’"}}});
        persist();
        break;
      case SideStory::NightWindow: {
        if(!has_curio(Curio::ArchiveCall)||!(grid.switch_flags&iceberg::BasementCircuitCut)) {
          inspect_nearby();break;
        }
        auto &v=state.adventure;
        v.night_beat=1;v.night_left=2.8;
        audio.play(AudioCue::Rumble,.62f);
        unlock_achievement(Achievement::LastPassenger);
        notice(t("Peron 11. Po drugiej stronie ktoś podnosi torbę z twoim numerem.",
                 "Platform 11. On the other side, someone picks up a satchel with your number."),5);
        persist();
        break;
      }
      case SideStory::GardenButton:
        remember_curio(Curio::GardenButton);
        animate_pickup(node,0);
        notice(t("Mosiężny guzik. Od spodu: «Z dachu, karmnik przy oknie 11».",
          "A brass button. On the back: ‘From the roof, birdhouse by window 11.’"),5);
        persist();
        break;
      case SideStory::EstateBirdhouse:
        if(has_curio(Curio::GardenButtonReturned))
          notice(t("Karmnik jest zamknięty. Ze środka dochodzi skrobanie.",
            "The birdhouse is shut. Something scratches inside."),4);
        else if(has_curio(Curio::GardenButton)) {
          remember_curio(Curio::GardenButtonReturned);
          unlock_achievement(Achievement::WrenHouse);
          audio.play(AudioCue::Wood1,.5f);
          notice(t("Guzik pasuje do gwoździa. Dach wreszcie się domyka. W środku ktoś już ułożył mech.",
            "The button fits over the nail. The roof finally closes. Somebody has already lined it with moss."),6);
          persist();
        } else {
          notice(t("Luźny daszek. Po zawleczce został gwóźdź i kółko wielkości guzika.",
            "A loose roof. A nail and a button-sized ring mark where the catch was."),5);
        }
        break;
      case SideStory::HarborTicket:
        if(has_curio(Curio::CrownTicket)&&!has_curio(Curio::HarborTicket)) {
          remember_curio(Curio::HarborTicket);
          unlock_achievement(Achievement::LostReceipt);
          animate_pickup(node,0);
          notice(t("W środku leży twój bilet. Pod datą: «Odjazd 00:07, między trzecią a czwartą».",
            "Your ticket is inside. Below the date: ‘Departs 00:07, between three and four.’"),7);
          persist();
        } else notice(t("Okienko zwrotów. Nieodebrane bilety trafiają tu z kasownika na Koronie.",
          "Returns hatch. Unclaimed tickets come here from the punch on the Crown."),5);
        break;
      case SideStory::CrownPunch:
        unlock_achievement(Achievement::PunchTomorrow);
        remember_curio(Curio::CrownTicket);
        audio.play(AudioCue::Metal1,.6f);
        notice(t("Kasownik wybija datę na pustym bilecie. Jutrzejszą, bez roku.",
          "The punch stamps a blank ticket. Tomorrow's date, with the year missing."),6);
        persist();
        break;
      default:return false;
    }
    return true;
  }
