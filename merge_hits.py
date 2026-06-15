#!/usr/bin/env python3
"""合并 Geant4 MT 多线程产生的 CSV 分片。

输入: hits{runID}_nt_hits_t{thread}.csv  (每个 worker 一个)
输出: hits{runID}.csv                     (合并后, 含单一 header)

用法:
    python3 merge_hits.py                 # 默认合并当前目录
    python3 merge_hits.py build/          # 指定目录
    python3 merge_hits.py build/ -o out/  # 指定输出目录
    python3 merge_hits.py build/ -r 0     # 只合并 runID=0
"""

from __future__ import annotations

import argparse
import csv
import re
import sys
from collections import defaultdict
from pathlib import Path

# hits0_nt_hits_t7.csv -> runID=0
PATTERN = re.compile(r"^hits(\d+)_nt_hits_t\d+\.csv$")


def group_shards(directory: Path, run_filter: int | None) -> dict[int, list[Path]]:
    """按 runID 分组所有线程分片文件。"""
    groups: dict[int, list[Path]] = defaultdict(list)
    for path in sorted(directory.glob("hits*_nt_hits_t*.csv")):
        m = PATTERN.match(path.name)
        if not m:
            continue
        run_id = int(m.group(1))
        if run_filter is not None and run_id != run_filter:
            continue
        groups[run_id].append(path)
    return groups


def read_header(path: Path) -> list[str]:
    """读取 tools::wcsv 的 # 注释 header 行。"""
    with path.open(newline="") as f:
        return [line.rstrip("\n") for line in f if line.startswith("#")]


def merge_group(run_id: int, shards: list[Path], out_dir: Path) -> tuple[Path, int]:
    """合并一个 runID 的所有分片, 返回 (输出路径, 数据行数)。"""
    out_path = out_dir / f"hits{run_id}.csv"
    if not shards:
        return out_path, 0

    header = read_header(shards[0])
    total_rows = 0

    with out_path.open("w", newline="") as fout:
        writer = csv.writer(fout)
        for line in header:
            fout.write(line + "\n")
        for shard in shards:
            with shard.open(newline="") as fin:
                reader = csv.reader(fin)
                for row in reader:
                    if not row or row[0].startswith("#"):
                        continue
                    if len(row) == 1 and not row[0].strip():
                        continue
                    writer.writerow(row)
                    total_rows += 1

    return out_path, total_rows


def main() -> int:
    parser = argparse.ArgumentParser(description="合并 Geant4 MT CSV 分片")
    parser.add_argument("directory", nargs="?", default=".", help="含 hits*_t*.csv 的目录 (默认当前目录)")
    parser.add_argument("-o", "--output", default=None, help="输出目录 (默认与输入相同)")
    parser.add_argument("-r", "--run", type=int, default=None, help="只合并指定 runID")
    args = parser.parse_args()

    in_dir = Path(args.directory).resolve()
    out_dir = Path(args.output).resolve() if args.output else in_dir

    if not in_dir.is_dir():
        print(f"错误: 目录不存在: {in_dir}", file=sys.stderr)
        return 1

    groups = group_shards(in_dir, args.run)
    if not groups:
        print(f"未找到匹配的分片文件 (hits*_nt_hits_t*.csv): {in_dir}", file=sys.stderr)
        return 1

    if out_dir != in_dir:
        out_dir.mkdir(parents=True, exist_ok=True)

    print(f"{'runID':>6}  {'分片数':>6}  {'命中数':>8}  输出")
    print("-" * 60)
    grand_total = 0
    for run_id in sorted(groups):
        shards = groups[run_id]
        out_path, rows = merge_group(run_id, shards, out_dir)
        grand_total += rows
        print(f"{run_id:>6}  {len(shards):>6}  {rows:>8}  {out_path}")

    print("-" * 60)
    print(f"合计命中: {grand_total}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
