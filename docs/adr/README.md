# Architecture decision records

Short records of decisions that are easy to undo by accident. Each states the context, the decision, and what follows from it. Add a new numbered file instead of rewriting an accepted one; mark the old one *Superseded by NNNN*.

| # | Decision | Status |
| --- | --- | --- |
| [0001](0001-own-wifi-reconnect.md) | The firmware owns WiFi reconnects; core auto-reconnect is off | Accepted |
| [0002](0002-wifi-modem-sleep-off.md) | WiFi modem sleep stays off | Accepted |
| [0003](0003-self-contained-web-ui.md) | The web UI is one self-contained file with no CDN | Accepted |
| [0004](0004-platformio-builds.md) | Firmware builds use PlatformIO | Accepted |
| [0005](0005-pure-logic-host-tests.md) | Decision logic lives in Arduino-free modules with host tests | Accepted |

Template:

```markdown
# NNNN. Title

Status: Proposed | Accepted | Superseded by NNNN
Date: YYYY-MM-DD

## Context
## Decision
## Consequences
```
