#!/usr/bin/env python3
"""app/cjson/bin/extract_package.py

packages/ 配下の cJSON アーカイブ (zip) を prod/include/cjson,
prod/libsrc/cjson へ展開する。外部ツール (unzip 等) に依存せず、
標準ライブラリ zipfile のみを使用する。

展開後、patches/ 配下の unified diff (framework/makefw/bin/apply_patches.py)
を順に適用する。zip の内容は加工せずそのまま書き出し、cJSON 本体への
改変はすべてパッチ側で行う。
"""

import argparse
import errno
import os
import re
import stat
import sys
import tempfile
import time
import zipfile
from pathlib import Path

sys.stdout.reconfigure(encoding="utf-8")
sys.stderr.reconfigure(encoding="utf-8")

PACKAGE_NAME_PATTERN = re.compile(r"^cJSON-.*\.zip$", re.IGNORECASE)
VERSION_PATTERN = re.compile(r"^cJSON-(\d+)\.(\d+)\.(\d+)\.zip$", re.IGNORECASE)

# 展開対象: zip 内のファイル名 -> 展開先 (プレースホルダーは app_dir からの相対パス)
#
# cJSON.c / cJSON_Utils.c は内部で #include "cJSON.h" のように同一ディレクトリ
# 相対 (ダブル クォート) インクルードを使う。サブディレクトリ (cjson/ 等) に
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

# 展開後、パッチ適用前に代表ファイルを最後に置換する順序に使う。
MARKER_SOURCE = "cJSON.c"

# 再展開要否の判定に使うスタンプ ファイル。
STAMP_FILENAME = "make_extract.stamp"

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


def _same_content(path, data):
    """path の内容が data と一致すれば True。読めない場合は False。"""
    try:
        if isinstance(data, str):
            with open(path, "r", encoding="utf-8", newline="") as f:
                return f.read() == data
        with open(path, "rb") as f:
            return f.read() == data
    except OSError:
        return False


def _is_retryable_replace_error(exc):
    """並列プロセスによる Windows の replace 失敗を再試行対象とみなす。"""
    if isinstance(exc, PermissionError):
        return True
    if isinstance(exc, OSError):
        winerror = getattr(exc, "winerror", None)
        # 5: ACCESS_DENIED, 32: SHARING_VIOLATION
        if winerror in (5, 32):
            return True
        if exc.errno in (errno.EACCES, errno.EPERM):
            return True
    return False


def atomic_replace(path, data, retries=10, base_delay=0.05):
    """同じディレクトリの一意な一時ファイルを使ってファイルを置換する。

    内容が既に同一なら何もしない。Windows の並列 make で複数プロセスが
    同一パスへ os.replace する際の PermissionError (WinError 5) 等には
    再試行する。再試行中に他プロセスが正しい内容を書いた場合は成功とみなす。
    """
    if _same_content(path, data):
        return

    dir_path = os.path.dirname(path) or "."
    prefix = f".{os.path.basename(path)}."
    try:
        file_mode = stat.S_IMODE(os.stat(path).st_mode)
    except FileNotFoundError:
        current_umask = os.umask(0)
        os.umask(current_umask)
        file_mode = 0o666 & ~current_umask

    last_err = None
    for attempt in range(retries):
        if attempt > 0 and _same_content(path, data):
            return

        tmp_path = None
        try:
            if isinstance(data, str):
                with tempfile.NamedTemporaryFile(
                    mode="w",
                    encoding="utf-8",
                    newline="",
                    dir=dir_path,
                    prefix=prefix,
                    suffix=".tmp",
                    delete=False,
                ) as f:
                    tmp_path = f.name
                    f.write(data)
            else:
                with tempfile.NamedTemporaryFile(
                    mode="wb",
                    dir=dir_path,
                    prefix=prefix,
                    suffix=".tmp",
                    delete=False,
                ) as f:
                    tmp_path = f.name
                    f.write(data)
            os.chmod(tmp_path, file_mode)
            os.replace(tmp_path, path)
            return
        except OSError as e:
            if not _is_retryable_replace_error(e):
                raise
            last_err = e
            time.sleep(base_delay * (2 ** min(attempt, 4)))
        finally:
            if tmp_path is not None:
                try:
                    os.unlink(tmp_path)
                except FileNotFoundError:
                    pass

    if _same_content(path, data):
        return
    raise last_err


def iter_target_paths(app_dir):
    for rel_parts in EXTRACT_TARGETS.values():
        yield os.path.join(app_dir, *rel_parts)


def ensure_gitignore(app_dir):
    for rel_parts, names in GITIGNORE_TARGETS.items():
        dir_path = os.path.join(app_dir, *rel_parts)
        os.makedirs(dir_path, exist_ok=True)
        gitignore_path = os.path.join(dir_path, ".gitignore")
        content = GITIGNORE_HEADER + "".join(f"/{name}\n" for name in names)
        atomic_replace(gitignore_path, content)


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


def stamp_path(app_dir):
    return os.path.join(app_dir, STAMP_FILENAME)


def compute_stamp_fields(zip_path, selected_name, patches_dir, apply_patches_mod):
    """スタンプへ書き出す項目を辞書で返す。zip の stat とパッチ系列の
    digest のみを使い、展開先ファイルの内容はハッシュしない
    (makepart.mk から 1 ビルドで何十回も起動されるため軽量さを優先する)。
    """
    st = os.stat(zip_path)
    return {
        "package": selected_name,
        "zip_mtime": repr(st.st_mtime),
        "zip_size": str(st.st_size),
        "patches_digest": apply_patches_mod.series_digest(Path(patches_dir)),
    }


def read_stamp(path):
    """スタンプ ファイルを {キー: 値} で返す。読めない場合は None。"""
    try:
        with open(path, "r", encoding="utf-8") as f:
            lines = f.read().splitlines()
    except OSError:
        return None

    fields = {}
    for line in lines:
        if not line or "=" not in line:
            return None
        key, _, value = line.partition("=")
        fields[key] = value
    return fields


def write_stamp(path, fields):
    content = "".join(f"{key}={value}\n" for key, value in fields.items())
    atomic_replace(path, content)


def needs_extraction(zip_path, app_dir, selected_name, patches_dir, apply_patches_mod):
    if any(not os.path.isfile(path) for path in iter_target_paths(app_dir)):
        return True

    current = read_stamp(stamp_path(app_dir))
    if current is None:
        return True

    expected = compute_stamp_fields(zip_path, selected_name, patches_dir, apply_patches_mod)
    return current != expected


def extract(zip_path, app_dir):
    """zip の内容を加工せず、EXTRACT_TARGETS の展開先へそのまま書き出す。"""
    dest_paths = {}
    for src_name, rel_parts in EXTRACT_TARGETS.items():
        dest_path = os.path.join(app_dir, *rel_parts)
        os.makedirs(os.path.dirname(dest_path), exist_ok=True)
        dest_paths[src_name] = dest_path

    with zipfile.ZipFile(zip_path) as zf:
        names = zf.namelist()
        # 代表ファイルを最後に置換し、全出力の準備前に別プロセスが
        # 展開完了と判定しないようにする。
        ordered_sources = [name for name in dest_paths if name != MARKER_SOURCE]
        ordered_sources.append(MARKER_SOURCE)
        for src_name in ordered_sources:
            dest_path = dest_paths[src_name]
            member = find_member(names, src_name)
            if member is None:
                print(f"ERROR: zip 内に {src_name} が見つかりません: {zip_path}", file=sys.stderr)
                return False
            data = zf.read(member)
            atomic_replace(dest_path, data)

    # パッチ適用がこの後ファイルを書き換え、mtime は適用時刻になる。
    # これはパッチ変更時に make がソースの更新を検知するために必要な
    # 正しい挙動であり、パッチ適用後に mtime を zip の値へ戻さないこと。
    zip_mtime = os.path.getmtime(zip_path)
    for dest_path in dest_paths.values():
        os.utime(dest_path, (zip_mtime, zip_mtime))
    return True


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--app-dir", required=True)
    parser.add_argument(
        "--makefw-home",
        required=True,
        help="framework/makefw のパス。<makefw-home>/bin を sys.path へ加えて "
        "apply_patches を import するために使う。",
    )
    args = parser.parse_args()

    sys.path.insert(0, os.path.join(args.makefw_home, "bin"))
    import apply_patches  # noqa: E402  (sys.path 設定後に import する)

    packages_dir = os.path.join(args.app_dir, "packages")
    patches_dir = os.path.join(args.app_dir, "patches")
    candidates = find_candidates(packages_dir)

    if not candidates:
        print_missing_package_guide(packages_dir)
        return 1

    selected, rejected = select_package(packages_dir, candidates)
    if rejected:
        print_multiple_package_warning(selected, rejected)

    zip_path = os.path.join(packages_dir, selected)

    ensure_gitignore(args.app_dir)

    if not needs_extraction(zip_path, args.app_dir, selected, patches_dir, apply_patches):
        return 0

    stamp_file = stamp_path(args.app_dir)
    try:
        os.remove(stamp_file)
    except FileNotFoundError:
        pass

    print(f"INFO: cJSON パッケージを展開しています: {selected}", file=sys.stderr)
    if not extract(zip_path, args.app_dir):
        return 2

    try:
        apply_patches.apply_series(Path(patches_dir), Path(args.app_dir))
    except apply_patches.PatchError as exc:
        print(f"ERROR: cJSON パッチの適用に失敗しました: {exc}", file=sys.stderr)
        return 3

    write_stamp(
        stamp_file,
        compute_stamp_fields(zip_path, selected, patches_dir, apply_patches),
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
