# Run continuously with systemd

Build and test from the project folder first:

```bash
make -j2
make test
```

The supplied service uses a dedicated `nginxwatch` user and stores its log in `/var/lib/nginxwatch`. These commands require a Linux system with systemd and administrator access.

## 1. Install the executable and settings

```bash
id nginxwatch >/dev/null 2>&1 || sudo useradd --system --no-create-home --shell /usr/sbin/nologin nginxwatch
sudo install -d -m 755 /opt/nginxwatch
sudo install -d -m 750 -o root -g nginxwatch /etc/nginxwatch
sudo install -m 755 build/nginxwatch /opt/nginxwatch/nginxwatch
sudo install -m 640 -o root -g nginxwatch nginxwatch.conf.example /etc/nginxwatch/nginxwatch.conf
sudoedit /etc/nginxwatch/nginxwatch.conf
```

Set `nginx_dir` and email settings. Keep `log_file = nginxwatch.log` to write into the service working directory. Verify that the service user can read the enabled-site files and their symlink targets. Grant access to the needed files or deploy a readable copy of just the site definitions; avoid making private TLS keys readable.

## 2. Set the SMTP password (if using email)

```bash
sudo install -m 600 /dev/null /etc/nginxwatch/secret.env
sudoedit /etc/nginxwatch/secret.env
```

Add a single line (systemd environment-file syntax, no `export`):

```ini
SMTP_PASS=your-provider-password
```

systemd reads this root-owned file before starting the service as `nginxwatch`. The environment file is optional when email is disabled. Never commit real credentials.

## 3. Install and start the service

From the project folder:

```bash
sudo install -m 644 deploy/nginxwatch.service /etc/systemd/system/nginxwatch.service
sudo systemctl daemon-reload
sudo systemctl enable --now nginxwatch
sudo systemctl status nginxwatch
sudo journalctl -u nginxwatch -n 50 --no-pager
```

The service creates `/var/lib/nginxwatch` through `StateDirectory`. It restarts after an error and stops gracefully on SIGTERM, after any current network request completes. HTTP requests can take `timeout_sec` each; SMTP requests can take up to 30 seconds.

Check logs and verify a controlled outage and recovery before leaving it unattended. For a one-time SMTP test, run the executable with `--test-email` and the configured `SMTP_PASS` from a private shell; the service always runs in continuous mode.

## Update or remove

After building a new version:

```bash
sudo systemctl stop nginxwatch
sudo install -m 755 build/nginxwatch /opt/nginxwatch/nginxwatch
sudo systemctl start nginxwatch
```

After changing settings, run `sudo systemctl restart nginxwatch`. To stop automatic monitoring, run `sudo systemctl disable --now nginxwatch`. Logs are append-only; configure logrotate for long-running deployments.

## Monitor from another machine

Copy complete nginx site definitions into a readable folder on that machine. Set `nginx_dir` to that folder, `check_local = 0`, and `check_public = 1`. Sync copies whenever nginx configuration changes. This mode does not discover remote nginx configuration automatically.
