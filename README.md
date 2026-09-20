# JevUnreal

**JevUnreal v0.2** brings [TypeSafe Jev](https://typesafe.ai) decisions into Unreal Engine 5.8 Blueprints. Send a description of your game state and a question; an async node returns the result through **On Success** or an error through **On Error**. Your Blueprint decides what to do with the answer.

## Blueprint nodes

| Node | Inputs | On Success result |
|---|---|---|
| **Jev Yes / No** | State, Question | YES/NO answer, Yes probability, confidence, raw response, latency |
| **Jev Choose** | State, Question, Options | Selected option and index, confidence, raw response, latency |
| **Jev Probability** | State, Question | Probability from 0.0 to 1.0, confidence, raw response, latency |

Each node also has an optional timeout override. **Jev Yes / No** answers YES when the returned probability is at least 0.5. **Jev Probability** exposes that probability directly; its confidence is `max(Probability, 1 - Probability)`. **Jev Choose** returns one of the supplied options and the confidence reported by Jev. Latency is in milliseconds.

### Example uses

```text
Dynamic difficulty
  Jev Yes / No
  State: "The player has died five times in ten minutes."
  Question: "Should the game reduce the difficulty?"
  On Success -> branch on Result.Answer

Dialogue
  Jev Choose
  State: "The player helped the merchant."
  Question: "Which response should the merchant give?"
  Options: ["Offer a discount", "Share a rumor", "Say thanks"]
  On Success -> use Result.SelectedOption

NPC behavior
  Jev Probability
  State: "Health is low; cover is nearby."
  Question: "Should this NPC retreat?"
  On Success -> use Result.Probability in your own gameplay logic
```

For custom question JSON, the advanced **Make Jev Request** node returns the raw API response.

## Setup

1. Copy this repository into `<YourProject>/Plugins/Jev`.
2. Rebuild your Unreal Engine 5.8 project and open it in the editor.
3. Open **Project Settings → Plugins → Jev**. Set the endpoint, model, request timeout, and either an API key or a proxy endpoint.
4. Search for **Jev Yes / No**, **Jev Choose**, or **Jev Probability** in a Blueprint.

The default endpoint is `https://api.typesafe.ai/v1/systemone`, the default model is `jev-latest`, and the default timeout is 30 seconds. See [Config/JevExample.ini](Config/JevExample.ini) for a configuration example. A missing API key in direct mode, or a missing proxy endpoint in proxy mode, reaches the node's **On Error** output.

## API key and packaged games

**Direct API mode is for local development.** It sends the configured API key as a bearer token. A key stored in Unreal project config can be extracted from a packaged game, so do not ship one or commit it to version control. Endpoint overrides in direct mode also receive that key; use only trusted endpoints.

For a public packaged build, enable **Use Proxy**, set **Proxy Endpoint** to your backend, and leave the game's API key empty. Your backend holds the key and forwards requests to TypeSafe. The plugin sends no Authorization header in proxy mode.

## How it works

The Blueprint nodes use Unreal's HTTP module asynchronously. The subsystem keeps requests alive while they are active, and the HTTP client uses a single completion handler for responses. The Yes / No and Probability nodes share the same `noul` request and parsing path; Choose uses Jev's `choice` response. Requests that fail or time out reach **On Error**; there is no automatic retry.

## Vibe coded

This project was vibe coded with AI coding agents as an experiment in building small, useful tools quickly. I directed the product design, architecture, testing, debugging, and iterations while AI agents helped write much of the implementation.

## License

[MIT](LICENSE) — © 2026 Veysel
