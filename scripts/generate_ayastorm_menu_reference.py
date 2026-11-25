#!/usr/bin/env python3
"""Generate the AYAstorm release menu inventory from a pinned Git tag."""

import csv
import json
import re
import subprocess
import xml.etree.ElementTree as ET
from collections import Counter
from pathlib import Path


TAG = "v7.2.4-ayastorm-r31-bugfix-2"
ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "docs/specs"
XUI = "indra/newview/skins/default/xui"
SKIP = {
    "menu_bar.xml", "menu_item.xml", "menu_item_call.xml",
    "menu_item_check.xml", "menu_item_separator.xml", "menu_item_tear_off.xml",
}
STRUCTURAL = {
    "menu_bar", "menu", "context_menu", "toggleable_menu", "pie_menu",
    "menu_item_call", "menu_item_check", "pie_slice",
}


def git(*args):
    return subprocess.check_output(["git", *args], cwd=ROOT, text=True)


def root_xml(path):
    return ET.fromstring(git("show", f"{TAG}:{path}"))


def children(node):
    return [child for child in node if child.tag in STRUCTURAL]


def localized(parent, node, ordinal):
    if parent is None:
        return None
    matches = [child for child in children(parent)
               if child.tag == node.tag and child.get("name") == node.get("name")]
    return matches[ordinal] if ordinal < len(matches) else None


def callback(node, suffix):
    for child in node:
        if child.tag == suffix or child.tag.endswith("." + suffix):
            function = child.get("function", "")
            parameter = child.get("parameter", child.get("control", ""))
            return function + (f"({parameter})" if parameter else "")
    return ""


def one_line(value):
    return (value or "").replace("\n", " ").replace("|", "｜").strip()


def draft_bucket(filename, names):
    """Path-based planning bucket, never a confirmed AYAviewer placement."""
    if filename == "menu_login.xml":
        return "ログイン前"
    if filename == "menu_viewer.xml":
        top = names[1] if len(names) > 1 else ""
        second = names[2] if len(names) > 2 else ""
        last = names[-1]
        if top == "Me":
            if second in {"Take Off >", "avhealth"} or last in {
                    "Inventory", "NowWearing", "ChangeOutfit", "WearableFavorites"}:
                return "自分・外見・持ち物"
            if second == "Movement" or last in {"Movement Controls", "Fly", "Stop flying"}:
                return "場所・移動"
            if last in {"Take Snapshot", "Capture 360", "Camera Controls"}:
                return "表示・撮影"
            if last in {"Preferences", "Toolbar Buttons"}:
                return "設定・UI"
            return "自分・外見・持ち物"
        if top == "Communicate":
            return "会話・人"
        if top == "World":
            if second == "photo_and_video":
                return "表示・撮影"
            if second in {"Environment", "LandShow", "AvatarComplexityMode"} or last in {
                    "Performance", "Avatar Render Settings", "Render Friends Only"}:
                return "表示・撮影"
            return "場所・移動"
        return {
            "BuildTools": "制作", "Content": "情報・外部リンク",
            "AYAstorm": "表示・撮影", "Help": "ヘルプ",
            "RLVa Main": "RLVa", "Advanced": "診断・詳細設定",
            "Develop": "開発者ツール", "Admin": "管理権限",
            "Deprecated": "旧定義・移行保留",
        }.get(top, "要分類")
    n = filename.lower()
    if any(word in n for word in ("inventory", "outfit", "wearing", "wearable",
                                   "attachment", "cof_", "gallery", "avatar_self")):
        return "対象別：自分・持ち物"
    if any(word in n for word in ("people", "conversation", "chat", "imchiclet",
                                   "participant", "profile", "radar", "group",
                                   "avatar_other", "contact", "block_list")):
        return "対象別：会話・人"
    if any(word in n for word in ("land", "place", "map", "teleport", "navbar", "slurl")):
        return "対象別：場所"
    if any(word in n for word in ("object", "copy_paste", "model", "script",
                                   "settings", "lsl_")):
        return "対象別：制作"
    if any(word in n for word in ("toolbar", "topinfobar", "hide_navbar")):
        return "対象別：UI"
    return "対象別：その他"


def records(filename, en_root, ja_root):
    rows = []

    def visit(node, ja, parents_en, parents_ja, parents_name, hidden_parent=False):
        en = one_line(node.get("label") or node.get("name") or node.tag)
        jp = one_line(ja.get("label")) if ja is not None else ""
        display = jp or en
        explicit_hidden = node.get("visible") == "false"
        hidden = hidden_parent or explicit_hidden
        visible_fn = callback(node, "on_visible")
        row = {
            "surface": filename, "node_index": len(rows) + 1,
            "depth": len(parents_name),
            "kind": node.tag, "name": node.get("name", ""),
            "draft_bucket": draft_bucket(filename, [*parents_name, node.get("name", node.tag)]),
            "path_ja": " > ".join([*parents_ja, display]),
            "path_en": " > ".join([*parents_en, en]),
            "path_name": " > ".join([*parents_name, node.get("name", node.tag)]),
            "label_ja": jp, "label_en": en,
            "xml_visible": node.get("visible", ""),
            "hidden_by_parent": str(hidden_parent).lower(),
            "on_visible": visible_fn,
            "on_enable": callback(node, "on_enable"),
            "on_click": callback(node, "on_click"),
            "shortcut": node.get("shortcut", ""),
            "source": f"{XUI}/en/{filename}",
            "display": display,
            "hidden": hidden,
        }
        rows.append(row)
        counts = Counter()
        for child in children(node):
            key = (child.tag, child.get("name"))
            child_ja = localized(ja, child, counts[key])
            counts[key] += 1
            visit(child, child_ja, [*parents_en, en], [*parents_ja, display],
                  [*parents_name, node.get("name", node.tag)], hidden)

    visit(en_root, ja_root, [], [], [])
    return rows


def tree(rows):
    lines = []
    for row in rows:
        depth = row["depth"]
        label = row["display"]
        if row["label_ja"] and row["label_ja"] != row["label_en"]:
            label += f" / {row['label_en']}"
        if row["surface"] == "menu_viewer.xml" and depth == 1 and row["name"] == "Deprecated":
            label += "（旧定義・通常は非表示）"
        flags = []
        if row["kind"] == "menu_item_check":
            flags.append("切替")
        if row["xml_visible"] == "false":
            flags.append("初期非表示")
        if row["on_visible"]:
            flags.append("表示条件あり")
        if row["on_enable"]:
            flags.append("有効条件あり")
        if flags:
            label += " [" + "・".join(flags) + "]"
        lines.append("  " * depth + "- " + label)
    return lines


def main():
    names = git("ls-tree", "-r", "--name-only", TAG, f"{XUI}/en").splitlines()
    files = sorted(p.rsplit("/", 1)[-1] for p in names
                   if p.rsplit("/", 1)[-1].startswith("menu_")
                   and p.endswith(".xml")
                   and p.rsplit("/", 1)[-1] not in SKIP)
    all_rows = []
    sections = []
    for filename in files:
        en = root_xml(f"{XUI}/en/{filename}")
        try:
            ja = root_xml(f"{XUI}/ja/{filename}")
        except subprocess.CalledProcessError:
            ja = None
        rows = records(filename, en, ja)
        all_rows += rows
        sections.append((filename, rows))

    columns = [
        "surface", "node_index", "depth", "kind", "name", "draft_bucket",
        "path_ja", "path_en", "label_ja",
        "path_name", "label_en", "xml_visible", "hidden_by_parent", "on_visible",
        "on_enable", "on_click", "shortcut", "source",
    ]
    OUT.mkdir(parents=True, exist_ok=True)
    with (OUT / "ayaviewer-ui-menu-items-r31.csv").open("w", encoding="utf-8-sig", newline="") as file:
        writer = csv.DictWriter(file, columns, extrasaction="ignore")
        writer.writeheader()
        writer.writerows(all_rows)

    grep_lines = git("grep", "-n", "-E", '"menu_[A-Za-z0-9_]+\\.xml"', TAG,
                     "--", "indra/newview", ":!indra/newview/skins").splitlines()
    references = []
    refs_by_menu = {}
    for line in grep_lines:
        _, source, number, snippet = line.split(":", 3)
        for filename in re.findall(r'"(menu_[A-Za-z0-9_]+\.xml)"', snippet):
            if filename not in files:
                continue
            references.append((filename, source, number, snippet.strip()))
            refs_by_menu.setdefault(filename, []).append(f"{source}:{number}")
    with (OUT / "ayaviewer-ui-menu-code-references-r31.csv").open(
            "w", encoding="utf-8-sig", newline="") as file:
        writer = csv.writer(file)
        writer.writerow(["menu_file", "code_file", "line", "reference"])
        writer.writerows(references)

    compact = []
    stack = []
    previous_file = None
    for row in all_rows:
        if row["surface"] != previous_file:
            stack = []
            previous_file = row["surface"]
        depth = row["depth"]
        parent = stack[depth - 1] if depth else -1
        item = {
            "s": row["surface"], "i": row["node_index"], "bucket": row["draft_bucket"],
            "k": row["kind"], "n": row["name"],
            "ja": row["display"], "en": row["label_en"],
            "p": row["path_ja"], "pe": row["path_en"],
            "pn": row["path_name"], "v": row["xml_visible"],
            "hidden": row["hidden"], "hp": row["hidden_by_parent"] == "true",
            "vis": row["on_visible"], "ena": row["on_enable"],
            "act": row["on_click"], "key": row["shortcut"],
            "src": row["source"], "parent": parent,
            "refs": refs_by_menu.get(row["surface"], []),
        }
        compact.append(item)
        stack = stack[:depth]
        stack.append(len(compact) - 1)
    template = (ROOT / "scripts/ayastorm_menu_explorer_template.html").read_text(encoding="utf-8")
    payload = json.dumps(compact, ensure_ascii=False, separators=(",", ":")).replace("<", "\\u003c")
    (OUT / "ayaviewer-ui-menu-explorer-r31.html").write_text(
        template.replace("/*MENU_DATA*/", payload), encoding="utf-8")

    order = {"menu_viewer.xml": 0, "menu_login.xml": 1}
    sections.sort(key=lambda entry: (
        order.get(entry[0], 3 if entry[0].startswith("menu_pie_") else 2), entry[0]))
    md = [
        "# AYAstorm r31-bugfix-2 メニュー全階層",
        "",
        "AYAviewer UI 設計用の現状資料。対象はリリースタグ "
        f"`{TAG}`（`214054a5a71619c83a9802d37a185f6bc44c6908`）の "
        "`skins/default/xui/en/menu_*.xml`。日本語表示名は同タグの `xui/ja` に"
        "対応名がある場合だけ採用し、対応がない項目は英語を残した。",
        "",
        "この階層は XML 定義を列挙した静的資料。実画面ではログイン状態、権限、"
        "グリッド、RLVa、選択対象、設定、実行時追加・削除によって表示と操作可否が変わる。"
        "「初期非表示」はそのノードの XML 属性、「表示条件あり」「有効条件あり」は"
        "コールバックの存在を示す。親が非表示なら子も表示されない。"
        "セパレーター、コールバック要素、汎用ウィジェット定義 6 件は省いた。",
        "",
        "設計上の機能整理は [作業台](ayaviewer-ui-ia-working-map-r31.ja.md)、"
        "階層の検索・展開は [探索図](ayaviewer-ui-menu-explorer-r31.html)、"
        "経路と処理の機械検索は [CSV](ayaviewer-ui-menu-items-r31.csv) を参照。",
        "",
        "## 通常画面のメニューバー",
        "",
    ]
    for filename, rows in sections:
        if filename == "menu_login.xml":
            md += ["## ログイン画面のメニューバー", ""]
        elif filename != "menu_viewer.xml" and sections[sections.index((filename, rows))-1][0] == "menu_login.xml":
            md += ["## その他のクリック／右クリックメニュー", "",
                   "以下の入口名はファイル名からの推定であり、個々の起動箇所を実機確認したものではない。", ""]
        elif filename.startswith("menu_pie_") and not any(x.startswith("## 円形") for x in md):
            md += ["## 円形コンテキストメニュー", ""]
        md += [f"### `{filename}`", "",
               f"定義: `{XUI}/en/{filename}`", ""]
        md += tree(rows)
        md += [""]
    (OUT / "ayaviewer-ui-menu-tree-r31.ja.md").write_text("\n".join(md), encoding="utf-8")
    print(f"{len(files)} menu definitions, {len(all_rows)} hierarchy nodes, "
          f"{len(references)} code references")
    print(f"menu_viewer nodes: {len(next(rows for fn, rows in sections if fn == 'menu_viewer.xml'))}")


if __name__ == "__main__":
    main()
