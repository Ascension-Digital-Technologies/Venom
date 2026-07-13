# Installing Venom releases

Venom binary releases include a verifier and a cross-platform installer. The installer verifies the release before changing the installation directory, copies the complete release into a versioned location, installs the `venom` command atomically, and writes an uninstall receipt.

## Linux and macOS

```bash
python3 tools/install_release.py install venom-v1.13.0-linux-x86_64.zip
```

Default locations:

```text
~/.local/share/venom/releases/1.13.0/
~/.local/bin/venom
```

Ensure `~/.local/bin` is on `PATH`.

## Windows

```powershell
python tools\install_release.py install venom-v1.13.0-windows-amd64.zip
```

The default root is `%LOCALAPPDATA%\Venom`. The command is installed under `%LOCALAPPDATA%\Venom\bin`; add that directory to the user `PATH` when needed.

## Require a signed stable release

```bash
python3 tools/install_release.py install venom-release.zip \
  --require-signature \
  --public-key venom-release-public.pem
```

Installation stops before filesystem changes when verification fails.

## Custom locations

```bash
python3 tools/install_release.py install venom-release.zip \
  --prefix /opt/venom \
  --bin-dir /usr/local/bin
```

System paths may require elevated permissions. Prefer a user installation unless Venom is managed by a system package.

## Status

```bash
python3 tools/install_release.py status
python3 tools/install_release.py status --format json
```

The command checks that the recorded release directory and command entry point still exist.

## Upgrade or repair

Install a newer release normally. Reinstall the same version with:

```bash
python3 tools/install_release.py install venom-release.zip --force
```

The versioned release, command, and receipt are replaced atomically where the platform permits.

## Uninstall

```bash
python3 tools/install_release.py uninstall
```

Keep the versioned release while removing the active command and receipt:

```bash
python3 tools/install_release.py uninstall --keep-releases
```

The uninstaller removes only paths recorded in the receipt and refuses to delete a release directory outside the configured prefix.

## Security behavior

The installer:

- runs Venom's release verifier before installation;
- requires SBOM and provenance metadata;
- can require an Ed25519 signature and trusted public key;
- rejects ZIP and TAR path traversal;
- rejects duplicate archive entries;
- rejects TAR symbolic and hard links;
- installs into versioned directories;
- records installed paths for bounded uninstallation.

Verification establishes consistency with the selected verifier and trusted key. It does not prevent later local modification by an administrator or the current user.
