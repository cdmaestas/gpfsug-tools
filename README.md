
GPFS/Spectrum Scale Usergroup Tools Git Repo
--------------------------------------------

 http://www.gpfsug.org
 http://www.spectrumscale.org

 These files are provided as-is and a contributed by members of the user group.

Development
----------

Linting, formatting, and a security scan run automatically on pull requests via
GitHub Actions (`.github/workflows/ci.yml`). To run the same checks locally
before committing, install [pre-commit](https://pre-commit.com/) and enable the
hooks:

```
pip install pre-commit
pre-commit install
pre-commit run --all-files
```

The hooks cover ShellCheck + shfmt for the Bash/sh scripts and Ruff (lint +
format) for Python. The vendored `bin/rsync/rsync-3.0.9-patched/` tree is
third-party and is excluded from all of the above.

