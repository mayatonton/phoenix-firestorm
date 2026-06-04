// AYAstorm r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.C PC-1
// Global_ReflectionProbes UBO shell blueprint
//   = set=0 binding=3 (06c §3 接合表機械決定)
//   = cadence = singleton owner (`Global_` prefix → CADENCE_SINGLETON in main.py:71)
//                + frame-start stable set bind (= Frame* と同居、06c §3 配置根拠)
//   = Phase 1.C PC-0 (Q1) AYA 確定値 = UB_REFLECTION_PROBES (= 06c §3 rename 後 UB_GLOBAL_REFLECTION_PROBES)
//     + Template A (singleton 系最小 UBO 起点、AYA literal「Claude 推奨で OK」2026-06-04)
//
// shell 性質 (= 09 §4.2 shell ↔ 本実装 layout 互換性):
//   - shell = 空 struct + 空 dirty flag + 空 flush 実装、zero memcpy data (1.C で経路通電のみ確認)
//   - shell の set/binding/size/PSO layout は Phase 2 本実装と完全一致 (= 不可触契約)
//   - shell の member 定義 / dirty 判定 / flush logic は Phase 2 で全面書換可 (= 書き捨て可能)
//   - buffer size = padded std140 size、chapter 08 §6.4 で 256B 倍数 padding (= 本 shell は std140=16B → device-padded 256B)
//
// Spec:
//   docs/specs/ayastorm-r41-gl-removal/design/06c-descriptor-set-bind-wiring.md §3 接合表 (set=0 binding=3)
//   docs/specs/ayastorm-r41-gl-removal/design/09-phase-roadmap.md §4.2 shell layout 互換性 + §5.2 Template A
//   docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-c-prep.md §4.1 PC-0 確定値

#version 450

layout(std140, set = 0, binding = 3) uniform Global_ReflectionProbes
{
    // shell placeholder (= Phase 2 で実 member 群に置換予定、layout 不可触 = set/binding/size 不変契約)
    // 1 vec4 = 16B std140、device-padded 256B → 256 倍数満足
    vec4 _shell_placeholder;
};

void main() {}
