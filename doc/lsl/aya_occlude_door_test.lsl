// =====================================================================
//  aya_occlude_door_test.lsl — AYAstorm r13 dynamic occluder test door
//
//  r13 で導入した [ayastorm:occlude] タグの「動く occluder の追従」
//  動作を in-world で検証するためのヒンジ式回転ドア。
//  refreshOccluders が毎 tick OBB を再取得していることを耳と目で
//  確認するのが目的。
//
//  配置:
//    1. ドアパネルをパスカット (Path Cut Begin=0.5 など) で半分にし、
//       visible 半パネルの根元 = prim center = ヒンジ になるよう成形
//    2. ルートプリム + そのドアパネル child を link (ドア = link #2)
//    3. ドアパネル (子) の Description に
//         [ayastorm:occlude{direct:0.7}{reverb:0.5}]
//       を入れる (値は実物に合わせて調整可)
//    4. ドアの反対側に [3dstream:url=...] の発音プリムを置いて再生
//    5. このスクリプトを ROOT prim に投入
//    6. ドアをクリック → 1.5 秒かけて 90° 回転で開閉
//
//  チューニング:
//    OPEN_AXIS  — 回転軸 (ルート相対のローカル座標)
//                 直立扉なら <0,0,1>。横倒し扉や跳ね上げなら適宜。
//    OPEN_ANGLE — 開ききった時の角度 [度]。+ で反時計、- で時計回り。
//
//  確認ポイント:
//    - 閉時: lowpass + 減衰 / 開時 (パネルが横を向く): 素通し
//    - アニメ中も滑らかに遮蔽度が変化 (refreshOccluders 毎 tick 再読み)
//    - Stream3DShowOccluders ON (Alt+Shift+O) で OBB ワイヤーフレームが
//      ドアと一緒に回転するのが見える
// =====================================================================

integer DOOR_LINK   = 2;
vector  OPEN_AXIS   = <0.0, 0.0, 1.0>;    // 回転軸 (root local)、垂直ヒンジ = Z
float   OPEN_ANGLE  = 90.0;                // 開角 [deg]
float   ANIM_TIME   = 1.5;                 // 開閉アニメ時間 [s]
float   ANIM_STEP   = 0.05;                // 30 frame / 1.5 s

rotation gClosedRot;
integer  gOpen      = FALSE;
integer  gAnimating = FALSE;
float    gT0;
float    gFromAngle;
float    gToAngle;

apply_angle(float angle_deg)
{
    rotation delta = llAxisAngle2Rot(OPEN_AXIS, angle_deg * DEG_TO_RAD);
    llSetLinkPrimitiveParamsFast(DOOR_LINK,
        [PRIM_ROT_LOCAL, delta * gClosedRot]);
}

default
{
    state_entry()
    {
        gClosedRot = llList2Rot(
            llGetLinkPrimitiveParams(DOOR_LINK, [PRIM_ROT_LOCAL]), 0);
    }

    touch_start(integer n)
    {
        if (gAnimating)
        {
            return;
        }
        gOpen = !gOpen;
        if (gOpen)
        {
            gFromAngle = 0.0;
            gToAngle   = OPEN_ANGLE;
        }
        else
        {
            gFromAngle = OPEN_ANGLE;
            gToAngle   = 0.0;
        }
        gT0        = llGetTime();
        gAnimating = TRUE;
        llSetTimerEvent(ANIM_STEP);
    }

    timer()
    {
        float dt = llGetTime() - gT0;
        float k  = dt / ANIM_TIME;
        if (k >= 1.0)
        {
            k = 1.0;
            llSetTimerEvent(0.0);
            gAnimating = FALSE;
        }
        float a = gFromAngle + (gToAngle - gFromAngle) * k;
        apply_angle(a);
    }
}
