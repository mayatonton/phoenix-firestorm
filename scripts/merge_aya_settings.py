import xml.etree.ElementTree as ET, copy, sys

REPO = "indra/newview/app_settings/settings.xml"
AYA  = "/home/ishikawa/.ayastorm_x64/user_settings/settings.xml"
OUT  = "indra/newview/app_settings/settings.AYA-merged.xml"

def top_map(path):
    root = ET.parse(path).getroot()      # <llsd>
    m = root.find("map")
    return root, m

def kv_pairs(map_elem):
    # map children alternate: key, value, key, value...
    ch = list(map_elem)
    d = {}
    for i in range(0, len(ch)-1, 2):
        if ch[i].tag == "key":
            d[ch[i].text] = ch[i+1]      # value element
    return d

def get_value_elem(cvar_map):
    # cvar_map is the inner <map> with Comment/Persist/Type/Value
    ch = list(cvar_map)
    for i in range(0, len(ch)-1, 2):
        if ch[i].tag == "key" and ch[i].text == "Value":
            return cvar_map, i+1, ch[i+1]
    return cvar_map, None, None

repo_root, repo_map = top_map(REPO)
aya_root,  aya_map  = top_map(AYA)
repo_cvars = kv_pairs(repo_map)
aya_cvars  = kv_pairs(aya_map)

overridden, missing_in_repo, no_value = 0, [], []
for name, aya_cvarmap in aya_cvars.items():
    if name not in repo_cvars:
        missing_in_repo.append(name); continue
    _, a_idx, a_val = get_value_elem(aya_cvarmap)
    repo_cvarmap = repo_cvars[name]
    rmap, r_idx, r_val = get_value_elem(repo_cvarmap)
    if a_val is None or r_idx is None:
        no_value.append(name); continue
    rmap[r_idx] = copy.deepcopy(a_val)   # replace repo Value with AYA Value
    overridden += 1

ET.ElementTree(repo_root).write(OUT, encoding="unicode", xml_declaration=False)
print(f"repo cvars         = {len(repo_cvars)}")
print(f"AYA override cvars = {len(aya_cvars)}")
print(f"applied (overridden)= {overridden}")
print(f"AYA-only (repo 不在) = {len(missing_in_repo)} (skip) e.g. {missing_in_repo[:5]}")
print(f"no Value node       = {len(no_value)} {no_value[:5]}")
