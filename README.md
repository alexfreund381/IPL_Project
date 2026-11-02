# IPL course repository

My Route A work: the Mini compiler

- `MiniLang/weekNN-start/`: the starter project with my implementation; my explanations are in `wNN/`

- `MiniLang/weekNN-solution/`: the posted solution, unchanged

- `MiniLang/comparisons/`: **report**, with one file per week (progress note and comparison with the solution)
- `mini8/`: processor notebooks

**Setup:** MSYS2 UCRT64 g++ on `PATH`, CMake, and Python 3.8+ for the tests. `.gitattributes` keeps the `.mini` inputs at LF line endings. Without it, Git on Windows converts them to CRLF and `CHECK_PROJECT.py` rejects them as "changed fixture".

**Build and run** from a week folder:
```
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug
cmake --build build
.\build\minic.exe run inputs\acceptance.mini
```

**Tests**, in each week folder:
- Supplied: `inputs/*.mini` (the acceptance program and edge cases) with expected results in `checks/CASES.json`. Run them all with
`python CHECK_PROJECT.py --mode solution --compiler .\build\minic.exe`

- Mine: `wNN/*.mini`. The comments in each file give the command to run (`tokens`, `check` or `run`) and the output I predicted.

**Submissions:** each week's submission commit is tagged `weekNN-submission` (e.g. `week03-submission`). List them with `git tag`, view one with `git checkout week03-submission`, or find them under **Tags** on GitHub.


