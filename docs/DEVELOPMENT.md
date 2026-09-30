# Development

## Layout

Every named application function has a separate source file. To change a function, open the file with the same name.

```text
include/nginxwatch.hpp   Shared types and function declarations
src/main.cpp            Program entry point
src/cli/                CLI help and run modes
src/config/             Read and validate settings
src/nginx/              Tokenize nginx, parse listeners and discover sites
src/http/               URLs, local/public requests and health results
src/email/              SMTP upload and message formatting
src/monitor/            Failure counts, transitions, notifications and shutdown
src/helpers/            Small string, time, file and logging functions
tests/unit.cpp          Parser, settings, state and mail-format tests
tests/integration.py    Real executable with local HTTP and encrypted SMTP fixtures
deploy/                 systemd service
examples/               Sample nginx configuration
scripts/                Optional local dependency setup without sudo
```

`siteLess.cpp` implements the `Site` ordering operator. `requestStop.cpp` owns the shutdown flag and signal handler. Other `.cpp` files contain one named function each. The shared header declares the interfaces; function bodies live in their own files.

## Build and test

Requires Linux, C++17, make, libcurl **7.85.0 or newer** development headers/library, Python 3 and OpenSSL CLI. Tests need no third-party Python packages or external websites. Their HTTP/SMTP fixtures bind only to localhost on temporary ports. The SMTP fixture creates a temporary test certificate.

```bash
make -j2
make test
```

On Debian/Ubuntu without sudo access, `make local-deps` prepares matching libcurl headers in `.deps/curl` using the already installed runtime library. See [editor and dependency troubleshooting](TROUBLESHOOTING.md#editor-says-to-update-includepath). Local dependencies survive `make clean` and are ignored by Git. The setup script also accepts a previously downloaded matching `.deb` as its only argument for offline use.

The Makefile discovers source files automatically, compiles one object per source, and tracks header dependencies. Unit tests link the application functions without `main.cpp`. Tests report failures with a nonzero exit code. When nginx is installed, an additional end-to-end test starts a temporary nginx instance and verifies both HTTP and HTTPS listeners; otherwise that one test is skipped.

For a debug build, clean first because make does not track changed compiler flags:

```bash
make clean
make -j2 CXXFLAGS='-O0 -g -std=c++17 -Wall -Wextra -Wpedantic'
make test CXXFLAGS='-O0 -g -std=c++17 -Wall -Wextra -Wpedantic'
```

## Making a change

1. Update the relevant function's `.cpp` file.
2. Update its declaration in `include/nginxwatch.hpp` if its interface changes.
3. Add or adjust a meaningful behavioral test, then run `make test`.
4. Update the example configuration and guides if user behavior changes.

## Git hygiene

`.gitignore` excludes common AI agent instructions, settings and folders, including `.agents/`, `.codex/`, `AGENTS.md`, Claude, Cursor, Copilot and Gemini artifacts. Keep new agent-specific artifacts in an ignored folder or add a pattern for the tool you use. Do not exclude normal application files just because an AI helped create them.

Ignore rules prevent new files from being added normally; they do not remove files already tracked in Git. Before committing, use `git status --short` to check which project files will be included.
