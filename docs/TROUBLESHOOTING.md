# Troubleshooting

Start with `./build/nginxwatch --once` and read the error. Exit `1` means a site failed; exit `2` means the monitor could not perform the requested operation.

## Editor says to update includePath

Open the project root in VS Code, rather than just the `src` folder. In the C/C++ extension configuration, use `/usr/bin/g++`, C++17 and `${workspaceFolder}/include`. The compiler supplies the standard C++ header paths automatically.

If the missing file is `curl/curl.h`, the runtime curl command is not enough: install `libcurl4-openssl-dev` as shown in the README. If you cannot use sudo on Debian/Ubuntu, use the optional local dependency setup:

```bash
make local-deps
make -j2
```

This downloads a development package matching the installed libcurl runtime and extracts it into the ignored `.deps/curl` folder, without modifying system packages. It requires `apt`, `dpkg`, the libcurl runtime, and network access. If the exact package version is no longer available, install/update the system development package instead.

For this local setup on Linux x64, add `${workspaceFolder}/.deps/curl/usr/include/x86_64-linux-gnu` to the editor's include paths. On other architectures, use the directory inside `.deps/curl/usr/include` that contains `curl/`. `make` detects this folder automatically. `make clean` preserves local dependencies.

After updating the configuration, run **C/C++: Reset IntelliSense Database** or **Developer: Reload Window** if the warning remains. Use `make` to build the complete project; compiling a single function file is not sufficient.

## Runtime and build errors

| Problem | What to check |
| --- | --- |
| `curl/curl.h` missing | Install the libcurl development package, e.g. `libcurl4-openssl-dev` on Ubuntu/Debian |
| curl link errors | Install development libraries and use the supplied Makefile; curl must appear after object files when linking |
| Settings file cannot open | Copy the example, or supply `-c /absolute/path/to/nginxwatch.conf` |
| Invalid setting | Check spelling, use whole-line comments and avoid quotes around values |
| No testable sites | Check `nginx_dir`, concrete domain names, ignore list and supported listeners |
| Permission denied / broken symlink | Check readability of files, directories and symlink targets as the service user |
| Local fails, public succeeds | Check `local_address`, nginx bind address, port, TLS certificate chain and whether public DNS serves a different machine |
| HTTP 502/503/504 | Inspect nginx error logs and the application's upstream service |
| Public DNS/connect failure | Check DNS, routing, firewall and proxy environment settings |
| Certificate failure | Fix expiry, trust chain or the certificate's domain name; TLS validation stays enabled |
| SMTP authentication failed | Check provider settings and app password; `SMTP_PASS` overrides the file password |
| SMTP connect timeout | Check outbound SMTP restrictions, hostname, port and TLS mode |
| Accepted email absent | Check spam and SMTP provider delivery logs |
| systemd exits repeatedly | Read `journalctl -u nginxwatch`; check configuration, read access and writable log path |

For a direct local HTTPS check:

```bash
curl --noproxy '*' --resolve example.com:443:127.0.0.1 https://example.com/
```

## Supported nginx configuration

The parser handles comments, quoted strings, escaped characters, nested locations, multiple server names, HTTP/HTTPS listeners, address-and-port listeners, IPv6 listen addresses, symlinks and duplicate endpoints. SSL is determined by the `ssl` listen option, rather than guessing from port 443. Duplicate domain/port/scheme endpoints are checked once. A server block without `listen` defaults to HTTP port 80.

Limits:

- Only files directly inside `nginx_dir` are scanned. nginx `include` directives and subdirectories are not followed. Use complete site definitions in this folder. Stream/mail blocks, Unix sockets and QUIC-only listeners are skipped.
- Wildcards, regular-expression names, variables, empty names, `_`, and `localhost` are skipped. Use a literal DNS name or IPv4 address. IPv6 literals in `server_name` and internationalized Unicode names are not supported; use a DNS name/punycode.
- The local connection uses `local_address`, not the address in each `listen` directive. Configure a reachable address if nginx listens only on a specific interface. A single address applies to all sites.
- Parsing finds endpoints; it does not validate nginx itself or evaluate nginx variables, templating, includes, or every nginx directive. Use `nginx -t` separately.
- Each endpoint gets one request at the configured path. Redirect targets, page contents and upstream dependencies are not inspected. A 404 counts as healthy.
- Checks are sequential. Failure thresholds count complete rounds, so many slow sites increase alert delay.
- State is in memory. Config discovery errors are logged and retried, without a separate email alert. A full monitor/server outage cannot produce an alert from this process.
- Tests use local fixtures. Real systemd deployment and production provider delivery require checks on your server.
