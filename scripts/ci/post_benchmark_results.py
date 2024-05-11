import requests
import os
import re
import sys

GITHUB_TOKEN = os.getenv('GITHUB_TOKEN')
REPO = os.getenv('GITHUB_REPOSITORY')  
PR_NUMBER = os.getenv('PR_NUMBER')

REPO_OWNER, REPO_NAME = REPO.split('/')

def create_markdown_table(results):
    header = "| Benchmark | Base | PR |\n|--------|----|"
    rows = [f"| {result['name']} | {result['base']} | {result['pr']} |" for result in results]
    return f"{header}\n" + "\n".join(rows)

def get_pr_comments(repo_owner, repo_name, pr_number):
    url = f"https://api.github.com/repos/{repo_owner}/{repo_name}/issues/{pr_number}/comments"
    headers = {'Authorization': f'token {GITHUB_TOKEN}'}
    response = requests.get(url, headers=headers)
    response.raise_for_status()
    return response.json()

def update_comment(comment_id, repo_owner, repo_name, body):
    url = f"https://api.github.com/repos/{repo_owner}/{repo_name}/issues/comments/{comment_id}"
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

def main():
    if len(sys.argv) != 3:
        print("Usage: python post_benchmark_results.py <base_folder> <pr_folder>")
        exit(1)

    base_folder = sys.argv[1]
    pr_folder = sys.argv[2]

    benchmark_results = collect_benchmark_results(base_folder, pr_folder)

    comments = get_pr_comments(REPO_OWNER, REPO_NAME, PR_NUMBER)
    if not comments or len(comments) > 0:
        print("No comments found on this PR.")
        exit(1)
    
    first_comment = comments[0]
    markdown_table = create_markdown_table(benchmark_results)
    new_benchmark_section = f"<!-- BENCHMARK_RESULTS_START -->\n## Benchmark Results\n{markdown_table}\n<!-- BENCHMARK_RESULTS_END -->"

    if re.search(r'<!-- BENCHMARK_RESULTS_START -->.*<!-- BENCHMARK_RESULTS_END -->', first_comment['body'], re.DOTALL):
        updated_body = re.sub(
            r'<!-- BENCHMARK_RESULTS_START -->.*<!-- BENCHMARK_RESULTS_END -->',
            new_benchmark_section,
            first_comment['body'],
            flags=re.DOTALL
        )
    else:
        updated_body = f"{first_comment['body']}\n\n{new_benchmark_section}"
    
    update_comment(first_comment['id'], REPO_OWNER, REPO_NAME, updated_body)
    print("PR comment updated successfully.")

        

if __name__ == "__main__":
    main()
