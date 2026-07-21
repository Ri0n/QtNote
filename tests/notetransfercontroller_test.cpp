#include "notetransfercontroller.h"

#include <QBuffer>
#include <QMimeData>
#include <QTest>

using namespace QtNote;

class NoteTransferControllerTest : public QObject {
    Q_OBJECT

private slots:
    void exportsMultipleFormatsAndRestoresPrivateFragment();
    void importsMarkdownBeforeHtmlAndPlainText();
    void importsTsvAsTable();
    void importsHtmlTableAsTable();
    void importsInlineHtmlLinkWithinParagraph();
    void roundTripsSingleImageAsPng();
    void malformedPrivateFormatFallsBackToPlainText();
};

void NoteTransferControllerTest::exportsMultipleFormatsAndRestoresPrivateFragment()
{
    NoteFragment      fragment;
    NoteFragmentBlock list;
    list.type      = NoteFragmentBlockType::List;
    list.listItems = { { QStringLiteral("**done**"), 0, NoteFragmentListKind::Check, true },
                       { QStringLiteral("nested"), 1, NoteFragmentListKind::Numbered, false } };
    fragment.blocks.append(list);

    NoteTransferController controller;
    const auto             exported = controller.createMimeData(fragment);
    QVERIFY2(exported, qPrintable(exported.error));
    QVERIFY(exported.mimeData->hasFormat(QString::fromLatin1(NoteTransferController::FragmentMimeType)));
    QVERIFY(exported.mimeData->hasFormat(QString::fromLatin1(NoteTransferController::MarkdownMimeType)));
    QVERIFY(exported.mimeData->hasHtml());
    QVERIFY(exported.mimeData->hasText());
    QCOMPARE(QString::fromUtf8(exported.mimeData->data(QString::fromLatin1(NoteTransferController::MarkdownMimeType))),
             QStringLiteral("- [x] **done**\n    1. nested"));

    const auto imported = controller.importMimeData(exported.mimeData.get());
    QVERIFY2(imported, qPrintable(imported.error));
    QCOMPARE(imported.sourceMimeType, QString::fromLatin1(NoteTransferController::FragmentMimeType));
    QCOMPARE(imported.fragment.blocks.at(0).listItems.at(1).kind, NoteFragmentListKind::Numbered);
}

void NoteTransferControllerTest::importsMarkdownBeforeHtmlAndPlainText()
{
    QMimeData mime;
    mime.setText(QStringLiteral("plain"));
    mime.setHtml(QStringLiteral("<p>html</p>"));
    mime.setData(QString::fromLatin1(NoteTransferController::MarkdownMimeType), QByteArrayLiteral("## markdown"));

    NoteTransferController controller;
    const auto             imported = controller.importMimeData(&mime);
    QVERIFY2(imported, qPrintable(imported.error));
    QCOMPARE(imported.sourceMimeType, QString::fromLatin1(NoteTransferController::MarkdownMimeType));
    QCOMPARE(imported.fragment.blocks.at(0).type, NoteFragmentBlockType::Heading);
    QCOMPARE(imported.fragment.blocks.at(0).markdown, QStringLiteral("markdown"));
}

void NoteTransferControllerTest::importsTsvAsTable()
{
    QMimeData mime;
    mime.setData(QString::fromLatin1(NoteTransferController::TsvMimeType), QByteArrayLiteral("A\tB\n1\t2"));

    NoteTransferController controller;
    const auto             imported = controller.importMimeData(&mime);
    QVERIFY2(imported, qPrintable(imported.error));
    QCOMPARE(imported.sourceMimeType, QString::fromLatin1(NoteTransferController::TsvMimeType));
    QCOMPARE(imported.fragment.blocks.size(), 1);
    QCOMPARE(imported.fragment.blocks.at(0).table.rows, 2);
    QCOMPARE(imported.fragment.blocks.at(0).table.columns, 2);
    QCOMPARE(imported.fragment.blocks.at(0).table.markdownCells, QStringList({ "A", "B", "1", "2" }));
}

void NoteTransferControllerTest::importsHtmlTableAsTable()
{
    QMimeData mime;
    mime.setHtml(QStringLiteral("<html><body><table><tr><td>A</td><td>B</td></tr>"
                                "<tr><td>1</td><td>2</td></tr></table></body></html>"));
    QImage preview(1, 1, QImage::Format_ARGB32_Premultiplied);
    preview.fill(Qt::red);
    mime.setImageData(preview);

    NoteTransferController controller;
    const auto             imported = controller.importMimeData(&mime);
    QVERIFY2(imported, qPrintable(imported.error));
    QCOMPARE(imported.sourceMimeType, QStringLiteral("text/html"));
    QCOMPARE(imported.fragment.blocks.size(), 1);
    QCOMPARE(imported.fragment.blocks.at(0).type, NoteFragmentBlockType::Table);
    QCOMPARE(imported.fragment.blocks.at(0).table.rows, 2);
    QCOMPARE(imported.fragment.blocks.at(0).table.columns, 2);
    QCOMPARE(imported.fragment.blocks.at(0).table.markdownCells, QStringList({ "A", "B", "1", "2" }));
}

void NoteTransferControllerTest::importsInlineHtmlLinkWithinParagraph()
{
    QMimeData mime;
    mime.setHtml(QStringLiteral("<p>before <a href=\"https://example.org\">link</a> after</p>"));

    NoteTransferController controller;
    const auto             imported = controller.importMimeData(&mime);
    QVERIFY2(imported, qPrintable(imported.error));
    QCOMPARE(imported.fragment.blocks.size(), 1);
    QString error;
    QCOMPARE(controller.markdownForFragment(imported.fragment, &error),
             QStringLiteral("before [link](https://example.org) after"));
    QVERIFY2(error.isEmpty(), qPrintable(error));
}

void NoteTransferControllerTest::roundTripsSingleImageAsPng()
{
    QImage image(2, 2, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::red);
    QBuffer buffer;
    QVERIFY(buffer.open(QIODevice::WriteOnly));
    QVERIFY(image.save(&buffer, "PNG"));

    NoteFragment      fragment;
    NoteFragmentBlock block;
    block.type            = NoteFragmentBlockType::Image;
    block.image.sourceUri = QStringLiteral("qtnote-media:/11111111-1111-1111-1111-111111111111/image.png");
    fragment.blocks.append(block);
    NoteFragmentMedia media;
    media.sourceUri              = block.image.sourceUri;
    media.reference.id           = QUuid(QStringLiteral("{11111111-1111-1111-1111-111111111111}"));
    media.reference.blobId       = QByteArray::fromHex("abcd");
    media.reference.portableName = QStringLiteral("image.png");
    media.reference.originalName = QStringLiteral("image.png");
    media.reference.mediaType    = QStringLiteral("image/png");
    media.reference.size         = buffer.data().size();
    media.data                   = buffer.data();
    fragment.media.append(media);

    // An unrelated entry must not become the copied image merely because it
    // appears first in the payload.
    NoteFragmentMedia unrelated = media;
    unrelated.sourceUri         = QStringLiteral("qtnote-media:/22222222-2222-2222-2222-222222222222/other.png");
    unrelated.reference.id      = QUuid(QStringLiteral("{22222222-2222-2222-2222-222222222222}"));
    unrelated.data              = QByteArrayLiteral("not an image");
    fragment.media.prepend(unrelated);

    NoteTransferController controller;
    const auto             exported = controller.createMimeData(fragment);
    QVERIFY2(exported, qPrintable(exported.error));
    QVERIFY(exported.mimeData->hasImage());
    QVERIFY(exported.mimeData->hasFormat(QStringLiteral("image/png")));
    const auto imported = controller.importMimeData(exported.mimeData.get());
    QVERIFY2(imported, qPrintable(imported.error));
    // The private QtNote representation remains the preferred round-trip.
    QCOMPARE(imported.sourceMimeType, QString::fromLatin1(NoteTransferController::FragmentMimeType));
}

void NoteTransferControllerTest::malformedPrivateFormatFallsBackToPlainText()
{
    QMimeData mime;
    mime.setData(QString::fromLatin1(NoteTransferController::FragmentMimeType), QByteArrayLiteral("bad"));
    mime.setText(QStringLiteral("fallback"));

    NoteTransferController controller;
    const auto             imported = controller.importMimeData(&mime);
    QVERIFY2(imported, qPrintable(imported.error));
    QCOMPARE(imported.sourceMimeType, QStringLiteral("text/plain"));
    QCOMPARE(imported.fragment.blocks.at(0).markdown, QStringLiteral("fallback"));
}

QTEST_MAIN(NoteTransferControllerTest)

#include "notetransfercontroller_test.moc"
