# QtNote XMPP PubSub storage — first version

Copy this directory to `QtNote/plugins/xmpppubsub` and add `xmpppubsub` to
`plugins_list` in `plugins/CMakeLists.txt`.

## Build dependency

The plugin targets QXmpp 1.11 or newer and links `QXmpp::QXmpp`.

## Storage model

- One private PEP node per account, by default `urn:xmpp:qtnote:notes:0`.
- Each installation gets a stable UUID and a default resource such as
  `QtNote-1a2b3c4d`, so two QtNote clients can stay connected simultaneously.
- One persistent PubSub item per note.
- The PubSub item ID is the QtNote note ID and is generated as a UUID.
- Re-publishing the same item updates the note.
- A PubSub retract deletes the note and requests a notification for other resources.
- The node is configured with `access_model=whitelist`/QXmpp `Allowlist`,
  `persist_items=true`, `max_items=max`, payload delivery, and retract notifications.
- The plugin refuses to store notes when the account does not advertise the
  PubSub `publish-options` feature required by XEP-0223.

## Payload schema

```xml
<item id="note-uuid">
  <note xmlns="urn:xmpp:qtnote:note:0"
        schema="1"
        revision="revision-uuid"
        parent-revision="previous-revision-uuid"
        origin="installation-uuid"
        modified="2026-07-11T01:30:00.000Z"
        format="markdown">
    <plaintext>
      <title>Title</title>
      <content>Markdown text</content>
    </plaintext>
  </note>
</item>
```

The outer revision envelope is intentionally independent from the content
container. A future E2E version can replace `<plaintext>` with an `<encrypted>`
container while retaining note IDs, revision chains and PubSub behavior.
This first version rejects encrypted or otherwise unknown content containers
instead of treating their content as empty.

A more detailed migration and key-management outline is in `E2E_FUTURE.md`.

## Conflict behavior

Before updating an existing item the client fetches the current server item and
compares its revision with the revision that QtNote loaded. A mismatch aborts the
publish, preserves the local text and reports a conflict.

This is optimistic conflict detection, not an atomic compare-and-swap: XEP-0060
does not provide an `If-Match` equivalent. A narrow race remains between the
revision check and publication. `parent-revision` and `origin` are stored so a
future version can keep version history and perform a three-way merge.

## Events and reconnects

The extension advertises `<node>+notify` and handles item publication, retract,
purge and node deletion events. Private-data events are accepted only when the
PubSub service JID is empty or matches the configured account's bare JID.
A full list refresh is expected after reconnect or node invalidation.

## Current limitations

- No end-to-end encryption yet.
- No persistent local cache or offline write queue.
- No automatic merge UI.
- The legacy synchronous `NoteStorage` methods wait for operations performed by
  a QXmpp client living in a dedicated worker thread.
- QXmpp exposes a continuation marker for paginated item responses but no public
  continuation overload. The plugin therefore first discovers all item IDs and
  requests them in batches. If a server rejects item-ID discovery, it falls back
  to one item page and reports when that page is partial.
- The XMPP password is stored in `QSettings`; a release version should use
  QtKeychain or the platform secret store.
- TLS is required and certificate errors are not ignored.

## Compatibility policy

The implementation is intentionally strict about privacy. It refuses to use a
server that does not advertise PEP plus `publish-options`, and it verifies that
the resulting node is persistent and allowlist-only. Missing nodes are created;
other configuration errors are reported instead of being treated as a missing
node.

## Validation performed

The code and method signatures were checked against the public QXmpp 1.11.3
headers and the current public QtNote plugin interfaces. The archive was not
fully compiled in the generation environment because Qt and QXmpp development
packages were not available there.

## Suggested first test

Use a test XMPP account and verify in this order:

1. The plugin connects and creates/configures the PEP node.
2. Create one note and restart QtNote.
3. Edit it from a second QtNote instance using another XMPP resource.
4. Verify publication events appear in the first instance.
5. Delete the note and verify retract propagation.
6. Open the same revision on both instances, save on the first, then save on the
   second and verify that the second save is rejected as a conflict.

## v2 build fix

`QXmppError` is now included explicitly in `xmppworker.h`. The previous
declaration `const class QXmppError &` was inside `namespace QtNote` and
therefore accidentally declared `QtNote::QXmppError` instead of referring
to the global QXmpp library type.

## v3 QXmpp discovery API compatibility

QXmpp 1.12 and newer use `QXmppDiscoveryManager::info()`.
QXmpp 1.11 keeps using `requestDiscoInfo()`. Selection is made at
compile time with `QXMPP_VERSION`.


## v4 QXmpp 1.11 typed PEP retrieval workaround

QXmpp 1.11 convenience methods `requestOwnPepItems<T>()` and
`requestOwnPepItem<T>()` do not forward `T` to the underlying typed request.
The plugin now calls `requestItems<T>()` and `requestItem<T>()` directly with
our own bare JID. The CMake package is also selected as `QXmppQt5` or
`QXmppQt6` through `QXmppQt${QT_VERSION_MAJOR}`.
