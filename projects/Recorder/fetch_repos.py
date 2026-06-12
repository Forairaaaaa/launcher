import subprocess
import json
from pathlib import Path


def clone_or_update_repo(
    repo_url, path, ref=None, with_submodules=False, patch_path=None, update_existing=True
):
    path = str(path)
    if not Path(path).exists():
        subprocess.run(["git", "clone", repo_url, path], check=True)
    elif update_existing:
        subprocess.run(["git", "-C", path, "fetch"], check=True)

    if ref:
        subprocess.run(["git", "-C", path, "checkout", ref], check=True)

    if with_submodules:
        subprocess.run(
            ["git", "-C", path, "submodule", "update", "--init", "--recursive"],
            check=True,
        )

    # 应用 patch
    if patch_path:
        patch_full_path = (
            patch_path
            if os.path.isabs(patch_path)
            else str(Path.cwd() / patch_path)
        )
        # 使用 git apply --check 先检测补丁是否能应用，避免报错
        check_result = subprocess.run(
            ["git", "-C", path, "apply", "--check", patch_full_path]
        )
        if check_result.returncode == 0:
            subprocess.run(["git", "-C", path, "apply", patch_full_path], check=True)
            print(f"Applied patch {patch_path} to {path}")
        else:
            print(f"Patch {patch_path} cannot be applied cleanly to {path}, skipped.")


def load_repos():
    script_dir = Path(__file__).resolve().parent
    config_path = script_dir / "repos.json"

    with open(config_path) as f:
        return json.load(f)


def repo_path(repo):
    return Path(__file__).resolve().parent / repo["path"]


def is_repo_fetched(repo):
    path = repo_path(repo)
    return path.exists() and (path / ".git").exists()


def fetch_dependencies(repos=None, update_existing=True):
    repos = repos or load_repos()

    for repo in repos:
        path = repo_path(repo)
        branch = repo.get("branch")
        with_submodules = repo.get("with_submodules", False)
        patch = repo.get("patch")
        if patch:
            patch = Path(patch)
            if not patch.is_absolute():
                patch = Path(__file__).resolve().parent / patch

        clone_or_update_repo(
            repo["url"],
            path,
            branch,
            with_submodules,
            patch,
            update_existing=update_existing,
        )


def ensure_dependencies():
    repos = load_repos()
    missing = [repo for repo in repos if not is_repo_fetched(repo)]

    if not missing:
        return

    names = ", ".join(repo["path"] for repo in missing)
    print(f"Recorder: fetching missing dependencies: {names}")
    fetch_dependencies(missing, update_existing=False)


if __name__ == "__main__":
    fetch_dependencies()
