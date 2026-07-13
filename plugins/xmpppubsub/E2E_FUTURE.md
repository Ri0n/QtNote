# E2E follow-up work

Application-level end-to-end encryption, OMEMO-protected storage-key transfer,
encrypted local drafts, and canonical-key migration are implemented. The old
pre-E2E design proposal that used to live in this file is therefore obsolete.

The current design and protocol flows are documented in [README.md](README.md).
Remaining security work includes moving the XMPP password from `QSettings` to
the platform keychain, adding merge/history support, and implementing the
`XmppBackend` contract with Iris.
