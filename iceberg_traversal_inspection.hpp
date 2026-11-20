#pragma once

namespace iceworld {
struct TraversalInspection {
  iceberg::Text title, body;
};

inline constexpr TraversalInspection CargoSling{
  {"Zawiesie ładunkowe","Cargo sling"},
  {"Zaczekaj, aż zawiesie dobije do pomostu. Wskocz na skrzynię i przejedź nad wodą.",
   "Wait for the sling to dock. Jump onto the crate and ride it across the water."}};
inline constexpr TraversalInspection PressurePiston{
  {"Tłok ciśnieniowy","Pressure piston"},
  {"Gdy tłok zatrzyma się przy pomoście, wskocz na głowicę. Zejdź na następnym poziomie.",
   "Board the piston head when it stops at the landing. Step off at the next level."}};
inline constexpr TraversalInspection TurbineDraft{
  {"Strumień z turbiny","Turbine draft"},
  {"Prąd powietrza narasta i słabnie. Wejdź, gdy strumień jest mocny, i daj się ponieść. Podczas zrywu wiatr cię nie pcha.",
   "The airflow builds and fades. Enter when it is strong and let it carry you. A dash cuts through the airflow."}};
inline constexpr TraversalInspection SteamVent{
  {"Zawór parowy","Steam vent"},
  {"Przed wyrzutem pary zawór klika i puszcza małe obłoki. Przejdź po wyrzucie, kiedy rura ucichnie.",
   "The valve clicks and gives off wisps before a blast. Cross after the blast when the pipe goes quiet."}};
inline constexpr TraversalInspection CableBalcony{
  {"Balkon na linie","Cable balcony"},
  {"Balkon zatrzymuje się przy końcach liny. Wejdź z podestu, przejedź nad ulicą i wysiądź po drugiej stronie.",
   "The balcony stops at both ends of its cable. Board from the landing, cross the street and step off on the other side."}};
inline constexpr TraversalInspection LooseMasonry{
  {"Luźny gzyms","Loose cornice"},
  {"Spod gzymsu sypie się pył, zanim spadnie gruz. Przeczekaj poza miejscem upadku i przejdź, gdy blok minie.",
   "Dust falls before the masonry drops. Wait outside the drop zone and pass once the block has fallen."}};
inline constexpr TraversalInspection RisingRack{
  {"Podnoszony regał","Rising rack"},
  {"Wskocz na górną półkę, kiedy regał zjedzie. Zabierze cię do podestu wyżej.",
   "Jump onto the top shelf when the rack comes down. It carries you up to the next landing."}};
inline constexpr TraversalInspection Scanner{
  {"Skaner archiwum","Archive scanner"},
  {"Bursztynowy sygnał ostrzega przed skanem. Zaczekaj poza torem czerwonej wiązki, a potem przejdź w przerwie.",
   "The amber signal warns of a scan. Stay clear of the red beam and cross in the gap between sweeps."}};
inline constexpr TraversalInspection SwingVineGuide{
  {"Pnącze do huśtania","Swing vine"},
  {"Przytrzymaj skok, gdy zbliżasz się do końca pnącza. Kiedy wahadło niesie cię w stronę półki, naciśnij skok ponownie, by się odbić.",
   "Hold Jump as you approach the vine tip to catch it. When the swing takes you toward the landing, press Jump again to launch."}};
inline constexpr TraversalInspection Flytrap{
  {"Muchołówka","Flytrap"},
  {"Liście drżą przed zatrzaśnięciem. Przeskocz nad nimi po ugryzieniu, kiedy roślina znów spokojnie się otworzy.",
   "The leaves shiver before the jaws snap. Jump over after the bite when the plant has opened again."}};
inline constexpr TraversalInspection BrittleIce{
  {"Kruchy lód","Brittle ice"},
  {"Obciążony lód najpierw pęka, a po chwili się zapada. Wyląduj i odbij się od razu.",
   "This ice cracks under your weight, then gives way. Land and jump off immediately."}};
inline constexpr TraversalInspection Crosswind{
  {"Boczny wiatr","Crosswind"},
  {"Smugi pokazują kierunek wiatru. Skocz, gdy wiatr wieje w stronę półki. Zryw na chwilę uniezależnia cię od podmuchu.",
   "The streaks show which way the wind is blowing. Jump when it blows toward the landing. A dash briefly cuts through the gust."}};

inline const TraversalInspection *nearby_traversal_inspection(const EngineGrid &room,const GameState &state) {
  if(!room.adventure_level||room.room_role!=iceberg::RoomRole::Traverse)return nullptr;
  const Rect courier=state.player.rect();
  const double x=courier.x+courier.w*.5,y=courier.bottom()-.4;
  const TraversalInspection *nearest=nullptr;
  double best=100.;
  const auto consider=[&](Rect r,const TraversalInspection &description) {
    const double dx=x-std::clamp(x,r.left(),r.right());
    const double dy=y-std::clamp(y,r.top(),r.bottom());
    if(std::abs(dx)>3.5||std::abs(dy)>5.5)return;
    const double distance=dx*dx+dy*dy;
    if(distance<best){best=distance;nearest=&description;}
  };
  for(size_t i=0;i<room.platforms.size();++i) {
    const auto &deck=room.platforms[i];
    const auto *description=deck.harbor_sling?&CargoSling:deck.pressure_piston?&PressurePiston:
        deck.estate_balcony?&CableBalcony:deck.archive_rack?&RisingRack:
        deck.fragile&&(deck.icy||deck.rime_shelf)?&BrittleIce:nullptr;
    if(!description)continue;
    consider(deck.at(state.elapsed),*description);
    // Both docks remain inspectable while the deck is away.
    if(deck.dx!=0.||deck.dy!=0.)for(int end:{-1,1}) {
      auto dock=deck.base;
      dock.x+=end*deck.dx;dock.y+=end*deck.dy;
      consider(dock,*description);
    }
  }
  for(const auto &pulse:room.pulses) {
    if(!pulse.powered(room.switch_flags))continue;
    switch(pulse.kind) {
      case PulseKind::Steam:consider(pulse.area,SteamVent);break;
      case PulseKind::Masonry:consider(pulse.area,LooseMasonry);break;
      case PulseKind::Scanner:consider(pulse.area,Scanner);break;
      case PulseKind::Flytrap:consider(pulse.area,Flytrap);break;
      default:break;
    }
  }
  for(const auto &vine:room.vines) {
    const auto [tip_x,tip_y]=vine.tip(state.elapsed);
    consider({tip_x,tip_y,0.,0.},SwingVineGuide);
    const double reach=vine.length*std::sin(SwingVine::MaxAngle);
    const double height=vine.y+vine.length*std::cos(SwingVine::MaxAngle);
    for(double dock:{vine.x-reach,vine.x+reach})consider({dock,height,0.,0.},SwingVineGuide);
  }
  for(const auto &field:room.air_fields) {
    if(!field.active(room.switch_flags))continue;
    if(field.turbine)consider(field.area,TurbineDraft);
    else if(room.district==5&&field.ax!=0.)consider(field.area,Crosswind);
  }
  return nearest;
}
} // namespace iceworld
