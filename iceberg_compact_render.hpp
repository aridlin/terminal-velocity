// Compact cell layouts are shared by all frontends. No browser dialogue UI.
  void compact_paragraph(int &row,const std::string &value,int bottom,uint32_t color=0xeee7d6) {
    for(const auto &line:wrap(value,cols_-4))if(row<bottom)paint_text(row++,2,line,color);
  }
  void draw_iceberg_compact() {
    const auto &v=state_.adventure;
    const auto &d=iceberg::Districts[size_t(grid_.district)];
    fill(0,0,cols_,4,Color::Default);
    paint_text(0,1,v.tour_scene>=0?tr("PRZEGLĄD ZMIAN ","DEBUG TOUR ")+std::to_string(v.tour_scene+1)+"/"+std::to_string(iceberg::DebugTour.size()):d.name.get(v.language),d.accent,true);
    int y=1;compact_paragraph(y,v.room_name,3,d.light);
    if(y==2&&cols_>=56)paint_text(2,1,room_hud_stats(cols_-2),0xb3c3c8);
    for(int x=0;x<cols_;++x)paint_text(3,x,"─",d.stone);
    fill(rows_-3,0,cols_,3,Color::Default);
    const bool at_exit=state_.won&&!state_.paused&&!state_.journal&&!v.map&&!v.inspecting&&!v.dialogue.active();
    const auto footing=fragile_footing_hint();
    const auto footer=v.save_error?save_failure_hint():at_exit?tr("R — wróć do lampy","R — return to the lamp"):
      state_.player.ledge&&state_.player.ledge_climb_left<=0.?ledge_hint():
      !footing.empty()?footing:v.notice_left>0?v.notice:v.prompt.empty()?quest_focus_hint():v.prompt;
    const auto lines=wrap(footer,cols_-4);
    const uint32_t warning_color=cell_color({" ",Color::Hazard});
    y=rows_-(lines.size()>2?3:2);
    if(y>rows_-3) {
      if(at_exit)paint_text(rows_-3,2,v.branch_at_door?branch_door_hint():
        tr("E / ENTER — przejdź dalej","E / ENTER — pass through"),d.accent);
      else paint_text(rows_-3,1,tr("E użyj · B obejrzyj · P pauza","E use · B inspect · P pause"),d.light);
    }
    for(const auto &line:lines) {
      if(y==rows_)break;
      paint_text(y++,2,line,v.save_error?warning_color:at_exit?d.light:d.accent);
    }
    auto panel=[&](const std::string &title){frame(0,0,cols_,rows_,d.stone);paint_text(1,2,title,d.accent,true);};
    if(state_.menu) {
      if(v.save_picker||v.tour_picker){draw_profile_picker();return;}
      panel("TERMINAL VELOCITY");
      if(v.confirm_new) {
        y=5;compact_paragraph(y,restart_title(),rows_-3,d.accent);++y;
        compact_paragraph(y,v.selected==1?tr("Zastąpi to zapis obecnej podróży. Rekordy i stroje zostaną.","This replaces the current journey. Records and outfits are kept."):restart_warning(),rows_-3);
        paint_text(rows_-2,2,v.selected==1?tr("ENTER tak · P anuluj","ENTER yes · P cancel"):
          tr("E zastąp · P anuluj","E replace · P cancel"),d.light);return;
      }
      if(v.settings) {
        paint_text(4,2,tr("USTAWIENIA","SETTINGS"),d.accent);
        paint_text(7,2,tr("1 Muzyka: ","1 Music: ")+std::to_string(int(v.music_volume*100))+"%",d.light);
        paint_text(9,2,tr("2 Efekty: ","2 Effects: ")+std::to_string(int(v.effects_volume*100))+"%",d.light);
        paint_text(11,2,tr("3 Otoczenie: ","3 Ambience: ")+std::to_string(int(v.ambience_volume*100))+"%",d.light);
        paint_text(13,2,tr("4 Strój: ","4 Outfit: ")+std::to_string(v.outfit+1),d.light);
        paint_text(15,2,tr("5 Pauza po utracie fokusu ","5 Pause on focus loss ")+
          (v.pause_on_focus_loss?tr("wł.","on"):tr("wył.","off")),d.light);
        paint_text(16,2,tr("6 Zapisy i przegląd zmian","6 Saves and debug tour"),d.light);
        paint_text(17,2,tr("7 Płynna kamera (test) ","7 Smooth camera (exp.) ")+
          (v.smooth_camera?tr("wł.","on"):tr("wył.","off")),d.light);
        paint_text(18,2,tr("L Język: POLSKI","L Language: ENGLISH"),d.light);
        paint_text(rows_-2,2,tr("ENTER wróć","ENTER return"),d.accent);return;
      }
      paint_text(3,2,tr("POD POWIERZCHNIĄ","BENEATH THE SURFACE"),d.light);
      const std::array<std::string,6> labels{
        v.can_continue?tr("Kontynuuj podróż","Continue journey"):tr("Rozpocznij podróż","Begin journey"),
        tr("Nowa historia","New story"),
        v.endless_resume?tr("Wznów wyprawę","Resume expedition"):tr("Wyprawa bez końca","Endless expedition"),
        v.trial_resume?tr("Wznów próbę","Resume trial"):
          v.trial_replay?tr("Powtórz próbę","Replay trial"):tr("Próba mistrzowska","Mastery trial"),
        tr("Ustawienia","Settings"),tr("Trasy klasyczne","Classic courses")};
      for(int i=0;i<6;++i)paint_text((rows_<24?4+i:5+i*2),2,(v.selected==i?"▸ ":"  ")+std::to_string(i+1)+" "+labels[size_t(i)],v.selected==i?d.accent:0xc8bbb8);
      const int notice_row=rows_<24?14:20;
      if(v.selected==2||v.selected==3) {
        const auto &record=v.trials[size_t(v.run_district)];
        std::vector<std::string> stats;
        if(v.selected==2)
          stats=wrap(tr("Najdalsza wyprawa: ","Farthest expedition: ")+std::to_string(v.endless_best),cols_-4);
        else if(record.complete)stats=trial_record_lines(record,cols_-4);
        y=notice_row-2-int(stats.size());
        paint_text(y++,2,iceberg::Districts[size_t(v.run_district)].name.get(v.language),d.light);
        paint_text(y++,2,tr("J zmień dzielnicę","J change district"),d.light);
        for(const auto &stat:stats)paint_text(y++,2,stat,d.accent);
      }
      if(v.notice_left>0){y=notice_row;compact_paragraph(y,v.notice,rows_-3,d.accent);}
      paint_text(rows_-3,2,tr("↑↓ wybór · ENTER graj","↑↓ choose · ENTER play"),d.light);
      paint_text(rows_-2,2,tr("R nowa wyprawa · L język","R new expedition · L language"),d.light);return;
    }
    if(state_.journal||v.map)draw_iceberg_journal();
    else if(state_.paused&&!v.dialogue.active()) {
      panel(tr("CHWILA ODDECHU","A MOMENT TO BREATHE"));
      y=6;
      if(v.save_error)compact_paragraph(y,save_failure_hint(),rows_-3,warning_color);
      else if(v.tour_scene>=0) {
        compact_paragraph(y,v.room_name,rows_-3,d.light);
        ++y;
        compact_paragraph(y,tr("↑ poprzedni · ↓ następny","↑ previous · ↓ next"),rows_-3,d.accent);
        compact_paragraph(y,tr("Przegląd nie zapisuje postępu.","Tour progress is never saved."),rows_-3);
      } else compact_paragraph(y,tr("Postęp zapisuje się przy lampach i wyjściu.","Progress saves at lamps and when you leave."),rows_-3);
      paint_text(rows_-2,2,tr("ENTER dalej · M menu","ENTER continue · M menu"),d.accent);
    }
    draw_iceberg_dialogue();
    if(v.inspecting)draw_iceberg_inspection();
  }
