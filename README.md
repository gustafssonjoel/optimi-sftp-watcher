# optimi-sftp-watcher

Tiny polling-based C app that watches a remote SFTP directory and downloads matching changed files locally. This is still in progress and will be finished later.

## Planned behavior

- Load JSON config (host/auth/path/poll/filter/logging)
- Poll remote SFTP listings via libcurl
- Detect create/modify/delete via snapshot diffs
- Apply wildcard include/exclude filters before download
- Mirror remote relative paths under a local download root
- Log startup, polling, retries, diffs, and failures

## Quick run

```bash
make
make test
./bin/optimi-sftp-watcher ./config/example.json
```
