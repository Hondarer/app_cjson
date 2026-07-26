#!/usr/bin/env python3
"""app/cjson/bin/extract_package.py

packages/ 配下の cJSON アーカイブ (zip) を prod/include/cjson,
prod/libsrc/cjson へ展開する。外部ツール (unzip 等) に依存せず、
標準ライブラリ zipfile のみを使用する。
"""

import argparse
import os
import re
import sys
import zipfile

sys.stdout.reconfigure(encoding="utf-8")
sys.stderr.reconfigure(encoding="utf-8")

PACKAGE_NAME_PATTERN = re.compile(r"^cJSON-.*\.zip$", re.IGNORECASE)
VERSION_PATTERN = re.compile(r"^cJSON-(\d+)\.(\d+)\.(\d+)\.zip$", re.IGNORECASE)

# 展開対象: zip 内のファイル名 -> 展開先 (プレースホルダーは app_dir からの相対パス)
#
# cJSON.c / cJSON_Utils.c は内部で #include "cJSON.h" のように同一ディレクトリ
# 相対 (ダブルクォート) インクルードを使う。サブディレクトリ (cjson/ 等) に
# ヘッダーを配置すると、コンパイラの include 検索パス (-I prod/include) だけでは
# 解決できず cJSON 自身のビルドが失敗するため、prod/include 直下に配置する
# (app/calc の calc.h / calcbase.h と同じ「直下アンブレラ ヘッダー」パターン)。
EXTRACT_TARGETS = {
    "cJSON.h": ("prod", "include", "cJSON.h"),
    "cJSON.c": ("prod", "libsrc", "cjson", "cJSON.c"),
    "cJSON_Utils.h": ("prod", "include", "cJSON_Utils.h"),
    "cJSON_Utils.c": ("prod", "libsrc", "cjson", "cJSON_Utils.c"),
    "LICENSE": ("prod", "libsrc", "cjson", "LICENSE.cjson"),
}

# 再展開要否の判定に使う代表ファイル
MARKER_TARGET = ("prod", "libsrc", "cjson", "cJSON.c")

# 生成物を除外するための .gitignore を配置するディレクトリと、その内容。
#
# framework/makefw/makefiles/makelibsrc_c_cpp.mk / makesrc_c_cpp.mk は、
# TEST_SRCS/ADD_SRCS を使わないビルド リーフ ディレクトリでは、
# `make clean` のたびに無条件で .gitignore を削除する
# (CP_SRCS/LINK_SRCS 経由で取り込むディレクトリ専用に .gitignore を
#  自動再生成する仕組みの副作用であり、TEST_SRCS/ADD_SRCS 不使用の
#  prod/libsrc/cjson もこの削除対象に含まれる)。
# そのため、手動配置の .gitignore はここで毎回 (再展開の有無に関わらず)
# 再生成し、`make clean` 後も次回 make で必ず復元されるようにする。
GITIGNORE_TARGETS = {
    ("prod", "libsrc", "cjson"): ["cJSON.c", "cJSON_Utils.c", "LICENSE.cjson"],
}


GITIGNORE_HEADER = "# app/cjson/packages 配下の zip から展開される生成物。手動改変しないため Git 管理対象外とする。\n"


def ensure_gitignore(app_dir):
    for rel_parts, names in GITIGNORE_TARGETS.items():
        dir_path = os.path.join(app_dir, *rel_parts)
        os.makedirs(dir_path, exist_ok=True)
        gitignore_path = os.path.join(dir_path, ".gitignore")
        content = GITIGNORE_HEADER + "".join(f"/{name}\n" for name in names)
        tmp_path = gitignore_path + ".tmp"
        with open(tmp_path, "w", encoding="utf-8", newline="") as f:
            f.write(content)
        os.replace(tmp_path, gitignore_path)


def find_candidates(packages_dir):
    if not os.path.isdir(packages_dir):
        return []
    return sorted(f for f in os.listdir(packages_dir) if PACKAGE_NAME_PATTERN.match(f))


def parse_version(filename):
    m = VERSION_PATTERN.match(filename)
    if m is None:
        return None
    return tuple(int(part) for part in m.groups())


def select_package(packages_dir, candidates):
    """複数候補がある場合、ファイル名のバージョン番号が最も新しいものを採用する。
    バージョン番号が抽出できないファイルが混在する場合は、mtime が最も新しい
    ものにフォールバックする。"""
    if len(candidates) == 1:
        return candidates[0], []

    versions = {name: parse_version(name) for name in candidates}
    if all(v is not None for v in versions.values()):
        selected = max(candidates, key=lambda name: versions[name])
    else:
        selected = max(
            candidates,
            key=lambda name: os.path.getmtime(os.path.join(packages_dir, name)),
        )
    rejected = [name for name in candidates if name != selected]
    return selected, rejected


def print_missing_package_guide(packages_dir):
    lines = [
        "",
        "ERROR: cJSON のアーカイブ (zip) が app/cjson/packages に見つかりません。",
        "",
        f"  配置先: {packages_dir}",
        "  ファイル名の例: cJSON-1.7.18.zip (バージョン番号をファイル名に含めること)",
        "",
        "  GitHub のタグ アーカイブを取得する場合の例 (curl):",
        "    curl -L -o app/cjson/packages/cJSON-1.7.18.zip \\",
        "      https://github.com/DaveGamble/cJSON/archive/refs/tags/v1.7.18.zip",
        "",
        "  取得後、このディレクトリには常に 1 個の zip のみを配置してください。",
        "  バージョン更新時は、古いアーカイブを新しいものに置き換えてください。",
        "",
    ]
    print("\n".join(lines), file=sys.stderr)


def print_multiple_package_warning(selected, rejected):
    lines = [
        "",
        "WARNING: app/cjson/packages に複数の cJSON アーカイブが見つかりました。",
        f"  採用: {selected} (バージョンが最も新しいと判断)",
    ]
    lines += [f"  未採用: {name}" for name in rejected]
    lines += [
        "  単一ファイル運用のため、未採用のアーカイブは削除してください。",
        "",
    ]
    print("\n".join(lines), file=sys.stderr)


def find_member(names, filename):
    """zip 内から <トップディレクトリ>/filename に一致するメンバー名を返す。"""
    matches = [n for n in names if n.split("/", 1)[-1] == filename and n.count("/") == 1]
    return matches[0] if matches else None


def needs_extraction(zip_path, app_dir):
    marker = os.path.join(app_dir, *MARKER_TARGET)
    if not os.path.isfile(marker):
        return True
    return os.path.getmtime(zip_path) > os.path.getmtime(marker)


def extract(zip_path, app_dir):
    dest_paths = {}
    for src_name, rel_parts in EXTRACT_TARGETS.items():
        dest_path = os.path.join(app_dir, *rel_parts)
        os.makedirs(os.path.dirname(dest_path), exist_ok=True)
        dest_paths[src_name] = dest_path

    with zipfile.ZipFile(zip_path) as zf:
        names = zf.namelist()
        for src_name, dest_path in dest_paths.items():
            member = find_member(names, src_name)
            if member is None:
                print(f"ERROR: zip 内に {src_name} が見つかりません: {zip_path}", file=sys.stderr)
                return False
            data = zf.read(member)
            tmp_path = dest_path + ".tmp"
            with open(tmp_path, "wb") as f:
                f.write(data)
            os.replace(tmp_path, dest_path)

    zip_mtime = os.path.getmtime(zip_path)
    for dest_path in dest_paths.values():
        os.utime(dest_path, (zip_mtime, zip_mtime))
    return True


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--app-dir", required=True)
    args = parser.parse_args()

    packages_dir = os.path.join(args.app_dir, "packages")
    candidates = find_candidates(packages_dir)

    if not candidates:
        print_missing_package_guide(packages_dir)
        return 1

    selected, rejected = select_package(packages_dir, candidates)
    if rejected:
        print_multiple_package_warning(selected, rejected)

    zip_path = os.path.join(packages_dir, selected)

    ensure_gitignore(args.app_dir)

    if not needs_extraction(zip_path, args.app_dir):
        return 0

    print(f"INFO: cJSON パッケージを展開しています: {selected}", file=sys.stderr)
    ok = extract(zip_path, args.app_dir)
    return 0 if ok else 2


if __name__ == "__main__":
    sys.exit(main())
