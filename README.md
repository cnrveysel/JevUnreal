# Jev for Unreal Engine

Use **Jev decisions** directly from Unreal Engine Blueprints. This is the v0.2 code preview. The v0.1 release was tested with Unreal Engine 5.8, including a manual Play In Editor request to the real TypeSafe API.

```
State  →  Jev  →  Decision  →  Blueprint
```

Jev is a general-purpose decision layer. Give it a natural-language snapshot of your game state and a question; it returns a decision and a probability. Unreal stays in control: your Blueprints execute the behavior.

## What it is

**Jev** is an open-source Unreal Engine 5 plugin that exposes [TypeSafe Jev](https://typesafe.ai) as reusable Blueprint nodes. It is *not* an NPC framework — it is a thin, native-feeling async bridge between any game state and a Jev decision endpoint.

Use it anywhere a yes/no or weighted decision helps:

- NPC decisions
- Dynamic difficulty
- Dialogue decisions
- Procedural generation
- Game director logic
- Yes / no gameplay decisions
- UI decisions
- Simulation logic
- Experimentation

Combine it freely with Behavior Trees, State Trees, EQS, or anything else — Jev supplies decisions; Unreal executes behavior. It is not a replacement for those systems.

## Features

- **Jev Yes / No** async Blueprint node: State + Question → YES/NO, Yes probability, confidence, raw response, latency
- **Jev Choose** async Blueprint node: State + Question + Options → one supplied option, selected index, confidence, raw response, latency
- **Make Jev Request** advanced async node: send raw Questions JSON, get the raw Jev response
- Fully asynchronous via Unreal's HTTP module — no game-thread blocking
- Project Settings → Plugins → Jev configuration (endpoint, model, API key, timeout, debug logging, proxy)
- Proxy mode for production backends
- Optional debug logging that never prints API keys or Authorization headers
- Dedicated `LogJev` category
- Focused parser/normalization tests

## Installation

1. Clone or download this repository.
2. Copy the `Jev` plugin folder (the whole repo) into your project's `Plugins/` directory, or add it to your `.uproject` plugins list.
3. Rebuild your project and open the editor.
4. In any Blueprint, search for **Jev Yes / No**.

Tested on Unreal Engine 5.8. Other Unreal Engine versions have not been verified for this release.

## Blueprint usage

### Example A — dynamic difficulty

```text
Begin Play
  → Jev Yes / No
      State:    "Player has died five times in ten minutes."
      Question: "Should the game reduce the difficulty?"
    On Success → Branch on Answer
```

### Example B — NPC retreat

```text
Tick / interval
  → Jev Yes / No
      State:    "Health: 18\nAmmo: 0\nEnemy distance: 300\nCover nearby: true"
      Question: "Should this character retreat?"
    On Success → if Answer == YES → Play Retreat behavior
```

### Example C — generic request

```text
→ Make Jev Request
    State:             "Sandbox world with sparse resources."
    Raw Questions JSON: {"decision":{"type":"noul","instructions":"Should we spawn a storm?"}}
  On Success → parse RawJsonResponse yourself
```

The **Yes / No** node normalizes the answer for you:

| Yes probability | Answer | Confidence |
|-----------------|--------|------------|
| ≥ 0.5 | YES | probability |
| < 0.5 | NO  | 1 − probability |

Probabilities outside `0..1` are rejected with an error, never silently rescaled.

## Configuration

Open **Project Settings → Plugins → Jev**:

| Setting | Default | Notes |
|---|---|---|
| Endpoint | `https://api.typesafe.ai/v1/systemone` | TypeSafe or proxy URL |
| Model | `jev-latest` | Jev model name |
| API Key | — | Bearer key; see security below |
| Request Timeout | 30 s | Seconds |
| Debug Logging | off | Decision and response-size diagnostics, no secrets |
| Use Proxy | off | Production mode |
| Proxy Endpoint | — | Your backend URL used instead of Endpoint |

An example ini block lives in `Config/JevExample.ini`.

In direct API mode, a missing API key is reported through the Blueprint error output at runtime. Proxy mode similarly reports an error if no proxy endpoint is configured.

## Development direct API mode

For prototyping, set **API Key** in Project Settings (or your local `DefaultEngine.ini`) and leave **Use Proxy** off:

```ini
[/Script/Jev.JevSettings]
ApiKey=PASTE_YOUR_LOCAL_KEY_HERE
bDebugLogging=True
```

Requests go straight from Unreal to TypeSafe with `Authorization: Bearer <key>`.

## Production proxy recommendation

**Do not ship a TypeSafe API key inside a packaged game.** The key lives in config files that ship with the build; it is extractable by anyone. This plugin will never make that safe.

For production:

1. Run a tiny backend that holds your real key.
2. Expose an endpoint that accepts the same request shape.
3. Enable **Use Proxy** and set **Proxy Endpoint** to your backend.
4. Leave the in-game **API Key** empty.

The plugin then sends requests to your backend with no Authorization header; your proxy injects the key and forwards to TypeSafe.

## Security

- The API key is plain-text config. It is **not** safe in source, packaged builds, or version control.
- Never commit real keys. `.gitignore` excludes local secret configs.
- Debug logging never prints the API key, Authorization header, or request secrets.
- In direct mode, an endpoint override receives the configured bearer key. Use overrides only with trusted endpoints.
- Use proxy mode for anything public or released.

## Architecture

```
Blueprint async node (UAsyncActionJevYesNo / UAsyncActionJevChoose / UAsyncActionJevRequest)
  → UJevSubsystem (game instance subsystem, request lifetime)
    → FJevHttpClient (Unreal HTTP module, async POST + JSON)
      → TypeSafe / proxy endpoint
    ← response
  → FJevParser (pure JSON/normalization logic, unit tested)
← Blueprint delegates (OnSuccess / OnError)
```

- `JevTypes.h` — Blueprint structs and enums
- `JevSettings.h` — `UDeveloperSettings` for Project Settings
- `JevParser` — pure probability extraction and YES/NO normalization (no HTTP coupling)
- `JevHttpClient` — async HTTP wrapper; never blocks the game thread
- `JevSubsystem` — request orchestration and lifetime handling
- `JevAsyncActions` — Blueprint-facing async nodes
- `JevEditorTests` — automation specs for parsing and normalization

## Current limitations

- No retry logic — a failed or timed-out request surfaces as an error to your Blueprint.
- The API key travels through your process in plaintext config; production use requires a proxy.
- Rate limits, billing, and upstream schema changes are your responsibility to handle (the node reports HTTP status and errors).

## Roadmap

- [x] `Jev Choose` node using the verified native `choice` response schema
- [ ] `Jev Choose` compile and live-endpoint verification
- [ ] Request batching helpers
- [ ] Sample project / demo map

## Vibe coded

This project was vibe coded with AI coding agents as an experiment in building small, useful tools quickly. I directed the product design, architecture, testing, debugging, and iterations while AI agents helped write much of the implementation.

## Documentation assets

Blueprint screenshots and a Play In Editor demo are planned but are not included in v0.1. See [docs/README.md](docs/README.md) for the capture checklist.

## License

[MIT](LICENSE) — © 2026 Veysel
