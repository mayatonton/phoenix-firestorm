# AYAstorm r42 macOS MoltenVK runtime bootstrap

## Purpose

Fix the macOS arm64 development build that reaches `STATE_LOGIN_SHOW` but presents
only a black window. The target is Vulkan/MoltenVK presentation; this change does
not add an OpenGL fallback.

## Observed failure

On 2026-07-24, the development app logged `Initializing graphics capabilities
from Vulkan device` followed by `Vulkan device unavailable; graphics capability
values remain defaults`. It subsequently initialized the login screen, but the
window stayed black.

The app bundle already contains all three required runtime artifacts:

- `Contents/Frameworks/libvulkan.dylib`
- `Contents/Frameworks/libMoltenVK.dylib`
- `Contents/Resources/vulkan/icd.d/MoltenVK_icd.json`

The Vulkan Loader does not automatically search that application-private ICD
directory. Before this change, no runtime code supplied that path to the Loader,
so no MoltenVK physical device was enumerated. macOS also requires portability
enumeration when discovering MoltenVK through the Vulkan Loader.

## Design

1. On Darwin, `volk.c` first opens the bundled Loader via
   `@executable_path/../Frameworks/libvulkan.dylib`; it retains the existing
   system-wide lookup fallbacks for non-bundled developer environments.
2. In `LLVKLoader::initVulkan()`, before `volkInitialize()`, macOS constructs the
   absolute path to the bundled `MoltenVK_icd.json` from `gDirUtilp`'s read-only
   resource directory and sets the process-local `VK_DRIVER_FILES` to it. The bundled
   Loader is version 1.4.350 and supports this Loader variable. The value is set
   internally for every normal app launch; no shell environment override is
   required.
3. `createInstance()` enumerates instance extensions. On Darwin it requires and
   enables `VK_KHR_portability_enumeration` together with
   `VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR`.
4. Every early initialisation failure logs its stage and `VkResult`. The app
   remains Vulkan-only: a missing manifest, Loader, portability extension, or
   physical device stops Vulkan initialisation instead of selecting OpenGL.
5. `pipeline_cache.bin` is implementation-specific. If MoltenVK rejects a
   cache written by an older MoltenVK or Metal driver, the same Vulkan device
   retries `vkCreatePipelineCache` with empty initial data. The rejected blob
   is never used for that run, and a normal cache is written after startup;
   this is not an OpenGL fallback.
6. MoltenVK's surface extent cannot be assumed to use backing-pixel units. On
   Darwin the swapchain therefore always uses the native window's
   backing-pixel size, not `currentExtent` or the historical `1280x720`
   placeholder. This keeps the swapchain, CAMetalLayer, rendered UI, and
   pointer coordinates aligned on Retina displays.
7. The legacy GPU benchmark renders offscreen before the first window has
   established the Vulkan frame/swapchain lifecycle. On Darwin with Vulkan
   initialized, startup assigns the existing conservative GPU class 3 directly
   and does not run that benchmark. This is not a GL fallback and does not
   disable Vulkan or MoltenVK rendering.
8. Present Engine jobs that synchronously wait on a stack-owned completion
   object notify that object while holding its mutex. This prevents the waiting
   submitter from returning and destroying the condition variable before the
   Present Engine has finished the notification.
9. Scene-per-draw descriptor sets are populated exactly once from the shader's
   declared layout. A second hard-coded shared-UBO update is not permitted,
   because it duplicates that work without preserving each layout's descriptor
   type and crashes MoltenVK during deferred terrain rendering.

## Scope boundaries

- `indra/llrender/volk.c`, `indra/llrender/llvkloader.cpp`,
  `indra/newview/llfeaturemanager.cpp`, and `indra/newview/llappviewer.cpp`
  contain the related runtime/bootstrap changes.
- All new behavior is under `LL_DARWIN`; Linux and Windows retain their existing
  Loader discovery and instance-extension behavior.
- Existing manifest/CMake edits that stage Loader, MoltenVK, and the ICD are
  prerequisites and are not replaced here.
- No `DYLD_LIBRARY_PATH`, `VK_ICD_FILENAMES`, `VK_LAYER_PATH`, or other shell
  override is part of product-equivalent launch validation.

## Verification

Build the arm64 `ayastorm-bin` target, then launch the bundle normally. The
startup log must include all of the following before login-screen creation:

```
Vulkan: macOS Loader driver manifest: .../Resources/vulkan/icd.d/MoltenVK_icd.json
Vulkan: VK_KHR_portability_enumeration enabled
Vulkan: initialized device=...
Vulkan: macOS swapchain drawable extent=...
RenderInit: Vulkan/MoltenVK startup: bypassing legacy GPU benchmark; using GPU class 3
```

When the previous cache is no longer compatible, the following warning is
expected once and startup must continue with `initialized device=...`:

```
Vulkan: vkCreatePipelineCache rejected cached data result=...; retrying with an empty cache
```

The visible acceptance condition is the normal login screen, not a black window.
The bundle must still contain the three runtime artifacts above, and
`codesign --verify --deep --strict` must pass.

## Failure diagnostics

- `missing bundled driver manifest`: the packaging/manifest staging prerequisite
  is absent.
- `volkInitialize failed`: the bundled Loader dylib cannot be opened.
- `VK_KHR_portability_enumeration unavailable`: the loaded driver is not a
  usable MoltenVK portability driver.
- `no suitable physical device`: the Loader found a driver, but it did not expose
  a Vulkan 1.3-capable device required by this renderer.
- `vkCreatePipelineCache rejected cached data`: a stale driver-specific cache was
  detected; the empty-cache retry is the expected recovery path.
