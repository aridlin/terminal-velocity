# Online expeditions (development)

Two couriers share an eight-room expedition. Race advances each courier independently and records their finishing order/time. Co-op shares machinery, pickups and checkpoints, and opens the next room once both reach the exit. Everyone confirms readiness before a common countdown. A disconnected player freezes the match; reconnect uses an opaque per-slot token. Matches are isolated from campaign saves, achievements and solo records.

The host runs the real 120 Hz game simulation, collision, checkpoints, interactions and room generation. Clients send controls and receive compressed Unicode cell frames at 30 Hz; they cannot submit positions, switch flags or results. Network latency therefore affects input response. Desktop clients currently connect using the command below; browser creation/join setup is inside the canvas.

```
./terminal_velocity --online HOST:27890 --mode race --room new
./terminal_velocity_sdl --terminal-view --online HOST:27890 --mode coop --room new
# Share the displayed six-digit code; replace `new` with that code to join.
```

A/D, Space, X/Shift, E, B, Tab, J, P, R retain their meanings. Enter marks ready. M or Q leaves on desktop; M/Escape leaves in the browser. U controls local recorded audio. The native main menu includes Multiplayer: create a race or co-op room, or join using a six-digit code. W/S chooses and changes digits, A/D moves the digit cursor, Enter confirms. Desktop mode defaults to English; the browser uses its current selected language. Launch options remain available.

## Host

```
make multiplayer_engine
python3 -m venv .multiplayer-venv
.multiplayer-venv/bin/pip install -r multiplayer/requirements.txt
.multiplayer-venv/bin/python multiplayer/server.py --engine ./multiplayer_engine
```

The service binds loopback by default. A TLS reverse proxy should forward `/terminal-velocity/multiplayer` WebSocket upgrades to port27889; expose native TCP27890 only when intended (`--tcp-host`). No account credentials or player save files are transmitted. Rooms are limited to16, two players each; messages, input rates, frame sizes and slow clients are bounded. Empty rooms expire after60seconds. Input inactivity disconnects after10seconds. Keep the engine/service under a dedicated unprivileged user and retain rollback for updates. No deployment is claimed by this document.

The browser default endpoint is `wss://prol.aridlin.pl/terminal-velocity/multiplayer`; an HTML meta `tv-multiplayer-url` overrides it for local testing. WebSocket origins are explicitly allowed with `--origin`.

## Checks

`multiplayer_match_tests` covers the match lifecycle, actual ordinary-input traversal, independent race progression and shared co-op state. `multiplayer/test_service.py` connects real browser WebSockets and native TCP to test room admission, code matching, protocol checks, reconnection and actual cell frames. Existing single-player suites remain required.

Pending before release: complete a two-person terminal/SDL/browser play review, measure latency/bandwidth, verify mute/focus/resizing and control restoration, run native Windows CI, deploy the actual service, and verify public endpoints. Desktop/browser clients retry a dropped connection using their existing per-slot token; reconnection still needs real interruption testing.
