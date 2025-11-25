# Assets and Embedding

## Build-time embedding

- `visage_file_embed` provides CMake helpers to bake assets (shaders, images, etc.) into the binary.
- Use `add_embedded_resources(project, include_filename, namespace, files)` from `visage_file_embed/CMakeLists.txt`.
- Generated sources end up in a `_generated` folder per target; an include header exposes `EmbeddedFile` lookups.

## Typical flow

1. List files to embed in your CMakeLists.
2. Call `add_embedded_resources` with a namespace (e.g., `visage::graphics`).
3. Include the generated header and fetch resources by name.

## Guidance

- Keep embedded files small; large assets may belong on disk or streamed.
- Prefer embedding shaders to avoid runtime file I/O and ensure cross-platform packaging.
