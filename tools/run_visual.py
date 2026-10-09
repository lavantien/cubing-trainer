import hashlib
import os
import shutil
import subprocess
import sys
import time

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
VISUAL_CHECK = os.path.join(REPO_ROOT, "tools", "visual_check.py")
LOCK_STALE_SECONDS = 600
ANIMATED_ATTEMPTS = 2

EXIT_COMPARATOR = 1
EXIT_REGRESSION = 2
EXIT_FLAKE = 3
EXIT_BUSY = 4

STATIC_SCENES = [
    ("golden_solved_3.png", ["solved", "3"]),
    ("golden_solved_7.png", ["solved", "7"]),
    ("golden_scramble_3.png", ["scramble", "3", "R U R' U R U2 R'"]),
]
ANIMATED_SCENES = [
    ("golden_mid_3.png", ["mid", "3", "R U R' U R U2 R'"]),
]


def sha256_file(path):
    digest = hashlib.sha256()
    with open(path, "rb") as handle:
        digest.update(handle.read())
    return digest.hexdigest()


def acquire_lock(path):
    while True:
        try:
            fd = os.open(path, os.O_CREAT | os.O_EXCL | os.O_WRONLY)
            os.write(fd, str(os.getpid()).encode())
            os.close(fd)
            return True
        except FileExistsError:
            try:
                age = time.time() - os.path.getmtime(path)
            except OSError:
                return False
            if age < LOCK_STALE_SECONDS:
                return False
            os.remove(path)


def render_scene(binary, scene_args, out_path):
    proc = subprocess.run(
        [binary] + scene_args + [out_path],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
        cwd=REPO_ROOT,
    )
    if proc.returncode != 0 or not os.path.isfile(out_path):
        print(
            "visual: render failed for {} exit {}".format(" ".join(scene_args), proc.returncode)
        )
        sys.exit(EXIT_COMPARATOR)


def compare_with_golden(golden_path, fresh_path):
    proc = subprocess.run(
        [sys.executable, "-I", VISUAL_CHECK, golden_path, fresh_path],
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        cwd=REPO_ROOT,
        text=True,
    )
    return proc.returncode, proc.stdout.strip()


def run_static(binary, golden_dir, tmp_dir):
    for name, scene_args in STATIC_SCENES:
        fresh = os.path.join(tmp_dir, name)
        render_scene(binary, scene_args, fresh)
        code, output = compare_with_golden(os.path.join(golden_dir, name), fresh)
        if code != 0:
            print(output)
            print("visual: static scene {} mismatch, real regression".format(name))
            sys.exit(EXIT_COMPARATOR)
        print("pass: {} (single render)".format(name))


def run_animated(binary, golden_dir, tmp_dir, evidence_dir):
    for name, scene_args in ANIMATED_SCENES:
        golden = os.path.join(golden_dir, name)
        agreed = None
        disagreements = 0
        for attempt in range(ANIMATED_ATTEMPTS):
            first = os.path.join(tmp_dir, "pair_a_{}".format(name))
            second = os.path.join(tmp_dir, "pair_b_{}".format(name))
            render_scene(binary, scene_args, first)
            render_scene(binary, scene_args, second)
            if sha256_file(first) == sha256_file(second):
                agreed = first
                break
            disagreements += 1
        if agreed is None:
            print(
                "visual: {} renders disagree with themselves across {} pairs, infra flake "
                "from the two-attractor GPU state, not a code regression, see "
                "tests/golden/PROVENANCE.md".format(name, disagreements)
            )
            sys.exit(EXIT_FLAKE)
        code, output = compare_with_golden(golden, agreed)
        if code != 0:
            print(output)
            os.makedirs(evidence_dir, exist_ok=True)
            stamp = time.strftime("%Y%m%d-%H%M%S")
            shutil.copyfile(agreed, os.path.join(evidence_dir, "{}_{}".format(stamp, name)))
            print(
                "visual: {} self-consistent across 2 renders but differs from the committed "
                "golden, real regression signal unless the machine sits in the drifted "
                "attractor window, evidence at {}/{}_{}".format(name, evidence_dir, stamp, name)
            )
            sys.exit(EXIT_REGRESSION)
        print("pass: {} (2 renders self-agree, golden match)".format(name))


def main():
    if len(sys.argv) != 4:
        print("usage: run_visual.py <render_scene.exe> <golden_dir> <tmp_dir>")
        sys.exit(EXIT_COMPARATOR)
    binary = os.path.abspath(sys.argv[1])
    golden_dir = os.path.abspath(sys.argv[2])
    tmp_dir = os.path.abspath(sys.argv[3])
    build_root = os.path.dirname(tmp_dir)
    lock_path = os.path.join(build_root, "visual.lock")
    evidence_dir = os.path.join(build_root, "visual_evidence")
    os.makedirs(tmp_dir, exist_ok=True)
    if not acquire_lock(lock_path):
        print("visual: another render run holds {}, retry after it exits".format(lock_path))
        sys.exit(EXIT_BUSY)
    try:
        run_static(binary, golden_dir, tmp_dir)
        run_animated(binary, golden_dir, tmp_dir, evidence_dir)
    finally:
        try:
            os.remove(lock_path)
        except OSError:
            pass
    print("visual: all scenes pass")


if __name__ == "__main__":
    main()
