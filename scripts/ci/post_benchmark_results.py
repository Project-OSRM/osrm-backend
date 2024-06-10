import requests
import os
import re
import sys
import json

GITHUB_TOKEN = os.getenv('GITHUB_TOKEN')
REPO = os.getenv('GITHUB_REPOSITORY')  
PR_NUMBER = os.getenv('PR_NUMBER')

REPO_OWNER, REPO_NAME = REPO.split('/')

def create_markdown_table(results):
    results = sorted(results, key=lambda x: x['name'])
    header = "| Benchmark | Base | PR |\n|-----------|------|----|"
    rows = []
    for result in results:
        name = result['name']
        base = result['base'] or ''
        base = base.replace('\n', '<br/>')
        pr = result['pr'] or ''
        pr = pr.replace('\n', '<br/>')
        row = f"| {name} | {base} | {pr} |"
        rows.append(row)
    return f"{header}\n" + "\n".join(rows)

def get_pr_details(repo_owner, repo_name, pr_number):
    url = f"https://api.github.com/repos/{repo_owner}/{repo_name}/pulls/{pr_number}"
    headers = {'Authorization': f'token {GITHUB_TOKEN}'}
    response = requests.get(url, headers=headers)
    response.raise_for_status()
    return response.json()

def update_pr_description(repo_owner, repo_name, pr_number, body):
    url = f"https://api.github.com/repos/{repo_owner}/{repo_name}/pulls/{pr_number}"
    headers = {'Authorization': f'token {GITHUB_TOKEN}'}
    data = {'body': body}
    response = requests.patch(url, headers=headers, json=data)
    response.raise_for_status()
    return response.json()


def collect_benchmark_results(base_folder, pr_folder):
    results = []
    results_index = {}

    for file in os.listdir(base_folder):
        if not file.endswith('.bench'): continue
        with open(f"{base_folder}/{file}") as f:
            result = f.read().strip()
            results.append({'base': result, 'pr': None, 'name': os.path.splitext(file)[0]})
            results_index[file] = len(results) - 1

    for file in os.listdir(pr_folder):
        if not file.endswith('.bench'): continue
        with open(f"{pr_folder}/{file}") as f:
            result = f.read().strip()
            if file in results_index:
                results[results_index[file]]['pr'] = result
            else:
                results.append({'base': None, 'pr': result, 'name': os.path.splitext(file)[0]})

    return results

def main():
    if len(sys.argv) != 3:
        print("Usage: python post_benchmark_results.py <base_folder> <pr_folder>")
        exit(1)

    base_folder = sys.argv[1]
    pr_folder = sys.argv[2]

    benchmark_results = collect_benchmark_results(base_folder, pr_folder)

    pr_details = get_pr_details(REPO_OWNER, REPO_NAME, PR_NUMBER)
    # in both cases when there is no PR body or PR body is None fallback to empty string
    pr_body = pr_details.get('body', '') or ''

    markdown_table = create_markdown_table(benchmark_results)
    new_benchmark_section = f"""
<!-- BENCHMARK_RESULTS_START -->
<details><summary><h2>Benchmark Results</h2></summary>

{markdown_table}
</details>
<!-- BENCHMARK_RESULTS_END -->
"""

    if re.search(r'<!-- BENCHMARK_RESULTS_START -->.*<!-- BENCHMARK_RESULTS_END -->', pr_body, re.DOTALL):
        updated_body = re.sub(
            r'<!-- BENCHMARK_RESULTS_START -->.*<!-- BENCHMARK_RESULTS_END -->',
            new_benchmark_section,
            pr_body,
            flags=re.DOTALL
        )
    else:
        updated_body = f"{pr_body}\n\n{new_benchmark_section}" if len(pr_body) > 0 else new_benchmark_section
    
    update_pr_description(REPO_OWNER, REPO_NAME, PR_NUMBER, updated_body)
    print("PR description updated successfully.")

        

if __name__ == "__main__":
    main()
