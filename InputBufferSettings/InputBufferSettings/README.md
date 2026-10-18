# Input Buffer (Configurable)

## Build
1. Install the Geode SDK + CLI (https://docs.geode-sdk.org/getting-started/).
2. Run `geode sdk version` and put that value in the `"geode"` field of `mod.json`.
3. In this folder:
   ```
   geode build
   ```
   The `.geode` file ends up in `build/`. Drop it in your Geometry Dash `geode/mods` folder.

No local toolchain? Push this folder to a GitHub repo; the included workflow builds it for you.

## Before you ship it
- Change `id` and `developer` in `mod.json` (id format: `yourname.modname`, lowercase).
- Add `android`/`mac`/`ios` entries under `"gd"` if you want other platforms.
