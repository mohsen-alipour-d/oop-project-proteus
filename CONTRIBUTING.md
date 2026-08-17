# Contributing

Thanks for considering a contribution! This is a course project, but we follow
real-world workflow.

## Ground rules
- Read docs/ARCHITECTURE.md and docs/GIT_MERGE_GUIDE_FA.md first.
- Keep changes small and focused; one feature per branch.
- The UI never touches the domain directly; go through BackendAdapter.
- Memory ownership stays with Circuit; always new + addComponent.

## Workflow
    git switch main && git pull --ff-only
    git switch -c feature/short-name
    cmake -S . -B build && cmake --build build
    ./build/run_tests && ./build/run_integration_tests
    git commit -m "feat: ..." && git push -u origin feature/short-name

## Definition of done
- Both test suites green (155 checks).
- New files registered only in CMakeLists.txt.
- Docs updated if behavior changed.
