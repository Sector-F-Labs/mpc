# SETUP

MPC is intentionally minimal. Most users only need:
```
./configure
```
That single command:
- Detects your platform
- Installs required libraries (libcurl, jansson) if possible
- Installs / updates Ollama (if local backend desired)
- Builds `./bin/mpc`
- Pulls a default model (`gemma4`)
- Runs a quick generation sanity test

If that worked, you’re done. This file covers optional / manual details.

---

## 1. Dependencies (Manual Path)

Core build-time:
- C compiler (clang or gcc)
- libcurl (development headers)
- jansson (development headers)
- pkg-config (recommended)

LLM backend:
- Ollama with a code-capable model (e.g. `gemma4`, `deepseek-coder`)
- It can run locally or on another host (point at it with `-u URL`)

---

## 2. Manual Installation (If You Don’t Want ./configure)

Example platform commands (adapt to your distro):

macOS (Homebrew):
```
brew install curl jansson ollama
make
```

Debian / Ubuntu:
```
sudo apt-get update
sudo apt-get install -y build-essential pkg-config libcurl4-openssl-dev libjansson-dev
curl -fsSL https://ollama.ai/install.sh | sh   # optional
make
```

Fedora / RHEL:
```
sudo dnf install -y gcc make pkg-config libcurl-devel jansson-devel
curl -fsSL https://ollama.ai/install.sh | sh   # optional
make
```

Alpine:
```
sudo apk add build-base curl-dev jansson-dev pkgconfig
# (Optional) Ollama: see upstream instructions (Alpine variants may need extra glibc layer)
make
```

If `pkg-config` is missing, the Makefile’s detection may fail—install it.

---

## 3. Local Model Setup (Ollama)

```
ollama serve            # start daemon
ollama pull gemma4
./bin/mpc examples/simple.1
```

Try alternatives:
```
ollama pull deepseek-coder
mpc -m deepseek-coder examples/simple.1
```

---

## 4. Environment Variables

| Variable          | Purpose                              | When Needed              |
|-------------------|--------------------------------------|--------------------------|
| MPC_MODEL         | Default model override               | Optional convenience     |
| MPC_OLLAMA_URL    | Non-default Ollama base URL          | Custom deployments       |

All have CLI overrides; environment is optional.

---

## 5. Directory Layout (Key Paths)

| Path            | Description                         |
|-----------------|-------------------------------------|
| `bin/mpc`       | The compiler binary                 |
| `examples/`     | Sample man pages                    |
| `build/`        | Temp artifacts (debug / retries)    |
| `configure`     | Auto setup script                   |
| `TROUBLESHOOTING.md` | Common issue reference         |

`build/` appears only with `-d` or during retry flows.

---

## 6. Verifying a Clean Install

```
./configure
./bin/mpc examples/simple.1
./simple --help
```

Or manually:
```
make clean && make
./bin/mpc -v examples/hello.1
```

---

## 7. Common Manual Checks

Check libs:
```
pkg-config --exists libcurl jansson && echo OK
```

Check Ollama service:
```
curl -s http://localhost:11434/api/tags | jq .
```

List models:
```
ollama list
```

---

## 8. Debug & Retry

Retry with error feedback:
```
mpc -r 5 -v examples/count.1
```
Keep intermediate artifacts:
```
mpc -d -v examples/simple.1
ls build/
```

---

## 9. Minimal / Air‑Gapped Mode

If you cannot install Ollama locally:
- Point at an Ollama instance on another machine with `-u http://host:11434`.
- If fully offline: you must have Ollama + model already cached; otherwise generation cannot proceed.

---

## 10. Uninstall / Clean

Remove build outputs:
```
make clean
```
Remove generated example binaries but keep compiler:
```
make clean-examples
```
Remove `mpc` if installed system-wide:
```
sudo rm -f /usr/local/bin/mpc
```

---

## 11. Failure Modes Summary

| Symptom                            | Fix Hint                                |
|-----------------------------------|-----------------------------------------|
| “libcurl not found”               | Install dev package / rerun configure   |
| “jansson not found”               | Same as above                           |
| Hangs contacting Ollama           | Ensure `ollama serve` running           |
| Generated code won’t compile      | Increase `-r`, add `-d`, refine man page|
| Poor behavior / logic off         | Add explicit EXAMPLES + exit codes      |

See `TROUBLESHOOTING.md` for more.

---

## 12. Writing Better Man Pages (Impact on Output)

Good:
```
.SH NAME
count - count lines from files or stdin

.SH SYNOPSIS
.B count
.RI [ -v ]
.RI [ file... ]
```
Include:
- Deterministic examples (with output)
- Exit codes (especially non-zero cases)
- Option semantics (not vague prose)

---

## 13. Philosophy Recap (Why This File Exists)

README stays lean. This file absorbs platform noise, optional procedures, and deeper operational notes so the core message (docs → code) remains uncluttered.

---

## 14. If All Else Fails

Quick triage script:
```
which cc || which gcc || which clang
pkg-config --modversion libcurl
pkg-config --modversion jansson
./bin/mpc --help
```
Send that plus your man page (if shareable) when asking for help.

---

That’s it. If `./configure` succeeded you probably didn’t need most of this—mission accomplished.