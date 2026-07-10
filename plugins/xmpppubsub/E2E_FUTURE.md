# Future E2E encryption plan

The first storage version deliberately keeps cryptography out of the transport
implementation, but the payload envelope is designed so encryption can be added
without changing PubSub item IDs or the `NoteStorage` integration.

## Stable outer envelope

The following fields remain outside the encrypted container so clients can
synchronize and detect conflicts before decrypting content:

- PubSub item ID: stable note UUID;
- schema version;
- revision UUID;
- parent revision UUID;
- originating installation UUID;
- modification timestamp;
- content format identifier.

The server will therefore still see item identifiers, revision relationships,
client activity and timestamps. E2E encryption protects note title and content,
not all metadata.

## Proposed encrypted container

```xml
<item id="note-uuid">
  <note xmlns="urn:xmpp:qtnote:note:0"
        schema="1"
        revision="revision-uuid"
        parent-revision="previous-revision-uuid"
        origin="installation-uuid"
        modified="2026-07-11T01:30:00.000Z"
        format="markdown">
    <encrypted xmlns="urn:xmpp:qtnote:encrypted:0"
               algorithm="xchacha20-poly1305"
               key-id="account-key-version">
      <nonce>base64</nonce>
      <ciphertext>base64</ciphertext>
    </encrypted>
  </note>
</item>
```

The plaintext serialized inside the authenticated ciphertext should contain at
least the title and note body. A compact deterministic representation such as
canonical CBOR is preferable to encrypting arbitrary XML.

## Authenticated associated data

The AEAD associated data should bind ciphertext to its storage context and
revision envelope. At minimum include:

- PEP node name;
- PubSub item ID;
- payload namespace and schema;
- revision and parent revision;
- modification timestamp;
- format identifier.

This prevents copying valid ciphertext between notes or revisions without
detection.

## Key management boundary

Content encryption and device key distribution should be separate components.
A practical design is:

1. Generate a random account master key.
2. Encrypt note payloads with versioned content keys derived from or wrapped by
   that master key.
3. Wrap the master key independently for each trusted device.
4. Store only wrapped key material in a separate private PEP node.
5. Support key rotation without changing note IDs.

QXmpp's OMEMO facilities may help authenticate devices and transport wrapped key
material, but note payload encryption should remain application-level so it is
independent of whether PubSub IQ stanzas themselves support an E2E extension.

## Migration rule

An unencrypted client must never interpret an unknown or encrypted content
container as an empty note. The first-version parser already rejects anything
other than `<plaintext>`, which makes mixed-version operation fail safely rather
than destroying encrypted content.
