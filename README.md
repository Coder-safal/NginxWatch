# nginxwatch

A C++17 command-line monitor for websites configured in nginx. It discovers literal `server_name` entries, checks each HTTP/HTTPS listener, and logs or emails DOWN and RECOVERED events.

## Start here

On Ubuntu/Debian, install the build dependencies:

```bash
sudo apt update
sudo apt install g++ make libcurl4-openssl-dev python3 openssl
```

From this project folder:

```bash
make -j2
cp nginxwatch.conf.example nginxwatch.conf
```

Without sudo access on Debian/Ubuntu, you can run `make local-deps` before building to prepare local libcurl headers using the installed runtime. See [include-path and dependency help](docs/TROUBLESHOOTING.md#editor-says-to-update-includepath).

Edit `nginxwatch.conf`. Set `nginx_dir` to your enabled nginx configuration folder, usually `/etc/nginx/sites-enabled` or `/etc/nginx/conf.d`. Then run:

```bash
./build/nginxwatch --once    # check sites once, without sending email
./build/nginxwatch          # keep checking and log state changes
```

For email, fill in the commented SMTP settings in your configuration, then:

```bash
read -rsp 'SMTP password: ' SMTP_PASS; echo
export SMTP_PASS
./build/nginxwatch --test-email
./build/nginxwatch
```

No SMTP settings are needed for checks and logging. Your real configuration, credentials, logs, build outputs, and common AI agent files are ignored by Git.

## How it works

1. Read the files directly inside `nginx_dir`, following enabled-site symlinks.
2. Find domains and each HTTP/HTTPS listener in their `server` blocks.
3. Check `/` locally using the real Host header and TLS name, and through public DNS.
4. After three consecutive failed rounds, emit a DOWN event. Emit RECOVERED after the first healthy round.
5. Rescan the nginx folder each round to discover changes automatically.

HTTP statuses **100–499** count as an answer, including redirects, 401 and 404. HTTP 5xx, connection errors, invalid TLS certificates and timeouts count as failures. Redirects are not followed. This checks availability, not whether a page contains the expected content.

| Local | Public | What to investigate |
| --- | --- | --- |
| OK | OK | Both paths answered |
| OK | Failed | DNS, routing, firewall, proxy/CDN or TLS |
| Failed | Failed | nginx, application, local binding or TLS |
| Failed | OK | Local address/binding, certificate or a public response served elsewhere |

Public requests use normal libcurl proxy environment settings. Local requests bypass proxies. Public checks from your own server are useful, but are not an independent external availability check.

## Commands

```bash
./build/nginxwatch --help
./build/nginxwatch -c /path/to/settings.conf --once
./build/nginxwatch -c /path/to/settings.conf --test-email
./build/nginxwatch -c /path/to/settings.conf
make test
make clean
```

Exit codes: `0` = checks healthy, test email accepted, or clean shutdown; `1` = unhealthy sites or failed test email; `2` = invalid arguments/settings, startup/discovery failure, or another fatal error. `--once` never sends alerts. Stop continuous monitoring with Ctrl+C or SIGTERM.

## Guides

- [Configuration and email](docs/CONFIGURATION.md)
- [Install as a systemd service](docs/DEPLOYMENT.md)
- [Troubleshooting and limits](docs/TROUBLESHOOTING.md)
- [Code layout and development](docs/DEVELOPMENT.md)

Each function has its own `.cpp` file under `src/`, grouped by purpose. Use this README and the guides above for the implemented commands and behavior.

## Before relying on alerts

Run `make test`, check your actual nginx configuration with `--once`, send a test email, and verify a controlled outage and recovery. The automated tests use local HTTP and TLS SMTP fixtures; they do not verify your production nginx, DNS, firewall, or mail provider.

A monitor on the same server cannot report that server losing power or network access. For a second machine, copy the relevant nginx files there and set `check_local = 0`, `check_public = 1`.
