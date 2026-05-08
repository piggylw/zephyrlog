# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build

```bash
cmake -B build
cmake --build build
```

`compile_commands.json` is symlinked at the repo root → `build/compile_commands.json` for clangd.

## Run examples

```bash
./build/test1
./build/test2
```

There are no unit tests — the two examples under `examples/` serve as manual verification programs.

## Architecture

ZephyrLog is a C++17 header-only-style logging library (with a small compiled static lib) using a "encode first, format later" design.

**Core flow (synchronous, current working path):**

```
ZINFO << "msg" << 42;
  → macro expands to level-check + LOG(LEVEL)
  → LOG creates a temporary LogLine, streams args into it via operator<<
  → ZephyrLog::operator== receives the LogLine, calls stringify(std::cout)
```

**LogLine** (`include/zephyrlog/logline.h`, `src/logline.cpp`):
- Encodes metadata (timestamp, thread id, file/function/line, level) + user args into a contiguous binary buffer as `[ArgType | value]` pairs.
- Small Buffer Optimization: 224-byte stack buffer; heap-allocates (starting at 512 bytes, 2x growth) only when exceeded.
- Move-only; non-copyable.
- `stringify(std::ostream&)` walks the binary buffer and formats output: `[timestamp][LEVEL][thread-id][file:function:line] arg1 arg2...`

**Logging macros** (`include/zephyrlog/zephyrlog.h`):
- `ZDEBUG`, `ZINFO`, `ZWARN`, `ZERROR`, `ZCRIT` — each checks `getLogLevel()` before constructing a `LogLine`, so filtered-out levels avoid the encoding cost entirely.
- `setLogLevel()` / `getLogLevel()` use a global `std::atomic<LogLevel>`.

**Operator<< overloads**: char, int32_t, uint32_t, int64_t, uint64_t, double, std::string, and a template overload handling `const char*`, `char*`, and string literals (`const char[N]`).

**Async path (in-progress, not wired up):**

```
User threads → push(LogLine) → RingBuffer (MPSC, fixed-size, lossy)
                                    ↓
Background thread ← tryPop() ← RingBuffer
        ↓
FileWriter::write() → stdout
```

- **RingBuffer** (`include/zephyrlog/ringbuffer.h`, `src/ringbuffer.cpp`): Multi-producer, single-consumer ring buffer. Each 256-byte-aligned slot holds a `LogLine`. Producers acquire a write index via atomic fetch_add, then SpinLock the slot. Consumer SpinLocks each slot and checks a `written` flag. Full buffer silently overwrites old entries.
- **SpinLock** (`include/zephyrlog/spinlock.h`): RAII spinlock over `std::atomic_flag` with acquire/release semantics.
- **BufferBase** (`include/zephyrlog/bufferbase.h`): Abstract interface with `push(LogLine&&)` and `tryPop(LogLine&)`.
- **AsyncLogger** (`include/zephyrlog/asynclogger.h`): Placeholder class, not yet implemented.
- **FileWriter** (`include/zephyrlog/filewriter.h`): Minimal wrapper that calls `logline.stringify(std::cout)`.

## Key details

- Everything lives in the `zephyrlog` namespace.
- Log levels: `DEBUG(0) < INFO(1) < WARN(2) < ERROR(3) < CRITICAL(4)`.
- CRITICAL messages trigger `os.flush()` after write.
- Timestamps are UTC, formatted as `[YYYY-MM-DD HH:MM:SS.microseconds]`.
- The `string_literal_t` struct stores a raw `const char*` — safe for `__FILE__`, `__func__`, and string literals but not for transient C strings (those get copied via `encodeCString`).
