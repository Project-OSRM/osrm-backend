import os
import sys


def create_markdown_table(results):
    results = sorted(results, key=lambda x: x["name"])
    rows = []
    rows.append("| Benchmark    | Result |")
    rows.append("| ------------ | ------ |")
    for result in results:
        name = result["name"]
        base = result["base"]
        base = base.replace("\n", "<br/>")
        row = f"| {name:12} | {base} |"
        rows.append(row)
    return "\n".join(rows)


def collect_benchmark_results(results_folder):
    results = []
    results_index = {}

    for file in os.listdir(results_folder):
        if not file.endswith(".bench"):
            continue
        with open(os.path.join(results_folder, file)) as f:
            result = f.read().strip()
            results.append({"base": result, "name": os.path.splitext(file)[0]})
            results_index[file] = len(results) - 1

    return results


def main():
    if len(sys.argv) != 2:
        print(f"Usage: python {sys.argv[0]} <results_folder>")
        sys.exit(1)

    results_folder = sys.argv[1]
    benchmark_results = collect_benchmark_results(results_folder)
    markdown_table = create_markdown_table(benchmark_results)

    if "GITHUB_STEP_SUMMARY" in os.environ:
        with open(os.environ["GITHUB_STEP_SUMMARY"], "a") as summary:
            summary.write("### Benchmark Results\n\n")
            summary.write(markdown_table)
    else:
        print(markdown_table)


if __name__ == "__main__":
    main()
