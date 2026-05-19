# Run_2 Firmware Tracker — Right Cluster

Repo: `living-relation/right-side-cluster-esp32s3`
Branch: `feature/Run_2`
Display role: right ESP32-S3, 480 x 480, dumb UART-only display slave for lambda, boost, ECT, ignition advance, and IAT UI.

## Canonical source

The canonical design source for this firmware branch is:

- Repo: `living-relation/TrackCluster_ECU_dashboard_design_system`
- Branch: `feature/Run_2`

Current Design System Run_2 commits to keep aligned with:

| Commit | Purpose |
|---|---|
| `5c26922c2fb21c984f2b0ab687ee256cc1522110` | Document center CAN I/O and controls contract |
| `7c12e0058e54ea791f518a3e4acb7631339b2631` | Correct center CAN direction and controls docs |
| `6fcf296a294cd18b18d82d1ba3f5904b9c2c7939` | Document center pinout and encoder menu UI |
| `4cd39847ae073f6af20355826f8ef78217a390ee` | Add verified pinout table and headlight dim input |

## Run_2 locked requirements

- Right cluster is a dumb UART-only display slave.
- Right cluster owns no CAN, no GPIO controls, no menu logic, no GPS, no lap timing, and no analog inputs.
- Right cluster receives only the center-produced RIGHT UART display frame.
- Side display UART RX is ESP32-S3 GPIO44 / RXD per verified Waveshare 2.8C hardware notes.
- Right display renders what the center tells it to render and does not independently invent global mode or warning state.
- Headlight dim state originates at center GPIO47 and is forwarded to the side displays by center firmware.
- AFR is banned; lambda only.
- Boost is sent directly from ECU through the center data model; MAP-derived boost is fallback only if direct boost is omitted later.

## Display scope

Right UI scope from the Design System:

- Lambda glyph/value block.
- Boost bar gauge.
- ECT bar gauge.
- Ignition advance bar gauge.
- IAT bar gauge.
- Warning/protect side overlay only when forwarded by center.

## Current implementation state

No Run_2 firmware implementation changes have been made in this repo yet. This tracker commit exists so the firmware branch can track the Design System branch state before replacing legacy firmware files.

## Next implementation phase

1. Replace legacy data model with Run_2 right UART payload model.
2. Remove CAN, analog, GPS, lap timer, AFR, and any independent control assumptions from firmware.
3. Replace old SquareLine/STI UI files with TrackCluster right LVGL widgets.
4. Implement UART receive/staleness behavior using center-produced RIGHT frames.
5. Forward-render center-owned dim/warning/protect state.
6. Static-check CMake, headers, symbol names, and LVGL API compatibility before each commit.
