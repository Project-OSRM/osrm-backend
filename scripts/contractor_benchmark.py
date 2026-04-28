import argparse
import datetime
import glob
import os
import pathlib
import re
import subprocess
import shutil
import tempfile

import pandas as pd
import tabulate


def run(args):
    for logfile in args.logfiles:
        path = pathlib.Path(logfile)
        if path.exists():
            file_time = datetime.datetime.fromtimestamp(path.stat().st_ctime)
            new_name = path.with_suffix(".log." + file_time.isoformat())
            path.rename(new_name)

    while len(args.logfiles) < len(args.binaries):
        args.logfiles.append(args.logfiles[-1])

    for i in range(0, args.runs):
        for binary, logfile in zip(args.binaries, args.logfiles):
            with open(logfile, "a") as log:

                def tee(text):
                    if text != ".\n":
                        print(text, end="")
                        log.write(text)
                        log.flush()

                with tempfile.TemporaryDirectory(
                    prefix="osrm-benchmark-", delete=not args.keep
                ) as tmp_dir:
                    for file in glob.glob(f"{args.dataset}.*"):
                        shutil.copy(file, tmp_dir)

                    tee(f"### {i} {binary} => {logfile}\n")
                    if args.keep:
                        tee(f"{tmp_dir}\n")
                    proc = subprocess.Popen(
                        [binary, os.path.join(tmp_dir, os.path.basename(args.dataset))],
                        stdout=subprocess.PIPE,
                        stderr=subprocess.STDOUT,
                        text=True,
                    )
                    while proc.poll() is None:
                        text = proc.stdout.readline()
                        tee(text)
    report(args)


def copy(args):
    with tempfile.TemporaryDirectory(prefix="osrm-benchmark-", delete=False) as tmp_dir:
        for file in glob.glob(f"{args.dataset}.*"):
            shutil.copy(file, tmp_dir)
        print(tmp_dir)


def report(args):
    rows = list()
    index = list()
    for logfile in args.logfiles:
        rows_read = 0
        with open(logfile) as log:
            for line in log:
                if m := re.search(r"^### (\d+) (.*?) => (.*)$", line):
                    if rows_read >= args.runs:
                        break
                    index.append((int(m.group(1)), m.group(3)))
                    rows.append({})
                    rows_read += 1
                if m := re.search(r"Contracted graph has (\d+) edges", line):
                    rows[-1]["edges"] = int(m.group(1))
                if m := re.search(r"Contraction took ([.\d]+) sec", line):
                    rows[-1]["time"] = float(m.group(1))
                if m := re.search(r"RAM: peak bytes used: (\d+)", line):
                    rows[-1]["mem"] = int(m.group(1))

    df = pd.DataFrame(
        rows, index=pd.MultiIndex.from_tuples(index, names=("run", "log"))
    )
    df.mem /= 1024 * 1024

    print(f"## RAW data - {datetime.datetime.now().isoformat()}\n```")
    print(df)
    print("```")

    def norm(series):
        return series / series.iloc[0]

    agg = df.groupby("log", sort=False).agg(["median"])
    agg.insert(3, "mem_norm", norm(agg["mem", "median"]), allow_duplicates=True)
    agg.insert(2, "time_norm", norm(agg["time", "median"]), allow_duplicates=True)
    agg.insert(1, "edges_norm", norm(agg["edges", "median"]), allow_duplicates=True)

    print("\n## Summary\n")
    headers = ("log", "edges", "norm", "time (s)", "norm", "mem (MB)", "norm")
    floatfmt = ("", ".0f", ".3f", ".2f", ".3f", ".0f", ".3f")
    print(
        tabulate.tabulate(
            agg, headers, tablefmt="github", floatfmt=floatfmt, showindex=True
        )
    )


def build_parser():
    parser = argparse.ArgumentParser(
        description="Benchmark and compare contractor binaries"
    )
    subparsers = parser.add_subparsers(help="subcommand help")

    parser_run = subparsers.add_parser("run", help="run the benchmark")
    parser_run.set_defaults(func=run)
    parser_run.add_argument(
        "--runs",
        help="How many times to run the comparison",
        type=int,
        default=10,
        metavar="NUM",
    )
    parser_run.add_argument(
        "--keep",
        help="Keep the generated datasets",
        action="store_true",
    )
    parser_run.add_argument("--dataset", help="path/to/dataset.osrm", metavar="PATH")
    parser_run.add_argument(
        "--logfiles",
        nargs="+",
        help="where to write the test logs",
        metavar="PATH",
    )
    parser_run.add_argument(
        "--binaries",
        nargs="+",
        help="The contractor binaries to compare",
        metavar="CONTRACTOR",
    )

    parser_report = subparsers.add_parser(
        "report", help="analyze the benchmark logfile"
    )
    parser_report.set_defaults(func=report)
    parser_report.add_argument(
        "--logfiles",
        nargs="+",
        help="the log file to process",
        metavar="PATH",
    )
    parser_report.add_argument(
        "--runs",
        help="How many runs to read from the logs",
        type=int,
        default=0,
        metavar="NUM",
    )

    parser_copy = subparsers.add_parser(
        "copy", help="make a copy of the source dataset"
    )
    parser_copy.set_defaults(func=copy)
    parser_copy.add_argument("--dataset", help="path/to/dataset.osrm", metavar="PATH")

    return parser


def main():
    args = build_parser().parse_args()
    args.func(args)


if __name__ == "__main__":
    main()
