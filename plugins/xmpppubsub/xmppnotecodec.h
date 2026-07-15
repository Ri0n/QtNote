#ifndef XMPPNOTECODEC_H
#define XMPPNOTECODEC_H

#include "xmppdto.h"

#include "secureenvelope.h"

namespace QtNote {

/**
 * @brief Stateless authenticated codec for note index and content payloads.
 *
 * Index metadata and content are encrypted independently so note lists can be
 * loaded without fetching every body. The node name and item metadata are
 * bound as authenticated data, preventing ciphertext from being moved between
 * nodes or payload kinds unnoticed.
 */
class XmppNoteCodec {
public:
    static CryptoResult<XmppEncryptedPayload> encodeIndex(const XmppRemoteNote &note, const QByteArray &masterKey,
                                                          const QString &nodeName);
    static CryptoResult<XmppEncryptedPayload> encodeContent(const XmppRemoteNote &note, const QByteArray &masterKey,
                                                            const QString &nodeName);
    static CryptoResult<XmppRemoteNote> decodeIndex(const XmppEncryptedPayload &payload, const QByteArray &masterKey,
                                                    const QString &nodeName);
    static CryptoResult<QString>        decodeContent(const XmppEncryptedPayload &payload, const QByteArray &masterKey,
                                                      const QString &nodeName, const XmppRemoteNote &index);
};

} // namespace QtNote

#endif // XMPPNOTECODEC_H
