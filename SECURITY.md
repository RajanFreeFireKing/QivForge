# QivForge Security Policy (SECURITY.md)

This security policy defines the vulnerability reporting, classification, and remediation process for QivForge per Doc 14 Section 11 and Section 14.

---

## 1. Supported Versions

Security updates are actively maintained for the current major/minor release:

| Version | Supported |
|---|:---:|
| 1.0.x (Current) | Yes |
| < 1.0 | No |

---

## 2. Reporting a Vulnerability

We take the security of QivForge, its mobile sandbox, and creator project credentials very seriously. If you discover a security vulnerability, please do NOT create a public issue on GitHub or public forums.

Instead, report it responsibly:
- **Email**: `security@qivora.io`
- **PGP Key**: Fingerprint available on keyserver or upon request.
- **Report Details**:
  - Detailed description of the vulnerability.
  - Subsystem affected (`engine/core/vault`, `engine/ai`, `engine/export`, `engine/script/qvm`, etc.).
  - Step-by-step reproduction instructions or proof-of-concept payload.
  - Impact assessment (e.g. credential disclosure, path traversal, sandbox escape).

---

## 3. Vulnerability Response Timeline Targets (Doc 14 §11)

- **Initial Acknowledgement**: Within 24 hours.
- **Triage & Severity Assessment**: Within 48 hours.
- **Remediation & Patch Target**:
  - **Critical (CVSS 9.0 - 10.0)**: Patched and released within **7 calendar days**.
  - **High (CVSS 7.0 - 8.9)**: Patched within 14 calendar days.
  - **Medium / Low**: Included in the next regular maintenance update.

---

## 4. Vault & Credential Incident Procedure (Doc 14 §14)

If a confirmed vulnerability could expose Android Keystore-wrapped credentials or Vault tokens (Hugging Face tokens, Google Gemini API keys, Groq/OpenRouter keys):
1. A security patch will be tagged and distributed immediately.
2. In-app release notes and security advisories will instruct all users to immediately revoke and rotate their affected provider tokens.
3. The **"Delete all local data"** setting in Settings will be highlighted to allow creators to scrub local caches and encrypted stores completely.

---

## 5. Security Architecture Commitments

- **No Root Required**: QivForge operates strictly within the standard Android application sandbox.
- **No Broad Storage**: `MANAGE_EXTERNAL_STORAGE` is strictly forbidden; only Storage Access Framework (SAF) is used (Doc 14 §4).
- **No Dangerous Permissions**: Camera, microphone, and location are not requested.
- **Zero Silent Network Egress**: Network access is restricted exclusively to user-initiated Hugging Face sync, cloud streaming, or AI requests. All egress is logged in the debug console.
