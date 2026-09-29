# Configuration

Copy `nginxwatch.conf.example` to `nginxwatch.conf`, then edit it. Use `key = value`, one setting per line. Blank lines and lines beginning with `#` are ignored. Values are literal: do not add quotation marks, inline comments, shell variables or `~` expansions. Unknown keys, duplicate keys and invalid numbers cause an error.

The settings file must exist. Paths are relative to the process working directory; absolute paths are easiest for deployment. Settings load at startup, so restart after editing them. nginx files are rescanned automatically.

| Setting | Default | Meaning |
| --- | --- | --- |
| `nginx_dir` | `/etc/nginx/sites-enabled` | Folder containing complete nginx site definitions |
| `interval_sec` | `30` | Wait after a complete round; 1–86400 seconds |
| `fails_before_down` | `3` | Consecutive unhealthy rounds before DOWN; 1–86400 |
| `timeout_sec` | `10` | Maximum duration of each individual HTTP request; 1–86400 seconds |
| `check_local` | `1` | Check via `local_address`; 0 or 1 |
| `check_public` | `1` | Check via normal DNS; 0 or 1 |
| `local_address` | `127.0.0.1` | Local IPv4 or IPv6 address, without square brackets |
| `health_path` | `/` | Request path for all sites, e.g. `/health` |
| `http_ca_file` | system trust | Optional CA certificate bundle for HTTPS checks against private certificates |
| `ignore` | empty | Comma-separated exact domains to skip, case insensitive |
| `log_file` | `nginxwatch.log` | Append-only event log, also printed to stdout |
| `smtp_url` | empty | SMTP endpoint; empty disables email unless `mail_to` is set |
| `smtp_user` | empty | SMTP authentication username; omit for a trusted relay without authentication |
| `smtp_pass` | empty | SMTP password; prefer environment variable `SMTP_PASS` |
| `mail_from` | `smtp_user` | Sender address, without a display name or angle brackets |
| `mail_to` | empty | One recipient address, without a display name or angle brackets |
| `smtp_starttls` | `0` | Must be 1 with a `smtp://` endpoint |
| `smtp_ca_file` | system trust | Optional CA certificate bundle for a private SMTP server |

At least one of `check_local` or `check_public` must be enabled. When both are enabled, both must succeed for the site to be healthy. Checks are sequential: a round can take up to approximately `site count × enabled paths × timeout_sec`, plus email time. `interval_sec` is a wait between rounds, not a fixed start-to-start schedule.

## Email example

Add the settings supplied by your mail provider:

```ini
smtp_url = smtps://smtp.example.com:465
smtp_user = monitor@example.com
mail_from = monitor@example.com
mail_to = you@example.com
```

Alternatively, use STARTTLS:

```ini
smtp_url = smtp://smtp.example.com:587
smtp_starttls = 1
```

Both modes require encrypted SMTP and validate the server certificate. Plaintext SMTP is rejected. Set your password in the shell before running:

```bash
read -rsp 'SMTP password: ' SMTP_PASS; echo
export SMTP_PASS
./build/nginxwatch --test-email
```

`SMTP_PASS` overrides `smtp_pass`, including when the environment variable is empty. Use a provider-issued app password when your provider requires one. Protect any configuration containing credentials with `chmod 600 nginxwatch.conf`.

Successful SMTP submission means the SMTP server accepted the message, not that it reached the inbox. Check spam and provider delivery logs if necessary.

## Alert behavior

Continuous mode sends one notification per transition after successful SMTP submission. Failed submissions are logged and retried on the next round. A newer transition replaces an older undelivered notification. SMTP ambiguity can still cause duplicate deliveries; there is no exactly-once delivery guarantee.

State and pending notifications live in memory. Restarting resets failure counts and outage history. Removed sites are pruned on the next successful discovery round. If discovery fails, existing state is retained, an error is logged, and the monitor retries next round. Discovery errors themselves are not emailed.
