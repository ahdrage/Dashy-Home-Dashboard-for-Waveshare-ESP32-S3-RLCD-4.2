# Security policy

## Reporting a vulnerability
Please do not open public issues for sensitive vulnerabilities.

Report privately with:
- affected file(s)
- reproduction steps
- impact
- suggested fix (if available)

## Secret handling
- Never commit `secrets.h`.
- Rotate credentials immediately if exposed.
- Treat Wi-Fi and API tokens as compromised if they appear in git history.
